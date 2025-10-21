/***************************************************************************
 *      ____  _____________  __    __  __ _           _____ ___   _        *
 *     / __ \/ ____/ ___/\ \/ /   |  \/  (_)__ _ _ __|_   _/ __| /_\  (R)  *
 *    / / / / __/  \__ \  \  /    | |\/| | / _| '_/ _ \| || (__ / _ \      *
 *   / /_/ / /___ ___/ /  / /     |_|  |_|_\__|_| \___/|_| \___/_/ \_\     *
 *  /_____/_____//____/  /_/      T  E  C  H  N  O  L  O  G  Y   L A B     *
 *                                                                         *
 *          Copyright 2022 Deutsches Elektronen-Synchrotron DESY.          *
 *                          All rights reserved.                           *
 *                                                                         *
 ***************************************************************************/

#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#ifdef ENABLE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include "mmcmb/fpga_mailbox_layout.h"
#include "mmcmb/mmcmb.h"

// Poll FPGA control register 4 times per second.
#define POLL_INTERVAL_MS 250

static bool terminate = false;

static void sigterm_handler(int signum)
{
#ifdef ENABLE_SYSTEMD
	sd_notify(0, "STOPPING=1\n");
#endif
    (void)signum;
    terminate = true;
}

#ifndef ENABLE_SYSTEMD
static void daemonize()
{
    pid_t pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    struct sigaction action = {
        .sa_handler = SIG_IGN,
    };
    sigaction(SIGCHLD, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    if (chdir("/") < 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    openlog("mmcctrld", LOG_PID, LOG_DAEMON);
}
#endif

void handle_fpga_ctrl(const mb_fpga_ctrl_t* ctrl)
{
    if (!ctrl->req_shutdown) {
        return;
    }
    syslog(LOG_NOTICE, "Shutdown requested by MMC");

    execl("/sbin/shutdown", "shutdown", "-h", "now", (char*)NULL);

    syslog(LOG_ERR, "Could not execute shutdown command: %s", strerror(errno));
    terminate = true;
}

mb_nic_information_t get_nic_info(const char* ifname)
{
    mb_nic_information_t result = {0};

    // Get MAC address via ioctl
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd > 0) {
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
            const uint8_t* mac_addr = (const uint8_t*)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
            memcpy(result.mac_addr, mac_addr, sizeof(result.mac_addr));
        } else {
            syslog(LOG_ERR, "Could not get MAC address of %s: %s", ifname, strerror(errno));
        }
        close(fd);
    } else {
        syslog(LOG_ERR, "Error: socket(): %s", strerror(errno));
    }

    // Get primary IPv4 & IPv6 addresses via getifaddrs()
    struct ifaddrs *ifap, *ifa;
    if (getifaddrs(&ifap) < 0) {
        syslog(LOG_ERR, "Error: getifaddrs(): %s", strerror(errno));
        goto out;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && !strcmp(ifa->ifa_name, ifname)) {
            switch (ifa->ifa_addr->sa_family) {
                case AF_INET: {
                    const struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
                    memcpy(result.ipv4_addr, &sa->sin_addr, sizeof(result.ipv4_addr));
                } break;
                case AF_INET6: {
                    const struct sockaddr_in6* sa = (struct sockaddr_in6*)ifa->ifa_addr;
                    memcpy(result.ipv6_addr, sa->sin6_addr.s6_addr, sizeof(result.ipv6_addr));
                } break;
                default:
                    break;
            }
        }
    }
    freeifaddrs(ifap);

out:
    return result;
}

int main()
{
    if (geteuid() != 0) {
        fprintf(stderr, "mmcctrld: needs to be launched with root privileges\r\n");
        return EXIT_FAILURE;
    }

#ifndef ENABLE_SYSTEMD
    daemonize();
#endif

    const char* eeprom = mb_get_eeprom_path();
    if (eeprom != NULL) {
        syslog(LOG_NOTICE, "Opened mailbox at %s", eeprom);
    } else {
        syslog(LOG_ERR, "Could not open mailbox");
        goto finish;
    }
    if (!mb_check_magic()) {
        syslog(LOG_ERR, "Mailbox not available");
        goto finish;
    }

    const mb_fpga_status_t stat = {
        .app_startup_finished = true,
    };
    if (!mb_set_fpga_status(&stat)) {
        syslog(LOG_ERR, "Could not set FPGA status");
        goto finish;
    }

    const char* bp_eth_ifname = getenv("BP_ETH_IFNAME");
    if (!bp_eth_ifname) {
        bp_eth_ifname = "eth0";
    }

    const struct timespec ts_poll = {
        .tv_nsec = POLL_INTERVAL_MS * 1e6,
    };

    syslog(LOG_NOTICE, "Started");

#ifdef ENABLE_SYSTEMD
    sd_notify(0, "READY=1");
#endif

    struct sigaction action = {
        .sa_handler = SIG_IGN,
    };
    action.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &action, NULL);

    while (!terminate) {
        mb_fpga_ctrl_t ctrl;
        if (!mb_get_fpga_ctrl(&ctrl)) {
            syslog(LOG_ERR, "Could not read FPGA_CTRL");
            break;
        }
        handle_fpga_ctrl(&ctrl);

        mb_nic_information_t nic_info = get_nic_info(bp_eth_ifname);
        mb_set_bp_eth_info(&nic_info);

        nanosleep(&ts_poll, NULL);
    }

finish:
    syslog(LOG_NOTICE, "Terminated");
    closelog();

    return EXIT_SUCCESS;
}
