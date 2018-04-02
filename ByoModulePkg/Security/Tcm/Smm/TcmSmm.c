/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmSmm.c

Abstract: 
  Tcm Smm support.

Revision History:

Bug 3592 - Cpu save state may write error under IA32 CPU. 
TIME:       2012-04-28
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. R8 SMM Arch may destroy cpu save state when do write back under
     IA32 CPU. So we should update save state buffer to avoid it.
$END------------------------------------------------------------

Bug 3269 - Add TCM int1A function support. 
TIME: 2011-12-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  Use Smi to handle legacy int 1A(0xBB) interrupt.
$END------------------------------------------------------------

**/


/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/*++

Copyright (c) 1999 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcgSmm.c

Abstract:

--*/


#include "TcmSmm.h"

STATIC TCM_SMM_HASH_SM3_PROTOCOL  *gSmmSm3Hash  = NULL;
STATIC EFI_SMM_VARIABLE_PROTOCOL  *gSmmVariable = NULL;
STATIC EFI_SMM_CPU_PROTOCOL       *gSmmCpu      = NULL;
STATIC EFI_SMM_SYSTEM_TABLE                *gFrameworkSmst  = NULL;
STATIC EFI_SMM_BASE_HELPER_READY_PROTOCOL  *gSmmBaseHelperReady = NULL;
STATIC UINT8                      gIfceFlags    = 0;
STATIC BOOLEAN                    gTcmPresent   = FALSE;
STATIC EFI_TCM_CLIENT_ACPI_TABLE  gTcmAcpiTable;

STATIC TCM_SMM_DATA gTcmSmmData = {
  { 0 },
  &gTcmAcpiTable
};

UINT32  gTcmLegacyStatus[] = {
  TCM_PC_OK,                                           // #define EFI_SUCCESS               0
  TCM_GENERAL_ERROR,                                   // #define EFI_LOAD_ERROR            EFIERR (1)
  TCM_DRIVER_ERROR (TCM_INVALID_INPUT_PARA),           // #define EFI_INVALID_PARAMETER     EFIERR (2)
  TCM_PC_UNSUPPORTED,                                  // #define EFI_UNSUPPORTED           EFIERR (3)
  TCM_DRIVER_ERROR (TCM_INVALID_INPUT_PARA),           // #define EFI_BAD_BUFFER_SIZE       EFIERR (4)
  TCM_DRIVER_ERROR (TCM_OUTPUT_BUFFER_TOO_SHORT),      // #define EFI_BUFFER_TOO_SMALL      EFIERR (5)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_NOT_READY             EFIERR (6)
  TCM_DRIVER_ERROR (TCM_INVALID_RESPONSE),             // #define EFI_DEVICE_ERROR          EFIERR (7)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_WRITE_PROTECTED       EFIERR (8)
  TCM_PC_LOGOVERFLOW,                                  // #define EFI_OUT_OF_RESOURCES      EFIERR (9)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_VOLUME_CORRUPTED      EFIERR (10)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_VOLUME_FULL           EFIERR (11)
  TCM_DRIVER_ERROR (TCM_DRIVER_TCM_NOT_PRESENT),       // #define EFI_NO_MEDIA              EFIERR (12)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_MEDIA_CHANGED         EFIERR (13)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_NOT_FOUND             EFIERR (14)
  TCM_DRIVER_ERROR (TCM_UNABLE_TO_OPEN),               // #define EFI_ACCESS_DENIED         EFIERR (15)
  TCM_DRIVER_ERROR (TCM_NO_RESPONSE),                  // #define EFI_NO_RESPONSE           EFIERR (16)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_NO_MAPPING            EFIERR (17)
  TCM_DRIVER_ERROR (TCM_CHAR_READ_TIMEOUT),            // #define EFI_TIMEOUT               EFIERR (18)
  TCM_DRIVER_ERROR (TCM_INTERFACE_SHUTDOWN),           // #define EFI_NOT_STARTED           EFIERR (19)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_ALREADY_STARTED       EFIERR (20)
  TCM_DRIVER_ERROR (TCM_TRANSFER_ABORT),               // #define EFI_ABORTED               EFIERR (21)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_ICMP_ERROR            EFIERR (22)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_TFTP_ERROR            EFIERR (23)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_PROTOCOL_ERROR        EFIERR (24)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_INCOMPATIBLE_VERSION  EFIERR (25)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_SECURITY_VIOLATION    EFIERR (26)
  TCM_DRIVER_ERROR (TCM_GENERAL_ERROR),                // #define EFI_CRC_ERROR             EFIERR (27)
};

