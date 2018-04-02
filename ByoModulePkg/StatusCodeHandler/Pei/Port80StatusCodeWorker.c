/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Port80StatusCodeWorker.c

Abstract: 
  Port80 status code implementation.

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
/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
  Port80 status code implementation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include "StatusCodeHandlerPei.h"
#include <Port80MapTable.h>

STATUS_CODE_TO_DATA_MAP* CheckPointStatusCodes[] = 
{
    //#define EFI_PROGRESS_CODE 0x00000001
    mProgressPort80MapTable,
    //#define EFI_ERROR_CODE 0x00000002
    mErrorPort80MapTable
    //#define EFI_DEBUG_CODE 0x00000003
};

UINT32
FindByteCode (
  STATUS_CODE_TO_DATA_MAP *Map, 
  EFI_STATUS_CODE_VALUE      Value
  )
{
  while(Map->Value != 0) {
    if (Map->Value == Value) {
      return Map->Data;
    }
    Map++;
  }
  return 0;
}

UINT32
GetCheckPointCode (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value
  )
{
  UINT32 CodeTypeIndex;
  
  CodeTypeIndex = STATUS_CODE_TYPE (Type) - 1;
  
  if (CodeTypeIndex >= sizeof (CheckPointStatusCodes) / sizeof(STATUS_CODE_TO_DATA_MAP*)) {
    return 0;
  }
  
  return FindByteCode (CheckPointStatusCodes[CodeTypeIndex], Value);
}

/**
  Initialization port80 status code worker.

  @retval EFI_SUCCESS  Initialization is successfully.

**/
EFI_STATUS
Port80StatusCodeInitialize (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Convert status code value and write data to port 0x80.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or
                           software entity. This includes information about the class and
                           subclass that is used to classify the entity as well as an operation.
                           For progress codes, the operation is the current activity.
                           For error codes, it is the exception.For debug codes,it is not defined at this time.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. A system may contain multiple entities that match a class/subclass
                           pairing. The instance differentiates between them. An instance of 0 indicates
                           that instance information is unavailable, not meaningful, or not relevant.
                           Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      Status code reported to port 0x80 successfully.

**/
EFI_STATUS
EFIAPI
Port80StatusCodeReportWorker (
  IN CONST  EFI_PEI_SERVICES        **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  UINT8 Port80Code;
  
  Port80Code = (UINT8) GetCheckPointCode (CodeType, Value);
  if (Port80Code != 0) {
    DEBUG ((EFI_D_ERROR, "POSTCODE=<%02x>\n", Port80Code));
    IoWrite8 (0x80, Port80Code);
  }

  return EFI_SUCCESS;
}

