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

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Include/SpiFlashLib.h>
#include <Library/PcdLib.h>

#define SF_VENDOR_ID_ST        0x20
#define SF_DEVICE_ID0_STM25P64       0x20
#define SF_DEVICE_ID1_STM25P64       0x17

#define WINBOND_STM25P64_SIZE      	0x800000
#define BIOS_FLASH_SIZE             0x200000

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
#define SF_INST_READ_ID         0xab     // Read ID
#define SF_INST_JEDEC_READ_ID   0x9f     // JEDEC Read ID

//
// Physical Sector Size on the Serial Flash device
//
#define SF_SECTOR_SIZE    0x1000
#define SF_BLOCK_SIZE     0x8000

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
#define RECORD_ENTRY_16M_OF_4K_BLOCKS    RECORD_ENTRY_4K(32 * 16)

#define RECORD_MAP_END  { 0, 0 }

#endif  // _EFI_SPI_DEVICE_H_
