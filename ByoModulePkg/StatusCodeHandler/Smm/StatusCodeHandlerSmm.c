/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  StatusCodeHandlerSmm.c

Abstract: 
  Report Status Code Handler PEIM which produces general handlers and hook them
  onto the SMM status code router.

Revision History:

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/
/** @file
  Status Code Handler Driver which produces general handlers and hook them
  onto the SMM status code router.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "StatusCodeHandlerSmm.h"
#include "Library/BaseLib.h"
EFI_SMM_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol       = NULL;


/**
  Unregister status code callback functions only available at boot time from
  report status code router when exiting boot services.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
EFI_STATUS
EFIAPI
StatusCodeHadlerSmmCallback (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  
  EFI_STATUS      Status;
  STATIC BOOLEAN  HasDone = FALSE;     

  if(HasDone){
    goto ProcExit;
  }
  
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmRscHandlerProtocolGuid,
                    NULL,
                    (VOID**)&mRscHandlerProtocol
                    );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  
  HasDone = TRUE;
                    
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    mRscHandlerProtocol->Register (SerialStatusCodeReportWorker);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    mRscHandlerProtocol->Register (MemoryStatusCodeReportWorker);
  }
  if (FeaturePcdGet (PcdStatusCodeUsePostCode)) {
    mRscHandlerProtocol->Register (Port80StatusCodeReportWorker);
  }
  if (FeaturePcdGet (PcdStatusCodeUseBeep)) {
    mRscHandlerProtocol->Register (BeepStatusCodeReportWorker);
  }
  if (FeaturePcdGet (PcdStatusCodeUseOem)) {
    mRscHandlerProtocol->Register (OemStatusCodeReportWorker);
  }
	
ProcExit:  
  return EFI_SUCCESS;
}

/**
  Dispatch initialization request to sub status code devices based on 
  customized feature flags.
 
**/
VOID
InitializationDispatcherWorker (
  VOID
  )
{
  EFI_STATUS                        Status;

  //
  // If enable UseSerial, then initialize serial port.
  // if enable UseRuntimeMemory, then initialize runtime memory status code worker.
  //
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    //
    // Call Serial Port Lib API to initialize serial port.
    //
    Status = SerialPortInitialize ();
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUsePostCode)) {
    Status = Port80StatusCodeInitialize ();
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseOem)) {
		Status = OemStatusCodeInitialize ();
    ASSERT_EFI_ERROR (Status);
  }

}

/**
  Entry point of SMM Status Code Driver.

  This function is the entry point of SMM Status Code Driver.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerSmmEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                Status;
  VOID                      *Registeration;

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmRscHandlerProtocolGuid,
                    StatusCodeHadlerSmmCallback,
                    &Registeration
                    );
  ASSERT_EFI_ERROR (Status);
  StatusCodeHadlerSmmCallback(NULL, NULL, NULL);    // fire it immediately as the protocol maybe already has been installed.
  
  return EFI_SUCCESS;
}
