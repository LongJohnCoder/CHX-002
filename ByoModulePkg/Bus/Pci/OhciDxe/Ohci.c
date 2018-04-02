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
  Ohci.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "PiDxe.h"  
#include "Ohci.h"

#include <Protocol/smmbase2.h>  
#include <Protocol/CpuIo2.h>    
#include <Protocol/UsbPolicy.h>
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
GetOhcinfo(USB_OHCI_DEV*Hc)
{
  EFI_PCI_FUNCTION_ADDRESS   PciAddress;
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
    SMMCALL_4(GetOhcInfo, &Status, &PciAddress, &Hc->SmmOhc, &Hc->SmmUsbHc, &Hc->SmmUsb2Hc);
   }
  return Status;
  
}

STATIC
EFI_STATUS
EFIAPI
OhciReset (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT16                  Attributes
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(OhciReset, &Ret, Ohc->SmmUsbHc, Attributes);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
OhciGetState (
  IN  CONST EFI_USB_HC_PROTOCOL     *This,
  OUT EFI_USB_HC_STATE        *State
  )
{

  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(OhciGetState, &Ret, Ohc->SmmUsbHc, State);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
OhciSetState (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN EFI_USB_HC_STATE        State
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(OhciSetState, &Ret, Ohc->SmmUsbHc, State);

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
OhciControlTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_10(OhciControlTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciBulkTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_9(OhciBulkTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciAsyncInterruptTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_11(OhciAsyncInterruptTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciSyncInterruptTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_10(OhciSyncInterruptTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *TransferResult
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_7(OhciIsochronousTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciAsyncIsochronousTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_8(OhciAsyncIsochronousTransfer, &Ret,
    Ohc->SmmUsbHc,
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
OhciGetRootHubPortNumber (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT UINT8                   *PortNumber
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_2(OhciGetRootHubPortNumber, &Ret,
    Ohc->SmmUsbHc,
    PortNumber
    );

  return Ret;
}

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  OUT EFI_USB_PORT_STATUS     *PortStatus
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(OhciGetRootHubPortStatus, &Ret,
    Ohc->SmmUsbHc,
    PortNumber,
    PortStatus
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(OhciSetRootHubPortFeature, &Ret,
    Ohc->SmmUsbHc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB_HC_PROTO (This);
  SMMCALL_3(OhciClearRootHubPortFeature, &Ret,
    Ohc->SmmUsbHc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2GetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
{

  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_4(Ohci2GetCapability, &Ret,
    Ohc->SmmUsb2Hc,
    MaxSpeed,
    PortNumber,
    Is64BitCapable
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2Reset (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT16                 Attributes
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Ohci2Reset, &Ret,
    Ohc->SmmUsb2Hc,
    Attributes
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2GetState (
  IN  CONST EFI_USB2_HC_PROTOCOL   *This,
  OUT EFI_USB_HC_STATE       *State
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Ohci2GetState, &Ret,
    Ohc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2SetState (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN EFI_USB_HC_STATE        State
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_2(Ohci2SetState, &Ret,
    Ohc->SmmUsb2Hc,
    State
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2ControlTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Ohci2ControlTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2BulkTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_12(Ohci2BulkTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2AsyncInterruptTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_12(Ohci2AsyncInterruptTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2SyncInterruptTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Ohci2SyncInterruptTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2IsochronousTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_10(Ohci2IsochronousTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2AsyncIsochronousTransfer (
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
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_11(Ohci2AsyncIsochronousTransfer, &Ret,
    Ohc->SmmUsb2Hc,
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
Ohci2GetRootHubPortStatus (
  IN  CONST EFI_USB2_HC_PROTOCOL   *This,
  IN  CONST UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    *PortStatus
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Ohci2GetRootHubPortStatus, &Ret,
    Ohc->SmmUsb2Hc,
    PortNumber,
    PortStatus
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2SetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Ohci2SetRootHubPortFeature, &Ret,
    Ohc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
EFI_STATUS
EFIAPI
Ohci2ClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  EFI_STATUS Ret;
  USB_OHCI_DEV*Ohc;

  Ohc     = OHC_FROM_USB2_HC_PROTO (This);
  SMMCALL_3(Ohci2ClearRootHubPortFeature, &Ret,
    Ohc->SmmUsb2Hc,
    PortNumber,
    PortFeature
    );

  return Ret;
}

STATIC
USB_OHCI_DEV*
OhciAllocateDev (
  IN EFI_PCI_IO_PROTOCOL    *PciIo
  )
/*++

Routine Description:

  Allocate and initialize the empty Ohci device

Arguments:

  PciIo - The PCIIO to use

Returns:

  Allocated Ohci device

--*/
{
  USB_OHCI_DEV *Ohc;
  EFI_STATUS  Status;

  Ohc = AllocateZeroPool (sizeof (USB_OHCI_DEV));

  if (Ohc == NULL) {
    return NULL;
  }

  //
  // This driver supports both USB_HC_PROTOCOL and USB2_HC_PROTOCOL.
  // USB_HC_PROTOCOL is for EFI 1.1 backward compability.
  //
  Ohc->Signature                        = USB_OHCI_DEV_SIGNATURE;
  Ohc->UsbHc.Reset                      = OhciReset;
  Ohc->UsbHc.GetState                   = OhciGetState;
  Ohc->UsbHc.SetState                   = OhciSetState;
  Ohc->UsbHc.ControlTransfer            = OhciControlTransfer;
  Ohc->UsbHc.BulkTransfer               = OhciBulkTransfer;
  Ohc->UsbHc.AsyncInterruptTransfer     = OhciAsyncInterruptTransfer;
  Ohc->UsbHc.SyncInterruptTransfer      = OhciSyncInterruptTransfer;
  Ohc->UsbHc.IsochronousTransfer        = OhciIsochronousTransfer;
  Ohc->UsbHc.AsyncIsochronousTransfer   = OhciAsyncIsochronousTransfer;
  Ohc->UsbHc.GetRootHubPortNumber       = OhciGetRootHubPortNumber;
  Ohc->UsbHc.GetRootHubPortStatus       = OhciGetRootHubPortStatus;
  Ohc->UsbHc.SetRootHubPortFeature      = OhciSetRootHubPortFeature;
  Ohc->UsbHc.ClearRootHubPortFeature    = OhciClearRootHubPortFeature;
  Ohc->UsbHc.MajorRevision              = 0x1;
  Ohc->UsbHc.MinorRevision              = 0x1;

  Ohc->Usb2Hc.GetCapability             = Ohci2GetCapability;
  Ohc->Usb2Hc.Reset                     = Ohci2Reset;
  Ohc->Usb2Hc.GetState                  = Ohci2GetState;
  Ohc->Usb2Hc.SetState                  = Ohci2SetState;
  Ohc->Usb2Hc.ControlTransfer           = Ohci2ControlTransfer;
  Ohc->Usb2Hc.BulkTransfer              = Ohci2BulkTransfer;
  Ohc->Usb2Hc.AsyncInterruptTransfer    = Ohci2AsyncInterruptTransfer;
  Ohc->Usb2Hc.SyncInterruptTransfer     = Ohci2SyncInterruptTransfer;
  Ohc->Usb2Hc.IsochronousTransfer       = Ohci2IsochronousTransfer;
  Ohc->Usb2Hc.AsyncIsochronousTransfer  = Ohci2AsyncIsochronousTransfer;
  Ohc->Usb2Hc.GetRootHubPortStatus      = Ohci2GetRootHubPortStatus;
  Ohc->Usb2Hc.SetRootHubPortFeature     = Ohci2SetRootHubPortFeature;
  Ohc->Usb2Hc.ClearRootHubPortFeature   = Ohci2ClearRootHubPortFeature;
  Ohc->Usb2Hc.MajorRevision             = 0x1;
  Ohc->Usb2Hc.MinorRevision             = 0x1;

  Ohc->PciIo   = PciIo;

  Status = GetOhcinfo(Ohc);

  if (Status != EFI_SUCCESS) {
    goto ON_ERROR;
  }

  return Ohc;

ON_ERROR:
  FreePool (Ohc);
  return NULL;
}

STATIC
VOID
OhciFreeDev (
  IN USB_OHCI_DEV          *Ohc
  )
/*++

Routine Description:

  Free the Ohci device and release its associated resources

Arguments:

  Ohc - The Ohci device to release

Returns:

  None

--*/
{
  FreePool (Ohc);
}

STATIC
VOID
OhciCleanDevUp (
  IN  EFI_HANDLE          Controller,
  IN  EFI_USB_HC_PROTOCOL *This
  )
/*++
  Routine Description:

    Uninstall all Ohci Interface

  Arguments:

    Controller        - Controller handle
    This              - Protocol instance pointer.

  Returns:

    VOID

--*/
{
  USB_OHCI_DEV         *Ohc;

  //
  // Uninstall the USB_HC and USB_HC2 protocol, then disable the controller
  //
  Ohc = OHC_FROM_USB_HC_PROTO (This);

  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiUsbHcProtocolGuid,
        &Ohc->UsbHc
        );

  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiUsb2HcProtocolGuid,
        &Ohc->Usb2Hc
        );

  OhciFreeDev (Ohc);
}

EFI_STATUS
EFIAPI
OhciDriverBindingSupported (
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
  // Test whether the controller belongs to Ohci type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.PI != PCI_CLASSC_PI_OHCI)
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
OhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

  Routine Description:

    Starting the Usb Ohci Driver

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
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB_OHCI_DEV          *Ohc;

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
  // Open PCIIO, then enable the EHC device and turn off emulation
  //
  Ohc = NULL;
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

  Ohc = OhciAllocateDev (PciIo);

  if (Ohc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  //
  // Install both USB_HC_PROTOCOL and USB2_HC_PROTOCOL
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiUsbHcProtocolGuid,
                  &Ohc->UsbHc,
                  &gEfiUsb2HcProtocolGuid,
                  &Ohc->Usb2Hc,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_Ohc;
  }

  Ohc->ControllerNameTable = NULL;
  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gOhciComponentName.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"Open Host Controller (USB 1.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gOhciComponentName2.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"Open Host Controller (USB 1.0)",
    FALSE
    );

  return EFI_SUCCESS;

FREE_Ohc:
  OhciFreeDev (Ohc);

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
OhciDriverBindingStop (
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
  USB_OHCI_DEV          *OhcDev;


  
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

  OhcDev = OHC_FROM_USB_HC_PROTO (UsbHc);
  if (OhcDev->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (OhcDev->ControllerNameTable);
  }
  OhciCleanDevUp (Controller, UsbHc);

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gOhciDriverBinding = {
  OhciDriverBindingSupported,
  OhciDriverBindingStart,
  OhciDriverBindingStop,
  0x20,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
OhciDriverEntryPoint (
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
           &gOhciDriverBinding,
           ImageHandle,
           &gOhciComponentName,
           &gOhciComponentName2
           );         
       
  
}


