/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugMask.h

Abstract:

  This protocol is used to abstract the Debug Mask serivces for 
  the specific driver or application image.

--*/

#ifndef _DEBUG_MASK_H_
#define _DEBUG_MASK_H_

#define EFI_DEBUG_MASK_PROTOCOL_GUID \
  { 0x4c8a2451, 0xc207, 0x405b, 0x96, 0x94, 0x99, 0xea, 0x13, 0x25, 0x13, 0x41 }

#define EFI_SMM_DEBUG_MASK_PROTOCOL_GUID \
  { 0x3bce1d9f, 0x3b8d, 0x4117, 0x9f, 0x7, 0x5c, 0x4f, 0x79, 0x3e, 0xb0, 0xa4 }

#define EFI_RUNTIME_DEBUG_MASK_PROTOCOL_GUID \
  { 0x6378fef3, 0xc89f, 0x492f, 0xbd, 0xb1, 0xbf, 0xb4, 0x43, 0x19, 0xcf, 0xda }

#define EFI_DEBUG_MASK_REVISION        0x00010000

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_DEBUG_MASK_PROTOCOL);

//
// DebugMask member functions definition
//
typedef
EFI_STATUS
(EFIAPI * EFI_GET_DEBUG_MASK) (
  IN EFI_DEBUG_MASK_PROTOCOL      *This,             // Calling context
  IN OUT UINTN                    *CurrentDebugMask  // Ptr to store current debug mask
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_SET_DEBUG_MASK) (
  IN  EFI_DEBUG_MASK_PROTOCOL     *This,             // Calling context
  IN  UINTN                       NewDebugMask       // New Debug Mask value to set
  );

//
// DebugMask protocol definition
//
typedef struct _EFI_DEBUG_MASK_PROTOCOL {
  INT64                               Revision;
  EFI_GET_DEBUG_MASK                  GetDebugMask;
  EFI_SET_DEBUG_MASK                  SetDebugMask;
} EFI_DEBUG_MASK_PROTOCOL;

extern EFI_GUID gEfiDebugMaskProtocolGuid;
extern EFI_GUID gEfiSmmDebugMaskProtocolGuid;
extern EFI_GUID gEfiRuntimeDebugMaskProtocolGuid;

#endif
