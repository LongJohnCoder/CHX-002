/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  UsbBus.c

Abstract: 
  USB Module file.

Revision History:

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/
/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  UsbBus.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "PiDxe.h"
#include "UsbBus.h"
#include <Protocol/smmbase2.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/LegacyUsbInf.h>
#include <Protocol/SmmControl2.h>

//
// USB_BUS_PROTOCOL is only used to locate USB_BUS
//
EFI_GUID                mUsbBusProtocolGuid = EFI_USB_BUS_PROTOCOL_GUID;
LIST_ENTRY              mNativeUsbIfList;

EFI_CPU_IO2_PROTOCOL      *mCpuIo = NULL;
volatile USB_SMMCALL_COMM *mSmmCallComm;
SMMCALL_ENTRY             *mSmmCallTablePtr;
BOOLEAN                   mIfUsbBusScanTimerStarted = FALSE;
EFI_EVENT                 mUsbBusPeriodicEvent;
EFI_SMM_BASE2_PROTOCOL    *gSMM2 = NULL;
EFI_SMM_CONTROL2_PROTOCOL *mSmmControl;
UINT8                     mArgBufferSize = 1;
UINT8                     mArgBuffer     = SMMCALL_SMI_VALUE;

BOOLEAN
IfInSmm (
  VOID
  )
{
    BOOLEAN InSmm; 
    gSMM2->InSmm (gSMM2, &InSmm);
    return InSmm;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoControlTransfer (
  IN  EFI_USB_IO_PROTOCOL     *This,
  IN  EFI_USB_DEVICE_REQUEST  *Request,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT32                  Timeout,
  IN  OUT VOID                *Data,      OPTIONAL
  IN  UINTN                   DataLength, OPTIONAL
  OUT UINT32                  *UsbStatus
  )
/*++

Routine Description:

  USB_IO function to execute a control transfer. This 
  function will execute the USB transfer. If transfer
  successes, it will sync the internal state of USB bus
  with device state.

Arguments:

  This        - The USB_IO instance
  Request     - The control transfer request
  Direction   - Direction for data stage
  Timeout     - The time to wait before timeout
  Data        - The buffer holding the data
  DataLength  - Then length of the data
  UsbStatus   - USB result

Returns:

  EFI_INVALID_PARAMETER - The parameters are invalid
  EFI_SUCCESS           - The control transfer succeded.
  Others                - Failed to execute the transfer

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_7(UsbIoControlTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    Request,
    Direction,
    Timeout,
    Data,
    DataLength,
    UsbStatus
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoBulkTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               Endpoint,
  IN  OUT VOID            *Data,
  IN  OUT UINTN           *DataLength,
  IN  UINTN               Timeout,
  OUT UINT32              *UsbStatus
  )
/*++

Routine Description:

  Execute a bulk transfer to the device endpoint

Arguments:

  This            - The USB IO instance
  Endpoint        - The device endpoint
  Data            - The data to transfer
  DataLength      - The length of the data to transfer
  Timeout         - Time to wait before timeout
  UsbStatus       - The result of USB transfer
  
Returns:

  EFI_SUCCESS           - The bulk transfer is OK
  EFI_INVALID_PARAMETER - Some parameters are invalid
  Others                - Failed to execute transfer, reason returned in UsbStatus

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_6(UsbIoBulkTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    Endpoint,
    Data,
    DataLength,
    Timeout,
    UsbStatus
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoSyncInterruptTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               Endpoint,
  IN  OUT VOID            *Data,
  IN  OUT UINTN           *DataLength,
  IN  UINTN               Timeout,
  OUT UINT32              *UsbStatus
  )
/*++

Routine Description:

  Execute a synchronous interrupt transfer

Arguments:

  This            - The USB IO instance
  Endpoint        - The device endpoint
  Data            - The data to transfer
  DataLength      - The length of the data to transfer
  Timeout         - Time to wait before timeout
  UsbStatus       - The result of USB transfer

Returns:

  EFI_SUCCESS           - The synchronous interrupt transfer is OK
  EFI_INVALID_PARAMETER - Some parameters are invalid
  Others                - Failed to execute transfer, reason returned in UsbStatus

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_6(UsbIoSyncInterruptTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    Endpoint,
    Data,
    DataLength,
    Timeout,
    UsbStatus
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoAsyncInterruptTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            Endpoint,
  IN BOOLEAN                          IsNewTransfer,
  IN UINTN                            PollInterval,       OPTIONAL
  IN UINTN                            DataLength,         OPTIONAL
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,           OPTIONAL
  IN VOID                             *Context            OPTIONAL
  )
/*++

Routine Description:

  Queue a new asynchronous interrupt transfer, or remove the old
  request if (IsNewTransfer == FALSE)

Arguments:

  This          - The USB_IO instance
  Endpoint      - The device endpoint
  IsNewTransfer - Whether this is a new request, if it's old, remove the request
  PollInterval  - The interval to poll the transfer result, (in ms)
  DataLength    - The length of perodic data transfer
  Callback      - The function to call periodicaly when transfer is ready
  Context       - The context to the callback

Returns:

  EFI_SUCCESS           - New transfer is queued or old request is removed
  EFI_INVALID_PARAMETER - Some parameters are invalid
  Others                - Failed to queue the new request or remove the old request
  
--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_7(UsbIoAsyncInterruptTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    Endpoint,
    IsNewTransfer,
    PollInterval,
    DataLength,
    Callback,
    Context
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoIsochronousTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               DeviceEndpoint,
  IN  OUT VOID            *Data,
  IN  UINTN               DataLength,
  OUT UINT32              *UsbStatus
  )
/*++

Routine Description:

  Execute a synchronous isochronous transfer

Arguments:

  This            - The USB IO instance
  DeviceEndpoint  - The device endpoint
  Data            - The data to transfer
  DataLength      - The length of the data to transfer
  UsbStatus       - The result of USB transfer

Returns:

  EFI_UNSUPPORTED - Currently isochronous transfer isn't supported

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_5(UsbIoIsochronousTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    DeviceEndpoint,
    Data,
    DataLength,
    UsbStatus
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoAsyncIsochronousTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            DeviceEndpoint,
  IN OUT VOID                         *Data,
  IN UINTN                            DataLength,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  IsochronousCallBack,
  IN VOID                             *Context              OPTIONAL
  )
/*++

Routine Description:

  Queue an asynchronous isochronous transfer

Arguments:

  This                - The USB_IO instance
  DeviceEndpoint      - The device endpoint
  DataLength          - The length of perodic data transfer
  IsochronousCallBack - The function to call periodicaly when transfer is ready
  Context             - The context to the callback

Returns:

  EFI_UNSUPPORTED - Currently isochronous transfer isn't supported

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_6(UsbIoAsyncIsochronousTransfer, &Status, 
    UsbIf->LegacyUsbIo,
    DeviceEndpoint,
    Data,
    DataLength,
    IsochronousCallBack,
    Context
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetDeviceDescriptor (
  IN  EFI_USB_IO_PROTOCOL       *This,
  OUT EFI_USB_DEVICE_DESCRIPTOR *Descriptor
  )
/*++

Routine Description:

  Retrieve the device descriptor of the device

Arguments:

  This        - The USB IO instance
  Descriptor  - The variable to receive the device descriptor

Returns:

  EFI_SUCCESS           - The device descriptor is returned
  EFI_INVALID_PARAMETER - The parameter is invalid

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_2(UsbIoGetDeviceDescriptor, &Status, 
    UsbIf->LegacyUsbIo,
    Descriptor
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetActiveConfigDescriptor (
  IN  EFI_USB_IO_PROTOCOL       *This,
  OUT EFI_USB_CONFIG_DESCRIPTOR *Descriptor
  )
/*++

Routine Description:

  Return the configuration descriptor of the current active configuration

Arguments:

  This       - The USB IO instance
  Descriptor - The USB configuration descriptor

Returns:

  EFI_SUCCESS           - The active configuration descriptor is returned
  EFI_INVALID_PARAMETER - Some parameter is invalid
  EFI_NOT_FOUND         - Currently no active configuration is selected.

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_2(UsbIoGetActiveConfigDescriptor, &Status, 
    UsbIf->LegacyUsbIo,
    Descriptor
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor
  )
/*++

Routine Description:

  Retrieve the active interface setting descriptor for this USB IO instance

Arguments:

  This       - The USB IO instance
  Descriptor - The variable to receive active interface setting

Returns:

  EFI_SUCCESS           - The active interface setting is returned 
  EFI_INVALID_PARAMETER - Some parameter is invalid

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_2(UsbIoGetInterfaceDescriptor, &Status, 
    UsbIf->LegacyUsbIo,
    Descriptor
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL         *This,
  IN  UINT8                       Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR *Descriptor
  )
/*++

Routine Description:

  Retrieve the endpoint descriptor from this interface setting

Arguments:

  This        - The USB IO instance
  Index       - The index (start from zero) of the endpoint to retrieve
  Descriptor  - The variable to receive the descriptor

Returns:

  EFI_SUCCESS           - The endpoint descriptor is returned
  EFI_INVALID_PARAMETER - Some parameter is invalid

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_3(UsbIoGetEndpointDescriptor, &Status, 
    UsbIf->LegacyUsbIo,
    Index,
    Descriptor
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetSupportedLanguages (
  IN  EFI_USB_IO_PROTOCOL *This,
  OUT UINT16              **LangIDTable,
  OUT UINT16              *TableSize
  )
/*++

Routine Description:

  Retrieve the supported language ID table from the device

Arguments:

  This        - The USB IO instance
  LangIDTable - The table to return the language IDs
  TableSize   - The number of supported languanges

Returns:

  EFI_SUCCESS - The language ID is return

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_3(UsbIoGetSupportedLanguages, &Status, 
    UsbIf->LegacyUsbIo,
    LangIDTable,
    TableSize
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UsbIoGetStringDescriptor (
  IN  EFI_USB_IO_PROTOCOL   *This,
  IN  UINT16                LangID,
  IN  UINT8                 StringIndex,
  OUT CHAR16                **String
  )
/*++

Routine Description:

  Retrieve an indexed string in the language of LangID

Arguments:

  This        - The USB IO instance
  LangID      - The language ID of the string to retrieve
  StringIndex - The index of the string
  String      - The variable to receive the string

Returns:

  EFI_SUCCESS   - The string is returned
  EFI_NOT_FOUND - No such string existed

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_4(UsbIoGetStringDescriptor, &Status, 
    UsbIf->LegacyUsbIo,
    LangID,
    StringIndex,
    String
    );

  return Status;
}

EFI_STATUS
EFIAPI
UsbIoPortReset (
  IN EFI_USB_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  Reset the device, then if that succeeds, reconfigure the 
  device with its address and current active configuration.

Arguments:

  This  - The USB IO instance

Returns:

  EFI_SUCCESS - The device is reset and configured
  Others      - Failed to reset the device

--*/
{
  EFI_STATUS    Status = EFI_NOT_FOUND;
  NATIVE_USBIF  *UsbIf;

  UsbIf = USB_INTERFACE_FROM_USBIO(This);
  if (UsbIf->UsbIfState == EMPTY_INTERFACE) {
    return EFI_NOT_FOUND;
  }
  SMMCALL_1(UsbIoPortReset, &Status, 
    UsbIf->LegacyUsbIo
    );

  return Status;
}

EFI_STATUS
EFIAPI
UsbBusBuildProtocol (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  Install Usb Bus Protocol on host controller, and start the Usb bus

Arguments:

  This                - The USB bus driver binding instance
  Controller          - The controller to check
  RemainingDevicePath - The remaining device patch
  
Returns:

  EFI_SUCCESS           - The controller is controlled by the usb bus
  EFI_ALREADY_STARTED   - The controller is already controlled by the usb bus
  EFI_OUT_OF_RESOURCES  - Failed to allocate resources

--*/
{
  USB_BUS                 *UsbBus;
  EFI_STATUS              Status;
  EFI_STATUS              Status2;

  UsbBus = AllocateZeroPool (sizeof (USB_BUS));
  

  if (UsbBus == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UsbBus->Signature   = USB_BUS_SIGNATURE;
  UsbBus->HostHandle  = Controller;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbBus->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to open device path %r\n", Status));

    FreePool (UsbBus);
    return Status;
  }
  
  //
  // Get USB_HC2/USB_HC host controller protocol (EHCI/UHCI).
  // This is for backward compatbility with EFI 1.x. In UEFI
  // 2.x, USB_HC2 replaces USB_HC. We will open both USB_HC2
  // and USB_HC because EHCI driver will install both protocols
  // (for the same reason). If we don't consume both of them,
  // the unconsumed one may be opened by others.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &(UsbBus->Usb2Hc),
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  Status2 = gBS->OpenProtocol (
                   Controller,
                   &gEfiUsbHcProtocolGuid,
                   (VOID **) &(UsbBus->UsbHc),
                   This->DriverBindingHandle,
                   Controller,
                   EFI_OPEN_PROTOCOL_BY_DRIVER
                   );

  if (EFI_ERROR (Status) && EFI_ERROR (Status2)) {
    DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to open USB_HC/USB2_HC %r\n", Status));

    Status = EFI_DEVICE_ERROR;
    goto CLOSE_HC;
  }

  //
  // Install an EFI_USB_BUS_PROTOCOL to host controler to identify it.
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &mUsbBusProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbBus->BusId
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to install bus protocol %r\n", Status));
    goto CLOSE_HC;
  }

  return EFI_SUCCESS;
  
CLOSE_HC:
  if (UsbBus->Usb2Hc != NULL) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsb2HcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  }
  if (UsbBus->UsbHc != NULL) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbHcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  FreePool (UsbBus);

  DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to start bus driver %r\n", Status));
  return Status;
}

