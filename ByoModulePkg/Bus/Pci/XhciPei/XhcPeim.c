/*++

Copyright (c)  1999 - 2005 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  XhcPeim.c

Abstract:

  Usb Host Controller PEIM

--*/

#include "XhcPeim.h"




//
// Two arrays used to translate the XHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {PORTSC_CONN,     USB_PORT_STAT_CONNECTION},
  {PORTSC_ENABLED,  USB_PORT_STAT_ENABLE},
//  {PORTSC_SUSPEND,  USB_PORT_STAT_SUSPEND},
  {PORTSC_OVERCUR,  USB_PORT_STAT_OVERCURRENT},
  {PORTSC_RESET,    USB_PORT_STAT_RESET},
  {PORTSC_POWER,    USB_PORT_STAT_POWER},
//  {PORTSC_OWNER,    USB_PORT_STAT_OWNER}
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {PORTSC_CONN_CHANGE,    USB_PORT_STAT_C_CONNECTION},
  {PORTSC_ENABLE_CHANGE,  USB_PORT_STAT_C_ENABLE},
  {PORTSC_WARM_RESET_CHANGE, USB_PORT_STAT_C_BH_RESET},
  {PORTSC_OVERCUR_CHANGE, USB_PORT_STAT_C_OVERCURRENT},
  {PORTSC_RESET_CHANGE ,   USB_PORT_STAT_C_RESET}
};



          
UINT32
XhcReadOpReg (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT32              Offset
  )
/*++

Routine Description:

  Read  Xhc Operation register

Arguments:

  Xhc      - The XHCI device
  Offset   - The operation register offset

Returns:

  The register content read
  
--*/
{
  UINT32                  Data;
 
  //ASSERT (Xhc->CapLength != 0);
  Data = MmioRead32 ((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset) );
  return Data;
}


VOID
XhcWriteOpReg (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
/*++

Routine Description:

  Write  the data to the XHCI operation register

Arguments:

  Xhc      - The XHCI device
  Offset   - XHCI operation register offset
  Data     - The data to write

Returns:

  None

--*/
{

  //ASSERT (Xhc->CapLength != 0);

  MmioWrite32((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->CapLength+ Offset), Data);

}


VOID
XhcSetOpRegBit (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
/*++

Routine Description:
  Set one bit of the operational register while keeping other bits

Arguments:
  Xhc     - The XHCI device
  Offset  - The offset of the operational register
  Bit     - The bit mask of the register to set
  
Returns:

  None
  
--*/
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}


VOID
XhcClearOpRegBit (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
/*++

Routine Description:
  Clear one bit of the operational register while keeping other bits

Arguments:
  Xhc - The XHCI device
  Offset  - The offset of the operational register
  Bit     - The bit mask of the register to clear

Returns:

  None

--*/
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}


EFI_STATUS
XhcWaitOpRegBit (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
/*++

Routine Description:

  Wait the operation register's bit as specified by Bit 
  to become set (or clear)

Arguments:

  Xhc         - The XHCI device
  Offset      - The offset of the operation register 
  Bit         - The bit of the register to wait for
  WaitToSet   - Wait the bit to set or clear
  Timeout     - The time to wait before abort (in millisecond)

Returns:

  EFI_SUCCESS - The bit successfully changed by host controller
  EFI_TIMEOUT - The time out occurred

--*/
{
  UINT32                  Index;

  for (Index = 0; Index < Timeout / XHC_SYNC_POLL_INTERVAL + 1; Index++) {
    if (XHC_REG_BIT_IS_SET (Xhc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay(XHC_SYNC_POLL_INTERVAL);
  }

  return EFI_TIMEOUT;
}




UINT32
XhcReadCapRegister (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT32              Offset
  )
/*++

Routine Description:

  Read  XHCI capability register

Arguments:

  Xhc     - The Xhc device 
  Offset  - Capability register address

Returns:

  The register content read
  
--*/
{
  UINT32                  Data;
  
  Data = MmioRead32 ((UINTN)(Xhc->UsbHostControllerBaseAddress + Offset) );
  
  return Data;
}

EFI_STATUS
XhcSetAndWaitDoorBell (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT32              Timeout
  )
/*++

Routine Description:

  Set door bell and wait it to be ACKed by host controller.
  This function is used to synchronize with the hardware.

Arguments:

  Xhc     - The XHCI device
  Timeout - The time to wait before abort (in millisecond, ms)
Returns:

  EFI_SUCCESS : Synchronized with the hardware
  EFI_TIMEOUT : Time out happened while waiting door bell to set

--*/
{
  EFI_STATUS              Status;
  UINT32                  Data;

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_IAAD);

  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, USBSTS_IAA, TRUE, Timeout);

  //
  // ACK the IAA bit in USBSTS register. Make sure other
  // interrupt bits are not ACKed. These bits are WC (Write Clean).
  //
  Data  = XhcReadOpReg (Xhc, XHC_USBSTS_OFFSET);
  Data &= ~USBSTS_INTACK_MASK;
  Data |= USBSTS_IAA;

  XhcWriteOpReg (Xhc, XHC_USBSTS_OFFSET, Data);

  return Status;
}

VOID
XhcAckAllInterrupt (
  IN  USB3_HC_DEV         *Xhc
  )
/*++

Routine Description:

  Clear all the interrutp status bits, these bits 
  are Write-Clean

Arguments:

  Xhc - The XHCI device

Returns:

  None
  
--*/
{
  XhcWriteOpReg (Xhc, XHC_USBSTS_OFFSET, 0x0002);
}

BOOLEAN
XhcIsHalt (
  IN USB3_HC_DEV          *Xhc
  )
/*++

Routine Description:

  Whether Xhc is halted

Arguments:

  Xhc - The XHCI device

Returns:

  TRUE  : The controller is halted
  FALSE : It isn't halted

--*/
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHCSTS_HALT);
}

BOOLEAN
XhcIsSysError (
  IN USB3_HC_DEV          *Xhc
  )
/*++

Routine Description:

  Whether system error occurred

Arguments:

  Xhc - The XHCI device

Returns:

  TRUE  : System error happened 
  FALSE : No system error

--*/
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
}

EFI_STATUS
XhcResetHC (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Timeout
  )
/*++

Routine Description:

  Reset the host controller

Arguments:

  Xhc     - The XHCI device
  Timeout - Time to wait before abort (in millisecond, ms)

Returns:

  EFI_SUCCESS : The host controller is reset
  Others      : Failed to reset the host

--*/
{
  EFI_STATUS              Status;

  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHCSTS_HALT)) {
    Status = XhcHaltHC (Xhc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_RESET);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_RESET, FALSE, Timeout);
  return Status;
}

EFI_STATUS
XhcHaltHC (
  IN USB3_HC_DEV         *Xhc,
  IN UINT32              Timeout
  )
