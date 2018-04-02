/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  PlatformAccess.h

Abstract: 
  This platform access protocol defines the basic functions to
  enable/disable platform.

Revision History:

**/

#ifndef __PLATFORM_ACCESS_H__
#define __PLATFORM_ACCESS_H__

#define PLATFORM_ACCESS_PROTOCOL_GUID \
  { 0x409af98, 0x6f4d, 0x49c4, { 0xa9, 0xc7, 0x9d, 0xc4, 0xf1, 0x95, 0x8b, 0xdb}}

#define SMM_PLATFORM_ACCESS_PROTOCOL_GUID \
  { 0x7a64d94a, 0x17ce, 0x4bfd, { 0xb6, 0xfe, 0x71, 0x81, 0xd3, 0x41, 0x64, 0xb6}}

//
// Forward reference for pure ANSI compatability
//
typedef struct _PLATFORM_ACCESS_PROTOCOL  PLATFORM_ACCESS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI * PLATFORM_ENABLE) (
    IN CONST PLATFORM_ACCESS_PROTOCOL* this
);

typedef
EFI_STATUS
(EFIAPI * PLATFORM_DISABLE) (
    IN CONST PLATFORM_ACCESS_PROTOCOL* this
);

typedef struct _PLATFORM_ACCESS_PROTOCOL {
    PLATFORM_ENABLE   Enable;
    PLATFORM_DISABLE  Disable;
};

extern EFI_GUID gEfiPlatformAccessProtocolGuid;
extern EFI_GUID gEfiSmmPlatformAccessProtocolGuid;

#endif  // __PLATFORM_ACCESS_H__
