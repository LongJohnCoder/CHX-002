/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordSmm.c

Abstract: 
  Hdd password SMM driver.

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

Bug 2749 - Fix S3 resume failed if HDD password enabled.
TIME: 2011-08-18
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Give more delay according to SATA spec.
$END--------------------------------------------------------------------

Bug2274: improve hdd password feature.
TIME: 2011-06-09
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. give user max 3 times to input password at post.
  2. check device is hdd or not before unlock it when S3 resume.
  3. remove personal label such as wztest.
$END--------------------------------------------------------------------

Bug2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: 
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
  HDD password smm driver which is used to support HDD security feature at s3 path.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation
(if any) is furnished under a license and may only be used or
copied in accordance with the terms of the license.  Except as
permitted by such license, no part of this software or
documentation may be reproduced, stored in a retrieval system, or
transmitted in any form or by any means without the express written
consent of Intel Corporation.            

**/

#include "HddPasswordSmm.h"

//
// To unlock the HDD password at S3 Resume, restored the following registers.
//
const HDD_HC_PCI_REGISTER_SAVE mHddHcRegisterSaveTemplate[] = {
//-  0x90, EfiBootScriptWidthUint8,  0, 0,
  0x9,  EfiBootScriptWidthUint8,  0, 0,
//-  0x3c, EfiBootScriptWidthUint8,  0, 0,
//-  0x40, EfiBootScriptWidthUint32, 0, 0,
//-  0x44, EfiBootScriptWidthUint8,  0, 0,
//-  0x48, EfiBootScriptWidthUint8,  0, 0,
//-  0x4a, EfiBootScriptWidthUint16, 0, 0,
//-  0x54, EfiBootScriptWidthUint32, 0, 0,
//-  0x74, EfiBootScriptWidthUint16, 0, 0,
//-  0x92, EfiBootScriptWidthUint16, 0, 0,
  0x10, EfiBootScriptWidthUint32, 0, 0,
  0x14, EfiBootScriptWidthUint32, 0, 0,
  0x18, EfiBootScriptWidthUint32, 0, 0,
  0x1c, EfiBootScriptWidthUint32, 0, 0,
  0x20, EfiBootScriptWidthUint32, 0, 0,
  0x24, EfiBootScriptWidthUint32, 0, 0,
  0x4,  EfiBootScriptWidthUint8,  0, 0,
};

VOID                     *mHddHcRegisterSaveBase = NULL;
UINTN                    mHddHcNumber = 0;

EFI_HDD_DEVICE_LIST      *mDeviceList = NULL;
UINTN                    mDeviceListSize = 0;

UINTN                    mAllDeviceListSize = 0;
EFI_ALL_HDD_DEVICE_LIST  *mAllDeviceList = NULL;

// DMA can not read/write data to smram, so we pre-allocates buffer from AcpiNVS.
VOID    *mBuffer     = NULL;





