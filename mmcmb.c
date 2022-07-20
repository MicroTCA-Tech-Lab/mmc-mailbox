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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmcmb/fpga_mailbox_layout.h"

#ifndef MB_ADAPTER_DT_NAME
#define MB_ADAPTER_DT_NAME "iic_axi_iic_mmc"
#endif

#ifndef MB_I2C_ADDR
#define MB_I2C_ADDR "002a"
#endif

static int fd = -1;

#define SYSFS_ADAPTERS "/sys/class/i2c-adapter"
#define I2CDIR_PREFIX "i2c-"
#define I2CDIR_PREFIX_LEN (sizeof(I2CDIR_PREFIX) - 1)

static char adapter_name[80];
static char bus_name[20];
static char eeprom_path[120];

static bool get_i2c_adapter(const char* adapter_dt_name)
{
    DIR* d = opendir(SYSFS_ADAPTERS);
    if (!d) {
        fprintf(stderr, "Could not list %s: %s\n", SYSFS_ADAPTERS, strerror(errno));
        return false;
    }

    bool found = false;
    struct dirent* dir;

    while (!found && (dir = readdir(d)) != NULL) {
        // Can't check for DT_DIR as these pseudo-file dirs are not actual directories
        // Just check if they begin with "i2c-"
        if (strncmp(dir->d_name, I2CDIR_PREFIX, I2CDIR_PREFIX_LEN)) {
            continue;
        }

        // "i2c-" dir found, check adapter name
        char namepath[28 + sizeof(dir->d_name)];  // avoid -Wformat-truncation
        snprintf(namepath, sizeof(namepath), SYSFS_ADAPTERS "/%s/name", dir->d_name);
        int name_fd = open(namepath, O_RDONLY);
        if (name_fd < 0) {
            continue;
        }
        int name_len = read(name_fd, adapter_name, sizeof(adapter_name) - 1);
        close(name_fd);
        if (name_len <= 0) {
            continue;
        }
        adapter_name[name_len] = '\0';
        adapter_name[strcspn(adapter_name, "\n")] = 0;

        if (strstr(adapter_name, adapter_dt_name)) {
            found = true;
            strncpy(bus_name, dir->d_name, sizeof(bus_name));
        }
    }
    closedir(d);

    return found;
}

static bool mb_open(void)
{
    if (!get_i2c_adapter(MB_ADAPTER_DT_NAME)) {
        fprintf(stderr, "No adapter '%s' found\n", MB_ADAPTER_DT_NAME);
        return false;
    }

    snprintf(eeprom_path,
             sizeof(eeprom_path),
             SYSFS_ADAPTERS "/%s/%s-%s/eeprom",
             bus_name,
             bus_name + I2CDIR_PREFIX_LEN,
             MB_I2C_ADDR);

    fd = open(eeprom_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", eeprom_path, strerror(errno));
        return false;
    }
    return true;
}

static bool mb_read_at(size_t offs, void* buf, size_t n)
{
    if (!mb_open()) {
        return false;
    }

    ssize_t n_read = pread(fd, buf, n, offs);
    if (n_read != n) {
        perror("read error");
        return false;
    }
    return true;
}

const char* mb_get_adapter_name(void)
{
    return mb_open() ? adapter_name : NULL;
}

const char* mb_get_bus_name(void)
{
    return mb_open() ? bus_name : NULL;
}

const char* mb_get_eeprom_path(void)
{
    return mb_open() ? eeprom_path : NULL;
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

bool mb_get_fpga_ctrl(mb_fpga_ctrl_t* ctrl)
{
    return mb_read_at(MB_EEPROM_OFFS(fpga_ctrl), ctrl, sizeof(mb_fpga_ctrl_t));
}
