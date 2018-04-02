/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmSmm.h

Abstract: 
  Tcm Smm support.

Revision History:

Bug 3592 - Cpu save state may write error under IA32 CPU. 
TIME: 2012-04-28
$AUTHOR: ZhangLin
$REVIEWERS:
$SCOPE: Sugar Bay.
$TECHNICAL: 
  1. R8 SMM Arch may destroy cpu save state when do write back under
     IA32 CPU. So we should update save state buffer to avoid it.
$END------------------------------------------------------------

Bug 3269 - Add TCM int1A function support. 
TIME: 2011-12-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  Use Smi to handle legacy int 1A(0xBB) interrupt.
$END------------------------------------------------------------

**/

/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
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

  TCMSmm.h

Abstract:

  TCMSmm.h

--*/
#ifndef __TCM_SMM_H__
#define __TCM_SMM_H__

#include <uefi.h>
#include <Framework/SmmCis.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Protocol/TcmService.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/TcmSmmInt1AReady.h>
#include <Protocol/TcmSmmHashSm3.h>
#include <protocol/SmmBaseHelperReady.h>
#include "../DxeCommLib/TcmDxeLib.h"



#define SMIFN_TCM                   0xB3
#define EV_COMPACT_HASH             11


typedef struct {
  TCM_EFI_BOOT_SERVICE_CAPABILITY   BsCap;
  EFI_TCM_CLIENT_ACPI_TABLE         *TcmAcpiTable;
  UINTN                             EventLogSize;
  UINT8                             *LastEvent;
  EFI_TCM_HANDLE                    TcmHandle;
} TCM_SMM_DATA;


/*
Data structure copied from TCM PC Client Specific Implementation Specification For Conventional BIOS
*/
typedef enum {
  TCM_StatusCheck,
  TCM_HashLogExtendEvent,
  TCM_PassThroughToTCM,
  TCM_ShutdownPreBootInterface,
  TCM_HashLogEvent,
  TCM_HashAll,
  TCM_TSS,
  TCM_CompactHashLogExtendEvent,
  TCM_MemoryOverwriteRequest,
  TCM_DriverInit = 0x20
};

#pragma pack(1)



typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 AlgorithmID;
} IPB_HashAll_Struc;

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 PCRIndex;
  UINT32 LogEventType;
  UINT32 LogDataPtr; // TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} IPB_HashLogEventStruc;

typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT32 EventNumber;
} OPB_HashLogEventStruc;

// Input parameter block definition for TCG Pass Thru To TCM function
typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT16 OPBLength;
  UINT16 Reserved2;
  UINT8  TCMOperandIn;
} IPB_TCMStruc;

//
// Input Parameter Block Definition for TCG_HashLogExtendEvent func
//
typedef struct {
  UINT32 Reserved2;
  UINT32 LogDataPtr; // TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} LongLogDataStruc;

typedef struct {
  UINT32 LogDataPtr;  //TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} ShortLogDataStruc;

union LogDataUnion {
  LongLogDataStruc longLog;
  ShortLogDataStruc shortLog;
};

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 PCRIndex;
  union LogDataUnion ulog;
} IPB_HashLogExtendEventStruc;

// Output parameter block definition for TCG Pass Thru To TPM function
typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT8  TCMOperandOut;
} OPB_TCMStruc;

//
// Output Parameter Block Definition for TCG_HashLogExtendEvent func
//
typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT32 EventNumber;
  UINT8  HashValue[32];
} OPB_HashLogExtendEventStruc;

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT8 MORBitValue;
} IPB_MORStruc;

#pragma pack()







typedef enum {
  TCM_OK,
  TCM_RET_BASE,
  TCM_GENERAL_ERROR = TCM_RET_BASE, // A general unidentified error occurred.
  TCM_IS_LOCKED,                    // The access cannot be granted the device is open.
  TCM_NO_RESPONSE,                  // No response from the TPM device.
  TCM_INVALID_RESPONSE,             // The response from the TPM was invalid.
  TCM_INVALID_ACCESS_REQUEST,       // The access parameters for this function are invalid.
  TCM_FIRMWARE_ERROR,               // Firmware error during start up.
  TCM_INTEGRITY_CHECK_FAILED,       // Integrity checks of TPM parameter failed.
  TCM_INVALID_DEVICE_ID,            // The device ID for the TPM is invalid.
  TCM_INVALID_VENDOR_ID,            // The vendor ID for the TPM is invalid.
  TCM_UNABLE_TO_OPEN,               // Unable to open a connection to the TPM device.
  TCM_UNABLE_TO_CLOSE,              // Unable to close a connection to the TPM device.
  TCM_RESPONSE_TIMEOUT,             // Time out for TPM response.
  TCM_INVALID_COM_REQUEST,          // The parameters for the communication access are invalid.
  TCM_INVALID_ADR_REQUEST,          // The address parameter for the access is invalid.
  TCM_WRITE_BYTE_ERROR,             // Bytes write error on the interface.
  TCM_READ_BYTE_ERROR,              // Bytes read error on the interface.
  TCM_BLOCK_WRITE_TIMEOUT,          // Blocks write error on the interface.
  TCM_CHAR_WRITE_TIMEOUT,           // Bytes write time out on the interface.
  TCM_CHAR_READ_TIMEOUT,            // Bytes read time out on the interface.
  TCM_BLOCK_READ_TIMEOUT,           // Blocks read error on the interface.
  TCM_TRANSFER_ABORT,               // Transfer abort in communication with TPM device.
  TCM_INVALID_DRV_FUNCTION,         // Function number (AL-Register) invalid for this driver.
  TCM_OUTPUT_BUFFER_TOO_SHORT,      // Output buffer for the TPM response to short.
  TCM_FATAL_COM_ERROR,              // Fatal error in TPM communication.
  TCM_INVALID_INPUT_PARA,           // Input parameter for the function invalid.
  TCM_TCM_COMMAND_ERROR,            // Error during execution of a TCM command.
  TCM_Reserved1,
  TCM_Reserved2,
  TCM_Reserved3,
  TCM_Reserved4,
  TCM_Reserved5,
  TCM_Reserved6,
  TCM_INTERFACE_SHUTDOWN,           // TPM BIOS interface has been shutdown using the TCM_ShutdownPreBootInterface.
  TCM_DRIVER_UNSUPPORTED,               // The requested function is not supported.
  TCM_DRIVER_TCM_NOT_PRESENT,           // The TPM is not installed.
  TCM_DRIVER_TCM_DEACTIVATED,           // The TPM is deactivated.
  TCM_VENDOR_BASE_RET     = 0x80    // Start point for return codes are reserved for use by TPM vendors.
} TCM_DRIVER_ERROR_CODE;


typedef enum {
  TCM_PC_OK,                        // The function returned successful.
  TCM_PC_TPMERROR,                  // TCG_PC_OK + 01h | (TPM driver error << 16)
                                    // The TCM driver returned an error.
                                    // The upper 16 bits contain the actual error code returned by the driver
  TCM_PC_LOGOVERFLOW,               // There is insufficient memory to create the log entry.
  TCM_PC_UNSUPPORTED,               // The requested function is not supported.
} TCM_PC_ERROR_CODE;


#define TCM_DRIVER_ERROR(x)         (TCM_PC_TPMERROR | ((x) << 16))
#define IFCE_OFF                    0x1        // INT 1Ah interface status flag
#define LEGACY_TCM_STATUS(Status)   gTcmLegacyStatus[(Status) & ~MAX_BIT]

//--------------------------------------------------------
#endif
