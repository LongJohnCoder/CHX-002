/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmEventHob.h

Abstract: 
  Tcm event hob guid define.

Revision History:

Bug 3144 - Add Tcm Measurement Architecture.
TIME: 2011-11-24
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. PEI: Measure CRTM Version.
          Measure Main Bios.
  2. DXE: add 'TCPA' acpi table.
          add event log.
          Measure Handoff Tables.
          Measure All Boot Variables.
          Measure Action.
  Note: As software of SM3's hash has not been implemented, so hash
        function is invalid.
$END------------------------------------------------------------

**/

//
// This file contains 'Framework Code' and is licensed as such 
// under the terms of your license agreement with Intel or your
// vendor.  This file may not be modified, except as allowed by
// additional terms of your license agreement.                 
//
/** @file
  Defines the HOB GUID used to pass a TCG_PCR_EVENT from a TPM PEIM to 
  a TPM DXE Driver. A GUIDed HOB is generated for each measurement 
  made in the PEI Phase.
    
Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef __TCM_EVENT_HOB_H__
#define __TCM_EVENT_HOB_H__


#define EFI_TCM_EVENT_HOB_GUID \
  {0x62186492, 0x30bc, 0x421a, {0xa5, 0x71, 0x82, 0xe4, 0xeb, 0x7, 0xfb, 0x7 }}

extern EFI_GUID gTcmEventEntryHobGuid;

#endif