/*++

Routine Description:

  Halt the host controller

Arguments:

  Xhc     - The XHCI device
  Timeout - Time to wait before abort

Returns:

  EFI_SUCCESS : The XHCI is halt
  EFI_TIMEOUT : Failed to halt the controller before Timeout 

--*/
{
  EFI_STATUS              Status;

  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHCSTS_HALT, TRUE, Timeout);
  return Status;
}

EFI_STATUS
XhcRunHC (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Timeout
  )
/*++

Routine Description:

  Set the XHCI to run

Arguments:

  Xhc     - The XHCI device
  Timeout - Time to wait before abort

Returns:

  EFI_SUCCESS : The XHCI is running
  Others      : Failed to set the XHCI to run

--*/
{
  EFI_STATUS              Status;

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHCSTS_HALT, FALSE, Timeout);
  return Status;
}

VOID
XhcWriteOpReg64 (
  IN USB3_HC_DEV          *Xhc,
  IN UINT64               Offset,
  IN UINT64               Data
  )
/*++

Routine Description:

  Write  the data to the XHCI operation register

Arguments:

  Xhc      - The XHCI device
  Offset   - XHCI operation register offset
  Data     - The data to write

Returns:

  None

--*/
{
    MmioWrite32((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset), XHC_LOW_32BIT(Data));
    MmioWrite32((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset+4), XHC_HIGH_32BIT(Data));
}



VOID
XhcWriteOpReg16 (
  IN USB3_HC_DEV          *Xhc,
  IN UINT64               Offset,
  IN UINT64               Data
  )
/*++

Routine Description:

  Write  the data to the XHCI operation register

Arguments:

  Xhc      - The XHCI device
  Offset   - XHCI operation register offset
  Data     - The data to write

Returns:

  None

--*/
{
    MmioWrite16((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset),(UINT16)Data);
}

VOID
XhcWriteDBReg (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
/*++

Routine Description:

  Write  the data to the XHCI door bell register

Arguments:

  Xhc      - The XHCI device
  Offset   - XHCI runtime register offset
  Data     - The data to write

Returns:

  None

--*/
{
  MmioWrite32((UINTN)(Xhc->UsbHostControllerBaseAddress + Xhc->DBOff + Offset), Data);
  return ;
}

EFI_STATUS
XhcInitHC (
  IN USB3_HC_DEV          *Xhc
  )
/*++

Routine Description:

  Initialize the HC hardware. 
  XHCI spec lists the  things to do to initialize the hardware

  5.  Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register
    (5.4.5) with a 64-bit address pointing to the starting address of the first TRB of the Command Ring.

Arguments:

  Xhc - The XHCI device

Returns:

  EFI_SUCCESS : The XHCI has come out of halt state
  EFI_TIMEOUT : Time out happened

--*/
{

EFI_STATUS              Status;
Status          = EFI_SUCCESS;

#if 0 
 //reserved for future use 
 if (Xhc->DCBAA != NULL) {
    XhcFreeSched (Xhc);
  }
#endif 
  //
  // Allocate the DCBAA  and COMMAND ring and
  // COMMAND event ring for Host controller use
  //
  Status = XhcInitSched (Xhc);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // 1. Clear USBINTR to disable all the interrupt. UEFI works by polling
  //
  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_INTE);
  
  //
  // 2. Start the Host Controller
  //
  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, USBCMD_RUN);

  //
  // Do port Power according the Port Power switch ,1223,need  to add set port power
  //

  MicroSecondDelay(XHC_ROOT_PORT_RECOVERY_STALL);
  
return Status;
}


EFI_STATUS
EFIAPI
XhcPeimEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Initializes the Atapi Block Io PPI

Arguments:
  FfsHeader   - EFI_FFS_FILE_HEADER
  PeiServices - General purpose services available to every PEIM.
    
Returns:

  EFI_UNSUPPORTED       - Can't find required PPI
  EFI_OUT_OF_RESOURCES  - Can't allocate memory resource
  EFI_SUCCESS           - Success
