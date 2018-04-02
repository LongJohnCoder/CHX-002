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

  XhciSched.h

Abstract:

  This file contains the definination for host controller schedule routines

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_XHCI_SCHED_H_
#define _EFI_XHCI_SCHED_H_

#include "XhciDebug.h"

//============================================
//
// ----------------Global Constant--------------------
//
//============================================
typedef struct _USB3_HC_DEV  USB3_HC_DEV;
#define EFI_LIST_ENTRY LIST_ENTRY
#define USB3_MAX_DEVICES    128

enum {
  MAX_DEVICE_SLOTS_ENABLED     = 0x20,
  CMD_RING_TRB_NUMBER = 0x10,
  TR_RING_TRB_NUMBER = 30,
  ERST_NUMBER = 1,
  EVENT_RING_TRB_NUMBER = 0x10, // bugbug, set EVENT_RING_TRB_NUMBER = 0x200, boot Dos will overflow cause system hang
};

enum {
  XHC_1_MICROSECOND            = 1,
  XHC_1_MILLISECOND            = 1000 * XHC_1_MICROSECOND,
  XHC_1_SECOND                 = 1000 * XHC_1_MILLISECOND,

  //
  // XHCI register operation timeout, set by experience
  //
  XHC_RESET_TIMEOUT            = 1 * XHC_1_SECOND,
  XHC_GENERIC_TIMEOUT          = 100 * XHC_1_MILLISECOND,

  //
  // Wait for roothub port power stable, refers to Spec[XHCI1.0-2.3.9]
  //
  XHC_ROOT_PORT_RECOVERY_STALL = 20 * XHC_1_MILLISECOND,
  XHC_SET_PORT_RESET_STALL       = 20 * XHC_1_MILLISECOND,

  //
  // Sync and Async transfer polling interval, set by experience,
  // and the unit of Async is 100us, means 50ms as interval.
  //
  XHC_SYNC_POLL_INTERVAL       = 20 * XHC_1_MILLISECOND,
  XHC_ASYNC_POLL_INTERVAL      = 50 * 10000U,

  USB3_HC_DEV_SIGNATURE        = SIGNATURE_32 ('x', 'h', 'c', 'i'),

};

enum {
	XHC_URB_SIG             = SIGNATURE_32 ('U', 'S', 'B', 'R'),
		
  //
  // 6.4.6 TRB Types
  //
  TRB_TYPE_NORMAL         = 1, 
  TRB_TYPE_SETUP_STAGE    = 2, 
  TRB_TYPE_DATA_STAGE     = 3, 
  TRB_TYPE_STATUS_STAGE   = 4, 
  TRB_TYPE_ISOCH          = 5, 
  TRB_TYPE_LINK           = 6, 
  TRB_TYPE_EVENT_DATA     = 7, 
  TRB_TYPE_NO_OP          = 8, 
  TRB_TYPE_EN_SLOT        = 9, 
  TRB_TYPE_DIS_SLOT       = 10, 
  TRB_TYPE_ADDRESS_DEV    = 11, 
  TRB_TYPE_CON_ENDPOINT   = 12, 
  TRB_TYPE_EVALU_CONTXT   = 13, 
  TRB_TYPE_RESET_ENDPOINT = 14, 
  TRB_TYPE_STOP_ENDPOINT  = 15, 
  TRB_TYPE_SET_TR_DEQUE   = 16, 
  TRB_TYPE_RESET_DEV      = 17, 
  TRB_TYPE_GET_PORT_BANW  = 21, 
  TRB_TYPE_FORCE_HEADER   = 22, 
  TRB_TYPE_NO_OP_COMMAND  = 23, 
  TRB_TYPE_TRANS_EVENT              = 32, 
  TRB_TYPE_COMMAND_COMPLT_EVENT     = 33, 
  TRB_TYPE_PORT_STATUS_CHANGE_EVENT = 34, 
  TRB_TYPE_HOST_CONTROLLER_EVENT    = 37, 
  TRB_TYPE_DEVICE_NOTIFI_EVENT      = 38, 
  TRB_TYPE_MFINDEX_WRAP_EVENT       = 39,

