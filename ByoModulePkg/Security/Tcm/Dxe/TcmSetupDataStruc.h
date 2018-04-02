/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmSetupDataStruc.h

Abstract: 
  Dxe part of TCM Module.

Revision History:

Bug 3075 - Add TCM support.
TIME: 2011-11-14
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Tcm module init version.
     Only support setup function.
$END--------------------------------------------------------------------

**/



#ifndef __TCM_SETUP_DATA_STRUC__
#define __TCM_SETUP_DATA_STRUC__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/TcgConfigHii.h>
#include <Guid/TcmSetupCfgGuid.h>


#define TCM_CONFIG_VARSTORE_ID      0x0001
#define TCM_CONFIG_FORM_ID          0x0001
#define KEY_TCM_ENABLE 		          1000
#define KEY_TCM_FORCE_CLEAR         1001

#pragma pack(1)
typedef struct {
  UINT8   TcmPresent;
  UINT8   TcmEnable;
  UINT8   TcmActive;
  
  UINT8   TcmUserEn;
  UINT8   TcmUserClear;
} TCM_SETUP_CONFIG;
#pragma pack()

#endif      // __TCM_SETUP_DATA_STRUC__