--*/
{
  PEI_USB_CONTROLLER_PPI      *ChipSetUsbControllerPpi;
  EFI_STATUS                  Status;
  UINT8                       Index;
  UINTN                       ControllerType;
  UINTN                       BaseAddress;
  UINTN                       MemPages;
  USB3_HC_DEV                 *XhcDev;
  EFI_PHYSICAL_ADDRESS        TempPtr;
  UINT32                      Data;

  XHC_PEI_DEBUG((EFI_D_INFO, "XhcPeimEntry\n"));
  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  Status = PeiServicesLocatePpi (
             &gPeiUsbControllerPpiGuid,
             0,
             NULL,
             (VOID **) &ChipSetUsbControllerPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  
  //
  //Pick out all Xhc and init
  //
  Index = 0;
  while (TRUE) 
  {
      Status = ChipSetUsbControllerPpi->GetUsbController (
                                        (EFI_PEI_SERVICES **)PeiServices,
                                        ChipSetUsbControllerPpi,
                                        Index,
                                        &ControllerType,
                                        &BaseAddress
                                        );
      if (EFI_ERROR (Status)) {
         break;
      }
    
    if ((ControllerType  != PEI_XHCI_CONTROLLER)||(BaseAddress==0)) {
        Index++;
        continue;
    }
    XHC_PEI_DEBUG((EFI_D_INFO, "One Xhc found!\n"));

    
    //----Create xhc USB3_HC_DEV instance and init -------
    
    MemPages = sizeof (USB3_HC_DEV) / PAGESIZE + 1;
    Status = PeiServicesAllocatePages (
                 EfiBootServicesCode,
                 MemPages,
                 &TempPtr
                 );
    if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
    }
  
    ZeroMem((VOID *)(UINTN)TempPtr, MemPages*PAGESIZE);
    XhcDev = (USB3_HC_DEV *) ((UINTN) TempPtr);
  
    XhcDev->Signature = USB3_HC_DEV_SIGNATURE;
  
    XhcDev->UsbHostControllerBaseAddress = (UINT32) BaseAddress;
  
  
    Data              = XhcReadCapRegister (XhcDev, XHC_CAPLENGTH_OFFSET);   // XhcReadCapRegister Return Dword from Offset
    XhcDev->CapLength    = (UINT8) (Data & 0x0FF);
    XhcDev->HciVersion   = (UINT16) ((Data & 0x0FFFF0000) >> 16);
    XhcDev->HcSParams1   = XhcReadCapRegister (XhcDev, XHC_HCSPARAMS1_OFFSET);
    XhcDev->HcSParams2   = XhcReadCapRegister (XhcDev, XHC_HCSPARAMS2_OFFSET);
    XhcDev->HcSParams3   = XhcReadCapRegister (XhcDev, XHC_HCSPARAMS3_OFFSET);
    XhcDev->HcCParams    = XhcReadCapRegister (XhcDev, XHC_HCCPARAMS_OFFSET);
    XhcDev->DBOff        = XhcReadCapRegister (XhcDev, XHC_DBOFF_OFFSET);
    XhcDev->RTSOff       = XhcReadCapRegister (XhcDev, XHC_RTSOFF_OFFSET);
     //----PPI related Items -------
    XhcDev->Usb2HostControllerPpi.ControlTransfer          = XhcControlTransfer;
    XhcDev->Usb2HostControllerPpi.BulkTransfer             = XhcBulkTransfer;
    XhcDev->Usb2HostControllerPpi.GetRootHubPortNumber     = XhcGetRootHubPortNumber;
    XhcDev->Usb2HostControllerPpi.GetRootHubPortStatus     = XhcGetRootHubPortStatus;
    XhcDev->Usb2HostControllerPpi.SetRootHubPortFeature    = XhcSetRootHubPortFeature;
    XhcDev->Usb2HostControllerPpi.ClearRootHubPortFeature  = XhcClearRootHubPortFeature;
    XhcDev->Usb2HostControllerPpi.GetHostControllerType    =XhcGetHostControllerType;
    XhcDev->Usb2HostControllerPpi.Usb3HostControllerFunc.ApplyDeviceSlot = XhcApplyDeviceSlot;
    XhcDev->Usb2HostControllerPpi.Usb3HostControllerFunc.EvaluateContext = XhcEvaluateContext;
    XhcDev->Usb2HostControllerPpi.Usb3HostControllerFunc.SetConfiguration =XhcSetConfigCmd;
    XhcDev->Usb2HostControllerPpi.Usb3HostControllerFunc.UpdateSlotContext=XhcUpdateSlotContext;
    
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->CapLength=%x,\n",XhcDev->CapLength));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->HciVersion=%x\n",XhcDev->HciVersion));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->HcSParams1=%x\n",XhcDev->HcSParams1));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->HcSParams2=%x\n",XhcDev->HcSParams2));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->HcSParams3=%x\n",XhcDev->HcSParams3));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->HcCParams=%x\n",XhcDev->HcCParams));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->DBOff=%x\n",XhcDev->DBOff));
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcDev->RTSOff=%x\n",XhcDev->RTSOff));
      
     //----Do xhc hardware initialization(Allocate the resources it required) -------
      Status = InitializeUsbHC (XhcDev);
      if (EFI_ERROR (Status)) {
        return Status;
      }

     //----Publish the Usb2HostControllerPpiGuid (mainly for UsbPeim use) -------
      XhcDev->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
      XhcDev->PpiDescriptor.Guid = &gPeiUsb2HostControllerPpiGuid;
      XhcDev->PpiDescriptor.Ppi = &XhcDev->Usb2HostControllerPpi;
  
      Status = PeiServicesInstallPpi(&XhcDev->PpiDescriptor);
      if (EFI_ERROR (Status)) {
        continue;
      }
  
      Index++;
  }

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
XhcGetRootHubPortNumber (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  OUT UINT8                                 *PortNumber
  )
{

  USB3_HC_DEV                 *XhcDev;
  XhcDev = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  *PortNumber =(UINT8)((XhcDev->HcSParams1 >> 24) & MAX_PORTS);
  XHC_PEI_DEBUG((EFI_D_INFO,"*PortNumber=%x\n", *PortNumber));
  return EFI_SUCCESS;
  
}


