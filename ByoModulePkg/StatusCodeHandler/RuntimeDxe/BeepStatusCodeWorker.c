/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  BeepStatusCodeWorker.c

Abstract: 
  Beep status code implementation.

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
  Beep status code implementation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include "StatusCodeHandlerRuntimeDxe.h"
#include "BeepMapTable.h"

#define NOTE(x) ((119318200 + (x) / 2) / (x))

STATUS_CODE_TO_DATA_MAP* mBeepStatusCodes[] = 
{
    //#define EFI_PROGRESS_CODE 0x00000001
    mProgressBeepMap,
    //#define EFI_ERROR_CODE 0x00000002
    mErrorBeepMap
    //#define EFI_DEBUG_CODE 0x00000003
};

/**
  Switch on Beep.

  @param  Note         The note of beep.
  @param  Octave       The octave of beep.

  @retval None

**/
VOID
BeepOn (
  UINT8 Note,
  UINT8 Octave
  )
{
  UINT16  Frequency;

  //
  // beep tones
  //
  UINT16  tones[8] = {
    NOTE (26163),
    NOTE (29366),
    NOTE (32963),
    NOTE (34923),
    NOTE (39200),
    NOTE (44000),
    NOTE (49388),
    NOTE (26163 * 2)
  };

  Frequency = tones[(Note % 8)];

  if (Octave - 1 >= 0) {
    Frequency >>= Octave - 1;
  } else {
    Frequency <<= 1 - Octave;
  }
  //
  // set up channel 1 (used for delays)
  //
  IoWrite8 (0x43, 0x54);
  IoWrite8 (0x41, 0x12);
  //
  // set up channel 2 (used by speaker)
  //
  IoWrite8 (0x43, 0xb6);
  IoWrite8 (0x42, (UINT8) Frequency);
  IoWrite8 (0x42, (UINT8) (Frequency >> 8));
  //
  // turn the speaker on
  //
  IoWrite8 (0x61, IoRead8 (0x61) | 3);
}

/**
  Switch off Beep.

  @retval None

**/
VOID
BeepOff (
  VOID
  )
{
  IoWrite8 (0x61, IoRead8 (0x61) & 0xfc);
}

/**
  Produces Beep.

  @param  Note         The note of beep.
  @param  Octave       The octave of beep.
  @param  Duration     The duration of beep.

  @retval None

**/
VOID
Beep (
  UINT8            Note,
  UINT8            Octave,
  UINT32           Duration
  )
{
  BeepOn (Note, Octave);
  MicroSecondDelay (Duration);
  BeepOff ();
}

UINT32
FindBeepByteCode (
  STATUS_CODE_TO_DATA_MAP *Map, 
  EFI_STATUS_CODE_VALUE   Value
  )
{
  while ( Map->Value != 0 ) {
    if ( Map->Value == Value ) {
      return Map->Data;
    }
    Map++;
  }
  return 0;
}

/**
  Convert status code value to the times of beep.

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

  @retval EFI_SUCCESS      Status code reported to beep successfully.

**/
EFI_STATUS
EFIAPI
BeepStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  UINT32  CodeTypeIndex;
  UINT8   Index;
  UINT32  BeepTimes;

  CodeTypeIndex = STATUS_CODE_TYPE (CodeType) - 1;

  if (CodeTypeIndex >= sizeof (mBeepStatusCodes) / sizeof (STATUS_CODE_TO_DATA_MAP *)) {
    return EFI_SUCCESS;
  }

  BeepTimes = FindBeepByteCode (mBeepStatusCodes[CodeTypeIndex], Value);
  if (BeepTimes > 0) {
    for (Index = 0; Index < BeepTimes; Index++) {
      Beep (1, 2, 400000);
      MicroSecondDelay (100000);
    }
  }

  return EFI_SUCCESS;
}