STATIC BOOLEAN gOpbNeedSet[] = {
  FALSE,  // TCG_StatusCheck
  TRUE,   // TCG_HashLogExtendEvent
  TRUE,   // TCG_PassThroughToTPM
  FALSE,  // TCG_ShutdownPreBootInterface
  TRUE,   // TCG_HashLogEvent
  FALSE,  // TCG_HashAll
  FALSE,  // TCG_TSS
  FALSE,  // TCG_CompactHashLogExtendEvent
  FALSE,  // TCG_MemoryOverwriteRequest
};





VOID
UpdateLastEventAddress (
  VOID
  )
{
  TCM_PCR_EVENT_HDR     *EventHdr;
  VOID                  *LastEvent;

  if (gTcmSmmData.TcmAcpiTable->Lasa == 0){
    return;
  }

  LastEvent = NULL;
  for (
    EventHdr = (TCM_PCR_EVENT_HDR *)(UINTN)gTcmSmmData.TcmAcpiTable->Lasa;
    EventHdr->PCRIndex != (TCM_PCRINDEX)-1;
    EventHdr = (TCM_PCR_EVENT_HDR *)((UINT8 *)EventHdr + sizeof(TCM_PCR_EVENT_HDR) + EventHdr->EventSize)
    ) {
    LastEvent = EventHdr;
  }

  ASSERT(LastEvent >= (VOID*)gTcmSmmData.LastEvent);
  gTcmSmmData.LastEvent     = LastEvent;
  gTcmSmmData.EventLogSize  = (UINTN) EventHdr - (UINTN)gTcmSmmData.TcmAcpiTable->Lasa;
}


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
TcmSmmStatusCheck (
  IN      EFI_TCM_PROTOCOL                *This,
  OUT     TCM_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
  OUT     UINT32                          *TcmFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLastEntry
  )
{
  TCM_SMM_DATA *Private;

  Private = &gTcmSmmData;

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
    if(Private->BsCap.TcmDeactivatedFlag) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)Private->LastEvent;
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
  if(gSmmSm3Hash==NULL){
    return EFI_UNSUPPORTED;  
  }
  return gSmmSm3Hash->HashSm3(Data, DataLen, (UINT8*)Digest);  
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
TcmSmmHashAll (
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
/*      
      if(*HashedDataLen == 0){
        *HashedDataLen    = sizeof (TCM_DIGEST);
        *HashedDataResult = AllocatePool((UINTN)*HashedDataLen);
        if(*HashedDataResult == NULL){
          return EFI_OUT_OF_RESOURCES;
        }
      }
*/
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
  TCM_SMM_DATA           *Private;  
  UINT32                 NewLogSize;
  UINT8                  *EventLogStart;
  UINTN                  CurLogSize;
  UINTN                  MaxSize;

  Private = &gTcmSmmData;

  if(Private->BsCap.TcmDeactivatedFlag){
    return EFI_DEVICE_ERROR;
  }
  
  EventLogStart = (UINT8*)(UINTN)Private->TcmAcpiTable->Lasa;
  CurLogSize    = Private->EventLogSize;
  MaxSize       = Private->TcmAcpiTable->Laml;
  NewLogSize    = sizeof(*NewEvent) + NewEvent->EventSize - 1;
  
  if(NewLogSize + CurLogSize > MaxSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem(EventLogStart+CurLogSize, NewEvent, NewLogSize);
  Private->LastEvent     = (UINT8*)EventLogStart + CurLogSize;
  Private->EventLogSize += NewLogSize;
  
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
TcmSmmLogEvent (
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
TcmSmmPassThroughToTcm (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      UINT32                    TcmInputParameterBlockSize,
  IN      UINT8                     *TcmInputParameterBlock,
  IN      UINT32                    TcmOutputParameterBlockSize,
  IN      UINT8                     *TcmOutputParameterBlock
  )
{
  TCM_SMM_DATA   *Private;  
  
  Private = &gTcmSmmData;

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
TcmSmmHashLogExtendEvent (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  )
{
  TCM_SMM_DATA          *Private; 
  EFI_STATUS            Status;
  
  Private = &gTcmSmmData;
  
  if(Private->BsCap.TcmDeactivatedFlag){
    return EFI_DEVICE_ERROR;
  }
    
  if(AlgorithmId != TCM_ALG_SM3){
    return EFI_UNSUPPORTED;
  }

  if(HashDataLen > 0){
    Status = TcmCommHashAll(
               (CONST UINT8*)(UINTN)HashData,
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
  if (!EFI_ERROR (Status)) {
    Status = TcmCommLogEvent(This, TCMLogData);
    if(EventLogLastEntry != NULL){
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)Private->LastEvent;
    }
  }

ProcExit:
  return Status;
}




STATIC
UINT32
ReadRegister32 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex
  )
{
  EFI_STATUS  Status;
  UINT32      Data32;

  Status = gSmmCpu->ReadSaveState (
                      gSmmCpu,
                      sizeof(Data32),
                      Register,
                      CpuIndex,
                      &Data32
                      );
  ASSERT_EFI_ERROR(Status);
  return Data32;
}

STATIC
UINT16
ReadRegister16 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex
  )
{
  EFI_STATUS   Status;
  UINT16       Data16;

  Status = gSmmCpu->ReadSaveState (
                      gSmmCpu,
                      sizeof(Data16),
                      Register,
                      CpuIndex,
                      &Data16
                      );
  ASSERT_EFI_ERROR(Status);
  return Data16;
}

STATIC
VOID
WriteRegister32 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex,
  UINT32                       Data32
  )
{
  EFI_STATUS  Status;

  DEBUG((EFI_D_ERROR, "%a() R:%d, I:%d, D:0x%X\n", __FUNCTION__, Register, CpuIndex, Data32));

  Status = gSmmCpu->WriteSaveState (
                        gSmmCpu,
                        sizeof(Data32),
                        Register,
                        CpuIndex,
                        &Data32
                        );
  ASSERT_EFI_ERROR (Status);

  if(gFrameworkSmst != NULL){
    EFI_SMI_CPU_SAVE_STATE *SaveState;
    SaveState = &gFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
    switch(Register){
      case EFI_SMM_SAVE_STATE_REGISTER_RAX:
        SaveState->EAX = Data32;
        break;
      case EFI_SMM_SAVE_STATE_REGISTER_RBX:
        SaveState->EBX = Data32;
        break;        
      case EFI_SMM_SAVE_STATE_REGISTER_RCX:
        SaveState->ECX = Data32;
        break;        
      case EFI_SMM_SAVE_STATE_REGISTER_RDX:
        SaveState->EDX = Data32;
        break;        
      case EFI_SMM_SAVE_STATE_REGISTER_RSI:
        SaveState->ESI = Data32;
        break;        
      case EFI_SMM_SAVE_STATE_REGISTER_RDI:
        SaveState->EDI = Data32;
        break;
      default:
        DEBUG((EFI_D_ERROR, "Need update this function!\n"));
        ASSERT(FALSE);
    }
  }
}

STATIC
VOID
WriteRegister16 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex,
  UINT16                       Data16
  )
{
  UINT32  Data32;
  
  Data32 = ReadRegister32(Register, CpuIndex);
  Data32 = (Data32 & 0xFFFF0000) | Data16;
  
  WriteRegister32(Register, CpuIndex, Data32);
}





STATIC
EFI_STATUS
EFIAPI
TcmSmmDispatcher (
  IN EFI_HANDLE                        DispatchHandle,
  IN CONST EFI_SMM_SW_REGISTER_CONTEXT *Context,
  IN OUT EFI_SMM_SW_CONTEXT            *SwContext,
  IN OUT UINTN                         *CommBufferSize
  )
{
  EFI_STATUS                      Status;
  TCM_EFI_BOOT_SERVICE_CAPABILITY Capability;
  EFI_PHYSICAL_ADDRESS            EventLogLocation;
  EFI_PHYSICAL_ADDRESS            EventLogLastEntry;
  UINT32                          FeatureFlags;
  UINT8                           *DigestPointer;
  UINT8                           FunctionId;
  OPB_TCMStruc                    *OpbStructure;
  UINT32                          EventNumber;
  UINT64                          HashedDataLen;
  
  Status     = EFI_SUCCESS;
  FunctionId = (UINT8)(ReadRegister16(EFI_SMM_SAVE_STATE_REGISTER_RAX, SwContext->SwSmiCpuIndex) >> 8);

  if(!gTcmPresent){
    Status = EFI_NO_MEDIA;
    goto lExit;
  }
  
  UpdateLastEventAddress();

  if(FunctionId == TCM_StatusCheck){
    Status = TcmSmmStatusCheck(NULL, &Capability, &FeatureFlags, &EventLogLocation, &EventLogLastEntry);

    WriteRegister32 (
      EFI_SMM_SAVE_STATE_REGISTER_RBX,
      SwContext->SwSmiCpuIndex,
      SIGNATURE_32 ('T', 'C', 'P', 'A')
      );
    WriteRegister16 (
      EFI_SMM_SAVE_STATE_REGISTER_RCX,
      SwContext->SwSmiCpuIndex,
      (((UINT16)Capability.ProtocolSpecVersion.major) << 8) | Capability.ProtocolSpecVersion.minor
      );
    WriteRegister32 (
      EFI_SMM_SAVE_STATE_REGISTER_RDX,
      SwContext->SwSmiCpuIndex,
      FeatureFlags
      );
    WriteRegister32 (
      EFI_SMM_SAVE_STATE_REGISTER_RSI,
      SwContext->SwSmiCpuIndex,
      (UINT32) EventLogLocation
      );
    WriteRegister32 (
      EFI_SMM_SAVE_STATE_REGISTER_RDI,
      SwContext->SwSmiCpuIndex,
      (UINT32) EventLogLastEntry
      );

    if (gIfceFlags & IFCE_OFF) {
      Status = EFI_NOT_STARTED;
    }
    goto lExit;
  }

  //
  // The rest of the functions need to look at the TCM state and Interface Off flag
  // (interface off takes precedence over TCM state)
  //
  if (gIfceFlags & IFCE_OFF) {
    Status = EFI_NOT_STARTED;
    goto lExit;
  }

  switch (FunctionId) {

  case TCM_HashLogExtendEvent:
    {
      IPB_HashLogExtendEventStruc   *Ipb;
      OPB_HashLogExtendEventStruc   *Opb;

      Ipb = (IPB_HashLogExtendEventStruc *)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_HashLogExtendEventStruc *)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
      if(Ipb->ulog.longLog.LogDataLen !=
          ((TCM_PCR_EVENT*)(UINTN)Ipb->ulog.longLog.LogDataPtr)->EventSize +
          sizeof(TCM_PCR_EVENT) - sizeof(UINT8)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (((TCM_PCR_EVENT*)(UINTN)Ipb->ulog.longLog.LogDataPtr)->PCRIndex != Ipb->PCRIndex) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      Status = TcmSmmHashLogExtendEvent (
                 NULL,
                 (EFI_PHYSICAL_ADDRESS) (UINTN) Ipb->HashDataPtr,
                 Ipb->HashDataLen,
                 TCM_ALG_SM3,
                 (TCM_PCR_EVENT*)(UINTN)Ipb->ulog.longLog.LogDataPtr,
                 &EventNumber,
                 &EventLogLastEntry
                 );
      if (!EFI_ERROR (Status)) {
        Opb->EventNumber = EventNumber;
        Opb->OPBLength   = sizeof(OPB_HashLogExtendEventStruc);
        CopyMem(Opb->HashValue, 
                ((TCM_PCR_EVENT*)(UINTN)Ipb->ulog.longLog.LogDataPtr)->Digest.digest, 
                sizeof(Opb->HashValue));
      }

      break;
    }

  case TCM_PassThroughToTCM:
    {
      IPB_TCMStruc  *Ipb;
      OPB_TCMStruc  *Opb;

      Ipb = (IPB_TCMStruc*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_TCMStruc*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      if (Ipb->IPBLength != 8 + (UINT16) SwapBytes32(((TCM_RQU_COMMAND_HDR*)&Ipb->TCMOperandIn)->paramSize)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      Status = TcmSmmPassThroughToTcm (
                  NULL,
                  Ipb->IPBLength - 8,
                  &Ipb->TCMOperandIn,
                  Ipb->OPBLength - 4,
                  &Opb->TCMOperandOut
                  );
      if (!EFI_ERROR (Status)) {
        Opb->OPBLength = 4 + (UINT16)SwapBytes32(((TCM_RSP_COMMAND_HDR*)&Opb->TCMOperandOut)->paramSize);
        //
        // Update output length if what caller gives to me is shorter than we want to receive from TCM
        //
        if(Ipb->OPBLength > Opb->OPBLength){
          Opb->OPBLength = Ipb->OPBLength;
        }
      }
      break;
    }

  case TCM_ShutdownPreBootInterface:
    {
      gIfceFlags |= IFCE_OFF;
      Status = EFI_SUCCESS;
      break;
    }

  case TCM_HashLogEvent:
    {
      IPB_HashLogEventStruc      *Ipb;
      OPB_HashLogEventStruc      *Opb;

      Ipb = (IPB_HashLogEventStruc*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_HashLogEventStruc*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      if (Ipb->LogDataLen !=
              sizeof(TCM_PCR_EVENT)
              + ((TCM_PCR_EVENT*)(UINTN)Ipb->LogDataPtr)->EventSize - sizeof(UINT8)){
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (Ipb->PCRIndex != ((TCM_PCR_EVENT*)(UINTN)Ipb->LogDataPtr)->PCRIndex) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (Ipb->LogEventType != ((TCM_PCR_EVENT*)(UINTN)Ipb->LogDataPtr)->EventType){
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      // If either the hash data length or hash data pointer is null we do not need
      // to hash the data since the digest is already included in the event structure.
      //
      if((Ipb->HashDataLen != 0) && ((UINT8*)(UINTN)Ipb->HashDataPtr != NULL)){
        HashedDataLen = sizeof(TCM_DIGEST);
        DigestPointer = (UINT8 *)((TCM_PCR_EVENT *)(UINTN)Ipb->LogDataPtr)->Digest.digest;
        Status = TcmSmmHashAll (
                   NULL,
                   (UINT8*)(UINTN)Ipb->HashDataPtr,
                   Ipb->HashDataLen,
                   TCM_ALG_SM3,
                   &HashedDataLen,
                   &DigestPointer
                   );
        if(EFI_ERROR (Status)){
          break;
        }
      }
      Status = TcmSmmLogEvent (
                  NULL,
                  (TCM_PCR_EVENT*)(UINTN)Ipb->LogDataPtr,
                  &EventNumber,
                  0x01
                  );
      if (!EFI_ERROR (Status)) {
        Opb->EventNumber = EventNumber;
        Opb->OPBLength   = sizeof (OPB_HashLogEventStruc);
        Opb->Reserved1   = 0;
      }
      break;
    }

  case TCM_HashAll:
    {
      IPB_HashAll_Struc       *Ipb;
      TCM_DIGEST              *Digest;

      Ipb    = (IPB_HashAll_Struc *)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Digest = (TCM_DIGEST*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      HashedDataLen = sizeof(TCM_DIGEST);
      DigestPointer = (UINT8*)Digest->digest;
      Status = TcmSmmHashAll(
                 NULL,
                 (UINT8*)(UINTN)Ipb->HashDataPtr,
                 Ipb->HashDataLen,
                 Ipb->AlgorithmID,
                 &HashedDataLen,
                 (UINT8 **)&DigestPointer
                 );
      break;
    }
    
  case TCM_CompactHashLogExtendEvent:
    {
      UINT8                *HashData;
      TCM_PCR_EVENT        *Event;
      UINT32               EventData;
      UINT64               HashDataLen;

      HashData    = (UINT8 *) (UINTN) ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      EventData   =                   ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
      HashDataLen = (UINT64)          ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RCX, SwContext->SwSmiCpuIndex);

      Event = AllocatePool(sizeof(TCM_PCR_EVENT_HDR) + sizeof(EventData));
      Event->PCRIndex  = ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex);
      Event->EventType = EV_COMPACT_HASH;
      Event->EventSize = sizeof(EventData);
      CopyMem(Event->Event, &EventData, Event->EventSize);

      Status = TcmSmmHashLogExtendEvent (
                 NULL,
                 (EFI_PHYSICAL_ADDRESS)(UINTN)HashData,
                 HashDataLen,
                 TCM_ALG_SM3,
                 Event,
                 &EventNumber,
                 &EventLogLastEntry
                 );
      if(!EFI_ERROR(Status)){
        WriteRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex, EventNumber);
        //
        // Work-around for tcgbios check
        //
        WriteRegister32(EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex, 1);
      }
      break;
    }
    
    
  case TCM_MemoryOverwriteRequest:
    {
      IPB_MORStruc      *Ipb;
      Ipb    = (IPB_MORStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Status = EFI_UNSUPPORTED;
      break;
    }
    
  default:
    {
      Status = EFI_UNSUPPORTED;
      break;
    }
  }

lExit:
  if (EFI_ERROR (Status) && gOpbNeedSet[FunctionId]) {
    OpbStructure = (OPB_TCMStruc*)(UINTN)ReadRegister32(EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
    OpbStructure->OPBLength = 4;
    OpbStructure->Reserved1 = 0;
  }
  WriteRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RAX, SwContext->SwSmiCpuIndex, LEGACY_TCM_STATUS(Status));

  return EFI_SUCCESS;
}




STATIC
VOID
EFIAPI
TcmHandler (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                        Status;
  UINT32                            FeatureFlags;
  EFI_TCM_PROTOCOL                  *ptTcm;
  EFI_PHYSICAL_ADDRESS              EventLogLocation;
  EFI_PHYSICAL_ADDRESS              EventLogLastEntry;
  BOOLEAN                           TcmActivated;
  EFI_TCM_HANDLE                    TcmHandle;
  
  TcmHandle = (EFI_TCM_HANDLE)(UINTN)TCM_BASE_ADDRESS;
  
  Status = gBS->LocateProtocol(&gEfiTcmProtocolGuid, NULL, &ptTcm);
  if(EFI_ERROR(Status)){
    return;
  }

  gTcmPresent = TRUE;
  Status = ptTcm->StatusCheck (
                    ptTcm,
                    &gTcmSmmData.BsCap,
                    &FeatureFlags,
                    &EventLogLocation,
                    &EventLogLastEntry
                    );
  if(EFI_ERROR(Status)){
    return;
  }

  Status = GetTcmState(TcmHandle, NULL, &TcmActivated, NULL, NULL, NULL);
  if(EFI_ERROR(Status) || gTcmSmmData.BsCap.TcmDeactivatedFlag || !TcmActivated){
    gIfceFlags |= IFCE_OFF;
  } else {
    gTcmSmmData.TcmHandle          = TcmHandle;
    gTcmSmmData.TcmAcpiTable->Lasa = (UINT64) EventLogLocation;
    gTcmSmmData.TcmAcpiTable->Laml = (UINT32) 0x10000; // LAML is 64KB
    gTcmSmmData.LastEvent          = (UINT8 *)(UINTN)EventLogLastEntry;
    gTcmSmmData.EventLogSize       = (UINT32) (EventLogLastEntry
                                                - EventLogLocation
                                                + sizeof (TCM_PCR_EVENT_HDR)
                                                + ((TCM_PCR_EVENT_HDR *)(UINTN)EventLogLastEntry)->EventSize
                                                );
  }
}




EFI_STATUS
EFIAPI
InitializeTcmSmm (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL     *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT       SwContext;
  EFI_HANDLE                        SwHandle;
  EFI_HANDLE                        Handle;
  TCM_SMM_INT1A_READY_PROTOCOL      *Int1AReady;
  

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID**)&SwDispatch
                    );
  ASSERT_EFI_ERROR(Status);
  SwContext.SwSmiInputValue = (UINTN)-1;
  Status = SwDispatch->Register(SwDispatch, TcmSmmDispatcher, &SwContext, &SwHandle);
  if(EFI_ERROR(Status)){
    return Status;
  }

//AllocatePool() cannot be used as it will get memory from SMRAM which BS cannot visit.
//Int1AReady = AllocatePool(sizeof(*Int1AReady));
  Status = gBS->AllocatePool(EfiBootServicesData, sizeof(*Int1AReady), (VOID**)&Int1AReady);
  if(EFI_ERROR(Status)){
    return Status;
  }

  Handle = NULL;
  Int1AReady->SwSmiInputValue = (UINT8)SwContext.SwSmiInputValue;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gTcmSmmInt1AReadyProtocolGuid, 
                  Int1AReady,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);
  
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid, 
                    NULL, 
                    (VOID**)&gSmmCpu
                    );
  ASSERT_EFI_ERROR(Status);

  Status = gBS->LocateProtocol (
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  NULL,
                  (VOID**)&gSmmBaseHelperReady
                  );
  DEBUG((EFI_D_ERROR, "(L%d) %a() %r\n", __LINE__, __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
      gFrameworkSmst = gSmmBaseHelperReady->FrameworkSmst;
  }

  
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID**)&gSmmVariable
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gTcmSmmHashSm3ProtocolGuid,
                    NULL,
                    (VOID**)&gSmmSm3Hash
                    );
  ASSERT_EFI_ERROR(Status);

// Even TCM Protocol is not installed (TCM not present), we still need to handle INT1A
// Assuming TCM Protocol is installed before SMRAM is closed
  TcmHandler(NULL, NULL);
  
  return EFI_SUCCESS;
}

