/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MediaDeviceDriver.c

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

  MediaDeviceDriver.c

Abstract:

  UEFI Driver Entry and Binding support.

**/

#include "MediaDeviceDriver.h"
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePath.h>

//
// MMCSDIOController Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gMediaDeviceDriverBinding = {
  MediaDeviceDriverBindingSupported,
  MediaDeviceDriverBindingStart,
  MediaDeviceDriverBindingStop,
  0x10,
  NULL,
  NULL
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
MediaDeviceDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMediaDeviceDriverBinding,
           ImageHandle,
           &gMediaDeviceComponentName,
           &gMediaDeviceComponentName2
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
MediaDeviceDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_SD_HOST_IO_PROTOCOL   *SDHostIo;
  //DEBUG ((EFI_D_ERROR, "Native MediaDeviceDriverBindingSupported \n"));
  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdHostIoProtocolGuid,
                  &SDHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiSdHostIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

Exit:
  //DEBUG ((EFI_D_ERROR, "Native MediaDeviceDriverBindingSupported Status = %r \n", Status));
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
MediaDeviceDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo;
  CARD_DATA                 *CardData;
  UINTN                     Loop;

  DEBUG ((EFI_D_ERROR, "Native MediaDeviceDriver \n"));
  CardData = NULL;
  //
  // Open PCI I/O Protocol and save pointer to open protocol
  // in private data area.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdHostIoProtocolGuid,
                  (VOID **) &SdHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to open gEfiSdHostIoProtocolGuid \n"));
    goto Exit;
  }

  Status = SdHostIo->DetectCardAndInitHost (SdHostIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to DetectCardAndInitHost %r\n", Status));
    goto Exit;
  }

  CardData = (CARD_DATA*)AllocateZeroPool(sizeof (CARD_DATA));
  if (CardData == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to AllocateZeroPool(CARD_DATA) \n"));
    goto Exit;
  }

  ASSERT (SdHostIo->HostCapability.BoundarySize >= 4 * 1024);
  //CardData->RawBufferPointer = (UINT8*)AllocateZeroPool (2 * SdHostIo->HostCapability.BoundarySize);
  CardData->RawBufferPointer = (UINT8*)(UINTN)0xffffffff;
 
  DEBUG ((EFI_D_ERROR, "CardData->RawBufferPointer = 0x%x \n",CardData->RawBufferPointer));
  DEBUG ((EFI_D_ERROR, "requesting 0x%x pages \n",EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize)));
