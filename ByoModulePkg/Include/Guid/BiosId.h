/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  BiosId.h

Abstract:
  GUIDs used for Bios ID.

Revision History:

Bug 2263:   Needs to move R8SnbClientPkg\Platform\LegacyBiosPlatform
            to SnbClientX64Pkg
TIME:       2011-06-21
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     Sugar Bay Customer Refernce Board.
$TECHNICAL: 
  1. Change the name of LegacyBiosPlatform.inf to 
     LegacyBiosPlatformDxe.inf and Revise the coding style 
     following latest standard.
  2. Use EDKII libraries instead of EDK libraries.
  3. Add gWdtProtocolGuid to PlatformPkg.dec.
  4. Move the library BiosIdLib to ByoModulePkg.
$END--------------------------------------------------------------------

**/
/*++
 This file contains an 'Intel Peripheral Driver' and is        
 licensed for Intel CPUs and chipsets under the terms of your  
 license agreement with Intel or your vendor.  This file may   
 be modified by the user, subject to additional terms of the   
 license agreement                                             
--*/
/*++

Copyright (c)  1999 - 2003 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:
  
    BiosId.h
    
Abstract:

    GUIDs used for Bios ID.

--*/

#ifndef _BIOS_ID_H_
#define _BIOS_ID_H_


#define EFI_BIOS_ID_GUID \
{ 0xC3E36D09, 0x8294, 0x4b97, 0xA8, 0x57, 0xD5, 0x28, 0x8F, 0xE3, 0x3E, 0x28 }


extern EFI_GUID gEfiBiosIdGuid;

#endif