	  //
  // Endpoint Type (EP Type).
  //
  ED_NOT_VALID      =  0,
  ED_ISOCH_OUT      =  1,
  ED_BULK_OUT       =  2,
  ED_INTERRUPT_OUT  =  3,
  ED_CONTROL_BIDIR  =  4,
  ED_ISOCH_IN       =  5,
  ED_BULK_IN        =  6,
  ED_INTERRUPT_IN   =  7,
  //
  // 6.4.5 TRB Completion Codes
  //
  TRB_COMPLETION_INVALID = 0,
  TRB_COMPLETION_SUCCESS = 1,
  TRB_COMPLETION_DATA_BUFFER_ERROR = 2,
  TRB_COMPLETION_BABBLE_ERROR = 3,
  TRB_COMPLETION_USB_TRANSACTION_ERROR = 4,
  TRB_COMPLETION_TRB_ERROR = 5,
  TRB_COMPLETION_STALL_ERROR = 6,
  TRB_COMPLETION_SHORT_PACKET = 13,
 
};


enum {
  //
  // Transfer types, used in URB to identify the transfer type
  //
  XHC_CTRL_TRANSFER       = 0x01,
  XHC_BULK_TRANSFER       = 0x02,
  XHC_INT_TRANSFER_SYNC   = 0x04,
  XHC_INT_TRANSFER_ASYNC  = 0x08,
};
//============================================
//
// ----------------Data Structure Definition--------------------
//
//============================================

//
// Endpoint address and its capabilities
//
typedef struct _USB_ENDPOINT {
  UINT8                   DevAddr;
  UINT8                   EpAddr;     // Endpoint address, no direction encoded in
  EFI_USB_DATA_DIRECTION  Direction;
  UINT8                   DevSpeed;
  UINTN                   MaxPacket;
  UINT8                   Toggle;     // Data toggle, not used for control transfer
  UINTN                   Type;
  UINTN                   PollRate;   // Polling interval used by XHCI
} USB_ENDPOINT;

typedef struct _TRB {
  UINT32                  Dword1;
  UINT32                  Dword2;
  UINT32                  Dword3;
  UINT32                  Dword4;  
} TRB;


typedef struct _COMMAND_RING {
  VOID                     *CmdRingHeader;
  TRB                       *CmdRingEnqueue;
  TRB                       *CmdRingDequeue;
  UINT32                    CmdRingPCS;
  TRB                       *CmdTrbPtr;
} COMMAND_RING;

typedef struct _EVENT_RING {
  VOID                     *EventRingSeg0;  
  VOID                     *ERSTBase;  
  TRB                       *EventRingEnqueue;
  TRB                       *EventRingDequeue;
  UINT32                    EventRingCCS;
  UINTN                     EventInterrupter;
  UINTN                     InUsePipeNum;
} EVENT_RING;

typedef struct _ENDPOINT_RING {

  UINTN                     EPType;                 
  VOID                      *TransferRing;
  VOID                      *TransferRingEnqueue;
  VOID                      *TransferRingDequeue;
  UINT32                    TransferRingPCS;
} ENDPOINT_RING;



typedef struct _PEI_USB3_DEVICE{
  UINT8                     Speed;
  UINT8                     Address;
  UINT32                    MaxPacket0;
  UINT8                     ParentAddr;
  UINT8                     ParentSlotID;
  UINT8                     ParentPort;       // Start at 0
  USB_DEV_ROUTE             RouteChart; //zyq,bugbug3
  //
  // XHC related stuff
  //
  UINT8                     SlotID;
  VOID        		        *InputContxt;
  VOID                      *OutputDevContxt;
  
  ENDPOINT_RING   			    EPRing[30];  
}PEI_USB3_DEVICE;

struct _USB3_HC_DEV { 
  UINTN                         Signature;
  PEI_USB2_HOST_CONTROLLER_PPI  Usb2HostControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR        PpiDescriptor;
  UINT32                        UsbHostControllerBaseAddress;

