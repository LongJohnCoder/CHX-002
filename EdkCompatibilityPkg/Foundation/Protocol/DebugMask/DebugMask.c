/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    DebugMask.c

Abstract:

  This protocol is used to abstract the Debug Mask serivces for 
  the specific driver or application image.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION(DebugMask)
 

EFI_GUID gEfiDebugMaskProtocolGuid = EFI_DEBUG_MASK_PROTOCOL_GUID;
EFI_GUID gEfiSmmDebugMaskProtocolGuid = EFI_SMM_DEBUG_MASK_PROTOCOL_GUID;
EFI_GUID gEfiRuntimeDebugMaskProtocolGuid = EFI_RUNTIME_DEBUG_MASK_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDebugMaskProtocolGuid, "DebugMask Protocol", "Efi Debug Mask Protocol");
EFI_GUID_STRING(&gEfiSmmDebugMaskProtocolGuid, "Smm DebugMask Protocol", "Efi Smm Debug Mask Protocol");
EFI_GUID_STRING(&gEfiRuntimeDebugMaskProtocolGuid, "Runtime DebugMask Protocol", "Efi Runtime Debug Mask Protocol");