/** @file

Copyright (c) 2006 - 2014, Byosoft Corporation.<BR> 
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
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/
/*++
  This file contains an 'Intel Peripheral Driver' and uniquely  
  identified as Intel "Mobile Silicon Support Code" and is      
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

Copyright (c) 2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


File Name:

  SdHostDriver.c

Abstract:

  UEFI Driver Entry and Binding support.

**/

#include "SDHostDriver.h"

EFI_GUID gEfiLegacySDInfProtocolGuid = EFI_LEGACY_SD_INF_PROTOCOL_GUID;
EFI_SMM_BASE2_PROTOCOL    *gSMM2     = NULL;
EFI_CPU_IO2_PROTOCOL      *mCpuIo2   = NULL;
volatile SD_SMMCALL_COMM  *mSmmCallComm;
SMMCALL_ENTRY             *mSmmCallTablePtr;
EFI_SMM_CONTROL2_PROTOCOL *mSmmControl;
UINT8                     mArgBufferSize = 1;
UINT8                     mArgBuffer     = SD_SMI_VALUE;

//------------------------------------------------------------------------------
// Debug helper
//------------------------------------------------------------------------------
int _inp (unsigned short port);
int _outp (unsigned short port, int databyte );
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)

/**
  Test if current operation mode is in SMM or not.

  @return InSmm     TRUE is in SMM or FALSE is not in SMM.

**/
BOOLEAN
IfInSmm (
  VOID
  )
{
    BOOLEAN InSmm;
    gSMM2->InSmm (gSMM2, &InSmm);
    return InSmm;
}

/**
  Writes an 8-bit I/O port.

  Writes the 8-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
VOID
IoWrite8 (
  IN UINT16  Port,
  IN UINT8   Value
  )
{
  mCpuIo2->Io.Write (mCpuIo2, EfiCpuIoWidthUint8, Port, 1, &Value);
}

//
// MMCSDIOController Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gSdHostDriverBinding = {
  SdHostDriverBindingSupported,
  SdHostDriverBindingStart,
  SdHostDriverBindingStop,
  0x10,
  NULL,
  NULL
};

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

/**
  The entry point for SD driver which installs the driver binding and component name
  protocol on its ImageHandle.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            if the driver binding and component name protocols 
                                 are successfully
  @retval Others                 Failed to install the protocols.

**/
EFI_STATUS
EFIAPI
SdHostDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gSdHostDriverBinding,
           ImageHandle,
           &gSdHostComponentName,
           &gSdHostComponentName2
           );
}

/**
  Tests to see if this driver supports a given controller. 

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, 
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver 
                                   specified by This.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the 
                                   driver specified by This.

**/
EFI_STATUS
EFIAPI
SdHostDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo;
  PCI_CLASSC                PciClass;
  UINT32                    VidDid;
  EFI_LEGACY_SD_INF_PROTOCOL   *LegacySDInf;

  //DEBUG ((EFI_D_ERROR, "Native SDHostDriverBindingSupported \n"));

  //
  // Verify the SD IO Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdHostIoProtocolGuid,
                  (VOID **)&SdHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG (( DEBUG_INFO, "SdHost controller already started, Controller:0x%016Lx\r\n",  (UINT64)(UINTN)Controller));
    Status = EFI_ALREADY_STARTED;
    return Status;
  }

  Status = gBS->LocateProtocol (
          &gEfiLegacySDInfProtocolGuid,
          NULL,
          &LegacySDInf
          );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    return Status;
  }

  mSmmCallComm     = LegacySDInf->SmmCallCommBuf;
  mSmmCallTablePtr = LegacySDInf->SmmCallTablePtr;
  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (PCI_CLASSC) / sizeof (UINT8),
                        &PciClass
                        );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  if ((PciClass.BaseCode != PCI_CLASS_SYSTEM_PERIPHERAL) ||
      (PciClass.SubClassCode != PCI_SUBCLASS_SD_HOST_CONTROLLER) ||
      ((PciClass.PI != PCI_IF_STANDARD_HOST_NO_DMA) && (PciClass.PI != PCI_IF_STANDARD_HOST_SUPPORT_DMA))
      ) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &VidDid
                        );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
/*
 if (!(VidDid == 0x0F148086 || 
       VidDid == 0x0F508086 || 
       VidDid == 0x0F168086 || 
       VidDid == 0x22948086 || 
       VidDid == 0x22968086 ||
       VidDid == 0x95D01106)){
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }
*/

  Status = gBS->LocateProtocol(&gEfiSmmControl2ProtocolGuid, NULL, &mSmmControl);

Exit:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  //DEBUG ((EFI_D_ERROR, "Native SDHostDriverBindingSupported Status = %r \n", Status));
  return Status;
}

