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

  XhcPeim.h

Abstract:

  Header file for Usb Host Controller PEIM

--*/

#ifndef _RECOVERY_XHC_H
#define _RECOVERY_XHC_H

#include <PiPei.h>

#include <Ppi/UsbController.h>
#include <Ppi/Usb2HostController.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>


#include "UsbHcMem.h"
#include "XhciSched.h"



#define EFI_USB_SPEED_FULL 0x0000
#define EFI_USB_SPEED_LOW  0x0001
#define EFI_USB_SPEED_HIGH 0x0002
#define EFI_USB_SPEED_SUPER 0x0003

#define PAGESIZE  4096

#define BIT(a)  (1 << (a))


enum {
 //
  // Capability register offset
  //
  XHC_CAPLENGTH_OFFSET    = 0,    // Capability register length offset 

  XHC_HCIVERSION_OFFSET    = 0x02, // Interface Version Number 02-03h
  XHC_HCSPARAMS1_OFFSET    = 0x04, // Structural Parameters 1
  XHC_HCSPARAMS2_OFFSET    = 0x08, // Structural Parameters 2
  XHC_HCSPARAMS3_OFFSET    = 0x0c, // Structural Parameters 3
  XHC_HCCPARAMS_OFFSET     = 0x10, // Capability Parameters
  XHC_DBOFF_OFFSET         = 0x14, // Doorbell Offset
  XHC_RTSOFF_OFFSET        = 0x18, // Runtime Register Space Offset
  
  //
  // Capability register bit definition
  //
  HCCP_64BIT              = 0x01, // 64-bit addressing capability
  MAX_PORTS               = 0xFF,

  //
  // Operational register offset
  //
  XHC_USBCMD_OFFSET       = 0x0,  // USB command register offset
  XHC_USBSTS_OFFSET       = 0x04, // Statue register offset
  XHC_PAGESIZE_OFFSET      = 0x08, // USB interrutp offset
  XHC_DNCTRL_OFFSET       = 0x14, // Frame list base address offset
  XHC_CRCR_OFFSET         = 0x18, // Command Ring Control Register offset
  XHC_DCBAAP_OFFSET       = 0x30, // Device Context Base Address Array Pointer Register address offset
  XHC_CONFIG_OFFSET       = 0x38, // Configure Register (CONFIG) address offset
  XHC_CONFIG_FLAG_OFFSET  = 0x40, // Configure flag register offset
  XHC_PORT_STAT_OFFSET    = 0x400, // Port status/control offset

  XHC_FRAME_LEN           = 1024, 
  
  //
  // Runtime register offset
  //
  XHC_IMAN_OFFSET         = 0x20,  // Interrupter Management Register (IMAN)
  XHC_IMOD_OFFSET         = 0x24,  // Interrupter Moderation Register (IMOD)
  XHC_ERSTSZ_OFFSET       = 0x28,  // Event Ring Segment Table Size Register (ERSTSZ)
  XHC_ERSTBA_OFFSET       = 0x30,  // Event Ring Segment Table Base Address Register (ERSTBA)
  XHC_ERDP_OFFSET         = 0x38,  // Event Ring Dequeue Pointer Register (ERDP)
  
  //
  // Register bit definition
  //
  CONFIGFLAG_ROUTE_XHC    = 0x01, // Route port to XHC

  USBLEGSP_BIOS_SEMAPHORE = BIT(16), // HC BIOS Owned Semaphore
  USBLEGSP_OS_SEMAPHORE   = BIT(24), // HC OS Owned Semaphore

  USBLEGCTLSTS_IOC_ENABLE       = BIT(0), // SMI on USB Complete Enable
  USBLEGCTLSTS_ERROR_ENABLE     = BIT(1), // SMI on USB Error Enable
  USBLEGCTLSTS_PORT_CHANGE_ENABLE      = BIT(2), // SMI on Port Change Enable
  USBLEGCTLSTS_FRAME_LIST_ROLLOVER_ENABLE     = BIT(3), // SMI on Frame List Rollover Enable
  USBLEGCTLSTS_HOST_ERROR_ENABLE       = BIT(4), // SMI on Host System Error Enable
  USBLEGCTLSTS_ASYNC_ADVANCE_ENABLE    = BIT(5), // SMI on Async Advance Enable
  USBLEGCTLSTS_OWNERSHIP_ENABLE        = BIT(13), // SMI on OS Ownership Enable
  USBLEGCTLSTS_PCI_COMMAND_ENABLE      = BIT(14), // SMI on PCI Command Enable
  USBLEGCTLSTS_BAR_ENABLE              = BIT(15), // SMI on BAR Enable
  USBLEGCTLSTS_IOC              = BIT(16), // SMI on USB Complete
  USBLEGCTLSTS_ERROR            = BIT(17), // SMI on USB Error
  USBLEGCTLSTS_PORT_CHANGE      = BIT(18), // SMI on Port Change
  USBLEGCTLSTS_FRAME_LIST_ROLLOVER     = BIT(19), // SMI on Frame List Rollover
  USBLEGCTLSTS_HOST_ERROR       = BIT(20), // SMI on Host System Error
  USBLEGCTLSTS_ASYNC_ADVANCE    = BIT(21), // SMI on Async Advance
  USBLEGCTLSTS_OWNERSHIP        = BIT(29), // SMI on OS Ownership
  USBLEGCTLSTS_PCI_COMMAND      = BIT(30), // SMI on PCI Command
  USBLEGCTLSTS_BAR              = BIT(31), // SMI on BAR

