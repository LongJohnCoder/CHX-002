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


Bug 3386 - SCT_HII Test-HII Config Access Protocol Test Failed
TIME:       2012-02-24
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. We should handle Extract Config invoker separately by its verdor 
  id and name. Do not override last result when there are mutli handle 
  in a function.
$END--------------------------------------------------------------------


Bug 3211 - Add a setup item to control enable HDD frozen or not. 
TIME:       2011-12-09
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. add a setup save notify protocol. NV Variable should be
     handled by module itself.
$END--------------------------------------------------------------------

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

Bug 2806 - unify protocol notification function format in hdd password.
TIME: 2011-08-31
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. EfiCreateProtocolNotifyEvent() will signal callback function immediately.
     so we should add protocol installation check first in callback function.
     this update will follow above rule instead of create callback event in my 
     own way.
$END--------------------------------------------------------------------

Bug 2730 - Clear HddPassword saved info if no disk being unlocked.
TIME: 2011-08-15
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. If HddPassword has been disabled, info should be removed as it
     was overdue. This may affect S3 Hdd unlock action.
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

Bug 2646: Disable user HDD password prompt message should be change.
TIME: 2011-08-05
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. tell setup browser we need old password, then browser will show
     "Please type in your password" instead of "Please type in your
     new password".
     this revision modification also adds password correct check for
     disable user password(read back hdd status)
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
HDD password driver which is used to support HDD security feature.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/


#include "HddPasswordDxe.h"

EFI_GUID   mHddPasswordVendorGuid                    = HDD_PASSWORD_CONFIG_GUID;
CHAR16     mHddPasswordVendorStorageName[]           = L"HDD_PASSWORD_CONFIG";
CHAR16     mHddPasswordVendorNVStorName[]            = L"HPW_NV_CONF";
CHAR16     mHddPasswordVendorNVStorNameUserDefault[] = L"HPW_NV_CONF_UD";
LIST_ENTRY mHddPasswordConfigFormList;
UINT32     mNumberOfHddDevices = 0;
UINT32     mNumberOfHddDevicesLockEnAndUnlocked = 0;
STATIC UINT64 gMonotonicCount;
CONST CHAR16 *gDefaultMasterPassword       = L"byo";
HDD_PASSWORD_NV_CONFIG gHpwPostNvConfig;
HDD_PASSWORD_NV_CONFIG gHpwDefaultNvConfig = {0};

#define BDS_ALL_DRIVERS_CONNECTED_PROTOCOL_GUID \
    {0xdbc9fd21, 0xfad8, 0x45b0, 0x9e, 0x78, 0x27, 0x15, 0x88, 0x67, 0xcc, 0x93}

STATIC EFI_GUID mBdsAllDriversConnectedProtocolGuid = BDS_ALL_DRIVERS_CONNECTED_PROTOCOL_GUID;

HII_VENDOR_DEVICE_PATH  mHddPasswordHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    HDD_PASSWORD_CONFIG_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

//-----------------------------------------------------------------
EFI_STATUS 
NvSaveValue(  
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS                      Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;  

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(This);

// We cannot read Browser Data when we current are not in that page
// (it has been destroyed when we exit).
  Status = gRT->SetVariable (
                  mHddPasswordVendorNVStorName,
                  &mHddPasswordVendorGuid,
                  HDDPASSWORD_NVSAVE_VAR_ATTRIBUTE,
                  sizeof(Private->NvConfig),
                  &Private->NvConfig
                  );
  
  return Status;
}



EFI_STATUS 
NvDiscardValue(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;
  
  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(This);
  
  HiiSetBrowserData(
     &mHddPasswordVendorGuid,
     mHddPasswordVendorNVStorName,
     sizeof(gHpwPostNvConfig),
     (UINT8*)&gHpwPostNvConfig,
     NULL
     );
  CopyMem(&Private->NvConfig, &gHpwPostNvConfig, sizeof(gHpwPostNvConfig));

  return EFI_SUCCESS; 
}



EFI_STATUS 
NvLoadDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;

  Private  = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(This);     

  HiiSetBrowserData (
     &mHddPasswordVendorGuid,
     mHddPasswordVendorNVStorName,
     sizeof(gHpwDefaultNvConfig),
     (UINT8*)&gHpwDefaultNvConfig,
     NULL
     );
  CopyMem(&Private->NvConfig, &gHpwDefaultNvConfig, sizeof(gHpwDefaultNvConfig));
  return EFI_SUCCESS;
}


EFI_STATUS 
NvSaveUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS                      Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(This); 
  
  Status = gRT->SetVariable (
                  mHddPasswordVendorNVStorNameUserDefault,
                  &mHddPasswordVendorGuid,
                  HDDPASSWORD_USER_DEFAULT_VAR_ATTRIBUTE,
                  sizeof(Private->NvConfig),
                  &Private->NvConfig
                  );
  return Status;  
}


EFI_STATUS 
NvLoadUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  UINTN                           VariableSize;
  EFI_STATUS                      Status;
  HDD_PASSWORD_NV_CONFIG          NvConfig;   
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;

  Private      = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_SSN(This); 
  VariableSize = sizeof(NvConfig);
  Status = gRT->GetVariable (
                  mHddPasswordVendorNVStorNameUserDefault,
                  &mHddPasswordVendorGuid,
                  NULL,
                  &VariableSize,
                  &NvConfig
                  );
                   
  if(!EFI_ERROR(Status)){
    HiiSetBrowserData(
       &mHddPasswordVendorGuid,
       mHddPasswordVendorNVStorName,
       VariableSize,
       (UINT8*)&NvConfig,
       NULL
       );
    CopyMem(&Private->NvConfig, &NvConfig, sizeof(NvConfig));
  }

  return Status;
}

EFI_STATUS 
IsNvDataChanged(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This,
  BOOLEAN                       *IsDataChanged
  )
{
  *IsDataChanged = FALSE;
  return EFI_SUCCESS;
}


//-----------------------------------------------------------------
STATIC
VOID
HddPasswordSetResetStatusVar(
  UINT8 IsHDDUnlocked, 
  UINT8 IsUserPassorwdChanged
  )
{
  EFI_HDDPASSWORD_SETUP_RESET_STATUS HddResetStatus;
  EFI_HDDPASSWORD_SETUP_RESET_STATUS NewHddResetStatus;  
  UINTN                              VarDataSize;
  EFI_STATUS                         Status;
  
  VarDataSize = sizeof(HddResetStatus);
  Status = gRT->GetVariable (
                  HDDPASSWORD_STATUS_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  &VarDataSize,
                  &HddResetStatus
                  );
  if(Status == EFI_NOT_FOUND){
  	if(IsHDDUnlocked == 0xFF){
  		IsHDDUnlocked = FALSE;
  	}
  	HddResetStatus.IsHDDUnlocked = (BOOLEAN)IsHDDUnlocked;
  	if(IsUserPassorwdChanged == 0xFF){
  		IsUserPassorwdChanged = FALSE;
  	}
  	HddResetStatus.IsUserPassorwdChanged = (BOOLEAN)IsUserPassorwdChanged;
  	VarDataSize = sizeof(HddResetStatus);
	  Status = gRT->SetVariable (
	                  HDDPASSWORD_STATUS_VAR_NAME,
	                  &gEfiHddPasswordSecurityVariableGuid,
	                  HDDPASSWORD_STATUS_VAR_ATTRIBUTE,
	                  VarDataSize,
	                  &HddResetStatus
	                  );    	
  }else if(Status == EFI_SUCCESS){
  	NewHddResetStatus.IsHDDUnlocked         = HddResetStatus.IsHDDUnlocked;
  	NewHddResetStatus.IsUserPassorwdChanged = HddResetStatus.IsUserPassorwdChanged;
  	if(IsHDDUnlocked != 0xFF){
      NewHddResetStatus.IsHDDUnlocked = (BOOLEAN)IsHDDUnlocked;
  	}
  	if(IsUserPassorwdChanged != 0xFF){
      NewHddResetStatus.IsUserPassorwdChanged = (BOOLEAN)IsUserPassorwdChanged;  		
  	}
  	
  	if((NewHddResetStatus.IsHDDUnlocked != HddResetStatus.IsHDDUnlocked) ||
  		  (NewHddResetStatus.IsUserPassorwdChanged != HddResetStatus.IsUserPassorwdChanged)){
	  	VarDataSize = sizeof(NewHddResetStatus);
		  Status = gRT->SetVariable (
		                  HDDPASSWORD_STATUS_VAR_NAME,
		                  &gEfiHddPasswordSecurityVariableGuid,
		                  HDDPASSWORD_STATUS_VAR_ATTRIBUTE,
		                  VarDataSize,
		                  &NewHddResetStatus
		                  );
		}   	
  }
}




/**
  Encode password which is stored at variable region.

  @param  Password       The pointer to the password to be encoded.

  @return None

**/
VOID
EFIAPI
EncodeHddPassword (
  IN OUT  CHAR8       *Password,
  IN      UINTN       Size
  )
{
  UINTN     Index;
  CHAR8     Mask[32];

  ASSERT (Size <= 32);

  CopyMem (Mask, "ALSIER*$*$JM<>D>E)E*O)UE)(&*KD K", 32);

  for (Index = 0; Index < Size; Index += 1) {
    Password[Index] ^= Mask[Index];
  }
}

