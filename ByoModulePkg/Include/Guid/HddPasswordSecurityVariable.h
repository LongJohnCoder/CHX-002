
/** @file
  
Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordSecurityVariable.h

Abstract: 
  Defines the GUID of HDD password security variable and HDD list structure

Revision History:

Bug 3178 - add Hdd security Frozen support.
TIME: 2011-12-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Send ATA security frozen command to HDDs at ReadyToBoot and 
     _WAK().
$END--------------------------------------------------------------------

Bug 2956 - Issue a full reset when hdd password installed or removed at setup exit.
TIME: 2011-10-01
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Hdd password set need a power cycle to be valid.
     Note: when password has been installed, if SATA mode changed, 
     full reset is also need.
$END--------------------------------------------------------------------

Bug 2311: Remove redundant HDD Password info in NVRAM.
TIME: 2011-06-15
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Original action is add new password info to nvram, so password
     info in nvram is bigger and bigger. Now we add a Monotonic Count
     to judge new and old info and save new info instead of all.
$END--------------------------------------------------------------------

Bug 2274: improve hdd password feature.
TIME: 2011-06-09
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. give user max 3 times to input password at post.
  2. check device is hdd or not before unlock it when S3 resume.
  3. remove personal label such as wztest.
$END--------------------------------------------------------------------

Bug 2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: 
$END--------------------------------------------------------------------

**/



#ifndef _HDD_PASSWORD_SECURITY_VARIABLE_H_
#define _HDD_PASSWORD_SECURITY_VARIABLE_H_
//-----------------------------------------------------------

#define SW_SMI_HDD_UNLOCK_PASSWORD      0x78
#define SW_SMI_HDD_SAVE_PASSWORD        0x79
#define SW_SMI_HDD_FROZEN_HDD           0x7A


#define EFI_HDD_PASSWORD_SECURITY_VARIABLE_GUID \
  { \
    0x5e22f4e2, 0x35ea, 0x421b, { 0x8f, 0xd1, 0xd4, 0xae, 0x85, 0x67, 0xa4, 0x51 } \
  };


// For Unlock Hdd When Resuming From S3.
//-----------------------------------------------------------
typedef struct _EFI_HDD_DEVICE_LIST {
  UINT32       Bus;
  UINT32       Device;
  UINT32       Function;
  UINT8        HostSCC;				// SubClassCode
  UINT16       Port;
  UINT16       PortMultiplierPort;
	UINT64       MonotonicCount;
  UINT8        Password[32];
} EFI_HDD_DEVICE_LIST;

#define HDDPASSWORD_S3UNLOCK_VAR_NAME     L"HddPassword"
#define HDDPASSWORD_S3UNLOCK_VAR_ATTRIBUTE  \
(EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)





// For Setup Reset Issue.
//-----------------------------------------------------------
typedef struct {
  BOOLEAN      IsHDDUnlocked;					// when SATA mode changed, should do a full reset to re-record password.
  BOOLEAN      IsUserPassorwdChanged;	// set user password at setup.
} EFI_HDDPASSWORD_SETUP_RESET_STATUS;

#define HDDPASSWORD_STATUS_VAR_NAME       L"HddPasswdSts"
#define HDDPASSWORD_STATUS_VAR_ATTRIBUTE  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)





// For Frozen Hdd When Resuming From S3.
//-----------------------------------------------------------
typedef struct  {
  UINT8       Bus;
  UINT8       Device;
  UINT8       Function;
  UINT16       Port;
  UINT16       PortMultiplierPort;
} EFI_ALL_HDD_DEVICE_LIST;

#define HDDPASSWORD_ALLHDD_VAR_NAME       L"HddPasswdAllHDD"
#define HDDPASSWORD_ALLHDD_VAR_ATTRIBUTE  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)



typedef struct {
  UINT32                  Address;
  EFI_BOOT_SCRIPT_WIDTH   Width;
  UINT32                  Value;
  UINT32                  OriVal;
} HDD_HC_PCI_REGISTER_SAVE;

#define HDDPASSWORD_HOSTINFO_VAR_NAME     L"HostInfo"
#define HOST_INFO_VAR_ATTRIBUTE           (EFI_VARIABLE_BOOTSERVICE_ACCESS)


//-----------------------------------------------------------
extern EFI_GUID gEfiHddPasswordSecurityVariableGuid;

#endif
