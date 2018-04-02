/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TopSwap.h

Abstract: 

Revision History:

**/

#ifndef __TOP_SWAP_H__
#define __TOP_SWAP_H__

//
// Forward reference for pure ANSI compatability
//
typedef struct _FIRMWARE_READ_TOP_SWAP_PROTOCOL FIRMWARE_READ_TOP_SWAP_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI * FIRMWARE_READ_TOP_SWAP_GET_SIZE) (
  IN FIRMWARE_READ_TOP_SWAP_PROTOCOL*       This,
  OUT UINTN*                                Size
  );

typedef
EFI_STATUS
(EFIAPI * FIRMWARE_READ_TOP_SWAP_GET_SWAP) (
  IN FIRMWARE_READ_TOP_SWAP_PROTOCOL*       This,
  OUT BOOLEAN*                              Swapped
  );

typedef
EFI_STATUS
(EFIAPI * FIRMWARE_READ_TOP_SWAP_SET_SWAP) (
  IN FIRMWARE_READ_TOP_SWAP_PROTOCOL*       This,
  IN BOOLEAN                                Swap
  );

typedef
struct _FIRMWARE_READ_TOP_SWAP_PROTOCOL {
  FIRMWARE_READ_TOP_SWAP_GET_SIZE     GetSize;
  FIRMWARE_READ_TOP_SWAP_GET_SWAP     GetSwap;
  FIRMWARE_READ_TOP_SWAP_SET_SWAP     SetSwap;
};

typedef struct {
  UINT32                          Signature;
  FIRMWARE_READ_TOP_SWAP_PROTOCOL This;
} FW_TOP_SWAP_PRIVATE;

#define FW_TOP_SWAP_PRIVATE_SIGNATURE SIGNATURE_32('f','r','t','s')
#define FW_TOP_SWAP_PRIVATE_FROM_THIS(a) \
          CR(a, FW_TOP_SWAP_PRIVATE, This, FW_TOP_SWAP_PRIVATE_SIGNATURE)

extern EFI_GUID gFirmwareReadTopSwapProtocolGuid;

#endif  // __TOP_SWAP_H__
