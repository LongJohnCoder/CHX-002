/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  PeiPerformanceHob.c
    
Abstract:

  The GUID of the GUIDed HOB that represents the Pei Firmware Performance Hob.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

EFI_GUID  gPeiFirmwarePerformanceGuid  = PEI_FIRMWARE_PERFORMANCE_GUID;

EFI_GUID_STRING (&gPeiFirmwarePerformanceGuid, "PEI Firmware Performance HOB",
                 "Guid for PEI Firmware Performance HOB");