/**
  Add device node to global list for s3 resume.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
AddHddDeviceNode (
  IN  EFI_HANDLE                    Controller,
  IN  UINT8                         HostSCC,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  CHAR16                        *Password
  )
{
  EFI_STATUS               Status;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  UINTN                    SegNum;
  UINTN                    BusNum;
  UINTN                    DevNum;
  UINTN                    FuncNum;
  CHAR8                    AsciiPassword[32 + 1];
  VOID                     *HddPasswordDeviceList;
  UINTN                    DeviceListSize;
  EFI_HDD_DEVICE_LIST      *DeviceNode;
  EFI_HDD_DEVICE_LIST      *DeviceList;
  UINTN                    Index;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo
                  );
  ASSERT_EFI_ERROR (Status);
  
  Status = PciIo->GetLocation (
                    PciIo,
                    &SegNum,
                    &BusNum,
                    &DevNum,
                    &FuncNum
                    );
  ASSERT_EFI_ERROR (Status);

  ZeroMem (AsciiPassword, sizeof (AsciiPassword));
  UnicodeStrToAsciiStr(Password, AsciiPassword);

  HddPasswordDeviceList = NULL;
  DeviceList     = NULL;
  DeviceListSize = 0;
  Status = gRT->GetVariable (
                  HDDPASSWORD_S3UNLOCK_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  &DeviceListSize,
                  HddPasswordDeviceList
                  );

  if (Status == EFI_NOT_FOUND) {													// New var
    DEBUG ((EFI_D_INFO, "HddPass: add new P:%d MP:%d\n", Port, PortMultiplierPort));
    DeviceNode = AllocateZeroPool (sizeof (EFI_HDD_DEVICE_LIST));

    DeviceNode->Bus      = (UINT32)BusNum;
    DeviceNode->Device   = (UINT32)DevNum;
    DeviceNode->Function = (UINT32)FuncNum;
	  DeviceNode->HostSCC  = HostSCC;
    DeviceNode->Port     = Port;
		DeviceNode->MonotonicCount = gMonotonicCount;
    DeviceNode->PortMultiplierPort = PortMultiplierPort;
    CopyMem (DeviceNode->Password, AsciiPassword, 32);
    EncodeHddPassword(DeviceNode->Password, sizeof (DeviceNode->Password));

    DeviceList     = DeviceNode;
    DeviceListSize = sizeof (EFI_HDD_DEVICE_LIST);
  } else if (Status == EFI_BUFFER_TOO_SMALL) {						// exist var

    //
    // Allocate the buffer to return
    //
    HddPasswordDeviceList = AllocateZeroPool (DeviceListSize);
    if (HddPasswordDeviceList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable (
                    HDDPASSWORD_S3UNLOCK_VAR_NAME,
                    &gEfiHddPasswordSecurityVariableGuid,
                    NULL,
                    &DeviceListSize,
                    HddPasswordDeviceList
                    );
    ASSERT_EFI_ERROR (Status);

    for (Index = 0; Index < DeviceListSize / sizeof (EFI_HDD_DEVICE_LIST); Index += 1) {
      DeviceNode = (EFI_HDD_DEVICE_LIST *)((UINTN)HddPasswordDeviceList + Index * sizeof (EFI_HDD_DEVICE_LIST));

      if ((DeviceNode->Bus == BusNum) &&
          (DeviceNode->Device == DevNum) &&
          (DeviceNode->Function == FuncNum) &&
          (DeviceNode->HostSCC == HostSCC) &&
          (DeviceNode->Port == Port) && 
          (DeviceNode->PortMultiplierPort == PortMultiplierPort)) {
        //
        // if the device node has existed, then update the password and MonotonicCount.
        //
        DEBUG ((EFI_D_INFO, "HddPass: update new P:%d MP:%d\n", Port, PortMultiplierPort));
        DeviceNode->MonotonicCount = gMonotonicCount;
        CopyMem (DeviceNode->Password, AsciiPassword, 32);
        EncodeHddPassword(DeviceNode->Password, sizeof (DeviceNode->Password));
        DeviceList = HddPasswordDeviceList;
        break;
      }
    }

    //
    // Device node is not found, so add the new device node into device list variable.
    //
    if (Index >= DeviceListSize / sizeof (EFI_HDD_DEVICE_LIST)) {
      DeviceList = AllocateZeroPool (sizeof (EFI_HDD_DEVICE_LIST) + DeviceListSize);
      if (DeviceList == NULL) {
        FreePool (HddPasswordDeviceList);
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (DeviceList, HddPasswordDeviceList, DeviceListSize);
      ZeroMem (HddPasswordDeviceList, DeviceListSize);
      FreePool (HddPasswordDeviceList);

      DeviceNode = (EFI_HDD_DEVICE_LIST *)((UINTN)DeviceList + DeviceListSize);
      DeviceNode->Bus      = (UINT32) BusNum;
      DeviceNode->Device   = (UINT32) DevNum;
      DeviceNode->Function = (UINT32) FuncNum;
      DeviceNode->HostSCC  = HostSCC;
      DeviceNode->Port     = Port;
      DeviceNode->PortMultiplierPort = PortMultiplierPort;
			DeviceNode->MonotonicCount = gMonotonicCount;
      CopyMem (DeviceNode->Password, AsciiPassword, 32);
      EncodeHddPassword(DeviceNode->Password, sizeof (DeviceNode->Password));
      DeviceListSize = sizeof (EFI_HDD_DEVICE_LIST) + DeviceListSize;
      DEBUG ((EFI_D_INFO, "HddPass: add new P:%d MP:%d\n", Port, PortMultiplierPort));
    }
  } else {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  {
	  EFI_HDD_DEVICE_LIST *NewDeviceList;
		EFI_HDD_DEVICE_LIST *List;
		UINTN               i, j;
		
    NewDeviceList = AllocateZeroPool(DeviceListSize);
    if (NewDeviceList == NULL) {
      FreePool (DeviceList);
      return EFI_OUT_OF_RESOURCES;
    }
		
    List = NewDeviceList;
		DeviceNode = DeviceList;
		j = DeviceListSize/sizeof(EFI_HDD_DEVICE_LIST);
		DeviceListSize = 0;
    for (i=0;i<j;i++){
      if(DeviceNode->MonotonicCount == gMonotonicCount){
        CopyMem(List, DeviceNode, sizeof(EFI_HDD_DEVICE_LIST));
				List++;
				DeviceListSize += sizeof(EFI_HDD_DEVICE_LIST);
	    }
			DeviceNode++;
    }
		FreePool(DeviceList);
		DeviceList = NewDeviceList;
  }

  Status = gRT->SetVariable (
                  HDDPASSWORD_S3UNLOCK_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  HDDPASSWORD_S3UNLOCK_VAR_ATTRIBUTE,
                  DeviceListSize,
                  DeviceList
                  );
  ASSERT_EFI_ERROR (Status);
  mNumberOfHddDevicesLockEnAndUnlocked++;
  HddPasswordSetResetStatusVar(TRUE, 0xFF);
  
  //
  // clear temporary password buffer for security.
  //
  FreePool (DeviceList);
  ZeroMem (Password, StrSize (Password));
  ZeroMem (AsciiPassword, sizeof (AsciiPassword));

  return EFI_SUCCESS;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
GetHddDeviceIdentifyData (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  ATA_IDENTIFY_DATA             *IdentifyData
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;

  if ((AtaPassThru == NULL) || (IdentifyData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.InDataBuffer     = IdentifyData;
  Packet.InTransferLength = sizeof (ATA_IDENTIFY_DATA);
  Packet.Timeout          = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  
  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecurityUnlockHdd (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  CHAR8                         Identifier,
  IN  CHAR16                        *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  CHAR16                            PayLoad[256];
  CHAR8                             Temp[32 + 1];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF2;   //SECURITY UNLOCK CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 

  //
  // Payload passed from Host to Device
  //
  ZeroMem (Temp, sizeof (Temp));
  UnicodeStrToAsciiStr(Password, Temp);
  ZeroMem (PayLoad, sizeof (PayLoad));
  PayLoad[0] = Identifier & BIT0;
  CopyMem (&PayLoad[1], Temp, 32);
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.OutDataBuffer     = PayLoad;
  Packet.OutTransferLength = 512;
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );

  //
  // For security issue, need to clean buffer to store password
  //  
  ZeroMem (PayLoad, sizeof (PayLoad));
  ZeroMem (Temp, sizeof (Temp));

  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecuritySetHddPassword (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  CHAR8                         Identifier,
  IN  CHAR8                         MasterPasswordCapability,
  IN  CHAR16                        MasterPasswordIdentifier,
  IN  CHAR16                        *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  CHAR16                            PayLoad[256];
  CHAR8                             Temp[32 + 1];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF1;   //SECURITY SET PASSWORD CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 

  //
  // Payload passed from Host to Device
  //  
  ZeroMem (Temp, sizeof (Temp));
  UnicodeStrToAsciiStr(Password, Temp);
  ZeroMem (PayLoad, sizeof (PayLoad));
  PayLoad[0] = (Identifier | (UINT16)(MasterPasswordCapability << 8)) & (BIT0 | BIT8);
  CopyMem (&PayLoad[1], Temp, 32);
  if (Identifier & BIT0) {
    PayLoad[17] = MasterPasswordIdentifier;
  }
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.OutDataBuffer     = PayLoad;
  Packet.OutTransferLength = 512;
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );

  //
  // For security issue, need to clean buffer to store password
  //  
  ZeroMem (PayLoad, sizeof (PayLoad));
  ZeroMem (Temp, sizeof (Temp));
  
  if(!EFI_ERROR(Status)){
  	HddPasswordSetResetStatusVar(0xFF, TRUE);
  }

  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecurityDisableHddPassword (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  CHAR8                         Identifier,
  IN  CHAR16                        *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  CHAR16                            PayLoad[256];
  CHAR8                             Temp[32 + 1];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF6;   //SECURITY DISABLE PASSWORD CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 

  //
  // Payload passed from Host to Device
  //
  ZeroMem (Temp, sizeof (Temp));
  UnicodeStrToAsciiStr(Password, Temp);
  ZeroMem (PayLoad, sizeof (PayLoad));
  PayLoad[0] = Identifier & BIT0;
  CopyMem (&PayLoad[1], Temp, 32);
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.OutDataBuffer     = PayLoad;
  Packet.OutTransferLength = 512;
  Packet.Timeout          = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );

  //
  // For security issue, need to clean buffer to store password
  //  
  ZeroMem (PayLoad, sizeof (PayLoad));
  ZeroMem (Temp, sizeof (Temp));

  if(!EFI_ERROR(Status)){
  	HddPasswordSetResetStatusVar(0xFF, TRUE);
  }

  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecurityFrozenLock (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;

  if (AtaPassThru == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF5;   //SECURITY FROZEN LOCK CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.Timeout  = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  
  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecurityErasePrepare (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;

  if (AtaPassThru == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF3;   //SECURITY ERASE PREPARE CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.Timeout  = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  
  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
SecurityEraseUnit (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  CHAR8                         Identifier,
  IN  CHAR8                         EraseMode,
  IN  CHAR16                        *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  CHAR16                            PayLoad[256];
  CHAR8                             Temp[32 + 1];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (&Asb, sizeof (Asb));
  Acb.AtaCommand    = 0xF4;   //SECURITY ERASE UNIT CMD
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4)); 

  //
  // Payload passed from Host to Device
  //
  ZeroMem (Temp, sizeof (Temp));
  UnicodeStrToAsciiStr(Password, Temp);
  ZeroMem (PayLoad, sizeof (PayLoad));
  PayLoad[0] = (Identifier & BIT0) | ((EraseMode << 1) & BIT1);
  CopyMem (&PayLoad[1], Temp, 32);
  
  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.OutDataBuffer     = PayLoad;
  Packet.OutTransferLength = 512;
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );

  //
  // For security issue, need to clean buffer to store password
  //  
  ZeroMem (PayLoad, sizeof (PayLoad));
  ZeroMem (Temp, sizeof (Temp));

  return Status;
}

/**
  Get attached harddisk device port info through Ata Pass Thru Protocol.

  @param[in] AtaPassThru        The device handle to a Ide/Sata host controller.
  @param[in] Port               The port number of the ATA device to send the command. 
  @param[in] PortMultiplierPort The port multiplier port number of the ATA device to send the command.
                                If there is no port multiplier, then specify 0.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
GetHddPasswordSecurityStatus (
  IN     ATA_IDENTIFY_DATA    *IdentifyData,
  IN OUT HDD_PASSWORD_CONFIG  *IfrData
  )
{
  UINT16  RevisionCode;
  
  IfrData->Supported           = (IdentifyData->command_set_supported_82 & BIT1) ? 1 : 0;
  IfrData->Enabled             = (IdentifyData->security_status & BIT1) ? 1 : 0;
  IfrData->Locked              = (IdentifyData->security_status & BIT2) ? 1 : 0;
  IfrData->Frozen              = (IdentifyData->security_status & BIT3) ? 1 : 0;
  IfrData->CountExpired        = (IdentifyData->security_status & BIT4) ? 1 : 0;
  IfrData->UserPasswordStatus  = IfrData->Enabled;
  
  RevisionCode = IdentifyData->master_password_identifier;
  if(RevisionCode == MASTER_PASSWORD_INVALID_REVISION_CODE0 ||
     RevisionCode == MASTER_PASSWORD_INVALID_REVISION_CODE1 ||
     RevisionCode == MASTER_PASSWORD_MANUFACTURER_REVISION_CODE){
    IfrData->MasterPasswordStatus = 0;
  }else{
    IfrData->MasterPasswordStatus = 1;
  }
  return EFI_SUCCESS;
}

/**
  Get attached harddisk model number through Ata Pass Thru Protocol.

  @param[in] Controller    The device handle to a Ide/Sata host controller.
  @param[in] DevicePath    Pointer of device path node to the attached harddisk.
  @param[in] String        The buffer to store harddisk model number.

  @retval EFI_SUCCESS            Successful to get harddisk model number.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_DEVICE_ERROR       Can not get harddisk model number.

**/
EFI_STATUS
GetHddDeviceModelNumber (
  IN ATA_IDENTIFY_DATA             *IdentifyData,
  IN OUT CHAR16                    *String
  )
{
  UINTN             Index;
  
  //
  // Swap the byte order in the original module name.
  // From Ata spec, the maximum length is 40 bytes.
  //
  for (Index = 0; Index < 40; Index += 2) {
    String[Index]      = IdentifyData->ModelName[Index + 1];
    String[Index + 1]  = IdentifyData->ModelName[Index];
  }

  //
  // Chap it off after 11 characters
  //
//String[11] = L'\0';

// try to remove more SPACEs at string tail.
	Index--;		// now point to last wchar.
  while(Index){
		if(String[Index] == L' '){
			Index--;
		}else{
		  break;
		}
  }
	String[Index+1] = L'\0';
			

  return EFI_SUCCESS;
}


