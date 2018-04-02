/** @file

Copyright (c) 2006 - 2012, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmbiosSmm.c

Abstract:
  Source file for the Smm Smbios driver.

Revision History:

**/

#include <SmbiosSmm.h>

NV_MEDIA_ACCESS_PROTOCOL *pMediaAccessProtocol;
EFI_SMM_CPU_PROTOCOL     *mSmmCpu;

EFI_STATUS
DmiPnP50_57Callback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
/*++

Routine Description:
  It is DMI_PNP_50_57 SMM function defined in CSM spec.
  This notification function is called when an SMM Mode is invoked through SMI.
  This may happen during RT, so it must be RT safe.

Arguments:
  DispatchHandle  - EFI Handle
  DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

Returns:
  None

--*/
{
  EFI_STATUS                  Status;
  EFI_DWORD_REGS              *RegBuf;
  UINTN                       Index;
  UINT32                      RegESI;
  UINTN                       CpuIndex;
  EFI_SMM_SAVE_STATE_IO_INFO  IoState;
  UINT16                      *ParameterBuffer;
  UINT16                      FunctionId;
  UINT16                      ReturnCode;
  //
  // Check parameter condition
  //
  CpuIndex = 0;
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                        EFI_SMM_SAVE_STATE_REGISTER_IO,
                        Index,
                        &IoState
                        );
    if (!EFI_ERROR (Status) && (IoState.IoData == SMM_PnP_BIOS_CALL)) {
      CpuIndex = Index;
      break;
    }
  }

  //
  // Ready save state for register ESI
  //
  RegESI = 0;
  Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      sizeof (UINT32),
                      EFI_SMM_SAVE_STATE_REGISTER_RSI,
                      CpuIndex,
                      &RegESI
                      );
  ASSERT_EFI_ERROR (Status);

  //
  // Check parameter condition
  //
  if (RegESI == (UINT32)(-1) || RegESI == 0) {
    return PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
  }
  //
  // Get the pointer to saved CPU regs.
  //
  RegBuf = (VOID*)(UINTN)RegESI;

  //
  // Get the PnP Function call parameter
  // ( + 1) means ???
  //
  ParameterBuffer = (UINT16 *)(UINTN)((RegBuf->SS << 4) + RegBuf->ESP);

  //
  // Fix the entry call
  // ( + 2) means skip CS and IP
  //
  ParameterBuffer += 2;

  //
  // Get the first PnP Function call parameter - Function ID
  //
  FunctionId = *ParameterBuffer;

  //
  // Check FunctionId and call the function
  // PnP BIOS DMI Function Call
  // They are defined in CSM spec, need to be handled.
  //
  switch (FunctionId) {
  case PNPBIOS_FUNCID_GET_SMBIOS_INFORMATION:
      ReturnCode = PnpGetSmbiosInformation(ParameterBuffer);
      break;

  case PNPBIOS_FUNCID_GET_SMBIOS_STRUCTURE:
      ReturnCode = PnpGetSmbiosStructure(ParameterBuffer);
      break;

  case PNPBIOS_FUNCID_SET_SMBIOS_STRUCTURE:
      GetFromFlash ();
      ReturnCode = PnpSetSmbiosStructure(ParameterBuffer);
      break;

  case PNPBIOS_FUNCID_GET_STRUCTURE_CHANGE_INFORMATION:
  case PNPBIOS_FUNCID_SMBIOS_CONTROL:
  case PNPBIOS_FUNCID_GET_GENERAL_PURPOSE_NV_INFORMATION:
  case PNPBIOS_FUNCID_READ_GENERAL_PURPOSE_NV_DATA:
  case PNPBIOS_FUNCID_WRITE_GENERAL_PURPOSE_NV_DATA:
      ReturnCode = PNP_BIOS_DMI_RETURN_FUNCTION_NOT_SUPPORTED;
      break;

  default:
      ReturnCode = PNP_BIOS_DMI_RETURN_UNKNOWN_FUNCTION;
      break;
  }

  RegBuf->EAX = (RegBuf->EAX & 0xFF00) | (UINT32)ReturnCode;

  return EFI_SUCCESS;
}

EFI_STATUS
InitSmbiosSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
/*++

Routine Description:

  Initializes the SMM Platfrom Driver

Arguments:

  ImageHandle   - Pointer to the loaded image protocol for this driver
  SystemTable   - Pointer to the EFI System Table

Returns:

  Status        - EFI_SUCCESS

--*/
{
  EFI_STATUS                                Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT               SwContext;
  EFI_HANDLE                                SwHandle;

  if (FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE == 0 ||
      FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE == 0)
    return EFI_SUCCESS;

  //
  // Locate the SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmCpuProtocolGuid, NULL, (VOID **) &mSmmCpu);
  ASSERT_EFI_ERROR (Status);
  //
  //  Locate the SMM SW dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  ASSERT_EFI_ERROR (Status);

  //
  // Register the DMI PnP 50_57 handler
  //
  Status = gBS->LocateProtocol (&gEfiSmmNvMediaAccessProtocolGuid, NULL, &pMediaAccessProtocol);
  ASSERT_EFI_ERROR (Status);
  Status = AllocDataBuffer();
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = SMM_PnP_BIOS_CALL;
  Status = SwDispatch->Register (
               SwDispatch,
               DmiPnP50_57Callback,
               &SwContext,
               &SwHandle
           );

  return EFI_SUCCESS;
}

