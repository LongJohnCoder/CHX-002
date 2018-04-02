/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmSmmInstallInt1A.c

Abstract: 
  int 1A interrupt support.

Revision History:

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

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesLib.h>
#include <Protocol/LegacyInterruptHandler.h>
#include <Protocol/TcmSmmInt1AReady.h>



// 3E6AB2DB-449B-48c2-A6B4-E91CAE54B042
EFI_GUID gInt1ATcmFile = \
  {0x3E6AB2DB, 0x449B, 0x48c2, {0xA6, 0xB4, 0xE9, 0x1C, 0xAE, 0x54, 0xB0, 0x42}};

EFI_LEGACY_INTERRUPT_HANDLER_PROTOCOL gInterruptHandler = {
  0x1A,
  NULL,
  0
};



EFI_STATUS
EFIAPI
TcmSmmInstallInt1A (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    Handle;
  TCM_SMM_INT1A_READY_PROTOCOL  *Int1AReady;

  Status = gBS->LocateProtocol(&gTcmSmmInt1AReadyProtocolGuid, NULL, (VOID**)&Int1AReady);
  ASSERT_EFI_ERROR (Status);

  Status = GetSectionFromFv(
             &gInt1ATcmFile, 
             EFI_SECTION_RAW, 
             0, 
             &gInterruptHandler.Handler, 
             &gInterruptHandler.Length
             );
  ASSERT_EFI_ERROR(Status);
  ASSERT(*(UINT32 *)gInterruptHandler.Handler == SIGNATURE_32('$', 'F', 'I', 'X'));
  ASSERT(((UINT8 *)gInterruptHandler.Handler)[gInterruptHandler.Length-1] == '$');
  
  ((UINT8 *) gInterruptHandler.Handler)[gInterruptHandler.Length-1] = Int1AReady->SwSmiInputValue;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiLegacyInterruptHandlerProtocolGuid,  
                  &gInterruptHandler,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

