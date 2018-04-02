/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcgSmm.c

Abstract: 
  Tcg legacy support.

Revision History:

Bug 3843 - Fix sometimes int19h loader IPL measurment will be failed.
TIME:       2012-06-26
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Core
$TECHNICAL: 
  1. Enum blockio to read IPL data by BBS devicepath may not match real
     boot device. Now we issue a int 1A hook before csm jump to 7c00.
$END--------------------------------------------------------------------

Bug 3592 - Cpu save state may write error under IA32 CPU. 
TIME:       2012-04-28
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. R8 SMM Arch may destroy cpu save state when do writeback under
     IA32 CPU. So we should update save state buffer to avoid it.
$END--------------------------------------------------------------------


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

#include <PiDxe.h>
#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/SmBios.h>

#include <Guid/GlobalVariable.h>
#include <Guid/SmBios.h>
#include <Guid/HobList.h>
#include <Guid/TcgEventHob.h>
#include <Guid/EventGroup.h>
#include <Guid/MemoryOverwriteControl.h>

#include <Framework/SmmCis.h>

#include <Protocol/LegacyBios.h>
#include <Protocol/TcgService.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/TcgSmmInt1AReady.h>
#include <protocol/SmmBaseHelperReady.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmCommLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesLib.h>

#include "TpmComm.h"
#include "TcgSmm.h"

#define SMIFN_TCG 0xB3
#define EV_COMPACT_HASH             11

#define EFI_CALLING_INT19           "Calling INT 19H"

EFI_SMM_VARIABLE_PROTOCOL           *mSmmVariable;
EFI_SMM_CPU_PROTOCOL                *mSmmCpu;
EFI_SMM_SYSTEM_TABLE                *mFrameworkSmst  = NULL;
EFI_SMM_BASE_HELPER_READY_PROTOCOL  *mSmmBaseHelperReady;


#pragma pack (1)
typedef struct _EFI_TCG_CLIENT_ACPI_TABLE {
  EFI_ACPI_DESCRIPTION_HEADER       Header;
  UINT16                            PlatformClass;
  UINT32                            Laml;
  EFI_PHYSICAL_ADDRESS              Lasa;
} EFI_TCG_CLIENT_ACPI_TABLE;
#pragma pack ()

typedef struct _TCG_SMM_DATA {
  TCG_EFI_BOOT_SERVICE_CAPABILITY   BsCap;
  EFI_TCG_CLIENT_ACPI_TABLE         *TcgAcpiTable;
  UINTN                             EventLogSize;
  UINT8                             *LastEvent;
  TIS_TPM_HANDLE                    TpmHandle;
} TCG_SMM_DATA;

EFI_TCG_CLIENT_ACPI_TABLE    mTcgAcpiTable;

TCG_SMM_DATA                 mTcgSmmData = {
  { 0 },
  &mTcgAcpiTable
};

VOID
UpdateLastEventAddress (
  VOID
  )
{
  TCG_PCR_EVENT_HDR     *EventHdr;
  VOID                  *LastEvent;

  if (mTcgSmmData.TcgAcpiTable->Lasa == 0) {
    return;
  }

  LastEvent = NULL;
  for (
    EventHdr = (TCG_PCR_EVENT_HDR *) (UINTN) mTcgSmmData.TcgAcpiTable->Lasa;
    EventHdr->PCRIndex != (TCG_PCRINDEX) -1;
    EventHdr = (TCG_PCR_EVENT_HDR *) ((UINT8 *) EventHdr + sizeof (TCG_PCR_EVENT_HDR) + EventHdr->EventSize)
    ) {
    LastEvent = EventHdr;
  }

  ASSERT (LastEvent >= (VOID *) mTcgSmmData.LastEvent);
  mTcgSmmData.LastEvent     = LastEvent;
  mTcgSmmData.EventLogSize  = (UINTN) EventHdr - (UINTN) mTcgSmmData.TcgAcpiTable->Lasa;
}


