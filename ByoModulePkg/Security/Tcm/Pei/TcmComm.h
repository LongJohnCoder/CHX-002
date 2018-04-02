/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmComm.h

Abstract: 
  Pei part of TCM Module.

Revision History:

Bug 3282 - improve TCM action stability.
TIME: 2012-01-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. update command flow as porting guide request.
  2. use retry mechanism.
$END------------------------------------------------------------

Bug 3223 - package ZTE SM3 hash source to .efi for ZTE's copyrights.
TIME: 2011-12-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use ppi or protocol to let hash be independent.
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



//
// This file contains 'Framework Code' and is licensed as such 
// under the terms of your license agreement with Intel or your
// vendor.  This file may not be modified, except as allowed by
// additional terms of your license agreement.                 
//
/** @file

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcmComm.h

Abstract:

  Definitions and function prototypes used by TCM Pei driver

**/



#ifndef __TCM_COMM_H__
#define __TCM_COMM_H__

#include <IndustryStandard/Tcm12.h>
#include <IndustryStandard/TcmPlatForm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>



//---------------------------------------------------------------
#define TCM_PEI_DEBUG_SUPPORT             0


//---------------------------------------------------------------
#if     TCM_PEI_DEBUG_SUPPORT
#define TCM_PEI_MEM_DEBUG_LOG_MAX_SIZE    1024
#define TCM_MEM_LOG(Expression)           TcmMemLogDebug Expression
VOID
TcmMemLogDebug (
  IN  CONST CHAR8  *Format,
  ...
  );
#else
#define TCM_MEM_LOG(Expression)
#endif




//---------------------------------------------------------------
#define TCM_BASE_ADDRESS            0xfed40000

typedef EFI_HANDLE  EFI_TCM_HANDLE;

#pragma pack (1)
typedef struct {
  UINT8                             Access;             // 0
  UINT8                             Reserved1[7];       // 1
  UINT32                            IntEnable;          // 8
  UINT8                             IntVector;          // 0ch
  UINT8                             Reserved2[3];       // 0dh
  UINT32                            IntSts;             // 10h
  UINT32                            IntfCapability;     // 14h
  UINT8                             Status;             // 18h
  UINT16                            BurstCount;         // 19h
  UINT8                             Reserved3[9];
  UINT32                            DataFifo;           // 24h
  UINT8                             Reserved4[0xed8];   // 28h
  UINT16                            Vid;                // 0f00h
  UINT16                            Did;                // 0f02h
  UINT8                             Rid;                // 0f04h
  UINT8                             TcgDefined[0x7b];   // 0f05h
  UINT32                            LegacyAddress1;     // 0f80h
  UINT32                            LegacyAddress1Ex;   // 0f84h
  UINT32                            LegacyAddress2;     // 0f88h
  UINT32                            LegacyAddress2Ex;   // 0f8ch
  UINT8                             VendorDefined[0x70];// 0f90h
} TCM_PC_REGISTERS;


#define TCM_PC_VALID                BIT7
#define TCM_PC_ACC_ACTIVE           BIT5
#define TCM_PC_ACC_RQUUSE           BIT1

#define TCM_PC_STS_READY            BIT6
#define TCM_PC_STS_GO               BIT5
#define TCM_PC_STS_DATA             BIT4
#define TCM_PC_STS_EXPECT           BIT3

#define TCM_TIMEOUT_B               2000 * 1000  // 2s
#define TCM_TIMEOUT_C               750 * 1000   // 750ms
#define TCM_TIMEOUT_D               750 * 1000   // 750ms



typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
  TCM_STARTUP_TYPE      TcmSt;
} TCM_CMD_START_UP;

typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
} TCM_CMD_SELF_TEST;

typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
  UINT32                Capability;
  UINT32                CapabilityFlagSize;
  UINT32                CapabilityFlag;
} TCM_CMD_GET_CAPABILITY;

typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
  TCM_PCRINDEX          PcrIndex;
  TCM_DIGEST            TcmDigest;
} TCM_CMD_EXTEND;

typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
  TCM_PHYSICAL_PRESENCE PhysicalPresence;
} TCM_CMD_PHYSICAL_PRESENCE;

#pragma pack()









/**
  Send TCM_Startup command to TCM.

  @param[in] TcmRegs    TCM MMIO Base Address Pointer.  
  @param[in] BootMode   Boot mode.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommStartup (
  IN      volatile TCM_PC_REGISTERS *TcmRegs,
  IN      EFI_BOOT_MODE             BootMode
  );









/**
  Send TCM_ContinueSelfTest command to TCM.

  @param[in] TcmRegs    TCM MMIO Base Address Pointer.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommContinueSelfTest (
  IN      volatile TCM_PC_REGISTERS *TcmRegs
  );







/**
  Get TCM capability flags.

  @param[in] TcmRegs        TCM MMIO Base Address Pointer.  
  @param[out] Deactivated   Returns deactivated flag.
  @param[out] LifetimeLock  Returns physicalPresenceLifetimeLock permanent flag.  
  @param[out] CmdEnable     Returns physicalPresenceCMDEnable permanent flag.
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommGetCapability (
     IN   volatile TCM_PC_REGISTERS *TcmRegs,
     OUT  BOOLEAN                   *Deactivated, OPTIONAL
     OUT  BOOLEAN                   *LifetimeLock, OPTIONAL
     OUT  BOOLEAN                   *CmdEnable OPTIONAL
  );







/**
  Extend a TCM PCR.

  @param[in]  TcmRegs         TCM MMIO Base Address Pointer.  
  @param[in]  DigestToExtend  The 256 bit value representing the event to be recorded.  
  @param[in]  PcrIndex        The PCR to be updated.
  @param[out] NewPcrValue     New PCR value after extend.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommExtend (
  IN      volatile TCM_PC_REGISTERS *TcmRegs,
  IN      TCM_DIGEST                *DigestToExtend,
  IN      TCM_PCRINDEX              PcrIndex,
  OUT     TCM_DIGEST                *NewPcrValue
  );







/**
  Send TSC_PhysicalPresence command to TCM.

  @param[in] TcmRegs            TCM MMIO Base Address Pointer.  
  @param[in] PhysicalPresence   The state to set the TPMs Physical Presence flags.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommPhysicalPresence (
  IN      volatile TCM_PC_REGISTERS *TcmRegs,
  IN      TCM_PHYSICAL_PRESENCE     PhysicalPresence
  );






/**
  Get the control of TCM chip by sending requestUse command TCM_PC_ACC_RQUUSE 
  to ACCESS Register in the time of default TCM_TIMEOUT_D.

  @param[in] TcmRegs            TCM MMIO Base Address Pointer.

  @retval EFI_SUCCESS           Get the control of TCM chip.
  @retval EFI_INVALID_PARAMETER TcmRegs is NULL.
  @retval EFI_NOT_FOUND         TCM chip doesn't exit.
  @retval EFI_TIMEOUT           Can't get the TCM control in time.
**/
EFI_STATUS
EFIAPI
TcmPcRequestUseTcm (
  volatile TCM_PC_REGISTERS *TcmRegs
  );





//-------------------------------------------------------------
#endif