/**
  Clear screen.

  @param[in] NONE
  
  @retval    NONE

**/
STATIC VOID HddPasswordClearScreen()
{
  gST->ConOut->ClearScreen(gST->ConOut);
}


/**
  PopUpBox to show message on the screen, then wait user press ENTER key to contine.

  @param[in] Result              Unicode string to show

  @retval NONE

**/
STATIC VOID ShowResultAndWaitCRKey(CHAR16 *Result)
{
  EFI_INPUT_KEY Key;

	if(Result == NULL){
		return;
	}
	
  HddPasswordClearScreen();
	while (TRUE){
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      Result,
      L"Press ENTER key to continue ...",
      NULL
    );
    if (Key.ScanCode == SCAN_NULL && Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      break;
    }
	}
}


/**
  Get password input from the popup windows.
  
  @param[in]       HddString    On entry, point to UserName buffer lengh, in bytes.
                                On exit, point to input user name length, in bytes.
  @param[in, out]  Password     The buffer to hold the input user name.
 
  @retval EFI_ABORTED           It is given up by pressing 'ESC' key.
  @retval EFI_NOT_READY         Not a valid input at all.
  @retval EFI_SUCCESS           Get a user name successfully.

**/
EFI_STATUS
PopupHddPasswordInputWindows (
  IN     CHAR16      *HddString,
  IN OUT CHAR16      *Password
  )
{
  EFI_INPUT_KEY Key;
  UINTN         Length;
  CHAR16        String[8 + 64];
  CHAR16        Mask[32 + 1 + 1];   // 32*('*') + '_' + '\0'
  CHAR16        *StrTip;
  CHAR16        *StrTipCommon  = L"        Enter HDD User Password:        ";
  CHAR16        *StrTipOverLen = L"Warning: meet password length max limit!";

  StrTip = StrTipCommon;
  ZeroMem (Password, sizeof(CHAR16)*(32+1));
	ZeroMem (Mask, sizeof(Mask));
  ZeroMem (String, sizeof(String));
  UnicodeSPrint(String, sizeof(String), L"Unlock %s", HddString);

  Length = 0;
  while (TRUE) {
    Mask[Length] = L'_';
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      String,
      StrTip,
      L"---------------------",
      Mask,
      NULL
      );
    //
    // Check key.
    //
    if (Key.ScanCode == SCAN_NULL) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Password[Length] = 0;
        Length++;
        break;
      }else if((Key.UnicodeChar == CHAR_NULL) ||
                 (Key.UnicodeChar == CHAR_TAB) ||
                 (Key.UnicodeChar == CHAR_LINEFEED)
                 ) {
        continue;
      }else{
        if(Key.UnicodeChar == CHAR_BACKSPACE){
          if (Length > 0) {       
            Password[Length] = 0;
            Mask[Length] = 0;
            Length--;
          }
        }else{
          if(Length < HDD_USER_PASSOWORD_MAX_LENGTH){
            Password[Length] = Key.UnicodeChar;
            Mask[Length] = L'*';
            Length++;
          }
        }
        
        if(Length < HDD_USER_PASSOWORD_MAX_LENGTH){
          StrTip = StrTipCommon;
        }else{
          StrTip = StrTipOverLen;
        }
      }
    }

    if (Key.ScanCode == SCAN_ESC) {
      return EFI_ABORTED;
    }
  }

  if (Length <= 1) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Get the HDD Password configuration form entry by the index of the goto opcode actived.

  @param[in]  Index The 0-based index of the goto opcode actived.

  @return The HDD Password configuration form entry found.
