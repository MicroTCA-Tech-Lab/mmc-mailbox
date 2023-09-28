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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "fpga_mailbox_layout.h"

// Functions return true for success

// Check MMC Mailbox magic string
bool mb_check_magic(void);

// Get MMC information
bool mb_get_mmc_information(mb_mmc_information_t* info);

// Get <n> MMC sensors starting at <first_sensor>
bool mb_get_mmc_sensors(mb_mmc_sensor_t* sen, size_t first_sensor, size_t n);

// Get FRU description of <fru_id> (0=AMC, 1=RTM, 2=FMC1, 3=FMC2)
bool mb_get_fru_description(mb_fru_description_t* desc, size_t fru_id);

// Get FRU status of <fru_id> (0=AMC, 1=RTM, 2=FMC1, 3=FMC2)
bool mb_get_fru_status(mb_fru_status_t* stat, size_t fru_id);

// Get application specific data, <len> bytes at <offs> offset into the data block
bool mb_get_application_specific_data(void* buf, size_t offs, size_t len);

// Get FPGA control
bool mb_get_fpga_ctrl(mb_fpga_ctrl_t* ctrl);

// Set FPGA status
bool mb_set_fpga_status(const mb_fpga_status_t* stat);

// Set Backplane NIC addresses
bool mb_set_bp_eth_info(const mb_nic_information_t* nic_info);

// Get mmc-mailbox "EEPROM" device path, returns NULL on error
const char* mb_get_eeprom_path(void);

#ifdef __cplusplus
}
#endif
