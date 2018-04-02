/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  Xhci.h

Abstract:
  USB Module file.
  
Revision History:

Bug 3145:  Release initial verion bios for ChiefRiver/MahoBay platform base on
           Sugarbay Platform core drop.
TIME:       2011-11-25
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     ChiefRiver/MahoBay Customer Refernce Board.
$TECHNICAL: 
  1. Change SnbClientX64Pkg to $(PLATFORM_PACKAGE) in INF file
     to fix failure to build other platform.
  2. Add the EDK debug libraries for IvyBridge Platform.
  3. Use UDK library IoLib in Xhci.c
  4. Add the Family ID and Model ID for IvyBridge processor.
$END--------------------------------------------------------------------------

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
  Xhci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_XHCI_H_
#define _EFI_XHCI_H_


#include <Uefi.h>    
#include <Library/UefiLib.h> 
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/PciIo.h>       
#include <Protocol/LegacyUsbInf.h>
#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/pci22.h>
#include "ComponentName.h"

#include <Library/IoLib.h>

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

#pragma pack(1)
typedef struct{
  UINT16   Function:3;
  UINT16   Device:5;
  UINT16   Bus:8;
} EFI_PCI_FUNCTION_ADDRESS;

#pragma pack()

#pragma pack(1)
typedef struct {
  UINT8                   PI;
  UINT8                   SubClassCode;
  UINT8                   BaseCode;
} USB_CLASSC;
#pragma pack()

enum {
  XHC_1_MICROSECOND            = 1,
  XHC_1_MILLISECOND            = 1000 * XHC_1_MICROSECOND,
  XHC_1_SECOND                 = 1000 * XHC_1_MILLISECOND,

  //
  // XHCI register operation timeout, set by experience
  //
  XHC_RESET_TIMEOUT            = 1 * XHC_1_SECOND,
  XHC_GENERIC_TIMEOUT          = 10 * XHC_1_MILLISECOND,

  //
  // Wait for roothub port power stable, refers to Spec[XHCI1.0-2.3.9]
  //
  XHC_ROOT_PORT_RECOVERY_STALL = 20 * XHC_1_MILLISECOND,

  //
  // Sync and Async transfer polling interval, set by experience,
  // and the unit of Async is 100us, means 50ms as interval.
  //
  XHC_SYNC_POLL_INTERVAL       = 20 * XHC_1_MICROSECOND,
  XHC_ASYNC_POLL_INTERVAL      = 50 * 10000U,

  //
  // XHC raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
  XHC_TPL                      = EFI_TPL_NOTIFY,

  //
  // PCI Configuration Registers
  //
  XHC_PCI_CLASSC               = 0x09,
  XHC_PCI_CLASSC_PI            = 0x30,
  XHC_BAR_INDEX                = 0, /* how many bytes away from USB_BASE to 0x10 */

  USB2_HC_DEV_SIGNATURE        = SIGNATURE_32 ('e', 'h', 'c', 'i'),
};

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the doule linked list. This is delete-safe.
//Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) _CR(Entry, Type, Field)

#define XHC_FROM_THIS(a)   CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

typedef struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  VOID                      *SmmUsb2Hc;
  VOID                      *SmmUsb2HcDev;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
} USB2_HC_DEV;

extern EFI_DRIVER_BINDING_PROTOCOL     gXhciDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL    gXhciComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL     gXhciComponentName;


#endif