/**
  Decode password which is stored at variable region.

  @param  Password       The pointer to the password to be decoded.

  @return None

**/
VOID
EFIAPI
SmmDecodeHddPassword (
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
  store sata controller some pci settings

  @param  None
  
  @return None
**/
VOID
EFIAPI
StoreSataHostOriSetting()
{
  UINTN                        Index;
  UINTN                        SubIndex;
  UINTN                        Count;
  HDD_HC_PCI_REGISTER_SAVE     *HddHcSaveEntry;
  
  for (Index = 0; Index < mHddHcNumber; Index += 1) {
    HddHcSaveEntry = (HDD_HC_PCI_REGISTER_SAVE *)((UINT8 *)mHddHcRegisterSaveBase + 
		               Index * sizeof (mHddHcRegisterSaveTemplate));
    Count = sizeof (mHddHcRegisterSaveTemplate) / sizeof (HDD_HC_PCI_REGISTER_SAVE);
	
    for(SubIndex = 0; SubIndex < Count; SubIndex++){
      switch (HddHcSaveEntry[SubIndex].Width){
        case EfiBootScriptWidthUint8:
          HddHcSaveEntry[SubIndex].OriVal = PciRead8(HddHcSaveEntry[SubIndex].Address);
          break;
        case EfiBootScriptWidthUint16:
          HddHcSaveEntry[SubIndex].OriVal = PciRead16(HddHcSaveEntry[SubIndex].Address);
          break;
        case EfiBootScriptWidthUint32:
          HddHcSaveEntry[SubIndex].OriVal = PciRead32(HddHcSaveEntry[SubIndex].Address);
          break;
        default:
          ASSERT(FALSE);
          continue;
      }
    }
  }
}


/**
  restore sata controller some pci settings

  @param  None
  
  @return None
**/
VOID
EFIAPI
RestoreSataHostOriSetting()
{
  UINTN                        Index;
  UINTN                        SubIndex;
  UINTN                        Count;
  HDD_HC_PCI_REGISTER_SAVE     *HddHcSaveEntry;
  
  for (Index = 0; Index < mHddHcNumber; Index += 1) {
    HddHcSaveEntry = (HDD_HC_PCI_REGISTER_SAVE *)((UINT8 *)mHddHcRegisterSaveBase + 
		               Index * sizeof (mHddHcRegisterSaveTemplate));
    Count = sizeof (mHddHcRegisterSaveTemplate) / sizeof (HDD_HC_PCI_REGISTER_SAVE);
	
    for(SubIndex = 0; SubIndex < Count; SubIndex++){
      switch (HddHcSaveEntry[SubIndex].Width){
        case EfiBootScriptWidthUint8:
          PciWrite8(HddHcSaveEntry[SubIndex].Address, (UINT8)HddHcSaveEntry[SubIndex].OriVal);
          break;
        case EfiBootScriptWidthUint16:
          PciWrite16(HddHcSaveEntry[SubIndex].Address, (UINT16)HddHcSaveEntry[SubIndex].OriVal);
          break;
        case EfiBootScriptWidthUint32:
          PciWrite32(HddHcSaveEntry[SubIndex].Address, (UINT32)HddHcSaveEntry[SubIndex].OriVal);
          break;
        default:
          ASSERT(FALSE);
          continue;
      }
    }
  }
}



VOID
EFIAPI
RestoreSataHostPostSetting()
{
  UINTN                        Index;
  UINTN                        SubIndex;
  UINTN                        Count;
  HDD_HC_PCI_REGISTER_SAVE     *HddHcSaveEntry;
    
  for(Index = 0; Index < mHddHcNumber; Index++){
    HddHcSaveEntry = (HDD_HC_PCI_REGISTER_SAVE *)((UINT8 *)mHddHcRegisterSaveBase + 
                     Index * sizeof (mHddHcRegisterSaveTemplate));
    Count = sizeof(mHddHcRegisterSaveTemplate) / sizeof(HDD_HC_PCI_REGISTER_SAVE);
    for(SubIndex = 0; SubIndex < Count; SubIndex++) {
      switch (HddHcSaveEntry[SubIndex].Width) {
      case EfiBootScriptWidthUint8:
        PciWrite8(HddHcSaveEntry[SubIndex].Address, (UINT8)HddHcSaveEntry[SubIndex].Value);
        break;
      case EfiBootScriptWidthUint16:
        PciWrite16(HddHcSaveEntry[SubIndex].Address, (UINT16)HddHcSaveEntry[SubIndex].Value);
        break;
      case EfiBootScriptWidthUint32:
        PciWrite32(HddHcSaveEntry[SubIndex].Address, (UINT32)HddHcSaveEntry[SubIndex].Value);
        break;
      default:
        ASSERT (FALSE);
        continue;
      }
    }
  }
}








/**
  Dispatch function for a Software SMI handler.

  @param  DispatchHandle        The handle of this dispatch function.
  @param  DispatchContext       The pointer to the dispatch function's context.
                                The SwSmiInputValue field is filled in
                                by the software dispatch driver prior to
                                invoking this dispatch function.
                                The dispatch function will only be called
                                for input values for which it is registered.

  @return None

**/
EFI_STATUS
EFIAPI
SmmFrozenAllSBHddSecurity (
  IN  EFI_HANDLE                   DispatchHandle,
	IN CONST VOID                  	 *DispatchContext,
  IN OUT VOID 										 *CommBuffer,
	IN OUT UINTN										 *CommBufferSize  
  )
{
  EFI_ALL_HDD_DEVICE_LIST      *DeviceNode;
  UINTN                        Bus;
  UINTN                        Device;
  UINTN                        Function;
  UINT16                       Port;
  UINT16                       PortMultiplierPort;
  UINT8                        SubClassCode;
  UINT8                        BaseClassCode;
  EFI_IDE_REGISTERS            IdeRegisters[2];
  EFI_STATUS                   Status;
  UINTN                        Index;
  BOOLEAN                      Frozen;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  if(mAllDeviceList == NULL){
    goto ProcExit;
  }

  StoreSataHostOriSetting();
  RestoreSataHostPostSetting();

  for(Index = 0; Index < mAllDeviceListSize/sizeof(EFI_ALL_HDD_DEVICE_LIST); Index++){
    DeviceNode = (EFI_ALL_HDD_DEVICE_LIST *)((UINTN)mAllDeviceList + Index*sizeof(EFI_ALL_HDD_DEVICE_LIST));
    Bus        = DeviceNode->Bus;
    Device     = DeviceNode->Device;
    Function   = DeviceNode->Function;
    Port       = DeviceNode->Port;
    PortMultiplierPort = DeviceNode->PortMultiplierPort;

    SubClassCode  = PciRead8(PCI_LIB_ADDRESS(Bus, Device, Function, 0x0A));
    BaseClassCode = PciRead8(PCI_LIB_ADDRESS(Bus, Device, Function, 0x0B));

    DEBUG((EFI_D_INFO, "(%X,%X,%X,%X,%X)\n", Bus, Device, Function, Port, PortMultiplierPort));
    DEBUG((EFI_D_INFO, "%X,%X\n", SubClassCode, BaseClassCode));    

    if(SubClassCode == 0x01){				// IDE mode
      ASSERT (Port < EfiIdeMaxChannel);
      Status = GetIdeRegisterIoAddr(Bus, Device, Function, IdeRegisters);
      ASSERT_EFI_ERROR(Status);

      Status = WaitIDEDeviceReady(&IdeRegisters[Port]);
      if(EFI_ERROR(Status)){
        DEBUG((EFI_D_ERROR, "WaitIDEDeviceReady:%r\n", Status));
        continue; 
      }
      Status = AtaIdentify(&IdeRegisters[Port], (UINT8)Port, (UINT8)PortMultiplierPort, mBuffer, NULL);
      if(EFI_ERROR(Status)){
        DEBUG((EFI_D_ERROR, "AtaIdentify:%r\n", Status));
        continue; 
      }
      Frozen = (((ATA_IDENTIFY_DATA*)mBuffer)->security_status)&BIT3?TRUE:FALSE;
      if(!Frozen){
        Status = AtaFrozenHddSecurity(&IdeRegisters[Port], (UINT8)PortMultiplierPort, NULL);
        DEBUG((EFI_D_ERROR, "AtaFrozenHdd:%r\n", Status));
      }
    }else if((SubClassCode == 0x06) || (SubClassCode == 0x04)){      // AHCI or RAID

      Status = GetAhciBaseAddress(Bus, Device, Function);
      ASSERT_EFI_ERROR(Status);

      Status = AhciModeInitialize((UINT8)Port);
      if(EFI_ERROR(Status)){
        DEBUG((EFI_D_ERROR, "AhciModeInitialize:%r\n", Status));
        continue;
      }

      Status = AhciIdentify(&mAhciRegisters, (UINT8)Port, (UINT8)PortMultiplierPort, mBuffer);
			if(EFI_ERROR(Status)){
			  DEBUG((EFI_D_ERROR, "AhciIdentify:%r\n", Status));
        continue;
      }
      
      Frozen = (((ATA_IDENTIFY_DATA*)mBuffer)->security_status)&BIT3?TRUE:FALSE;
      if(!Frozen){
        Status = AhciFrozenHddSecurity(&mAhciRegisters, (UINT8)Port, (UINT8)PortMultiplierPort);
        DEBUG((EFI_D_INFO, "AhciFrozenHdd:%r\n", Status));
      }
    }
  }
  RestoreSataHostOriSetting();

ProcExit:
  return EFI_SUCCESS;
}



/**
  Dispatch function for a Software SMI handler.

  @param  DispatchHandle        The handle of this dispatch function.
  @param  DispatchContext       The pointer to the dispatch function's context.
                                The SwSmiInputValue field is filled in
                                by the software dispatch driver prior to
                                invoking this dispatch function.
                                The dispatch function will only be called
                                for input values for which it is registered.

  @return None

**/
EFI_STATUS
EFIAPI
SmmUnlockHddPassword (
  IN  EFI_HANDLE                    DispatchHandle,
	IN CONST VOID                  	  *DispatchContext,
  IN OUT VOID 											*CommBuffer,
	IN OUT UINTN											*CommBufferSize  
  )
{
  EFI_HDD_DEVICE_LIST          *DeviceNode;
  UINTN                        Bus;
  UINTN                        Device;
  UINTN                        Function;
  UINT8                        HostSCC;
  UINT16                       Port;
  UINT16                       PortMultiplierPort;
  UINT8                        SubClassCode;
  UINT8                        BaseClassCode;
  EFI_IDE_REGISTERS            IdeRegisters[2];
  EFI_STATUS                   Status;
  UINTN                        Index;
  BOOLEAN                      HddLock;
  CHAR8                        Password[32];

  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  if(mDeviceList == NULL){
    DEBUG((EFI_D_INFO, "No locked HDD\n"));
    goto ProcExit;
  }

  StoreSataHostOriSetting();
  RestoreSataHostPostSetting();

  for (Index = 0; Index < mDeviceListSize / sizeof (EFI_HDD_DEVICE_LIST); Index++) {
    DeviceNode = (EFI_HDD_DEVICE_LIST *)((UINTN)mDeviceList + Index * sizeof(EFI_HDD_DEVICE_LIST));
    Bus      = DeviceNode->Bus;
    Device   = DeviceNode->Device;
    Function = DeviceNode->Function;
    HostSCC  = DeviceNode->HostSCC;
    Port     = DeviceNode->Port;
    PortMultiplierPort = DeviceNode->PortMultiplierPort;

    SubClassCode  = PciRead8(PCI_LIB_ADDRESS (Bus, Device, Function, 0x0A));
    BaseClassCode = PciRead8(PCI_LIB_ADDRESS (Bus, Device, Function, 0x0B));

    DEBUG((EFI_D_INFO, "(%X,%X,%X,%X,%X)\n", Bus, Device, Function, Port, PortMultiplierPort));
    DEBUG((EFI_D_INFO, "%X,%X\n", SubClassCode, BaseClassCode));     
    
// In IDE mode, variable "Port" means Channel Number(primary/secondary), 
// and variable "PortMultiplierPort" means Device number(master/slave). 
// So we should distinguish them in different mode.
	  if(HostSCC != SubClassCode){
		  continue;
	  }
	
    if (SubClassCode == 0x01) {				// IDE mode

      ASSERT (Port < EfiIdeMaxChannel);
      Status = GetIdeRegisterIoAddr(Bus, Device, Function, IdeRegisters);
      ASSERT_EFI_ERROR(Status);

      Status = WaitIDEDeviceReady(&IdeRegisters[Port]);
      if(EFI_ERROR(Status)){
        continue; 
      }
      Status = AtaIdentify(&IdeRegisters[Port], (UINT8)Port, (UINT8)PortMultiplierPort, mBuffer, NULL);
      if(EFI_ERROR(Status)){
        continue; 
      }
      HddLock = (((ATA_IDENTIFY_DATA*)mBuffer)->security_status)&BIT2?TRUE:FALSE;
      if(HddLock){
        CopyMem(Password, DeviceNode->Password, sizeof(Password));
        SmmDecodeHddPassword(Password, sizeof (Password));
        Status = AtaUnlockHddPassword(&IdeRegisters[Port], 
                                      (UINT8)Port, 
                                      (UINT8)PortMultiplierPort, 
                                      0, 
                                      Password, 
                                      NULL
                                      );
        ZeroMem(Password, sizeof(Password));
      }
    }else if((SubClassCode == 0x06) || (SubClassCode == 0x04)){      // AHCI or RAID

      Status = GetAhciBaseAddress(Bus, Device, Function);
      ASSERT_EFI_ERROR(Status);

      Status = AhciModeInitialize((UINT8)Port);
      if (EFI_ERROR (Status)) {
        continue;				
      }

      Status = AhciIdentify(&mAhciRegisters, (UINT8)Port, (UINT8)PortMultiplierPort, mBuffer);
			if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "AhciIdentify:%r\n", Status));
        continue;
      }
      HddLock = (((ATA_IDENTIFY_DATA*)mBuffer)->security_status)&BIT2?TRUE:FALSE;
      if(HddLock){
        CopyMem (Password, DeviceNode->Password, sizeof (Password));
        SmmDecodeHddPassword(Password, sizeof (Password));
        Status = AhciUnlockHddPassword(&mAhciRegisters, 
                                       (UINT8)Port, 
                                       (UINT8)PortMultiplierPort, 
                                       0, 
                                       Password
                                       );
        DEBUG((EFI_D_ERROR, "AhciUnlockHddPassword:%r\n", Status));                               
        ZeroMem(Password, sizeof(Password));
      }
    }
  }
  RestoreSataHostOriSetting();