STATIC
EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
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
  USB3_HC_DEV             *Xhc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;

  Xhc       = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  Status    = EFI_SUCCESS;
  TotalPort = ((Xhc->HcSParams1 >> 24) & MAX_PORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = XHC_PORT_STAT_OFFSET + (0x10 * PortNumber);
  State   = XhcReadOpReg (Xhc, Offset);
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcReadOpReg_Start=%x\n", State));
  //State &= ~PORTSC_CHANGE_MASK;
  State = 0;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a ¡®1¡¯ to this flag.
    // A port may be disabled by software writing a ¡®1¡¯ to this flag.
    //
    break;

  case EfiUsbPortSuspend:
  	//
  	//When used then to add.
  	//
  	XHC_PEI_ERROR((EFI_D_INFO,"EfiUsbPortSuspend Not support and do deadloop!\n"));
    break;

  case EfiUsbPortReset:
    //
    // PORTSC_RESET BIT(4) bit is RW1S attribute, which means Write-1-to-set status: 
    // Register bits indicate status when read, a clear bit may be set by
    // writing a ¡®1¡¯. Writing a ¡®0¡¯ to RW1S bits has no effect..
    //
    break;

  case EfiUsbPortOwner:
    //
    // XHCI root hub port don't has the owner bit, ignore the operation
    //
    break;

  case EfiUsbPortConnectChange:
    //
    // Clear connect status change
    //
    State |= PORTSC_CONN_CHANGE;
    State = State >> 16;
    XHC_PEI_DEBUG((EFI_D_INFO,"PortConnectChange=%x\n", State));
    XhcWriteOpReg16 (Xhc, Offset + 2, State);
    break;

  case EfiUsbPortEnableChange:
    //
    // Clear enable status change
    //
    State |= PORTSC_ENABLE_CHANGE;
    State = State >> 16;
    XHC_PEI_DEBUG((EFI_D_INFO,"State=%x\n", State));
    XhcWriteOpReg16 (Xhc, Offset + 2, State);
    break;
    
  case EfiUsbPortWarmResetChange:
    //
    // Clear enable status change
    //
    State |= PORTSC_WARM_RESET_CHANGE;
    State = State >> 16;
    XHC_PEI_DEBUG((EFI_D_INFO,"State=%x\n", State));
    XhcWriteOpReg16 (Xhc, Offset + 2, State);
    break;
    
   case EfiUsbPortOverCurrentChange:
    //
    // Clear PortOverCurrent change
    //
    State |= PORTSC_OVERCUR_CHANGE;
    State = State >> 16;
    XHC_PEI_DEBUG((EFI_D_INFO,"State=%x\n", State));
    XhcWriteOpReg16 (Xhc, Offset, State);
    break;

  case EfiUsbPortResetChange:
    //
    // Clear connect status change
    //
    State |= PORTSC_RESET_CHANGE;
    State = State >> 16;
    XHC_PEI_DEBUG((EFI_D_INFO,"PortResetChange=%x\n", State));
    XhcWriteOpReg16 (Xhc, Offset + 2, State);
    break;
  case EfiUsbPortPower:
  case EfiUsbPortSuspendChange:
  
  	
    //
    // Not supported or not related operation
    //
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }
  State   = XhcReadOpReg (Xhc, Offset);
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcReadOpReg_End=%x\n", State));
ON_EXIT:
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcClearRootHubPortFeature: exit status %r\n", Status));
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
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
  USB3_HC_DEV             *Xhc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;
  UINT32               CheckLoop;
  UINT32                  CheckState;

  Xhc       = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  Status    = EFI_SUCCESS;
  
  TotalPort = ((Xhc->HcSParams1 >> 24) & MAX_PORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = (UINT32) (XHC_PORT_STAT_OFFSET + (0x10 * PortNumber));
  State   = XhcReadOpReg (Xhc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~PORTSC_CHANGE_MASK;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a ¡®1¡¯ to this flag.
    // A port may be disabled by software writing a ¡®1¡¯ to this flag.
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortSuspend:
    State |= PORTSC_LINK_WRT_STROB;
    XHC_PEI_DEBUG((EFI_D_INFO,"State=%x\n", State));
    XhcWriteOpReg (Xhc, Offset, State);
    State &= ~PORTSC_LINK_STATE_MASK;
    State |= (3 << 5) ;
    XHC_PEI_DEBUG((EFI_D_INFO,"State=%x\n", State));
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortReset:
    XHC_PEI_DEBUG((EFI_D_INFO,"EfiUsbPortReset!!!\n"));

    //
    // Make sure Host Controller not halt before reset it
    //
    if (XhcIsHalt (Xhc)) {
      Status = XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        XHC_PEI_ERROR((EFI_D_INFO,"XhcSetRootHubPortFeature :failed to start HC - %r\n", Status));
        break;
      }
    }

    //
    // 4.3.1 Resetting a Root Hub Port
    // 1) Write the PORTSC register with the Port Reset (PR) bit set to ¡®1¡¯.
    //
    State |= PORTSC_RESET;
    State &= ~PORTSC_ENABLED;
    XHC_PEI_DEBUG((EFI_D_INFO,"State1 =%x\n", State));
    XhcWriteOpReg (Xhc, Offset, State);    
    
    //
    //(2) Wait for a successful Port Status Change Event for the port, where the Port Reset Change (PRC)
    //bit in the PORTSC field is set to ¡®1¡¯.
    //
        for(CheckLoop=0;CheckLoop<0xFF;CheckLoop++)
        	{
               CheckState   = XhcReadOpReg (Xhc, Offset);
               if(CheckState &PORTSC_RESET_CHANGE)  //Reset change ocur
              	{
                     if((CheckState &PORTSC_RESET)==0) //Reset end 
                     break;
              	}
               MicroSecondDelay(XHC_SET_PORT_RESET_STALL); 
        	}
    XHC_PEI_DEBUG((EFI_D_INFO,"State2=%x\n", State));
    //
    // 4.3.2 Device Slot Assignment
    // The first operation that software shall perform after detecting a device attach event and resetting the port is
    // to obtain a Device Slot for the device by issuing an Enable Slot Command to the xHC through the
    // Command Ring. The Enable Slot Command returns a Slot ID that is selected by the host controller.
    //
    if (XHC_BIT_IS_SET (State, PORTSC_CONN)){
      
    }
    
    break;

  case EfiUsbPortPower:
    //
    // Not supported, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortOwner:
    //
    // XHCI root hub port don't has the owner bit, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcSetRootHubPortFeature: exit status %r\n", Status));

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
XhcGetHostControllerType (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  OUT UINT8                   * ControllerType
  )
{
      USB3_HC_DEV             *Xhc;
      EFI_STATUS                 Status;
      
      Status    = EFI_SUCCESS;
      Xhc       = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
      *ControllerType=PEI_XHCI_CONTROLLER;
      return Status;
}

STATIC
EFI_STATUS
EFIAPI
XhcGetRootHubPortStatus (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN  UINT8                 PortNumber,
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
  USB3_HC_DEV             *Xhc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINTN                   Index;
  UINTN                   MapSize;
  EFI_STATUS              Status;


  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc       = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = ((Xhc->HcSParams1 >> 24) & MAX_PORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                        = (UINT32) (XHC_PORT_STAT_OFFSET + (0x10 * PortNumber));
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  State                         = XhcReadOpReg (Xhc, Offset);
  XHC_PEI_DEBUG((EFI_D_INFO,"RootHubPort %x State=%x\n", PortNumber,State));
  //
  //filter the odd status and correct (USB3.0 :CS=1 ,PED=1,PLS=0 ,SPEED; USB2.0:CS=1 ,PED=1,PLS=0 ,SPEED!=4)
  //
  if(State==0x340)
  {
      XHC_PEI_ERROR((EFI_D_INFO,"clear port  power!\n"));
      XhcWriteOpReg (Xhc, Offset, 0); //clear power 
     // State                         = XhcReadOpReg (Xhc, Offset);
     // if((State&0x200)==0)
     // 	{
     // 	    XHC_DEBUG (("Set port power!\n"));
     //      XhcWriteOpReg (Xhc, Offset, 0x200); //re-set  power
     // 	}
  }

  State                         = XhcReadOpReg (Xhc, Offset);

  switch ((State & PORTSC_SPEED_MASK) >> 10) {
  case 1:
    PortStatus->PortStatus |= USB_PORT_STAT_FULL_SPEED; 
    break;
  case 2:
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;  
    break;
    
  case 3:
    PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;  
    break;
    
  case 4:
    PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;  
    break;
    
  default:
    break;
  } 

  //
  // Convert the XHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);
   //XHC_PEI_DUMP (PeiServices,"MapSize=%x\n", MapSize);
   
  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
    	//XHC_PEI_DUMP (PeiServices,"Index=%x\n", Index);
    	//XHC_PEI_DUMP (PeiServices," mUsbPortStateMap[Index].UefiState=%x\n",  mUsbPortStateMap[Index].UefiState);
    	//XHC_PEI_DUMP (PeiServices,"PortStatus->PortStatus=%x\n", PortStatus->PortStatus);
      PortStatus->PortStatus |= mUsbPortStateMap[Index].UefiState;
    }
  }
  //XHC_PEI_DUMP (PeiServices,"PortStatus->PortStatus2=%x\n", PortStatus->PortStatus);
  if ((State & PORTSC_LINK_STATE_MASK) >> 5 == 3) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }
  
  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus |= mUsbPortChangeMap[Index].UefiState;
    }
  }

ON_EXIT:
  return Status;
}


