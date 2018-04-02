/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Cmos.h

Abstract: 

Revision History:

**/
#ifndef _EFI_CMOS_DEVICE_H_
#define _EFI_CMOS_DEVICE_H_
#include <Uefi.h>

#include <Protocol/NvMediaAccess.h>
#include <Protocol/NvMediaDevice.h>
#include <Protocol/PlatformAccess.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

int
_outp (
    unsigned short port,
    int databyte
);
#pragma intrinsic(_outp)

int
_inp (
    unsigned short port
);
#pragma intrinsic(_inp)

#define NV_DEVICE_DATA_SIGNATURE   SIGNATURE_32('B','O','N','V')
typedef struct _NV_DEVICE_INSTANCE {
    UINT32 						Signature;
    EFI_HANDLE 					Handle;
    NV_MEDIA_DEVICE_PROTOCOL		DeviceProtocol;
    MEDIA_BLOCK_MAP 				*BlockMap;
    UINT16						Bank0Port;
    UINTN							Bank0Size;
    UINT16						Bank1Port;
    UINTN							Bank1Size;
} NV_DEVICE_INSTANCE;

#define DEVICE_INSTANCE_FROM_DEVICEPROTOCOL(a)  CR (a, NV_DEVICE_INSTANCE, DeviceProtocol, NV_DEVICE_DATA_SIGNATURE)

#endif  // _EFI_CMOS_DEVICE_H_