  //
  // XHCI configuration data
  //
  USBHC_MEM_POOL            *MemPool;
  UINT8                     CapLength;    // Capability Register Length
  UINT16                    HciVersion;   // Interface Version Number
  UINT32                    HcSParams1;   // Structural Parameters 1
  UINT32                    HcSParams2;   // Structural Parameters 2
  UINT32                    HcSParams3;   // Structural Parameters 3
  UINT32                    HcCParams;    // Capability Parameters
  UINT32                    DBOff;        // Doorbell Offset
  UINT32                    RTSOff;       // Runtime Register Space Offset
  UINT64                    *DCBAA; 
  UINTN                     DCBAAPages;
  UINT32                    MaxSlotsEn;
  UINT8                     AddressToSlot[MAX_DEVICE_SLOTS_ENABLED];
  COMMAND_RING              CmdRing;
  EVENT_RING                CmdEventRing; // CmdEventRing
  EVENT_RING                CtrlTrEventRing;// ControlTREventRing
  EVENT_RING                BulkTrEventRing; // BulkTREventRing
  EVENT_RING                AsynIntTrEventRing;  // IntTREventRing
  EVENT_RING                SynIntTrEventRing;  // IntTREventRing
  PEI_USB3_DEVICE           *Devices[USB3_MAX_DEVICES];
 
};

#define PEI_RECOVERY_USB_XHC_DEV_FROM_XHCI_THIS(a)  CR (a, USB3_HC_DEV, Usb2HostControllerPpi, USB3_HC_DEV_SIGNATURE)

//
// 6.2.2 Slot Context
//
typedef struct _SLOT_CONTEXT {
  UINT32                  RouteStr:20;
  UINT32                  Speed:4;
  UINT32                  RsvdZ1:1;  
  UINT32                  MTT:1; 
  UINT32                  Hub:1; 
  UINT32                  ContextEntries:5; 
  
  UINT32                  MaxExitLatency:16; 
  UINT32                  RootHubPortNum:8; 
  UINT32                  PortNum:8; 
  
  UINT32                  TTHubSlotID:8; 
  UINT32                  TTPortNum:8; 
  UINT32                  TTT:2;
  UINT32                  RsvdZ2:4; 
  UINT32                  InterTarget:10; 
  
  UINT32                  DeviceAddress:8;
  UINT32                  RsvdZ3:19;
  UINT32                  SlotState:5;

  UINT32                  RsvdZ4;
  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
  UINT32                  RsvdZ7;
} SLOT_CONTEXT;

//
// 6.2.3 Endpoint Context
//
typedef struct _ENDPOINT_CONTEXT {
  UINT32                  EPState:3;
  UINT32                  RsvdZ1:5;
  UINT32                  Mult:2;
  UINT32                  MaxPStreams:5;
  UINT32                  LSA:1;
  UINT32                  Interval:8;
  UINT32                  RsvdZ2:8;
  
  UINT32                  RsvdZ3:1;
  UINT32                  CErr:2;
  UINT32                  EPType:3;
  UINT32                  RsvdZ4:1;
  UINT32                  HID:1;
  UINT32                  MaxBurstSize:8;
  UINT32                  MaxPacketSize:16;
  
  UINT32                  PtrLo;
  
  UINT32                  PtrHi;
  
  UINT32                  AverageTRBLength:16;
  UINT32                  MaxESITPayload:16;
  
  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
  UINT32                  RsvdZ7;  
} ENDPOINT_CONTEXT;

//
// 6.2.1 Device Context
//
typedef struct _DEVICE_CONTEXT {
  SLOT_CONTEXT		  		Slot;
  ENDPOINT_CONTEXT			EP[31];  
} DEVICE_CONTEXT;


//
// 6.2.5.1 Input Control Context
//
typedef struct _INPUT_CONTRL_CONTEXT {
  UINT32                  Dword1;
  UINT32                  Dword2;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  RsvdZ3;
  UINT32                  RsvdZ4;
  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
} INPUT_CONTRL_CONTEXT;

//
// 6.2.5 Input Context
//
typedef struct _INPUT_CONTEXT {
  INPUT_CONTRL_CONTEXT    	InputControlContext;
  SLOT_CONTEXT		  		Slot;
  ENDPOINT_CONTEXT			EP[31];  
} INPUT_CONTEXT;

