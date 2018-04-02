/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TpmSetup.h

Abstract: 
  Setup part of TPM Module.

Revision History:

Bug 3128 - Move Tpm setup module part from SnbClientX64Pkg to ByoModulePkg.
TIME: 2011-11-21
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. dynamic insert setup package to let it be independent with Platform.
$END--------------------------------------------------------------------

**/



#ifndef  __TPM_SETUP_H__
#define  __TPM_SETUP_H__
//--------------------------------------------------------------
#include <IndustryStandard/Tpm12.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/TcgService.h>
#include "TpmSetupDataStruc.h"


//--------------------------------------------------------------
extern unsigned char TpmSetupBin[];        // .vfr

#define TPM_SETUP_DATA_FROM_THIS_HII_CONFIG(this)  BASE_CR(this, TPM_SETUP_PRIVATE_DATA, ConfigAccess)




typedef struct{
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  TPM_SETUP_CONFIG                  ConfigData;
} TPM_SETUP_PRIVATE_DATA;




#pragma pack(1)

typedef struct {
  VENDOR_DEVICE_PATH           VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL     End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()


//--------------------------------------------------------------
#endif    // __TPM_SETUP_H__

