/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  VirtualKbc.h

Abstract:
  

Revision History:

**/
#ifndef _EFI_USB_VIRTUAL_KBC_H_
#define _EFI_USB_VIRTUAL_KBC_H_

//
//SMM USB SERVICE Protocol GUID.
//

#define EFI_USB_VIRTUAL_KBC_PROTOCOL_GUID { \
  0x87d53dc2, 0x3e20, 0x459f, 0x88, 0x2d, 0x64, 0x34, 0x39, 0x44, 0x84, 0xb0}

typedef struct _EFI_USB_VIRTUAL_KBC_PROTOCOL    EFI_USB_VIRTUAL_KBC_PROTOCOL;
//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiUsbVirtualKbcProtocolGuid;

#define VIRTUAL_KBC_NOTRAPORSTATUS  0x0
#define VIRTUAL_KBC_PORT64_WRITE    0x08
#define VIRTUAL_KBC_PORT64_READ     0x04
#define VIRTUAL_KBC_PORT60_WRITE    0x02
#define VIRTUAL_KBC_PORT60_READ     0x01

typedef enum {
  VirtualKbcOn = 1,
  VirtualKbcOff,
  VirtualKbcCheckStatus,
  VirtualKbcClearStatus
} EFI_USB_VIRTUAL_KBC_CONTROL_TYPE;

typedef enum {
  VirtualKbcIrq1 = 1,
  VirtualKbcIrq12
} EFI_USB_VIRTUAL_KBC_IRQ_TYPE;

typedef enum {
  VirtualKbcReadData = 1,
  VirtualKbcWriteData,
  VirtualKbcReadStatus
} EFI_USB_VIRTUAL_KBC_DATA_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_USB_VIRTUAL_KBC_CALLBACK) (
  IN EFI_USB_VIRTUAL_KBC_PROTOCOL *This,
  IN VOID                         *Pointer
 );

typedef
EFI_STATUS
(EFIAPI *EFI_USB_VIRTUAL_KBC_CONTROL) (
  IN EFI_USB_VIRTUAL_KBC_PROTOCOL     *This,
  IN EFI_USB_VIRTUAL_KBC_CONTROL_TYPE VirtualKbcControlType,
  IN UINT8                            *Data
 );

typedef
EFI_STATUS
(EFIAPI *EFI_USB_VIRTUAL_KBC_IRQ) (
  IN EFI_USB_VIRTUAL_KBC_PROTOCOL *This,
  IN EFI_USB_VIRTUAL_KBC_IRQ_TYPE VirtualKbcIrqType
 );

typedef
EFI_STATUS
(EFIAPI *EFI_USB_VIRTUAL_KBC_DATA) (
  IN EFI_USB_VIRTUAL_KBC_PROTOCOL  *This,
  IN EFI_USB_VIRTUAL_KBC_DATA_TYPE VirtualKbcDataType,
  IN UINT8                         *VirtualKbcData
 );

//
// EFI USB Virtual KBC Protocol.
//

typedef struct _EFI_USB_VIRTUAL_KBC_PROTOCOL {
  EFI_USB_VIRTUAL_KBC_CALLBACK UsbVirtualKbcCallBack;
  EFI_USB_VIRTUAL_KBC_CONTROL  UsbVirtualKbcControl;
  EFI_USB_VIRTUAL_KBC_IRQ      UsbVirtualKbcIrq;
  EFI_USB_VIRTUAL_KBC_DATA     UsbVirtualKbcData;
} EFI_USB_VIRTUAL_KBC_PROTOCOL;

#endif
