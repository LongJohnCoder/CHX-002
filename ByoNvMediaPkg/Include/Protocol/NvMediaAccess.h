/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  NvMediaAccess.h

Abstract: 
  This NV Media access protocol defines six basic functions to
  access the NV storage.

Revision History:

**/

#ifndef __NV_MEDIA_ACCESS_H__
#define __NV_MEDIA_ACCESS_H__

#define NV_MEDIA_ACCESS_PROTOCOL_GUID  \
  { 0xf37f2747, 0xf30b, 0x4d7f, { 0xad, 0xd3, 0x75, 0x00, 0xaf, 0xb4, 0x5d, 0x60 }}

#define SMM_NV_MEDIA_ACCESS_PROTOCOL_GUID  \
  { 0xf18afa82, 0x5f80, 0x4a30, { 0x88, 0xb9, 0x4c, 0x23, 0x60, 0xc4, 0xc6, 0xb5 }}

typedef enum {
    CMOS_MEDIA_TYPE,
    SPI_MEDIA_TYPE,
    FWH_MEDIA_TYPE,
    LPC_MEDIA_TYPE,
    MAX_MEDIA_TYPR
} NV_MEDIA_TYPE;

typedef struct _MEDIA_BLOCK_MAP {
    UINT16 Size;
    UINT16 Count;
} MEDIA_BLOCK_MAP;

//
// Forward reference for pure ANSI compatability
//
typedef struct _NV_MEDIA_ACCESS_PROTOCOL  NV_MEDIA_ACCESS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI * MEDIA_INIT) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN VOID *                           Device,
    IN NV_MEDIA_TYPE                    Type
);

typedef
EFI_STATUS
(EFIAPI * MEDIA_INFO) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN OUT MEDIA_BLOCK_MAP**            MapInfo,
    IN NV_MEDIA_TYPE                    Type
);

typedef
EFI_STATUS
(EFIAPI * MEDIA_READ) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN OUT UINT8*                       Buffer,
    IN OUT UINTN*                       Length,
    IN NV_MEDIA_TYPE                    Type
);

typedef
EFI_STATUS
(EFIAPI * MEDIA_WRITE) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINT8*                           Buffer,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
);

typedef
EFI_STATUS
(EFIAPI * MEDIA_ERASE) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
);

typedef
EFI_STATUS
(EFIAPI * MEDIA_LOCK) (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
);

struct _NV_MEDIA_ACCESS_PROTOCOL {
    MEDIA_INIT    Init;
    MEDIA_INFO    Info;
    MEDIA_READ    Read;
    MEDIA_WRITE   Write;
    MEDIA_ERASE   Erase;
    MEDIA_LOCK    Lock;
};

extern EFI_GUID gEfiNvMediaAccessProtocolGuid;
extern EFI_GUID gEfiSmmNvMediaAccessProtocolGuid;

extern EFI_GUID gEfiNvMediaAccessSpiReadyGuid;
extern EFI_GUID gEfiSmmNvMediaAccessSpiReadyGuid;

#endif  // __NV_MEDIA_ACCESS_H__
