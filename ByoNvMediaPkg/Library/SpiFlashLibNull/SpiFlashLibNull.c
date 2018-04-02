/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SpiFlashLibNull.c

Abstract: 

Revision History:

**/
#include "SpiFlashLibNull.h"

VOID
InitSpiTable (
    IN OUT SPI_INIT_TABLE         **pInitTable,
    IN DEVICE_TABLE               *pDeviceTable,
    IN SPI_OPCODE_MENU_ENTRY      *pOpcodeMenuList,
    IN UINT8                      *PrefixOpcodeList
)
{
    return;
}

EFI_STATUS
device_info (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN OUT MEDIA_BLOCK_MAP**           MapInfo
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
device_sense (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
devcie_read (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN OUT UINT8* Buffer,
    IN OUT UINTN* Length
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
device_write (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINT8* Buffer,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}
EFI_STATUS
device_erase (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
device_lock (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}
