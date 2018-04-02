/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  TcgMor.h

Abstract:


Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

**/
/*++
  This file contains a 'Sample Driver' and is licensed as such  
  under the terms of your license agreement with Intel or your  
  vendor.  This file may be modified by the user, subject to    
  the additional terms of the license agreement                 
--*/
/*++

Copyright (c) 2007 - 2009 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcgMor.h

Abstract:

  Interface Definition used by TcgMor modules

--*/

#ifndef _MOR_DRIVER_H_
#define _MOR_DRIVER_H_

#include <PiDxe.h>


#define TCG_MOR   0x6c
//
// Entry Point prototype
//
EFI_STATUS
EFIAPI
MorDriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  );

#endif // _MOR_DRIVER_H_