  USBCMD_RUN              = 0x01,   // Run/stop 
  USBCMD_RESET            = 0x02,   // Start the host controller reset
  XHC_USBCMD_INTE         = BIT(2),
  XHC_USBCMD_HSEE         = BIT(3),
  USBCMD_ENABLE_PERIOD    = 0x10,   // Enable periodic schedule
  USBCMD_ENABLE_ASYNC     = 0x20,   // Enable asynchronous schedule
  USBCMD_IAAD             = 0x40,   // Interrupt on async advance doorbell
  
  USBSTS_IOC              = 0x01,
  USBSTS_USBERRINT        = 0x02,

  USBSTS_FARME_LIST_ROLLVER      = 0x08,
  USBSTS_IAA              = 0x20,   // Interrupt on async advance
  USBSTS_PERIOD_ENABLED   = 0x4000, // Periodic schedule status 
  USBSTS_ASYNC_ENABLED    = 0x8000, // Asynchronous schedule status 
  XHCSTS_HALT             = 0x0001, // Host controller halted
  XHCSTS_CNR              = 0x0800, // Host controller Controller Not Ready (CNR)
  USBSTS_SYS_ERROR        = BIT(2), // Host system error
  USBSTS_EINT             = BIT(3), // Event Interrupt (EINT)
  USBSTS_PORT_CHANGE      = BIT(4), // Port Change Detect (PCD)
  USBSTS_INTACK_MASK      = 0x003F, // Mask for the interrupt ACK, the WC 
                                    // (write clean) bits in USBSTS register
  
  USBINTR_IOCEN           = 0x01,   // USB Interrupt Enable
  USBINTR_ERROR           = 0x02,   // USB Error Interrupt Enable
  USBINTR_PORT_CHANGE     = 0x04,   // Port Change Interrupt Enable
  USBINTR_FAME_LIST_ROLLOVER   = 0x08,   // Frame List Rollover Enable
  USBINTR_HOST_ERROR      = 0x10,   // Host System Error Enable
  USBINTR_ASYNC_ADVANCE   = 0x20,   // Interrupt on Async Advance Enable
  
  PORTSC_CONN             = BIT(0),   // Current Connect Status
  PORTSC_ENABLED          = BIT(1),   // Port Enable / Disable 
  PORTSC_OVERCUR          = BIT(3),   // Over current Active 
  PORTSC_RESET            = BIT(4),  // Port Reset
  PORTSC_LINK_STATE_MASK  = (BIT (5)|BIT (6)|BIT (7)|BIT (8)),// Port Link State (PLS) ?RWS
  PORTSC_POWER            = BIT (9),  // Port Power
  PORTSC_SPEED_MASK       = (BIT (10)|BIT (11)|BIT (12)|BIT (13)),// Port Speed (Port Speed) ?ROS
  PORTSC_LINK_WRT_STROB   = BIT(16),   // Connect Status Change 
  PORTSC_CONN_CHANGE      = BIT(17),   // Connect Status Change 
  PORTSC_ENABLE_CHANGE    = BIT(18),   // Port Enable / Disable Change
  PORTSC_WARM_RESET_CHANGE    = BIT(19),   // Port Enable / Disable Change

  PORTSC_OVERCUR_CHANGE   = BIT(20),   // Over current Change 
  PORTSC_RESET_CHANGE   = BIT(21),   // Over current Change 
  
  PORSTSC_RESUME          = 0x40,   // Force Port Resume 
  PORTSC_SUSPEND          = 0x80,   // Port Suspend State

  PORTSC_LINESTATE_K      = 0x400,  // Line Status K-state
  PORTSC_LINESTATE_J      = 0x800,  // Line Status J-state

  PORTSC_OWNER            = 0x2000, // Port Owner
  PORTSC_CHANGE_MASK      = (BIT(17)),   // Mask of the port change bits, 
                                                         // they are WC (write clean)
  //
  // PCI Configuration Registers
  //
  XHC_PCI_CLASSC          = 0x09,
  XHC_PCI_CLASSC_PI       = 0x30, 
  XHC_BAR_INDEX           = 0, /* how many bytes away from USB_BASE to 0x10 */
  XHC_PCI_MEM_BASE_ADDRESS = 0x10,    // Memory Bar register address
  XHC_PCI_MEM_BASE_MASK = 0x0ffffff00, // Memory address MASK
  
