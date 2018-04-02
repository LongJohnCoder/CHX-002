/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmDxe.c

Abstract: 
  Dxe part of TCM Module.

Revision History:

Bug 3341 - Add retry mechanism for more tcm command to imporve stability. 
TIME: 2012-02-03
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Max retry count is 3.
  2. remove all ASSERT() in TcmDxe.c
$END------------------------------------------------------------

bug 3312 - update tcm settings. 
TIME: 2012-01-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Update Tcm.BsCap.HashAlgorithmBitmap to 0x0D.
  2. Always allocate Acpi EventLog memory even if tcm is deactived.
$END------------------------------------------------------------

Bug 3282 - improve TCM action stability.
TIME: 2012-01-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. update command flow as porting guide request.
  2. use retry mechanism.
$END------------------------------------------------------------

Bug 3223 - package ZTE SM3 hash source to .efi for ZTE's copyrights.
TIME: 2011-12-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use ppi or protocol to let hash be independent.
$END------------------------------------------------------------

Bug 3216 - add Tcm SW SM3 hash support.
TIME: 2011-12-13
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Use ZTE lib to do sm3 hash.
$END------------------------------------------------------------

Bug 3144 - Add Tcm Measurement Architecture.
TIME: 2011-11-24
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. PEI: Measure CRTM Version.
          Measure Main Bios.
  2. DXE: add 'TCPA' acpi table.
          add event log.
          Measure Handoff Tables.
          Measure All Boot Variables.
          Measure Action.
  Note: As software of SM3's hash has not been implemented, so hash
        function is invalid.
$END------------------------------------------------------------

Bug 3075 - Add TCM support.
TIME: 2011-11-14
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Tcm module init version.
     Only support setup function.
$END------------------------------------------------------------

**/


/*++
  This file contains 'Framework Code' and is licensed as such   
  under the terms of your license agreement with Intel or your  
  vendor.  This file may not be modified, except as allowed by  
  additional terms of your license agreement.                   
--*/
/** @file
  
  This module implements TCG EFI Protocol and TCG Platform protocol.
  
Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcgDxe.c

Abstract:

  TCG EFI protocol implementation conform to TCG_EFI_Protocol_1_20_Final_Rev_1.00

  See http://www.trustedcomputinggroup.org for latest specification
  for the interface definition

Note:
  The PE/COFF measurement algorithm is based upon the authenticode image hashing in
  MSFT PE/COFF Specification 8.0 Appendix A.
**/




#include "TcmDxe.h"
#include <Guid/TcmSetupCfgGuid.h>


#define TCM_MEM_LOG(x)

//--------------------------------------------------------------
HII_VENDOR_DEVICE_PATH  mTcmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TCM_SETUP_CONFIG_GUID
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

STATIC CHAR16 *gTcmSetupConfigName  = L"TCM_SETUP_CONFIG";
STATIC CHAR16 *gBootVarName         = L"BootOrder";

EFI_TCM_CLIENT_ACPI_TABLE  gTcmAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof(gTcmAcpiTemplate),
    0x02,                                                 // Revision
    0,                                                    // checksum
    {'B', 'Y', 'O', 'S', 'O','F'},                        // OemId
    SIGNATURE_64('B','Y','O','S','O','F','T','\0'),       // OemTableId
    1,                                                    // OemRevision
    SIGNATURE_32('B','Y','O','\0'),                       // CreatorId
    0x01000001                                            // CreatorRevision
  },
  0,                                                      // 0 for PC Client Platform Class
  EFI_TCM_LOG_AREA_SIZE,                                  // Log Area Minimum Length
  (EFI_PHYSICAL_ADDRESS)-1                                // Log Area Start Address
};

STATIC TCM_DXE_PRIVATE_DATA gTcmDxeData = {
  TCM_DXE_CTX_SIGNATURE,
  {                                           // EFI_TCM_PROTOCOL
    TcmDxeStatusCheck,
    TcmDxeHashAll,
    TcmDxeLogEvent,
    TcmDxePassThroughToTcm,
    TcmDxeHashLogExtendEvent
  },
  {                                           // TCM_EFI_BOOT_SERVICE_CAPABILITY
    sizeof(gTcmDxeData.BsCap),
    { 1, 2, 0, 0 },
    { 1, 2, 0, 0 },
    1,
    TRUE,
    FALSE
  },
  &gTcmAcpiTemplate,                          // EFI_TCM_ACPI_TABLE
  0,                                          // CurEventLogSize
  0,                                          // EventLogLastEntry
  NULL,                                       // TcmHandle
  NULL,                                       // DriverHandle
  NULL,                                       // HiiHandle
  {                                           // EFI_HII_CONFIG_ACCESS_PROTOCOL
    TcmFormExtractConfig,
    TcmFormRouteConfig,
    TcmFormCallback
  },
  {
    TcmNvSaveValue,
    TcmNvDiscardValue,
    TcmNvLoadDefault,
    TcmNvSaveUserDefault,
    TcmNvLoadUserDefault,
    TcmIsNvDataChanged  
  },
  {0,}                                        // TCM_SETUP_CONFIG
};





