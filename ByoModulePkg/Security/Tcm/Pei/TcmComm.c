/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmComm.c

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

Bug 3216 - add Tcm SW SM3 hash support.
TIME: 2011-12-13
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Use ZTE lib to do sm3 hash.
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

/*++
  This file contains 'Framework Code' and is licensed as such   
  under the terms of your license agreement with Intel or your  
  vendor.  This file may not be modified, except as allowed by  
  additional terms of your license agreement.                   
--*/
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

  TcmComm.c

Abstract:

  Utility functions used by TPM Pei driver

**/

#include "TcmComm.h"




/**
  Check whether the value of TCM chip register satisfies the input BIT setting.

  @param  Register     Address port of register to be checked.
  @param  BitSet       Check these data bits are set.
  @param  BitClear     Check these data bits are clear.
  @param  TimeOut      The max wait time (unit 30MicroSecond) when checking register.

  @retval EFI_SUCCESS  The register satisfies the check bit.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
EFIAPI
TcmPcWaitRegisterBits (
  IN      volatile UINT8    *Register,
  IN               UINT8    BitSet,
  IN               UINT8    BitClear,
  IN               UINT32   TimeOut
  )
{
  UINT8   RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30){
    RegRead = MmioRead8((UINTN)Register);
    if ((RegRead & BitSet) == BitSet && (RegRead & BitClear) == 0)
      return EFI_SUCCESS;
    MicroSecondDelay(30);
  }
  return EFI_TIMEOUT;
}




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
  IN volatile TCM_PC_REGISTERS *TcmRegs
  )
{
  EFI_STATUS  Status;
  UINT8       Data8;
  
  Status = EFI_SUCCESS;
  
  if(TcmRegs == NULL){
    return EFI_INVALID_PARAMETER;
  }
  
  Data8 = TcmRegs->Access;
  if(Data8==0xFF){
    return EFI_NOT_FOUND;    
  }
  
  if((Data8 & (TCM_PC_ACC_ACTIVE |TCM_PC_VALID)) != (TCM_PC_ACC_ACTIVE |TCM_PC_VALID)){
    MmioWrite8((UINTN)&TcmRegs->Access, TCM_PC_ACC_RQUUSE);
    Status = TcmPcWaitRegisterBits (
               &TcmRegs->Access,
               (UINT8)(TCM_PC_ACC_ACTIVE |TCM_PC_VALID),
               0,
               TCM_TIMEOUT_D
               );
  }
  
  return Status;
}




/**
  Set TCM chip to ready state by sending ready command TCM_PC_STS_READY 
  to Status Register in time.

  @param[in] TcmRegs            TCM MMIO Base Address Pointer.

  @retval EFI_SUCCESS           TCM chip enters into ready state.
  @retval EFI_INVALID_PARAMETER TcmRegs is NULL.
  @retval EFI_TIMEOUT           TCM chip can't be set to ready state in time.
**/
EFI_STATUS
EFIAPI
TcmPcPrepareCommand (
  IN volatile TCM_PC_REGISTERS *TcmRegs
  )
{
  EFI_STATUS Status;

  if (TcmRegs == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TcmRegs->Status = TCM_PC_STS_READY;
  Status = TcmPcWaitRegisterBits (
             &TcmRegs->Status,
             TCM_PC_STS_READY,
             0,
             TCM_TIMEOUT_B
             );
  return Status;
}







/**
  Get BurstCount by reading burstCount field of TCM regiger 
  in the time of default TCM_TIMEOUT_D.

  @param[in]  TcmRegs           TCM MMIO Base Address Pointer.
  @param[OUT] BurstCount        Pointer to a buffer to store the got BurstConut.

  @retval EFI_SUCCESS           Get BurstCount.
  @retval EFI_INVALID_PARAMETER TcmRegs is NULL or BurstCount is NULL.
  @retval EFI_TIMEOUT           BurstCount can't be got in time.
**/
EFI_STATUS
EFIAPI
TcmPcReadBurstCount (
  IN   volatile TCM_PC_REGISTERS *TcmRegs,
  OUT           UINT16           *BurstCount
  )
{
  UINT32  WaitTime;
  UINT8   DataByte0;
  UINT8   DataByte1;

  if (BurstCount == NULL || TcmRegs == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  WaitTime = 0;
  do {
    DataByte0   = MmioRead8((UINTN)&TcmRegs->BurstCount);
    DataByte1   = MmioRead8((UINTN)&TcmRegs->BurstCount + 1);
    *BurstCount = (UINT16)((DataByte1 << 8) + DataByte0);
    if (*BurstCount != 0) {
      return EFI_SUCCESS;
    }
    MicroSecondDelay(30);
    WaitTime += 30;
  } while (WaitTime < TCM_TIMEOUT_D);

  return EFI_TIMEOUT;
}









/**
  Send a command to TCM for execution and return response data.

  @param[in]      TcmRegs     TCM MMIO Base Address Pointer. 
  @param[in]      BufferIn    Buffer for command data.  
  @param[in]      SizeIn      Size of command data.  
  @param[in, out] BufferOut   Buffer for response data.  
  @param[in, out] SizeOut     size of response data.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
SendTcmCommand (
  IN     volatile TCM_PC_REGISTERS  *TcmRegs,
  IN              UINT8             *BufferIn,
  IN              UINT32            SizeIn,
  IN OUT          UINT8             *BufferOut,
  IN OUT          UINT32            *SizeOut
  )
{
  EFI_STATUS                        Status;
  UINT16                            BurstCount;
  UINT32                            Index;
  UINT32                            TcmOutSize;
  TCM_RSP_COMMAND_HDR               *RspHdr;
  UINT16                            Dummy16;

  Status = TcmPcPrepareCommand(TcmRegs);
  if(EFI_ERROR(Status)){
    return Status;
  }

  Index = 0;
  while (Index < SizeIn) {
    Status = TcmPcReadBurstCount (TcmRegs, &BurstCount);
    if (EFI_ERROR (Status)) {
      goto ProcExit;
    }
    for (; BurstCount > 0 && Index < SizeIn; BurstCount--) {
      MmioWrite8((UINTN)&TcmRegs->DataFifo, BufferIn[Index]);
      Dummy16 = MmioRead16((UINTN)&TcmRegs->Vid);		// delay
      Index++;
    }
    if(Index < SizeIn){
      Status = TcmPcWaitRegisterBits(
                 &TcmRegs->Status,
                 TCM_PC_STS_EXPECT,
                 0,
                 TCM_TIMEOUT_C
                 );
      if(EFI_ERROR(Status)){
        goto ProcExit;
      }
    }      
  }

  Status = TcmPcWaitRegisterBits (
             &TcmRegs->Status,
             TCM_PC_VALID,
             TCM_PC_STS_EXPECT,
             TCM_TIMEOUT_C
             );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  MmioWrite8((UINTN)&TcmRegs->Status, TCM_PC_STS_GO);
  Status = TcmPcWaitRegisterBits (
             &TcmRegs->Status,
             (UINT8) (TCM_PC_VALID | TCM_PC_STS_DATA),
             0,
             TCM_TIMEOUT_B
             );
  if(EFI_ERROR(Status)){
    Status = EFI_TIMEOUT;
    goto ProcExit;
  }

  Index = 0;
  BurstCount = 0;
  while (Index < sizeof (TCM_RSP_COMMAND_HDR)) {
    Status = TcmPcReadBurstCount (TcmRegs, &BurstCount);
    if (EFI_ERROR (Status)) {
      goto ProcExit;
    }
    for (; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8 ((UINTN)&TcmRegs->DataFifo);
      Index++;
      if (Index == sizeof (TCM_RSP_COMMAND_HDR)) break;
    }
  }

  RspHdr = (TCM_RSP_COMMAND_HDR*)BufferOut;
  if(SwapBytes16(RspHdr->tag) != TCM_TAG_RSP_COMMAND){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;
  }
  
  TcmOutSize = SwapBytes32(RspHdr->paramSize);
  if(*SizeOut < TcmOutSize){
    Status = EFI_BUFFER_TOO_SMALL;
    goto ProcExit;
  }
  *SizeOut = TcmOutSize;

  while(Index < TcmOutSize){
    for (; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8((UINTN)&TcmRegs->DataFifo);
      Index++;
      if(Index == TcmOutSize){
        Status = EFI_SUCCESS;
        goto ProcExit;
      }
    }
    Status = TcmPcReadBurstCount(TcmRegs, &BurstCount);
    if (EFI_ERROR(Status)) {
      goto ProcExit;
    }
  }

  if(SwapBytes32(RspHdr->returnCode)!=TCM_SUCCESS){
    TCM_MEM_LOG(("RC:%X", SwapBytes32(RspHdr->returnCode)));
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }
  
ProcExit:
  MmioWrite8((UINTN)&TcmRegs->Status, TCM_PC_STS_READY);
  return Status;
}









/**
  Send TCM_Startup command to TCM.

  @param[in] TcmRegs     TCM MMIO Base Address Pointer.
  @param[in] BootMode    Boot mode.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommStartup (
  IN     volatile TCM_PC_REGISTERS *TcmRegs,
  IN              EFI_BOOT_MODE    BootMode
  )
{
  EFI_STATUS                        Status;
  TCM_STARTUP_TYPE                  TcmSt;
  UINT32                            TcmRecvSize;
  UINT32                            TcmSendSize;
  TCM_CMD_START_UP                  SendBuffer;
  UINT8                             RecvBuffer[20];

  TcmSt = TCM_ST_CLEAR;
  if (BootMode == BOOT_ON_S3_RESUME) {
    TcmSt = TCM_ST_STATE;
  }

  TcmRecvSize               = 20;
  TcmSendSize               = sizeof(TCM_CMD_START_UP);
  SendBuffer.Hdr.tag        = SwapBytes16(TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32(TcmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32(TCM_ORD_Startup);
  SendBuffer.TcmSt          = SwapBytes16(TcmSt);
  Status = SendTcmCommand(TcmRegs, (UINT8 *)&SendBuffer, TcmSendSize, RecvBuffer, &TcmRecvSize);
  return Status;
}








/**
  Send TCM_ContinueSelfTest command to TCM.

  @param[in] TcmRegs      TCM MMIO Base Address Pointer.
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommContinueSelfTest (
  IN  volatile TCM_PC_REGISTERS *TcmRegs
  )
{
  EFI_STATUS                        Status;
  UINT32                            TcmRecvSize;
  UINT32                            TcmSendSize;
  TCM_CMD_SELF_TEST                 SendBuffer;
  UINT8                             RecvBuffer[20];

  //
  // send Tpm command TPM_ORD_ContinueSelfTest
  //
  TcmRecvSize               = 20;
  TcmSendSize               = sizeof(TCM_CMD_SELF_TEST);
  SendBuffer.Hdr.tag        = SwapBytes16(TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32(TcmSendSize);  
  SendBuffer.Hdr.ordinal    = SwapBytes32(TCM_ORD_ContinueSelfTest);
  Status = SendTcmCommand(TcmRegs, (UINT8 *)&SendBuffer, TcmSendSize, RecvBuffer, &TcmRecvSize);
  return Status;
}










/**
  Get TCM capability flags.

  @param[in]  TcmRegs       TCM MMIO Base Address Pointer.
  
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
     OUT           BOOLEAN          *Deactivated, OPTIONAL
     OUT           BOOLEAN          *LifetimeLock, OPTIONAL
     OUT           BOOLEAN          *CmdEnable OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINT32                            TcmRecvSize;
  UINT32                            TcmSendSize;
  TCM_CMD_GET_CAPABILITY            SendBuffer;
  UINT8                             RecvBuffer[40];
  TCM_PERMANENT_FLAGS               *TcmPermanentFlags;

  TcmRecvSize                   = 40;
  TcmSendSize                   = sizeof(TCM_CMD_GET_CAPABILITY);
  SendBuffer.Hdr.tag            = SwapBytes16(TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize      = SwapBytes32(TcmSendSize);  
  SendBuffer.Hdr.ordinal        = SwapBytes32(TCM_ORD_GetCapability);
  SendBuffer.Capability         = SwapBytes32(TCM_CAP_FLAG);
  SendBuffer.CapabilityFlagSize = SwapBytes32(sizeof(TCM_CAP_FLAG_PERMANENT));
  SendBuffer.CapabilityFlag     = SwapBytes32(TCM_CAP_FLAG_PERMANENT);
  Status = SendTcmCommand(TcmRegs, (UINT8 *)&SendBuffer, TcmSendSize, RecvBuffer, &TcmRecvSize);
  if(EFI_ERROR(Status)){
    return Status;
  }
  TcmPermanentFlags = (TCM_PERMANENT_FLAGS *)&RecvBuffer[sizeof(TCM_RSP_COMMAND_HDR) + sizeof(UINT32)];
  if (Deactivated != NULL) {
    *Deactivated  = TcmPermanentFlags->deactivated;
  }

  if (LifetimeLock != NULL) {
    *LifetimeLock = TcmPermanentFlags->physicalPresenceLifetimeLock;
  }

  if (CmdEnable != NULL) {
    *CmdEnable = TcmPermanentFlags->physicalPresenceCMDEnable;
  }
  return Status;
}











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
  IN               TCM_DIGEST       *DigestToExtend,
  IN               TCM_PCRINDEX     PcrIndex,
  OUT              TCM_DIGEST       *NewPcrValue
  )
{
  EFI_STATUS                        Status;
  UINT32                            TcmSendSize;
  UINT32                            TcmRecvSize;
  TCM_CMD_EXTEND                    SendBuffer;
  UINT8                             RecvBuffer[sizeof(TCM_RSP_COMMAND_HDR) + sizeof(TCM_DIGEST)];

  TcmRecvSize               = sizeof(TCM_RSP_COMMAND_HDR) + sizeof (TCM_DIGEST);
  TcmSendSize               = sizeof(TCM_CMD_EXTEND);
  SendBuffer.Hdr.tag        = SwapBytes16 (TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TcmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TCM_ORD_Extend);
  SendBuffer.PcrIndex       = SwapBytes32 (PcrIndex);
  CopyMem (&SendBuffer.TcmDigest, (UINT8 *)DigestToExtend, sizeof(TCM_DIGEST));
  Status = SendTcmCommand(TcmRegs, (UINT8 *)&SendBuffer, TcmSendSize, RecvBuffer, &TcmRecvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if(NewPcrValue != NULL) {
    CopyMem((UINT8*)NewPcrValue, &RecvBuffer[sizeof(TCM_RSP_COMMAND_HDR)], sizeof (TCM_DIGEST));
  }

  return Status;
}









/**
  Send TSC_PhysicalPresence command to TCM.

  @param[in] TcmRegs            TCM MMIO Base Address Pointer. 
  @param[in] PhysicalPresence   The state to set the TCM's Physical Presence flags.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TcmCommPhysicalPresence (
  IN   volatile TCM_PC_REGISTERS          *TcmRegs,
  IN            TCM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  UINT32                            TcmSendSize;
  UINT32                            TcmRecvSize;
  TCM_CMD_PHYSICAL_PRESENCE         SendBuffer;
  UINT8                             RecvBuffer[sizeof(TCM_RSP_COMMAND_HDR)];

  TcmRecvSize                 = sizeof(TCM_RSP_COMMAND_HDR);
  TcmSendSize                 = sizeof(TCM_CMD_PHYSICAL_PRESENCE);
  SendBuffer.Hdr.tag          = SwapBytes16 (TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize    = SwapBytes32 (TcmSendSize);
  SendBuffer.Hdr.ordinal      = SwapBytes32 (TSC_ORD_PhysicalPresence);
  SendBuffer.PhysicalPresence = SwapBytes16 (PhysicalPresence);
  Status = SendTcmCommand(TcmRegs, (UINT8 *)&SendBuffer, TcmSendSize, RecvBuffer, &TcmRecvSize);
  return Status;
}




#if  TCM_PEI_DEBUG_SUPPORT

EFI_GUID gTcmMemDebugLogGuid = \
  {0xbd72a681, 0xb008, 0x471c, {0x8f, 0xcc, 0xfa, 0xcb, 0x91, 0x47, 0xef, 0xf8}};

VOID
TcmMemLogDebug (
  IN  CONST CHAR8  *Format,
  ...
  )
{
  CHAR8    Buffer[64];
  VA_LIST  Marker;
  UINT8    *Log;
  UINT8    *LogStart;
  UINT16   Cur;
  UINTN    StrSize;

  VA_START (Marker, Format);
  AsciiVSPrint(Buffer, sizeof(Buffer), Format, Marker);
  VA_END (Marker);

  Log = GetFirstGuidHob(&gTcmMemDebugLogGuid);
  if(Log==NULL){
    Log = BuildGuidHob(&gTcmMemDebugLogGuid, TCM_PEI_MEM_DEBUG_LOG_MAX_SIZE);
    *(UINT16*)Log = 0;
  }else{
    Log = Log + sizeof(EFI_HOB_GUID_TYPE);
  }
  LogStart  = Log;
  Cur       = *(UINT16*)Log;
  Log       = Log + sizeof(UINT16) + Cur;
  StrSize   = AsciiStrSize(Buffer);
  CopyMem(Log, (UINT8 *)Buffer, StrSize);

  *(UINT16*)LogStart = *(UINT16*)LogStart + (UINT16)StrSize;
}

#endif