/**
  This service provides EFI protocol capability information, state information 
  about the TPM, and Event Log state information.

  @param  This                   Indicates the calling context
  @param  ProtocolCapability     The callee allocates memory for a TCG_BOOT_SERVICE_CAPABILITY 
                                 structure and fills in the fields with the EFI protocol 
                                 capability information and the current TPM state information.
  @param  TCGFeatureFlags        This is a pointer to the feature flags. No feature 
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
TcgSmmStatusCheck (
  IN      EFI_TCG_PROTOCOL          *This,
  OUT     TCG_EFI_BOOT_SERVICE_CAPABILITY
                                    *ProtocolCapability,
  OUT     UINT32                    *TCGFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS      *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  )
{
  TCG_SMM_DATA                      *TcgData;

  TcgData = &mTcgSmmData;

  if (ProtocolCapability != NULL) {
    *ProtocolCapability = TcgData->BsCap;
  }

  if (TCGFeatureFlags != NULL) {
    *TCGFeatureFlags = 0;
  }

  if (EventLogLocation != NULL) {
    *EventLogLocation = TcgData->TcgAcpiTable->Lasa;
  }

  if (EventLogLastEntry != NULL) {
    if (TcgData->BsCap.TPMDeactivatedFlag) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)TcgData->LastEvent;
    }
  }

  return EFI_SUCCESS;
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
TcgSmmHashAll (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCG_ALGORITHM_ID          AlgorithmId,
  IN OUT  UINT64                    *HashedDataLen,
  IN OUT  UINT8                     **HashedDataResult
  )
{
  if (HashedDataLen == NULL || HashedDataResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (AlgorithmId) {
    case TPM_ALG_SHA:
      if (*HashedDataLen < sizeof (TPM_DIGEST)) {
        *HashedDataLen = sizeof (TPM_DIGEST);
        return EFI_BUFFER_TOO_SMALL;
      }
      *HashedDataLen = sizeof (TPM_DIGEST);

      return TpmCommHashAll (
               HashData,
               (UINTN) HashDataLen,
               (TPM_DIGEST*)*HashedDataResult
               );
    default:
      return EFI_UNSUPPORTED;
  }
}

/**
  Add a new entry to the Event Log.

  @param[in] TcgData       TCG_DXE_DATA structure.
  @param[in] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in] NewEventData  Pointer to the new event data.  
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
EFIAPI
TcgSmmLogEventI (
  IN      TCG_SMM_DATA              *TcgData,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  TcgData->LastEvent = (UINT8*)(UINTN)TcgData->TcgAcpiTable->Lasa;
  return TpmCommLogEvent (
            &TcgData->LastEvent,
            &TcgData->EventLogSize,
            (UINTN)TcgData->TcgAcpiTable->Laml,
            NewEventHdr,
            NewEventData
            );
}

/**
  This service abstracts the capability to add an entry to the Event Log.

  @param  This                   Indicates the calling context
  @param  TCGLogData             Pointer to the start of the data buffer containing 
                                 the TCG_PCR_EVENT data structure. All fields in 
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
TcgSmmLogEvent (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  )
{
  TCG_SMM_DATA  *TcgData;

  TcgData = &mTcgSmmData;
  
  if (TcgData->BsCap.TPMDeactivatedFlag) {
    return EFI_DEVICE_ERROR;
  }
  return TcgSmmLogEventI (
           TcgData,
           (TCG_PCR_EVENT_HDR*)TCGLogData,
           TCGLogData->Event
           );
}

/**
  This service is a proxy for commands to the TPM.

  @param  This                        Indicates the calling context
  @param  TpmInputParameterBlockSize  Size of the TPM input parameter block
  @param  TpmInputParameterBlock      Pointer to the TPM input parameter block
  @param  TpmOutputParameterBlockSize Size of the TPM output parameter block
  @param  TpmOutputParameterBlock     Pointer to the TPM output parameter block

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Invalid ordinal.
  @retval EFI_UNSUPPORTED        Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval EFI_TIMEOUT            The TIS timed-out.
**/
EFI_STATUS
EFIAPI
TcgSmmPassThroughToTpm (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT32                    TpmInputParameterBlockSize,
  IN      UINT8                     *TpmInputParameterBlock,
  IN      UINT32                    TpmOutputParameterBlockSize,
  IN      UINT8                     *TpmOutputParameterBlock
  )
{
  TCG_SMM_DATA                      *TcgData;

  TcgData = &mTcgSmmData;

  return TisPcExecute (
           TcgData->TpmHandle,
           "%r%/%r",
           TpmInputParameterBlock,
           (UINTN) TpmInputParameterBlockSize,
           TpmOutputParameterBlock,
           (UINTN) TpmOutputParameterBlockSize
           );
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and add an entry to the Event Log.

  @param[in]      TcgData       TCG_DXE_DATA structure.
  @param[in]      HashData      Physical address of the start of the data buffer 
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in, out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
TcgSmmHashLogExtendEventI (
  IN      TCG_SMM_DATA              *TcgData,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN OUT  TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;

  if (HashDataLen > 0) {
    Status = TpmCommHashAll (
               HashData,
               (UINTN) HashDataLen,
               &NewEventHdr->Digest
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = TpmCommExtend (
             TcgData->TpmHandle,
             &NewEventHdr->Digest,
             NewEventHdr->PCRIndex,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    Status = TcgSmmLogEventI (TcgData, NewEventHdr, NewEventData);
  }

  return Status;
}

/**
  This service abstracts the capability to do a hash operation on a data buffer,
  extend a specific TPM PCR with the hash result, and add an entry to the Event Log

  @param  This                   Indicates the calling context
  @param  HashData               Physical address of the start of the data buffer 
                                 to be hashed, extended, and logged.
  @param  HashDataLen            The length, in bytes, of the buffer referenced by HashData
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  TCGLogData             The physical address of the start of the data 
                                 buffer containing the TCG_PCR_EVENT data structure.
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
TcgSmmHashLogExtendEvent (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TPM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  )
{
  TCG_SMM_DATA  *TcgData;

  TcgData = &mTcgSmmData;
  
  if (TcgData->BsCap.TPMDeactivatedFlag) {
    return EFI_DEVICE_ERROR;
  }
    
  if (AlgorithmId != TPM_ALG_SHA) {
    return EFI_UNSUPPORTED;
  }

  return TcgSmmHashLogExtendEventI (
           TcgData,
           (UINT8 *) (UINTN) HashData,
           HashDataLen,
           (TCG_PCR_EVENT_HDR*)TCGLogData,
           TCGLogData->Event
           );
}

typedef enum {
  TCG_OK,
  TPM_RET_BASE,
  TCG_GENERAL_ERROR = TPM_RET_BASE, // A general unidentified error occurred.
  TCG_TPM_IS_LOCKED,                // The access cannot be granted the device is open.
  TCG_NO_RESPONSE,                  // No response from the TPM device.
  TCG_INVALID_RESPONSE,             // The response from the TPM was invalid.
  TCG_INVALID_ACCESS_REQUEST,       // The access parameters for this function are invalid.
  TCG_FIRMWARE_ERROR,               // Firmware error during start up.
  TCG_INTEGRITY_CHECK_FAILED,       // Integrity checks of TPM parameter failed.
  TCG_INVALID_DEVICE_ID,            // The device ID for the TPM is invalid.
  TCG_INVALID_VENDOR_ID,            // The vendor ID for the TPM is invalid.
  TCG_UNABLE_TO_OPEN,               // Unable to open a connection to the TPM device.
  TCG_UNABLE_TO_CLOSE,              // Unable to close a connection to the TPM device.
  TCG_RESPONSE_TIMEOUT,             // Time out for TPM response.
  TCG_INVALID_COM_REQUEST,          // The parameters for the communication access are invalid.
  TCG_INVALID_ADR_REQUEST,          // The address parameter for the access is invalid.
  TCG_WRITE_BYTE_ERROR,             // Bytes write error on the interface.
  TCG_READ_BYTE_ERROR,              // Bytes read error on the interface.
  TCG_BLOCK_WRITE_TIMEOUT,          // Blocks write error on the interface.
  TCG_CHAR_WRITE_TIMEOUT,           // Bytes write time out on the interface.
  TCG_CHAR_READ_TIMEOUT,            // Bytes read time out on the interface.
  TCG_BLOCK_READ_TIMEOUT,           // Blocks read error on the interface.
  TCG_TRANSFER_ABORT,               // Transfer abort in communication with TPM device.
  TCG_INVALID_DRV_FUNCTION,         // Function number (AL-Register) invalid for this driver.
  TCG_OUTPUT_BUFFER_TOO_SHORT,      // Output buffer for the TPM response to short.
  TCG_FATAL_COM_ERROR,              // Fatal error in TPM communication.
  TCG_INVALID_INPUT_PARA,           // Input parameter for the function invalid.
  TCG_TCG_COMMAND_ERROR,            // Error during execution of a TCG command.
  TCG_Reserved1,
  TCG_Reserved2,
  TCG_Reserved3,
  TCG_Reserved4,
  TCG_Reserved5,
  TCG_Reserved6,
  TCG_INTERFACE_SHUTDOWN,           // TPM BIOS interface has been shutdown using the TCG_ShutdownPreBootInterface.
  TCG_DRIVER_UNSUPPORTED,               // The requested function is not supported.
  TCG_DRIVER_TPM_NOT_PRESENT,           // The TPM is not installed.
  TCG_DRIVER_TPM_DEACTIVATED,           // The TPM is deactivated.
  TCG_VENDOR_BASE_RET     = 0x80    // Start point for return codes are reserved for use by TPM vendors.
} TCG_DRIVER_ERROR_CODE;

typedef enum {
  TCG_PC_OK,                        // The function returned successful.
  TCG_PC_TPMERROR,                  // TCG_PC_OK + 01h | (TPM driver error << 16)
                                    // The TPM driver returned an error.
                                    // The upper 16 bits contain the actual error code returned by the driver
  TCG_PC_LOGOVERFLOW,               // There is insufficient memory to create the log entry.
  TCG_PC_UNSUPPORTED,               // The requested function is not supported.
} TCG_PC_ERROR_CODE;

#define TCG_DRIVER_ERROR(x) (TCG_PC_TPMERROR | ((x) << 16))

UINT32  mTcgLegacyStatus[] = {
  TCG_PC_OK,                                           // #define EFI_SUCCESS               0
  TCG_GENERAL_ERROR,                                   // #define EFI_LOAD_ERROR            EFIERR (1)
  TCG_DRIVER_ERROR (TCG_INVALID_INPUT_PARA),           // #define EFI_INVALID_PARAMETER     EFIERR (2)
  TCG_PC_UNSUPPORTED,                                  // #define EFI_UNSUPPORTED           EFIERR (3)
  TCG_DRIVER_ERROR (TCG_INVALID_INPUT_PARA),           // #define EFI_BAD_BUFFER_SIZE       EFIERR (4)
  TCG_DRIVER_ERROR (TCG_OUTPUT_BUFFER_TOO_SHORT),      // #define EFI_BUFFER_TOO_SMALL      EFIERR (5)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_NOT_READY             EFIERR (6)
  TCG_DRIVER_ERROR (TCG_INVALID_RESPONSE),             // #define EFI_DEVICE_ERROR          EFIERR (7)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_WRITE_PROTECTED       EFIERR (8)
  TCG_PC_LOGOVERFLOW,                                  // #define EFI_OUT_OF_RESOURCES      EFIERR (9)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_VOLUME_CORRUPTED      EFIERR (10)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_VOLUME_FULL           EFIERR (11)
  TCG_DRIVER_ERROR (TCG_DRIVER_TPM_NOT_PRESENT),       // #define EFI_NO_MEDIA              EFIERR (12)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_MEDIA_CHANGED         EFIERR (13)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_NOT_FOUND             EFIERR (14)
  TCG_DRIVER_ERROR (TCG_UNABLE_TO_OPEN),               // #define EFI_ACCESS_DENIED         EFIERR (15)
  TCG_DRIVER_ERROR (TCG_NO_RESPONSE),                  // #define EFI_NO_RESPONSE           EFIERR (16)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_NO_MAPPING            EFIERR (17)
  TCG_DRIVER_ERROR (TCG_CHAR_READ_TIMEOUT),            // #define EFI_TIMEOUT               EFIERR (18)
  TCG_DRIVER_ERROR (TCG_INTERFACE_SHUTDOWN),           // #define EFI_NOT_STARTED           EFIERR (19)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_ALREADY_STARTED       EFIERR (20)
  TCG_DRIVER_ERROR (TCG_TRANSFER_ABORT),               // #define EFI_ABORTED               EFIERR (21)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_ICMP_ERROR            EFIERR (22)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_TFTP_ERROR            EFIERR (23)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_PROTOCOL_ERROR        EFIERR (24)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_INCOMPATIBLE_VERSION  EFIERR (25)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_SECURITY_VIOLATION    EFIERR (26)
  TCG_DRIVER_ERROR (TCG_GENERAL_ERROR),                // #define EFI_CRC_ERROR             EFIERR (27)
};

// INT 1Ah interface status flag
#define  IFCE_OFF        0x1
UINT8 mIfceFlags = 0;

#define LEGACY_TCG_STATUS(Status)   mTcgLegacyStatus[(Status) & ~MAX_BIT]

BOOLEAN    mTpmPresent    = FALSE;
BOOLEAN    mOpbNeedSet[] = {
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

UINT32
ReadRegister32 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex
  )
{
  EFI_STATUS                   Status;
  UINT32                       Uint32;

  Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      sizeof (Uint32),
                      Register,
                      CpuIndex,
                      &Uint32
                      );
  ASSERT_EFI_ERROR (Status);

  return Uint32;
}
UINT16
ReadRegister16 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex
  )
{
  EFI_STATUS                   Status;
  UINT16                       Uint16;

  Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      sizeof (Uint16),
                      Register,
                      CpuIndex,
                      &Uint16
                      );
  ASSERT_EFI_ERROR (Status);

  return Uint16;
}

VOID
WriteRegister32 (
  EFI_SMM_SAVE_STATE_REGISTER  Register,
  UINTN                        CpuIndex,
  UINT32                       Data32
  )
{
  EFI_STATUS  Status;

  DEBUG((EFI_D_ERROR, "%a() R:%d, I:%d, D:0x%X\n", __FUNCTION__, Register, CpuIndex, Data32));

  Status = mSmmCpu->WriteSaveState (
                        mSmmCpu,
                        sizeof (Data32),
                        Register,
                        CpuIndex,
                        &Data32
                        );
  ASSERT_EFI_ERROR(Status);

  if(mFrameworkSmst != NULL){
    EFI_SMI_CPU_SAVE_STATE *SaveState;
    SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
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

EFI_STATUS
EFIAPI
TcgSmmDispatcher (
  IN EFI_HANDLE                        DispatchHandle,
  IN CONST EFI_SMM_SW_REGISTER_CONTEXT *Context,
  IN OUT EFI_SMM_SW_CONTEXT            *SwContext,
  IN OUT UINTN                         *CommBufferSize
  )
{
  EFI_STATUS                      Status;
  TCG_EFI_BOOT_SERVICE_CAPABILITY Capability;
  EFI_PHYSICAL_ADDRESS            EventLogLocation;
  EFI_PHYSICAL_ADDRESS            EventLogLastEntry;
  UINT32                          FeatureFlags;
  UINT8                           *DigestPointer;
  UINT8                           FunctionId;
  OPB_TPMStruc                    *OpbStructure;
  UINT32                          EventNumber;
  UINT64                          HashedDataLen;
  
  Status = EFI_SUCCESS;
  //
  // Get the function ID from AH register
  //
  FunctionId = (UINT8)(ReadRegister16 (EFI_SMM_SAVE_STATE_REGISTER_RAX, SwContext->SwSmiCpuIndex) >> 8);

  if (!mTpmPresent) {
    Status = EFI_NO_MEDIA;
    goto lExit;
  }
  
  UpdateLastEventAddress ();

  if (FunctionId == TCG_StatusCheck) {
    Status = TcgSmmStatusCheck (NULL, &Capability, &FeatureFlags, &EventLogLocation, &EventLogLastEntry);

    WriteRegister32 (
      EFI_SMM_SAVE_STATE_REGISTER_RBX,
      SwContext->SwSmiCpuIndex,
      SIGNATURE_32 ('T', 'C', 'P', 'A') // 41504354h
      );
    WriteRegister16 (
      EFI_SMM_SAVE_STATE_REGISTER_RCX,
      SwContext->SwSmiCpuIndex,
      ((UINT16) Capability.ProtocolSpecVersion.Major) << 8 | Capability.ProtocolSpecVersion.Minor
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

    if (mIfceFlags & IFCE_OFF) {
      Status = EFI_NOT_STARTED;
    }
    goto lExit;
  }

  //
  // The rest of the functions need to look at the TPM state and Interface Off flag
  // (interface off takes precedence over TPM state)
  //
  if (mIfceFlags & IFCE_OFF) {
    Status = EFI_NOT_STARTED;
    goto lExit;
  }

  switch (FunctionId) {

  case TCG_HashLogExtendEvent:
    {
      IPB_HashLogExtendEventStruc   *Ipb;
      OPB_HashLogExtendEventStruc   *Opb;

      Ipb = (IPB_HashLogExtendEventStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_HashLogExtendEventStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
      if (Ipb->ulog.longLog.LogDataLen !=
          ((TCG_PCClientPCREventStruc *) (UINTN) Ipb->ulog.longLog.LogDataPtr)->eventDataSize +
          sizeof (TCG_PCClientPCREventStruc) - sizeof (UINT8)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (((TCG_PCClientPCREventStruc *) (UINTN) Ipb->ulog.longLog.LogDataPtr)->PCRIndex != Ipb->PCRIndex) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      Status = TcgSmmHashLogExtendEvent (
                 NULL,
                 (EFI_PHYSICAL_ADDRESS) (UINTN) Ipb->HashDataPtr,
                 Ipb->HashDataLen,
                 TPM_ALG_SHA,
                 (TCG_PCR_EVENT *) (UINTN) Ipb->ulog.longLog.LogDataPtr,
                 &EventNumber,
                 &EventLogLastEntry
                 );
      if (!EFI_ERROR (Status)) {
        Opb->EventNumber = EventNumber;
        Opb->OPBLength   = sizeof (OPB_HashLogExtendEventStruc);
        CopyMem (Opb->HashValue, ((TCG_PCClientPCREventStruc *) (UINTN)Ipb->ulog.longLog.LogDataPtr)->digest.sha_digest, 20);
      }

      break;
    }

  case TCG_PassThroughToTPM:
    {
      IPB_TPMStruc        *Ipb;
      OPB_TPMStruc        *Opb;

      Ipb = (IPB_TPMStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_TPMStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      if (Ipb->IPBLength != 8 + (UINT16) SwapBytes32 (((TPM_RQU_COMMAND_HDR *) &Ipb->TPMOperandIn)->paramSize)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      Status = TcgSmmPassThroughToTpm (
                                      NULL,
                                      Ipb->IPBLength - 8,
                                      &Ipb->TPMOperandIn,
                                      Ipb->OPBLength - 4,
                                      &Opb->TPMOperandOut
                                      );
      if (!EFI_ERROR (Status)) {
        Opb->OPBLength = 4 + (UINT16) SwapBytes32 (((TPM_RSP_COMMAND_HDR *) &Opb->TPMOperandOut)->paramSize);
        //
        // Update output length if what caller gives to me is shorter than we want to receive from TPM
        //
        if (Ipb->OPBLength > Opb->OPBLength) {
          Opb->OPBLength = Ipb->OPBLength;
        }
      }
      break;
    }

  case TCG_ShutdownPreBootInterface:
    {
      mIfceFlags |= IFCE_OFF;
      Status = EFI_SUCCESS;
      break;
    }

  case TCG_HashLogEvent:
    {
      IPB_HashLogEventStruc      *Ipb;
      OPB_HashLogEventStruc      *Opb;

      Ipb = (IPB_HashLogEventStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Opb = (OPB_HashLogEventStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      if (Ipb->LogDataLen !=
              sizeof (TCG_PCClientPCREventStruc)
              + ((TCG_PCClientPCREventStruc *) (UINTN) Ipb->LogDataPtr)->eventDataSize - sizeof (UINT8)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (Ipb->PCRIndex != ((TCG_PCClientPCREventStruc *) (UINTN) Ipb->LogDataPtr)->PCRIndex) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      if (Ipb->LogEventType != ((TCG_PCClientPCREventStruc *) (UINTN) Ipb->LogDataPtr)->eventType) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      // If either the hash data length or hash data pointer is null we do not need
      // to hash the data since the digest is already included in the event structure.
      //
      if ((Ipb->HashDataLen != 0) && ((UINT8 *) (UINTN) Ipb->HashDataPtr != NULL)) {
        HashedDataLen = 20;
        DigestPointer = (UINT8 *) ((TCG_PCClientPCREventStruc *) (UINTN) Ipb->LogDataPtr)->digest.sha_digest;
        Status = TcgSmmHashAll (
                               NULL,
                               (UINT8 *) (UINTN) Ipb->HashDataPtr,
                               Ipb->HashDataLen,
                               TPM_ALG_SHA,
                               &HashedDataLen,
                               &DigestPointer
                               );
        if (EFI_ERROR (Status)) {
          break;
        }
      }
      Status = TcgSmmLogEvent (
                              NULL,
                              (TCG_PCR_EVENT *) (UINTN) Ipb->LogDataPtr,
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

  case TCG_HashAll:
    {
      IPB_HashAll_Struc       *Ipb;
      SHADigestStruc          *Digest;

      Ipb    = (IPB_HashAll_Struc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Digest = (SHADigestStruc *)    (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);

      HashedDataLen = 20;
      DigestPointer = (UINT8 *) Digest->sha_digest;
      Status = TcgSmmHashAll (
                 NULL,
                 (UINT8 *) (UINTN) Ipb->HashDataPtr,
                 Ipb->HashDataLen,
                 Ipb->AlgorithmID,
                 &HashedDataLen,
                 (UINT8 **) &DigestPointer
                 );
      break;
    }
  case TCG_CompactHashLogExtendEvent:
    {
      UINT8                *HashData;
      TCG_PCR_EVENT        *Event;
      UINT32               EventData;
      UINT64               HashDataLen;

      HashData    = (UINT8 *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      EventData   =                   ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
      HashDataLen = (UINT64)          ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RCX, SwContext->SwSmiCpuIndex);

      Event = AllocatePool (sizeof (TCG_PCR_EVENT_HDR) + sizeof (EventData));
      Event->PCRIndex  = ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex);
      Event->EventType = EV_COMPACT_HASH;
      Event->EventSize = sizeof (EventData);
      CopyMem (Event->Event, &EventData, Event->EventSize);

      Status = TcgSmmHashLogExtendEvent (
                 NULL,
                 (EFI_PHYSICAL_ADDRESS) (UINTN) HashData,
                 HashDataLen,
                 TPM_ALG_SHA,
                 Event,
                 &EventNumber,
                 &EventLogLastEntry
                 );
      if (!EFI_ERROR (Status)) {
        WriteRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex, EventNumber);
        //
        // Work-around for tcgbios check
        //
        WriteRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDX, SwContext->SwSmiCpuIndex, 1);
      }
      break;
    }
  case TCG_MemoryOverwriteRequest:
    {
      IPB_MORStruc      *Ipb;

      Ipb    = (IPB_MORStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RDI, SwContext->SwSmiCpuIndex);
      Status = mSmmVariable->SmmSetVariable (
                               MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                               &gEfiMemoryOverwriteControlDataGuid,
                               EFI_VARIABLE_NON_VOLATILE |
                                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                 EFI_VARIABLE_RUNTIME_ACCESS,
                               sizeof (UINT8),
                               &Ipb->MORBitValue
                               );
      break;
    }
  
  case TCG_MeasureLoaderIpl:
    {
      UINT8                   Buffer[sizeof(TCG_PCR_EVENT) + sizeof(EFI_CALLING_INT19)];
      TCG_PCR_EVENT           *TcgEvent;
      EFI_PHYSICAL_ADDRESS    EventLogLastEntry;
      UINT32                  EventNumber;

      DEBUG((EFI_D_INFO, "(L%d) TCG_MeasureLoaderIpl\n", __LINE__, Status));

      TcgEvent = (TCG_PCR_EVENT*)Buffer;
      TcgEvent->PCRIndex  = 4;
      TcgEvent->EventType = EV_EFI_BOOT_SERVICES_APPLICATION;
      TcgEvent->EventSize = sizeof(EFI_CALLING_INT19);
      CopyMem(TcgEvent->Event, EFI_CALLING_INT19, sizeof(EFI_CALLING_INT19));      
      Status = TcgSmmHashLogExtendEvent (
                 NULL,
                 0x7C00,
                 512,
                 TPM_ALG_SHA,
                 TcgEvent,
                 &EventNumber,
                 &EventLogLastEntry
                 );
      if(EFI_ERROR(Status)){
        DEBUG((EFI_D_ERROR, "(L%d) %r\n", __LINE__, Status));
      }
      FunctionId = TCG_StatusCheck;
      break;     
    }
  
  default:
    {
      Status = EFI_UNSUPPORTED;
      FunctionId = TCG_StatusCheck;
      break;
    }
  }

lExit:
  if (EFI_ERROR (Status) && mOpbNeedSet[FunctionId]) {
    OpbStructure = (OPB_TPMStruc *) (UINTN) ReadRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RSI, SwContext->SwSmiCpuIndex);
    OpbStructure->OPBLength = 4;
    OpbStructure->Reserved1 = 0;
  }
  WriteRegister32 (EFI_SMM_SAVE_STATE_REGISTER_RAX, SwContext->SwSmiCpuIndex, LEGACY_TCG_STATUS (Status));

  return EFI_SUCCESS;
}

VOID                      *mRegisteration;

VOID
EFIAPI
TcgHandler (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                        Status;
  UINT32                            FeatureFlags;
  EFI_TCG_PROTOCOL                  *Tcg;
  EFI_PHYSICAL_ADDRESS              EventLogLocation;
  EFI_PHYSICAL_ADDRESS              EventLogLastEntry;
  TPM_PERMANENT_FLAGS               PFlags;

  
  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, &Tcg);
  if (EFI_ERROR (Status)) {
    return;
  }

  mTpmPresent = TRUE;
  Status = Tcg->StatusCheck (
                  Tcg,
                  &mTcgSmmData.BsCap,
                  &FeatureFlags,
                  &EventLogLocation,
                  &EventLogLastEntry
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Also get disable state from TPM hardware
  //
  Status = TpmCommGetFlags (
              (TIS_TPM_HANDLE) (UINTN) TPM_BASE_ADDRESS,
              TPM_CAP_FLAG_PERMANENT,
              &PFlags,
              sizeof (PFlags)
              );

  if (EFI_ERROR (Status) || mTcgSmmData.BsCap.TPMDeactivatedFlag || PFlags.disable) {
    mIfceFlags |= IFCE_OFF;
  } else {
    mTcgSmmData.TpmHandle          = (TIS_TPM_HANDLE)(UINTN)TPM_BASE_ADDRESS;
    mTcgSmmData.TcgAcpiTable->Lasa = (UINT64) EventLogLocation;
    mTcgSmmData.TcgAcpiTable->Laml = (UINT32) 0x10000; // LAML is 64KB
    mTcgSmmData.LastEvent          = (UINT8 *) (UINTN)EventLogLastEntry;
    mTcgSmmData.EventLogSize       = (UINT32) (EventLogLastEntry
                                                - EventLogLocation
                                                + sizeof (TCG_PCR_EVENT_HDR)
                                                + ((TCG_PCR_EVENT_HDR *) (UINTN) EventLogLastEntry)->EventSize
                                                );
  }
}

EFI_STATUS
EFIAPI
InitializeTcgSmm (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL     *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT       SwContext;
  EFI_HANDLE                        SwHandle;
  EFI_HANDLE                        Handle;
  TCG_SMM_INT1A_READY_PROTOCOL      *Int1AReady;
  
  //
  //  Get the Sw dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID**)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  SwContext.SwSmiInputValue = (UINTN) -1;
  Status = SwDispatch->Register (SwDispatch, TcgSmmDispatcher, &SwContext, &SwHandle);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the Int1AReady protocol so that the TcgSmmInstallInt1A module can get dispatched.
  // We also pass the SwSmiInputValue to TcgSmmInstallInt1A module.
  //
  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (*Int1AReady), (VOID **) &Int1AReady);
  ASSERT_EFI_ERROR (Status);
  Handle = NULL;
  Int1AReady->SwSmiInputValue = (UINT8) SwContext.SwSmiInputValue;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gTcgSmmInt1AReadyProtocolGuid, Int1AReady,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  //
  // Get SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid, 
                    NULL, 
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  NULL,
                  (VOID**)&mSmmBaseHelperReady
                  );
//DEBUG((EFI_D_ERROR, "(L%d) %a() %r\n", __LINE__, __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    mFrameworkSmst = mSmmBaseHelperReady->FrameworkSmst;
  }

  
  //
  // Locate SmmVariableProtocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID**)&mSmmVariable
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Even TCG Protocol is not installed (TPM not present), we still need to handle INT1A
  // Assuming TCG Protocol is installed before SMRAM is closed
  //
  /* Causing Invalid OP CODE exception because the callback is running in a different context (non-SMM mode).
  EfiCreateProtocolNotifyEvent (
    &gEfiTcgProtocolGuid,
    TPL_CALLBACK,
    TcgHandler,
    NULL,
    &mRegisteration
    );
  */
  //
  // Assuming TCG Protocol has been installed.
  //
  TcgHandler (NULL, NULL);
  
  return EFI_SUCCESS;
}

