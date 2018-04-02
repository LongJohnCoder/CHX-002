/** @file

Copyright (c) 2006 - 2012, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmbiosSmm.h

Abstract:
  Header file for the Smm Smbios driver.

Revision History:

**/
#ifndef _SMBIOS_SMM_H_
#define _SMBIOS_SMM_H_

#include <PnpSmbios.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmCpuSaveState.h>
#include <Protocol/SmmSwDispatch2.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#define SMM_PnP_BIOS_CALL   0x47

//
// definition for PnP BIOS DMI Call Function ID
//
#define PNPBIOS_FUNCID_GET_SMBIOS_INFORMATION                  0x50
#define PNPBIOS_FUNCID_GET_SMBIOS_STRUCTURE                    0x51
#define PNPBIOS_FUNCID_SET_SMBIOS_STRUCTURE                    0x52
#define PNPBIOS_FUNCID_GET_STRUCTURE_CHANGE_INFORMATION        0x53
#define PNPBIOS_FUNCID_SMBIOS_CONTROL                          0x54
#define PNPBIOS_FUNCID_GET_GENERAL_PURPOSE_NV_INFORMATION      0x55
#define PNPBIOS_FUNCID_READ_GENERAL_PURPOSE_NV_DATA            0x56
#define PNPBIOS_FUNCID_WRITE_GENERAL_PURPOSE_NV_DATA           0x57

//
// definition for PnP BIOS DMI Call Return Code
//
#define PNP_BIOS_DMI_RETURN_SUCCESS                            0x0
#define PNP_BIOS_DMI_RETURN_UNKNOWN_FUNCTION                   0x81
#define PNP_BIOS_DMI_RETURN_FUNCTION_NOT_SUPPORTED             0x82
#define PNP_BIOS_DMI_RETURN_INVALID_HANDLE                     0x83
#define PNP_BIOS_DMI_RETURN_BAD_PARAMETER                      0x84
#define PNP_BIOS_DMI_RETURN_INVALID_SUBFUNCTION                0x85
#define PNP_BIOS_DMI_RETURN_NO_CHANGE                          0x86
#define PNP_BIOS_DMI_RETURN_ADD_STRUCTURE_FAIL                 0x87
#define PNP_BIOS_DMI_RETURN_READ_ONLY                          0x8D
#define PNP_BIOS_DMI_RETURN_LOCK_NOT_SUPPORTED                 0x90
#define PNP_BIOS_DMI_RETURN_CURRENTLY_LOCKED                   0x91
#define PNP_BIOS_DMI_RETURN_INVALID_LOCK                       0x92

EFI_STATUS
DmiPnP50_57Callback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
);

EFI_STATUS
AllocDataBuffer ();

#endif //_SMBIOS_SMM_H_


