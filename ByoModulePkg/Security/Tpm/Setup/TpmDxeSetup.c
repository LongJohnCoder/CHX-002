/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TpmDxeSetup.c

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


//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/*++

Copyright (c)  2003 - 2010 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:
  SecurityCallback.c

Abstract:
  Updates the IFR with runtime information.

Revision History:

Bug 2733: Can not configured TPM option in BIOS TPM Setup menu .
TIME: 2011-8-16
$AUTHOR: Ken Zhu
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Add one new function InitTpmStrings(),This function will be called in the initialization setup.
$END--------------------------------------------------------------------
--*/


#include "TpmSetup.h"

HII_VENDOR_DEVICE_PATH  gTpmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TPM_SETUP_CONFIG_GUID
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

STATIC EFI_GUID gTpmSetupConfigGuid = TPM_SETUP_CONFIG_GUID;
STATIC CHAR16 *gTpmSetupConfigName  = L"TPM_SETUP_CONFIG";





UINT8
TpmCurrentState (
  IN     UINT8  Enable,
  IN     UINT8  Activate
  )
{
  if (Enable) {
    if (Activate) {
      return 2;
    } else {
      return 1;
    }
  } else {
    return 0;
  }
}



#define H2NS(x) ((((x) << 8) | ((x) >> 8)) & 0xffff)
#define H2NL(x) (H2NS ((x) >> 16) | (H2NS ((x) & 0xffff) << 16))