ProcExit:
  return EFI_SUCCESS;
}




/**
  Get device list stored at HddPasswordDxe driver from variable region.

  @param  DeviceListSize   The size of stored device list.

  @return  The pointer to the device list.

**/
EFI_HDD_DEVICE_LIST *
EFIAPI
GetHddPasswordDeviceList (
  IN OUT  UINTN         *DeviceListSize
  )
{
  EFI_STATUS               Status;
  EFI_HDD_DEVICE_LIST      *DeviceList;

  //
  // Get the locked device list from HddPasswordDxe driver.
  //
  DeviceList      = NULL;
  *DeviceListSize = 0;
  Status = gRT->GetVariable (
                  HDDPASSWORD_S3UNLOCK_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  DeviceListSize,
                  DeviceList
  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    DeviceList = AllocateRuntimePool (*DeviceListSize);
    if (DeviceList == NULL) {
      return NULL;
    }

    Status = gRT->GetVariable (
                    HDDPASSWORD_S3UNLOCK_VAR_NAME,
                    &gEfiHddPasswordSecurityVariableGuid,
                    NULL,
                    DeviceListSize,
                    DeviceList
                    );
    ASSERT_EFI_ERROR (Status);

    return DeviceList;
  } else if (Status == EFI_NOT_FOUND) {
    return NULL;
  } else {
    ASSERT (FALSE);
    return NULL;
  }
}

