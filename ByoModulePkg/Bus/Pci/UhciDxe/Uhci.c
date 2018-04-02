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
  Uhci.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/
#include "PiDxe.h"
#include "Uhci.h"

#include <Protocol/smmbase2.h>  
#include <Protocol/CpuIo2.h>    
#include <Library/IoLib.h>
#include <Protocol/SmmControl2.h>

EFI_CPU_IO2_PROTOCOL     *mCpuIo = NULL;
volatile USB_SMMCALL_COMM        *mSmmCallComm;
SMMCALL_ENTRY           *mSmmCallTablePtr;
EFI_SMM_BASE2_PROTOCOL   *gSMM = NULL;
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

/*
VOID
IoWrite8 (
  IN UINT16  Port,
  IN UINT8   Value
  )
{
  mCpuIo->Io.Write (mCpuIo, EfiCpuIoWidthUint8, Port, 1, &Value);
}
*/

STATIC
EFI_STATUS
EFIAPI
GetUhcInfo(USB_HC_DEV *Hc)
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
    SMMCALL_4(GetUhcInfo, &Status, &PciAddress, &Hc->SmmUhc, &Hc->SmmUsbHc, &Hc->SmmUsb2Hc);
  }
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
UhciReset (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT16                  Attributes
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(UhciReset, &Ret, Uhc->SmmUsbHc, Attributes);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciGetState (
  IN  CONST EFI_USB_HC_PROTOCOL     *This,
  OUT EFI_USB_HC_STATE        *State
  )
{

  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(UhciGetState, &Ret, Uhc->SmmUsbHc, State);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciSetState (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN EFI_USB_HC_STATE        State
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(UhciSetState, &Ret, Uhc->SmmUsbHc, State);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciControlTransfer (
  IN       EFI_USB_HC_PROTOCOL        *This,
  IN       UINT8                      DeviceAddress,
  IN       BOOLEAN                    IsSlowDevice,
  IN       UINT8                      MaximumPacketLength,
  IN       EFI_USB_DEVICE_REQUEST     *Request,
  IN       EFI_USB_DATA_DIRECTION     TransferDirection,
  IN OUT   VOID                       *Data,              OPTIONAL
  IN OUT   UINTN                      *DataLength,        OPTIONAL
  IN       UINTN                      TimeOut,
  OUT      UINT32                     *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_10(UhciControlTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    IsSlowDevice,
    MaximumPacketLength,
    Request,
    TransferDirection,
    Data,
    DataLength,
    TimeOut,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciBulkTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_9(UhciBulkTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    EndPointAddress,
    MaximumPacketLength,
    Data,
    DataLength,
    DataToggle,
    TimeOut,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciAsyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL                * This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     BOOLEAN                            IsSlowDevice,
  IN     UINT8                              MaximumPacketLength,
  IN     BOOLEAN                            IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              PollingInterval,    OPTIONAL
  IN     UINTN                              DataLength,         OPTIONAL
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,   OPTIONAL
  IN     VOID                               *Context OPTIONAL
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_11(UhciAsyncInterruptTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    EndPointAddress,
    IsSlowDevice,
    MaximumPacketLength,
    IsNewTransfer,
    DataToggle,
    PollingInterval,
    DataLength,
    CallBackFunction,
    Context
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciSyncInterruptTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       BOOLEAN                 IsSlowDevice,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_10(UhciSyncInterruptTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    EndPointAddress,
    IsSlowDevice,
    MaximumPacketLength,
    Data,
    DataLength,
    DataToggle,
    TimeOut,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_7(UhciIsochronousTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    EndPointAddress,
    MaximumPacketLength,
    Data,
    DataLength,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciAsyncIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL                 * This,
  IN       UINT8                               DeviceAddress,
  IN       UINT8                               EndPointAddress,
  IN       UINT8                               MaximumPacketLength,
  IN OUT   VOID                                *Data,
  IN       UINTN                               DataLength,
  IN       EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN       VOID                                *Context OPTIONAL
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_8(UhciAsyncIsochronousTransfer, &Ret,
    Uhc->SmmUsbHc,
    DeviceAddress,
    EndPointAddress,
    MaximumPacketLength,
    Data,
    DataLength,
    IsochronousCallBack,
    Context
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciGetRootHubPortNumber (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT UINT8                   *PortNumber
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(UhciGetRootHubPortNumber, &Ret,
    Uhc->SmmUsbHc,
    PortNumber
    );

  return Ret;
}

EFI_STATUS
EFIAPI
UhciGetRootHubPortStatus (
  IN  CONST EFI_USB_HC_PROTOCOL     *This,
  IN  CONST UINT8                   PortNumber,
  OUT EFI_USB_PORT_STATUS     *PortStatus
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(UhciGetRootHubPortStatus, &Ret,
    Uhc->SmmUsbHc,
    PortNumber,
    PortStatus
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciSetRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(UhciSetRootHubPortFeature, &Ret,
    Uhc->SmmUsbHc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
UhciClearRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(UhciClearRootHubPortFeature, &Ret,
    Uhc->SmmUsbHc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2GetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
{

  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_4(Uhci2GetCapability, &Ret,
    Uhc->SmmUsb2Hc,
    MaxSpeed,
    PortNumber,
    Is64BitCapable
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2Reset (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT16                 Attributes
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Uhci2Reset, &Ret,
    Uhc->SmmUsb2Hc,
    Attributes
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2GetState (
  IN  CONST EFI_USB2_HC_PROTOCOL   *This,
  OUT EFI_USB_HC_STATE       *State
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Uhci2GetState, &Ret,
    Uhc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2SetState (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN EFI_USB_HC_STATE        State
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Uhci2SetState, &Ret,
    Uhc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2ControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL                 *This,
  IN     UINT8                                DeviceAddress,
  IN     UINT8                                DeviceSpeed,
  IN     UINTN                                MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST               *Request,
  IN     EFI_USB_DATA_DIRECTION               TransferDirection,
  IN OUT VOID                                 *Data,
  IN OUT UINTN                                *DataLength,
  IN     UINTN                                TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT    UINT32                               *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Uhci2ControlTransfer, &Ret,
    Uhc->SmmUsb2Hc,
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
Uhci2BulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_12(Uhci2BulkTransfer, &Ret,
    Uhc->SmmUsb2Hc,
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
Uhci2AsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     BOOLEAN                            IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              PollingInterval,
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
  IN     VOID                               *Context
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_12(Uhci2AsyncInterruptTransfer, &Ret,
    Uhc->SmmUsb2Hc,
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
Uhci2SyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                      *This,
  IN     UINT8                                     DeviceAddress,
  IN     UINT8                                     EndPointAddress,
  IN     UINT8                                     DeviceSpeed,
  IN     UINTN                                     MaximumPacketLength,
  IN OUT VOID                                      *Data,
  IN OUT UINTN                                     *DataLength,
  IN OUT UINT8                                     *DataToggle,
  IN     UINTN                                     TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR        *Translator,
  OUT    UINT32                                    *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Uhci2SyncInterruptTransfer, &Ret,
    Uhc->SmmUsb2Hc,
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
Uhci2IsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_10(Uhci2IsochronousTransfer, &Ret,
    Uhc->SmmUsb2Hc,
    DeviceAddress,
    EndPointAddress,
    DeviceSpeed,
    MaximumPacketLength,
    DataBuffersNumber,
    Data,
    DataLength,
    Translator,
    TransferResult
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2AsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN     VOID                                *Context
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Uhci2AsyncIsochronousTransfer, &Ret,
    Uhc->SmmUsb2Hc,
    DeviceAddress,
    EndPointAddress,
    DeviceSpeed,
    MaximumPacketLength,
    DataBuffersNumber,
    Data,
    DataLength,
    Translator,
    IsochronousCallBack,
    Context
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2GetRootHubPortStatus (
  IN  CONST EFI_USB2_HC_PROTOCOL   *This,
  IN  CONST UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    *PortStatus
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Uhci2GetRootHubPortStatus, &Ret,
    Uhc->SmmUsb2Hc,
    PortNumber,
    PortStatus
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2SetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Uhci2SetRootHubPortFeature, &Ret,
    Uhc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2ClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS    Ret;
  USB_HC_DEV    *Uhc;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Uhci2ClearRootHubPortFeature, &Ret,
    Uhc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
USB_HC_DEV *
UhciAllocateDev (
  IN EFI_PCI_IO_PROTOCOL    *PciIo
  )
/*++

Routine Description:

  Allocate and initialize the empty UHCI device

Arguments:

  PciIo - The PCIIO to use

Returns:

  Allocated UHCI device

--*/
{
  USB_HC_DEV    *Uhc;
  EFI_STATUS    Status;

  Uhc = AllocateZeroPool (sizeof (USB_HC_DEV));

  if (Uhc == NULL) {
    return NULL;
  }

  //
  // This driver supports both USB_HC_PROTOCOL and USB2_HC_PROTOCOL.
  // USB_HC_PROTOCOL is for EFI 1.1 backward compability.
  //
  Uhc->Signature                        = USB_HC_DEV_SIGNATURE;
  Uhc->UsbHc.Reset                      = UhciReset;
  Uhc->UsbHc.GetState                   = UhciGetState;
  Uhc->UsbHc.SetState                   = UhciSetState;
  Uhc->UsbHc.ControlTransfer            = UhciControlTransfer;
  Uhc->UsbHc.BulkTransfer               = UhciBulkTransfer;
  Uhc->UsbHc.AsyncInterruptTransfer     = UhciAsyncInterruptTransfer;
  Uhc->UsbHc.SyncInterruptTransfer      = UhciSyncInterruptTransfer;
  Uhc->UsbHc.IsochronousTransfer        = UhciIsochronousTransfer;
  Uhc->UsbHc.AsyncIsochronousTransfer   = UhciAsyncIsochronousTransfer;
  Uhc->UsbHc.GetRootHubPortNumber       = UhciGetRootHubPortNumber;
  Uhc->UsbHc.GetRootHubPortStatus       = UhciGetRootHubPortStatus;
  Uhc->UsbHc.SetRootHubPortFeature      = UhciSetRootHubPortFeature;
  Uhc->UsbHc.ClearRootHubPortFeature    = UhciClearRootHubPortFeature;
  Uhc->UsbHc.MajorRevision              = 0x1;
  Uhc->UsbHc.MinorRevision              = 0x1;

  Uhc->Usb2Hc.GetCapability             = Uhci2GetCapability;
  Uhc->Usb2Hc.Reset                     = Uhci2Reset;
  Uhc->Usb2Hc.GetState                  = Uhci2GetState;
  Uhc->Usb2Hc.SetState                  = Uhci2SetState;
  Uhc->Usb2Hc.ControlTransfer           = Uhci2ControlTransfer;
  Uhc->Usb2Hc.BulkTransfer              = Uhci2BulkTransfer;
  Uhc->Usb2Hc.AsyncInterruptTransfer    = Uhci2AsyncInterruptTransfer;
  Uhc->Usb2Hc.SyncInterruptTransfer     = Uhci2SyncInterruptTransfer;
  Uhc->Usb2Hc.IsochronousTransfer       = Uhci2IsochronousTransfer;
  Uhc->Usb2Hc.AsyncIsochronousTransfer  = Uhci2AsyncIsochronousTransfer;
  Uhc->Usb2Hc.GetRootHubPortStatus      = Uhci2GetRootHubPortStatus;
  Uhc->Usb2Hc.SetRootHubPortFeature     = Uhci2SetRootHubPortFeature;
  Uhc->Usb2Hc.ClearRootHubPortFeature   = Uhci2ClearRootHubPortFeature;
  Uhc->Usb2Hc.MajorRevision             = 0x1;
  Uhc->Usb2Hc.MinorRevision             = 0x1;

  Uhc->PciIo   = PciIo;

  Status = GetUhcInfo(Uhc);

  if (Status != EFI_SUCCESS) {
    goto ON_ERROR;
  }

  return Uhc;

ON_ERROR:
  FreePool (Uhc);
  return NULL;
}

STATIC
VOID
UhciFreeDev (
  IN USB_HC_DEV           *Uhc
  )
/*++

Routine Description:

  Free the UHCI device and release its associated resources

Arguments:

  Uhc - The UHCI device to release

Returns:

  None

--*/
{
  FreePool (Uhc);
}

STATIC
VOID
UhciCleanDevUp (
  IN  EFI_HANDLE          Controller,
  IN  EFI_USB_HC_PROTOCOL *This
  )
/*++
  Routine Description:

    Uninstall all Uhci Interface

  Arguments:

    Controller        - Controller handle
    This              - Protocol instance pointer.

  Returns:

    VOID

--*/
{
  USB_HC_DEV          *Uhc;

  //
  // Uninstall the USB_HC and USB_HC2 protocol, then disable the controller
  //
  Uhc = UHC_FROM_USB_HC_PROTO (This);

  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiUsbHcProtocolGuid,
        &Uhc->UsbHc
        );

  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiUsb2HcProtocolGuid,
        &Uhc->Usb2Hc
        );

  UhciFreeDev (Uhc);
}

EFI_STATUS
EFIAPI
UhciDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

  Routine Description:

    Test to see if this driver supports ControllerHandle. Any
    ControllerHandle that has UsbHcProtocol installed will be supported.

  Arguments:

    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS         : This driver supports this device.
    EFI_UNSUPPORTED     : This driver does not support this device.

--*/
{
  EFI_STATUS                    OpenStatus;
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  USB_CLASSC                    UsbClassCReg;
  EFI_LEGACY_USB_INF_PROTOCOL   *LegacyUsbInf;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      &PciIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        CLASSC_OFFSET,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to UHCI type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.PI != PCI_CLASSC_PI_UHCI)
      ) {

    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  Status = gBS->LocateProtocol (
          &gEfiLegacyUsbInfProtocolGuid,
          NULL,
          &LegacyUsbInf
          );

  if (Status != EFI_SUCCESS) {
      Status = EFI_UNSUPPORTED;
      goto ON_EXIT;
  }

  mSmmCallComm = LegacyUsbInf->SmmCallCommBuf;
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

EFI_STATUS
EFIAPI
UhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

  Routine Description:

    Starting the Usb UHCI Driver

  Arguments:

    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device Error
    EFI_OUT_OF_RESOURCES- Failed due to resource shortage

--*/
{
  EFI_STATUS               Status;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  USB_HC_DEV               *Uhc;

  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **) &mCpuIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, &gSMM);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SMMCALL_0(StartLegacyUsb, &Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open PCIIO, then enable the EHC device and turn off emulation
  //
  Uhc = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Uhc = UhciAllocateDev (PciIo);

  if (Uhc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  //
  // Install both USB_HC_PROTOCOL and USB2_HC_PROTOCOL
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiUsbHcProtocolGuid,
                  &Uhc->UsbHc,
                  &gEfiUsb2HcProtocolGuid,
                  &Uhc->Usb2Hc,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_UHC;
  }

  //
  // Install the component name protocol
  //
  Uhc->ControllerNameTable = NULL;

  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gUhciComponentName.SupportedLanguages,
    &Uhc->ControllerNameTable,
    L"Universial Host Controller (USB 1.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUhciComponentName2.SupportedLanguages,
    &Uhc->ControllerNameTable,
    L"Universial Host Controller (USB 1.0)",
    FALSE
    );

  return EFI_SUCCESS;

FREE_UHC:
  UhciFreeDev (Uhc);

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
UhciDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
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
    others

--*/
{
  EFI_USB_HC_PROTOCOL   *UsbHc;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  EFI_STATUS            Status;
  USB_HC_DEV            *UhcDev;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbHcProtocolGuid,
                  &UsbHc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UhcDev = UHC_FROM_USB_HC_PROTO (UsbHc);
  if (UhcDev->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UhcDev->ControllerNameTable);
  }

  UhciCleanDevUp (Controller, UsbHc);

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gUhciDriverBinding = {
  UhciDriverBindingSupported,
  UhciDriverBindingStart,
  UhciDriverBindingStop,
  0x20,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
UhciDriverEntryPoint (
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

    EFI_SUCCESS : Driver is successfully loaded
    Others      : Failed

--*/
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gUhciDriverBinding,
           ImageHandle,
           &gUhciComponentName,
           &gUhciComponentName2
           );  
}