//--------------------------------------------------------------
/**
  Send tcm command with optial added data.

  @param  Ordinal                   Indicates command code.
  @param  AdditionalParameterSize   Added data size.
  @param  AdditionalParameters      Added data.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
SimpleTcmCommand(
  IN     TCM_COMMAND_CODE  Ordinal,
  IN     UINTN             AdditionalParameterSize,
  IN     VOID              *AdditionalParameters
  )
{
  EFI_STATUS                       Status;
  TCM_RQU_COMMAND_HDR              *TcmRqu;
  TCM_RSP_COMMAND_HDR              TcmRsp;
  UINT32                           Size;
  EFI_TCM_PROTOCOL                 *TcmProtocol;
  TCM_DXE_PRIVATE_DATA             *Private;
  EFI_TCM_HANDLE                   TcmHandle;

  Status = gBS->LocateProtocol(&gEfiTcmProtocolGuid, NULL, &TcmProtocol);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(TcmProtocol);
  TcmHandle = Private->TcmHandle;

  TcmRqu = (TCM_RQU_COMMAND_HDR*)AllocatePool(sizeof(*TcmRqu) + AdditionalParameterSize);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  TcmRqu->tag       = SwapBytes16(TCM_TAG_RQU_COMMAND);
  Size              = (UINT32)(sizeof(*TcmRqu) + AdditionalParameterSize);
  TcmRqu->paramSize = SwapBytes32(Size);
  TcmRqu->ordinal   = SwapBytes32(Ordinal);
  CopyMem(TcmRqu+1, AdditionalParameters, AdditionalParameterSize);

  Status = IssueTcmCommandWithRetry(TcmHandle,
                                    (UINT8*)TcmRqu,
                                    Size,
                                    (UINT8*)&TcmRsp,
                                    (UINT32)sizeof(TcmRsp)
                                    );
  FreePool(TcmRqu);

ProcExit:
  return Status;
}






/**
  Set TCM Physical Presence attribute.

  @param  PhysicalPresence       Indicates physical presence attribute.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
TcmPhysicalPresence (
  IN TCM_PHYSICAL_PRESENCE  PhysicalPresence
  )
{
  EFI_STATUS              Status;
  EFI_TCM_PROTOCOL        *TcmProtocol;
  TCM_RQU_COMMAND_HDR     *TcmRqu;
  TCM_PHYSICAL_PRESENCE   *TcmPp;
  TCM_RSP_COMMAND_HDR     TcmRsp;
  UINT8                   Buffer[sizeof (*TcmRqu) + sizeof (*TcmPp)];
  TCM_DXE_PRIVATE_DATA    *Private;
  EFI_TCM_HANDLE          TcmHandle;

  Status = gBS->LocateProtocol(&gEfiTcmProtocolGuid, NULL, &TcmProtocol);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(TcmProtocol);
  TcmHandle = Private->TcmHandle;

  TcmRqu = (TCM_RQU_COMMAND_HDR*)Buffer;
  TcmPp = (TCM_PHYSICAL_PRESENCE*)(TcmRqu + 1);

  TcmRqu->tag       = SwapBytes16(TCM_TAG_RQU_COMMAND);
  TcmRqu->paramSize = SwapBytes32(sizeof(Buffer));
  TcmRqu->ordinal   = SwapBytes32(TSC_ORD_PhysicalPresence);
  *TcmPp            = SwapBytes16(PhysicalPresence);

  Status = IssueTcmCommandWithRetry(TcmHandle,
                                    (UINT8*)TcmRqu,
                                    sizeof(Buffer),
                                    (UINT8*)&TcmRsp,
                                    (UINT32)sizeof(TcmRsp)
                                    );
ProcExit:  
  return Status;
}






/**
  Set TCM physical presence command enable and presence.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
TcmSetOppPresent (
  VOID
  )
{
  EFI_STATUS  Status;
  BOOLEAN     LifetimeLock;
  BOOLEAN     CmdEnable;
  
  Status = GetTcmState(gTcmDxeData.TcmHandle, 
                       NULL, 
                       NULL, 
                       NULL, 
                       &LifetimeLock, 
                       &CmdEnable
                       );
  if(EFI_ERROR(Status)){
    return Status;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {
      return EFI_ABORTED;
    }

    Status = TcmPhysicalPresence(TCM_PHYSICAL_PRESENCE_CMD_ENABLE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = TcmPhysicalPresence(TCM_PHYSICAL_PRESENCE_PRESENT);
  return Status;
}






/**
  Enable or Disable TPM.

  @param  TcmEnable              Indicates to enable or disable TCM.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
EnableTCMDevice (
  IN BOOLEAN TcmEnable
  )
{
  EFI_STATUS  Status;
  
  Status = TcmSetOppPresent();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (TcmEnable) {
    Status = SimpleTcmCommand(TCM_ORD_PhysicalEnable, 0, NULL);
  } else {
    Status = SimpleTcmCommand(TCM_ORD_PhysicalDisable, 0, NULL);     
  }
  return EFI_SUCCESS;
}



/**
  Active or Deactive TPM.

  @param  TcmActive              Indicates to active or deactive TCM.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
ActivateTCMDevice (
  IN BOOLEAN  TcmActive
  )
{
  EFI_STATUS   Status;
  BOOLEAN      TcmDeactive;

  Status = TcmSetOppPresent();
  if(EFI_ERROR(Status)){
    return Status;
  }
 
  TcmDeactive = !TcmActive;
  Status = SimpleTcmCommand(TCM_ORD_PhysicalSetDeactivated, sizeof(TcmDeactive), &TcmDeactive);

  return EFI_SUCCESS;
}







/**
  Force clear TPM.

  @retval EFI_SUCCESS            Operation completed successfully.
  @Others                        Operation completed unsuccessfully.
**/
EFI_STATUS
TcmForceClear (
  VOID
  )
{
  EFI_STATUS  Status;
  
  Status = TcmSetOppPresent();
  if(EFI_ERROR (Status)){
    return Status;
  }

  Status = SimpleTcmCommand(TCM_ORD_ForceClear, 0, NULL);        

  return EFI_SUCCESS;
}