**/
HDD_PASSWORD_CONFIG_FORM_ENTRY *
HddPasswordGetConfigFormEntryByIndex (
  IN UINT32 Index
  )
{
  LIST_ENTRY                     *Entry;
  UINT32                         CurrentIndex;
  HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry;

  CurrentIndex    = 0;
  ConfigFormEntry = NULL;

  EFI_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    if (CurrentIndex == Index) {
      ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);
      break;
    }

    CurrentIndex++;
  }

  return ConfigFormEntry;
}





EFI_STATUS
EFIAPI
HddPasswordDoSingleFormExtractConfig (
  IN  CONST EFI_STRING  Request,
  IN  EFI_GUID          *VendorGuid,
  IN  CHAR16            *VendorName,
  IN  EFI_HANDLE        DriverHandle,
  IN  UINT8             *IfrData,
  IN  UINTN             DataSize,
  OUT EFI_STRING        *Progress,
  OUT EFI_STRING        *Results
  )
{
  EFI_STATUS                       Status;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch(Request, VendorGuid, VendorName)){
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr(VendorGuid, VendorName, DriverHandle);
    Size = (StrLen(ConfigRequestHdr) + 32 + 1) * sizeof(CHAR16);
    ConfigRequest = AllocateZeroPool(Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint(ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)DataSize);
    FreePool (ConfigRequestHdr);
  }
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                IfrData,
                                DataSize,
                                Results,
                                Progress
                                );
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen(Request);
  }
  
  return Status;
}







/**
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in] This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request    A null-terminated Unicode string in
                        <ConfigRequest> format. Note that this
                        includes the routing information as well as
                        the configurable name / value pairs. It is
                        invalid for this string to be in
                        <MultiConfigRequest> format.
  @param[out] Progress  On return, points to a character in the
                        Request string. Points to the string's null
                        terminator if request was successful. Points
                        to the most recent "&" before the first
                        failing name / value pair (or the beginning
                        of the string if the failure is in the first
                        name / value pair) if the request was not
                        successful.
  @param[out] Results   A null-terminated Unicode string in
                        <ConfigAltResp> format which has all values
                        filled in for the names in the Request string.
                        String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL. 
  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.Currently not implemented.
**/
EFI_STATUS
EFIAPI
HddPasswordFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINT8                            *IfrData;
  HDD_PASSWORD_DXE_PRIVATE_DATA    *Private;
  UINTN                            Size;
  EFI_GUID                         *VendorGuid;
  CHAR16                           *VendorName;

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_HII(This);
    
  if(Progress == NULL || Results == NULL){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  *Progress  = Request;
  VendorGuid = &mHddPasswordVendorGuid;
  VendorName = mHddPasswordVendorStorageName;
  if((Request != NULL) && !HiiIsConfigHdrMatch(Request, VendorGuid, VendorName)){
    goto TryNextItem;
  }
  Size    = sizeof(HDD_PASSWORD_CONFIG);
  IfrData = AllocateZeroPool(Size);
  ASSERT (IfrData != NULL);
  if (Private->Current != NULL) {
    CopyMem(IfrData, &Private->Current->IfrData, sizeof(HDD_PASSWORD_CONFIG));
  }
  Status = HddPasswordDoSingleFormExtractConfig(
             Request,
             VendorGuid,
             VendorName,
             Private->DriverHandle,
             IfrData,
             Size,
             Progress,
             Results
             );
  FreePool(IfrData);
  goto ProcExit;
  

TryNextItem:
  VendorGuid = &mHddPasswordVendorGuid;
  VendorName = mHddPasswordVendorNVStorName;
  if((Request != NULL) && !HiiIsConfigHdrMatch(Request, VendorGuid, VendorName)){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }  
  Size    = sizeof(HDD_PASSWORD_NV_CONFIG);
  IfrData = AllocateZeroPool(Size);
  ASSERT (IfrData != NULL);
  CopyMem(IfrData, &Private->NvConfig, sizeof(HDD_PASSWORD_NV_CONFIG));
  Status = HddPasswordDoSingleFormExtractConfig(
             Request,
             VendorGuid,
             VendorName,
             Private->DriverHandle,
             IfrData,
             Size,
             Progress,
             Results
             );
  FreePool(IfrData);

ProcExit:
  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.   
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginn ing of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.  
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
HddPasswordFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if(HiiIsConfigHdrMatch(Configuration, &mHddPasswordVendorGuid, mHddPasswordVendorStorageName)){
    *Progress = Configuration + StrLen(Configuration);
    return EFI_SUCCESS;
  }
  if(HiiIsConfigHdrMatch(Configuration, &mHddPasswordVendorGuid, mHddPasswordVendorNVStorName)){
    *Progress = Configuration + StrLen(Configuration);
    return EFI_SUCCESS;
  }
  
  *Progress = Configuration;
  return EFI_NOT_FOUND;
}



STATIC VOID *LibGetVariableAndSize(
  IN CHAR16 *Name, 
  IN EFI_GUID *VendorGuid, 
  OUT UINTN *VariableSize)
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  VOID        *Buffer;

  Buffer = NULL;
  BufferSize = 0;
  
  Status = gRT->GetVariable(Name, VendorGuid, NULL, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL){
    Buffer = AllocatePool(BufferSize);
    if (Buffer == NULL) {
      return NULL;
    }
    Status = gRT->GetVariable(Name, VendorGuid, NULL, &BufferSize, Buffer);
    if(EFI_ERROR(Status)){
      FreePool(Buffer);
      Buffer = NULL;
      BufferSize = 0;
    }
  }

  if(VariableSize != NULL){
    *VariableSize = BufferSize;
  }
  return Buffer;
}

STATIC VOID DecodeHddPassword(CHAR8 *Password)
{
  UINTN     Index;
  CHAR8     Mask[32];

  CopyMem(Mask, "ALSIER*$*$JM<>D>E)E*O)UE)(&*KD K", 32);

  for (Index = 0; Index < 32; Index++) {
    Password[Index] ^= Mask[Index];
  }
}

STATIC EFI_STATUS GetNVSHddUserPassword(
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *Config,
  IN CHAR16                         *Password)
{
  EFI_STATUS           Status;
  EFI_HDD_DEVICE_LIST  *DeviceList;
  UINTN                DeviceListSize;
  UINTN                Count, i;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINTN                Seg;
  UINTN                Bus;
  UINTN                Dev;
  UINTN                Func;
  BOOLEAN              HasFound;
  CHAR8                UserPassword8[32+1];
  
  HasFound = FALSE;
  Status   = EFI_NOT_FOUND;
  
  DeviceList = LibGetVariableAndSize(
               HDDPASSWORD_S3UNLOCK_VAR_NAME,
               &gEfiHddPasswordSecurityVariableGuid,
               &DeviceListSize);
  if(DeviceList == NULL){
    return EFI_NOT_FOUND;
  }
  Count = DeviceListSize/sizeof(EFI_HDD_DEVICE_LIST);
  Status = gBS->HandleProtocol (
                  Config->Controller,
                  &gEfiPciIoProtocolGuid,
                  &PciIo
                  );
  Status = PciIo->GetLocation (
                    PciIo,
                    &Seg,
                    &Bus,
                    &Dev,
                    &Func
                    );
                    
  for(i=0;i<Count;i++){
    if(Bus == DeviceList->Bus && Dev == DeviceList->Device &&
       Func == DeviceList->Function && Config->HostSCC == DeviceList->HostSCC &&
       Config->Port == DeviceList->Port && 
       Config->PortMultiplierPort == DeviceList->PortMultiplierPort){
        ZeroMem(UserPassword8, sizeof(UserPassword8));
        CopyMem(UserPassword8, DeviceList->Password, 32);
        DecodeHddPassword(UserPassword8);
        AsciiStrToUnicodeStr(UserPassword8, Password);
        ZeroMem(UserPassword8, sizeof(UserPassword8));     
        HasFound = TRUE;
        break;
    }
    DeviceList++;
  }
  
  FreePool(DeviceList);
  
  if(HasFound){
    Status = EFI_SUCCESS;
  }
  return Status;                 
}

STATIC VOID SaveInputPassword(HDD_PASSWORD_CONFIG_FORM_ENTRY *Config, CHAR16 *Password)
{
  ZeroMem(Config->InputUserPassword, sizeof(Config->InputUserPassword));
  UnicodeStrToAsciiStr(Password, Config->InputUserPassword);
  EncodeHddPassword(Config->InputUserPassword, 32);
  Config->InputFlag = TRUE; 
}

STATIC BOOLEAN GetSavedInputPassword(HDD_PASSWORD_CONFIG_FORM_ENTRY *Config, CHAR16 *Password)
{
  CHAR8  UserPassword8[32+1];
   
  if(Config->InputFlag){
    CopyMem(UserPassword8, Config->InputUserPassword, sizeof(Config->InputUserPassword));
    DecodeHddPassword(UserPassword8);
    AsciiStrToUnicodeStr(UserPassword8, Password);
    ZeroMem(UserPassword8, sizeof(UserPassword8));
    return TRUE;   
  }else{
    return FALSE;
  }
}


