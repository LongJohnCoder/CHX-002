/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordDxe.h

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

Bug 2654 : Modify user HDD password's have not check Old user passowrd 
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

Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
TIME:       2011-6-15
$AUTHOR:    Peng Xianbing
$REVIEWERS:
$SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
            get SwSMI value.
$TECHNICAL:
$END-------------------------------------------------------------------------
  
Bug 2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: 
$END----------------------------------------------------------------------------

**/

//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef _HDD_PASSWORD_DXE_H_
#define _HDD_PASSWORD_DXE_H_
//-----------------------------------------------------------------------

#include <Uefi.h>
#include <IndustryStandard/Atapi.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>
#include <Protocol/HiiConfigAccess.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Guid/HddPasswordSecurityVariable.h>
#include "HddPasswordNVDataStruc.h"
#include <Protocol/SetupSaveNotify.h>
#include <Protocol/LegacyBios.h>



//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  HddPasswordBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  HddPasswordDxeStrings[];

#define HDD_PASSWORD_DXE_PRIVATE_SIGNATURE SIGNATURE_32 ('H', 'D', 'D', 'P')

typedef struct _HDD_PASSWORD_CONFIG_FORM_ENTRY {
  LIST_ENTRY                    Link;
  EFI_HANDLE                    Controller;
  UINT8                         HostSCC;
  UINT16                        Port;
  UINT16                        PortMultiplierPort;
  CHAR16                        HddString[64];
  EFI_STRING_ID                 TitleToken;
  EFI_STRING_ID                 TitleHelpToken;
  HDD_PASSWORD_CONFIG           IfrData;
	UINT8                         InputUserPassword[32+1];		// crypt
	BOOLEAN                       InputFlag;
} HDD_PASSWORD_CONFIG_FORM_ENTRY;

typedef struct _HDD_PASSWORD_DXE_PRIVATE_DATA {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *Current;
  HDD_PASSWORD_NV_CONFIG           NvConfig;
  SETUP_SAVE_NOTIFY_PROTOCOL       SetupSaveNotify;
} HDD_PASSWORD_DXE_PRIVATE_DATA;

#define HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_HII(a)  \
  CR(a, HDD_PASSWORD_DXE_PRIVATE_DATA, ConfigAccess, HDD_PASSWORD_DXE_PRIVATE_SIGNATURE)

#define HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(a)  \
  CR(a, HDD_PASSWORD_DXE_PRIVATE_DATA, SetupSaveNotify, HDD_PASSWORD_DXE_PRIVATE_SIGNATURE)

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

#pragma pack(1)
  
///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH           VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL     End;
} HII_VENDOR_DEVICE_PATH;
  
#pragma pack()

//
// Time out value for ATA pass through protocol
//
#define ATA_TIMEOUT        EFI_TIMER_PERIOD_SECONDS (3)

#define MASTER_PASSWORD_MANUFACTURER_REVISION_CODE      0xFFFE
#define MASTER_PASSWORD_INVALID_REVISION_CODE0          0x0000
#define MASTER_PASSWORD_INVALID_REVISION_CODE1          0xFFFF
#define MASTER_PASSWORD_MY_REVISION_CODE                0x0001


#define HDDPASSWORD_USER_DEFAULT_VAR_ATTRIBUTE \
  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define HDDPASSWORD_NVSAVE_VAR_ATTRIBUTE \
  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)


#pragma pack(1)
/*
  40:00-01 Com1
  40:02-03 Com2
  40:04-05 Com3
  40:06-07 Com4
  40:08-09 Lpt1
  40:0A-0B Lpt2
  40:0C-0D Lpt3
  40:0E-0E Ebda segment
  40:10-11 MachineConfig
  40:12    Bda12 - skip
  40:13-14 MemSize below 1MB
  40:15-16 Bda15_16 - skip
  40:17    Keyboard Shift status
  40:18-19 Bda18_19 - skip
  40:1A-1B Key buffer head
  40:1C-1D Key buffer tail
  40:1E-3D Bda1E_3D- key buffer -skip
  40:3E-3F FloppyData 3E = Calibration status 3F = Motor status
  40:40    FloppyTimeout
  40:41-74 Bda41_74 - skip
  40:75    Number of HDD drives
  40:76-77 Bda76_77 - skip
  40:78-79 78 = Lpt1 timeout, 79 = Lpt2 timeout
  40:7A-7B 7A = Lpt3 timeout, 7B = Lpt4 timeout
  40:7C-7D 7C = Com1 timeout, 7D = Com2 timeout
  40:7E-7F 7E = Com3 timeout, 7F = Com4 timeout
  40:80-81 Pointer to start of key buffer
  40:82-83 Pointer to end of key buffer
  40:84-87 Bda84_87 - skip
  40:88    HDD Data Xmit rate
  40:89-8f skip
  40:90    Floppy data rate
  40:91-95 skip
  40:96    Keyboard Status
  40:97    LED Status
  40:98-101 skip
*/
typedef struct {
  UINT16  Com1;
  UINT16  Com2;
  UINT16  Com3;
  UINT16  Com4;
  UINT16  Lpt1;
  UINT16  Lpt2;
  UINT16  Lpt3;
  UINT16  Ebda;
  UINT16  MachineConfig;
  UINT8   Bda12;
  UINT16  MemSize;
  UINT8   Bda15_16[0x02];
  UINT8   ShiftStatus;
  UINT8   Bda18_19[0x02];
  UINT16  KeyHead;
  UINT16  KeyTail;
  UINT16  Bda1E_3D[0x10];
  UINT16  FloppyData;
  UINT8   FloppyTimeout;
  UINT8   Bda41_74[0x34];
  UINT8   NumberOfDrives;
  UINT8   Bda76_77[0x02];
  UINT16  Lpt1_2Timeout;
  UINT16  Lpt3_4Timeout;
  UINT16  Com1_2Timeout;
  UINT16  Com3_4Timeout;
  UINT16  KeyStart;
  UINT16  KeyEnd;
  UINT8   Bda84_87[0x4];
  UINT8   DataXmit;
  UINT8   Bda89_8F[0x07];
  UINT8   FloppyXRate;
  UINT8   Bda91_95[0x05];
  UINT8   KeyboardStatus;
  UINT8   LedStatus;
} BDA_STRUC;
#pragma pack()

  
//-----------------------------------------------------------------------
#endif