EFI_STATUS
EFIAPI
XhcControlTransfer (
  IN     EFI_PEI_SERVICES               **PeiServices,
  IN     PEI_USB2_HOST_CONTROLLER_PPI    *This,
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

  USB3_HC_DEV             *Xhc;
  URB                     *Urb;
  UINT8                   EpAddr;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  
  XHC_PEI_DEBUG((EFI_D_INFO,"xCtrl>\n"));
  
  Urb = NULL;
  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
  	 XHC_PEI_DEBUG((EFI_D_INFO,"xCtrl_1\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) &&
      ((Data != NULL) || (*DataLength != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) &&
     ((Data == NULL) || (*DataLength == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64) &&
      (MaximumPacketLength != 512)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

/*
  if ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength != 512)) {
  	XHC_PEI_DEBUG((EFI_D_INFO,"xCtrl_7\n"));
    return EFI_INVALID_PARAMETER;
  }
  */
  
    Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;
   RecoveryStatus  = EFI_SUCCESS;
  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHC_PEI_ERROR((EFI_D_INFO,"XhcControlTransfer: HC halted at entrance\n"));

    XhcAckAllInterrupt (Xhc);
    goto ON_EXIT;
  }

  XhcAckAllInterrupt (Xhc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  //
  // Encode the direction in address, although default control
  // endpoint is bidirectional. XhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  
  EpAddr = 0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0);
  
  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EpAddr,
          DeviceSpeed,
          0,
          MaximumPacketLength,
          Translator,
          XHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    XHC_PEI_DEBUG((EFI_D_INFO,"XhcControlTransfer: failed to create URB"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcExecTransfer (Xhc, Urb, TimeOut);

  //
  // Get the status from URB. The result is updated in XhcCheckUrbResult
  // which is called by XhcExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;
  
   if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }
  else
  {
     if(*TransferResult == EFI_USB_ERR_STALL)
     	{
             RecoveryStatus=XhcEndpointHaltRecovery(Xhc,Urb);
             XHC_PEI_ERROR((EFI_D_INFO,"Ctrl Halt RecoveryStatus %r!\n", RecoveryStatus));
     	}
     /*
     if(*TransferResult==EFI_USB_ERR_NOTEXECUTE)
     {
         XHC_PEI_ERROR((EFI_D_INFO,"Ctr No Execute Error occur\n"));
         RecoveryStatus=XhcEndpointNoExecuteRecovery(Xhc,Urb);
         XHC_PEI_ERROR ((EFI_D_INFO,"Ctr No Execute RecoveryStatus %r!\n",RecoveryStatus));
     }
     */
  }

  XhcAckAllInterrupt (Xhc);
  UsbHcFreeMem (Xhc->MemPool, Urb, sizeof (URB));

ON_EXIT:

  if (EFI_ERROR (Status)) {
    XHC_PEI_ERROR((EFI_D_INFO,"XhcControlTransfer: error - %r transfer %x\n", Status,*TransferResult));
  }
 XHC_PEI_DEBUG((EFI_D_INFO,"xCtrl<\n"));
   return Status;
 
}


STATIC
EFI_STATUS
EFIAPI
XhcBulkTransfer (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
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
                          On output, it is updated to to next data toggle to use                        of the subsequent bulk transfer.
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
  USB3_HC_DEV             *Xhc;
  URB                     *Urb;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  XHC_PEI_DEBUG((EFI_D_INFO,"xBulk>\n"));
  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512))) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  
  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;
  RecoveryStatus  = EFI_SUCCESS;
  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHC_PEI_ERROR((EFI_D_INFO,"XhcBulkTransfer: HC is halted\n"));

    XhcAckAllInterrupt (Xhc);
    goto ON_EXIT;
  }

  XhcAckAllInterrupt (Xhc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          *DataToggle,
          MaximumPacketLength,
          Translator,
          XHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    XHC_PEI_ERROR((EFI_D_INFO, "XhcBulkTransfer: failed to create URB\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcExecTransfer (Xhc, Urb, TimeOut);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;
  *DataToggle     = Urb->DataToggle;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }
  else
  {
      XHC_PEI_ERROR((EFI_D_INFO,"Error occur\n"));
      if(*TransferResult==EFI_USB_ERR_STALL)
      {
         XHC_PEI_ERROR((EFI_D_INFO,"BULK STALL Error occur\n"));
         RecoveryStatus=XhcEndpointHaltRecovery(Xhc,Urb);
         XHC_PEI_ERROR((EFI_D_INFO,"Bulk Halt RecoveryStatus %r!\n",RecoveryStatus));
      }
      /*
      	if(*TransferResult==EFI_USB_ERR_NOTEXECUTE)
      {
         XHC_PEI_ERROR((EFI_D_INFO,"BULK No Execute  Error occur\n"));
         RecoveryStatus=XhcEndpointNoExecuteRecovery(Xhc,Urb);
         XHC_PEI_ERROR((EFI_D_INFO,"Bulk No Execute RecoveryStatus %r!\n",RecoveryStatus));
      }
      */
  }
  XhcAckAllInterrupt (Xhc);
  UsbHcFreeMem (Xhc->MemPool, Urb, sizeof (URB));
ON_EXIT:

  if (EFI_ERROR (Status)) {
    XHC_PEI_ERROR((EFI_D_INFO,"XhcBulkTransfer: error - %r,transfer - %x\n", Status,*TransferResult));
  }
 XHC_PEI_DEBUG((EFI_D_INFO,"xBulk<\n"));
  return Status;
}



STATIC
EFI_STATUS
EFIAPI
XhcApplyDeviceSlot (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN USB_DEV_ROUTE                     RouteChart,     
  IN UINT8                                 DeviceSpeed,
  IN UINT8                                 ParentAddress,
  OUT  UINT8                               *DeviceAddress
  )
/*++

Routine Description:


Arguments:



Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
{
  EFI_STATUS            Status;
  USB3_HC_DEV             *Xhc;
  PEI_USB3_DEVICE      *Device;
  EVT_TRB_COMMAND       *EvtTrb;
  INPUT_CONTEXT			    *InputContxt;
  DEVICE_CONTEXT		    *OutputDevContxt;
  UINT32                PhyAddr;
  PEI_USB3_DEVICE            *ParentDevice;
  DEVICE_CONTEXT		    *ParentDevContxt=(DEVICE_CONTEXT*)0;
  
  //
  //(1)Device Slot apply 
  //
  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  
  Status = XhcSendCmd ( Xhc, TRB_TYPE_EN_SLOT, 0, 0, 0, 0,0,0);
  
  Status = XhcCheckEvent (
                Xhc,
                TRB_TYPE_COMMAND_COMPLT_EVENT,
                0,
                XHC_GENERIC_TIMEOUT,
                (TRB **) &EvtTrb
                );
  
  if (EFI_ERROR (Status)) {
    XHC_PEI_ERROR((EFI_D_INFO,"Device Slot apply fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
    return Status;
  }
  
  

 //
 //Allocate memory for PEI_USB3_DEVICE
 //
  Device = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (PEI_USB3_DEVICE));
  ZeroMem(Device, sizeof (PEI_USB3_DEVICE));
 
  Device->SlotID = (UINT8) EvtTrb->SlotID;
  Device->RouteChart=RouteChart;
  Device->Speed  =DeviceSpeed;
  Device->ParentSlotID=XhcAddressToSlotID(Xhc, ParentAddress);
  XHC_PEI_DEBUG((EFI_D_INFO,"Device->ParentSlotID=%x Slot=%x\n", Device->ParentSlotID,EvtTrb->SlotID));
  Xhc->Devices[Device->SlotID]=Device;
  //
  // (2)Device Slot Initialization
  // 1) Allocate an Input Context data structure (6.2.5) and initialize all fields to ¡®0¡¯.
  //

  InputContxt = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (INPUT_CONTEXT));
  ZeroMem(InputContxt, sizeof (INPUT_CONTEXT));
  Device->InputContxt = (VOID *) InputContxt;
  XHC_PEI_DEBUG((EFI_D_INFO,"InpCt=%x\n", Device->InputContxt)); 
  //
  // Initialize the Input Control Context (6.2.5.1) of the Input Context by setting the A0 and A1 flags to "1",
  //These flags indicate that the Slot Context and the Endpoint 0 Context of the Input Context are
  //affected by the command.
  //
  InputContxt->InputControlContext.Dword2 |= 0x3;
  //
  //Initialize the Input Slot Context data structure
  //
  InputContxt->Slot.RouteStr =Device->RouteChart.Route.RouteString; //zyq,add for bugbug3
  InputContxt->Slot.Speed = Device->Speed + 1; //
  InputContxt->Slot.ContextEntries = 1;
  InputContxt->Slot.RootHubPortNum =Device->RouteChart.Route.RootPortNum;//zyq,add for bugbug3
  
  
  //Special field for devices attached to Hub
  if(RouteChart.Route.RouteString) //indicate from Hub
  {
		  	 ParentDevContxt=(DEVICE_CONTEXT*)(UINTN)(Xhc->DCBAA[Device->ParentSlotID]);
		  	 ParentDevice=Xhc->Devices[Device->ParentSlotID];
		    //
		    //if the Full/Low device attached to a High Speed Hub ,Init the TTPortNum,TTHubSlotID
		    //
		    if((ParentDevContxt->Slot.TTPortNum==0)&&(ParentDevContxt->Slot.TTHubSlotID==0))
		    {
				  	if(((ParentDevice->Speed + 1)==3)&&((Device->Speed + 1)<3)) //
				  	{
				  		//Full/Low device attached to  High speed hub port that isolates the high speed signaling 
				  		//environment from Full/Low speed signaling environment for a device
					      InputContxt->Slot.TTPortNum =Device->ParentPort+1;
					      InputContxt->Slot.TTHubSlotID=Device->ParentSlotID;
					  }
			  }
			  else
			  {
			  	 //Inherit the TT parameters 
			  	 InputContxt->Slot.TTPortNum =ParentDevContxt->Slot.TTPortNum;
					 InputContxt->Slot.TTHubSlotID=ParentDevContxt->Slot.TTHubSlotID;
					 //if the device is a High speed device then down the speed to be the same as its parent Hub
					 if((Device->Speed + 1)==3)
					 {
					   InputContxt->Slot.Speed =ParentDevContxt->Slot.Speed;
					 }
					 
			  }
  }
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.RouteStr=0x%x\n", InputContxt->Slot.RouteStr));
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.Speed=0x%x\n", InputContxt->Slot.Speed));
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.ContextEntries=0x%x\n", InputContxt->Slot.ContextEntries));
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.RootHubPortNum=0x%x\n", InputContxt->Slot.RootHubPortNum));
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.TTPortNum=0x%x\n", InputContxt->Slot.TTPortNum));
	 XHC_PEI_DEBUG((EFI_D_INFO,"Slot.TTHubSlotID=0x%x\n", InputContxt->Slot.TTHubSlotID));
	  //
	  //Allocate and initialize the Transfer Ring for the Default Control Endpoint.
	  // ps. UsbAllocatePool is 64 bytes alignment
	  //
	  Status=XhcAllocateTransferRing(Xhc,Device,1,ED_CONTROL_BIDIR);
	  //
	  //5) Initialize the Input default control Endpoint 0 Context (6.2.3).
	  //
	  
	  InputContxt->EP[0].EPType = ED_CONTROL_BIDIR;
	  if (Device->Speed == EFI_USB_SPEED_SUPER) {
	    InputContxt->EP[0].MaxPacketSize = 512; // bugbug  
	  } else if(Device->Speed == EFI_USB_SPEED_HIGH){
	    InputContxt->EP[0].MaxPacketSize = 64; // bugbugzyq  
	  }else
	  {
	  	InputContxt->EP[0].MaxPacketSize = 8; // bugbug 
	  }
	  XHC_PEI_DEBUG((EFI_D_INFO,"InputContxt->EP[0].MaxPacketSize=0x%x\n", InputContxt->EP[0].MaxPacketSize)); 
	  
	  InputContxt->EP[0].AverageTRBLength = 0x1000; //bugbug: shall Refer to section 4.14.1.1 and the implementation note TRB Lengths and System Bus Bandwidth for more information
	  InputContxt->EP[0].MaxBurstSize = 0;
	  InputContxt->EP[0].Interval = 0;
	  InputContxt->EP[0].MaxPStreams = 0;
	  InputContxt->EP[0].Mult = 0;
	  InputContxt->EP[0].CErr = 3;
	  PhyAddr = XHC_LOW_32BIT (Device->EPRing[0].TransferRing);
	  PhyAddr &= ~(0x0f);
	  PhyAddr |= 0x01;//Init the DCS(dequeue cycle state) as the xHC's CCS
	  InputContxt->EP[0].PtrLo = PhyAddr;
	  InputContxt->EP[0].PtrHi = XHC_HIGH_32BIT (Device->EPRing[0].TransferRing);
	  //
	  //6) Allocate the Output Device Context data structure (6.2.1) and initialize it to ¡®0¡¯.
	  //
	  OutputDevContxt = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (DEVICE_CONTEXT));
    ZeroMem(OutputDevContxt, sizeof (DEVICE_CONTEXT));
	  
	  Device->OutputDevContxt = OutputDevContxt;
	   
	  XHC_PEI_DEBUG((EFI_D_INFO,"OtpCt=0x%x\n", Device->OutputDevContxt));  
	  //
	  // 7) Load the appropriate (Device Slot ID) entry in the Device Context Base Address Array (5.4.6) with
	  // a pointer to the Output Device Context data structure (6.2.1).
	  //
	  Xhc->DCBAA[Device->SlotID] = (UINT64) (UINTN) Device->OutputDevContxt;
	  //
	  //8) Issue an Address Device Command for the Device Slot, where the command points to the Input
	  //Context data structure described above.
	  //
	    Status = XhcSendCmd ( 
	                Xhc, 
	                TRB_TYPE_ADDRESS_DEV, 
	                Device->SlotID, 
	                XHC_LOW_32BIT (Device->InputContxt),
	                XHC_HIGH_32BIT (Device->InputContxt),
	                0,
	                0,
	                0
	                );

	                
	  Status = XhcCheckEvent (
	                Xhc,
	                TRB_TYPE_COMMAND_COMPLT_EVENT,
	                0,
	                XHC_GENERIC_TIMEOUT,
	                (TRB **) &EvtTrb
	                );
	  if (EFI_ERROR (Status)) {
      XHC_PEI_ERROR((EFI_D_INFO,"Address Device fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
	    return Status;
	  }
	  Device->Address  = (UINT8) ((DEVICE_CONTEXT *) Device->OutputDevContxt)->Slot.DeviceAddress;
	  Xhc->AddressToSlot[Device->Address]=Device->SlotID;
	  *DeviceAddress=Device->Address;
	  XHC_PEI_DEBUG((EFI_D_INFO,"Device address= %x\n", Device->Address));
	  
	  return Status;
}

EFI_STATUS
EFIAPI
XhcEvaluateContext (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN UINT8                                        DeviceAddress,
  IN BOOLEAN                  Slot,
  IN BOOLEAN                  EP0,
  IN UINT32                   MaxExitLatency,
  IN UINT32                   InterTarget,
  IN UINT32                   MaxPacketSize,
  IN UINT8                    PortNum
)
/*++

Routine Description:


Arguments:



Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
{
  EFI_STATUS            Status;
  USB3_HC_DEV             *Xhc;
  PEI_USB3_DEVICE      *Device;
  EVT_TRB_COMMAND       *EvtTrb;
  INPUT_CONTEXT			    *InputContxt;
  UINT8                        SlotID;
  
  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);
  
  if (!Slot && !EP0) {
    return EFI_SUCCESS;
  }


  SlotID = XhcAddressToSlotID(Xhc, DeviceAddress);
  
  Device = Xhc->Devices[SlotID];

  Device->MaxPacket0=MaxPacketSize; 
  InputContxt = Device->InputContxt;
  ZeroMem(InputContxt, sizeof (INPUT_CONTEXT));
  //
  //
  if (Slot) {
    InputContxt->InputControlContext.Dword2 |= 0x1;
    InputContxt->Slot.MaxExitLatency         = MaxExitLatency; 
    InputContxt->Slot.InterTarget            = InterTarget; 
    InputContxt->Slot.PortNum                =PortNum;
  }
  
  if (EP0) {
    InputContxt->InputControlContext.Dword2 |= 0x2;
    InputContxt->EP[0].MaxPacketSize = MaxPacketSize;
  }
  
  Status = XhcSendCmd ( 
                Xhc, 
                TRB_TYPE_EVALU_CONTXT, 
                Device->SlotID, 
                XHC_LOW_32BIT (Device->InputContxt),
                XHC_HIGH_32BIT (Device->InputContxt),
                0,
                0,
                0
                );
                
  Status = XhcCheckEvent (
                Xhc,
                TRB_TYPE_COMMAND_COMPLT_EVENT,
                0,
                XHC_GENERIC_TIMEOUT,
                (TRB **) &EvtTrb
                );
  if (EFI_ERROR (Status)) {
      XHC_PEI_ERROR((EFI_D_INFO,"Evaluate context fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
  }
  return Status;
}


EFI_STATUS
EFIAPI
XhcUpdateSlotContext(
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN UINT8                                        DeviceAddress,
  IN UINT8                   Hub,
  IN UINT8                   PortNum,
  IN UINT8                   TTT,
  IN UINT8                   MTT
)
/*++

Routine Description:


Arguments:



Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
{
	
  EFI_STATUS            Status;
  USB3_HC_DEV             *Xhc;
  PEI_USB3_DEVICE      *Device;
  UINT8                        SlotID;
  EVT_TRB_COMMAND       *EvtTrb;
  INPUT_CONTEXT			    *InputContxt;


  Status=EFI_SUCCESS;
  
  XHC_PEI_DEBUG((EFI_D_INFO,"UpdateSlotContext¡¡\n"));
  //
  //(1)Device Slot apply 
  //
  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);

  SlotID = XhcAddressToSlotID(Xhc, DeviceAddress);
  
  Device = Xhc->Devices[SlotID];
  

  InputContxt = Device->InputContxt;
  ZeroMem(InputContxt, sizeof (INPUT_CONTEXT));
  
  InputContxt->Slot.Hub=1;
  InputContxt->Slot.PortNum=PortNum;
  #if 0 
  //if Hub=1,and the speed ==High Speed, then TT think time and MTT shall be initialized
  if(InputContxt->Slot.Speed==3)
  {
  	 InputContxt->Slot.TTT= TTT;
  	 //If the High speed Hub support MTT then set to 1, do the same to the  device connect to the MTT Hub
  	 InputContxt->Slot.MTT= MTT; 
  }
  #endif
   Status = XhcSendCmd ( 
                Xhc, 
                TRB_TYPE_CON_ENDPOINT, 
                Device->SlotID, 
                XHC_LOW_32BIT (Device->InputContxt),
                XHC_HIGH_32BIT (Device->InputContxt),
                0,
                0,
                0
                );
                
  Status = XhcCheckEvent (
                Xhc,
                TRB_TYPE_COMMAND_COMPLT_EVENT,
                0,
                XHC_GENERIC_TIMEOUT,
                (TRB **) &EvtTrb
                );
  if (EFI_ERROR (Status)) {
      XHC_PEI_ERROR((EFI_D_INFO,"Configure endpoint fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));              
  }
  
  return Status;
}


EFI_STATUS
EFIAPI
XhcSetConfigCmd (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN UINT8                                        DeviceAddress,
  IN EFI_USB_ENDPOINT_DESCRIPTOR **EndPointDescriptors,
  IN UINT8                                    EndPointNum
)
/*++

Routine Description:


Arguments:



Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
{

  EFI_STATUS            Status;
  USB3_HC_DEV             *Xhc;
  PEI_USB3_DEVICE      *Device;
  UINT8                        SlotID;
  EVT_TRB_COMMAND       *EvtTrb;
  INPUT_CONTEXT			    *InputContxt;
  UINT32                PhyAddr;
  UINTN                   EpIndex;
  EFI_USB_ENDPOINT_DESCRIPTOR       *EpDesc;
  UINT8                   EpAddr;
  EFI_USB_DATA_DIRECTION   Direction;
  UINT8                   Dci;
  UINT8                   MaxDci=0;
  UINT8                   Interval;

  Status=EFI_SUCCESS;
  
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcSetConfigCmd!\n"));
  //
  //(1)Device Slot apply 
  //
  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS (This);

  SlotID = XhcAddressToSlotID(Xhc, DeviceAddress);
  
  Device = Xhc->Devices[SlotID];
  

  InputContxt = Device->InputContxt;
  ZeroMem(InputContxt, sizeof (INPUT_CONTEXT));
  

  
    
  for (EpIndex = 0; EpIndex < EndPointNum; EpIndex++) 
  {
      EpDesc = EndPointDescriptors[EpIndex];
      EpAddr      = EpDesc->EndpointAddress & 0x0F;
      Direction   = ((EpDesc->EndpointAddress & 0x80) ? EfiUsbDataIn : EfiUsbDataOut);
      Dci = XhcEPToDci (EpAddr, Direction);     
      if (Dci > MaxDci) {
        MaxDci = Dci;
      }
      XHC_PEI_DEBUG((EFI_D_INFO,"Dci = %x\n", Dci)); 
      InputContxt->InputControlContext.Dword2 |= (0x1 << Dci);
      InputContxt->EP[Dci-1].MaxPacketSize = EpDesc->MaxPacketSize;  
      if (Device->Speed == EFI_USB_SPEED_SUPER) {
        InputContxt->EP[Dci-1].MaxBurstSize = 0xf; //bugbug: 6.2.3.4, shall be set to the value defined in the bMaxBurst field of theSuperSpeed Endpoint Companion Descriptor.
      } else {
        InputContxt->EP[Dci-1].MaxBurstSize = 0x0; //bugbug: 6.2.3.4, Not right for For High-Speed isochronous
      }
      switch (USB_ENDPOINT_TYPE (EpDesc)) {
      case USB_ENDPOINT_BULK:
        if (Direction == EfiUsbDataIn) 
        	{
          InputContxt->EP[Dci-1].CErr = 0;
          InputContxt->EP[Dci-1].EPType = ED_BULK_IN;
          XHC_PEI_DEBUG((EFI_D_INFO,"ED_BULK_IN!\n"));
        } else 
        {
          InputContxt->EP[Dci-1].CErr = 0;
          InputContxt->EP[Dci-1].EPType = ED_BULK_OUT;
          XHC_PEI_DEBUG((EFI_D_INFO,"ED_BULK_OUT!\n"));
        }
        InputContxt->EP[Dci-1].AverageTRBLength = 0x1000; //bugbug: shall Refer to section 4.14.1.1 and the implementation note TRB Lengths and System Bus Bandwidth for more information
        break;


       case USB_ENDPOINT_INTERRUPT:
      	
        if (Direction == EfiUsbDataIn) 
        {
          InputContxt->EP[Dci-1].CErr = 1;
          InputContxt->EP[Dci-1].EPType = ED_INTERRUPT_IN;
        } 
        else
        	{
          InputContxt->EP[Dci-1].CErr = 3;
          InputContxt->EP[Dci-1].EPType = ED_INTERRUPT_OUT;
        }
        //Get the bInterval from descriptor and init the the interval field of endpoint context
        if((Device->Speed == EFI_USB_SPEED_FULL )
           ||(Device->Speed == EFI_USB_SPEED_LOW))
          {
           	Interval=EpDesc->Interval;
           
           InputContxt->EP[Dci-1].Interval=6;//bugbugzyq,Hard code the interval to MAX ,need calculated
          }
        else //0125_hub3.0>>
        	{
             if(Device->Speed == EFI_USB_SPEED_SUPER )
             	{
					           Interval=EpDesc->Interval;
					          
					           InputContxt->EP[Dci-1].Interval=0x0F;//bugbugzyq,Hard code the interval to MAX ,For usb3.0Hub
					           InputContxt->EP[Dci-1].AverageTRBLength=0x1000;
					           InputContxt->EP[Dci-1].MaxESITPayload=0x0002;
					           InputContxt->EP[Dci-1].MaxBurstSize = 0x0;
					           InputContxt->EP[Dci-1].CErr = 3;
             	}

        	}//0125_hub3.0<<
        break;
        
      case USB_ENDPOINT_ISO:
      case USB_ENDPOINT_CONTROL:
      default:
       XHC_PEI_DEBUG((EFI_D_INFO,"Abnormal EndPoint!!!\n"));
        break;
      }

      //
      //Allocate Ring for endpoint
      //
        if (Device->EPRing[Dci-1].TransferRing == NULL) {
        
            Status =XhcAllocateTransferRing(Xhc,Device,Dci,InputContxt->EP[Dci-1].EPType);
        }
        
        PhyAddr = XHC_LOW_32BIT (Device->EPRing[Dci-1].TransferRing);
        PhyAddr &= ~(0x0f);
        PhyAddr |= Device->EPRing[Dci-1].TransferRingPCS;
        InputContxt->EP[Dci-1].PtrLo = PhyAddr;
        InputContxt->EP[Dci-1].PtrHi = XHC_HIGH_32BIT (Device->EPRing[Dci-1].TransferRing);
      
    }
  
  InputContxt->InputControlContext.Dword2 |= 0x1;
  InputContxt->Slot.ContextEntries = MaxDci;
  
  XHC_PEI_DEBUG((EFI_D_INFO,"XhcSetConfigCmd-InputContxt = %x\n", InputContxt)); 
  if(MaxDci>0)
  {
        Status = XhcSendCmd ( 
                      Xhc, 
                      TRB_TYPE_CON_ENDPOINT, 
                      Device->SlotID, 
                      XHC_LOW_32BIT (Device->InputContxt),
                      XHC_HIGH_32BIT (Device->InputContxt),
                      0,
                      0,
                      0
                      );
                      
        Status = XhcCheckEvent (
                      Xhc,
                      TRB_TYPE_COMMAND_COMPLT_EVENT,
                      0,
                      XHC_GENERIC_TIMEOUT,
                      (TRB **) &EvtTrb
                      );
         if (EFI_ERROR (Status)) {
             XHC_PEI_ERROR((EFI_D_INFO,"configure endpoints fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
      }              
  	}
  return Status;
}


EFI_STATUS
InitializeUsbHC (
  IN USB3_HC_DEV          *XhcDev  
  )
/*++

Routine Description:

  Initialize XhcI

Arguments:

  PeiServices - EFI_PEI_SERVICES
  This        - PEI_USB2_HOST_CONTROLLER_PPI
  XhcDev      - USB_XHC_DEV

Returns:

  EFI_SUCCESS           - Success

--*/
{
  EFI_STATUS  Status;
 	
  XhcResetHC (XhcDev, XHC_RESET_TIMEOUT);

  Status = XhcInitHC (XhcDev);

  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;      
  }
  
  return EFI_SUCCESS;
}