/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to 
                                 vary based on the opcode that enerated the callback.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]  ActionRequest     On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.Currently not implemented.
  @retval EFI_INVALID_PARAMETERS Passing in wrong parameter. 
  @retval Others                 Other errors as indicated. 
**/
EFI_STATUS
EFIAPI
HddPasswordFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;
  EFI_STRING_ID                    DeviceFormTitleToken;
  HDD_PASSWORD_CONFIG              *IfrData;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  EFI_STATUS                       Status;
  EFI_ATA_PASS_THRU_PROTOCOL       *AtaPassThru;
  CHAR16                           *Password;
  ATA_IDENTIFY_DATA                IdentifyData;
  BOOLEAN                          NeedUpdateData;
  BOOLEAN                          rc;
  STATIC UINTN                     StateMathine = 0;


  Password = NULL;
	NeedUpdateData = FALSE;

  DEBUG((EFI_D_INFO, "%a() A:%d Q:%d T:%d\n", __FUNCTION__, Action, QuestionId, Type));

  if ((Action != EFI_BROWSER_ACTION_CHANGED) && (Action != EFI_BROWSER_ACTION_CHANGING)) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action
    //
    return EFI_SUCCESS;
  }

  if ((Type == EFI_IFR_TYPE_STRING) && (Value->string == 0)) {
		DEBUG((EFI_D_INFO, "StateMathine -> 0\n"));
    StateMathine = 0;
    return EFI_INVALID_PARAMETER;
  }

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS_HII(This);

  //
  // Retrive data from Browser
  //
  IfrData = AllocateZeroPool (sizeof (HDD_PASSWORD_CONFIG));
  ASSERT (IfrData != NULL);
  if (!HiiGetBrowserData(&mHddPasswordVendorGuid, 
                         mHddPasswordVendorStorageName, 
                         sizeof (HDD_PASSWORD_CONFIG), 
                         (UINT8*)IfrData)){
    FreePool(IfrData);
    return EFI_NOT_FOUND;
  }

  Status = EFI_SUCCESS;

  switch (QuestionId) {
    case KEY_HDD_FROZEN_ALL:
      {
        UINTN                   VariableSize;
        HDD_PASSWORD_NV_CONFIG  NvConfig;
        BOOLEAN                 rc;
        
        VariableSize = sizeof(HDD_PASSWORD_NV_CONFIG);
        rc = HiiGetBrowserData (
               &mHddPasswordVendorGuid, 
               mHddPasswordVendorNVStorName,
               VariableSize, 
               (UINT8*)&NvConfig
               );
        CopyMem(&Private->NvConfig, &NvConfig, sizeof(NvConfig));      
      }
      break;
      
    case KEY_HDD_USER_PASSWORD:
// if control goes here, HDD status must be Supported=1 and Frozen=0(by .vfr)
// Set Password Level as High.
      Password = HiiGetString(Private->HiiHandle, Value->string, NULL);
      if(Password==NULL){
        Status = EFI_INVALID_PARAMETER;
        goto ProcExit;       
      }
      Status = gBS->HandleProtocol(
                      Private->Current->Controller, 
                      &gEfiAtaPassThruProtocolGuid, 
                      &AtaPassThru
                      );
      if(EFI_ERROR(Status)){goto ProcExit;}
      
      if(!IfrData->Enabled){
        if(StateMathine==0){                  // check old password verify need?
          if(Password[0]==0){
            Status = EFI_SUCCESS;             // no need
          }
        }else if(StateMathine==1){            // set new password.
          StateMathine = (UINTN)-1;
          if(Password[0]!=0){                 // valid password.
            Status = SecuritySetHddPassword(
                       AtaPassThru,
                       Private->Current->Port, 
                       Private->Current->PortMultiplierPort, 
                       0, 
                       0, 
                       0, 
                       Password
                       );
            if(EFI_ERROR(Status)){goto ProcExit;}      
            Status = GetHddDeviceIdentifyData(AtaPassThru, 
                                              Private->Current->Port, 
                                              Private->Current->PortMultiplierPort, 
                                              &IdentifyData);
            if(EFI_ERROR(Status)){goto ProcExit;}                                              
            GetHddPasswordSecurityStatus(&IdentifyData, IfrData);
            CopyMem(&Private->Current->IfrData, IfrData, sizeof(HDD_PASSWORD_CONFIG));
            
            SaveInputPassword(Private->Current, Password);
            Status = EFI_SUCCESS;
          }else{
            Status = EFI_INVALID_PARAMETER;
          }
        }
      }else if(IfrData->Enabled && !IfrData->Locked){
        if(StateMathine==0){                  // check old password verify need?
          if(Password[0]==0){
            Status = EFI_INVALID_PARAMETER;   // need
          }
        }else if(StateMathine==1){
          CHAR16  LastUserPassword[32+1];
          if(GetSavedInputPassword(Private->Current, LastUserPassword)){
            if(StrCmp(Password, LastUserPassword)==0){
              Status = EFI_SUCCESS;
            }else{
              Status = EFI_NOT_READY;
            }
            ZeroMem(LastUserPassword, sizeof(LastUserPassword));
          }else{
            GetNVSHddUserPassword(Private->Current, LastUserPassword);
            if(StrCmp(LastUserPassword, Password)==0){
              Status = EFI_SUCCESS;
            }else{
              Status = EFI_NOT_READY;
            }
          }
        }else if(StateMathine==2){            // set new password
          StateMathine = (UINTN)-1;
          if(Password[0]==0){                 // clear
            CHAR16 LastUserPassword[32+1];
            if(GetSavedInputPassword(Private->Current, LastUserPassword)){
            }else{
              GetNVSHddUserPassword(Private->Current, LastUserPassword);
            }
            Status = SecurityDisableHddPassword(AtaPassThru,
                                                Private->Current->Port, 
                                                Private->Current->PortMultiplierPort, 
                                                0, 
                                                LastUserPassword);
            ZeroMem(LastUserPassword, sizeof(LastUserPassword));
          }else{                               // new
            Status = SecuritySetHddPassword(AtaPassThru, 
                                            Private->Current->Port, 
                                            Private->Current->PortMultiplierPort, 
                                            0, 
                                            0, 
                                            0, 
                                            Password);
            if(EFI_ERROR(Status)){goto ProcExit;}   
            SaveInputPassword(Private->Current, Password);
          }        
          Status = GetHddDeviceIdentifyData(AtaPassThru, 
                                          Private->Current->Port, 
                                          Private->Current->PortMultiplierPort, 
                                          &IdentifyData); 
          if(EFI_ERROR(Status)){goto ProcExit;}
          GetHddPasswordSecurityStatus(&IdentifyData, IfrData);
          CopyMem (&Private->Current->IfrData, IfrData, sizeof(HDD_PASSWORD_CONFIG));
          Status = EFI_SUCCESS;
        }
      }else if(IfrData->Enabled && IfrData->Locked){
        if(StateMathine==0){                              // check old password verify need?
          if(Password[0]==0){
            Status = EFI_INVALID_PARAMETER;               // need
          }
        }else if(StateMathine==1){
          if(Password[0]!=0){
// After each failed User or Master password SECURITY UNLOCK command, the counter is decremented.
// When the counter value reaches zero the EXPIRE bit (bit 4) of word 128 in the IDENTIFY DEVICE 
// information is set to one, and the SECURITY UNLOCK and SECURITY UNIT ERASE commands are command 
// aborted until the device is powered off or hardware reset.
// The counter shall be set to five after a power-on or hardware reset.						
            Status = SecurityUnlockHdd(AtaPassThru,
                                       Private->Current->Port, 
                                       Private->Current->PortMultiplierPort, 
                                       0, 
                                       Password);
            DEBUG((EFI_D_INFO, "SecurityUnlockHdd:%r\n", Status));
            NeedUpdateData = TRUE;
// always read hdd status as EXPIRE may happen.
            Status = GetHddDeviceIdentifyData(AtaPassThru, 
                                              Private->Current->Port, 
                                              Private->Current->PortMultiplierPort, 
                                              &IdentifyData);
            if(EFI_ERROR(Status)){goto ProcExit;}
            GetHddPasswordSecurityStatus(&IdentifyData, IfrData);
            DEBUG((EFI_D_INFO, "CountExpired:%d\n", IfrData->CountExpired));
            CopyMem(&Private->Current->IfrData, IfrData, sizeof(HDD_PASSWORD_CONFIG));
            if(IfrData->Locked){
              Status = EFI_NOT_READY;
            }else{
              SaveInputPassword(Private->Current, Password);
              Status = EFI_SUCCESS;
            }
          }else{
            Status = EFI_NOT_READY;
          }
        }
      }
      StateMathine++;
      break;
      
  case KEY_HDD_MASTER_PASSWORD:
    Password = HiiGetString (Private->HiiHandle, Value->string, NULL);
    if(Password==NULL){
      Status = EFI_INVALID_PARAMETER;
      goto ProcExit;       
    }
    Status = gBS->HandleProtocol(Private->Current->Controller, 
                                 &gEfiAtaPassThruProtocolGuid, 
                                 &AtaPassThru);
    if(EFI_ERROR(Status)){goto ProcExit;}     
    
    if(StateMathine==0){                  // check old password verify need?
      if(Password[0]==0){
        Status = EFI_SUCCESS;             // no need
      }
    }else if(StateMathine==1){            // set new password.
      StateMathine = (UINTN)-1;
      if(Password[0]!=0){                 // set
        Status = SecuritySetHddPassword(AtaPassThru,
                                        Private->Current->Port, 
                                        Private->Current->PortMultiplierPort, 
                                        1,            // user/master
                                        0,            // Security level
                                        MASTER_PASSWORD_MY_REVISION_CODE, 
                                        Password);
        if(EFI_ERROR(Status)){goto ProcExit;}
        Status = GetHddDeviceIdentifyData(AtaPassThru, 
                                          Private->Current->Port, 
                                          Private->Current->PortMultiplierPort, 
                                          &IdentifyData);
        if(EFI_ERROR(Status)){goto ProcExit;}
        GetHddPasswordSecurityStatus(&IdentifyData, IfrData);
        CopyMem(&Private->Current->IfrData, IfrData, sizeof(HDD_PASSWORD_CONFIG));
        Status = EFI_SUCCESS;
      }else{                              // clear
        if(!IfrData->MasterPasswordStatus){
          Status = EFI_INVALID_PARAMETER;
        }else{
          CHAR16  DefaultMasterPassword[32+1];
          ZeroMem(DefaultMasterPassword, sizeof(DefaultMasterPassword));
          StrCpy(DefaultMasterPassword, gDefaultMasterPassword);
          Status = SecuritySetHddPassword(AtaPassThru, 
                                          Private->Current->Port, 
                                          Private->Current->PortMultiplierPort, 
                                          1,            // user/master
                                          0,            // Security level
                                          MASTER_PASSWORD_MANUFACTURER_REVISION_CODE, 
                                          DefaultMasterPassword);
          ZeroMem(DefaultMasterPassword, sizeof(DefaultMasterPassword));
          if(EFI_ERROR(Status)){goto ProcExit;}
          Status = GetHddDeviceIdentifyData(AtaPassThru, 
                                            Private->Current->Port, 
                                            Private->Current->PortMultiplierPort, 
                                            &IdentifyData);
          if(EFI_ERROR(Status)){goto ProcExit;}
          GetHddPasswordSecurityStatus(&IdentifyData, IfrData);
          CopyMem(&Private->Current->IfrData, IfrData, sizeof(HDD_PASSWORD_CONFIG));
          Status = EFI_SUCCESS;
        }  
      }
    }
    StateMathine++;
    break;
    
  default:
    if ((QuestionId >= KEY_HDD_DEVICE_ENTRY_BASE) && (QuestionId < (mNumberOfHddDevices + KEY_HDD_DEVICE_ENTRY_BASE))) {
      //
      // In case goto the device configuration form, update the device form title.
      //
      ConfigFormEntry = HddPasswordGetConfigFormEntryByIndex ((UINT32) (QuestionId - KEY_HDD_DEVICE_ENTRY_BASE));
      ASSERT (ConfigFormEntry != NULL);

      DeviceFormTitleToken = (EFI_STRING_ID) STR_IDE_SECURITY_HD;
      HiiSetString (Private->HiiHandle, DeviceFormTitleToken, ConfigFormEntry->HddString, NULL);

      CopyMem (IfrData, &ConfigFormEntry->IfrData, sizeof (HDD_PASSWORD_CONFIG));
      Private->Current = ConfigFormEntry;
    }

    break;
  }

  if (!EFI_ERROR (Status) || NeedUpdateData) {
    //
    // Pass changed uncommitted data back to Form Browser
    //
    rc = HiiSetBrowserData(
           &mHddPasswordVendorGuid, 
           mHddPasswordVendorStorageName, 
           sizeof (HDD_PASSWORD_CONFIG), 
           (UINT8 *)IfrData, 
           NULL
           );
    DEBUG((EFI_D_INFO, "Status:%r, HiiSetBrowserData:%d\n", Status, rc));
  }

