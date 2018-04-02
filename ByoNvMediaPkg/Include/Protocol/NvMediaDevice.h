/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  NvMediaDevice.h

Abstract: 

Revision History:

**/

#ifndef __NV_MEDIA_DEVICE_H__
#define __NV_MEDIA_DEVICE_H__

#define NV_MEDIA_DEVICE_PROTOCOL_GUID  \
  { 0xa156dcc8, 0x1873, 0x45a9, { 0x93, 0x3, 0x15, 0x86, 0x28, 0x99, 0x7a, 0xf3}}

#define SMM_NV_MEDIA_DEVICE_PROTOCOL_GUID  \
  { 0xa235c112, 0x5b90, 0x40ec, { 0x88, 0x20, 0xd3, 0x27, 0xe5, 0xb1, 0xc1, 0x12}}

//
// Forward reference for pure ANSI compatability
//
typedef struct _NV_MEDIA_DEVICE_PROTOCOL  NV_MEDIA_DEVICE_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI * DEVICE_INFO) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this,
    IN OUT MEDIA_BLOCK_MAP**            MapInfo
);

typedef
EFI_STATUS
(EFIAPI * DEVICE_SENSE) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this
);

typedef
EFI_STATUS
(EFIAPI * DEVICE_READ) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this,
    IN UINTN                            Address,
    IN OUT UINT8*                       Buffer,
    IN OUT UINTN*                       Length
);

typedef
EFI_STATUS
(EFIAPI * DEVICE_WRITE) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINT8*                           Buffer,
    IN UINTN                            Length
);

typedef
EFI_STATUS
(EFIAPI * DEVICE_ERASE) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length
);

typedef
EFI_STATUS
(EFIAPI * DEVICE_LOCK) (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length
);

struct _NV_MEDIA_DEVICE_PROTOCOL {
    DEVICE_INFO       Info;
    DEVICE_SENSE      Sense;
    DEVICE_READ       Read;
    DEVICE_WRITE      Write;
    DEVICE_ERASE      Erase;
    DEVICE_LOCK       Lock;
};

extern EFI_GUID gEfiNvMediaDeviceProtocolGuid;
extern EFI_GUID gEfiSmmNvMediaDeviceProtocolGuid;

#endif  // __NV_MEDIA_DEVICE_H__