//
// 6.4.3.1 No Op Command TRB
// The No Op Command TRB provides a simple means for verifying the operation of the Command Ring
//  mechanisms offered by the xHCI.
//
typedef struct _CMD_TRB_NO_OP {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;  
} CMD_TRB_NO_OP;

//
// 6.4.3.2 Enable Slot Command TRB
// The Enable Slot Command TRB causes the xHC to select an available Device Slot and return the ID of the
// selected slot to the host in a Command Completion Event.
//
typedef struct _CMD_TRB_EN_SLOT {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;  
} CMD_TRB_EN_SLOT;

//
// 6.4.3.3 Disable Slot Command TRB
// The Disable Slot Command TRB releases any bandwidth assigned to the disabled slot and frees any
// internal xHC resources assigned to the slot.
//
typedef struct _CMD_TRB_DIS_SLOT {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:8;
  UINT32                  SlotID:8;
} CMD_TRB_DIS_SLOT;

//
// 6.4.3.4 Address Device Command TRB
// The Address Device Command TRB transitions the selected Device Context from the Default to the
// Addressed state and causes the xHC to select an address for the USB device in the Default State and
// issue a SET_ADDRESS request to the USB device.
//
typedef struct _CMD_TRB_ADDR_DEV {
  //UINT32                  RsvdZ0:4;
  UINT32                  PtrLo;//:28;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:8;
  UINT32                  BSR:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotID:8;
} CMD_TRB_ADDR_DEV;

//
// 6.4.3.5 Configure Endpoint Command TRB
// The Configure Endpoint Command TRB evaluates the bandwidth and resource requirements of the
// endpoints selected by the command.
//
typedef struct _CMD_CFG_ED {
  //UINT32                  RsvdZ0:4;
  UINT32                  PtrLo;//:28;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:8;
  UINT32                  DC:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotID:8;
} CMD_CFG_ED;

//
// 6.4.3.6 Evaluate Context Command TRB
// The Evaluate Context Command TRB is used by system software to inform the xHC that the selected
// Context data structures in the Device Context have been modified by system software and that the xHC
// shall evaluate any changes
//
typedef struct _CMD_TRB_EVALU_CONTX {
  UINT32                  PtrLo;//:28;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotID:8;
} CMD_TRB_EVALU_CONTX;

//
// 6.4.3.7 Reset Endpoint Command TRB
// The Reset Endpoint Command TRB is used by system software to reset a specified Transfer Ring
//
typedef struct _CMD_TRB_RESET_ED {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:8;
  UINT32                  TSP:1;
  UINT32                  Type:6;
  UINT32                  EDID:5;
  UINT32                  RsvdZ4:3;
  UINT32                  SlotID:8;
} CMD_TRB_RESET_ED;

//
// 6.4.3.8 Stop Endpoint Command TRB
// The Stop Endpoint Command TRB command allows software to stop the xHC execution of the TDs on a
// Transfer Ring and temporarily take ownership of TDs that had previously been passed to the xHC.
//
typedef struct _CMD_TRB_STOP_ED {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  EDID:5;
  UINT32                  RsvdZ4:2;
  UINT32                  SP:1;
  UINT32                  SlotID:8;
} CMD_TRB_STOP_ED;

//
// 6.4.3.9 Set TR Dequeue Pointer Command TRB
// The Set TR Dequeue Pointer Command TRB is used by system software to modify the TR Dequeue
// Pointer and DCS fields of an Endpoint or Stream Context.
//
typedef struct _CMD_SET_TR_DEQ {
  //union {
  //  UINT32                  DCS:1;
  //  UINT32                  SCT:3;
  //  UINT32                  PtrLo;  
 // } DW0;
  UINT32                  PtrLo; 
  UINT32                  PtrHi;
  UINT32                  RsvdZ1:16;
  UINT32                  StreamID:16;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:9;
  UINT32                  Type:6;
  UINT32                  EDID:5;
  UINT32                  RsvdZ3:3;
  UINT32                  SlotID:8;
} CMD_SET_TR_DEQ;



