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
  LegacyFreeMs.c

Abstract:


Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "PiDxe.h"
#include <Protocol/UsbPolicy.h>
#include "LegacyFreeUsbMs.h"

#include <industrystandard/Usb.h>
#include <Protocol/UsbIo.h>

#include "mousehid.h"

#define EBDA_MOUSE_DATA_BUFFER   0x28

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );

EFI_GUID  gEfiLegacyFreeMsDriverGuid = {
  0x290156b5, 0x6a05, 0x4ac0, 0xb8, 0x0, 0x51, 0x27, 0x55, 0xad, 0x14, 0x29
};

EFI_DRIVER_BINDING_PROTOCOL gLegacyFreeMsDriverBinding = {
  LegacyFreeMsDriverBindingSupported,
  LegacyFreeMsDriverBindingStart,
  LegacyFreeMsDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// helper functions
//
STATIC
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  );

STATIC
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_DEV           *UsbMouseDev
  );

STATIC
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Mouse Protocol
//
STATIC
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  );

STATIC
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

//
// Driver start here
//

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

  Routine Description:
    Entry point for EFI drivers.

  Arguments:
   ImageHandle - EFI_HANDLE
   SystemTable - EFI_SYSTEM_TABLE
  Returns:
    EFI_SUCCESS
    others

--*/
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gLegacyFreeMsDriverBinding,
             ImageHandle,
             &gLegacyFreeMsComponentName,
             &gLegacyFreeMsComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
          
}

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle. Any ControllerHandle
    that has UsbHcProtocol installed will be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller         - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS          OpenStatus;
  EFI_USB_IO_PROTOCOL *UsbIo;
  EFI_STATUS          Status;

  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                      &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus) && (OpenStatus != EFI_ALREADY_STARTED)) {
    return EFI_UNSUPPORTED;
  }

  if (OpenStatus == EFI_ALREADY_STARTED) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Use the USB I/O protocol interface to see the Controller is
  // the Mouse controller that can be managed by this driver.
  //
  Status = EFI_SUCCESS;
  if (!IsUsbMouse (UsbIo)) {
     Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );
  return Status;
}

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Starting the Usb Bus Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
                          Error
    EFI_OUT_OF_RESOURCES- Can't allocate memory resources
    EFI_ALREADY_STARTED - Thios driver has been started
--*/
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDesc;
  USB_MOUSE_DEV                 *UsbMouseDevice;
  EFI_ISA_IO_PROTOCOL           *IsaIo;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINTN                         Index;

  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiIsaIoProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NumberOfHandles; Index++) {
      Status = gBS->HandleProtocol (
                       HandleBuffer[Index],
                       &gEfiIsaIoProtocolGuid,
                       (VOID**) &IsaIo
                       );
      if (!EFI_ERROR (Status)) {
        if (IsaIo->ResourceList->Device.HID == EISA_PNP_ID (0x303)) {
          DEBUG((EFI_D_ERROR,"Has KBC, no need legacyFree support\n"));
          return EFI_SUCCESS;
        }
      }
    }
  }

  UsbMouseDevice  = NULL;
  Status          = EFI_SUCCESS;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbMouseDevice = AllocateZeroPool (sizeof (USB_MOUSE_DEV));
  if (UsbMouseDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbMouseDevice->UsbIo               = UsbIo;

  UsbMouseDevice->Signature           = USB_MOUSE_DEV_SIGNATURE;

  UsbMouseDevice->InterfaceDescriptor = AllocatePool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));
  if (UsbMouseDevice->InterfaceDescriptor == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  EndpointDesc = AllocatePool (sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
  if (EndpointDesc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbMouseDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = InitializeUsbMouseDevice (UsbMouseDevice);
  if (EFI_ERROR (Status)) {
    //
    // Fail to initialize USB mouse device.
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR),
      UsbMouseDevice->DevicePath
      );
    goto ErrorExit;
  }

  UsbMouseDevice->SimplePointerProtocol.GetState  = GetMouseState;
  UsbMouseDevice->SimplePointerProtocol.Reset     = UsbMouseReset;
  UsbMouseDevice->SimplePointerProtocol.Mode      = &UsbMouseDevice->Mode;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UsbMouseWaitForInput,
                  UsbMouseDevice,
                  &((UsbMouseDevice->SimplePointerProtocol).WaitForInput)
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSimplePointerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbMouseDevice->SimplePointerProtocol
                  );

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorExit;
  }

  UsbMouseDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gLegacyFreeMsComponentName.SupportedLanguages,
    &UsbMouseDevice->ControllerNameTable,
    L"Generic Usb Mouse",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gLegacyFreeMsComponentName2.SupportedLanguages,
    &UsbMouseDevice->ControllerNameTable,
    L"Generic Usb Mouse",
    FALSE
    );
    

  //
  // After Enabling Async Interrupt Transfer on this mouse Device
  // we will be able to get key data from it. Thus this is deemed as
  // the enable action of the mouse
  //

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE),
    UsbMouseDevice->DevicePath
    );

  return EFI_SUCCESS;
ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseDevice != NULL) {
      if (UsbMouseDevice->InterfaceDescriptor != NULL) {
        FreePool (UsbMouseDevice->InterfaceDescriptor);
      }

      if (UsbMouseDevice->IntEndpointDescriptor != NULL) {
        FreePool (UsbMouseDevice->IntEndpointDescriptor);
      }

      if ((UsbMouseDevice->SimplePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseDevice->SimplePointerProtocol).WaitForInput);
      }

      FreePool (UsbMouseDevice);
      UsbMouseDevice = NULL;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
LegacyFreeMsDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop this driver on ControllerHandle. Support stoping any child handles
    created by this driver.

  Arguments:
    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    others

--*/
{
  EFI_STATUS                  Status;
  USB_MOUSE_DEV               *UsbMouseDevice;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointerProtocol;
  EFI_USB_IO_PROTOCOL         *UsbIo;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &SimplePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbMouseDevice = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (SimplePointerProtocol);

  gBS->CloseProtocol (
        Controller,
        &gEfiSimplePointerProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  UsbIo = UsbMouseDevice->UsbIo;

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE),
    UsbMouseDevice->DevicePath
    );

  gBS->CloseEvent (UsbMouseDevice->SimplePointerProtocol.WaitForInput);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &UsbMouseDevice->SimplePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  FreePool (UsbMouseDevice->InterfaceDescriptor);
  FreePool (UsbMouseDevice->IntEndpointDescriptor);

  if (UsbMouseDevice->ControllerNameTable) {
    FreeUnicodeStringTable (UsbMouseDevice->ControllerNameTable);
  }

  FreePool (UsbMouseDevice);

  return EFI_SUCCESS;

}

BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  )
/*++

  Routine Description:
    Tell if a Usb Controller is a mouse

  Arguments:
    UsbIo              - Protocol instance pointer.

  Returns:
    TRUE              - It is a mouse
    FALSE             - It is not a mouse
--*/
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the Default interface descriptor, now we only
  // suppose it is interface 1
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((InterfaceDescriptor.InterfaceClass == CLASS_HID) &&
      (InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT) &&
      (InterfaceDescriptor.InterfaceProtocol == PROTOCOL_MOUSE)
      ) {
    return TRUE;
  }

  return FALSE;
}

STATIC
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_DEV           *UsbMouseDev
  )
