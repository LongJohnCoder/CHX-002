/*++

Copyright (c)  1999 - 2011 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  FastBootExceptionInfo.h
  
Abstract:

  This file defines the HOB data structure recording an Fast Boot exception
  occurrence in PEI phase, which will be used/referenced by some DXE phase drivers.
  
--*/

#ifndef _FAST_BOOT_EXCEPTION_INFO_HOB_GUID_H_
#define _FAST_BOOT_EXCEPTION_INFO_HOB_GUID_H_

#include <PeiHob.h>
#include <FastBootDataDef.h>

#define FAST_BOOT_EXCEPTION_INFO_HOB_GUID \
  { 0x4ed88276, 0xd4df, 0x4d03, 0x86, 0x61, 0x29, 0x58, 0x1, 0xb2, 0xda, 0x58 }
  
#define FAST_BOOT_FUNCTION_ENABLED_HOB_GUID \
  { 0x19fb1ca, 0xd411, 0x4948, 0xb7, 0x3c, 0x4c, 0x5, 0x4a, 0xba, 0x9e, 0x8e }
    
EFI_FORWARD_DECLARATION (FAST_BOOT_EXCEPTION_INFO_HOB);

EFI_FORWARD_DECLARATION (FAST_BOOT_FUNCTION_ENABLED_HOB);

typedef struct _FAST_BOOT_EXCEPTION_INFO_HOB {
  EFI_HOB_GUID_TYPE             Header;
  EFI_GUID                      GuidName;
  FAST_BOOT_EXCEPTION_TYPE      FbExceptionType;
  FAST_BOOT_EXCEPTION_CATEGORY  FbExceptionCategory;
};

typedef struct _FAST_BOOT_FUNCTION_ENABLED_HOB {
  EFI_HOB_GUID_TYPE             Header;
  BOOLEAN                       FastBootEnabled;
};

extern EFI_GUID gFastBootExceptionInfoHobGuid;
extern EFI_GUID gFastBootFunctionEnabledHobGuid;

#endif

