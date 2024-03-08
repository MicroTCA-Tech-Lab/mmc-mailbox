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

#include "mmcmb/mmcmb.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmcmb/fpga_mailbox_layout.h"

#ifndef MB_DT_COMPAT_ID
#define MB_DT_COMPAT_ID "desy,mmcmailbox"
#endif

#define SYSFS_DEVICES "/sys/bus/i2c/devices"
#define NODE_COMPATIBLE "of_node/compatible"
#define I2CDIR_PREFIX "i2c-"
#define I2CDIR_PREFIX_LEN (sizeof(I2CDIR_PREFIX) - 1)

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

static char eeprom_path[290] = {0};

// Use separate fd's for read & write, so non-root users can do reads
static int fd_rdonly = -1;
static int fd_wronly = -1;

static char* get_compatible_eeprom(const char* dt_compat_id)
{
    if (eeprom_path[0] != '\0') {
        return eeprom_path;
    }

    DIR* d = opendir(SYSFS_DEVICES);
    if (!d) {
        fprintf(stderr, "Could not list %s: %s\n", SYSFS_DEVICES, strerror(errno));
        return false;
    }

    bool found = false;
    struct dirent* dir;

    while (!found && (dir = readdir(d)) != NULL) {
        // Can't check for DT_DIR as these pseudo-file dirs are not actual directories
        // Skip entries beginnig with "." or "i2c-"
        if (dir->d_name[0] == '.' || !strncmp(dir->d_name, I2CDIR_PREFIX, I2CDIR_PREFIX_LEN)) {
            continue;
        }

        // Sysfs directory for I2C peripheral found, check adapter name
        char comp_id_path[40 + sizeof(dir->d_name)];  // avoid -Wformat-truncation
        snprintf(comp_id_path,
                 sizeof(comp_id_path),
                 SYSFS_DEVICES "/%s/" NODE_COMPATIBLE,
                 dir->d_name);

        int comp_id_fd = open(comp_id_path, O_RDONLY);
        if (comp_id_fd < 0) {
            continue;
        }
        char comp_id[80];
        int comp_len = read(comp_id_fd, comp_id, sizeof(comp_id) - 1);
        close(comp_id_fd);
        if (comp_len <= 0) {
            continue;
        }
        // DT compat. IDs are zero-terminated, but let's still terminate it just in case
        comp_id[comp_len] = '\0';

        if (!strcmp(comp_id, dt_compat_id)) {
            found = true;
            snprintf(eeprom_path, sizeof(eeprom_path), SYSFS_DEVICES "/%s/eeprom", dir->d_name);
        }
    }
    closedir(d);
    return found ? eeprom_path : NULL;
}

static bool mb_open(int* fd, int mode)
{
    if (*fd > 0) {
        return true;
    }

    char* path = get_compatible_eeprom(MB_DT_COMPAT_ID);
    if (!path) {
        fprintf(stderr, "No I2C device compatible to '%s' found\n", MB_DT_COMPAT_ID);
        return false;
    }

    *fd = open(path, mode);
    if (*fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
        return false;
    }
    return true;
}

static bool mb_read_at(size_t offs, void* buf, size_t n)
{
    if (!mb_open(&fd_rdonly, O_RDONLY)) {
        return false;
    }

    ssize_t n_read = pread(fd_rdonly, buf, n, offs);
    if (n_read != (ssize_t)n) {
        perror("read error");
        return false;
    }
    return true;
}

static bool mb_write_at(size_t offs, const void* buf, size_t n)
{
    if (!mb_open(&fd_wronly, O_WRONLY)) {
        return false;
    }

    ssize_t n_write = pwrite(fd_wronly, buf, n, offs);
    if (n_write != (ssize_t)n) {
        perror("write error");
        return false;
    }
    return true;
}

const char* mb_get_eeprom_path(void)
{
    return mb_open(&fd_rdonly, O_RDONLY) ? eeprom_path : NULL;
}

bool mb_check_magic(void)
{
    char magic_str_mb[MB_NUM_ELEMS(mailbox_magic_str)];

    if (!mb_read_at(MB_EEPROM_OFFS(mailbox_magic_str), &magic_str_mb, sizeof(magic_str_mb))) {
        return false;
    }
    return !memcmp(magic_str_mb, MB_MAGIC_STR, sizeof(magic_str_mb));
}

bool mb_get_mmc_information(mb_mmc_information_t* info)
{
    return mb_read_at(MB_EEPROM_OFFS(mmc_information), info, sizeof(mb_mmc_information_t));
}

bool mb_get_mmc_sensors(mb_mmc_sensor_t* sen, size_t first_sensor, size_t n)
{
    if (first_sensor + n > MAX_SENS_MMC) {
        fprintf(stderr, "Sensor index out of range (%zu > %zu)\n", first_sensor + n, MAX_SENS_MMC);
        return false;
    }
    return mb_read_at(MB_EEPROM_OFFS(mmc_sensor[first_sensor]), sen, sizeof(mb_mmc_sensor_t) * n);
}

bool mb_get_fru_description(mb_fru_description_t* desc, size_t fru_id)
{
    if (fru_id >= NUM_FRUS) {
        fprintf(stderr, "FRU index out of range (%zu >= %zu)\n", fru_id, NUM_FRUS);
        return false;
    }
    return mb_read_at(MB_EEPROM_OFFS(fru_information[fru_id].description),
                      desc,
                      sizeof(mb_fru_description_t));
}

bool mb_get_fru_status(mb_fru_status_t* stat, size_t fru_id)
{
    if (fru_id >= NUM_FRUS) {
        fprintf(stderr, "FRU index out of range (%zu >= %zu)\n", fru_id, NUM_FRUS);
        return false;
    }
    return mb_read_at(MB_EEPROM_OFFS(fru_information[fru_id].status),
                      stat,
                      sizeof(mb_fru_status_t));
}

bool mb_get_application_specific_data(void* buf, size_t offs, size_t len)
{
    const size_t d_size = MB_NUM_ELEMS(application_data);
    offs = MIN(offs, d_size);
    len = MIN(len, d_size - offs);
    return mb_read_at(MB_EEPROM_OFFS(application_data[offs]), buf, len);
}

bool mb_get_fpga_ctrl(mb_fpga_ctrl_t* ctrl)
{
    return mb_read_at(MB_EEPROM_OFFS(fpga_ctrl), ctrl, sizeof(*ctrl));
}

bool mb_set_fpga_status(const mb_fpga_status_t* stat)
{
    return mb_write_at(MB_EEPROM_OFFS(fpga_status), stat, sizeof(*stat));
}

bool mb_set_bp_eth_info(const mb_nic_information_t* nic_info)
{
    return mb_write_at(MB_EEPROM_OFFS(bp_eth_info), nic_info, sizeof(*nic_info));
}