//
// A Link TRB provides support for non-contiguous TRB Rings.
//
typedef struct _LNK_TRB {
  UINT32                  PtrLo;//:28;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1:22;
  UINT32                  InterTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  TC:1;
  UINT32                  RsvdZ2:2;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  RsvdZ3:4;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} LNK_TRB;

typedef struct _NO_OP_TRB {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} CMD_NO_OP_TRB;

//
// 6.4.1.1 Normal TRB
// A Normal TRB is used in several ways; exclusively on Bulk and Interrupt Transfer Rings for normal and
// Scatter/Gather operations, to define additional data buffers for Scatter/Gather operations on Isoch Transfer
// Rings, and to define the Data stage information for Control Transfer Rings.
//
typedef struct _TRANSFER_TRB_NORMAL {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:17;
  UINT32                  TDSize:5;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  ISP:1;
  UINT32                  NS:1;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ1:2;
  UINT32                  BEI:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ2:16;
} TRANSFER_TRB_NORMAL;

//
// 6.4.1.2.1 Setup Stage TRB
// A Setup Stage TRB is created by system software to initiate a USB Setup packet on a control endpoint.
//
typedef struct _TRANSFER_TRB_CONTROL_SETUP{
  UINT32                  bmRequestType:8;
  UINT32                  bRequest:8;
  UINT32                  wValue:16;
  
  UINT32                  wIndex:16;
  UINT32                  wLength:16;
  
  UINT32                  Lenth:17;
  UINT32                  RsvdZ1:5;
  UINT32                  IntTarget:10;
  
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:4;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ3:3;
  UINT32                  Type:6;
  UINT32                  TRT:2;
  UINT32                  RsvdZ4:14;
} TRANSFER_TRB_CONTROL_SETUP;

//
// 6.4.1.2.2 Data Stage TRB
// A Data Stage TRB is used generate the Data stage transaction of a USB Control transfer.
//
typedef struct _TRANSFER_TRB_CONTROL_DATA{
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:17;
  UINT32                  TDSize:5;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  ISP:1;
  UINT32                  NS:1;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ1:3;
  UINT32                  Type:6;
  UINT32                  DIR:1;
  UINT32                  RsvdZ2:15;
} TRANSFER_TRB_CONTROL_DATA;

//
// 6.4.1.2.2 Data Stage TRB
// A Data Stage TRB is used generate the Data stage transaction of a USB Control transfer.
//
typedef struct _TRANSFER_TRB_CONTROL_STATUS{
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  RsvdZ3:22;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  RsvdZ4:2;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  RsvdZ5:4;
  UINT32                  Type:6;
  UINT32                  DIR:1;
  UINT32                  RsvdZ6:15;
} TRANSFER_TRB_CONTROL_STATUS;

//
// 6.4.2.1 Transfer Event TRB
// A Transfer Event provides the completion status associated with a Transfer TRB. Refer to section 4.11.3.1
// for more information on the use and operation of Transfer Events.
//
typedef struct _EVT_TRB_TRANSFER {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:24;
  UINT32                  Completcode:8;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ1:1;
  UINT32                  ED:1;
  UINT32                  RsvdZ2:7;
  UINT32                  Type:6;
  UINT32                  EndpointID:5;
  UINT32                  RsvdZ3:3;
  UINT32                  SlotID:8; 
} EVT_TRB_TRANSFER;

//
// 6.4.2.2 Command Completion Event TRB
// A Command Completion Event TRB shall be generated by the xHC when a command completes on the
// Command Ring. Refer to section 4.11.4 for more information on the use of Command Completion Events.
//
typedef struct _EVT_TRB_COMMAND {
  //UINT32                  RsvdZ1:4;
  UINT32                  TRBPtrLo;//:28;
  UINT32                  TRBPtrHi;
  UINT32                  RsvdZ2:24;
  UINT32                  Completcode:8;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  VFID:8;
  UINT32                  SlotID:8; 
} EVT_TRB_COMMAND;

//
//5.5.2 Interrupter Register Set
//
 typedef  union _XHCI_64_32_ADDRESS
  {
           UINT64      Bits64;
           struct
           {
                UINT32   LBits32         :32;
                UINT32   HBits32         :32;
           }Bits32;
  }XHCI_64_32_ADDRESS;

