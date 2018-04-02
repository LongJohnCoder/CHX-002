/*++

Copyright (c) 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  TimerLibTsc.c
  
Abstract: 

  Timer Library based on TSC timer.

--*/

#include "EdkIIGlueBase.h"

/**
  Converts TSC value into nano second value.

  @param  Timestamp  TSC ticker

  @return UINT64 value in naosecond unit converted from TSC

**/
UINT64
EFIAPI
GetTimeInNanoSec (
  UINT64 Timestamp
  )
{
  UINT64  pi;
  UINT8   Ratio;

  pi    = AsmReadMsr64 (PcdGet16(PlatformInfoMsr));
  Ratio = (UINT8) (((UINT32) (UINTN) RShiftU64 (pi, 8)) & 0xff);

  return (UINT64) DivU64x32 (MultU64x32 (Timestamp, 10), (UINT32) (Ratio));
}
