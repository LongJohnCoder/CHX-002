/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  ByoHookStatusCodeSmmLib.c

Abstract: 
  Oem Hook Status Code SMM library instance.

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
	
#include <ByoHookStatusCodeSmmLib.h>


/**
  Initialize OEM status code device .

  @retval  EFI_SUCCESS   Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeInitialize (
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
  return EFI_SUCCESS;
}

/**
  Report status code to OEM device.
 
  @param  CodeType      Indicates the type of status code being reported.
  @param  Value         Describes the current status of a hardware or software entity.  
                        This included information about the class and subclass that is used to classify the entity 
                        as well as an operation.  For progress codes, the operation is the current activity. 
                        For error codes, it is the exception.  For debug codes, it is not defined at this time. 
  @param  Instance      The enumeration of a hardware or software entity within the system.  
                        A system may contain multiple entities that match a class/subclass pairing. 
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable, 
                        not meaningful, or not relevant.  Valid instance numbers start with 1.
  @param  CallerId      This optional parameter may be used to identify the caller. 
                        This parameter allows the status code driver to apply different rules to different callers. 
  @param  Data          This optional parameter may be used to pass additional data
 
  @retval EFI_SUCCESS   Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OemHookStatusCodeReport (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId, OPTIONAL
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  )
{
  EFI_STATUS                Status;

  //
  // Dispatch initialization request to supported devices
  //
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    Status = SerialStatusCodeReportWorker ( 
			         CodeType,
			         Value,
			         Instance,
			         CallerId,
			         Data
		           );
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeReportWorker ( 
			         CodeType,
			         Value,
			         Instance,
			         CallerId,
			         Data
		           );
  }
  if (FeaturePcdGet (PcdStatusCodeUsePostCode)) {
    Status = Port80StatusCodeReportWorker ( 
			         CodeType,
			         Value,
			         Instance,
			         CallerId,
			         Data
		           );
  }
  
  if (FeaturePcdGet (PcdStatusCodeUseBeep)) {
    Status = BeepStatusCodeReportWorker( 
			         CodeType,
			         Value,
			         Instance,
			         CallerId,
			         Data
		           );
  }
  if (FeaturePcdGet (PcdStatusCodeUseOem)) {
    Status = OemStatusCodeReportWorker( 
			         CodeType,
			         Value,
			         Instance,
			         CallerId,
			         Data
		           );
  }

  return EFI_SUCCESS;
}

