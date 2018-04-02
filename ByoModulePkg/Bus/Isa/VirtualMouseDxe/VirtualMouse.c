/** @file
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2012 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "VirtualMouse.h"
#include "CommPs2.h"

EFI_GUID  gEfiVirtualIsaIoProtocolGuid = { 0x893e441f, 0x5a6a, 0x40d1, { 0x83, 0xcb, 0x8a, 0xa1, 0x6, 0xdd, 0xd7, 0xab } };

///
/// DriverBinding Protocol Instance
///
EFI_DRIVER_BINDING_PROTOCOL gVirtualMouseDriver = {
  VirtualMouseDriverSupported,
  VirtualMouseDriverStart,
  VirtualMouseDriverStop,
  0xa,
  NULL,
  NULL
};

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a IsaIo protocol can be supported.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
VirtualMouseDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiVirtualIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiVirtualIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Start this driver on ControllerHandle by opening a IsaIo protocol, creating 
  PS2_MOUSE_ABSOLUTE_POINTER_DEV device and install gEfiAbsolutePointerProtocolGuid
  finally.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
VirtualMouseDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_STATUS                          EmptyStatus;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  PS2_MOUSE_DEV                       *MouseDev;
  UINT8                               Data;
  EFI_TPL                             OldTpl;
  //EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;

  MouseDev    = NULL;
  //
  // Raise TPL to avoid keyboard operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  Status = gBS->LocateProtocol (&gEfiVirtualIsaIoProtocolGuid, NULL, (VOID **) &IsaIo);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Allocate private data
  //
  MouseDev = AllocateZeroPool (sizeof (PS2_MOUSE_DEV));
  if (MouseDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Setup the device instance
  //
  MouseDev->Signature       = PS2_MOUSE_DEV_SIGNATURE;
  MouseDev->Handle          = Controller;
  MouseDev->SampleRate      = SampleRate20;
  MouseDev->Resolution      = MouseResolution4;
  MouseDev->Scaling         = Scaling1;
  MouseDev->DataPackageSize = 3;
  MouseDev->IsaIo           = IsaIo;
  //MouseDev->DevicePath      = ParentDevicePath;

  //
  // Resolution = 4 counts/mm
  //
  MouseDev->Mode.ResolutionX                = 4;
  MouseDev->Mode.ResolutionY                = 4;
  MouseDev->Mode.LeftButton                 = TRUE;
  MouseDev->Mode.RightButton                = TRUE;

  MouseDev->SimplePointerProtocol.Reset     = MouseReset;
  MouseDev->SimplePointerProtocol.GetState  = MouseGetState;
  MouseDev->SimplePointerProtocol.Mode      = &(MouseDev->Mode);

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  MouseWaitForInput,
                  MouseDev,
                  &((MouseDev->SimplePointerProtocol).WaitForInput)
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Setup a periodic timer, used to poll mouse state
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PollMouse,
                  MouseDev,
                  &MouseDev->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Start timer to poll mouse (100 samples per second)
  //
  Status = gBS->SetTimer (MouseDev->TimerEvent, TimerPeriodic, 100000);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  MouseDev->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gVirtualMouseComponentName.SupportedLanguages,
    &MouseDev->ControllerNameTable,
    L"PS/2 Mouse Device",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gVirtualMouseComponentName2.SupportedLanguages,
    &MouseDev->ControllerNameTable,
    L"PS/2 Mouse Device",
    FALSE
    );


  //
  // Install protocol interfaces for the mouse device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &MouseDev->SimplePointerProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;

ErrorExit:

  if ((MouseDev != NULL) && (MouseDev->SimplePointerProtocol.WaitForInput != NULL)) {
    gBS->CloseEvent (MouseDev->SimplePointerProtocol.WaitForInput);
  }

  if ((MouseDev != NULL) && (MouseDev->TimerEvent != NULL)) {
    gBS->CloseEvent (MouseDev->TimerEvent);
  }

  if ((MouseDev != NULL) && (MouseDev->ControllerNameTable != NULL)) {
    FreeUnicodeStringTable (MouseDev->ControllerNameTable);
  }
  
  if (Status != EFI_DEVICE_ERROR) {
    //
    // Since there will be no timer handler for mouse input any more,
    // exhaust input data just in case there is still mouse data left
    //
    EmptyStatus = EFI_SUCCESS;
    while (!EFI_ERROR (EmptyStatus)) {
      EmptyStatus = In8042Data (IsaIo, &Data);
    }
  }

  if (MouseDev != NULL) {
    FreePool (MouseDev);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
VirtualMouseDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointerProtocol;
  PS2_MOUSE_DEV               *MouseDev;
  UINT8                       Data;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  (VOID **) &SimplePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (SimplePointerProtocol);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &MouseDev->SimplePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Cancel mouse data polling timer, close timer event
  //
  gBS->SetTimer (MouseDev->TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (MouseDev->TimerEvent);

  //
  // Since there will be no timer handler for mouse input any more,
  // exhaust input data just in case there is still mouse data left
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseDev->IsaIo, &Data);
  }

  gBS->CloseEvent (MouseDev->SimplePointerProtocol.WaitForInput);
  FreeUnicodeStringTable (MouseDev->ControllerNameTable);
  FreePool (MouseDev);

  return EFI_SUCCESS;
}

/**
  Reset the Mouse and do BAT test for it, if ExtendedVerification isTRUE and there is a mouse device connectted to system

  @param This                 - Pointer of simple pointer Protocol.
  @param ExtendedVerification - Whether configure mouse parameters. True: do; FALSE: skip.


  @retval EFI_SUCCESS         - The command byte is written successfully.
  @retval EFI_DEVICE_ERROR    - Errors occurred during reseting keyboard.

**/
EFI_STATUS
EFIAPI
MouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  EFI_STATUS    Status;
  PS2_MOUSE_DEV *MouseDev;
  EFI_TPL       OldTpl;
  BOOLEAN       KeyboardEnable;
  UINT8         Data;

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (This);

  //
  // Report reset progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET,
    MouseDev->DevicePath
    );

  KeyboardEnable = FALSE;

  //
  // Raise TPL to avoid keyboard operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  ZeroMem (&MouseDev->State, sizeof (EFI_SIMPLE_POINTER_STATE));
  MouseDev->StateChanged = FALSE;

  //
  // Exhaust input data
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseDev->IsaIo, &Data);
  }

  CheckKbStatus (MouseDev->IsaIo, &KeyboardEnable);

  KbcDisableKb (MouseDev->IsaIo);

  MouseDev->IsaIo->Io.Read (MouseDev->IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

  //
  // if there's data block on KBC data port, read it out
  //
  if ((Data & KBC_OUTB) == KBC_OUTB) {
    MouseDev->IsaIo->Io.Read (MouseDev->IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Data);
  }

  Status = EFI_SUCCESS;
  //
  // The PS2 mouse driver reset behavior is always successfully return no matter wheater or not there is mouse connected to system.
  // This behavior is needed by performance speed. The following mouse command only succeessfully finish when mouse device is
  // connected to system, so if PS2 mouse device not connect to system or user not ask for, we skip the mouse configuration and enabling
  //
  if (ExtendedVerification && CheckMouseConnect (MouseDev)) {
    //
    // Send mouse reset command and set mouse default configure
    //
    Status = PS2MouseReset (MouseDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetSampleRate (MouseDev->IsaIo, MouseDev->SampleRate);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetResolution (MouseDev->IsaIo, MouseDev->Resolution);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetScaling (MouseDev->IsaIo, MouseDev->Scaling);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseEnable (MouseDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }
Exit:
  gBS->RestoreTPL (OldTpl);

  if (KeyboardEnable) {
    KbcEnableKb (MouseDev->IsaIo);
  }

  return Status;
}

/**
  Check whether there is Ps/2 mouse device in system

  @param MouseDev   - Mouse Private Data Structure

  @retval TRUE      - Keyboard in System.
  @retval FALSE     - Keyboard not in System.

**/
BOOLEAN
CheckMouseConnect (
  IN  PS2_MOUSE_DEV     *MouseDev
  )

{
  EFI_STATUS     Status;

  Status = PS2MouseEnable (MouseDev->IsaIo);
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Get and Clear mouse status.
  
  @param This                 - Pointer of simple pointer Protocol.
  @param State                - Output buffer holding status.

  @retval EFI_INVALID_PARAMETER Output buffer is invalid.
  @retval EFI_NOT_READY         Mouse is not changed status yet.
  @retval EFI_SUCCESS           Mouse status is changed and get successful.
**/
EFI_STATUS
EFIAPI
MouseGetState (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN OUT EFI_SIMPLE_POINTER_STATE   *State
  )
{
  PS2_MOUSE_DEV *MouseDev;
  EFI_TPL       OldTpl;

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (This);

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!MouseDev->StateChanged) {
    return EFI_NOT_READY;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  CopyMem (State, &(MouseDev->State), sizeof (EFI_SIMPLE_POINTER_STATE));

  //
  // clear mouse state
  //
  MouseDev->State.RelativeMovementX = 0;
  MouseDev->State.RelativeMovementY = 0;
  MouseDev->State.RelativeMovementZ = 0;
  MouseDev->StateChanged            = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**

  Event notification function for SIMPLE_POINTER.WaitForInput event.
  Signal the event if there is input from mouse.

  @param Event    event object
  @param Context  event context

**/
VOID
EFIAPI
MouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  PS2_MOUSE_DEV *MouseDev;

  MouseDev = (PS2_MOUSE_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (MouseDev->StateChanged) {
    gBS->SignalEvent (Event);
  }

}

/**
  Event notification function for TimerEvent event.
  If mouse device is connected to system, try to get the mouse packet data.

  @param Event      -  TimerEvent in PS2_MOUSE_DEV
  @param Context    -  Pointer to PS2_MOUSE_DEV structure

**/
VOID
EFIAPI
PollMouse (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )

{
  PS2_MOUSE_DEV *MouseDev;

  MouseDev = (PS2_MOUSE_DEV *) Context;

  //
  // Polling mouse packet data
  //
  PS2MouseGetPacket (MouseDev);
}

/**
  The user Entry Point for module Ps2Mouse. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeVirtualMouse(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gVirtualMouseDriver,
             ImageHandle,
             &gVirtualMouseComponentName,
             &gVirtualMouseComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

