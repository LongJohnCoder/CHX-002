/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TpmSetupDataStruc.h

Abstract: 
  setup part of TPM Module.

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



#ifndef __TPM_SETUP_DATA_STRUC__
#define __TPM_SETUP_DATA_STRUC__

#include <Guid/HiiPlatformSetupFormset.h>

#define TPM_SETUP_CONFIG_GUID \
  {0xee64cc88, 0xde44, 0x4880, { 0x8a, 0x47, 0x8c, 0xde, 0x70, 0xf7, 0x91, 0x98 }}



#define TPM_CONFIG_FORM_ID          1120
#define KEY_TPM_ENABLE 		          1121
#define KEY_TPM_ACTIVATE	          1122
#define KEY_TPM_FORCE_CLEAR         1123



#pragma pack(1)
typedef struct {
    UINT8   TpmPresent;
    UINT8   TpmEnable;
    UINT8   TpmState;
    UINT8   MorState;
    UINT8   PhysicalPresenceLock;
    UINT8   TpmCurrentState;
}TPM_SETUP_CONFIG;
#pragma pack()

#endif      // __TPM_SETUP_DATA_STRUC__

