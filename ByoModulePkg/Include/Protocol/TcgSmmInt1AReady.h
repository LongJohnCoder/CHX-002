/*++
Copyright (c) 2011 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.
--*/
#ifndef _TCG_SMM_INT1A_READY_H_
#define _TCG_SMM_INT1A_READY_H_

#define TCG_SMM_INT1A_READY_PROTOCOL_GUID \
  { \
    0x41e8252a, 0x9859, 0x4584, { 0xa0, 0x83, 0x2b, 0x6, 0x3, 0x3b, 0x5, 0x5c } \
  };

typedef struct {
  UINT8  SwSmiInputValue;
} TCG_SMM_INT1A_READY_PROTOCOL;

extern EFI_GUID gTcgSmmInt1AReadyProtocolGuid;
#endif