//Allocated the buffer under 4G
  Status = gBS->AllocatePages (
				  AllocateMaxAddress,
				  EfiBootServicesData,
				  EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize),
				  (EFI_PHYSICAL_ADDRESS *)(&(CardData->RawBufferPointer))
				  );
  DEBUG ((EFI_D_ERROR, "CardData->RawBufferPointer = 0x%x \n",CardData->RawBufferPointer));
  if (!EFI_ERROR (Status)) {
    CardData->RawBufferPointer = ZeroMem (CardData->RawBufferPointer, 2 * SdHostIo->HostCapability.BoundarySize);
  } else {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to AllocateZeroPool(2*x) \n"));
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CardData->AlignedBuffer = CardData->RawBufferPointer - ((UINTN)(CardData->RawBufferPointer) & (SdHostIo->HostCapability.BoundarySize - 1)) + SdHostIo->HostCapability.BoundarySize;
  DEBUG ((EFI_D_ERROR, "CardData->AlignedBuffer = 0x%x \n",CardData->AlignedBuffer));

  CardData->Signature = CARD_DATA_SIGNATURE;
  CardData->SdHostIo  = SdHostIo;
  CardData->Handle    = Controller;
  for (Loop = 0; Loop < MAX_NUMBER_OF_PARTITIONS; Loop++) {
    CardData->Partitions[Loop].Signature = CARD_PARTITION_SIGNATURE;
    CardData->Partitions[Loop].CardData = CardData;
  }

  Status = MmcSdCardInit (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to MMCSDCardInit \n"));
    goto Exit;
  }
  DEBUG ((EFI_D_INFO, "MediaDeviceDriverBindingStart: MMC SD card\n"));
  Status = MmcSdBlockIoInit (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Card BlockIo init failed\n"));
    goto Exit;
  }

  Status = MediaDeviceDriverInstallBlockIo (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to install gEfiBlockIoProtocolGuid \n"));
    goto Exit;
  }

  //
  // Component name protocol
  //
  AddUnicodeString2 (
             "eng",
             gMediaDeviceComponentName.SupportedLanguages,
             &CardData->ControllerNameTable,
             L"MMC/SD Media Device",
             TRUE
             );
    AddUnicodeString2 (
             "en",
             gMediaDeviceComponentName2.SupportedLanguages,
             &CardData->ControllerNameTable,
             L"MMC/SD Media Device",
             FALSE
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    MediaDeviceDriverUninstallBlockIo (CardData);
  }

  DEBUG ((EFI_D_INFO, "MediaDeviceDriverBindingStart: Started\n"));
Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: End with failure\n"));
    if (CardData != NULL) {
      if (CardData->RawBufferPointer != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)(CardData->RawBufferPointer), EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize));
      }
      FreePool (CardData);
    }
    gBS->CloseProtocol (
           Controller,
           &gEfiSdHostIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }
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
MediaDeviceDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  CARD_DATA                 *CardData;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo;

  //
  // First find BlockIo Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiSdHostIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );


  CardData  = CARD_DATA_FROM_THIS(BlockIo);
  SdHostIo  = CardData->SdHostIo;
  //
  // Uninstall Block I/O protocol from the device handle
  //
  Status = MediaDeviceDriverUninstallBlockIo (CardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CardData != NULL) {
    FreeUnicodeStringTable (CardData->ControllerNameTable);
    if (CardData->RawBufferPointer != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)(CardData->RawBufferPointer), EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize));
    }
    FreePool (CardData);
  }

  return Status;

}


STATIC
struct {
  CONTROLLER_DEVICE_PATH    Controller;
  EFI_DEVICE_PATH_PROTOCOL  End;
} ControllerDevPathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_CONTROLLER_DP,
      {
        sizeof (CONTROLLER_DEVICE_PATH),
        0
      },
    },
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};


EFI_STATUS
MediaDeviceDriverInstallBlockIo (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS                Status;
  UINTN                     Loop;
  MMC_PARTITION_DATA        *Partition;
  EFI_DEVICE_PATH_PROTOCOL  *MainPath;

  Partition = CardData->Partitions;

  Status = gBS->HandleProtocol (
                  CardData->Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**) &MainPath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Loop = 0; Loop < MAX_NUMBER_OF_PARTITIONS; Partition++, Loop++) {
    if (!Partition->Present) {
      continue;
    }

    DEBUG ((EFI_D_INFO, "MediaDeviceDriverInstallBlockIo: Installing Block I/O for partition %d\n", Loop));

    Partition->Handle = NULL;
    Partition->CardData = CardData;

    ControllerDevPathTemplate.Controller.ControllerNumber = (UINT32) Loop;
    Partition->DevPath =
      AppendDevicePath (
        MainPath,
        (EFI_DEVICE_PATH_PROTOCOL *) &ControllerDevPathTemplate
        );
    if (Partition->DevPath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    Status = gBS->InstallProtocolInterface (
                    &(Partition->Handle),
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    Partition->DevPath
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gBS->InstallProtocolInterface (
                    &(Partition->Handle),
                    &gEfiBlockIoProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Partition->BlockIo
                    );
  }

  return Status;
}


EFI_STATUS
MediaDeviceDriverUninstallBlockIo (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS          Status;
  UINTN               Loop;
  MMC_PARTITION_DATA  *Partition;

  Partition = CardData->Partitions;

  Status = EFI_SUCCESS;

  for (Loop = 0; Loop < MAX_NUMBER_OF_PARTITIONS; Partition++, Loop++) {
    if (!Partition->Present) {
      continue;
    }
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Partition->Handle,
                    &gEfiBlockIoProtocolGuid,
                    &Partition->BlockIo,
                    &gEfiDevicePathProtocolGuid,
                    Partition->DevPath,
                    NULL
                    );
  }

  return Status;
}