/*++

  Routine Description:
    Initialize the Usb Mouse Device.

  Arguments:
    UsbMouseDev         - Device instance to be initialized

  Returns:
    EFI_SUCCESS         - Success
    EFI_DEVICE_ERROR    - Init error.
    EFI_OUT_OF_RESOURCES- Can't allocate memory
--*/
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   Protocol;
  EFI_STATUS              Status;
  EFI_USB_HID_DESCRIPTOR  MouseHidDesc;
  UINT8                   *ReportDesc;

  UsbIo = UsbMouseDev->UsbIo;

  //
  // Get HID descriptor
  //
  Status = UsbGetHidDescriptor (
            UsbIo,
            UsbMouseDev->InterfaceDescriptor->InterfaceNumber,
            &MouseHidDesc
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Report descriptor
  //
  if (MouseHidDesc.HidClassDesc[0].DescriptorType != 0x22) {
    return EFI_UNSUPPORTED;
  }

  ReportDesc = AllocateZeroPool (MouseHidDesc.HidClassDesc[0].DescriptorLength);
  if (ReportDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UsbGetReportDescriptor (
            UsbIo,
            UsbMouseDev->InterfaceDescriptor->InterfaceNumber,
            MouseHidDesc.HidClassDesc[0].DescriptorLength,
            ReportDesc
            );

  if (EFI_ERROR (Status)) {
    FreePool (ReportDesc);
    return Status;
  }

  //
  // Parse report descriptor
  //
  Status = ParseMouseReportDescriptor (
            UsbMouseDev,
            ReportDesc,
            MouseHidDesc.HidClassDesc[0].DescriptorLength
            );

  if (EFI_ERROR (Status)) {
    FreePool (ReportDesc);
    return Status;
  }

  if (UsbMouseDev->NumberOfButtons >= 1) {
    UsbMouseDev->Mode.LeftButton = TRUE;
  }

  if (UsbMouseDev->NumberOfButtons > 1) {
    UsbMouseDev->Mode.RightButton = TRUE;
  }

  UsbMouseDev->Mode.ResolutionX = 8;
  UsbMouseDev->Mode.ResolutionY = 8;
  UsbMouseDev->Mode.ResolutionZ = 0;
  //
  // Here we just assume interface 0 is the mouse interface
  //
  UsbGetProtocolRequest (
    UsbIo,
    0,
    &Protocol
    );

  if (Protocol != BOOT_PROTOCOL) {
    Status = UsbSetProtocolRequest (
              UsbIo,
              0,
              BOOT_PROTOCOL
              );

    if (EFI_ERROR (Status)) {
      FreePool (ReportDesc);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Set indefinite Idle rate for USB Mouse
  //
  UsbSetIdleRequest (
    UsbIo,
    0,
    0,
    0
    );

  FreePool (ReportDesc);

  return EFI_SUCCESS;
}

BOOLEAN
MouseStateChanged(
  VOID
)
{
  UINT32                         EbdaPtr;

  EbdaPtr = (UINT32 )(*(UINT16 *)(UINTN)(0x400+0x0E) << 4);
  return ((*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER)) \
         |(*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 1)) \
         |(*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 2)) \
         |(*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 3)));
}

STATIC
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  )
/*++

  Routine Description:
    Get the mouse state, see SIMPLE POINTER PROTOCOL.

  Arguments:
    This              - Protocol instance pointer.
    MouseState        - Current mouse state

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_NOT_READY

--*/
{
  USB_MOUSE_DEV *MouseDev;
  UINT32                         EbdaPtr;

  if (MouseState == NULL) {
    return EFI_DEVICE_ERROR;
  }

  MouseDev = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);
  if(!MouseStateChanged()){
    return EFI_NOT_READY;
  }

  EbdaPtr = (UINT32 )(*(UINT16 *)(UINTN)(0x400+0x0E) << 4);
  //
  // Check mouse Data
  //
  MouseDev->State.LeftButton  = (UINT8)(((*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER)) & 0x01)?TRUE:FALSE);
  MouseDev->State.RightButton = (UINT8)(((*(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER)) & 0x02)?TRUE:FALSE);
  MouseDev->State.RelativeMovementX += *(INT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 1);
  MouseDev->State.RelativeMovementY -=  *(INT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 2);

  CopyMem (
    MouseState,
    &MouseDev->State,
    sizeof (EFI_SIMPLE_POINTER_STATE)
    );

  //
  // Clear previous move state
  //
  MouseDev->State.LeftButton  = 0;
  MouseDev->State.RightButton = 0;

  MouseDev->State.RelativeMovementX = 0;
  MouseDev->State.RelativeMovementY = 0;
  MouseDev->State.RelativeMovementZ = 0;

  *(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER) = 0;
  *(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 1) = 0;
  *(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 2) = 0;
  *(UINT8 *)(UINTN)(EbdaPtr + EBDA_MOUSE_DATA_BUFFER + 3) = 0;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
/*++

  Routine Description:
    Reset the mouse device, see SIMPLE POINTER PROTOCOL.

  Arguments:
    This                  - Protocol instance pointer.
    ExtendedVerification  - Ignored here/

  Returns:
    EFI_SUCCESS

--*/
{
  USB_MOUSE_DEV       *UsbMouseDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;

  UsbMouseDevice  = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);

  UsbIo           = UsbMouseDevice->UsbIo;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET),
    UsbMouseDevice->DevicePath
    );

  ZeroMem (
    &UsbMouseDevice->State,
    sizeof (EFI_SIMPLE_POINTER_STATE)
    );
  UsbMouseDevice->StateChanged = FALSE;

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  Event notification function for SIMPLE_POINTER.WaitForInput event
  Signal the event if there is input from mouse

Arguments:
  Event    - Wait Event
  Context  - Passed parameter to event handler
Returns:
  VOID
--*/
{
  USB_MOUSE_DEV *UsbMouseDev;

  UsbMouseDev = (USB_MOUSE_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (MouseStateChanged()) {
    gBS->SignalEvent (Event);
  }
}