typedef struct _INTERRUPTER_REGISTER_SET {
  UINT32                          InterrupterManagement;
  UINT32                          InterrupterModeration;
  UINT32                          RingSegTableSize:16;
  UINT32                          RsvdZ1:16;
  UINT32                          RsvdZ2;  
  XHCI_64_32_ADDRESS     BasePtr;
  XHCI_64_32_ADDRESS     DequePtr;
} INTERRUPTER_REGISTER_SET;

//
//Table 40: Host Controller Runtime Registers
//
typedef struct _HC_RUNTIME_REGS {
  UINT32                    MicroframeIndex;
  UINT32                    RsvdZ1;
  UINT64                    RsvdZ2; 
  UINT64                    RsvdZ3;  
  UINT64                    RsvdZ4; 
  INTERRUPTER_REGISTER_SET  IR[1];
} HC_RUNTIME_REGS;

//
// 6.5 Event Ring Segment Table
// The Event Ring Segment Table is used to define multi-segment Event Rings and to enable runtime
// expansion and shrinking of the Event Ring. The location of the Event Ring Segment Table is defined by the
// Event Ring Segment Table Base Address Register (5.5.2.3.2). The size of the Event Ring Segment Table
// is defined by the Event Ring Segment Table Base Size Register (5.5.2.3.1).
//
typedef struct _EVENT_RING_SEG_TABLE_ENTRY {
  //UINT32                  RsvdZ0:6;
  UINT32                  PtrLo;//:26;
  UINT32                  PtrHi;
  UINT32                  RingTrbSize:16;
  UINT32                  RsvdZ1:16;
  UINT32                  RsvdZ2;  
} EVENT_RING_SEG_TABLE_ENTRY;




//
//URB related data structure 
//

typedef
EFI_STATUS
(EFIAPI *EFI_ASYNC_USB_TRANSFER_CALLBACK) (
  IN VOID         *Data,
  IN UINTN        DataLength,
  IN VOID         *Context,
  IN UINT32       Status
  );

typedef struct _URB {
  UINT32                          Signature;
  EFI_LIST_ENTRY                  UrbList;

  //
  // Transaction information
  //
  USB_ENDPOINT                    Ep;
  EFI_USB_DEVICE_REQUEST          *Request;     // Control transfer only
  VOID                            *Data;
  UINTN                           DataLen;
  EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
  VOID                            *Context;

  //
  // Schedule data
  //
  
  TRB                             *TransferTrbStart;
  UINTN                           TransferTrbNum;
  TRB                             *TransferTrbEnd;
  UINTN                           InterrupterTarget;
  TRB                             *EventBegin;
 //
 //Transfer Terminals info
 //
  UINT8                         SlotID;    //for reset endpoint
  UINT8                         Dci;        //for reset endpoint
  DEVICE_CONTEXT         *OutputDevContxt; //check the status of the endpoint context
  ENDPOINT_RING   			    *EPRing; //the endpoint transfer ring 
  //
  // Transaction result
  //
  UINT32                          Result;
  UINTN                           Completed;    // completed data length
  UINT8                           DataToggle;
  //
  //AsynIntOnly used timer 
  //
  UINT32                          DelayCount;
  UINT32                          DelayRecord;
} URB;




//============================================
//
// ----------------Function Declaration------------------------
//
//============================================

EFI_STATUS
XhcInitSched (
  IN USB3_HC_DEV          *Xhc
  )
/*++

Routine Description:

  Initialize the schedule data structure such as frame list

Arguments:

  Xhc - The XHCI device to init schedule data for

Returns:

  EFI_OUT_OF_RESOURCES  - Failed to allocate resource to init schedule data
  EFI_SUCCESS           - The schedule data is initialized

--*/
;

