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
  Ohci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/
#ifndef _OHCI_H
#define _OHCI_H


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

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

enum {
  //
  // PCI Configuration Registers
  //
  CLASSC_OFFSET            = 0x09,
  OHC_BAR_INDEX            = 0,       //
  OHC_PCI_MEM_BASE_ADDRESS = 0x10,    // Memory Bar register address
  OHC_PCI_MEM_BASE_MASK    = 0x0ffffff00, // Memory address MASK

  PCI_CLASSC_PI_OHCI       = 0x10,
  
  //
  // OHCI time out define
  //
  OHC_1_MICROSECOND             = 1,
  OHC_1_MILLISECOND             = 1000 * OHC_1_MICROSECOND,
  OHC_1_SECOND                  = 1000 * OHC_1_MILLISECOND,

  //
  // OHCI register operation timeout, set by experience
  //
  OHC_PORT_RESET_RECOVERY_STALL = 8 * OHC_1_MILLISECOND,
  OHC_SYNC_POLL_INTERVAL        = 50 * OHC_1_MICROSECOND,
  
  USB_OHCI_DEV_SIGNATURE        = SIGNATURE_32 ('o', 'h', 'c', 'i'),  
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

#define OHC_FROM_USB_HC_PROTO(This)   CR(This, USB_OHCI_DEV, UsbHc, USB_OHCI_DEV_SIGNATURE)
#define OHC_FROM_USB2_HC_PROTO(This)  CR(This, USB_OHCI_DEV, Usb2Hc, USB_OHCI_DEV_SIGNATURE)

//
// USB_OHCI_DEV support the OHCI hardware controller. It schedules
// the asynchronous interrupt transfer with the same method as
// EHCI: a reversed tree structure. For synchronous interrupt,
// control and bulk transfer, it uses three static queue head to
// schedule them. SyncIntQh is for interrupt transfer. LsCtrlQh is
// for LOW speed control transfer, and FsCtrlBulkQh is for FULL
// speed control or bulk transfer. This is because FULL speed contrl
// or bulk transfer can reclaim the unused bandwidth. Some USB
// device requires this bandwidth reclamation capability.
//
typedef struct _USB_OHCI_DEV {
  UINT32                    Signature;
  EFI_USB_HC_PROTOCOL       UsbHc;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  VOID                      *SmmOhc;
  VOID                      *SmmUsbHc;
  VOID                      *SmmUsb2Hc;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
} USB_OHCI_DEV;

extern EFI_DRIVER_BINDING_PROTOCOL  gOhciDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL gOhciComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL  gOhciComponentName;


#endif
