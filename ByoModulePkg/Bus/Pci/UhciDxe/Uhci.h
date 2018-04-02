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
  Uhci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _UHCI_H
#define _UHCI_H

#include <Uefi.h>    
#include <Library/UefiLib.h> 
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/LegacyUsbInf.h>
#include <IndustryStandard/pci22.h>
#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Library/UefiBootServicesTableLib.h>

#include "ComponentName.h"

typedef struct _USB_HC_DEV  USB_HC_DEV;  

enum {
  CLASSC_OFFSET                 = 0x09,
  USBBASE_OFFSET                = 0x20,
  USB_BAR_INDEX                 = 4,
  PCI_CLASSC_PI_UHCI            = 0x00,

  UHC_1_MICROSECOND             = 1,
  UHC_1_MILLISECOND             = 1000 * UHC_1_MICROSECOND,
  UHC_1_SECOND                  = 1000 * UHC_1_MILLISECOND,

  //
  // UHCI register operation timeout, set by experience
  //
  UHC_GENERIC_TIMEOUT           = UHC_1_SECOND,

  //
  // Wait for force global resume(FGR) complete, refers to
  // specification[UHCI11-2.1.1]
  //
  UHC_FORCE_GLOBAL_RESUME_STALL = 20 * UHC_1_MILLISECOND,

  //
  // Wait for roothub port reset and recovery, reset stall
  // is set by experience, and recovery stall refers to
  // specification[UHCI11-2.1.1]
  //
  UHC_ROOT_PORT_RESET_STALL     = 50 * UHC_1_MILLISECOND,
  UHC_ROOT_PORT_RECOVERY_STALL  = 10 * UHC_1_MILLISECOND,

  //
  // Sync and Async transfer polling interval, set by experience,
  // and the unit of Async is 100us.
  //
  UHC_SYNC_POLL_INTERVAL        = 50 * UHC_1_MICROSECOND,
  UHC_ASYNC_POLL_INTERVAL       = 50 * 10000UL,

  //
  // UHC raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
  UHCI_TPL                      = EFI_TPL_NOTIFY,

  USB_HC_DEV_SIGNATURE          = SIGNATURE_32 ('u', 'h', 'c', 'i'),
};

#pragma pack(1)
typedef struct{
  UINT16   Function:3;
  UINT16   Device:5;
  UINT16   Bus:8;
} EFI_PCI_FUNCTION_ADDRESS;

#pragma pack()

#pragma pack(1)
typedef struct {
  UINT8               PI;
  UINT8               SubClassCode;
  UINT8               BaseCode;
} USB_CLASSC;
#pragma pack()

#define UHC_FROM_USB_HC_PROTO(This)   CR(This, USB_HC_DEV, UsbHc, USB_HC_DEV_SIGNATURE)
#define UHC_FROM_USB2_HC_PROTO(This)  CR(This, USB_HC_DEV, Usb2Hc, USB_HC_DEV_SIGNATURE)

//
// USB_HC_DEV support the UHCI hardware controller. It schedules
// the asynchronous interrupt transfer with the same method as
// EHCI: a reversed tree structure. For synchronous interrupt,
// control and bulk transfer, it uses three static queue head to
// schedule them. SyncIntQh is for interrupt transfer. LsCtrlQh is
// for LOW speed control transfer, and FsCtrlBulkQh is for FULL
// speed control or bulk transfer. This is because FULL speed contrl
// or bulk transfer can reclaim the unused bandwidth. Some USB
// device requires this bandwidth reclamation capability.
//
typedef struct _USB_HC_DEV {
  UINT32                    Signature;
  EFI_USB_HC_PROTOCOL       UsbHc;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  VOID                      *SmmUhc;
  VOID                      *SmmUsbHc;
  VOID                      *SmmUsb2Hc;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
} USB_HC_DEV;

extern EFI_DRIVER_BINDING_PROTOCOL  gUhciDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL gUhciComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL  gUhciComponentName;


#endif
