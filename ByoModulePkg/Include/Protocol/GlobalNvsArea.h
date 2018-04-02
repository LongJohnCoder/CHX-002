/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  GlobalNvsArea.h

Abstract:
  Definition of the global NVS area protocol.

Revision History:

Bug 2270:   Needs to move R8SnbClientPkg\Platform\SmmPlatform
            to SnbClientX64Pkg
TIME:       2011-06-20
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     Sugar Bay Customer Refernce Board.
$TECHNICAL: 
  1. Revise the coding style following latest standard.
  2. Use EDKII libraries instead of EDK libraries.
  3. Move KscLibPei, KscLibDxe, KscLibSmm, SmmAcpiCustomlib
     and InternalGraphicDeviceLib to SnbClientX64Pkg. 
     Change the name of these libraries to KscLib instance, 
     AcpiCustomSmmlib and InternalGraphicDeviceSmmLib.
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

Copyright (c)  2010 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  GlobalNvsArea.h

Abstract:

  Definition of the global NVS area protocol.  This protocol 
  publishes the address and format of a global ACPI NVS buffer used as a communications
  buffer between SMM code and ASL code.
  The format is derived from the ACPI reference code, version 0.95.

  Note:  Data structures defined in this protocol are not naturally aligned.

--*/

#ifndef _GLOBAL_NVS_AREA_H_
#define _GLOBAL_NVS_AREA_H_

//
// Includes
//
#define GLOBAL_NVS_DEVICE_ENABLE  1
#define GLOBAL_NVS_DEVICE_DISABLE 0

//
// Forward reference for pure ANSI compatability
//

typedef struct _EFI_GLOBAL_NVS_AREA_PROTOCOL EFI_GLOBAL_NVS_AREA_PROTOCOL;

//
// Global NVS Area Protocol GUID
//
#define EFI_GLOBAL_NVS_AREA_PROTOCOL_GUID \
{ 0x74e1e48, 0x8132, 0x47a1, {0x8c, 0x2c, 0x3f, 0x14, 0xad, 0x9a, 0x66, 0xdc} }
//
// Revision id - Added TPM related fields
//
#define GLOBAL_NVS_AREA_RIVISION_1       1
//
// Extern the GUID for protocol users.
//

#define MAX_NVS_FIELDS_NUM             100

typedef enum {
  GLOBAL_NVS_AREA_REVISION     = 0,
  GLOBAL_NVS_AREA_TCG_PARAMTER = 1,
  GLOBAL_NVS_AREA_TCG_MOR_DATA = 2,
  GLOBAL_NVS_AREA_TCG_PP_RESPONSE = 3,
  GLOBAL_NVS_AREA_TCG_PP_REQUEST  = 4,
  GLOBAL_NVS_AREA_TCG_PP_LAST_REQUEST = 5,  
} GLOBAL_NVS_AREA_FIELD_INDEX;

typedef struct _NVS_FIELD_MAP {
  UINT32 Offset;
  UINT8  Length;
} NVS_FIELD_MAP;

//
// Global NVS Area definition
//
#pragma pack (1)
typedef struct {
  UINT8 DataBuffer[425];
} EFI_GLOBAL_NVS_AREA;
#pragma pack ()

//
// Global NVS Area Protocol
//
struct _EFI_GLOBAL_NVS_AREA_PROTOCOL {
  EFI_GLOBAL_NVS_AREA     *Area;
  NVS_FIELD_MAP           *NvsMap;
};


//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiGlobalNvsAreaProtocolGuid;

#endif
