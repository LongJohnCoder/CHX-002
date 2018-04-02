/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HswSpiFlashLib.h

Abstract: 

Revision History:

**/
#ifndef _PCH_SPI_FLASH_LIB_H_
#define _PCH_SPI_FLASH_LIB_H_

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
#define SF_SR_BP3         0x20      // Block protection bit 3
#define SF_SR_WPE         0x3C      // Enable write protection on all blocks
#define SF_SR_AAI         0x40      // Auto Address Increment Programming status
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
#define SF_INST_SERASE          0x20     // Sector Erase (4KB)
#define SF_INST_BERASE          0x52     // Block Erase (32KB)
#define SF_INST_64KB_ERASE      0xD8     // Block Erase (64KB)
#define SF_INST_EWSR            0x50     // Enable Write Status Register      
#define SF_INST_READ_ID         0xAB     // Read ID
#define SF_INST_JEDEC_READ_ID   0x9F     // JEDEC Read ID

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
    SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address
    SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address
    SPI_WRSR,           // Opcode 7: Write Status Register, No address
} SPI_OPCODE_INDEX;

#endif  // _PCH_SPI_FLASH_LIB_H_
