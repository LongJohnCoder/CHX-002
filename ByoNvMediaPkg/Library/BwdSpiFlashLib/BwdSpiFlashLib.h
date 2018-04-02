/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  PchSpiFlashLib.h

Abstract: 

Revision History:

**/
#ifndef _BWD_SPI_FLASH_LIB_H_
#define _BWD_SPI_FLASH_LIB_H_

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Library/Include/SpiFlashLib.h>

//
// Serial Flash Status Register definitions
//
#define SF_SR_BUSY        0x01      // Indicates if internal write operation is in progress
#define SF_SR_WEL         0x02      // Indicates if device is memory write enabled
#define SF_SR_BP0         0x04      // Block protection bit 0
#define SF_SR_BP1         0x08      // Block protection bit 1
#define SF_SR_BP2         0x10      // Block protection bit 2
//#define SF_SR_BP3         0x20      // Block protection bit 3
//#define SF_SR_WPE         0x3C      // Enable write protection on all blocks
#define SF_SR_WPE         0x2C      // Enable write protection on all blocks
//#define SF_SR_AAI         0x40      // Auto Address Increment Programming status
#define SF_SR_BPL         0x80      // Block protection lock-down

//
// Operation Instruction definitions for  the Serial Flash Device
//
//
#define SF_INST_WRSR            0x01     // Write Status Register
#define SF_INST_PROG            0x02     // Byte Program    
#define SF_INST_READ            0x03     // Read
#define SF_INST_WRDI            0x04     // Write Disable
#define SF_INST_RDSR            0x05     // Read Status Register
#define SF_INST_WREN            0x06     // Write Enable
#define SF_INST_HS_READ         0x0B     // High-speed Read 
#define SF_INST_SERASE          0xD8     // Sector Erase (4KB)
//#define SF_INST_BERASE          0x52     // Block Erase (32KB)
#define SF_INST_64KB_ERASE      0xC7     // Block Erase (64KB)
#define SF_INST_EWSR            0x00     // Enable Write Status Register      
#define SF_INST_READ_ID         0x9F     // Read ID
#define SF_INST_JEDEC_READ_ID   0xAB     // JEDEC Read ID

//
// Prefix Opcode Index on the host SPI controller
//
typedef enum {
    SPI_WREN,             // Prefix Opcode 0: Write Enable
    SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller
//
typedef enum {
    SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address
    SPI_READ,           // Opcode 1: READ, Read cycle with address
    SPI_RDSR,           // Opcode 2: Read Status Register, No address
    SPI_WRDI,           // Opcode 3: Write Disable, No address
    SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address
    SPI_64KB_ERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address
    SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address
    SPI_WRSR,           // Opcode 7: Write Status Register, No address
} SPI_OPCODE_INDEX;

typedef struct _SPI_OPCODE_MENU_ENTRY_TPT {
    SPI_OPCODE_TYPE     Type;
    UINT8               Code;
    SPI_CYCLE_FREQUENCY Frequency;
 // SPI_OPERATION       Operation;
} SPI_OPCODE_MENU_ENTRY_TPT;

//
// Initialization data table loaded to the SPI host controller
//    VendorId        Vendor ID of the SPI device
//    DeviceId0       Device ID0 of the SPI device
//    DeviceId1       Device ID1 of the SPI device
//    PrefixOpcode    Prefix opcodes which are loaded into the SPI host controller
//    OpcodeMenu      Opcodes which are loaded into the SPI host controller Opcode Menu
//    BiosStartOffset The offset of the start of the BIOS image relative to the flash device.
//                    Please note this is a Flash Linear Address, NOT a memory space address.
//                    This value is platform specific and depends on the system flash map.
//                    This value is only used on non Descriptor mode.
//    BiosSize        The the BIOS Image size in flash. This value is platform specific
//                    and depends on the system flash map. Please note BIOS Image size may
//                    be smaller than BIOS Region size (in Descriptor Mode) or the flash size
//                    (in Non Descriptor Mode), and in this case, BIOS Image is supposed to be
//                    placed at the top end of the BIOS Region (in Descriptor Mode) or the flash
//                    (in Non Descriptor Mode)
//
typedef struct _SPI_INIT_TABLE_TPT {
    UINT8                 VendorId;
    UINT8                 DeviceId0;
    UINT8                 DeviceId1;
    UINT8                 PrefixOpcode[SPI_NUM_PREFIX_OPCODE];
    SPI_OPCODE_MENU_ENTRY_TPT OpcodeMenu[SPI_NUM_OPCODE];
    UINTN                 BiosStartOffset;
    UINTN                 BiosSize;
} SPI_INIT_TABLE_TPT;

#endif  // _BWD_SPI_FLASH_LIB_H_
