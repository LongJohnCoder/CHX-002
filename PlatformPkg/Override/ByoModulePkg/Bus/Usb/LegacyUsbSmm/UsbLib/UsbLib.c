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
  UsbLib.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "UsbLib.h"
#include "UsbMem.h"
#include "UsbUtility.h"

EFI_STATUS
UsbGetDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    IN  UINT16                  Index,
    IN  UINT16                  DescriptorLength,
    OUT VOID                    *Descriptor,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Get Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to contain result
  Status            - Transfer Status
Returns:
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    UINT8                   *UsbBuf;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    if ((DescriptorLength != 0) && (Descriptor != NULL)) {
        UsbBuf = UsbAllocatePool (DescriptorLength);
        if (UsbBuf == NULL) {
            UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
            return EFI_OUT_OF_RESOURCES;
        }
    }
    else
        UsbBuf = Descriptor;


    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_GET_DESCRIPTOR_REQ_TYPE;
    DevReq->Request      = USB_REQ_GET_DESCRIPTOR;
    DevReq->Value        = Value;
    DevReq->Index        = Index;
    DevReq->Length       = DescriptorLength;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 DescriptorLength,
                 Status
             );

    if ((DescriptorLength != 0) && (Descriptor != NULL)) {
        if (!EFI_ERROR(Result))
            CopyMem (Descriptor, UsbBuf, DescriptorLength);
        UsbFreePool (UsbBuf, DescriptorLength);
    }

    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

//
// Set Device Descriptor
//
EFI_STATUS
UsbSetDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    IN  UINT16                  Index,
    IN  UINT16                  DescriptorLength,
    IN  VOID                    *Descriptor,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Set Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to set
  Status            - Transfer Status
Returns:
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    UINT8                   *UsbBuf;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    if ((DescriptorLength != 0) && (Descriptor != NULL)) {
        UsbBuf = UsbAllocatePool (DescriptorLength);
        if (UsbBuf == NULL) {
            UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
            return EFI_OUT_OF_RESOURCES;
        }
        CopyMem (UsbBuf, Descriptor, DescriptorLength);
    }
    else
        UsbBuf = Descriptor;

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_SET_DESCRIPTOR_REQ_TYPE;
    DevReq->Request      = USB_REQ_SET_DESCRIPTOR;
    DevReq->Value        = Value;
    DevReq->Index        = Index;
    DevReq->Length       = DescriptorLength;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbDataOut,
                 TIMEOUT_VALUE,
                 Descriptor,
                 DescriptorLength,
                 Status
             );
    if ((DescriptorLength != 0) && (Descriptor != NULL)) {
        UsbFreePool (UsbBuf, DescriptorLength);
    }

    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbGetInterface (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Index,
    OUT UINT8                   *AltSetting,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Get Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  Index       - Interface index value
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response


--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    UINT8                   *UsbBuf;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (1);
    if (UsbBuf == NULL) {
        UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_GET_INTERFACE_REQ_TYPE;
    DevReq->Request      = USB_REQ_GET_INTERFACE;
    DevReq->Index        = Index;
    DevReq->Length       = 1;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 1,
                 Status
             );

    *AltSetting = *UsbBuf;
    UsbFreePool (UsbBuf, 1);
    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));

    return Result;
}

EFI_STATUS
UsbSetInterface (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  InterfaceNo,
    IN  UINT16                  AltSetting,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Set Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  InterfaceNo - Interface Number
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_SET_INTERFACE_REQ_TYPE;
    DevReq->Request      = USB_REQ_SET_INTERFACE;
    DevReq->Value        = AltSetting;
    DevReq->Index        = InterfaceNo;


    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 Status
             );

    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbGetConfiguration (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    OUT UINT8                   *ConfigValue,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Get Device Configuration

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  ConfigValue - Config Value
  Status      - Transfer Status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    UINT8                   *UsbBuf;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (1);
    if (UsbBuf == NULL) {
        UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_GET_CONFIGURATION_REQ_TYPE;
    DevReq->Request      = USB_REQ_GET_CONFIG;
    DevReq->Length       = 1;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 1,
                 Status
             );

    *ConfigValue = *UsbBuf;
    UsbFreePool (UsbBuf, 1);
    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));

    return Result;
}

EFI_STATUS
UsbSetConfiguration (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Set Device Configuration

Arguments:

  UsbIo   - EFI_USB_IO_PROTOCOL
  Value   - Configuration Value to set
  Status  - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq->RequestType  = USB_DEV_SET_CONFIGURATION_REQ_TYPE;
    DevReq->Request      = USB_REQ_SET_CONFIG;
    DevReq->Value        = Value;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 Status
             );
    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));

    return Result;
}

