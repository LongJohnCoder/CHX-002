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


#ifndef __TCM_HASH_SM3_PROTOCOL_H__
#define __TCM_HASH_SM3_PROTOCOL_H__
//-------------------------------------------------------------

#define TCM_HASH_SM3_PROTOCOL_GUID \
  {0xfd55aff1, 0x3926, 0x4ae7, {0x9a, 0x1c, 0x88, 0x35, 0x37, 0xbf, 0xbc, 0x93}}


typedef
EFI_STATUS
(EFIAPI * DXE_TCM_HASH_SM3)(
  IN CONST UINT8 *Data, 
  IN UINTN       DataLength, 
  OUT UINT8      *Hash
    );

typedef struct {
    DXE_TCM_HASH_SM3  HashSm3;
} TCM_HASH_SM3_PROTOCOL;


extern EFI_GUID gTcmHashSm3ProtocolGuid;

//-------------------------------------------------------------
#endif