EFI_STATUS
EFIAPI
XhcAllocateDeviceContextArray (
  IN  USB3_HC_DEV         *Xhc
)
/*++

Routine Description:
  // Chapter4.2  step3 Program the Max Device Slots Enabled (MaxSlotsEn) field in the CONFIG register (5.4.7) to enable
  // the device slots that system software is going to use.
 
Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;


EFI_STATUS
EFIAPI
XhcAllocateScratchpadBuffersArray (
  IN  USB3_HC_DEV         *Xhc
);

EFI_STATUS
EFIAPI
XhcAdvanceDequeuePointer (
  IN USB3_HC_DEV          *Xhc,
  IN UINT8                InterrupterTarget,
  IN TRB                  *EvtTrb
)
/*++

Routine Description:


Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;

VOID
EFIAPI
XhcRefreshDequeuePointer (
  IN USB3_HC_DEV          *Xhc,
  IN UINT8                InterrupterTarget
)
/*++

Routine Description:


Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;
EFI_STATUS
EFIAPI
XhcAllocateCommandRing (
  IN  USB3_HC_DEV         *Xhc
)

/*++
  //Chapter4.2  step5.  Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register
  //(5.4.5) with a 64-bit address pointing to the starting address of the first TRB of the Command Ring.
  //Note: The Command Ring is 64 byte aligned, so the low order 6 bits of the Command Ring Pointer shall
  //always be ¡®0¡¯.
  // ps. UsbAllocatePool is 64 bytes alignment
Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;

EFI_STATUS
EFIAPI
XhcAllocateEventRing (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT8                    InterrupterTarget,
  IN  EVENT_RING           *EventRing
)
/*++

Routine Description:
  //Defining the Event Ring: (refer to section 4.9.4 for a discussion of Event Ring Management.)
  // - Allocate and initialize the Event Ring Segment(s).
  // - Allocate the Event Ring Segment Table (ERST) (section 6.5). Initialize ERST table entries
  //   to point to and to define the size (in TRBs) of the respective Event Ring Segment.
  // - Program the Interrupter Event Ring Segment Table Size (ERSTSZ) register (5.5.2.3.1)
  // with the number of segments described by the Event Ring Segment Table.
  // - Program the Interrupter Event Ring Dequeue Pointer (ERDP) register (5.5.2.3.3) with the
  // starting address of the first segment described by the Event Ring Segment Table.
  // - Program the Interrupter Event Ring Segment Table Base Address (ERSTBA) register
  // (5.5.2.3.2) with a 64-bit address pointer to where the Event Ring Segment Table is
  // located.
  // Note that writing the ERSTBA enables the Event Ring. Refer to section 4.9.4 for more
  // information on the Event Ring registers and their initialization.

Arguments:


Returns:


--*/
;

EFI_STATUS
EFIAPI
XhcRingDoorBell (
  IN USB3_HC_DEV          *Xhc, 
  IN UINT8                SlotID,
  IN UINT8                DCI
)
/*++

Routine Description:


Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;

URB *
XhcCreateUrb (
  IN USB3_HC_DEV                        *Xhc,
  IN UINT8                              DevAddr,
  IN UINT8                              EpAddr,
  IN UINT8                              DevSpeed,
  IN UINT8                              Toggle,
  IN UINTN                              MaxPacket,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Hub,
  IN UINTN                              Type,
  IN EFI_USB_DEVICE_REQUEST             *Request,
  IN VOID                               *Data,
  IN UINTN                              DataLen,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK    Callback,
  IN VOID                               *Context,
  IN UINTN                              Interval
  )
/*++

Routine Description:

  Create a new URB and its associated QTD

Arguments:

  Xhc       - The XHCI device
  DevAddr   - The device address
  EpAddr    - Endpoint addrress & its direction
  DevSpeed  - The device speed
  Toggle    - Initial data toggle to use
  MaxPacket - The max packet length of the endpoint
  Hub       - The transaction translator to use
  Type      - The transaction type
  Request   - The standard USB request for control transfer
  Data      - The user data to transfer
  DataLen   - The length of data buffer
  Callback  - The function to call when data is transferred
  Context   - The context to the callback
  Interval  - The interval for interrupt transfer
Returns:

  Created URB or NULL

--*/
;

BOOLEAN
XhcCheckUrbResult (
  IN  USB3_HC_DEV         *Xhc,
  IN  URB                 *Urb
  )
/*++

Routine Description:

  Check the URB's execution result and update the URB's
  result accordingly.

Arguments:

  Xhc - The XHCI device
  Urb - The URB to check result

Returns:

  Whether the result of URB transfer is finialized.

--*/
;

