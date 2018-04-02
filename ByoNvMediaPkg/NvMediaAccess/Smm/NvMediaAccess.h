/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  NvMediaAccess.h

Abstract: 

Revision History:

**/
#ifndef _SMM_NVMEDIA_ACCESS_H_
#define _SMM_NVMEDIA_ACCESS_H_

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/NvMediaAccess.h>
#include <Protocol/NvMediaDevice.h>

#define NV_MEDIA_DATA_SIGNATURE   SIGNATURE_32('N','V','A','S')
typedef struct _NV_MEDIA_INSTANCE {
    UINT32 						Signature;
    EFI_HANDLE 					Handle;
    NV_MEDIA_ACCESS_PROTOCOL		MediaAccessProtocol;
    NV_MEDIA_DEVICE_PROTOCOL*		NvDevice[MAX_MEDIA_TYPR];
} NV_MEDIA_INSTANCE;

#define MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(a)  CR (a, NV_MEDIA_INSTANCE, MediaAccessProtocol, NV_MEDIA_DATA_SIGNATURE)

#endif  // _SMM_NVMEDIA_ACCESS_H_