ProcExit:
  if(Password != NULL){
    ZeroMem(Password, StrSize(Password));
    FreePool(Password);
  }  
  FreePool (IfrData);
  return Status;
}

/**
  Updates the HDD Password configuration form to add an entry for the attached
  ata harddisk device specified by the Controller.

  @param[in]  Private      Pointer to private instance.
  @param[in]  Controller   The controller handle of the attached ata harddisk device.
  @param[in]  DevicePath   Pointer of device path node to the attached harddisk.

  @retval EFI_SUCCESS             The Hdd Password configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HddPasswordConfigUpdateForm (
  IN EFI_HII_HANDLE              HiiHandle,
  IN EFI_ATA_PASS_THRU_PROTOCOL  *AtaPassThru,
  IN EFI_HANDLE                  Controller,
  IN UINT8                       HostSCC,
  IN UINT16                      Port,
  IN UINT16                      PortMultiplierPort
  )
{
  LIST_ENTRY                       *Entry;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  BOOLEAN                          EntryExisted;
  EFI_STATUS                       Status;
  UINT16                           FormIndex;
  VOID                             *StartOpCodeHandle;
  VOID                             *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL               *StartLabel;
  EFI_IFR_GUID_LABEL               *EndLabel;
  CHAR16                           HddString[40 + 1];
  ATA_IDENTIFY_DATA                IdentifyData;
  CHAR16                           Password[32 + 1];

  ConfigFormEntry = NULL;
  EntryExisted    = FALSE;

  EFI_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    if ((ConfigFormEntry->Controller == Controller) &&
		(ConfigFormEntry->HostSCC == HostSCC) &&
        (ConfigFormEntry->Port == Port) && 
        (ConfigFormEntry->PortMultiplierPort == PortMultiplierPort)) {
      EntryExisted = TRUE;
      break;
    }
  }

  if (EntryExisted) {
    return EFI_ALREADY_STARTED;
  } else {
    //
    // Add a new form.
    //
    ConfigFormEntry = AllocateZeroPool (sizeof (HDD_PASSWORD_CONFIG_FORM_ENTRY));
    if (ConfigFormEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    InitializeListHead (&ConfigFormEntry->Link);
    ConfigFormEntry->Controller = Controller;
	  ConfigFormEntry->HostSCC    = HostSCC;
    ConfigFormEntry->Port       = Port;
    ConfigFormEntry->PortMultiplierPort = PortMultiplierPort;
    ConfigFormEntry->InputFlag  = FALSE;
    //
    // Get attached harddisk model number
    //
    Status = GetHddDeviceIdentifyData (AtaPassThru, Port, PortMultiplierPort, &IdentifyData);
    ASSERT (!EFI_ERROR (Status));

    //
    // Check the device security status. 
    // If it's locked, popup a window to wait user input.
    //
    GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);

    GetHddDeviceModelNumber (&IdentifyData, HddString);

    //
    // Compose the Port string and create a new EFI_STRING_ID.
    //
//  UnicodeSPrint (ConfigFormEntry->HddString, 64, L"HDD %d:%s", mNumberOfHddDevices, HddString);
    UnicodeSPrint (ConfigFormEntry->HddString, 
                   sizeof(ConfigFormEntry->HddString), 
                   L"HDD %d:%s", mNumberOfHddDevices, HddString);
    ConfigFormEntry->TitleToken = HiiSetString (HiiHandle, 0, ConfigFormEntry->HddString, NULL);

    //
    // Compose the help string of this port and create a new EFI_STRING_ID.
    //
    ConfigFormEntry->TitleHelpToken = HiiSetString (HiiHandle, 0, L"Set HDD Password", NULL);

    InsertTailList (&mHddPasswordConfigFormList, &ConfigFormEntry->Link);

    if(ConfigFormEntry->IfrData.Supported && ConfigFormEntry->IfrData.Enabled &&
      !ConfigFormEntry->IfrData.Locked){
      mNumberOfHddDevicesLockEnAndUnlocked++;
      HddPasswordSetResetStatusVar(TRUE, 0xFF);
    }

    if (ConfigFormEntry->IfrData.Supported &&
        ConfigFormEntry->IfrData.Enabled &&
        ConfigFormEntry->IfrData.Locked &&
        ConfigFormEntry->IfrData.Frozen == 0 &&
        ConfigFormEntry->IfrData.CountExpired == 0
        ) {
      //
      // In this case, we should unlock hdd to get access
      //
      UINTN RetryCount = 0;
			CONST UINTN MaxRetryCount = 3;
			while(++RetryCount){
				HddPasswordClearScreen();
	      Status = PopupHddPasswordInputWindows(ConfigFormEntry->HddString, Password);
	      if (!EFI_ERROR (Status)) {
	        SecurityUnlockHdd(AtaPassThru, Port, PortMultiplierPort, 0, Password);
	        Status = GetHddDeviceIdentifyData (AtaPassThru, Port, PortMultiplierPort, &IdentifyData);
	        ASSERT (!EFI_ERROR (Status));
					
	        GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);
	        if (ConfigFormEntry->IfrData.Locked != 1) {					// unlocked
	          Status = AddHddDeviceNode (ConfigFormEntry->Controller, ConfigFormEntry->HostSCC, ConfigFormEntry->Port, ConfigFormEntry->PortMultiplierPort, Password);
	          ASSERT (!EFI_ERROR (Status));
						break;
	        }else{
            if(RetryCount<MaxRetryCount){            
							ShowResultAndWaitCRKey(L"Invalid Password. Please try again");
            }
	        }
 	      }else{
	        if(Status == EFI_ABORTED){					// given up by pressing 'ESC' key.
	          ShowResultAndWaitCRKey(L"User Abort. HDD is Locked!");
            break;
	        }else if(Status == EFI_NOT_READY){	// Nothing input but a ENTER KEY
	          if(RetryCount<MaxRetryCount){
	            ShowResultAndWaitCRKey(L"Invalid Password. Please try again");
	          }
	        }
	      }		// else
 
				if(RetryCount>=MaxRetryCount){
          ShowResultAndWaitCRKey(L"Invalid Password. Hdd is Locked!");
					break;
				}
			}
			ZeroMem (Password, StrSize (Password));		// clear the temporary password buffer for security.
			HddPasswordClearScreen();
    }

    mNumberOfHddDevices++;
  }

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = HDD_DEVICE_ENTRY_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = HDD_DEVICE_LABEL_END;

  FormIndex = 0;
  EFI_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    HiiCreateGotoOpCode (
      StartOpCodeHandle,                                // Container for dynamic created opcodes
      FORMID_HDD_DEVICE_FORM,                           // Target Form ID
      ConfigFormEntry->TitleToken,                      // Prompt text
      ConfigFormEntry->TitleHelpToken,                  // Help text
      EFI_IFR_FLAG_CALLBACK,                            // Question flag
      (UINT16)(KEY_HDD_DEVICE_ENTRY_BASE + FormIndex)   // Question ID
      );

    FormIndex++;
  }

  HiiUpdateForm (
    HiiHandle,
    &mHddPasswordVendorGuid,
    FORMID_HDD_MAIN_FORM,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  Ata Pass Thru Protocol notification event handler.

  Check attached harddisk status to see if it's locked. If yes, then pop up a password windows to require user input.
  It also registers a form for user configuration on Hdd password configuration. 
  
  For S3 resume path, it registers a smi handler to unlock the locked harddisk.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
HddPasswordNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA     *Private;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT16                            Port;
  UINT16                            PortMultiplierPort;
  EFI_HANDLE                        Controller;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  UINT8                             HostSCC;

  Private = (HDD_PASSWORD_DXE_PRIVATE_DATA *)Context;
  
  //
  // Locate all handles of AtaPassThru protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiAtaPassThruProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Check attached hard disk status to see if it's locked
  //
  for (Index = 0; Index < HandleCount; Index += 1) {
    Controller = HandleBuffer[Index];
    Status = gBS->HandleProtocol (
                    Controller,
                    &gEfiAtaPassThruProtocolGuid,
                    (VOID **) &AtaPassThru
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo
                  );
    ASSERT_EFI_ERROR (Status);
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, 0x0A, 1, &HostSCC);
    ASSERT_EFI_ERROR (Status);
	
    //
    // traverse all attached harddisk devices to update form and unlock it
    //    
    Port = 0xFFFF;

    while (TRUE) {
      Status = AtaPassThru->GetNextPort (AtaPassThru, &Port);
      if (EFI_ERROR (Status)) {
        //
        // We cannot find more legal port then we are done.
        //
        break;
      } 
     
      PortMultiplierPort = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextDevice (AtaPassThru, Port, &PortMultiplierPort);
        if (EFI_ERROR (Status)) {
          //
          // We cannot find more legal port multiplier port number for ATA device
          // on the port, then we are done.
          //
          break;
        }
        //
        // Find out the attached harddisk devices.
        // Try to add a HDD Password configuration page for the attached devices.
        //
        Status = HddPasswordConfigUpdateForm (Private->HiiHandle, AtaPassThru, Controller, HostSCC, Port, PortMultiplierPort);
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }
  }

  FreePool (HandleBuffer);
  return ;
}

/**
  Initialize the HDD Password configuration form.

  @param[out] Instance      Pointer to private instance.

  @retval EFI_SUCCESS              The HDD Password configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
HddPasswordConfigFormInit (
  OUT HDD_PASSWORD_DXE_PRIVATE_DATA    **Instance
  )
{
  EFI_STATUS                       Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA    *Private;
  HDD_PASSWORD_NV_CONFIG           NvConfig;
  UINTN                            DataSize;  

  InitializeListHead(&mHddPasswordConfigFormList);

  Status = gBS->GetNextMonotonicCount(&gMonotonicCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "GetNextMonotonicCount() in %a:%r\n", __FUNCTION__, Status));
    return Status;
  }  
  
  Private = AllocateZeroPool (sizeof (HDD_PASSWORD_DXE_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature   = HDD_PASSWORD_DXE_PRIVATE_SIGNATURE;

  Private->ConfigAccess.ExtractConfig = HddPasswordFormExtractConfig;
  Private->ConfigAccess.RouteConfig   = HddPasswordFormRouteConfig;
  Private->ConfigAccess.Callback      = HddPasswordFormCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHddPasswordHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Private->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  Private->HiiHandle = HiiAddPackages (
                         &mHddPasswordVendorGuid,
                         Private->DriverHandle,
                         HddPasswordDxeStrings,
                         HddPasswordBin,
                         NULL
                         );
  if (Private->HiiHandle == NULL) {
    FreePool(Private);
    return EFI_OUT_OF_RESOURCES;
  }

  *Instance = Private;
  
  HddPasswordSetResetStatusVar(FALSE, FALSE);
  
  Private->SetupSaveNotify.SaveValue       = NvSaveValue;
  Private->SetupSaveNotify.DiscardValue    = NvDiscardValue;
  Private->SetupSaveNotify.LoadDefault     = NvLoadDefault;
  Private->SetupSaveNotify.SaveUserDefault = NvSaveUserDefault;
  Private->SetupSaveNotify.LoadUserDefault = NvLoadUserDefault;
  Private->SetupSaveNotify.IsSetupDataChanged   = IsNvDataChanged;
  Status = gBS->InstallProtocolInterface (
                  &Private->DriverHandle,
                  &gSetupSaveNotifyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->SetupSaveNotify
                  );  
  ASSERT_EFI_ERROR (Status);
  
  DataSize = sizeof(NvConfig);
  Status = gRT->GetVariable(
                  mHddPasswordVendorNVStorName,
                  &mHddPasswordVendorGuid,
                  NULL,
                  &DataSize,
                  &NvConfig
                  );
  if(!EFI_ERROR(Status)){
    CopyMem(&Private->NvConfig, &NvConfig, sizeof(NvConfig));   // Init Private Data.
    CopyMem(&gHpwPostNvConfig,  &NvConfig, sizeof(NvConfig));
  }else{
    CopyMem(&Private->NvConfig, &gHpwDefaultNvConfig, sizeof(gHpwDefaultNvConfig));
    CopyMem(&gHpwPostNvConfig,  &gHpwDefaultNvConfig, sizeof(gHpwDefaultNvConfig));
    Status = gRT->SetVariable (
                    mHddPasswordVendorNVStorName,
                    &mHddPasswordVendorGuid,
                    HDDPASSWORD_NVSAVE_VAR_ATTRIBUTE,
                    sizeof(gHpwPostNvConfig),
                    &gHpwPostNvConfig
                    );    
  }   
  
  return Status;
}


STATIC
BOOLEAN
CLearHddPasswordS3VariableIfNeeded (
  )
{
  EFI_STATUS           Status;
  UINTN                DeviceListSize;
  EFI_HDD_DEVICE_LIST  *DeviceList;
  BOOLEAN              Existed;
  
// if this time there is no hdd being unlocked, we should clear
// last saved password info. And HddPasswordSmi will not check
// Hdd Status.
// This action may speed up S3 resume at certain condition.
//
// if GetVariable() return EFI_BUFFER_TOO_SMALL, Attribute will 
// not be filled.

  Existed    = FALSE;
  DeviceList = NULL;
  DeviceListSize = 0;
  Status = gRT->GetVariable (
                  HDDPASSWORD_S3UNLOCK_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  &DeviceListSize,
                  DeviceList
                  );
  if(Status == EFI_NOT_FOUND){                          // no exist
    return EFI_SUCCESS;
  }else if(Status == EFI_BUFFER_TOO_SMALL){             // exist
    if(mNumberOfHddDevicesLockEnAndUnlocked == 0){      // clear old unused password
      DeviceList = NULL;
      DeviceListSize = 0;      
      Status = gRT->SetVariable (
                      HDDPASSWORD_S3UNLOCK_VAR_NAME,
                      &gEfiHddPasswordSecurityVariableGuid,
                      HDDPASSWORD_S3UNLOCK_VAR_ATTRIBUTE, 
                      DeviceListSize,
                      DeviceList
                      );
    }else{
      Existed = TRUE;
    }
  }
  
  return Existed;
}



STATIC
EFI_STATUS
SaveAllSBHddsInfoToVariableForS3Frozen()
{
  LIST_ENTRY                       *Entry;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  UINTN                            VarSize;
  EFI_ALL_HDD_DEVICE_LIST          *List;
  EFI_ALL_HDD_DEVICE_LIST          *ListAlloc;    
  EFI_PCI_IO_PROTOCOL              *PciIo;
  UINTN                            SegNum;
  UINTN                            BusNum;
  UINTN                            DevNum;
  UINTN                            FuncNum;
  EFI_STATUS                       Status;
  
  VarSize   = 0;
  Status    = EFI_SUCCESS;
  ListAlloc = NULL;
  
  EFI_LIST_FOR_EACH(Entry, &mHddPasswordConfigFormList){
    VarSize = VarSize + sizeof(EFI_ALL_HDD_DEVICE_LIST);
  }
  
  ListAlloc = AllocatePool(VarSize);
  if(ListAlloc==NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  List = ListAlloc;
  EFI_LIST_FOR_EACH(Entry, &mHddPasswordConfigFormList){
    ConfigFormEntry = BASE_CR(Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);
    Status = gBS->HandleProtocol (
                    ConfigFormEntry->Controller,
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    ASSERT_EFI_ERROR(Status);
    Status = PciIo->GetLocation (
                      PciIo,
                      &SegNum,
                      &BusNum,
                      &DevNum,
                      &FuncNum
                      );
    ASSERT_EFI_ERROR(Status);      
    List->Bus                 = (UINT8)BusNum;
    List->Device              = (UINT8)DevNum;
    List->Function            = (UINT8)FuncNum;
    List->Port                = ConfigFormEntry->Port;
    List->PortMultiplierPort  = ConfigFormEntry->PortMultiplierPort;
    List++;
  }
  Status = gRT->SetVariable (
                  HDDPASSWORD_ALLHDD_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  HDDPASSWORD_ALLHDD_VAR_ATTRIBUTE, 
                  VarSize,
                  ListAlloc
                  );

ProcExit:  
  if(ListAlloc!=NULL){
    FreePool(ListAlloc); 
  }
  return Status;
}


BOOLEAN IsNeedFrozenAllHdd()
{
  HDD_PASSWORD_NV_CONFIG  NvConfig;
  UINTN                   DataSize;
  BOOLEAN                 rc;
  EFI_STATUS              Status;
  
  DataSize = sizeof(NvConfig);
  Status = gRT->GetVariable(
                  mHddPasswordVendorNVStorName,
                  &mHddPasswordVendorGuid,
                  NULL,
                  &DataSize,
                  &NvConfig
                  );
  ASSERT(!EFI_ERROR(Status));
  
  if(NvConfig.FrozenAllHdds){
    rc = TRUE;
  }else{
    rc = FALSE; 
  }
  
  return rc;
}






/**
  Frozen all Hdds connected to the controller which has AtaPassThruProtocol.
**/
STATIC
EFI_STATUS 
FrozenAllSBHdds()
{
  EFI_STATUS                        Status;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  UINT16                            Port;
  UINT16                            PortMultiplierPort;
  EFI_HANDLE                        Controller;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  ATA_IDENTIFY_DATA                 *IdentifyData;
  BOOLEAN                           Frozen;

  IdentifyData = NULL;
  HandleBuffer = NULL;

  IdentifyData = AllocatePool(sizeof(ATA_IDENTIFY_DATA));
  if(IdentifyData==NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiAtaPassThruProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  for(Index = 0; Index < HandleCount; Index++){
    Controller = HandleBuffer[Index];
    Status = gBS->HandleProtocol (
                    Controller,
                    &gEfiAtaPassThruProtocolGuid,
                    (VOID **)&AtaPassThru
                    );
    Port = 0xFFFF;
    while(1){
      Status = AtaPassThru->GetNextPort(AtaPassThru, &Port);
      if(EFI_ERROR(Status)){
        break;
      } 
      PortMultiplierPort = 0xFFFF;
      while(1){
        Status = AtaPassThru->GetNextDevice(AtaPassThru, Port, &PortMultiplierPort);
        if(EFI_ERROR(Status)){
          break;
        }
        Status = GetHddDeviceIdentifyData(AtaPassThru, Port, PortMultiplierPort, IdentifyData);
        if(EFI_ERROR(Status)){
          continue;
        }
        Frozen = (IdentifyData->security_status & BIT3)?TRUE:FALSE;
        if(!Frozen){
          Status = SecurityFrozenLock(AtaPassThru, Port, PortMultiplierPort);
        }
      }
    }
  }


ProcExit:
  if(HandleBuffer!=NULL){
    FreePool(HandleBuffer);
  }
  if(IdentifyData!=NULL){
    FreePool(IdentifyData);
  }
  return Status;
}



CONST HDD_HC_PCI_REGISTER_SAVE gHddHcRegisterSaveTemplate[] = {
  0x9,  EfiBootScriptWidthUint8,  0, 0,
  0x10, EfiBootScriptWidthUint32, 0, 0,
  0x14, EfiBootScriptWidthUint32, 0, 0,
  0x18, EfiBootScriptWidthUint32, 0, 0,
  0x1c, EfiBootScriptWidthUint32, 0, 0,
  0x20, EfiBootScriptWidthUint32, 0, 0,
  0x24, EfiBootScriptWidthUint32, 0, 0,
  0x4,  EfiBootScriptWidthUint8,  0, 0,
};


STATIC
EFI_STATUS
PassHddInfoToSmram (
  VOID
  )
{
  EFI_LEGACY_BIOS_PROTOCOL          *LegacyBios;
  BDA_STRUC                         *Bda;
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  UINTN                             SubIndex;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINTN                             SegNum;
  UINTN                             BusNum;
  UINTN                             DevNum;
  UINTN                             FuncNum;
  HDD_HC_PCI_REGISTER_SAVE          *Entry;
  VOID                              *HddHcRegSaveBase;  
  UINTN                             TotalSize;

  
  HandleBuffer     = NULL; 
  HddHcRegSaveBase = NULL;  
  
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (!EFI_ERROR (Status)) {
    Bda = (BDA_STRUC *) ((UINTN) 0x400);
    ZeroMem (Bda->Bda1E_3D, sizeof (Bda->Bda1E_3D));
  }    
  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiAtaPassThruProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || HandleCount==0) {
    goto ProcExit;
  }

  TotalSize = sizeof(gHddHcRegisterSaveTemplate) * HandleCount;
  HddHcRegSaveBase = AllocateRuntimePool(TotalSize);
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  ZeroMem(HddHcRegSaveBase, sizeof(TotalSize));

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID**)&PciIo,
                    NULL,
                    HandleBuffer[Index],
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR(Status);

    Status = PciIo->GetLocation (
                      PciIo,
                      &SegNum,
                      &BusNum,
                      &DevNum,
                      &FuncNum
                      );
    ASSERT_EFI_ERROR (Status);

    Entry = (HDD_HC_PCI_REGISTER_SAVE*)((UINT8*)HddHcRegSaveBase + Index*sizeof(gHddHcRegisterSaveTemplate));
    for (SubIndex = 0; SubIndex < (sizeof(gHddHcRegisterSaveTemplate) / sizeof (HDD_HC_PCI_REGISTER_SAVE)); SubIndex++) {
      Entry[SubIndex].Address = (UINT32)PCI_LIB_ADDRESS(BusNum, DevNum, FuncNum, gHddHcRegisterSaveTemplate[SubIndex].Address);
      Entry[SubIndex].Width   = gHddHcRegisterSaveTemplate[SubIndex].Width;
      Status = PciIo->Pci.Read(PciIo, Entry[SubIndex].Width, gHddHcRegisterSaveTemplate[SubIndex].Address, 1, &Entry[SubIndex].Value);      
      ASSERT_EFI_ERROR(Status);
    }
  }

  Status = gRT->SetVariable (
                  HDDPASSWORD_HOSTINFO_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  HOST_INFO_VAR_ATTRIBUTE, 
                  TotalSize,
                  HddHcRegSaveBase
                  );
  
  IoWrite8(0xB2, SW_SMI_HDD_SAVE_PASSWORD);
  
  Status = gRT->SetVariable (
                  HDDPASSWORD_HOSTINFO_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  HOST_INFO_VAR_ATTRIBUTE, 
                  0,
                  NULL
                  );  
  
