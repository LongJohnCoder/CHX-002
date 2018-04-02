/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmPlatForm.h

Abstract: 
  Tcm platfrom define.

Revision History:

Bug 3269 - Add TCM int1A function support. 
TIME: 2011-12-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  Use Smi to handle legacy int 1A(0xBB) interrupt.
$END------------------------------------------------------------


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

Bug 3075 - Add TCM support.
TIME: 2011-11-14
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Tcm module init version.
     Only support setup function.
$END------------------------------------------------------------

**/


/** @file
  TCG EFI Platform Definition in TCG_EFI_Platform_1_20_Final

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCM_PLATFORM_H__
#define __TCM_PLATFORM_H__


#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include "Tcm12.h"




//
// Standard event types
//
#define EV_POST_CODE                ((TCM_EVENTTYPE) 0x00000001)
#define EV_SEPARATOR                ((TCM_EVENTTYPE) 0x00000004)
#define EV_S_CRTM_CONTENTS          ((TCM_EVENTTYPE) 0x00000007)
#define EV_S_CRTM_VERSION           ((TCM_EVENTTYPE) 0x00000008)

//
// EFI specific event types
//
#define EV_EFI_EVENT_BASE                   ((TCM_EVENTTYPE)0x80000000)
#define EV_EFI_VARIABLE_DRIVER_CONFIG       (EV_EFI_EVENT_BASE + 1)
#define EV_EFI_VARIABLE_BOOT                (EV_EFI_EVENT_BASE + 2)
#define EV_EFI_BOOT_SERVICES_APPLICATION    (EV_EFI_EVENT_BASE + 3)
#define EV_EFI_BOOT_SERVICES_DRIVER         (EV_EFI_EVENT_BASE + 4)
#define EV_EFI_RUNTIME_SERVICES_DRIVER      (EV_EFI_EVENT_BASE + 5)
#define EV_EFI_GPT_EVENT                    (EV_EFI_EVENT_BASE + 6)
#define EV_EFI_ACTION                       (EV_EFI_EVENT_BASE + 7)
#define EV_EFI_PLATFORM_FIRMWARE_BLOB       (EV_EFI_EVENT_BASE + 8)
#define EV_EFI_HANDOFF_TABLES               (EV_EFI_EVENT_BASE + 9)


#define EFI_CALLING_EFI_APPLICATION         \
  "Calling EFI Application from Boot Option"
#define EFI_RETURNING_FROM_EFI_APPLICATOIN  \
  "Returning from EFI Application from Boot Option"
#define EFI_EXIT_BOOT_SERVICES_INVOCATION   \
  "Exit Boot Services Invocation"
#define EFI_EXIT_BOOT_SERVICES_FAILED       \
  "Exit Boot Services Returned with Failure"
#define EFI_EXIT_BOOT_SERVICES_SUCCEEDED    \
  "Exit Boot Services Returned with Success"

//
// Set structure alignment to 1-byte
//
#pragma pack (1)

typedef UINT32                     TCM_EVENTTYPE;


typedef struct {
  TCM_PCRINDEX                      PCRIndex;  ///< PCRIndex event extended to
  TCM_EVENTTYPE                     EventType; ///< TCG EFI event type
  TCM_DIGEST                        Digest;    ///< Value extended into PCRIndex
  UINT32                            EventSize; ///< Size of the event data
  UINT8                             Event[1];  ///< The event data
} TCM_PCR_EVENT;

#define TSS_EVENT_DATA_MAX_SIZE   256


typedef struct {
  TCM_PCRINDEX                      PCRIndex;
  TCM_EVENTTYPE                     EventType;
  TCM_DIGEST                        Digest;
  UINT32                            EventSize;
} TCM_PCR_EVENT_HDR;


typedef struct {
  EFI_PHYSICAL_ADDRESS              BlobBase;
  UINT64                            BlobLength;
} EFI_PLATFORM_FIRMWARE_BLOB;


typedef struct {
  UINTN                             NumberOfTables;
  EFI_CONFIGURATION_TABLE           TableEntry[1];
} EFI_HANDOFF_TABLE_POINTERS;


typedef struct {
  EFI_GUID                          VariableName;
  UINTN                             UnicodeNameLength;
  UINTN                             VariableDataLength;
  CHAR16                            UnicodeName[1];
  INT8                              VariableData[1];
} EFI_VARIABLE_DATA;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER       Header;
  UINT16                            PlatformClass;
  UINT32                            Laml;             // Log Area Minimum Length (LAML), 64KB
  EFI_PHYSICAL_ADDRESS              Lasa;             // Log Area Start Address (LASA)
} EFI_TCM_CLIENT_ACPI_TABLE;

#pragma pack ()

#endif

