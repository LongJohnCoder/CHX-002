/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SpiFlashLib.h

Abstract: 

Revision History:

**/
#ifndef _SPI_FLASH_LIB_H_
#define _SPI_FLASH_LIB_H_

#include <Uefi.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/NvMediaDevice.h>
#include <Protocol/PlatformAccess.h>
#include <Protocol/Spi.h>

typedef struct _DEVICE_TABLE {
    UINT8       VendorId;
    UINT8       DeviceId0;
    UINT8       DeviceId1;
    UINTN       Size;
} DEVICE_TABLE;

#define NV_DEVICE_DATA_SIGNATURE   SIGNATURE_32('B','O','N','V')

typedef struct _NV_DEVICE_INSTANCE {
    UINT32 						    Signature;
    EFI_HANDLE 					    Handle;
    NV_MEDIA_DEVICE_PROTOCOL		DeviceProtocol;
    EFI_SPI_PROTOCOL                *SpiProtocol;
    PLATFORM_ACCESS_PROTOCOL        *PlatformAccessProtocol;
    MEDIA_BLOCK_MAP 				*BlockMap;
    SPI_INIT_TABLE			     	*InitTable;
    UINTN 				    		Number;
    UINTN							FlashSize;
    UINTN							SectorSize;
} NV_DEVICE_INSTANCE;

#define DEVICE_INSTANCE_FROM_DEVICEPROTOCOL(a)  CR (a, NV_DEVICE_INSTANCE, DeviceProtocol, NV_DEVICE_DATA_SIGNATURE)

void
InitSpiTable (
    IN OUT SPI_INIT_TABLE **pInitTable,
    IN     DEVICE_TABLE *pDeviceTable,
    IN     SPI_OPCODE_MENU_ENTRY *pOpcodeMenuList,
    IN     UINT8                 *PrefixOpcodeList
);

EFI_STATUS
device_info (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN OUT MEDIA_BLOCK_MAP**           MapInfo
);

EFI_STATUS
device_sense (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this
);

EFI_STATUS
devcie_read (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN OUT UINT8* Buffer,
    IN OUT UINTN* Length
);

EFI_STATUS
device_write (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINT8* Buffer,
    IN UINTN Length
);

EFI_STATUS
device_erase (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
);

EFI_STATUS
device_lock (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
);

#endif  // _SPI_FLASH_LIB_H_