ProcExit:
  if(HandleBuffer == NULL){
    FreePool(HandleBuffer);  
  }
  if(HddHcRegSaveBase == NULL){
    FreePool(HddHcRegSaveBase);
  }
  return Status;
}




/**
  Do something when event ReadyToBoot is signalled.

  @param  Event        Event whose notification function is being invoked
  @param  Context      Pointer to the notification function's context
**/
VOID
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gBS->CloseEvent(Event);                 // once
  
  if(CLearHddPasswordS3VariableIfNeeded()){
    PassHddInfoToSmram();
  }
  
  if(IsNeedFrozenAllHdd()){
    SaveAllSBHddsInfoToVariableForS3Frozen();
  }  
  
  if(IsNeedFrozenAllHdd()){
    FrozenAllSBHdds();
  }
}






/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
HddPasswordDxeInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                     Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA  *Private;
  EFI_EVENT                      Event;  
  VOID                           *Registration;


  Status = HddPasswordConfigFormInit(&Private);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  EfiCreateProtocolNotifyEvent (
    &gEfiAtaPassThruProtocolGuid,
    TPL_CALLBACK,
    HddPasswordNotificationEvent,
    (VOID*)Private,
    &Registration
    );

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnReadyToBoot,
             NULL,
             &Event
             );

ProcExit:             
  return Status;
}




