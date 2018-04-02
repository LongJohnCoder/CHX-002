/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  HddPasswordPei.c

Abstract: 
  Hdd password pei driver.

Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

Bug 2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: add a PEI drvier instead of boot script as running order is hard to control.
$END----------------------------------------------------------------------------

**/

#include <Guid/HddPasswordSecurityVariable.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Ppi/PostBootScriptTable.h>


STATIC
EFI_STATUS
HddPasswordPeiS3 (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN      VOID                      *Ppi
  )
{
    (*PeiServices)->CpuIo->IoWrite8(PeiServices, (*PeiServices)->CpuIo, 0xB2, SW_SMI_HDD_UNLOCK_PASSWORD);
    return EFI_SUCCESS;
}


STATIC EFI_PEI_NOTIFY_DESCRIPTOR   mNotifyDesc = {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPeiPostScriptTablePpiGuid,
    HddPasswordPeiS3
};



/**
  Entry point of Status Code PEIM.
  
  This function is the entry point of this Status Code Router PEIM.
  It produces Report Stataus Code Handler PPI and Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.

**/
EFI_STATUS
EFIAPI
HddPasswordPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                 Status;
  EFI_BOOT_MODE              BootMode;
  
  Status = (*PeiServices)->GetBootMode (PeiServices, &BootMode);
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }
  if (BootMode != BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyDesc);
  return Status;
}