EFI_STATUS
XhcExecTransfer (
  IN  USB3_HC_DEV         *Xhc,
  IN  URB                 *Urb,
  IN  UINTN               TimeOut
  )
/*++

Routine Description:

  Execute the transfer by polling the URB. This is a synchronous operation.

Arguments:

  Xhc        - The XHCI device
  Urb        - The URB to execute
  TimeOut    - The time to wait before abort, in millisecond.

Returns:

  EFI_DEVICE_ERROR : The transfer failed due to transfer error
  EFI_TIMEOUT      : The transfer failed due to time out
  EFI_SUCCESS      : The transfer finished OK

--*/
;

EFI_STATUS 
EFIAPI
XhcEndpointHaltRecovery( 
 IN  USB3_HC_DEV         *Xhc,
 IN  URB                       *Urb
 );

EFI_STATUS 
EFIAPI
XhcEndpointNoExecuteRecovery( 
 IN  USB3_HC_DEV         *Xhc,
 IN  URB                       *Urb
 );

UINTN
XhcConvertPollRate (
  IN  UINTN               Interval
  )
/*++

Routine Description:

  Convert the poll interval from application to that
  be used by XHCI interface data structure. Only need
  to get the max 2^n that is less than interval. UEFI
  can't support high speed endpoint with a interval less
  than 8 microframe because interval is specified in
  the unit of ms (millisecond)

Arguments:

  Interval  - The interval to convert

Returns:

  The converted interval

--*/
;

EFI_STATUS
XhcCreateTransferTrb (
  IN USB3_HC_DEV                  *Xhc,
  IN URB                          *Urb
  )
/*++

Routine Description:

  Create a new URB and its associated QTD

Arguments:



Returns:

  Created URB or NULL

--*/
;

UINT8
EFIAPI
XhcAddressToSlotID (
  IN  USB3_HC_DEV         *Xhc,
  IN  UINT8               DevAddr
)
/*++

Routine Description:


Arguments:


Returns:


--*/
;

UINT8
XhcEPToDci (
  IN  UINT8                                                EpAddr,
  IN  EFI_USB_DATA_DIRECTION                   Direction
)
/*++

Routine Description:


Arguments:


Returns:


--*/
;

EFI_STATUS
EFIAPI
XhcTransferRingEnqueueAdvance (
  IN USB3_HC_DEV          *Xhc,
  IN ENDPOINT_RING   			*EPRing
)
;

BOOLEAN 
	EFIAPI
	XhcUrbEventCheck( 
  IN  URB                       *Urb,
  IN  UINT32                   TRBPtrLo)
 ;

EFI_STATUS
EFIAPI
XhcSendCmd (
  IN USB3_HC_DEV          *Xhc,
  IN UINT8                TRBType,  
  IN UINT8                SlotID,
  IN UINT32               PtrLo,
  IN UINT32               PtrHi,
  IN BOOLEAN              BSR,
  IN UINT8                  Dci,
  IN UINT8                  TSP
)
/*++

Routine Description:


Arguments:



Returns:

  EFI_UNSUPPORTED        - 
  EFI_SUCCESS          - The configuration is selected.

--*/
;

EFI_STATUS
EFIAPI
XhcCommandRingEnqueueAdvance (
  IN USB3_HC_DEV          *Xhc
)
;

EFI_STATUS
EFIAPI
XhcCheckEvent (
  IN USB3_HC_DEV          *Xhc,
  IN UINT8                TRBType,
  IN UINT32               TRBPtrLo,
  IN UINTN                TimeOut,
  OUT TRB                 **Trb
)
/*++

Routine Description:


Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;

EFI_STATUS
EFIAPI
XhcAllocateTransferRing (
  IN  USB3_HC_DEV         *Xhc,
  IN  PEI_USB3_DEVICE              *Device,
  IN  UINT8                      Dci,
  IN  UINTN                      EPType
)
/*++

Routine Description:
 Allocate transfer ring for endpoint,why DCI-1 
 Because, the DCI include the slot context ,
  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
;
#endif