EFI_USB_IO_PROTOCOL mUsbIoProtocol = {
  UsbIoControlTransfer,
  UsbIoBulkTransfer,
  UsbIoAsyncInterruptTransfer,
  UsbIoSyncInterruptTransfer,
  UsbIoIsochronousTransfer,
  UsbIoAsyncIsochronousTransfer,
  UsbIoGetDeviceDescriptor,
  UsbIoGetActiveConfigDescriptor,
  UsbIoGetInterfaceDescriptor,
  UsbIoGetEndpointDescriptor,
  UsbIoGetStringDescriptor,
  UsbIoGetSupportedLanguages,
  UsbIoPortReset
};

VOID
EFIAPI
UsbBusPeriodicScan (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  EFI_STATUS        Status = EFI_NOT_FOUND;
  LIST_ENTRY       *Node;
  NATIVE_USBIF      *NativeUsbIf;

  SMMCALL_2(ScanUsbBus, &Status, &mNativeUsbIfList, sizeof (NATIVE_USBIF));

  if (Status != EFI_SUCCESS) {
    return;
  }

  Node = mNativeUsbIfList.ForwardLink;

  while (Node != &mNativeUsbIfList) {
    NativeUsbIf = _CR(Node, NATIVE_USBIF, List);
    Node = Node->ForwardLink;

    switch (NativeUsbIf->UsbIfState) {
      case EMPTY_INTERFACE:
        DEBUG ((EFI_D_ERROR, "  case EMPTY_INTERFACE\n"));
        Status = gBS->DisconnectController(NativeUsbIf->Handle, NULL, NULL);
        DEBUG ((EFI_D_ERROR, "    DisconnectController Status - %r\n", Status));

        Status = gBS->UninstallMultipleProtocolInterfaces (
           NativeUsbIf->Handle,
           &gEfiDevicePathProtocolGuid,
           NativeUsbIf->DevicePath,
           &gEfiUsbIoProtocolGuid,
           &NativeUsbIf->UsbIo,
           NULL
           );
        NativeUsbIf->UsbIfState = INVALID_INTERFACE;
        DEBUG ((EFI_D_ERROR, "    UninstallMultipleProtocolInterfaces Status - %r\n", Status));

        RemoveEntryList(&NativeUsbIf->List);
        break;

      case NEW_INTERFACE:
        DEBUG ((EFI_D_ERROR, "  case NEW_INTERFACE\n"));
        NativeUsbIf->Signature = USB_INTERFACE_SIGNATURE;
        CopyMem(&(NativeUsbIf->UsbIo), &mUsbIoProtocol, sizeof(EFI_USB_IO_PROTOCOL));
        NativeUsbIf->Handle = NULL;

        Status = gBS->InstallMultipleProtocolInterfaces (
                        &NativeUsbIf->Handle,
                        &gEfiDevicePathProtocolGuid,
                        NativeUsbIf->DevicePath,
                        &gEfiUsbIoProtocolGuid,
                        &NativeUsbIf->UsbIo,
                        NULL
                        );

        NativeUsbIf->UsbIfState = OLD_INTERFACE;
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "    InstallMultipleProtocolInterfaces - %r\n", Status));
          break;
        }        

        Status = gBS->ConnectController (NativeUsbIf->Handle, NULL, NULL, TRUE);
        DEBUG ((EFI_D_ERROR, "    ConnectController Status - %r\n", Status));
       break;
       
      default:
        break;
    }
  }
}

