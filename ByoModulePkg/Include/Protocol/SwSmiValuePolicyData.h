/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SwSmiValuePolicyData.h

Abstract:


Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-26
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE:
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/

#ifndef _SWSMI_VALUE_POLICY_DATA_H_
#define _SWSMI_VALUE_POLICY_DATA_H_

typedef struct _EFI_SWSMI_VALUE_POLICY_PROTOCOL    EFI_SWSMI_VALUE_POLICY_PROTOCOL;

#define SWSMI_VALUE_POLICY_DATA_GUID \
  {\
    0xe443b5f7, 0x1cc1, 0x4e49, 0xa0, 0xe9, 0x42, 0x28, 0x1d, 0x61, 0x80, 0xc4\
  }

typedef struct {
   UINT8 Type;
   UINT8 Value;
} SWSMI_ENTRY;

typedef struct _EFI_SWSMI_VALUE_POLICY_PROTOCOL{
  UINT8 NumSmmEntries;
  SWSMI_ENTRY  SwSmiEntry[16];
} EFI_SWSMI_VALUE_POLICY_PROTOCOL;

extern EFI_GUID gSwSmiValuePolicyGuid;

#endif