//--------------------------------------------------------------
/**
  This service provides EFI protocol capability information, state information 
  about the TPM, and Event Log state information.

  @param  This                   Indicates the calling context
  @param  ProtocolCapability     The callee allocates memory for a TCG_BOOT_SERVICE_CAPABILITY 
                                 structure and fills in the fields with the EFI protocol 
                                 capability information and the current TPM state information.
  @param  TcmFeatureFlags        This is a pointer to the feature flags. No feature 
                                 flags are currently defined so this parameter 
                                 MUST be set to 0. However, in the future, 
                                 feature flags may be defined that, for example, 
                                 enable hash algorithm agility.
  @param  EventLogLocation       This is a pointer to the address of the event log in memory.
  @param  EventLogLastEntry      If the Event Log contains more than one entry, 
                                 this is a pointer to the address of the start of 
                                 the last entry in the event log in memory. 

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  ProtocolCapability does not match TCG capability.
**/
EFI_STATUS
EFIAPI
TcmDxeStatusCheck (
  IN      EFI_TCM_PROTOCOL                *This,
  OUT     TCM_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
  OUT     UINT32                          *TcmFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLastEntry
  )
{
  TCM_DXE_PRIVATE_DATA *Private;

  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(This);

  if (ProtocolCapability != NULL) {
    CopyMem(ProtocolCapability, &Private->BsCap, sizeof(TCM_EFI_BOOT_SERVICE_CAPABILITY));
  }

  if (TcmFeatureFlags != NULL) {
    *TcmFeatureFlags = 0;
  }

  if(EventLogLocation != NULL){
    *EventLogLocation = Private->TcmAcpiTable->Lasa;
  }

  if(EventLogLastEntry != NULL){
    if(Private->BsCap.TcmDeactivatedFlag){
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    }else{
      *EventLogLastEntry = Private->EventLogLastEntry;
    }
  }

  return EFI_SUCCESS;
}




/**
  Single function calculates SM3 digest value for all raw data. It
  combines TCM_SM3Start, TCM_SM3Update, TCM_SM3Complete

  @param  Data          Raw data to be digested.
  @param  DataLen       Size of the raw data.
  @param  Digest        Pointer to a buffer that stores the final digest.
  
  @retval EFI_SUCCESS   
**/
EFI_STATUS
EFIAPI
TcmCommHashAll (
  IN  CONST UINT8       *Data,
  IN        UINTN       DataLen,
  OUT       TCM_DIGEST  *Digest
  )
{
  EFI_STATUS             Status;
  TCM_HASH_SM3_PROTOCOL  *ptSm3;
  
  Status = gBS->LocateProtocol(&gTcmHashSm3ProtocolGuid, NULL, &ptSm3);
  if(EFI_ERROR(Status)){
    return Status; 
  }
  
  Status = ptSm3->HashSm3(Data, DataLen, (UINT8*)Digest);
  
  return Status;  
}



/**
  This service abstracts the capability to do a hash operation on a data buffer.
  
  @param  This                   Indicates the calling context
  @param  HashData               Pointer to the data buffer to be hashed
  @param  HashDataLen            Length of the data buffer to be hashed
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  HashedDataLen          Resultant length of the hashed data
  @param  HashedDataResult       Resultant buffer of the hashed data  
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  HashDataLen is NULL.
  @retval EFI_INVALID_PARAMETER  HashDataLenResult is NULL.
  @retval EFI_OUT_OF_RESOURCES   Cannot allocate buffer of size *HashedDataLen.
  @retval EFI_UNSUPPORTED        AlgorithmId not supported.
  @retval EFI_BUFFER_TOO_SMALL   *HashedDataLen < sizeof (TCG_DIGEST).
**/
EFI_STATUS
EFIAPI
TcmDxeHashAll (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  UINT64                    *HashedDataLen,
  IN OUT  UINT8                     **HashedDataResult
  )
{
  if(HashedDataLen == NULL || HashedDataResult == NULL){
    return EFI_INVALID_PARAMETER;
  }

  switch (AlgorithmId) {
    case TCM_ALG_SM3:
      if(*HashedDataLen == 0){
        *HashedDataLen    = sizeof (TCM_DIGEST);
        *HashedDataResult = AllocatePool((UINTN)*HashedDataLen);
        if(*HashedDataResult == NULL){
          return EFI_OUT_OF_RESOURCES;
        }
      }

      if(*HashedDataLen < sizeof(TCM_DIGEST)){
        *HashedDataLen = sizeof(TCM_DIGEST);
        return EFI_BUFFER_TOO_SMALL;
      }
      *HashedDataLen = sizeof(TCM_DIGEST);

      return TcmCommHashAll(
               HashData,
               (UINTN)HashDataLen,
               (TCM_DIGEST*)*HashedDataResult
               );
    default:
      return EFI_UNSUPPORTED;
  }
}






/**
  Add a new entry to the Event Log.

  @param[in, out] EventLogPtr   Pointer to the Event Log data.  
  @param[in, out] LogSize       Size of the Event Log.  
  @param[in]      MaxSize       Maximum size of the Event Log.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TcmCommLogEvent (
  IN      EFI_TCM_PROTOCOL      *This,
  IN      TCM_PCR_EVENT         *NewEvent
  )
{
  TCM_DXE_PRIVATE_DATA   *Private;  
  UINT32                 NewLogSize;
  UINT8                  *EventLogStart;
  UINTN                  CurLogSize;
  UINTN                  MaxSize;

  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(This);

  if(Private->BsCap.TcmDeactivatedFlag){
    return EFI_DEVICE_ERROR;
  }
  
  EventLogStart = (UINT8*)(UINTN)Private->TcmAcpiTable->Lasa;
  CurLogSize    = Private->CurEventLogSize;
  MaxSize       = Private->TcmAcpiTable->Laml;
  NewLogSize    = sizeof(*NewEvent) + NewEvent->EventSize - 1;
  
  if(NewLogSize + CurLogSize > MaxSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem(EventLogStart+CurLogSize, NewEvent, NewLogSize);
  Private->EventLogLastEntry  = (UINTN)EventLogStart + CurLogSize;
  Private->CurEventLogSize   += NewLogSize;
  
  return EFI_SUCCESS;
}




/**
  This service abstracts the capability to add an entry to the Event Log.

  @param  This                   Indicates the calling context
  @param  TCMLogData             Pointer to the start of the data buffer containing 
                                 the TCM_PCR_EVENT data structure. All fields in 
                                 this structure are properly filled by the caller.
  @param  EventNumber            The event number of the event just logged
  @param  Flags                  Indicate additional flags. Only one flag has been 
                                 defined at this time, which is 0x01 and means the 
                                 extend operation should not be performed. All 
                                 other bits are reserved. 
 
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory in the event log to complete this action.
**/
EFI_STATUS
EFIAPI
TcmDxeLogEvent (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  )
{
  return TcmCommLogEvent(This, TCMLogData);
}