EFI_STATUS
EFIAPI
UsbBusControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  Check whether USB bus driver support this device

Arguments:

  This                - The USB bus driver binding protocol
  Controller          - The controller handle to test againist
  RemainingDevicePath - The remaining device path

Returns:

  EFI_SUCCESS     - The bus supports this controller.
  EFI_UNSUPPORTED - This device isn't supported

--*/
{
  EFI_DEV_PATH_PTR              DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL      *ParentDevicePath;
  EFI_USB2_HC_PROTOCOL          *Usb2Hc;
  EFI_USB_HC_PROTOCOL           *UsbHc;
  EFI_STATUS                    Status;
  EFI_LEGACY_USB_INF_PROTOCOL   *LegacyUsbInf;

  //
  // Check whether device path is valid
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node, 
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      DevicePathNode.DevPath = RemainingDevicePath;

      if ((DevicePathNode.DevPath->Type    != MESSAGING_DEVICE_PATH) ||
          (DevicePathNode.DevPath->SubType != MSG_USB_DP &&
           DevicePathNode.DevPath->SubType != MSG_USB_CLASS_DP
           && DevicePathNode.DevPath->SubType != MSG_USB_WWID_DP
           )) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Check whether USB_HC2 protocol is installed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsb2HcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    goto GetLegacyUsbInfProtocol;
  }

  //
  // If failed to open USB_HC2, fall back to USB_HC
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbHcProtocolGuid,
                  (VOID **) &UsbHc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbHcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    goto GetLegacyUsbInfProtocol;
  }

  return Status;

