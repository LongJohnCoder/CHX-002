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

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesLib.h>

#include <Protocol/LegacyInterruptHandler.h>
#include <Protocol/TcgSmmInt1AReady.h>

EFI_GUID  mInt1ATcgFile = { 0xce20499f, 0x952, 0x4093, { 0xba, 0x8b, 0xce, 0x6a, 0xf3, 0x4, 0x5e, 0x55}};

EFI_LEGACY_INTERRUPT_HANDLER_PROTOCOL mInterruptHandler = {
  0x1A,
  NULL,
  0
};

EFI_STATUS
EFIAPI
TcgSmmInstallInt1A (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        Handle;
  TCG_SMM_INT1A_READY_PROTOCOL      *Int1AReady;
  UINT16                            EBdaSeg;
  UINT8                             *MbrInt1AEnPtr;


  Status = gBS->LocateProtocol (&gTcgSmmInt1AReadyProtocolGuid, NULL, (VOID **) &Int1AReady);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "Load TCG Int1A binary code ...\n"));
  Status = GetSectionFromFv (&mInt1ATcgFile, EFI_SECTION_RAW, 0, &mInterruptHandler.Handler, &mInterruptHandler.Length);
  ASSERT_EFI_ERROR (Status);
  ASSERT (*(UINT32 *) mInterruptHandler.Handler == SIGNATURE_32 ('$', 'F', 'I', 'X'));

  //
  // Update the SMI Command value
  //
  ASSERT (((UINT8 *) mInterruptHandler.Handler)[mInterruptHandler.Length-1] == '$');
  ((UINT8 *) mInterruptHandler.Handler)[mInterruptHandler.Length-1] = Int1AReady->SwSmiInputValue;


  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiLegacyInterruptHandlerProtocolGuid,  &mInterruptHandler,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  EBdaSeg = *(UINT16*)(UINTN)0x40E;
  MbrInt1AEnPtr = (UINT8*)(UINTN)(((UINT32)EBdaSeg << 4) + 0x1c5); // Offset 0x1C5
  *MbrInt1AEnPtr = 1;  
  
  return EFI_SUCCESS;
}