/**
  This service is a proxy for commands to the TCM.

  @param  This                        Indicates the calling context
  @param  TpmInputParameterBlockSize  Size of the TPM input parameter block
  @param  TpmInputParameterBlock      Pointer to the TPM input parameter block
  @param  TpmOutputParameterBlockSize Size of the TPM output parameter block
  @param  TpmOutputParameterBlock     Pointer to the TPM output parameter block

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Invalid ordinal.
  @retval EFI_UNSUPPORTED        Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval EFI_TIMEOUT            The TCM timed-out.
**/
EFI_STATUS
EFIAPI
TcmDxePassThroughToTcm (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      UINT32                    TcmInputParameterBlockSize,
  IN      UINT8                     *TcmInputParameterBlock,
  IN      UINT32                    TcmOutputParameterBlockSize,
  IN      UINT8                     *TcmOutputParameterBlock
  )
{
  TCM_DXE_PRIVATE_DATA *Private;

  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(This);

  return TcmPcExecute (
           Private->TcmHandle,
           "%r%/%r",
           TcmInputParameterBlock,
           (UINTN)TcmInputParameterBlockSize,
           TcmOutputParameterBlock,
           (UINTN)TcmOutputParameterBlockSize
           );
}




/**
  This service abstracts the capability to do a hash operation on a data buffer,
  extend a specific TCM PCR with the hash result, and add an entry to the Event Log

  @param  This                   Indicates the calling context
  @param  HashData               Physical address of the start of the data buffer 
                                 to be hashed, extended, and logged.
  @param  HashDataLen            The length, in bytes, of the buffer referenced by HashData
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  TCGLogData             The physical address of the start of the data 
                                 buffer containing the TCM_PCR_EVENT data structure.
  @param  EventNumber            The event number of the event just logged.
  @param  EventLogLastEntry      Physical address of the first byte of the entry 
                                 just placed in the Event Log. If the Event Log was 
                                 empty when this function was called then this physical 
                                 address will be the same as the physical address of 
                                 the start of the Event Log.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_UNSUPPORTED        AlgorithmId != TPM_ALG_SHA.
  @retval EFI_UNSUPPORTED        Current TPL >= EFI_TPL_CALLBACK.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
TcmDxeHashLogExtendEvent (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  )
{
  TCM_DXE_PRIVATE_DATA  *Private;
  EFI_STATUS            Status;
  
  Private = TCM_DXE_DATA_FROM_THIS_PROTOCOL(This);
  
  if(Private->BsCap.TcmDeactivatedFlag){
    return EFI_DEVICE_ERROR;
  }
    
  if(AlgorithmId != TCM_ALG_SM3){
    return EFI_UNSUPPORTED;
  }

  if(HashDataLen > 0){
    Status = TcmCommHashAll(
               (UINT8*)(UINTN)HashData,
               (UINTN)HashDataLen,
               &TCMLogData->Digest
               );
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }
  }

  Status = TcmCommExtend(
             Private->TcmHandle,
             &TCMLogData->Digest,
             TCMLogData->PCRIndex,
             NULL
             );
  if(!EFI_ERROR(Status)){
    Status = TcmCommLogEvent(This, TCMLogData);
    if(EventLogLastEntry != NULL){
      *EventLogLastEntry = Private->EventLogLastEntry;
    }
  }

ProcExit:
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
TcmFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  TCM_SETUP_CONFIG                 *IfrData;
  TCM_DXE_PRIVATE_DATA             *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  IfrData          = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Status           = EFI_SUCCESS;

  if (Progress == NULL || Results == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  *Progress = Request;
  if((Request != NULL) && !HiiIsConfigHdrMatch(Request, &gTcmSetupConfigGuid, gTcmSetupConfigName)){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  ConfigRequestHdr = NULL;
  Size             = 0;

  Private = TCM_DXE_DATA_FROM_THIS_HII_CONFIG(This);
  IfrData = AllocateZeroPool(sizeof(TCM_SETUP_CONFIG));
  if(IfrData == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  CopyMem(IfrData, &Private->ConfigData, sizeof(TCM_SETUP_CONFIG));

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof(TCM_SETUP_CONFIG);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr(&gTcmSetupConfigGuid, 
                                             gTcmSetupConfigName, 
                                             Private->DriverHandle);
    Size = (StrLen(ConfigRequestHdr) + 32 + 1) * sizeof(CHAR16);
    ConfigRequest = AllocateZeroPool(Size);
    if(ConfigRequest == NULL){
      Status = EFI_OUT_OF_RESOURCES;
      goto ProcExit;
    }
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8*)IfrData,
                                BufferSize,
                                Results,
                                Progress
                                );

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

ProcExit:
  if(IfrData != NULL){
    FreePool(IfrData);
  }
  if(AllocatedRequest){
    FreePool(ConfigRequest);
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
TcmFormRouteConfig (
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
  if (!HiiIsConfigHdrMatch (Configuration, &gTcmSetupConfigGuid, gTcmSetupConfigName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}



VOID DumpTcmStates()
{
#ifndef MDEPKG_NDEBUG

  EFI_STATUS          Status;
  BOOLEAN             TcmEnable;
  BOOLEAN             TcmActivated;
  TCM_STCLEAR_FLAGS   VFlags;

  
  Status = GetTcmState(
             gTcmDxeData.TcmHandle, 
             &TcmEnable, 
             &TcmActivated, 
             NULL, 
             NULL, 
             NULL
             );
  Status = TcmGetStclearFlag(gTcmDxeData.TcmHandle, &VFlags);
  DEBUG((EFI_D_INFO, "En:%d, Active:%d(%d)\n", TcmEnable, TcmActivated, !VFlags.deactivated));

#endif
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
TcmFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  TCM_DXE_PRIVATE_DATA  *Private;
  TCM_SETUP_CONFIG      *ConfigData;

  
  if (This == NULL || Value == NULL || ActionRequest == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Private    = TCM_DXE_DATA_FROM_THIS_HII_CONFIG(This);
  ConfigData = &Private->ConfigData;

  DEBUG((EFI_D_INFO, "Q(%d):%d\n", QuestionId, Value->u8));

  switch(QuestionId){
    
    case KEY_TCM_ENABLE:    
      ConfigData->TcmUserEn = Value->u8;
      break;

    case KEY_TCM_FORCE_CLEAR:    
      ConfigData->TcmUserClear = Value->u8;
      break;
      
    default:
      break;          
  }

ProcExit:
  return Status;
}


// Dpx: 
//   Enable - N/A
//   Active - Enable
//   Clear  - Enable + Active

EFI_STATUS 
TcmNvSaveValue(  
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  TCM_DXE_PRIVATE_DATA  *Private; 
  TCM_SETUP_CONFIG      *ConfigData;
  

  Private = TCM_DXE_DATA_FROM_THIS_SETUP_SAVE(This);
  ConfigData = &Private->ConfigData;

  DEBUG((EFI_D_INFO, "[Want] Enable:%d, Clear:%d\n", \
    ConfigData->TcmUserEn, ConfigData->TcmUserClear));
  DumpTcmStates();

  if(ConfigData->TcmUserEn != ConfigData->TcmEnable){
    if(ConfigData->TcmUserEn){                         // want enable
      Status = EnableTCMDevice(TRUE);
      DEBUG((EFI_D_INFO, "Enable:%r\n", Status));
      Status = ActivateTCMDevice(TRUE);
      DEBUG((EFI_D_INFO, "Active:%r\n", Status)); 
      ConfigData->TcmEnable = TRUE;
      ConfigData->TcmActive = TRUE;
    } else {                                           // want disable
      Status = ActivateTCMDevice(FALSE);
      DEBUG((EFI_D_INFO, "DeActive:%r\n", Status));     
      Status = EnableTCMDevice(FALSE);
      DEBUG((EFI_D_INFO, "Disable:%r\n", Status));
      ConfigData->TcmEnable = FALSE;
      ConfigData->TcmActive = FALSE;      
    }
    DumpTcmStates();
    goto ProcExit;
  }  

  if(ConfigData->TcmEnable && ConfigData->TcmUserClear){
    if(!ConfigData->TcmActive){
      Status = ActivateTCMDevice(TRUE);
      DEBUG((EFI_D_INFO, "Active:%r\n", Status));
    }  
    Status = TcmForceClear();
    DEBUG((EFI_D_INFO, "Clear:%r\n", Status));
    ConfigData->TcmEnable = FALSE;
    ConfigData->TcmActive = FALSE;     
    DumpTcmStates();    
  }  

ProcExit:  
  return Status;
}



EFI_STATUS 
TcmNvDiscardValue(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  TCM_DXE_PRIVATE_DATA  *Private; 

  Private = TCM_DXE_DATA_FROM_THIS_SETUP_SAVE(This);
  
  return Status;
}



EFI_STATUS 
TcmNvLoadDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  TCM_DXE_PRIVATE_DATA  *Private; 
  TCM_SETUP_CONFIG      *ConfigData;


  Private = TCM_DXE_DATA_FROM_THIS_SETUP_SAVE(This);
  ConfigData = &Private->ConfigData;

  ConfigData->TcmUserClear = 0;
  ConfigData->TcmUserEn    = ConfigData->TcmEnable;

  HiiSetBrowserData(
    &gTcmSetupConfigGuid, 
    gTcmSetupConfigName, 
    sizeof(TCM_SETUP_CONFIG), 
    (UINT8 *)ConfigData, 
    NULL
    );
  
  return Status;
}


EFI_STATUS 
TcmNvSaveUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_UNSUPPORTED;  
}


EFI_STATUS 
TcmNvLoadUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_UNSUPPORTED;  
}

EFI_STATUS 
TcmIsNvDataChanged(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This,
  BOOLEAN                       *IsDataChanged
  )
{
  *IsDataChanged = FALSE;
  return EFI_SUCCESS;
}





VOID TcmLockPhysicalPresence()
{
  EFI_STATUS    Status;
  BOOLEAN       LifetimeLock;
  BOOLEAN       CmdEnable;
  BOOLEAN       PhysicalPresenceLock;
  STATIC UINTN  BootAttempts = 0;

  if(BootAttempts++){
    return; 
  }

// Make sure TPM_PHYSICAL_PRESENCE_CMD_ENABLE is TRUE for software method physical presence
  Status = GetTcmState(gTcmDxeData.TcmHandle, 
                       NULL, 
                       NULL, 
                       &PhysicalPresenceLock, 
                       &LifetimeLock, 
                       &CmdEnable
                       );
  if(EFI_ERROR(Status)){
    TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
    return;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {     // physicalPresenceCMDEnable is locked, can't change.
      return;
    }
    Status = TcmPhysicalPresence(TCM_PHYSICAL_PRESENCE_CMD_ENABLE);
    if(EFI_ERROR(Status)){
      TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
      return;
    }
  }

// Check and execute pending TPM request command.
  
// Lock physical presence. It should be as early as possible.
  Status = TcmPhysicalPresence(TCM_PHYSICAL_PRESENCE_NOTPRESENT | TCM_PHYSICAL_PRESENCE_LOCK);
  if(EFI_ERROR(Status)){
    TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
  }    
}




/**
  Measure and log EFI handoff tables, and extend the measurement result into PCR[1].

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureHandoffTables (
  VOID
  )
{
  EFI_STATUS                        Status;
  SMBIOS_TABLE_ENTRY_POINT          *SmbiosTable;
  TCM_PCR_EVENT                     *TcmEvent;
  EFI_HANDOFF_TABLE_POINTERS        *HandoffTables;
  UINTN                             EventSize;
  UINT32                            EventNumber;

  TcmEvent = NULL;
  
  Status = EfiGetSystemConfigurationTable(&gEfiSmbiosTableGuid, &SmbiosTable);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  if(SmbiosTable == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  EventSize = sizeof(TCM_PCR_EVENT_HDR) + sizeof(EFI_HANDOFF_TABLE_POINTERS);
  TcmEvent  = AllocatePool(EventSize);
  if(TcmEvent==NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  TcmEvent->PCRIndex  = 1;
  TcmEvent->EventType = EV_EFI_HANDOFF_TABLES;
  TcmEvent->EventSize = sizeof(EFI_HANDOFF_TABLE_POINTERS);
  
  HandoffTables = (EFI_HANDOFF_TABLE_POINTERS*)&TcmEvent->Event[0];
  HandoffTables->NumberOfTables = 1;
  CopyMem(&(HandoffTables->TableEntry[0].VendorGuid), &gEfiSmbiosTableGuid, sizeof(EFI_GUID));
  HandoffTables->TableEntry[0].VendorTable = SmbiosTable;

  Status = TcmDxeHashLogExtendEvent (
             &gTcmDxeData.TcmProtocol,
             SmbiosTable->TableAddress,
             SmbiosTable->TableLength,
             TCM_ALG_SM3,
             TcmEvent,
             &EventNumber, 
             NULL
             );

ProcExit:
  if(TcmEvent!=NULL){
    FreePool(TcmEvent);
  }
  return Status;
}



/**
  Read an EFI Variable.

  This function allocates a buffer to return the contents of the variable. The caller is
  responsible for freeing the buffer.

  @param[in]  VarName     A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid  A unique identifier for the vendor.
  @param[out] VarSize     The size of the variable data.  

  @return A pointer to the buffer to return the contents of the variable.Otherwise NULL.

**/
VOID *
EFIAPI
ReadVariable (
  IN      CHAR16     *VarName,
  IN      EFI_GUID   *VendorGuid,
  OUT     UINTN      *VarSize
  )
{
  EFI_STATUS  Status;
  VOID        *VarData;

  *VarSize = 0;
  Status = gRT->GetVariable (
                  VarName,
                  VendorGuid,
                  NULL,
                  VarSize,
                  NULL
                  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  VarData = AllocatePool(*VarSize);
  if(VarData != NULL){
    Status = gRT->GetVariable (
                    VarName,
                    VendorGuid,
                    NULL,
                    VarSize,
                    VarData
                    );
    if(EFI_ERROR(Status)){
      FreePool(VarData);
      VarData = NULL;
      *VarSize = 0;
    }
  }
  return VarData;
}


/**
  Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in] PCRIndex    PCR Index.  
  @param[in] EventType   Event type.  
  @param[in] VarName     A Null-terminated string that is the name of the vendor's variable.
  @param[in] VendorGuid  A unique identifier for the vendor.
  @param[in] VarData     The content of the variable data.  
  @param[in] VarSize     The size of the variable data.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureVariable (
  IN      CHAR16         *VarName,
  IN      EFI_GUID       *VendorGuid,
  IN      TCM_PCRINDEX   PCRIndex,
  IN      TCM_EVENTTYPE  EventType
  )
{
  EFI_STATUS         Status;
  TCM_PCR_EVENT      *TcmEvent;
  UINTN              VarNameLength;
  EFI_VARIABLE_DATA  *VarLog;
  UINTN              VarSize;
  VOID               *VarData;
  UINT32             EventSize;
  UINT32             EventNumber;

  VarData  = NULL;
  TcmEvent = NULL;

  VarData = ReadVariable(VarName, VendorGuid, &VarSize);
  if(VarData == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }
  
  VarNameLength = StrLen(VarName);
  EventSize     = (UINT32)(sizeof(*VarLog) + VarNameLength*sizeof(*VarName) + VarSize
                        - sizeof(VarLog->UnicodeName) - sizeof(VarLog->VariableData));
  TcmEvent = AllocatePool(sizeof(TCM_PCR_EVENT_HDR) + EventSize);
  if (TcmEvent == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  TcmEvent->PCRIndex  = PCRIndex;
  TcmEvent->EventType = EventType;
  TcmEvent->EventSize = EventSize;

  VarLog = (EFI_VARIABLE_DATA*)&TcmEvent->Event[0];
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem(&VarLog->VariableName, VendorGuid, sizeof(EFI_GUID));
  CopyMem(VarLog->UnicodeName, VarName, VarNameLength*sizeof(*VarName));
  CopyMem((CHAR16 *)VarLog->UnicodeName+VarNameLength, VarData, VarSize);

  Status = TcmDxeHashLogExtendEvent (
             &gTcmDxeData.TcmProtocol,
             (UINTN)VarData,
             VarSize,
             TCM_ALG_SM3,
             TcmEvent,
             &EventNumber, 
             NULL
             );
  
ProcExit:
  if(VarData!=NULL){
    FreePool(VarData); 
  }
  if(TcmEvent!=NULL){
    FreePool(TcmEvent); 
  }  
  return Status;
}


/**
  Measure and log all EFI boot variables, and extend the measurement result into a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @param[in]  VarName     A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid  A unique identifier for the vendor.
  @param[out] VarSize     The size of the variable data.  
  @param[out] VarSize     Pointer to the content of the variable.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureAllBootVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *BootOrder;
  UINTN       BootCount;
  UINTN       Index;
  UINT16      BootXXXX[16];

  Status = MeasureVariable(gBootVarName, &gEfiGlobalVariableGuid, 5, EV_EFI_VARIABLE_BOOT);
  if(Status == EFI_NOT_FOUND){
    return EFI_SUCCESS;
  }

  BootOrder  = ReadVariable(gBootVarName, &gEfiGlobalVariableGuid, &BootCount);   // goes here means it must exist.
  BootCount /= sizeof(*BootOrder);
  for(Index = 0; Index < BootCount; Index++){
    UnicodeSPrint(BootXXXX, sizeof(BootXXXX), L"Boot%04x", BootOrder[Index]);
    Status = MeasureVariable(BootXXXX, &gEfiGlobalVariableGuid, 5, EV_EFI_VARIABLE_BOOT);
  }
  FreePool(BootOrder);
  return Status;
}






/**
  Measure and log an action string, and extend the measurement result into PCR[5].

  @param[in] String           A specific string that indicates an Action event.  
  
  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
TcmMeasureAction (
  IN      CHAR8  *String
  )
{
  TCM_PCR_EVENT   *TcmEvent;
  UINT32          EventSize;
  UINT32          EventNumber;
  EFI_STATUS      Status;
  
  TcmEvent = NULL;
  
  EventSize = (UINT32)AsciiStrLen(String);
  TcmEvent  = AllocatePool(sizeof(TCM_PCR_EVENT_HDR) + EventSize);
  if(TcmEvent==NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  TcmEvent->PCRIndex  = 5;
  TcmEvent->EventType = EV_EFI_ACTION;
  TcmEvent->EventSize = EventSize;
  CopyMem(TcmEvent->Event, String, EventSize);
  
  Status = TcmDxeHashLogExtendEvent(
             &gTcmDxeData.TcmProtocol,
             (UINTN)String,
             EventSize,
             TCM_ALG_SM3,
             TcmEvent,
             &EventNumber, 
             NULL
             );

ProcExit:
  if(TcmEvent!=NULL){
    FreePool(TcmEvent); 
  }
  return Status;
}



/**
  Measure and log Separator event, and extend the measurement result into a specific PCR.

  @param[in] PCRIndex         PCR index.  

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureSeparatorEvent (
  IN      TCM_PCRINDEX  PCRIndex
  )
{
  TCM_PCR_EVENT   *TcmEvent;
  UINT32          EventSize;
  UINT32          EventNumber;
  UINT32          EventData;
  EFI_STATUS      Status;
  
  TcmEvent  = NULL;
  EventData = 0;
  
  EventSize = sizeof(EventData);
  TcmEvent  = AllocatePool(sizeof(TCM_PCR_EVENT_HDR) + EventSize);
  if(TcmEvent==NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  TcmEvent->PCRIndex  = PCRIndex;
  TcmEvent->EventType = EV_SEPARATOR;
  TcmEvent->EventSize = EventSize;
  CopyMem(TcmEvent->Event, &EventData, EventSize);
  
  Status = TcmDxeHashLogExtendEvent(
             &gTcmDxeData.TcmProtocol,
             (UINTN)&EventData,
             EventSize,
             TCM_ALG_SM3,
             TcmEvent,
             &EventNumber, 
             NULL
             );

ProcExit:
  if(TcmEvent!=NULL){
    FreePool(TcmEvent); 
  }
  return Status;
}




VOID TcmMeasureBeforeBoot()
{
  EFI_STATUS    Status;
  TCM_PCRINDEX  PcrIndex;
  STATIC UINTN  BootAttempts = 0;

  if(BootAttempts == 0){
// Measure handoff tables
// Measure BootOrder & Boot#### variables
// first boot attempt
// Draw a line between pre-boot env and entering post-boot env
// Measure GPT, would be Done when BDS phase.
// Measure PE/COFF OS loader, would be done by DxeCore
// Read & Measure variable BootOrder Already measured

    Status = MeasureHandoffTables();
    Status = MeasureAllBootVariables();
    Status = TcmMeasureAction(EFI_CALLING_EFI_APPLICATION);
    
    for(PcrIndex = 0; PcrIndex < 8; PcrIndex++){
      Status = MeasureSeparatorEvent(PcrIndex);
    }

  }else{      // Not first attempt, meaning a return from last attempt
    Status = TcmMeasureAction(EFI_RETURNING_FROM_EFI_APPLICATOIN);
    if(EFI_ERROR(Status)){
      TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
    }    
  }

  BootAttempts++;
}





/**
  Check and execute the physical presence command requested from OS and
  Lock physical presence.

  @param  Event        Event whose notification function is being invoked
  @param  Context      Pointer to the notification function's context
**/
VOID
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gBS->CloseEvent(Event);         // run once.

  TcmMeasureBeforeBoot();
  TcmLockPhysicalPresence();
}