EFI_STATUS
UsbSetFeature (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Value,
    IN  UINT16                  Target,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Set Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    switch (Recipient) {

    case USB_TARGET_DEVICE:
        DevReq->RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_D;
        break;

    case USB_TARGET_INTERFACE:
        DevReq->RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_I;
        break;

    case USB_TARGET_ENDPOINT:
        DevReq->RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_E;
        break;
    }
    //
    // Fill device request, see USB1.1 spec
    //
    DevReq->Request  = USB_REQ_SET_FEATURE;
    DevReq->Value    = Value;
    DevReq->Index    = Target;


    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 Status
             );
    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbClearFeature (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Value,
    IN  UINT16                  Target,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Clear Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    switch (Recipient) {

    case USB_TARGET_DEVICE:
        DevReq->RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_D;
        break;

    case USB_TARGET_INTERFACE:
        DevReq->RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_I;
        break;

    case USB_TARGET_ENDPOINT:
        DevReq->RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_E;
        break;
    }
    //
    // Fill device request, see USB1.1 spec
    //
    DevReq->Request  = USB_REQ_CLEAR_FEATURE;
    DevReq->Value    = Value;
    DevReq->Index    = Target;


    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 Status
             );

    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbGetStatus (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Target,
    OUT UINT16                  *DevStatus,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Usb Get Device Status

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Target    - Request index
  DevStatus - Device status
  Status    - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response

--*/
{
    EFI_USB_DEVICE_REQUEST  *DevReq;
    UINT8                   *UsbBuf;
    EFI_STATUS              Result;

    if (UsbIo == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    DevReq = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (DevReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (2);
    if (UsbBuf == NULL) {
        UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    switch (Recipient) {

    case USB_TARGET_DEVICE:
        DevReq->RequestType = USB_DEV_GET_STATUS_REQ_TYPE_D;
        break;

    case USB_TARGET_INTERFACE:
        DevReq->RequestType = USB_DEV_GET_STATUS_REQ_TYPE_I;
        break;

    case USB_TARGET_ENDPOINT:
        DevReq->RequestType = USB_DEV_GET_STATUS_REQ_TYPE_E;
        break;
    }
    //
    // Fill device request, see USB1.1 spec
    //
    DevReq->Request  = USB_REQ_GET_STATUS;
    DevReq->Value    = 0;
    DevReq->Index    = Target;
    DevReq->Length   = 2;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 DevReq,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 2,
                 Status
             );

    CopyMem (DevStatus, UsbBuf, 2);
    UsbFreePool (UsbBuf, 2);
    UsbFreePool (DevReq, sizeof(EFI_USB_DEVICE_REQUEST));

    return Result;
}

EFI_STATUS
UsbClearEndpointHalt (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT8                   EndpointNo,
    OUT UINT32                  *Status
)
/*++

Routine Description:

  Clear endpoint stall

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  EndpointNo  - Endpoint Number
  Status      - Transfer Status

Returns:

  EFI_NOT_FOUND    - Can't find the Endpoint
  EFI_DEVICE_ERROR - Hardware error
  EFI_SUCCESS      - Success

--*/
{
    EFI_STATUS                    Result;
    EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
    EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
    UINT8                         Index;

    ZeroMem (&EndpointDescriptor, sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
    //
    // First seach the endpoint descriptor for that endpoint addr
    //
    Result = UsbIo->UsbGetInterfaceDescriptor (
                 UsbIo,
                 &InterfaceDescriptor
             );
    if (EFI_ERROR (Result)) {
        return Result;
    }

    for (Index = 0; Index < InterfaceDescriptor.NumEndpoints; Index++) {
        Result = UsbIo->UsbGetEndpointDescriptor (
                     UsbIo,
                     Index,
                     &EndpointDescriptor
                 );
        if (EFI_ERROR (Result)) {
            continue;
        }

        if (EndpointDescriptor.EndpointAddress == EndpointNo) {
            break;
        }
    }

    if (Index == InterfaceDescriptor.NumEndpoints) {
        //
        // No such endpoint
        //
        return EFI_NOT_FOUND;
    }

    Result = UsbClearFeature (
                 UsbIo,
                 USB_TARGET_ENDPOINT,
                 EfiUsbEndpointHalt,
                 EndpointDescriptor.EndpointAddress,
                 Status
             );

    return Result;
}



VOID *GetUsbPciDevPath(UINT8 Bus, UINT8 Dev, UINT8 Fun)
{
  STATIC FULL_USB_DEVICE_PATH      gUsbDevPath = USB_DEVICE_PATH_TEMPLATE;  
  STATIC ACPI_HID_DEVICE_PATH      gRbDevPath  = ACPI_PATH;
  STATIC PCI_DEVICE_PATH           gPciDp = PCI_USB_PATH(0, 0);
  STATIC EFI_DEVICE_PATH_PROTOCOL  gEndDp = END_PATH;
         FULL_USB_DEVICE_PATH      *TempDp;
         EFI_DEVICE_PATH_PROTOCOL  *Dp = NULL;
         UINT64                    PciAddr;
         UINT8                     MyBus, MyDev, MyFun;
         UINT32                    Data32;
         UINT8                     *Data8;
         UINTN                     Size;


  USB_DEBUG((EFI_D_INFO, __FUNCTION__"(%X,%X,%X)\n", Bus, Dev, Fun));

  if(Bus == 0){       
    Dp = AllocatePool(sizeof(gUsbDevPath));
    if (Dp == NULL){
      goto ProcExit;
    }
    CopyMem(Dp, &gUsbDevPath, sizeof(gUsbDevPath)); 
    TempDp = (FULL_USB_DEVICE_PATH*)Dp;
    TempDp->Pci.Device   = Dev;
    TempDp->Pci.Function = Fun;
    
  } else {
    Dp = AllocatePool(sizeof(gRbDevPath));
    if (Dp == NULL){
      goto ProcExit;
    }
    CopyMem(Dp, &gRbDevPath, sizeof(gRbDevPath)); 
  
    Data8 = (UINT8*)&Data32;
    Size  = sizeof(gRbDevPath);
    
    for(MyBus=0;MyBus<Bus;MyBus++){
      for(MyDev=0;MyDev<32;MyDev++){
        for(MyFun=0;MyFun<8;MyFun++){

//-       USB_DEBUG((EFI_D_INFO, "PCI(%X,%X,%X)\n", MyBus, MyDev, MyFun));
          
          PciAddr = SMM_PCI_ADDRESS(MyBus, MyDev, MyFun, 0);
          SmmPciCfgRead(gSmst, EfiPciIoWidthUint32, (SMM_PCI_IO_ADDRESS*)&PciAddr, &Data32);
          if((UINT16)Data32 == 0xFFFF){
            if(MyFun == 0){
              MyFun = 7;
            }
            continue;
          }

          PciAddr = SMM_PCI_ADDRESS(MyBus, MyDev, MyFun, 8);
          SmmPciCfgRead(gSmst, EfiPciIoWidthUint32, (SMM_PCI_IO_ADDRESS*)&PciAddr, &Data32);
          if(Data8[3] == 6 && Data8[2] == 4){
            PciAddr = SMM_PCI_ADDRESS(MyBus, MyDev, MyFun, 0x18);
            SmmPciCfgRead(gSmst, EfiPciIoWidthUint32, (SMM_PCI_IO_ADDRESS*)&PciAddr, &Data32);
//-         USB_DEBUG((EFI_D_INFO, "Bridge(%X,%X)\n", Data8[1], Data8[2]));
            if(Bus >= Data8[1] && Bus <= Data8[2]){
              Dp = ReallocatePool(Size, Size+sizeof(PCI_DEVICE_PATH), Dp);
              if (Dp == NULL){goto ProcExit;}
              gPciDp.Device   = MyDev;
              gPciDp.Function = MyFun;
              CopyMem((UINT8*)Dp+Size, &gPciDp, sizeof(gPciDp));
              Size += sizeof(PCI_DEVICE_PATH);
              USB_DEBUG((EFI_D_INFO, "+PCI(%X,%X)\n", MyDev, MyFun));
              
              MyBus = Data8[1] - 1;
              MyDev = 31;
              MyFun = 7;
              continue;
            }
          }

          if(MyFun == 0){
            PciAddr = SMM_PCI_ADDRESS(MyBus, MyDev, MyFun, 0xE);
            SmmPciCfgRead(gSmst, EfiPciIoWidthUint8, (SMM_PCI_IO_ADDRESS*)&PciAddr, &Data32);
            if((UINT8)Data32 & BIT7){
              MyFun = 7;
            }              
          }
          
        }
      }
    }

    Dp = ReallocatePool(
           Size, 
           Size+sizeof(PCI_DEVICE_PATH)+sizeof(EFI_DEVICE_PATH_PROTOCOL), 
           Dp
           );
    if (Dp == NULL){goto ProcExit;}
    gPciDp.Device   = Dev;
    gPciDp.Function = Fun;
    CopyMem((UINT8*)Dp+Size, &gPciDp, sizeof(gPciDp)); 
    CopyMem((UINT8*)Dp+Size+sizeof(gPciDp), &gEndDp, sizeof(gEndDp));
   
  }

ProcExit:
  return Dp;
}


