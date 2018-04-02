/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Ehci.h

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


#ifndef _EFI_EHCI_H_
#define _EFI_EHCI_H_

#include <Uefi.h>

#include <Protocol/Usb2HostController.h>

#include <Protocol/PciIo.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Pci.h>
#include "ComponentName.h"



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
  EHC_1_MICROSECOND            = 1,
  EHC_1_MILLISECOND            = 1000 * EHC_1_MICROSECOND,
  EHC_1_SECOND                 = 1000 * EHC_1_MILLISECOND,

  //
  // EHCI register operation timeout, set by experience
  //
  EHC_RESET_TIMEOUT            = 1 * EHC_1_SECOND,
  EHC_GENERIC_TIMEOUT          = 10 * EHC_1_MILLISECOND,

  //
  // Wait for roothub port power stable, refers to Spec[EHCI1.0-2.3.9]
  //
  EHC_ROOT_PORT_RECOVERY_STALL = 20 * EHC_1_MILLISECOND,

  //
  // Sync and Async transfer polling interval, set by experience,
  // and the unit of Async is 100us, means 50ms as interval.
  //
  EHC_SYNC_POLL_INTERVAL       = 20 * EHC_1_MICROSECOND,
  EHC_ASYNC_POLL_INTERVAL      = 50 * 10000U,

  //
  // EHC raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
  EHC_TPL                      = TPL_NOTIFY,

  //
  // PCI Configuration Registers
  //
  EHC_PCI_CLASSC               = 0x09,
  EHC_PCI_CLASSC_PI            = 0x20,
  EHC_BAR_INDEX                = 0, /* how many bytes away from USB_BASE to 0x10 */

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

#define EHC_FROM_THIS(a)   CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

typedef struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  VOID                      *SmmUsb2Hc;
  VOID                      *SmmUsb2HcDev;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
} USB2_HC_DEV;

extern EFI_DRIVER_BINDING_PROTOCOL  gEhciDriverBinding;


extern EFI_COMPONENT_NAME2_PROTOCOL    gEhciComponentName2;

extern EFI_COMPONENT_NAME_PROTOCOL     gEhciComponentName;



#endif
