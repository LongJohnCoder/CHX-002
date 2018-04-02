/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmHashSm3.h

Abstract: 
  protocol header for tcm hash sm3.

Revision History:

Bug 3269 - Add TCM int1A function support. 
TIME: 2011-12-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  Use Smi to handle legacy int 1A(0xBB) interrupt.
$END------------------------------------------------------------


Bug 3223 - package ZTE SM3 hash source to .efi for ZTE's copyrights.
TIME: 2011-12-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use ppi or protocol to let hash be independent.
$END------------------------------------------------------------

**/


#ifndef __TCM_SMM_HASH_SM3_PROTOCOL_H__
#define __TCM_SMM_HASH_SM3_PROTOCOL_H__
//-------------------------------------------------------------

#define TCM_SMM_HASH_SM3_PROTOCOL_GUID \
  {0x39dfe95, 0xd6b3, 0x4aa5, {0x82, 0x2f, 0xa3, 0x7a, 0x96, 0x30, 0x0, 0x89}}


typedef
EFI_STATUS
(EFIAPI *TCM_SMM_HASH_SM3)(
  IN CONST UINT8 *Data, 
  IN UINTN       DataLength, 
  OUT UINT8      *Hash
    );

typedef struct {
    TCM_SMM_HASH_SM3  HashSm3;
} TCM_SMM_HASH_SM3_PROTOCOL;


extern EFI_GUID gTcmSmmHashSm3ProtocolGuid;

//-------------------------------------------------------------
#endif