/**
  Start this driver on ControllerHandle. 

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, 
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of 
                                   resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
SdHostDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_PCI_IO_PROTOCOL          *PciIo;
  EFI_STATUS                   Status;
  SDHOST_DATA                  *SdHostData;
  UINT32                       Data;
  UINT16                       Data16;
  UINT32                VidDid;
  UINT32                Bar0 = 0;
  UINT32                Bar1 = 0;
  UINTN                 Seg, Bus, Dev, Func;

  DEBUG ((EFI_D_ERROR, "Native SDHostDriverBindingStart \n"));
  SdHostData = NULL;
  Data       = 0;
  
  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **) &mCpuIo2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, &gSMM2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open PCI I/O Protocol and save pointer to open protocol
  // in private data area.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Enable the SD Host Controller MMIO space
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  SdHostData = (SDHOST_DATA*)AllocateZeroPool (sizeof (SDHOST_DATA));
  if (SdHostData == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  DEBUG ((EFI_D_ERROR, "SdHostDriverBindingStart SdHostData:%x\n",SdHostData));
  SdHostData->Signature   = SDHOST_DATA_SIGNATURE;
  SdHostData->PciIo       = PciIo;

  CopyMem (&SdHostData->SdHostIo, &mSdHostIo, sizeof (EFI_SD_HOST_IO_PROTOCOL));

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &VidDid
                        );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: VidDid = 0x%08x\n", VidDid));

  Status = PciIo->GetLocation (
                    PciIo,
                    &Seg,
                    &Bus,
                    &Dev,
                    &Func
                    );
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: Seg %d, bus:%d,Dev:%d,Func:%d\n", Seg, Bus, Dev,Func));
  
  SdHostData->PciVid = (UINT16)(VidDid & 0xffff);
  SdHostData->PciDid = (UINT16)(VidDid >> 16);

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0x10,
                        1,
                        &Bar0
                        );
  SdHostData->SdHostIo.Slot0RegMap = Bar0;
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0x14,
                        1,
                        &Bar1
                        );

  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: MEMIO Base0 %x\n", Bar0));
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: MEMIO Base1 %x\n", Bar1));
  

  SdHostData->SdHostIo.ResetSdHost (&SdHostData->SdHostIo, Reset_All);
  SdHostData->EnableVerboseDebug = FALSE; 

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint16,    
               0,
               (UINT64)MMIO_CTRLRVER,
               1,
               &Data16
               );
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: MMIO_CTRLRVER = 0x%08x\n", Data16));
  
  SdHostData->ControllerVersion = Data16 & 0xFF;
  switch (SdHostData->ControllerVersion) {
        case 0: DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SD Host Controller Version 1.0\n")); break;
        case 1: DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SD Host Controller Version 2.0\n")); break;
        case 2: DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SD Host Controller Version 3.0\n")); break;
        case 3: DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: SD Host Controller Version 4.0\n")); break;
        default:
            DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: Unknown SD Host Controller Version, Stopping Driver!!\n")); 
        goto Exit;
  }

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,    
               0,
               (UINT32)MMIO_CAP,
               1,
               &Data
               );
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: Low MMIO_CAP = 0x%08x\n", Data));

  SdHostData->SdHostIo.HostCapability.BusWidth4 = TRUE;
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: BusWidth4\n"));
  if ((Data & BIT18) != 0) {
    SdHostData->SdHostIo.HostCapability.BusWidth8 = TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: BusWidth8\n"));
  }

  if ((Data & BIT21) != 0) {
    SdHostData->SdHostIo.HostCapability.HighSpeedSupport = TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: HighSpeedSupport\n"));
  }
  
  if ((Data & BIT24) != 0) {
    SdHostData->SdHostIo.HostCapability.V33Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: V33Support\n"));
  }

  if ((Data & BIT25) != 0) {
    SdHostData->SdHostIo.HostCapability.V30Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: V30Support\n"));
  }

  if ((Data & BIT26) != 0) {
    SdHostData->SdHostIo.HostCapability.V18Support = TRUE;
    DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: V18Support\n"));
  }

  SdHostData->BaseClockInMHz = (Data >> 8) & 0xFF;
  
  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,    
               0,
               (UINT32)(MMIO_CAP + 4),
               1,
               &Data
               );

  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: High MMIO_CAP = 0x%08x\n", Data));

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

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSdHostIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &SdHostData->SdHostIo
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Component name protocol
  //
  AddUnicodeString2 (
             "eng",
             gSdHostComponentName.SupportedLanguages,
             &SdHostData->ControllerNameTable,
             L"SD Host Controller",
             TRUE
             );
  AddUnicodeString2 (
    "en",
    gSdHostComponentName2.SupportedLanguages,
    &SdHostData->ControllerNameTable,
    L"SD Host Controller",
    FALSE
    );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiSdHostIoProtocolGuid,
           &SdHostData->SdHostIo
           );
  }

  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: Host Started\n"));

Exit:
  if (EFI_ERROR (Status)) {
    if (SdHostData != NULL) {
      FreePool (SdHostData);
    }
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }
  SMMCALL_0(StartLegacySd, &Status);
  return Status;
}

/**
  Stop this driver on ControllerHandle. 

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
                                Not used.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.Not used.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
SdHostDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  EFI_SD_HOST_IO_PROTOCOL  *SdHostIo;
  SDHOST_DATA              *SdHostData;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdHostIoProtocolGuid,
                  (VOID**) &SdHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto _exit_SdHostDriverBindingStop;
  }
  
  

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );


  SdHostData  = SDHOST_DATA_FROM_THIS(SdHostIo);

  //
  // Uninstall Block I/O protocol from the device handle
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSdHostIoProtocolGuid,
                  &SdHostData->SdHostIo
                  );
  if (EFI_ERROR (Status)) {
    goto _exit_SdHostDriverBindingStop;
  }
  FreeUnicodeStringTable (SdHostData->ControllerNameTable);

  FreePool (SdHostData);
_exit_SdHostDriverBindingStop:  
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStop exited with Status %r\n", Status));
  return Status;

}
