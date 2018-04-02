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
  ppi header for tcm hash sm3.

Revision History:

Bug 3223 - package ZTE SM3 hash source to .efi for ZTE's copyrights.
TIME: 2011-12-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use ppi or protocol to let hash be independent.
$END------------------------------------------------------------

**/



#ifndef __TCM_HASH_SM3_PEI_H__
#define __TCM_HASH_SM3_PEI_H__
//-------------------------------------------------------------

#define PEI_TCM_HASH_TM3_SERVICE_PPI_GUID \
  {0x99ff6eec, 0x8f28, 0x4cdb, {0xb0, 0x2, 0x73, 0xb2, 0xf4, 0x1a, 0x4, 0x4b}}



typedef
EFI_STATUS
(EFIAPI * PEI_TCM_HASH_SM3)(
  IN CONST UINT8 *Data, 
  IN UINTN       DataLength, 
  OUT UINT8      *Hash
    );

typedef struct {
    PEI_TCM_HASH_SM3  HashSm3;
} PEI_TCM_HASH_TM3_SERVICE_PPI;


extern EFI_GUID gPeiTcmHashSm3PpiGuid;

//-------------------------------------------------------------
#endif
