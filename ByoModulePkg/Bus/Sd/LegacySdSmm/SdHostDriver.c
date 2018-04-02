/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SDHostDriver.c

Abstract: 
  SD card Model.

Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-16
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE: SD card feature support.
$TECHNICAL:
$END--------------------------------------------------------------------------

**/

#include "SdHostDriver.h"
#include "MediaDeviceDriver.h"
//------------------------------------------------------------------------------
// Debug helper
//------------------------------------------------------------------------------
int            _inp (unsigned short port);
int            _outp (unsigned short port, int databyte );
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)

EFI_SD_HOST_IO_PROTOCOL  mSdHostIo = {
  EFI_SD_HOST_IO_PROTOCOL_REVISION_01,
  {
    0, // HighSpeedSupport
    0, // V18Support
    0, // V30Support
    0, // V33Support
    0, // Reserved0
    0, // BusWidth4
    0, // BusWidth8
    0, // Reserved1
    0,
    0,
    0,
    0,
    0, // ADMA2Support
    0, // DmaMode
    0, // ReTune Timer
    0, // ReTune Mode
    0, // Reserved2
    (512 * 1024) //BoundarySize 512 KB
  },
  0,//PFA
  0,//Slot0RegMap
  SendHCommand,
  SetClockFrequency,
  SetBusWidth,
  SetHostVoltage,
  SetHostDdrMode,
  ResetSdHost,
  EnableAutoStopCmd,
  DetectCardAndInitHost,
  SetBlockLength,
  SetupDevice,
  SetHostSpeedMode
};


EFI_STATUS
EFIAPI
SdHostDriverStart (
  IN UINT8  BusNum,
  IN UINT8  DeviceNum,
  IN UINT8  FunNum,
  IN UINT32 MemAddress,
  IN UINT32 PciDidVid
  )
{
  EFI_STATUS            Status;
  SDHOST_DATA           *SdHostData;
  UINT32                Data;
  UINT16                Data16;

  SdHostData = NULL;
  Data       = 0;

  SdHostData = (SDHOST_DATA*)AllocateZeroPool (sizeof (SDHOST_DATA));
  if (SdHostData == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  DEBUG ((EFI_D_ERROR, "SdHostDriverStart SdHostData:%x\n",SdHostData));
  SdHostData->Signature   = SDHOST_DATA_SIGNATURE;

  CopyMem (&SdHostData->SdHostIo, &mSdHostIo, sizeof (EFI_SD_HOST_IO_PROTOCOL));
  SdHostData->PciDid = (UINT16)(PciDidVid >> 16);
  SdHostData->SdHostIo.Slot0RegMap = MemAddress;

  ResetSdHost (&SdHostData->SdHostIo, Reset_All);
  SdHostData->EnableVerboseDebug = FALSE; 

  SdHostData->SdHostIo.PFA = (UINT16) (BusNum << 8 | (DeviceNum << 3) | FunNum);
  DEBUG ((EFI_D_ERROR, "SDHost BusNum = %x, DeviceNum = %x, FunNum = %x\n", BusNum, DeviceNum, FunNum));

  SdMemMapAccess(MemAddress, 0, MMIO_CTRLRVER, EfiPciIoWidthUint16, 1, (VOID *) &Data16);
  DEBUG ((EFI_D_INFO, "SdHost MMIO_CTRLRVER = 0x%08x\n", Data16));
  SdHostData->ControllerVersion = Data16 & 0xFF;

  SdMemMapAccess(MemAddress, 0, MMIO_CAP, EfiPciIoWidthUint32, 1, (VOID *) &Data);
  SdHostData->SdHostIo.HostCapability.BusWidth4 = TRUE;  
  DEBUG ((EFI_D_INFO, "SdHost BusWidth4\n"));
  if ((Data & BIT18) != 0) {
    SdHostData->SdHostIo.HostCapability.BusWidth8 = TRUE;
    DEBUG ((EFI_D_INFO, "SdHost BusWidth8\n"));
  }

  if ((Data & BIT21) != 0) {
    SdHostData->SdHostIo.HostCapability.HighSpeedSupport = TRUE;
    DEBUG ((EFI_D_INFO, "SdHost HighSpeedSupport\n"));
  }
  
  if ((Data & BIT24) != 0) {
    SdHostData->SdHostIo.HostCapability.V33Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHost V33Support\n"));
  }

  if ((Data & BIT25) != 0) {
    SdHostData->SdHostIo.HostCapability.V30Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHost V30Support\n"));
  }

  if ((Data & BIT26) != 0) {
    SdHostData->SdHostIo.HostCapability.V18Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHost V18Support\n"));
  }

  SdHostData->BaseClockInMHz = (Data >> 8) & 0xFF;

  SdMemMapAccess(MemAddress, 0, MMIO_CAP + 4, EfiPciIoWidthUint32, 1, (VOID *) &Data);
  DEBUG ((EFI_D_INFO, "High MMIO_CAP = 0x%08x\n", Data));
  if ((Data & 0x1<<(32-32)) != 0) {
    SdHostData->SdHostIo.HostCapability.SDR50Support= TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SDR50Support\n"));
  }

  if ((Data & 0x1<<(33-32)) != 0) {
    SdHostData->SdHostIo.HostCapability.SDR104Support= TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SDR104Support\n"));
  }

  if ((Data & 0x1<<(34-32)) != 0) {
    SdHostData->SdHostIo.HostCapability.DDR50Support= TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: DDR50Support\n"));
  }
  
  if (SdHostData->ControllerVersion >= 2) {
    SdHostData->SdHostIo.HostCapability.ReTuneMode = (Data >> (46-32)) & 0x3;   
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart:  ReTuneMode = 0x%08x\n", SdHostData->SdHostIo.HostCapability.ReTuneMode));
    SdHostData->SdHostIo.HostCapability.ReTuneTimer = (Data>>(40-32)) & 0xF;   
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart:  ReTuneTimer = 0x%08x\n", SdHostData->SdHostIo.HostCapability.ReTuneTimer));
  }

  SdHostData->BlockLength    = BLOCK_SIZE;
  SdHostData->IsAutoStopCmd  = TRUE;

  Status = MediaDeviceDriverStart(&SdHostData->SdHostIo);
Exit:
  if (EFI_ERROR (Status)) {
    if (SdHostData != NULL) {
      FreePool (SdHostData);
    }
  }
  return Status;
}



