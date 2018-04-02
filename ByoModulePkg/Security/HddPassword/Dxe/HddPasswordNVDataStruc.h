/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordDxe.c

Abstract: 
  Hdd password DXE driver.

Revision History:

Bug 3211 - Add a setup item to control enable HDD frozen or not. 
TIME:       2011-12-09
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. add a setup save notify protocol. NV Variable should be
     handled by module itself.
$END--------------------------------------------------------------------

Bug 2722 - Fix HDD Password status is not correct under setup.
TIME: 2011-08-12
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. According to ATA spec, there is no way to disable master password.
     so we make a internal rule: if master password revision code is
     same as manufacturer predefined, we treat it as uninstalled. And
     if user want to disable master password, we set revision code as
     manufacturer(0xFFFE).
  2. Hide item "Set Master Password" if user password has not been set.
$END--------------------------------------------------------------------

Bug 2720 - Fix Master Password min length is 0 caused by revision 488 
           coding error. 
TIME: 2011-08-11
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Master password valid length should be from 1 to 32, not 0 ~ 32.
$END--------------------------------------------------------------------

Bug 2654 : Modify user HDD password's have not check old user passowrd 
           of match.
TIME: 2011-08-11
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Check user input password with saved old password, if not equal, 
     reject it.
  2. this revision also remove item "disable user password". when new
     password is empty, it means remove it.
  3. show set user password result on the screen.
  4. valid password length is from 1 to 32.
$END--------------------------------------------------------------------

Bug 2650: Update hddpassword policy.
TIME: 2011-08-04
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. set password input length range as 6 to 32 instead of 1 to 32 under setup.
     And show this info at HDD Password Description area under security page 
     under setup.
  2. at password input popup window during post, prompt meet password max length
     limit when password input length reach 32.
  3. update "HDD Password Description" under setup.
$END--------------------------------------------------------------------

**/

//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
HddPassword NVData structure used by the driver.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef _HDD_PASSWORD_NVDATASTRUC_H_
#define _HDD_PASSWORD_NVDATASTRUC_H_

#include <Guid/HiiPlatformSetupFormset.h>

#define HDD_PASSWORD_CONFIG_GUID \
  { \
    0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80 } \
  }

#define FORMID_HDD_MAIN_FORM          1011  
#define FORMID_HDD_DEVICE_FORM        1012  

#define HDD_DEVICE_ENTRY_LABEL        0x1234
#define HDD_DEVICE_LABEL_END          0xffff

#define KEY_HDD_DEVICE_ENTRY_BASE     0x1000

#define KEY_HDD_USER_PASSWORD         0x101
#define KEY_HDD_DISABLE_USER_PASSWORD 0x102
#define KEY_HDD_MASTER_PASSWORD       0x103
#define KEY_HDD_FROZEN_ALL            0x104

#define HDD_USER_PASSOWORD_MIN_LENGTH      0      // actually min value is 1, "0" to support disable.
#define HDD_USER_PASSOWORD_MAX_LENGTH      32
#define HDD_MASTER_PASSOWORD_MIN_LENGTH    0      // actually min value is 1, "0" to support "disable".
#define HDD_MASTER_PASSOWORD_MAX_LENGTH    32

#pragma pack(1)

typedef struct _HDD_PASSWORD_CONFIG {
  UINT16      Supported;
  UINT16      Enabled;
  UINT16      Locked;
  UINT16      Frozen;
  UINT16      CountExpired;
  UINT16      UserPasswordStatus;
  UINT16      MasterPasswordStatus; 
  UINT16      IdeUserPassword[32];
  UINT16      DisableIdeUserPassword[32];
  UINT16      IdeMasterPassword[32];
} HDD_PASSWORD_CONFIG;


typedef struct {
  UINT8  FrozenAllHdds;
} HDD_PASSWORD_NV_CONFIG;

#pragma pack()

#endif