EFI_ALL_HDD_DEVICE_LIST *
EFIAPI
GetAllHddDeviceList (
  IN OUT  UINTN         *DeviceListSize
  )
{
  EFI_STATUS               Status;
  EFI_ALL_HDD_DEVICE_LIST  *DeviceList;

  DeviceList      = NULL;
  *DeviceListSize = 0;
  Status = gRT->GetVariable (
                  HDDPASSWORD_ALLHDD_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  DeviceListSize,
                  DeviceList
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    DeviceList = AllocateRuntimePool(*DeviceListSize);
    if(DeviceList == NULL){
      return NULL;
    }

    Status = gRT->GetVariable (
                    HDDPASSWORD_ALLHDD_VAR_NAME,
                    &gEfiHddPasswordSecurityVariableGuid,
                    NULL,
                    DeviceListSize,
                    DeviceList
                    );
    ASSERT_EFI_ERROR (Status);

    return DeviceList;
  } else if (Status == EFI_NOT_FOUND) {
    return NULL;
  } else {
    ASSERT (FALSE);
    return NULL;
  }
}

VOID *
EFIAPI
GetHostInfoList (
  IN OUT  UINTN         *InfoListCount
  )
{
  EFI_STATUS              Status;
  VOID                    *InfoList;

  InfoList       = NULL;
  *InfoListCount = 0;
  Status = gRT->GetVariable (
                  HDDPASSWORD_HOSTINFO_VAR_NAME,
                  &gEfiHddPasswordSecurityVariableGuid,
                  NULL,
                  InfoListCount,
                  InfoList
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    InfoList = AllocateRuntimePool(*InfoListCount);
    if(InfoList == NULL){
      return NULL;
    }

    Status = gRT->GetVariable (
                    HDDPASSWORD_HOSTINFO_VAR_NAME,
                    &gEfiHddPasswordSecurityVariableGuid,
                    NULL,
                    InfoListCount,
                    InfoList
                    );
    ASSERT_EFI_ERROR (Status);

    *InfoListCount = *InfoListCount/sizeof(mHddHcRegisterSaveTemplate);
    return InfoList;
  } else if (Status == EFI_NOT_FOUND) {
    return NULL;
  } else {
    ASSERT (FALSE);
    return NULL;
  }
}




