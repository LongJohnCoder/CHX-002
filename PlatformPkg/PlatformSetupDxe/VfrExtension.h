/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  VfrExtension.h

Abstract:

  OEM Specific Setup Variables and Structures
--*/

#ifndef __VFR_EXTENSION___H__
#define __VFR_EXTENSION___H__

#include "SetupVariable.h"
#include <Guid/SetupPassword.h>



//
// GUID for each of formset
//
#define MAIN_FORM_SET_GUID  \
  { \
    0x985eee91, 0xbcac, 0x4238, { 0x87, 0x78, 0x57, 0xef, 0xdc, 0x93, 0xf2, 0x4e } \
  }

#define ADVANCED_FORM_SET_GUID \
  { \
    0xe14f04fa, 0x8706, 0x4353, { 0x92, 0xf2, 0x9c, 0x24, 0x24, 0x74, 0x6f, 0x9f } \
  }

#define DEVICES_FORM_SET_GUID \
  { \
    0xadfe34c8, 0x9ae1, 0x4f8f, { 0xbe, 0x13, 0xcf, 0x96, 0xa2, 0xcb, 0x2c, 0x5b } \
  }

#define POWER_FORM_SET_GUID \
  { \
    0x5b5eb989, 0x4702, 0x47c5, { 0xbb, 0xe0, 0x4, 0xb9, 0x99, 0xf6, 0x2, 0x1e } \
  }

#define BOOT_FORM_SET_GUID \
  { \
    0x8b33ffe0, 0xd71c, 0x4f82, { 0x9c, 0xeb, 0xc9, 0x70, 0x58, 0xc1, 0x3f, 0x8e } \
  }

#define SECURITY_FORM_SET_GUID \
  { \
    0x981ceaee, 0x931c, 0x4a17, { 0xb9, 0xc8, 0x66, 0xc7, 0xbc, 0xfd, 0x77, 0xe1 } \
  }

#define EXIT_FORM_SET_GUID \
  { \
    0xa43b03dc, 0xc18a, 0x41b1, { 0x91, 0xc8, 0x3f, 0xf9, 0xaa, 0xa2, 0x57, 0x13 } \
  }

#define HDD_PASSWORD_CONFIG_GUID \
  { \
    0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80 } \
  }

#define SECUREBOOT_CONFIG_FORM_SET_GUID \
  { \
    0x5daf50a5, 0xea81, 0x4de2, {0x8f, 0x9b, 0xca, 0xbd, 0xa9, 0xcf, 0x5c, 0x14} \
  }

#define EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } }



 
//
// Class/Sub-Class GUID Definiton
//
#define MAIN_FORM_SET_CLASS             0x01
#define ADVANCED_FORM_SET_CLASS         0x02
#define DEVICES_FORM_SET_CLASS          0x03
#define BOOT_FORM_SET_CLASS             0x04
#define SECURITY_FORM_SET_CLASS         0x05
#define EXIT_FORM_SET_CLASS             0x06
#define POWER_FORM_SET_CLASS            0x08

#define NON_FRONT_PAGE_SUBCLASS         0x00
#define FRONT_PAGE_SUBCLASS             0x02

#define SETUP_DATA_ID                   0x1
#define TSESETUP_DATA_ID                0x2
#define SETUP_VOLATILE_DATA_ID          0x3


#pragma pack()


#endif

