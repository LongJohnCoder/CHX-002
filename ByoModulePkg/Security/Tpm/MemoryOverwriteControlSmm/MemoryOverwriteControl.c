/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  MemoryOverwriteControl.c

Abstract:


Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

**/
/*++
  This file contains a 'Sample Driver' and is licensed as such  
  under the terms of your license agreement with Intel or your  
  vendor.  This file may be modified by the user, subject to    
  the additional terms of the license agreement                 
--*/
/*++

Copyright (c) 2007 - 2009 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcgMor.c

Abstract:

  TCG MOR (Memory Overwrite Request) Control Module

--*/

#include "MemoryOverwriteControl.h"
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/GlobalNvsArea.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Guid/MemoryOverwriteControl.h>


EFI_SMM_VARIABLE_PROTOCOL         *mSmmVariable;
UINT8                             *mNvsArea;
NVS_FIELD_MAP                     *mNvsAreaMap;


EFI_STATUS
SmiMorCallback (
  IN EFI_HANDLE                    DispatchHandle,
  IN CONST VOID                    *Context,
  IN OUT VOID                      *CommBuffer,
  IN OUT UINTN                     *CommBufferSize          
  )
/*++

Routine Description:

  When an smi event for Mor happen.

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS            Status;
  UINTN                 DataSize;
  UINT8                 MorControl;
  UINT8                 Revision;
  UINT8                 TcgParamter;
  UINT8                 MorData;


  Revision = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_REVISION].Offset];
  TcgParamter = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_PARAMTER].Offset];
  MorData = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_MOR_DATA].Offset];

 if (Revision >= GLOBAL_NVS_AREA_RIVISION_1) {
  if (TcgParamter == 1) {
    //
    // Called from ACPI _DSM method, save the MOR data to variable.
    //
    MorControl = MorData;
  } else if (TcgParamter == 2) {
    //
    // Called from ACPI _PTS method, setup ClearMemory flags if needed.
    //
    DataSize = sizeof (UINT8);
    Status = mSmmVariable->SmmGetVariable (
               MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
               &gEfiMemoryOverwriteControlDataGuid,
               NULL,
               &DataSize,
               &MorControl
               );
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    if (MOR_CLEAR_MEMORY_VALUE (MorControl) == 0x0) {
      return EFI_SUCCESS;
    }
    MorControl &= 0xFE;
  }

  DataSize = sizeof (UINT8);
  Status = mSmmVariable->SmmSetVariable (
             MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
             &gEfiMemoryOverwriteControlDataGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             DataSize,
             &MorControl
             );
 }   
 return EFI_SUCCESS;
 
}

EFI_STATUS
EFIAPI
MorDriverEntryPoint (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
/*++

Routine Description:

  TCG MOR Driver Entry Point

Arguments:

  ImageHandle           - ImageHandle of the loaded driver
  SystemTable           - Pointer to the System Table

Returns:

--*/
{
  EFI_STATUS                        Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL     *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT       SwContext;
  EFI_GLOBAL_NVS_AREA_PROTOCOL      *GlobalNvsArea;
  EFI_HANDLE                        SwHandle;

  //
  // Locate SmmVariableProtocol
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, &mSmmVariable);
  ASSERT_EFI_ERROR (Status);

  //
  // Locate SmmVariableProtocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  &GlobalNvsArea
                  );
  ASSERT_EFI_ERROR (Status);

  mNvsArea = (UINT8 *)GlobalNvsArea->Area;
  mNvsAreaMap = GlobalNvsArea->NvsMap;

  //
  //  Get the Sw dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (
                  &gEfiSmmSwDispatch2ProtocolGuid,
                  NULL,
                  &SwDispatch
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register MOR set handler
  //
  SwContext.SwSmiInputValue = TCG_MOR;
  Status = SwDispatch->Register (
                         SwDispatch,
                         SmiMorCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}


