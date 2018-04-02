/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Ehci.c

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


#include "PiDxe.h"
#include <Protocol/SmmBase2.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/LegacyUsbInf.h>
#include <Protocol/UsbPolicy.h>
#include <Protocol/SmmControl2.h>

#include "Ehci.h"

EFI_CPU_IO2_PROTOCOL      *mCpuIo;
volatile USB_SMMCALL_COMM *mSmmCallComm;
SMMCALL_ENTRY             *mSmmCallTablePtr;
EFI_SMM_BASE2_PROTOCOL    *gSMM;
EFI_SMM_CONTROL2_PROTOCOL *mSmmControl;
UINT8                     mArgBufferSize = 1;
UINT8                     mArgBuffer     = SMMCALL_SMI_VALUE;

BOOLEAN
IfInSmm (
  VOID
  )
{
  BOOLEAN InSmm;
  gSMM->InSmm (gSMM, &InSmm);
  return InSmm;
}


STATIC
EFI_STATUS
EFIAPI
GetEhcInfo (
  USB2_HC_DEV *Hc
  )
{
  EFI_PCI_FUNCTION_ADDRESS  PciAddress;
  EFI_STATUS                Status;
  UINTN                     SegmentNumber;
  UINTN                     BusNumber;
  UINTN                     DeviceNumber;
  UINTN                     FunctionNumber;
  
  Status = Hc->PciIo->GetLocation (
                        Hc->PciIo,
                        &SegmentNumber, 
                        &BusNumber, 
                        &DeviceNumber, 
                        &FunctionNumber
                        );
  if (!EFI_ERROR (Status)) {
    PciAddress.Bus      =  (UINT16)BusNumber;
    PciAddress.Device   =  (UINT16)DeviceNumber;
    PciAddress.Function =  (UINT16)FunctionNumber;
    SMMCALL_3(GetEhcInfo, &Status, &PciAddress, &Hc->SmmUsb2HcDev, &Hc->SmmUsb2Hc);
   }
 
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
EhcGetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
/*++

  Routine Description:
    Retrieves the capablility of root hub ports.

  Arguments:
    This            - This EFI_USB_HC_PROTOCOL instance.
    MaxSpeed        - Max speed supported by the controller
    PortNumber      - Number of the root hub ports.
    Is64BitCapable  - Whether the controller supports 64-bit memory addressing.

  Returns:
    EFI_SUCCESS           : host controller capability were retrieved successfully.
    EFI_INVALID_PARAMETER : Either of the three capability pointer is NULL

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_4(
    EhcGetCapability, 
    &Ret,
    Ehc->SmmUsb2Hc,
    MaxSpeed,
    PortNumber,
    Is64BitCapable
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcReset (
  IN CONST EFI_USB2_HC_PROTOCOL *This,
  IN UINT16               Attributes
  )
/*++

  Routine Description:
    Provides software reset for the USB host controller.

  Arguments:

    This        - This EFI_USB2_HC_PROTOCOL instance.
    Attributes  - A bit mask of the reset operation to perform.

  Returns:
    EFI_SUCCESS           : The reset operation succeeded.
    EFI_INVALID_PARAMETER : Attributes is not valid.
    EFI_UNSUPPOURTED      : The type of reset specified by Attributes is
                            not currently supported by the host controller.
    EFI_DEVICE_ERROR      : Host controller isn't halted to reset.

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_2(EhcReset, &Ret,
    Ehc->SmmUsb2Hc,
    Attributes
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcGetState (
  IN  CONST EFI_USB2_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE      *State
  )
/*++

  Routine Description:
    Retrieve the current state of the USB host controller.

  Arguments:
    This    - This EFI_USB2_HC_PROTOCOL instance.
    State   - Variable to return the current host controller state.

  Returns:
    EFI_SUCCESS           : Host controller state was returned in State.
    EFI_INVALID_PARAMETER : State is NULL.
    EFI_DEVICE_ERROR      : An error was encountered while attempting to
                            retrieve the host controller's current state.
--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_2(EhcGetState, &Ret,
    Ehc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcSetState (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN EFI_USB_HC_STATE     State
  )
/*++

  Routine Description:

    Sets the USB host controller to a specific state.

  Arguments:

    This     - This EFI_USB2_HC_PROTOCOL instance.
    State    - The state of the host controller that will be set.

  Returns:

    EFI_SUCCESS           : The USB host controller was successfully placed
                            in the state specified by State.
    EFI_INVALID_PARAMETER : State is invalid.
    EFI_DEVICE_ERROR      : Failed to set the state due to device error.

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_2(EhcSetState, &Ret,
    Ehc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcGetRootHubPortStatus (
  IN  CONST EFI_USB2_HC_PROTOCOL  *This,
  IN  CONST UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  )
/*++

  Routine Description:
    Retrieves the current status of a USB root hub port.

  Arguments:
    This        - This EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - The root hub port to retrieve the state from.
                  This value is zero-based.
    PortStatus  - Variable to receive the port state

  Returns:
    EFI_SUCCESS           : The status of the USB root hub port specified
                            by PortNumber was returned in PortStatus.
    EFI_INVALID_PARAMETER : PortNumber is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_3(EhcGetRootHubPortStatus, &Ret,
    Ehc->SmmUsb2Hc,
    PortNumber,
    PortStatus
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcSetRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  )
/*++

  Routine Description:

    Sets a feature for the specified root hub port.

  Arguments:

    This        - This EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Root hub port to set.
    PortFeature - Feature to set

  Returns:

    EFI_SUCCESS           : The feature specified by PortFeature was set
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register
--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_3(EhcSetRootHubPortFeature, &Ret,
    Ehc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcClearRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  )
/*++

  Routine Description:

    Clears a feature for the specified root hub port.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be cleared.
    PortFeature - Indicates the feature selector associated with the
                  feature clear request.

  Returns:

    EFI_SUCCESS           : The feature specified by PortFeature was cleared
                            for the USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_3(EhcClearRootHubPortFeature, &Ret,
    Ehc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcControlTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              TransferDirection,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++

  Routine Description:

    Submits control transfer to a target USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - The target device address
    DeviceSpeed         - Target device speed.
    MaximumPacketLength - Maximum packet size the default control transfer
                          endpoint is capable of sending or receiving.
    Request             - USB device request to send
    TransferDirection   - Specifies the data direction for the data stage
    Data                - Data buffer to be transmitted or received from USB device.
    DataLength          - The size (in bytes) of the data buffer
    TimeOut             - Indicates the maximum timeout, in millisecond,
    Translator          - Transaction translator to be used by this device.
    TransferResult      - Return the result of this control transfer.

  Returns:

    EFI_SUCCESS           : Transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resources.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Transfer failed due to timeout.
    EFI_DEVICE_ERROR      : Transfer failed due to host controller or device error.

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_11(EhcControlTransfer, &Ret,
    Ehc->SmmUsb2Hc,
    DeviceAddress,
    DeviceSpeed,
    MaximumPacketLength,
    Request,
    TransferDirection,
    Data,
    DataLength,
    TimeOut,
    Translator,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcBulkTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++

  Routine Description:

    Submits bulk transfer to a bulk endpoint of a USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction in bit 7. .
    DeviceSpeed         - Device speed, Low speed device doesn't support
                          bulk transfer.
    MaximumPacketLength - Maximum packet size the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data to transmit
                          from or receive into.
    DataLength          - The lenght of the data buffer
    DataToggle          - On input, the initial data toggle for the transfer;
                          On output, it is updated to to next data toggle to use
                          of the subsequent bulk transfer.
    Translator          - A pointr to the transaction translator data.
    TimeOut             - Indicates the maximum time, in millisecond, which the
                          transfer is allowed to complete.
    TransferResult      - A pointer to the detailed result information of the
                          bulk transfer.

  Returns:

    EFI_SUCCESS           : The transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : The transfer failed due to timeout.
    EFI_DEVICE_ERROR      : The transfer failed due to host controller error.

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_12(EhcBulkTransfer, &Ret,
    Ehc->SmmUsb2Hc,
    DeviceAddress,
    EndPointAddress,
    DeviceSpeed,
    MaximumPacketLength,
    DataBuffersNumber,
    Data,
    DataLength,
    DataToggle,
    TimeOut,
    Translator,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcAsyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  * This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    * Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  )
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device.

  Arguments:
    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction encoded in bit 7
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Maximum packet size the target endpoint is capable
    IsNewTransfer       - If TRUE, to submit an new asynchronous interrupt transfer
                          If FALSE, to remove the specified asynchronous interrupt
    DataToggle          - On input, the initial data toggle to use;
                          on output, it is updated to indicate the next data toggle
    PollingInterval     - The he interval, in milliseconds, that the transfer is polled.
    DataLength          - The length of data to receive at the rate specified by
                          PollingInterval.
    Translator          - Transaction translator to use.
    CallBackFunction    - Function to call at the rate specified by PollingInterval
    Context             - Context to CallBackFunction.

  Returns:

    EFI_SUCCESS           : The request has been successfully submitted or canceled.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : The request failed due to a lack of resources.
    EFI_DEVICE_ERROR      : The transfer failed due to host controller error.

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_12(EhcAsyncInterruptTransfer, &Ret,
    Ehc->SmmUsb2Hc,
    DeviceAddress,
    EndPointAddress,
    DeviceSpeed,
    MaximumPacketLength,
    IsNewTransfer,
    DataToggle,
    PollingInterval,
    DataLength,
    Translator,
    CallBackFunction,
    Context
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcSyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++

  Routine Description:

    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction encoded in bit 7
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Maximum packet size the target endpoint is capable
                          of sending or receiving.
    Data                - Buffer of data that will be transmitted to
                          USB device or received from USB device.
    DataLength          - On input, the size, in bytes, of the data buffer;
                          On output, the number of bytes transferred.
    DataToggle          - On input, the initial data toggle to use;
                          on output, it is updated to indicate the next data toggle
    TimeOut             - Maximum time, in second, to complete
    Translator          - Transaction translator to use.
    TransferResult      - Variable to receive the transfer result

  Returns:

    EFI_SUCCESS           : The transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : The transfer failed due to timeout.
    EFI_DEVICE_ERROR      : The failed due to host controller or device error

--*/
{
  EFI_STATUS   Ret;
  USB2_HC_DEV  *Ehc;

  Ehc = EHC_FROM_THIS (This);
  SMMCALL_11(EhcSyncInterruptTransfer, &Ret,
    Ehc->SmmUsb2Hc,
    DeviceAddress,
    EndPointAddress,
    DeviceSpeed,
    MaximumPacketLength,
    Data,
    DataLength,
    DataToggle,
    TimeOut,
    Translator,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
EhcIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - End point address with its direction
    DeviceSpeed         - Device speed, Low speed device doesn't support this type.
    MaximumPacketLength - Maximum packet size that the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be
                          transmitted to USB device or received from USB device.
    DataLength          - The size, in bytes, of the data buffer
    Translator          - Transaction translator to use.
    TransferResult      - Variable to receive the transfer result

  Returns:

    EFI_UNSUPPORTED : Isochronous transfer is unsupported.

--*/
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
EhcAsyncIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  )
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device.

  Arguments:
    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - End point address with its direction
    DeviceSpeed         - Device speed, Low speed device doesn't support this type.
    MaximumPacketLength - Maximum packet size that the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be
                          transmitted to USB device or received from USB device.
    DataLength          - The size, in bytes, of the data buffer
    Translator          - Transaction translator to use.
    IsochronousCallBack - Function to be called when the transfer complete
    Context             - Context passed to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED : Isochronous transfer isn't supported

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EhcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
/*++

  Routine Description:

    Test to see if this driver supports ControllerHandle. Any
    ControllerHandle that has Usb2HcProtocol installed will
    be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controlle           - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         : This driver supports this device.
    EFI_UNSUPPORTED     : This driver does not support this device.

--*/
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  USB_CLASSC                    UsbClassCReg;
  EFI_LEGACY_USB_INF_PROTOCOL   *LegacyUsbInf;

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
    return EFI_UNSUPPORTED;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        EHC_PCI_CLASSC,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to Ehci type
  //
  if ((UsbClassCReg.BaseCode     != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.PI           != EHC_PCI_CLASSC_PI)) {

    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }


  Status = gBS->LocateProtocol (
                  &gEfiLegacyUsbInfProtocolGuid,
                  NULL,
                  &LegacyUsbInf
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  mSmmCallComm     = LegacyUsbInf->SmmCallCommBuf;
  mSmmCallTablePtr = LegacyUsbInf->SmmCallTablePtr;

  Status = gBS->LocateProtocol(&gEfiSmmControl2ProtocolGuid, NULL, &mSmmControl);

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

STATIC
USB2_HC_DEV *
EhcCreateUsb2Hc (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
/*++
  Routine Description:
    Create and initialize a USB2_HC_DEV

  Arguments:
    PciIo    - The PciIo on this device

  Returns:
    The allocated and initialized USB2_HC_DEV structure
    if created, otherwise NULL.

--*/
{
  USB2_HC_DEV             *Ehc;
  EFI_STATUS              Status;

  
  Ehc = AllocateZeroPool (sizeof (USB2_HC_DEV));

  if (Ehc == NULL) {
    return NULL;
  }

  //
  // Init EFI_USB2_HC_PROTOCOL interface and private data structure
  //
  Ehc->Signature                        = USB2_HC_DEV_SIGNATURE;

  Ehc->Usb2Hc.GetCapability             = EhcGetCapability;
  Ehc->Usb2Hc.Reset                     = EhcReset;
  Ehc->Usb2Hc.GetState                  = EhcGetState;
  Ehc->Usb2Hc.SetState                  = EhcSetState;
  Ehc->Usb2Hc.ControlTransfer           = EhcControlTransfer;
  Ehc->Usb2Hc.BulkTransfer              = EhcBulkTransfer;
  Ehc->Usb2Hc.AsyncInterruptTransfer    = EhcAsyncInterruptTransfer;
  Ehc->Usb2Hc.SyncInterruptTransfer     = EhcSyncInterruptTransfer;
  Ehc->Usb2Hc.IsochronousTransfer       = EhcIsochronousTransfer;
  Ehc->Usb2Hc.AsyncIsochronousTransfer  = EhcAsyncIsochronousTransfer;
  Ehc->Usb2Hc.GetRootHubPortStatus      = EhcGetRootHubPortStatus;
  Ehc->Usb2Hc.SetRootHubPortFeature     = EhcSetRootHubPortFeature;
  Ehc->Usb2Hc.ClearRootHubPortFeature   = EhcClearRootHubPortFeature;
  Ehc->Usb2Hc.MajorRevision             = 0x2;
  Ehc->Usb2Hc.MinorRevision             = 0x0;

  Ehc->PciIo = PciIo;
  
  Status = GetEhcInfo(Ehc);
  if (Status != EFI_SUCCESS) {
    goto ON_ERROR;
  }

  return Ehc;

ON_ERROR:
  FreePool (Ehc); 
  return NULL;
}

EFI_STATUS
EFIAPI
EhcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
/*++
  Routine Description:
    Starting the Usb EHCI Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS          : supports this device.
    EFI_UNSUPPORTED      : do not support this device.
    EFI_DEVICE_ERROR     : cannot be started due to device Error
    EFI_OUT_OF_RESOURCES : cannot allocate resources
--*/
{
  EFI_STATUS                Status;
  USB2_HC_DEV               *Ehc;
  EFI_PCI_IO_PROTOCOL       *PciIo;

    
  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **) &mCpuIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status  = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, &gSMM);
  if (EFI_ERROR (Status)) {
    return Status;
  }


  SMMCALL_0(StartLegacyUsb, &Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the PciIo Protocol, then enable the USB host controller
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
    DEBUG ((EFI_D_ERROR, "EhcDriverBindingStart: failed to open PCI_IO\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Create then install USB2_HC_PROTOCOL
  //
  Ehc = EhcCreateUsb2Hc (PciIo);

  if (Ehc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Ehc->Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_POOL;
  }
 
  Ehc->ControllerNameTable = NULL;
  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gEhciComponentName.SupportedLanguages,
    &Ehc->ControllerNameTable,
    L"Enhanced Host Controller (USB 2.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gEhciComponentName2.SupportedLanguages,
    &Ehc->ControllerNameTable,
    L"Enhanced Host Controller (USB 2.0)",
    FALSE
    );

  return EFI_SUCCESS;

FREE_POOL:
  FreePool (Ehc);

CLOSE_PCIIO:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

EFI_STATUS
EFIAPI
EhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
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

    EFI_SUCCESS         Success
    EFI_DEVICE_ERROR    Fail
--*/
{
  EFI_STATUS            Status;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB2_HC_DEV           *Ehc;
  
  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ehc   = EHC_FROM_THIS (Usb2Hc);
  PciIo = Ehc->PciIo;


  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Ehc->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (Ehc->ControllerNameTable);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Ehc);
  
  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL gEhciDriverBinding = {
  EhcDriverBindingSupported,
  EhcDriverBindingStart,
  EhcDriverBindingStop,
  0x10,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
EhcDriverEntryPoint (
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

  EFI_SUCCESS         Success
  EFI_DEVICE_ERROR    Fail

--*/
{

  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gEhciDriverBinding,
           ImageHandle,
           &gEhciComponentName,
           &gEhciComponentName2
           );         
       
  
}