GetLegacyUsbInfProtocol:

  Status = gBS->LocateProtocol (
          &gEfiLegacyUsbInfProtocolGuid,
          NULL,
          &LegacyUsbInf
          );

  if (Status != EFI_SUCCESS) {
      return EFI_UNSUPPORTED;
  }

  mSmmCallComm = LegacyUsbInf->SmmCallCommBuf;
  mSmmCallTablePtr = LegacyUsbInf->SmmCallTablePtr;

  Status = gBS->LocateProtocol(&gEfiSmmControl2ProtocolGuid, NULL, &mSmmControl);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  Start to process the controller

Arguments:

  This                - The USB bus driver binding instance
  Controller          - The controller to check
  RemainingDevicePath - The remaining device patch

Returns:

  EFI_SUCCESS           - The controller is controlled by the usb bus
  EFI_ALREADY_STARTED   - The controller is already controlled by the usb bus
  EFI_OUT_OF_RESOURCES  - Failed to allocate resources

--*/
{
  EFI_USB_BUS_PROTOCOL          *UsbBusId;
  EFI_STATUS                    Status;


  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **) &mCpuIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status  = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, &gSMM2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  
  //
  // Locate the USB bus protocol, if it is found, USB bus
  // is already started on this controller.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &mUsbBusProtocolGuid,
                  (VOID **) &UsbBusId,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    //
    // If first start, build the bus execute enviorment and install bus protocol
    //
    Status = UsbBusBuildProtocol (This, Controller, RemainingDevicePath);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Try get the Usb Bus protocol interface again
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &mUsbBusProtocolGuid,
                    (VOID **) &UsbBusId,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT (!EFI_ERROR (Status));

  }

  if (!mIfUsbBusScanTimerStarted) {

    //
    // Create callback event and set timer
    //
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    UsbBusPeriodicScan,
                    NULL,
                    &mUsbBusPeriodicEvent
                    );

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // It should signal the event immediately here, or device detection
    // by bus enumeration might be delayed by the timer interval.
    //
    gBS->SignalEvent (mUsbBusPeriodicEvent);
    
    Status = gBS->SetTimer (
                    mUsbBusPeriodicEvent,
                    TimerPeriodic,
                    USB_ROOTHUB_POLL_INTERVAL
                    );

    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (mUsbBusPeriodicEvent);
      return EFI_DEVICE_ERROR;
    }
    
    mIfUsbBusScanTimerStarted = TRUE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbBusControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

  Stop handle the controller by this USB bus driver