/**
  Exit Boot Services Event notification handler.

  Measure invocation and success of ExitBootServices.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnExitBootServices (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS    Status;

// Measure invocation of ExitBootServices,
  Status = TcmMeasureAction(EFI_EXIT_BOOT_SERVICES_INVOCATION);
  if(EFI_ERROR(Status)){
    TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
  }

// Measure success of ExitBootServices
  Status = TcmMeasureAction(EFI_EXIT_BOOT_SERVICES_SUCCEEDED);
  if(EFI_ERROR(Status)){
    TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
  }  
}







/**
  ACPI Table Protocol notification handler.

  Install TCM ACPI Table when ACPI Table Protocol is available.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
TcmInstallAcpiTable (
  IN EFI_EVENT    Event,
  IN VOID*        Context
  )
{
  UINTN                    TableKey;
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *ptAcpiTbl;
  UINT8                    Checksum;

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, &ptAcpiTbl);
  if(EFI_ERROR(Status)){       // not ready.
    return;
  }

  if(Event!=NULL){
    gBS->CloseEvent(Event);
  }

  gTcmAcpiTemplate.Header.Checksum = 0;
  Checksum = CalculateCheckSum8((UINT8 *)&gTcmAcpiTemplate, sizeof(gTcmAcpiTemplate));
  gTcmAcpiTemplate.Header.Checksum = Checksum;

  Status = ptAcpiTbl->InstallAcpiTable(
                        ptAcpiTbl,
                        &gTcmAcpiTemplate,
                        sizeof(gTcmAcpiTemplate),
                        &TableKey
                        );
  if(EFI_ERROR(Status)){
    TCM_MEM_LOG(("L%d:%r", __LINE__, Status));
  }
}






/**
  Initialize the Event Log.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
EFI_STATUS
EFIAPI
TcmSetupEventLog (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Lasa;


  Lasa = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(EFI_TCM_LOG_AREA_SIZE),
                  &Lasa
                  );
  if(EFI_ERROR(Status)){
    return Status;
  }
  gTcmAcpiTemplate.Lasa = Lasa;
  
  //
  // To initialize them as 0xFF is recommended 
  // because the OS can know the last entry for that.
  //
  SetMem((VOID *)(UINTN)Lasa, EFI_TCM_LOG_AREA_SIZE, 0xFF);
  gTcmAcpiTemplate.Laml = EFI_TCM_LOG_AREA_SIZE;

  return Status;
}


/**
  Log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
STATIC
EFI_STATUS
EFIAPI
TcmLogPeiHobEvent(
  VOID
  )
{
  EFI_STATUS            Status;  
  EFI_PEI_HOB_POINTERS  GuidHob;
  TCM_PCR_EVENT         *TcmEvent;  
  
  Status      = EFI_SUCCESS;
  GuidHob.Raw = GetHobList();
  while(1){
    GuidHob.Raw = GetNextGuidHob(&gTcmEventEntryHobGuid, GuidHob.Raw);
    if(GuidHob.Raw == NULL){
      break;
    }
    TcmEvent    = GET_GUID_HOB_DATA(GuidHob.Guid);
    GuidHob.Raw = GET_NEXT_HOB(GuidHob);
    Status = TcmCommLogEvent(&gTcmDxeData.TcmProtocol, TcmEvent);
    if(EFI_ERROR(Status)){
      break;
    }
  }
  
  return Status;  
}





/**
  The driver's entry point.

  It publishes EFI TCM Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
TcmDxeEntry (
  IN    EFI_HANDLE       ImageHandle,
  IN    EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;
  BOOLEAN     TcmEnable;
  BOOLEAN     TcmActivated;  


  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  gTcmDxeData.TcmHandle = (EFI_TCM_HANDLE)(UINTN)TCM_BASE_ADDRESS;
  Status = TcmPcRequestUseTcm(gTcmDxeData.TcmHandle);
  if(EFI_ERROR(Status)){
    return Status;
  }

  Status = GetTcmState(gTcmDxeData.TcmHandle, &TcmEnable, &TcmActivated, NULL, NULL, NULL);
  if(EFI_ERROR(Status)){
    return Status;
  }

  gTcmDxeData.BsCap.TcmPresentFlag      = TRUE;
  gTcmDxeData.BsCap.TcmDeactivatedFlag  = !TcmActivated;
  gTcmDxeData.BsCap.HashAlgorithmBitmap = 0x0D;               // 0x0D=SM3
  gTcmDxeData.ConfigData.TcmPresent     = TRUE;
  gTcmDxeData.ConfigData.TcmEnable      = TcmEnable;
  gTcmDxeData.ConfigData.TcmActive      = TcmActivated;

  gTcmDxeData.ConfigData.TcmUserEn      = TcmEnable;
  gTcmDxeData.ConfigData.TcmUserClear   = 0;

  gTcmDxeData.DriverHandle = NULL;

  DEBUG((EFI_D_INFO, "TCM En:%d, Active:%d\n", TcmEnable, TcmActivated));

  Status = gBS->InstallProtocolInterface (
                  &gTcmDxeData.DriverHandle,
                  &gEfiTcmProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gTcmDxeData.TcmProtocol
                  );
  ASSERT(!EFI_ERROR(Status));


// setup.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gTcmDxeData.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mTcmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gTcmDxeData.ConfigAccess,
                  &gSetupSaveNotifyProtocolGuid,
                  &gTcmDxeData.SetupSaveNotfiy,
                  NULL
                  );
  ASSERT(!EFI_ERROR(Status));

  gTcmDxeData.HiiHandle = HiiAddPackages (
                            &gTcmSetupConfigGuid,
                            gTcmDxeData.DriverHandle,
                            TcmDxeStrings,
                            TcmSetupBin,
                            NULL
                            );
  ASSERT(gTcmDxeData.HiiHandle != NULL);                            


// acpi
  Status = TcmSetupEventLog();
  gTcmDxeData.EventLogLastEntry = gTcmDxeData.TcmAcpiTable->Lasa;
  EfiCreateProtocolNotifyEvent(
    &gEfiAcpiTableProtocolGuid, 
    TPL_CALLBACK, 
    TcmInstallAcpiTable, 
    NULL, 
    &Registration
    );

  if(!gTcmDxeData.BsCap.TcmDeactivatedFlag){
    
    Status = TcmLogPeiHobEvent();
        
// ReadyToBoot        
    Status = EfiCreateEventReadyToBootEx (
               TPL_CALLBACK,
               OnReadyToBoot,
               NULL,
               &Event
               );
    ASSERT(!EFI_ERROR(Status));
  
// ExitBootService
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    OnExitBootServices,
                    NULL,
                    &gEfiEventExitBootServicesGuid,
                    &Event
                    );
    ASSERT(!EFI_ERROR(Status));


    
    }

  return EFI_SUCCESS;
}





