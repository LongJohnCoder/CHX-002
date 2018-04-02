/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  spiflashdevice.h

Abstract: 

Revision History:

**/
#ifndef _EFI_SPI_DEVICE_H_
#define _EFI_SPI_DEVICE_H_

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/Include/SpiFlashLib.h>
#include <Library/PcdLib.h>

#define SF_VENDOR_ID_SST            0xBF
#define SF_DEVICE_ID0_26VF064B      0x26
#define SF_DEVICE_ID1_26VF064B      0x43

#define SST_26VF064B_SIZE           0x800000


//
// Operation Instruction definitions for the Serial Flash Device
//
//

#define SF_INST_NOP             0x00     // No operation
//configuration
#define SF_INST_WRSR            0x01     // Write configuration Register
#define SF_INST_RDSR            0x05     // Read Status Register
#define SF_INST_RDCR            0x35     // Read Configuration Register

//read
#define SF_INST_READ            0x03     // Read
#define SF_INST_HS_READ         0x0B     // High-speed Read 

//identification
#define SF_INST_JEDEC_READ_ID   0x9F     // JEDEC Read ID

//write
#define SF_INST_PROG            0x02     // Page Program 

#define SF_INST_SERASE          0x20     // Sector Erase (4KB)
#define SF_INST_BERASE          0xD8     // Block Erase (8KB/32KB/64KB depending on address)
#define SF_INST_CERASE          0xC7     // erase full array

#define SF_INST_WRDI            0x04     // Write Disable
#define SF_INST_WREN            0x06     // Write Enable

//protection

#define SF_INST_RBPR            0x72     // Read Block-Protection Register
#define SF_INST_WBPR            0x42     // Write Block-Protection Register
#define SF_INST_LBPR            0x8D     // Lock down Block-protection Register
#define SF_INST_ULBPR           0x98     // Global Block Protection unlock
//
// Physical Sector Size on the Serial Flash device
//
#define SF_SECTOR_SIZE    0x1000        // 4KB
#define SF_BLOCK_SIZE     0x8000        // 64KB

#define SIZEOF_256BYTE 8
#define SIZEOF_4K      12
#define SIZEOF_8K      13
#define SIZEOF_16K     14
#define SIZEOF_32K     15
#define SIZEOF_64K     16

#define RECORD_ENTRY_256BYTE(Count)  { SIZEOF_256BYTE, Count }
#define RECORD_ENTRY_4K(Count)       { SIZEOF_4K, Count }
#define RECORD_ENTRY_8K(Count)       { SIZEOF_8K, Count }
#define RECORD_ENTRY_16K(Count)      { SIZEOF_16K, Count }
#define RECORD_ENTRY_32K(Count)      { SIZEOF_32K, Count }
#define RECORD_ENTRY_64K(Count)      { SIZEOF_64K, Count }

#define RECORD_ENTRY_64M_OF_4K_BLOCKS    RECORD_ENTRY_4K(128 * 16)
#define RECORD_ENTRY_32M_OF_4K_BLOCKS    RECORD_ENTRY_4K(64 * 16)
#define RECORD_ENTRY_16M_OF_4K_BLOCKS    RECORD_ENTRY_4K(32 * 16)

#define RECORD_MAP_END  { 0, 0 }

#endif  // _EFI_SPI_DEVICE_H_
