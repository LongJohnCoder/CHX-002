/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmApp.c

Abstract: 
  Tcm application.

Revision History:

Bug 3256 - add a application to read all PCRs.
TIME: 2011-12-27
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use Tcm protocol to read all PCRs.
$END------------------------------------------------------------

**/

//---------------------------------------------------------
#include <Uefi.h>
#include <IndustryStandard/Tcm12.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Protocol/TcmService.h>



//---------------------------------------------------------
#pragma pack(1)

typedef struct {
  TCM_RQU_COMMAND_HDR   Hdr;
  TCM_PCRINDEX          PcrIndex;
} TCM_CMD_READ_PCR;

typedef struct {
  TCM_RSP_COMMAND_HDR   Hdr;
  TCM_PCRVALUE          PcrValue;
} TCM_CMD_READ_PCR_RSP;

#pragma pack()








//---------------------------------------------------------
EFI_STATUS
TcmReadPCR (
  IN      EFI_TCM_PROTOCOL  *ptTcm,
  IN      TCM_PCRINDEX      PcrIndex,
  OUT     TCM_PCRVALUE      *PcrValue
  )
{
  EFI_STATUS            Status;
  UINT32                TcmSendSize;
  UINT32                TcmRecvSize;
  TCM_CMD_READ_PCR      SendBuffer;
  TCM_CMD_READ_PCR_RSP  RecvBuffer;

  if(PcrValue==NULL || ptTcm==NULL){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit; 
  }

  TcmRecvSize               = sizeof(RecvBuffer);
  TcmSendSize               = sizeof(SendBuffer);  
  SendBuffer.Hdr.tag        = SwapBytes16(TCM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32(TcmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32(TCM_ORD_PcrRead);
  SendBuffer.PcrIndex       = SwapBytes32(PcrIndex);
  
  Status = ptTcm->PassThroughToTcm(
                    ptTcm,
                    TcmSendSize,
                    (UINT8*)&SendBuffer,
                    TcmRecvSize,
                    (UINT8*)&RecvBuffer
                    );
  if(EFI_ERROR (Status)){
    goto ProcExit;
  }

  CopyMem(PcrValue, &RecvBuffer.PcrValue, sizeof(TCM_PCRVALUE));

ProcExit:
  return Status;
}






EFI_STATUS
EFIAPI
TcmAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  TCM_PCRINDEX      PcrIndex;
  TCM_PCRVALUE      PcrValue;
  UINTN             i;
  UINT8             *Data8;
  EFI_TCM_PROTOCOL  *ptTcm;  
  
  Status = gBS->LocateProtocol(&gEfiTcmProtocolGuid, NULL, &ptTcm);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  
  Print(L"Read All Tcm PCRs: \n");  
  
  for(PcrIndex=0;PcrIndex<8;PcrIndex++){
    Status = TcmReadPCR(ptTcm, PcrIndex, &PcrValue);
    if(EFI_ERROR(Status)){
      break; 
    }
    Print(L"PCR[%d]\n", PcrIndex);
    
    Data8 = (UINT8*)&PcrValue;
    for(i=0;i<sizeof(PcrValue);i++){
      Print(L"%02X ", Data8[i]);
      if((i+1)%16==0){
        Print(L"\n"); 
      }
    }
    Print(L"\n");
  }

ProcExit:   
  return Status;  
}

