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

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#define MB_PACKED __attribute__((packed)) __attribute__((scalar_storage_order("little-endian")))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

/* FPGA Mailbox data types */

#define FRU_TEMP_INVALID 0x7fff
typedef struct mb_fru_status {
    unsigned present : 1;
    unsigned compatible : 1;
    unsigned powered : 1;
    unsigned failure : 1;
    unsigned status_reserved : 4;
    uint8_t num_temp_sensors;
    uint16_t temperature[8];
    union stat_ext {
        uint8_t bytes_reserved[2];
        struct stat_fmc {
            unsigned hspc_prsnt : 1;
            unsigned clk_dir : 1;
            unsigned pg_m2c : 1;
            unsigned reserved_1 : 5;
            uint8_t reserved_2;
        } MB_PACKED fmc;
    } MB_PACKED ext;
} MB_PACKED mb_fru_status_t;

typedef struct mb_fru_description {
    uint8_t uid[6];
    char manufacturer[60];
    char product[60];
    char part_nr[60];
    char serial_nr[30];
    char version[20];
} MB_PACKED mb_fru_description_t;

typedef struct mb_fru_information {
    mb_fru_status_t status;
    mb_fru_description_t description;
} MB_PACKED mb_fru_information_t;

typedef struct mb_version_number {
    uint8_t major;
    uint8_t minor;
} MB_PACKED mb_version_number_t;

typedef struct mb_mmc_information {
    mb_version_number_t application_version;
    mb_version_number_t library_version;
    mb_version_number_t cpld_board_version;
    mb_version_number_t cpld_library_version;
    char stamp_hw_revision;
    uint8_t amc_slot_nr;
    uint8_t ipmb_addr;
    char board_name[23];
    uint16_t vendor_id;
    uint16_t product_id;
    uint32_t mmc_uptime;
    uint8_t reserved[6];
} MB_PACKED mb_mmc_information_t;

typedef struct mb_mmc_sensor {
    char name[12];
    float reading;
} MB_PACKED mb_mmc_sensor_t;

typedef struct mb_fpga_ctrl {
    unsigned req_shutdown : 1;
    unsigned req_pcie_reset : 1;
    unsigned reserved : 6;
} MB_PACKED mb_fpga_ctrl_t;

typedef struct mb_fpga_status {
    unsigned app_startup_finished : 1;
    unsigned app_failure : 1;
    unsigned app_shutdown_finished : 1;
    unsigned reserved : 5;
} MB_PACKED mb_fpga_status_t;

typedef struct mb_nic_information {
    uint8_t mac_addr[6];
    uint8_t ipv4_addr[4];
    uint8_t ipv6_addr[16];
} MB_PACKED mb_nic_information_t;

/* Full memory content, containing the sub-types */

typedef struct mb_memory_contents {
    char mailbox_magic_str[7];
    uint8_t mailbox_version;
    mb_fru_information_t fru_information[4];
    uint8_t application_data[256];
    mb_mmc_information_t mmc_information;
    mb_mmc_sensor_t mmc_sensor[40];
    uint8_t reserved[43];
    mb_nic_information_t bp_eth_info;
    mb_fpga_ctrl_t fpga_ctrl;
    mb_fpga_status_t fpga_status;
} MB_PACKED mb_memory_contents_t;

#pragma GCC diagnostic pop

#define MB_MAGIC_STR "MMCMBOX"

/* Convenience macros */

/* EEPROM offset of a data structure field */
#define MB_MEM_DUMMY ((const mb_memory_contents_t*)NULL)
#define MB_EEPROM_OFFS(x) offsetof(mb_memory_contents_t, x)

/* Array sizes */
#define MB_NUM_ELEMS(x) (sizeof(MB_MEM_DUMMY->x) / sizeof(MB_MEM_DUMMY->x[0]))
#define NUM_FRUS MB_NUM_ELEMS(fru_information)
#define MAX_SENS_PER_FRU MB_NUM_ELEMS(fru_information[0].status.temperature)
#define MAX_SENS_MMC MB_NUM_ELEMS(mmc_sensor)

/* Check MMC mailbox (minus lock register) total size */
static_assert(sizeof(mb_memory_contents_t) == 2047, "Mailbox contents must be 2047 bytes");

/* Check offsets for consistency with memory map, see doc/mmc-fpga-data-interface.md */
static_assert(MB_EEPROM_OFFS(mailbox_magic_str) == 0, "Offset of mailbox_magic_str incorrect");
static_assert(MB_EEPROM_OFFS(mailbox_version) == 7, "Offset of mailbox_version incorrect");
static_assert(MB_EEPROM_OFFS(fru_information[0]) == 8, "Offset of fru_information[0] incorrect");
static_assert(MB_EEPROM_OFFS(fru_information[1]) == 264, "Offset of fru_information[1] incorrect");
static_assert(MB_EEPROM_OFFS(fru_information[2]) == 520, "Offset of fru_information[2] incorrect");
static_assert(MB_EEPROM_OFFS(fru_information[3]) == 776, "Offset of fru_information[3] incorrect");
static_assert(MB_EEPROM_OFFS(application_data) == 1032, "Offset of application_data incorrect");
static_assert(MB_EEPROM_OFFS(mmc_information) == 1288, "Offset of mmc_information incorrect");
static_assert(MB_EEPROM_OFFS(mmc_sensor) == 1336, "Offset of mmc_sensor incorrect");
static_assert(MB_EEPROM_OFFS(reserved) == 1976, "Offset of reserved incorrect");
static_assert(MB_EEPROM_OFFS(bp_eth_info) == 2019, "Offset of NIC information incorrect");
static_assert(MB_EEPROM_OFFS(fpga_ctrl) == 2045, "Offset of fpga_ctrl incorrect");
static_assert(MB_EEPROM_OFFS(fpga_status) == 2046, "Offset of fpga_status incorrect");