  XHC_CRCR_RCS            = BIT(0),
  XHC_CRCR_CS             = BIT(1),
  XHC_CRCR_CA             = BIT(2),
  XHC_CRCR_CRR            = BIT(3),
  
  XHC_IMAN_IP             = BIT(0),
  XHC_IMAN_IE             = BIT(1),
};


//
// Structure to map the hardware port states to the 
// UEFI's port states. 
//
typedef struct {
  UINT32                  HwState;
  UINT16                  UefiState;
} USB_PORT_STATE_MAP;

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


#define XHC_LOW_32BIT(Addr64)     ((UINT32)(((UINTN)(Addr64)) & 0XFFFFFFFF))
#define XHC_HIGH_32BIT(Addr64)    ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))
#define XHC_BIT_IS_SET(Data, Bit) ((BOOLEAN)(((Data) & (Bit)) == (Bit)))
#define USB_ENDPOINT_TYPE(Desc)   ((Desc)->Attributes & USB_ENDPOINT_TYPE_MASK)         


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
  );

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
  );

STATIC
EFI_STATUS
EFIAPI
XhcGetRootHubPortNumber (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  OUT UINT8                                     *PortNumber
  );

STATIC
EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  );

STATIC
EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  );

STATIC
EFI_STATUS
EFIAPI
XhcGetRootHubPortStatus (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN  UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  );


STATIC
EFI_STATUS
EFIAPI
XhcGetHostControllerType (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  OUT UINT8                   * ControllerType
  )
  ;

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
  );

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
;

EFI_STATUS
EFIAPI
XhcSetConfigCmd (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI            * This,
  IN UINT8                                        DeviceAddress,
  IN EFI_USB_ENDPOINT_DESCRIPTOR **EndPointDescriptors,
  IN UINT8                                    EndPointNum
)
;

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
;


EFI_STATUS
InitializeUsbHC (
  IN USB3_HC_DEV          *XhcDev  
  );
   
USBHC_MEM_POOL *
UsbHcInitMemPool (
  IN USB3_HC_DEV          *Xhc,
  IN BOOLEAN              Check4G,
  IN UINT32               Which4G
  )
/*++

Routine Description:

  Initialize the memory management pool for the host controller

Arguments:

  Pool    - The USB memory pool to initialize
  PciIo   - The PciIo that can be used to access the host controller
  Check4G - Whether the host controller requires allocated memory 
            from one 4G address space.
  Which4G - The 4G memory area each memory allocated should be from

Returns:

  EFI_SUCCESS         : The memory pool is initialized
  EFI_OUT_OF_RESOURCE : Fail to init the memory pool

--*/
;
            

EFI_STATUS
UsbHcFreeMemPool (
  IN USBHC_MEM_POOL       *Pool
  )
/*++

Routine Description:

  Release the memory management pool

Arguments:

  Pool   - The USB memory pool to free

Returns:

  EFI_SUCCESS      : The memory pool is freed
  EFI_DEVICE_ERROR : Failed to free the memory pool

--*/
;


VOID *
UsbHcAllocateMem (
  IN USB3_HC_DEV          *Xhc,
  IN  USBHC_MEM_POOL      *Pool,
  IN  UINTN               Size
  )
/*++

Routine Description:

  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

Arguments:

  Pool      - The host controller's memory pool
  Size      - Size of the memory to allocate

Returns:

  The allocated memory or NULL
  
--*/
;


VOID
UsbHcFreeMem (
  IN USBHC_MEM_POOL       *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  )
/*++

Routine Description:

  Free the allocated memory back to the memory pool

Arguments:

  Pool    - The memory pool of the host controller
  Mem     - The memory to free
  Size    - The size of the memory to free

Returns:

  VOID

--*/
;

UINT32
XhcReadOpReg (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT32              Offset
  )
;

VOID
XhcWriteOpReg (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
;


BOOLEAN
XhcIsSysError (
  IN USB3_HC_DEV          *Xhc
  )
;
BOOLEAN
XhcIsHalt (
  IN USB3_HC_DEV          *Xhc
  )
;

VOID
XhcWriteOpReg64 (
  IN USB3_HC_DEV          *Xhc,
  IN UINT64               Offset,
  IN UINT64               Data
  )
;


VOID
XhcWriteDBReg (
  IN USB3_HC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
;
EFI_STATUS
XhcHaltHC (
  IN USB3_HC_DEV         *Xhc,
  IN UINT32              Timeout
  )
  ;
#define XHC_REG_BIT_IS_SET(Xhc, Offset, Bit) \
          (XHC_BIT_IS_SET(XhcReadOpReg ((Xhc), (Offset)), (Bit)))

#endif