Arguments:

  This              - The USB bus driver binding protocol
  Controller        - The controller to release
  NumberOfChildren  - The child of USB bus that opened controller BY_CHILD
  ChildHandleBuffer - The array of child handle

Returns:

  EFI_SUCCESS       - The controller or children are stopped
  EFI_DEVICE_ERROR  - Failed to stop the driver

--*/
{
  USB_BUS               *Bus;
  EFI_USB_BUS_PROTOCOL  *BusId;
  EFI_USB_IO_PROTOCOL   *UsbIo;
  EFI_TPL               OldTpl;
  UINTN                 Index;
  EFI_STATUS            Status;

  Status  = EFI_SUCCESS;
  
  //
  // Close the Usb Bus Periodic Event
  //
  if (mIfUsbBusScanTimerStarted) {
    gBS->CloseEvent (mUsbBusPeriodicEvent);
    mIfUsbBusScanTimerStarted = FALSE;
  }
  
  if (NumberOfChildren > 0) {
    //
    // BugBug: Raise TPL to callback level instead of USB_BUS_TPL to avoid TPL conflict
    //
    OldTpl   = gBS->RaiseTPL (TPL_CALLBACK);

    for (Index = 0; Index < NumberOfChildren; Index++) {
      Status = gBS->OpenProtocol (
                      ChildHandleBuffer[Index],
                      &gEfiUsbIoProtocolGuid,
                      (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );

      if (EFI_ERROR (Status)) {
        //
        // It is possible that the child has already been released:
        // 1. For combo device, free one device will release others.
        // 2. If a hub is released, all devices on its down facing
        //    ports are released also.
        //
        continue;
      }
    }

    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
  }

  //
  // Locate USB_BUS for the current host controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &mUsbBusProtocolGuid,
                  (VOID **) &BusId,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bus = USB_BUS_FROM_THIS (BusId);

  //
  // Uninstall the bus identifier and close USB_HC/USB2_HC protocols
  //
  gBS->UninstallProtocolInterface (Controller, &mUsbBusProtocolGuid, &Bus->BusId);

  if (Bus->Usb2Hc != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiUsb2HcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  if (Bus->UsbHc != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Bus);

  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL mUsbBusDriverBinding = {
  UsbBusControllerDriverSupported,
  UsbBusControllerDriverStart,
  UsbBusControllerDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
UsbBusDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  The USB bus driver entry pointer

Arguments:

  ImageHandle - The driver image handle
  SystemTable - The system table

Returns:

  EFI_SUCCESS - The component name protocol is installed
  Others      - Failed to init the usb driver

--*/
{
  InitializeListHead(&mNativeUsbIfList);

  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mUsbBusDriverBinding,
           ImageHandle,
           &gUsbBusComponentName,
           &gUsbBusComponentName2
           );

}


