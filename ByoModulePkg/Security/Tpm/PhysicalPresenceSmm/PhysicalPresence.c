/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PhysicalPresence.c

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

Copyright (c) 2006 - 2009 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

++*/

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/GlobalNvsArea.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Guid/PhysicalPresenceData.h>

#define TCG_PHY_PRESENCE          0x6d

EFI_SMM_VARIABLE_PROTOCOL         *mSmmVariable;
UINT8                             *mNvsArea;
NVS_FIELD_MAP                     *mNvsAreaMap;



EFI_STATUS
SmiPpCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN CONST VOID                    *Context,
  IN OUT VOID                       *CommBuffer,
  IN OUT UINTN                      *CommBufferSize          
  )
/*++

Routine Description:

  When an smi event for physical presence happen.

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS            Status;
  UINTN                 DataSize;
  EFI_PHYSICAL_PRESENCE PpData;
  UINT8                 Revision;
  UINT8                 TcgParamter;
  UINT8                 PPRequest;  
  
  Revision = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_REVISION].Offset];
  TcgParamter = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_PARAMTER].Offset];
  PPRequest = mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_PP_REQUEST].Offset];
  
  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
  Status = mSmmVariable->SmmGetVariable (
             PHYSICAL_PRESENCE_VARIABLE,
             &gEfiPhysicalPresenceGuid,
             NULL,
             &DataSize,
             &PpData
             );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }
  
  if (Revision >= GLOBAL_NVS_AREA_RIVISION_1) {
    if (TcgParamter == 5) {
      //
      // Return TPM Operation Response to OS Environment
      //
      mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_PP_LAST_REQUEST].Offset] = PpData.LastPPRequest;
      CopyMem (&mNvsArea[mNvsAreaMap[GLOBAL_NVS_AREA_TCG_PP_RESPONSE].Offset],
               &PpData.PPResponse,
               sizeof (PpData.PPResponse)
               );
    } else if (TcgParamter == 2) {
      //
      // Submit TPM Operation Request to Pre-OS Environment
      //
      if (PpData.PPRequest != PPRequest) {
        PpData.PPRequest = PPRequest;
        DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
        Status = mSmmVariable->SmmSetVariable (
                   PHYSICAL_PRESENCE_VARIABLE,
                   &gEfiPhysicalPresenceGuid,
                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                   DataSize,
                   &PpData
                   );
      }
    }
  }
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializePhysicalPresenceSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
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

  SwContext.SwSmiInputValue = TCG_PHY_PRESENCE;
  Status = SwDispatch->Register (
                         SwDispatch,
                         SmiPpCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