EFI_STATUS
SimpleTpmCommand (
  IN     TPM_COMMAND_CODE          Ordinal,
  IN     UINTN                     AdditionalParameterSize,
  IN     VOID                      *AdditionalParameters
  )
{
  EFI_STATUS                       Status;
  TPM_RQU_COMMAND_HDR              *TpmRqu;
  TPM_RSP_COMMAND_HDR              TpmRsp;
  UINT32                           Size;
  EFI_TCG_PROTOCOL                 *TcgProtocol;
  
  Status = gBS->LocateProtocol(&gEfiTcgProtocolGuid, NULL, &TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TpmRqu = (TPM_RQU_COMMAND_HDR*) AllocatePool (
                                   sizeof (*TpmRqu) + AdditionalParameterSize
                                   );
  ASSERT (TpmRqu != NULL);

  TpmRqu->tag       = H2NS (TPM_TAG_RQU_COMMAND);
  Size              = (UINT32)(sizeof (*TpmRqu) + AdditionalParameterSize);
  TpmRqu->paramSize = H2NL (Size);
  TpmRqu->ordinal   = H2NL (Ordinal);
  CopyMem (TpmRqu + 1, AdditionalParameters, AdditionalParameterSize);

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          Size,
                          (UINT8*)TpmRqu,
                          (UINT32)sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  FreePool (TpmRqu);
  ASSERT_EFI_ERROR (Status);
  ASSERT (TpmRsp.tag == H2NS (TPM_TAG_RSP_COMMAND));

  if (H2NL (TpmRsp.returnCode) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  return Status;
}



EFI_STATUS
GetTpmState (
     OUT BOOLEAN                   *TpmEnable, OPTIONAL
     OUT BOOLEAN                   *TpmActivated, OPTIONAL
     OUT BOOLEAN                   *PhysicalPresenceLock, OPTIONAL
     OUT BOOLEAN                   *LifetimeLock, OPTIONAL
     OUT BOOLEAN                   *CmdEnable OPTIONAL
  )
{
  EFI_STATUS                       Status;
  TPM_RSP_COMMAND_HDR              *TpmRsp;
  UINT32                           TpmSendSize;
  TPM_PERMANENT_FLAGS              *TpmPermanentFlags;
  TPM_STCLEAR_FLAGS                *VFlags;
  UINT8                            CmdBuf[64];
  EFI_TCG_PROTOCOL                 *TcgProtocol;
    
  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, &TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get TPM Permanent flags (TpmEnable, TpmActivated, LifetimeLock, CmdEnable)
  //
  if ((TpmEnable != NULL) || (TpmActivated != NULL) || (LifetimeLock != NULL) || (CmdEnable != NULL)) {
    TpmSendSize           = sizeof (TPM_RQU_COMMAND_HDR) + sizeof (UINT32) * 3;
    *(UINT16*)&CmdBuf[0]  = H2NS (TPM_TAG_RQU_COMMAND);
    *(UINT32*)&CmdBuf[2]  = H2NL (TpmSendSize);
    *(UINT32*)&CmdBuf[6]  = H2NL (TPM_ORD_GetCapability);
  
    *(UINT32*)&CmdBuf[10] = H2NL (TPM_CAP_FLAG);
    *(UINT32*)&CmdBuf[14] = H2NL (sizeof (TPM_CAP_FLAG_PERMANENT));
    *(UINT32*)&CmdBuf[18] = H2NL (TPM_CAP_FLAG_PERMANENT);

    Status = TcgProtocol->PassThroughToTpm (
                            TcgProtocol,
                            TpmSendSize,
                            CmdBuf,
                            sizeof (CmdBuf),
                            CmdBuf
                            ); 
    TpmRsp = (TPM_RSP_COMMAND_HDR*)&CmdBuf[0];
    if (EFI_ERROR (Status) || (TpmRsp->tag != H2NS (TPM_TAG_RSP_COMMAND)) || (TpmRsp->returnCode != 0)) {
      return EFI_DEVICE_ERROR;
    }
  
    TpmPermanentFlags = (TPM_PERMANENT_FLAGS *)&CmdBuf[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];

    if (TpmEnable != NULL) {
      *TpmEnable = !TpmPermanentFlags->disable;
    }

    if (TpmActivated != NULL) {
      *TpmActivated = !TpmPermanentFlags->deactivated;
    }
    if (LifetimeLock != NULL) {
      *LifetimeLock = TpmPermanentFlags->physicalPresenceLifetimeLock;
    }

    if (CmdEnable != NULL) {
      *CmdEnable = TpmPermanentFlags->physicalPresenceCMDEnable;
    }
  }
  
  //
  // Get TPM Volatile flags (PhysicalPresenceLock)
  //
  if (PhysicalPresenceLock != NULL) {
    TpmSendSize           = sizeof (TPM_RQU_COMMAND_HDR) + sizeof (UINT32) * 3;
    *(UINT16*)&CmdBuf[0]  = H2NS (TPM_TAG_RQU_COMMAND);
    *(UINT32*)&CmdBuf[2]  = H2NL (TpmSendSize);
    *(UINT32*)&CmdBuf[6]  = H2NL (TPM_ORD_GetCapability);
  
    *(UINT32*)&CmdBuf[10] = H2NL (TPM_CAP_FLAG);
    *(UINT32*)&CmdBuf[14] = H2NL (sizeof (TPM_CAP_FLAG_VOLATILE));
    *(UINT32*)&CmdBuf[18] = H2NL (TPM_CAP_FLAG_VOLATILE);

    Status = TcgProtocol->PassThroughToTpm (
                            TcgProtocol,
                            TpmSendSize,
                            CmdBuf,
                            sizeof (CmdBuf),
                            CmdBuf
                            ); 
    TpmRsp = (TPM_RSP_COMMAND_HDR*)&CmdBuf[0];
    if (EFI_ERROR (Status) || (TpmRsp->tag != H2NS (TPM_TAG_RSP_COMMAND)) || (TpmRsp->returnCode != 0)) {
      return EFI_DEVICE_ERROR;
    }
  
    VFlags = (TPM_STCLEAR_FLAGS *)&CmdBuf[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];

    if (PhysicalPresenceLock != NULL) {
      *PhysicalPresenceLock = VFlags->physicalPresenceLock;
    }   
  }
 
  return EFI_SUCCESS;  
}





EFI_STATUS
TpmPhysicalPresence (
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_PHYSICAL_PRESENCE             *TpmPp;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT8                             Buffer[sizeof (*TpmRqu) + sizeof (*TpmPp)];

  Status = gBS->LocateProtocol(&gEfiTcgProtocolGuid, NULL, &TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TpmRqu = (TPM_RQU_COMMAND_HDR*)Buffer;
  TpmPp = (TPM_PHYSICAL_PRESENCE*)(TpmRqu + 1);

  TpmRqu->tag = H2NS (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = H2NL (sizeof (Buffer));
  TpmRqu->ordinal = H2NL (TSC_ORD_PhysicalPresence);
  *TpmPp = H2NS (PhysicalPresence);

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          sizeof (Buffer),
                          (UINT8*)TpmRqu,
                          sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  ASSERT_EFI_ERROR (Status);
  ASSERT (TpmRsp.tag == H2NS (TPM_TAG_RSP_COMMAND));
  if (TpmRsp.returnCode != 0) {
    //
    // If it fails, some requirements may be needed for this command.
    //
    return EFI_SECURITY_VIOLATION;
  }
  return Status;
}





EFI_STATUS
SetOppPresent (
  VOID
  )
{
  EFI_STATUS  Status;
  BOOLEAN     LifetimeLock;
  BOOLEAN     CmdEnable;
  
  Status = GetTpmState(NULL, NULL, NULL, &LifetimeLock, &CmdEnable);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM] Get GetTpmState error Status = %r\n", Status));
    return Status;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {
      //
      // physicalPresenceCMDEnable is locked, can't change.
      //
      return EFI_ABORTED;
    }

    Status = TpmPhysicalPresence (TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Set operator physical presence flags
  // 
  Status = TpmPhysicalPresence(TPM_PHYSICAL_PRESENCE_PRESENT);
  return Status;
}










EFI_STATUS
EnableTPMDevice (
  IN  BOOLEAN  TpmEnable
  )
{
  EFI_STATUS  Status;
  
  Status = SetOppPresent();
  if(EFI_ERROR (Status)){
    return Status;
  }

  if (TpmEnable) {
    Status = SimpleTpmCommand (TPM_ORD_PhysicalEnable, 0, NULL);
    if(EFI_ERROR (Status)){
      DEBUG((EFI_D_ERROR, "[TPM] Fail to enable TPM device!\n"));
      return Status;
    }
  } else {
    Status = SimpleTpmCommand (TPM_ORD_PhysicalDisable, 0, NULL);     
    if(EFI_ERROR(Status)){
      DEBUG ((EFI_D_ERROR, "[TPM] Fail to disable TPM device!\n"));
      return Status;
    }  
  }
 
  return Status;
}



EFI_STATUS
ActivateTPMDevice (
  IN  BOOLEAN  TpmActive
  )
{
  EFI_STATUS   Status;
  BOOLEAN      TpmDeactive;

  Status = SetOppPresent();
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  TpmDeactive = !TpmActive;
  Status = SimpleTpmCommand(TPM_ORD_PhysicalSetDeactivated, sizeof(TpmDeactive), &TpmDeactive);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM] Fail to activate/deactivate TPM device!\n"));
    return Status;
  }

  return Status;
}




EFI_STATUS
TpmForceClear (
  VOID
  )
{
  EFI_STATUS  Status;
  
  Status = SetOppPresent();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SimpleTpmCommand(TPM_ORD_ForceClear, 0, NULL);        
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM] Fail to force clear TPM device!\n"));
    return Status;
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
TpmFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  TPM_SETUP_CONFIG                 *IfrData;
  TPM_SETUP_PRIVATE_DATA           *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if((Request != NULL) && !HiiIsConfigHdrMatch(Request, &gTpmSetupConfigGuid, gTpmSetupConfigName)){
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = TPM_SETUP_DATA_FROM_THIS_HII_CONFIG(This);
  IfrData = AllocateZeroPool(sizeof(TPM_SETUP_CONFIG));
  ASSERT (IfrData != NULL);
  CopyMem(IfrData, &Private->ConfigData, sizeof(TPM_SETUP_CONFIG));

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof(TPM_SETUP_CONFIG);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr(&gTpmSetupConfigGuid, 
                                             gTpmSetupConfigName, 
                                             Private->DriverHandle);
    Size = (StrLen(ConfigRequestHdr) + 32 + 1) * sizeof(CHAR16);
    ConfigRequest = AllocateZeroPool(Size);
    ASSERT(ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) IfrData,
                                BufferSize,
                                Results,
                                Progress
                                );
  FreePool (IfrData);
  //
  // Free the allocated config request string.
  //
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
    *Progress = Request + StrLen (Request);
  }

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
TpmFormRouteConfig (
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
  if (!HiiIsConfigHdrMatch (Configuration, &gTpmSetupConfigGuid, gTpmSetupConfigName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
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
TpmFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS              Status;
  TPM_SETUP_PRIVATE_DATA  *Private;
  TPM_SETUP_CONFIG        *ConfigData;
  UINT8                   TpmState;
  
  if((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_FORM_CLOSE)){
    return EFI_SUCCESS;
  }

  Private    = TPM_SETUP_DATA_FROM_THIS_HII_CONFIG(This);
  ConfigData = &Private->ConfigData;
  Status     = EFI_SUCCESS;

  switch(QuestionId){
    
    case KEY_TPM_ENABLE:    
      if(Type != EFI_IFR_TYPE_NUM_SIZE_8){
        Status = EFI_INVALID_PARAMETER;      
      }
      if(!EFI_ERROR(Status)){
        TpmState = Value->u8;
        if(ConfigData->TpmEnable != TpmState){
          Status = EnableTPMDevice((BOOLEAN)TpmState);
          if(!EFI_ERROR(Status)){
            ConfigData->TpmEnable = TpmState;
          }
        }
      }
      break;

    case KEY_TPM_ACTIVATE:
      if(Type != EFI_IFR_TYPE_NUM_SIZE_8){
        Status = EFI_INVALID_PARAMETER;      
      }
      if(!EFI_ERROR(Status)){
        TpmState = Value->u8;
        if(ConfigData->TpmState != TpmState){
          Status = ActivateTPMDevice(TpmState);
          if(!EFI_ERROR(Status)){
            ConfigData->TpmState = TpmState;
          }
        }
      }
      break;

    case KEY_TPM_FORCE_CLEAR:    
      Status = TpmForceClear();
      if(!EFI_ERROR(Status)){
        if(ConfigData->TpmState){
          ConfigData->TpmState = 0;
        }
        if(ConfigData->TpmEnable){
          ConfigData->TpmEnable = 0;
        }
      }
      break;
      
    default:
      Status = EFI_SUCCESS;
      break;          
  }

  if(!EFI_ERROR(Status)){
    HiiSetBrowserData(&gTpmSetupConfigGuid, 
                      gTpmSetupConfigName, 
                      sizeof(TPM_SETUP_CONFIG), 
                      (UINT8 *)ConfigData, 
                      NULL
                      );
  }

  return Status;
}








STATIC TPM_SETUP_PRIVATE_DATA gTpmSetupData =
{
  NULL,
  NULL,
  {
    TpmFormExtractConfig,
    TpmFormRouteConfig,
    TpmFormCallback    
  },
  {0,},
};


/**
  The driver's entry point.

  It publishes EFI TPM Setup Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
TpmSetupEntry (
  IN    EFI_HANDLE       ImageHandle,
  IN    EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS        Status;
  TPM_SETUP_CONFIG  *ConfigData;

  ConfigData = &gTpmSetupData.ConfigData;
  
  Status = GetTpmState (
             &ConfigData->TpmEnable, 
             &ConfigData->TpmState, 
             &ConfigData->PhysicalPresenceLock, 
             NULL, 
             NULL
             );
  ConfigData->TpmPresent = (BOOLEAN)(!EFI_ERROR(Status));

  ConfigData->TpmCurrentState = TpmCurrentState(ConfigData->TpmEnable, ConfigData->TpmState);
  gTpmSetupData.DriverHandle = NULL;

  Status = gBS->InstallMultipleProtocolInterfaces(
                  &gTpmSetupData.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &gTpmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gTpmSetupData.ConfigAccess,
                  NULL
                  );
  gTpmSetupData.HiiHandle = HiiAddPackages(
                         &gTpmSetupConfigGuid,
                         gTpmSetupData.DriverHandle,
                         TpmSetupStrings,
                         TpmSetupBin,
                         NULL
                         );

  return Status;
}
