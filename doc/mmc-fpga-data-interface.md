# MMC - FPGA Data Interface ("EEPROM" Contents)

<br>

## Memory Map

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 256  | [FRU Information](#fru-information) | FRU #0 Information       | AMC Information                                                                                                                                 |
|                 | 256    | 256  | [FRU Information](#fru-information) | FRU #1 Information       | RTM Information                                                                                                                                 |
|                 | 512    | 256  | [FRU Information](#fru-information) | FRU #2 Information       | FMC1 Information                                                                                                                                |
|                 | 768    | 256  | [FRU Information](#fru-information) | FRU #3 Information       | FMC2 Information                                                                                                                                |
|                 | 1024   | 256  | None            | Application-specific     | This area is not used by libdmmc. It is reserved for board-specific or application-specific implementation.                                     |
|                 | 1280   | 48   | [MMC Information](#mmc-information) | Information about MMC    |                                                                                                                                                 |
|                 | 1328   | 640  | [MMC Sensor](#mmc-sensor) [] | Sensor #0..#39           | All MMC sensors                                                                                                                                 |
|                 | 1968   | 77   | None            | Reserved                 |                                                                                                                                                 |
|                 | 2045   | 1    | Bitfield        | FPGA Control             | Bit 2..7: Reserved<br>Bit 1: Request PCIe reset<br>Bit 0: Request OS / Application Shutdown                                                     |
|                 | 2046   | 1    | Bitfield        | FPGA Status              | Bit 3..7: Reserved<br>Bit 2: OS / Application Shutdown Finished<br>Bit 1: OS / Application Failure<br>Bit 0: OS / Application Startup Finished  |
|                 | 2047   | 1    | Bitfield        | Lock Register            | Bit 1..7: Reserved<br>Bit 0: Lock against update by MMC                                                                                         |
| Size:           | 2048   |      |                 |                          | Note: Only "Lock Register" & "FPGA Status" written by FPGA. Everything else written by MMC                                                      |

<br>

## FRU Information

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 20   | [FRU Status](#fru-status) | Status                   | Dynamic FRU information                                                                                                                         |
|                 | 20     | 236  | [FRU Description](#fru-description) | Description              | Static FRU information                                                                                                                          |
| Size:           | 256    |      |                 |                          |                                                                                                                                                 |

<br>

## FRU Status

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 1    | Bitfield        | Status                   | Bit 4..7: Reserved<br>Bit 3: FRU Failure<br>Bit 2: FRU Payload Power Active<br>Bit 1: FRU Compatible<br>Bit 0: FRU Present                      |
|                 | 1      | 1    | u8              | Num. Temperature Sensors | 0..8                                                                                                                                            |
|                 | 2      | 16   | s16 []          | Temperature #1..#8       | Deg.C in 0.01° increments, 0x7fff if N/A or read error                                                                                          |
|                 | 18     | 2    | None            | Reserved                 |                                                                                                                                                 |
| Size:           | 20     |      |                 |                          |                                                                                                                                                 |

<br>

## FRU Description

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 6    | u8 []           | UID                      | Unique ID (0 if N/A)                                                                                                                            |
|                 | 6      | 60   | char []         | Manufacturer             | FRU record: Manufacturer name                                                                                                                   |
|                 | 66     | 60   | char []         | Product                  | FRU record: Product name                                                                                                                        |
|                 | 126    | 60   | char []         | Part number              | FRU record: Part number string                                                                                                                  |
|                 | 186    | 30   | char []         | Serial number            | FRU record: Serial number string                                                                                                                |
|                 | 216    | 20   | char []         | Version                  | FRU record: Version string                                                                                                                      |
| Size:           | 236    |      |                 |                          |                                                                                                                                                 |

<br>

## MMC Sensor

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 12   | char []         | Sensor name              | First 12 characters of sensor name as in IPMI sensor record                                                                                     |
|                 | 12     | 4    | float           | Sensor reading           | Last sensor reading (update at 1Hz), NaN if N/A or read error                                                                                   |
| Size:           | 16     |      |                 |                          |                                                                                                                                                 |

<br>

## MMC Information

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 2    | [Version Number](#version-number) | Application version      | MMC application version                                                                                                                         |
|                 | 2      | 2    | [Version Number](#version-number) | Library version          | libdmmc version                                                                                                                                 |
|                 | 4      | 2    | [Version Number](#version-number) | CPLD version             | STAMP CPLD version                                                                                                                              |
|                 | 6      | 1    | char            | STAMP HW Revision        | "A"..."Z"                                                                                                                                       |
|                 | 7      | 1    | u8              | AMC Slot Number          | 1...n (0 = Out of crate)                                                                                                                        |
|                 | 8      | 1    | u8              | AMC IPMB Address         | 0x72...n (0 = Out of crate)                                                                                                                     |
|                 | 9      | 23   | char []         | Board Name               | e.g. "DAMC-FMC2ZUP"                                                                                                                             |
|                 | 32     | 2    | u16             | Vendor ID                | Vendor ID as in IPMI GET_DEVICE_ID                                                                                                              |
|                 | 34     | 2    | u16             | Product ID               | Product ID as in IPMI GET_DEVICE_ID                                                                                                             |
|                 | 36     | 4    | u32             | MMC Uptime               | Seconds since last MMC boot                                                                                                                     |
|                 | 40     | 8    | None            | Reserved                 |                                                                                                                                                 |
| Size:           | 48     |      |                 |                          |                                                                                                                                                 |

<br>

## Version Number

|                 | Offset | Size | Type            | Name                     | Contents                                                                                                                                        |
|:----------------|:-------|:-----|:----------------|:-------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------|
|                 | 0      | 1    | u8              | Major version            | Major version (before dot)                                                                                                                      |
|                 | 1      | 1    | u8              | Minor version            | Minor version (after dot)                                                                                                                       |
| Size:           | 2      |      |                 |                          |                                                                                                                                                 |