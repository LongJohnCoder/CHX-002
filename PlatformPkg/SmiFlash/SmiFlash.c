/** @file

Copyright (c) 2006 - 2016, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmiFlash.c

Abstract:
  Provides Access to flash backup Services through SMI

Revision History:

**/

#include "SmiFlash.h"
#include <Framework/SmmCis.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BiosIdLib.h>




//---------------------------------------------------------------------------
NV_MEDIA_ACCESS_PROTOCOL              *mMediaAccess;
EFI_SMM_CPU_PROTOCOL                  *mSmmCpu;
UINT8                                 *BlockBuffer;
BOOLEAN                               EnvPrepared  = FALSE;
UINT16                                AcpiPmEnData = 0;
BIOS_ID_IMAGE                         gBiosIdImage;


EFI_STATUS
ProgramFlash (
  IN  UINT32  BiosAddr,
  IN  UINT32  Size,
  IN  UINT32  Buffer
)
{
  EFI_STATUS  Status = EFI_SUCCESS;

  WriteBackInvalidateDataCacheRange((VOID*)(UINTN)BiosAddr, Size);
  if(CompareMem((VOID*)(UINTN)BiosAddr, (VOID*)(UINTN)Buffer, Size) == 0){
    DEBUG((EFI_D_INFO, "%X Equal\n", BiosAddr));
    goto ProcExit;
  }
  Status = mMediaAccess->Erase (
                           mMediaAccess,
                           BiosAddr,
                           Size,
                           SPI_MEDIA_TYPE
                           );
  Status = mMediaAccess->Write (
                           mMediaAccess,
                           BiosAddr,
                           (VOID*)(UINTN)Buffer,
                           Size,
                           SPI_MEDIA_TYPE
                           );
  if (EFI_ERROR(Status)) {
    goto ProcExit;
  }

  WriteBackInvalidateDataCacheRange((VOID*)(UINTN)BiosAddr, Size);
  if (CompareMem((VOID*)(UINTN)BiosAddr, (VOID*)(UINTN)Buffer, Size)){
    DEBUG((EFI_D_ERROR, "After Write Verify Err\n"));
    Status = EFI_DEVICE_ERROR;
  }

ProcExit:
  return Status;
}



EFI_STATUS CheckBiosId(BIOS_ID_IMAGE *Id)
{
  BIOS_ID_IMAGE  *OldBiosId;
  BIOS_ID_STRING *IdStr;
  EFI_STATUS     Status;

  OldBiosId = &gBiosIdImage;
  if(CompareMem(OldBiosId->Signature, Id->Signature, sizeof(OldBiosId->Signature))){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  IdStr = &gBiosIdImage.BiosIdString;
  if(CompareMem(IdStr->BoardId, Id->BiosIdString.BoardId, sizeof(IdStr->BoardId)) ||
     CompareMem(IdStr->OemId, Id->BiosIdString.OemId, sizeof(IdStr->OemId))){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Status = EFI_SUCCESS;

ProcExit:
  return Status;
}



EFI_STATUS
FlashInterface (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  )
{
    EFI_STATUS                   Status;
    UINTN                        Index;
    UINTN                        CpuIndex;
    UINT8                        SubFunction;
    BIOS_UPDATE_BLOCK_PARAMETER  *BlockParam;
    UINT32                       RegEax;
    UINT32                       RegEbx;
    EFI_SMM_SAVE_STATE_IO_INFO   IoState;
    UINT16                       AcpiIoBase;
    

    AcpiIoBase = PcdGet16(AcpiIoPortBaseAddress);
    
    CpuIndex = 0;
    for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
        Status = mSmmCpu->ReadSaveState (
                            mSmmCpu,
                            sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                            EFI_SMM_SAVE_STATE_REGISTER_IO,
                            Index,
                            &IoState
                            );
        if (!EFI_ERROR (Status) && (IoState.IoData == SW_SMI_FLASH_SERVICES)) {
            CpuIndex = Index;
            break;
        }
    }
    if (Index >= gSmst->NumberOfCpus) {
        CpuDeadLoop ();
    }

    //
    // Ready save state for register
    //
    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);

    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RBX,
                        CpuIndex,
                        &RegEbx
                        );
    ASSERT_EFI_ERROR (Status);

    SubFunction = (UINT8)(RegEax >> 8);
    switch (SubFunction) {

      case FUNC_CHECK_BIOS_ID:
        Status = CheckBiosId((BIOS_ID_IMAGE*)(UINTN)RegEbx);
        break;

      case FUNC_PREPARE_BIOS_FLASH_ENV:
        AcpiPmEnData = IoRead16(AcpiIoBase+2);
        IoWrite16(AcpiIoBase+2, AcpiPmEnData & (UINT16)~BIT8);  // disable power button.
        EnvPrepared = TRUE;
        break;

      case FUNC_CLEAR_BIOS_FLASH_ENV:
        if(EnvPrepared){
          IoWrite16(AcpiIoBase, BIT8);           // clear power button status.
          IoWrite16(AcpiIoBase+2, AcpiPmEnData); // restore
          EnvPrepared = FALSE;
        }
        break;
          
      case FUNC_UPDATE_SMBIOS_DATA:
        Status = HandleSmbiosDataRequest((UPDATE_SMBIOS_PARAMETER*)(UINTN)RegEbx);
        break;

      case FUNC_PROGRAM_FLASH:
        BlockParam = (BIOS_UPDATE_BLOCK_PARAMETER*)(UINTN)RegEbx;
        Status = ProgramFlash (
                   BlockParam->BiosAddr, 
                   BlockParam->Size, 
                   BlockParam->Buffer
                   );
        break;

      default:
          Status = RETURN_INVALID_PARAMETER;
          break;
    }

    RegEax = (UINT32)Status;
    Status = mSmmCpu->WriteSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);

    return Status;
}






EFI_STATUS
SmiFlashEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch = NULL;
  EFI_SMM_SW_REGISTER_CONTEXT   SwContext;
  EFI_HANDLE                    Handle;


  Status = GetBiosId(&gBiosIdImage);
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    &SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
      return Status;
  }

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmNvMediaAccessProtocolGuid,
                    NULL,
                    &mMediaAccess
                    );
  ASSERT_EFI_ERROR (Status);                    

  SwContext.SwSmiInputValue = SW_SMI_FLASH_SERVICES;
  Status = SwDispatch->Register (
                         SwDispatch,
                         FlashInterface,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR (Status);

  Status = AllocDataBuffer();
  ASSERT_EFI_ERROR (Status);

  return Status;
}