/**
  Register a callback function for SmmReadyToBoot protocol.
  This function is used to save Ide/Sata controller's configuration at boot time,
  S3 path will restore them to access attached hard disk devices.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @return None

**/
EFI_STATUS
EFIAPI
SmmSaveHddPassword (
  IN  EFI_HANDLE                    DispatchHandle,
	IN CONST VOID                  	  *DispatchContext,
  IN OUT VOID 											*CommBuffer,
	IN OUT UINTN											*CommBufferSize  
  )
{

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  mDeviceList            = GetHddPasswordDeviceList(&mDeviceListSize);  
  mAllDeviceList         = GetAllHddDeviceList(&mAllDeviceListSize);
  mHddHcRegisterSaveBase = GetHostInfoList(&mHddHcNumber);
 
  return EFI_SUCCESS;
}


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
HddPasswordSmmInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_HANDLE                                SwHandle;
  EFI_SMM_SW_REGISTER_CONTEXT               SwContext;

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    &SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Preallocate a 512 bytes buffer to store Identify/Unlock cmd payload.
  // It's because DMA can not access smmram stack at the cmd execution.
  //
  mBuffer = (VOID *)(UINTN)0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(512),
                  (EFI_PHYSICAL_ADDRESS *)&mBuffer
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem ((VOID *)(UINTN)mBuffer, 512);
  Status = AhciAllocateResource();
  ASSERT_EFI_ERROR (Status);

  //
  // Register Hdd password smm unlock handler
  //
  SwContext.SwSmiInputValue = SW_SMI_HDD_UNLOCK_PASSWORD;
  Status = SwDispatch->Register (
               SwDispatch,
               SmmUnlockHddPassword,
               &SwContext,
               &SwHandle
               );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = SW_SMI_HDD_SAVE_PASSWORD;
  Status = SwDispatch->Register (
               SwDispatch,
               SmmSaveHddPassword,
               &SwContext,
               &SwHandle
               );
  ASSERT_EFI_ERROR (Status);
  
  SwContext.SwSmiInputValue = SW_SMI_HDD_FROZEN_HDD;
  Status = SwDispatch->Register (
               SwDispatch,
               SmmFrozenAllSBHddSecurity,
               &SwContext,
               &SwHandle
               );
  ASSERT_EFI_ERROR(Status);

  return Status;
}

