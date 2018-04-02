/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmSmmInt1AReady.h

Abstract: 
  report int 1A's sw smi's value.

Revision History:

Bug 3269 - Add TCM int1A function support. 
TIME: 2011-12-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  Use Smi to handle legacy int 1A(0xBB) interrupt.
$END------------------------------------------------------------

**/


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
#ifndef __TCM_SMM_INT1A_READY_H__
#define __TCM_SMM_INT1A_READY_H__

#define TCM_SMM_INT1A_READY_PROTOCOL_GUID \
  {0xea3bec1e, 0x9199, 0x4f2d, {0x8e, 0x3f, 0x9b, 0x75, 0x2e, 0x6c, 0x9b, 0x24}}

typedef struct {
  UINT8  SwSmiInputValue;
} TCM_SMM_INT1A_READY_PROTOCOL;

extern EFI_GUID gTcmSmmInt1AReadyProtocolGuid;


#endif
