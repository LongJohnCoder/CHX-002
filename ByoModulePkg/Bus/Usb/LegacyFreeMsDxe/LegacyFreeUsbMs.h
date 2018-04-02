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
  LegacyFreeMs.h

Abstract:


Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_MOUSE_H
#define _USB_MOUSE_H


#include <Uefi.h>

#include <Protocol/SimplePointer.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/IsaIo.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiUsbLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Usb.h>
#include "ComponentName.h"

#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_MOUSE          2

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

#define USB_MOUSE_DEV_SIGNATURE SIGNATURE_32 ('u', 'm', 'o', 'u')


typedef struct {
  BOOLEAN ButtonDetected;
  UINT8   ButtonMinIndex;
  UINT8   ButtonMaxIndex;
  UINT8   Reserved;
} PRIVATE_DATA;

typedef struct {
  UINTN                         Signature;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_EVENT                     DelayedRecoveryEvent;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   *IntEndpointDescriptor;
  UINT8                         NumberOfButtons;
  INT32                         XLogicMax;
  INT32                         XLogicMin;
  INT32                         YLogicMax;
  INT32                         YLogicMin;
  EFI_SIMPLE_POINTER_PROTOCOL   SimplePointerProtocol;
  EFI_SIMPLE_POINTER_STATE      State;
  EFI_SIMPLE_POINTER_MODE       Mode;
  BOOLEAN                       StateChanged;
  PRIVATE_DATA                  PrivateData;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
} USB_MOUSE_DEV;

#define USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL(a) \
    CR(a, USB_MOUSE_DEV, SimplePointerProtocol, USB_MOUSE_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gLegacyFreeMsDriverBinding;

extern EFI_COMPONENT_NAME2_PROTOCOL gLegacyFreeMsComponentName2;

extern EFI_COMPONENT_NAME_PROTOCOL  gLegacyFreeMsComponentName;

extern EFI_GUID                     gEfiLegacyFreeMsDriverGuid;

#endif
