/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  UsbBus.h

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
  UsbBus.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_USB_BUS_H_
#define _EFI_USB_BUS_H_   

#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ComponentName.h>

//
//  CONTAINING_RECORD - returns a pointer to the structure
//      from one of it's elements.
//
#define _CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))


enum {
  USB_MAX_LANG_ID               = 16,
  USB_MAX_INTERFACE             = 16,
  USB_MAX_DEVICES               = 128,

  USB_BUS_1_MILLISECOND         = 1000,

  //
  // Roothub and hub's polling interval, set by experience,
  // The unit of roothub is 100us, means 1s as interval, and
  // the unit of hub is 1ms, means 64ms as interval.
  //
  USB_ROOTHUB_POLL_INTERVAL     = 1000 * 10000U,
  USB_HUB_POLL_INTERVAL         = 64,

  //
  // Wait for port stable to work, refers to specification
  // [USB20-9.1.2]
  //
  USB_WAIT_PORT_STABLE_STALL     = 100 * USB_BUS_1_MILLISECOND,

  //
  // Wait for port statue reg change, set by experience
  //
  USB_WAIT_PORT_STS_CHANGE_STALL = 5 * USB_BUS_1_MILLISECOND,

  //
  // Wait for set device address, refers to specification
  // [USB20-9.2.6.3, it says 2ms]
  //
  USB_SET_DEVICE_ADDRESS_STALL   = 20 * USB_BUS_1_MILLISECOND,

  //
  // Wait for retry max packet size, set by experience
  //
  USB_RETRY_MAX_PACK_SIZE_STALL  = 100 * USB_BUS_1_MILLISECOND,

  //
  // Wait for hub port power-on, refers to specification
  // [USB20-11.23.2]
  //
  USB_SET_PORT_POWER_STALL       = 2 * USB_BUS_1_MILLISECOND,

  //
  // Wait for port reset, refers to specification
  // [USB20-7.1.7.5, it says 10ms for hub and 50ms for
  // root hub]
  //
  USB_SET_PORT_RESET_STALL       = 20 * USB_BUS_1_MILLISECOND,
  USB_SET_ROOT_PORT_RESET_STALL  = 50 * USB_BUS_1_MILLISECOND,

  //
  // Wait for clear roothub port reset, set by experience
  //
  USB_CLR_ROOT_PORT_RESET_STALL  = 1 * USB_BUS_1_MILLISECOND,

  //
  // Wait for set roothub port enable, set by experience
  //
  USB_SET_ROOT_PORT_ENABLE_STALL = 20 * USB_BUS_1_MILLISECOND,

  //
  // Send general device request timeout, refers to
  // specification[USB20-11.24.1]
  //
  USB_GENERAL_DEVICE_REQUEST_TIMEOUT = 50 * USB_BUS_1_MILLISECOND,

  //
  // Send clear feature request timeout, set by experience
  //
  USB_CLEAR_FEATURE_REQUEST_TIMEOUT  = 10 * USB_BUS_1_MILLISECOND,

  //
  // Bus raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
  USB_BUS_TPL                   = TPL_NOTIFY,

  USB_INTERFACE_SIGNATURE       = SIGNATURE_32 ('U', 'S', 'B', 'I'),
  USB_BUS_SIGNATURE             = SIGNATURE_32 ('U', 'S', 'B', 'B'),
};

#define USB_BIT(a)                  ((UINTN)(1 << (a)))
#define USB_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EFI_USB_BUS_PROTOCOL_GUID \
          {0x2B2F68CC, 0x0CD2, 0x44cf, 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75}

#define USB_INTERFACE_FROM_USBIO(a) \
          CR(a, NATIVE_USBIF, UsbIo, USB_INTERFACE_SIGNATURE)

#define USB_BUS_FROM_THIS(a) \
          CR(a, USB_BUS, BusId, USB_BUS_SIGNATURE)

//
// Used to locate USB_BUS
//
typedef struct _EFI_USB_BUS_PROTOCOL {
  UINT64                    Reserved;
} EFI_USB_BUS_PROTOCOL;

#define NEW_INTERFACE       0
#define OLD_INTERFACE       1
#define EMPTY_INTERFACE     2
#define INVALID_INTERFACE   3



//
// Stands for different functions of USB device
//
typedef struct _NATIVE_USBIF {
  UINTN                     Signature;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     UsbIfState;
  VOID                      *LegacyUsbIo;
  LIST_ENTRY                List;
  EFI_USB_IO_PROTOCOL       UsbIo;
} NATIVE_USBIF;

//
// Stands for the current USB Bus
//
typedef struct _USB_BUS {
  UINTN                     Signature;
  EFI_USB_BUS_PROTOCOL      BusId;

  //
  // Managed USB host controller
  //
  EFI_HANDLE                HostHandle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_USB2_HC_PROTOCOL      *Usb2Hc;
  EFI_USB_HC_PROTOCOL       *UsbHc;
} USB_BUS;

#define USB_US_LAND_ID   0x0409

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

extern EFI_USB_IO_PROTOCOL           mUsbIoProtocol;
extern EFI_DRIVER_BINDING_PROTOCOL   mUsbBusDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbBusComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbBusComponentName;


#endif
