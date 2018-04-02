/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  StatusCodeHandlerPei.c

Abstract: 
  Report Status Code Handler PEIM which produces general handlers and hook them
  onto the PEI status code router.

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
  Report Status Code Handler PEIM which produces general handlers and hook them
  onto the PEI status code router.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "StatusCodeHandlerPei.h"

EFI_STATUS
EFIAPI
StatusCodeHandlerPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  ) ;

STATIC EFI_PEI_NOTIFY_DESCRIPTOR        mNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiRscHandlerPpiGuid,
    StatusCodeHandlerPeiCallback
  }
};

/**
  Entry point of Status Code PEIM.
  
  This function is the entry point of this Status Code PEIM.
  It initializes supported status code devices according to PCD settings,
  and installs Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  ) 
{
  EFI_STATUS                  Status;
  EFI_PEI_RSC_HANDLER_PPI     *RscHandlerPpi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiRscHandlerPpiGuid,
             0,
             NULL,
             (VOID **) &RscHandlerPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch initialization request to sub-statuscode-devices.
  // If enable UseSerial, then initialize serial port.
  // if enable UseMemory, then initialize memory status code worker.
  //
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    Status = SerialPortInitialize();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (SerialStatusCodeReportWorker);                     
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (MemoryStatusCodeReportWorker);                     
    ASSERT_EFI_ERROR (Status);
  }

  if (FeaturePcdGet (PcdStatusCodeUsePostCode)) {
    Status = Port80StatusCodeInitialize ();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (Port80StatusCodeReportWorker);
    ASSERT_EFI_ERROR (Status);
  }

  if (FeaturePcdGet (PcdStatusCodeUseBeep)) {
    Status = RscHandlerPpi->Register (BeepStatusCodeReportWorker);                     
    ASSERT_EFI_ERROR (Status);
  }

  if (FeaturePcdGet (PcdStatusCodeUseOem)) {
		Status = OemStatusCodeInitialize ();
    Status = RscHandlerPpi->Register (OemStatusCodeReportWorker);                     
    ASSERT_EFI_ERROR (Status);
  }
  return EFI_SUCCESS;
	
}

/**
  Entry point of Status Code PEIM.
  
  This function is the entry point of this Status Code PEIM.
  It initializes supported status code devices according to PCD settings,
  and installs Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;

  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

