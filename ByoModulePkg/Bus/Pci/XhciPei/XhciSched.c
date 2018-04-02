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

    XhciSched.c

Abstract:

    XHCI transfer scheduling routines

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------

  ----------------------------------------------------------------------------------------
--*/

#include "XhcPeim.h"

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
{
  EFI_STATUS         Status;


	 //
	 //clear the port power bit (PP) in port status and control register ...
	 //
	  {

	  }


	 //
	 //Allocate memory for xhc use
	 //
	   Xhc->MemPool = UsbHcInitMemPool (
                   Xhc,
                   XHC_BIT_IS_SET(Xhc->HcCParams, HCCP_64BIT),
                   0
                   );

  if (Xhc->MemPool == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  XHC_PEI_DEBUG((EFI_D_INFO,"allocate Xhc MemPool at %x",Xhc->MemPool));
	 //
	 //Allocate DeviceContext Array(DCBAA)
	 //
  Status = XhcAllocateDeviceContextArray (Xhc);
	 //
	 //Allocate Scratchpad Buffers Array
	 //
  Status = XhcAllocateScratchpadBuffersArray (Xhc);
  //
  //Allocate Command  Ring
  //
  Status = XhcAllocateCommandRing(Xhc);
  //
  //Allocate Command/Ctrl/Bulk/Int  Event Ring
  //
  Status = XhcAllocateEventRing(Xhc, 0 ,&(Xhc->CmdEventRing));
  Status = XhcAllocateEventRing(Xhc, 1 ,&(Xhc->CtrlTrEventRing));
  Status = XhcAllocateEventRing(Xhc, 2 ,&(Xhc->BulkTrEventRing));
  Status = XhcAllocateEventRing(Xhc, 3 ,&(Xhc->AsynIntTrEventRing));
  Status = XhcAllocateEventRing(Xhc, 4 ,&(Xhc->SynIntTrEventRing));
  

  //XHC_DEBUG (("XhcInitSched:Command Event Ring Head TRB address =0x%x\n", Xhc->CmdEventRing.EventRingSeg0));
  
  return EFI_SUCCESS;
}


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
{
   EFI_STATUS                     Status;
   EFI_PHYSICAL_ADDRESS    Buf;
   UINTN                 Pages;
   UINTN                 Bytes;
  Xhc->MaxSlotsEn             = MAX_DEVICE_SLOTS_ENABLED;
  XhcWriteOpReg (Xhc, XHC_CONFIG_OFFSET, Xhc->MaxSlotsEn);//zyq,add.only notify the Xhc that host controller driver has loaded. 
  //
  //Chapter4.2  step4.  Program the Device Context Base Address Array Pointer (DCBAAP) register (5.4.6) with a 64-bit
  // address pointing to where the Device Context Base Address Array is located.
  //
  Bytes= (Xhc->MaxSlotsEn+1) * sizeof(UINT64);//zyqbugbug1,add.
  Pages = EFI_SIZE_TO_PAGES (Bytes);
  Xhc->DCBAAPages=Pages;
  Status = PeiServicesAllocatePages (
               EfiBootServicesCode,
                        Pages,
                        &Buf
             );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    
  ZeroMem((VOID *)(UINTN)Buf, Bytes);
  //
  // Software shall set Device Context Base Address Array entries for unallocated Device Slots to ¡®0¡¯.
  //
  Xhc->DCBAA                  = (UINT64 *) ((UINTN) Buf);
  
  XHC_PEI_ERROR((EFI_D_INFO,"XhcInitSched:DCBAA=0x%x\n", Xhc->DCBAA));
  XhcWriteOpReg64 (Xhc, XHC_DCBAAP_OFFSET, (UINT64)Xhc->DCBAA);
  return EFI_SUCCESS; 
}


EFI_STATUS
EFIAPI
XhcAllocateScratchpadBuffersArray (
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
{
  UINT8                  MaxScratchpadBuffers;
  UINT32                PageSize;
  UINTN                  Bytes;
  UINT64                *ScratchPadBuffersArray;
  EFI_PHYSICAL_ADDRESS                   Buf;
  UINT8                  Index;
  EFI_STATUS          Status;

  Status=EFI_SUCCESS;
  
  // 1.Get the page size and scratchpadbuffers    
  MaxScratchpadBuffers=(UINT8)( Xhc->HcSParams2 >>27);
  if(MaxScratchpadBuffers==0) {
  	XHC_PEI_ERROR((EFI_D_INFO,"No need MaxScratchpad!\n"));
  	return Status;
  }
  
  PageSize=XhcReadOpReg (Xhc, XHC_PAGESIZE_OFFSET)&0xFFFF;
  XHC_PEI_ERROR((EFI_D_INFO,"MaxScratchpadBuffers=%x,PageSize=%x\n", MaxScratchpadBuffers,PageSize));
  if(PageSize!=1)  {
  	XHC_PEI_ERROR((EFI_D_INFO,"xhc required PageSize is not 4k !\n"));
  	ASSERT(0);
  }
  
  // 2.allocate the array 

  Bytes= MaxScratchpadBuffers * sizeof(UINT64);
  ScratchPadBuffersArray = UsbHcAllocateMem (Xhc, Xhc->MemPool,Bytes);
  if(ScratchPadBuffersArray == NULL){
  	 XHC_PEI_ERROR((EFI_D_INFO,"Scratchpad buffers array allocate fail!\n"));
  	}
  // 3. allocate the pages for entries 
  for(Index=0;Index<MaxScratchpadBuffers;Index++)
  {
       Status = PeiServicesAllocatePages (
                    EfiBootServicesCode,
                        1,
                        &Buf
             );
       if (EFI_ERROR (Status)) {
      	     XHC_PEI_ERROR((EFI_D_INFO,"Scratchpad buffers array allocate fail!\n"));
  	   }
      ScratchPadBuffersArray[Index]=(UINT64)Buf;
  }
  // 4. write the array base address to the DCBAA[0]
  Xhc->DCBAA[0]=(UINT64)ScratchPadBuffersArray;
  
  return EFI_SUCCESS; 
}

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
{
  TRB                 *Trb;
  EVENT_RING          *pEventRing=(EVENT_RING*)NULL;
  //
  //Get the EventRing 
  //
  switch(InterrupterTarget)
  {
  	 case 0:
  	    pEventRing = (EVENT_RING*)&(Xhc->CmdEventRing);
  	   break;
  	 case 1:
  	    pEventRing = (EVENT_RING*)&(Xhc->CtrlTrEventRing);
  	   break;
  	 case 2:
  	    pEventRing = (EVENT_RING*)&(Xhc->BulkTrEventRing);
  	   break;
  	 case 3:
  	    pEventRing = (EVENT_RING*)&(Xhc->AsynIntTrEventRing);
		    
  	     break;
  	 case 4:
  	    pEventRing = (EVENT_RING*)&(Xhc->SynIntTrEventRing);
  	    
  	   break;
  	   default:
  	   pEventRing=(EVENT_RING*)NULL;
  	   break;
  }
  //Get the Next Event TRB
  Trb = EvtTrb;
  if (Trb == (TRB *) ((UINTN) pEventRing->EventRingSeg0 + (sizeof (TRB) *(EVENT_RING_TRB_NUMBER-1)))) 
  {
    Trb  = pEventRing->EventRingSeg0;
    pEventRing->EventRingCCS = (pEventRing->EventRingCCS) ? 0 : 1; //Return the CCS for the Next TRB use 
    XHC_PEI_DEBUG((EFI_D_INFO,"Cycle bit toggle to %x\n", pEventRing->EventRingCCS ));
  }
  else
  {
  	 Trb++;
  }
  pEventRing->EventRingDequeue=Trb;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
XhcSetEventDeQueue (
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
{
   HC_RUNTIME_REGS               *RuntimeRegs;
   UINT32                                InterrupterManagement;
   RuntimeRegs = (HC_RUNTIME_REGS *) (UINTN) (Xhc->UsbHostControllerBaseAddress + Xhc->RTSOff);
   //(1)clear IP
   InterrupterManagement =RuntimeRegs->IR[InterrupterTarget].InterrupterManagement;
   InterrupterManagement=(InterrupterManagement&(~0x02))|0x01;//disable interrupt(IE=0) and clean the IP
   RuntimeRegs->IR[InterrupterTarget].InterrupterManagement =InterrupterManagement;
   //(2)Wait the IP clear
   
		//while(RuntimeRegs->IR[InterrupterTarget].InterrupterManagement&0x01);
		
   //(3)Update the dequeue pointer
     RuntimeRegs->IR[InterrupterTarget].DequePtr.Bits32.LBits32=XHC_LOW_32BIT((UINT64)EvtTrb|0x08);
     RuntimeRegs->IR[InterrupterTarget].DequePtr.Bits32.HBits32=XHC_HIGH_32BIT((UINT64)EvtTrb);
   //(4)Re-enable the IE
    InterrupterManagement =RuntimeRegs->IR[InterrupterTarget].InterrupterManagement;
   if((InterrupterTarget==0)||(InterrupterTarget==3))
    RuntimeRegs->IR[InterrupterTarget].InterrupterManagement=InterrupterManagement|0x02;
    return EFI_SUCCESS;
}

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
{
   
  EVT_TRB_TRANSFER        *EvtTrb;
  EVENT_RING          *pEventRing=(EVENT_RING*)NULL;
  UINT32                  CCS;
  EFI_STATUS                Status;
  //
  //Get the EventRing 
  //
  switch(InterrupterTarget)
  {
  	 case 0:
  	    pEventRing = (EVENT_RING*)&(Xhc->CmdEventRing);
  	   break;
  	 case 1:
  	    pEventRing = (EVENT_RING*)&(Xhc->CtrlTrEventRing);
  	   break;
  	 case 2:
  	    pEventRing = (EVENT_RING*)&(Xhc->BulkTrEventRing);
  	   break;
   case 3:
  	    pEventRing = (EVENT_RING*)&(Xhc->AsynIntTrEventRing);
	  	     break;
  	 case 4:
  	    pEventRing = (EVENT_RING*)&(Xhc->SynIntTrEventRing);
  	       break;
  	  
  	   default:
  	   pEventRing=(EVENT_RING*)NULL;
  	   break;
  }
  //Get Current Even Ring Dequeue Pointer
  EvtTrb = (EVT_TRB_TRANSFER *) pEventRing->EventRingDequeue;
  //Get the CCS of the Even TRB
  CCS = ((TRB*)EvtTrb)->Dword4 & 0x1; 

  while(CCS==pEventRing->EventRingCCS)
  {
        //Check the halt error event and do halt recovery
        if(EvtTrb->Completcode==TRB_COMPLETION_STALL_ERROR)
        	{
        	   XHC_PEI_ERROR((EFI_D_INFO,"Recovery from halt ! \n")); 
        	}
        
        Status=XhcAdvanceDequeuePointer(Xhc,InterrupterTarget,(TRB*)EvtTrb);
        Status=XhcSetEventDeQueue(Xhc, InterrupterTarget, (TRB *) pEventRing->EventRingDequeue);
        EvtTrb = (EVT_TRB_TRANSFER *) pEventRing->EventRingDequeue;
        CCS = ((TRB*)EvtTrb)->Dword4 & 0x1; 
        XHC_PEI_DEBUG((EFI_D_INFO,"Refresh Out dequeuePointer=0x%x\n", EvtTrb)); 
  } 
  //At this point the EventRingDequeue has been refreshed 
}


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
{
  VOID                  *Buf;
  LNK_TRB               *EndTrb;
  UINT64               PhyAddr;
  
  Buf = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (TRB) * CMD_RING_TRB_NUMBER);
  ZeroMem(Buf, sizeof (TRB) * CMD_RING_TRB_NUMBER);
  
  Xhc->CmdRing.CmdRingHeader= Buf;
  Xhc->CmdRing.CmdRingEnqueue = Buf;
  XHC_PEI_ERROR((EFI_D_INFO,"1.Xhc->CmdRingEnqueue=0x%x\n", (UINT64)Xhc->CmdRing.CmdRingEnqueue));
  Xhc->CmdRing.CmdRingDequeue = Buf;
  Xhc->CmdRing.CmdRingPCS = 1;  
  //
  // 4.9.2 Transfer Ring Management
  // To form a ring (or circular queue) a Link TRB may be inserted at the end of a ring to point to the first TRB in
  // the ring.
  //
  EndTrb = (LNK_TRB*) ((UINTN)Buf + sizeof (TRB) * (CMD_RING_TRB_NUMBER - 1));
  EndTrb->Type = TRB_TYPE_LINK;
  EndTrb->PtrLo = XHC_LOW_32BIT (Buf);
  EndTrb->PtrHi = XHC_HIGH_32BIT (Buf);
  EndTrb->TC = 1; //Toggle Cycle (TC). When set to ¡®1¡¯, the xHC shall toggle its interpretation of the Cycle bit.
  EndTrb->CycleBit = 0; //Set Cycle bit as other TRB PCS init value
  
  //
  //The xHC uses the Enqueue Pointer to determine when a Transfer Ring is empty. As it fetches TRBs from a
  //Transfer Ring it checks for a Cycle bit transition. If a transition detected, the ring is empty.
  // So we set RCS as inverted PCS init value to let Command Ring empty
  //
  PhyAddr =  (UINT64)(Xhc->CmdRing.CmdRingHeader);
  PhyAddr &= ~(0x3F);
  PhyAddr |= XHC_CRCR_RCS; //Init the CCS
  
  XHC_PEI_ERROR((EFI_D_INFO,"XhcInitSched:XHC_CRCR=0x%x\n", XHC_LOW_32BIT(PhyAddr)));
  XhcWriteOpReg64 (Xhc, XHC_CRCR_OFFSET, PhyAddr);
  return EFI_SUCCESS; 
 }


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
{
  HC_RUNTIME_REGS               *RuntimeRegs;
  UINT8                         Index;
  EVENT_RING_SEG_TABLE_ENTRY    *TablePtr;
  VOID                          *RingBuf;
  VOID                          *RingHeadPtr;
  
  RingHeadPtr = NULL;
  RingBuf  = NULL;
  RuntimeRegs = (HC_RUNTIME_REGS *) (UINTN) (Xhc->UsbHostControllerBaseAddress + Xhc->RTSOff);
  //(1)Allocate event ring segment entries table 
  TablePtr = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER);
  ZeroMem((VOID *)(UINTN)TablePtr, sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER);
  for (Index= 0; Index < ERST_NUMBER; Index ++) 
  {
    //(2)allocate TRBs buffer for each event ring segment entries
    RingBuf = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (TRB) * EVENT_RING_TRB_NUMBER);
    ZeroMem((VOID *)(UINTN)RingBuf, sizeof (TRB) * EVENT_RING_TRB_NUMBER);
    //(3)save the header of the whole event ring (the same as the first segment's first TRB address)
    if (Index == 0) {
      RingHeadPtr = RingBuf;
    }
    //(4)Init the segment entry with the allocated TRBs info
    ((EVENT_RING_SEG_TABLE_ENTRY *) ((UINTN) TablePtr + Index * sizeof (EVENT_RING_SEG_TABLE_ENTRY)))->PtrLo = XHC_LOW_32BIT(RingBuf);
    ((EVENT_RING_SEG_TABLE_ENTRY *) ((UINTN) TablePtr + Index * sizeof (EVENT_RING_SEG_TABLE_ENTRY)))->PtrHi = XHC_HIGH_32BIT(RingBuf);
    ((EVENT_RING_SEG_TABLE_ENTRY *) ((UINTN) TablePtr + Index * sizeof (EVENT_RING_SEG_TABLE_ENTRY)))->RingTrbSize = EVENT_RING_TRB_NUMBER;    
  }
  //(5)Update the IR related register
  //(5.1)size
  RuntimeRegs->IR[InterrupterTarget].RingSegTableSize = ERST_NUMBER;
  //(5.2)dequeue pointer
  RuntimeRegs->IR[InterrupterTarget].DequePtr.Bits32.LBits32=XHC_LOW_32BIT(RingHeadPtr);
  RuntimeRegs->IR[InterrupterTarget].DequePtr.Bits32.HBits32=XHC_HIGH_32BIT(RingHeadPtr);
 
  //(5.3)Int Enable
  if((InterrupterTarget==0)||(InterrupterTarget==3))
  RuntimeRegs->IR[InterrupterTarget].InterrupterManagement |=0x02;
  //(5.4)ERSTBA
  RuntimeRegs->IR[InterrupterTarget].BasePtr.Bits32.LBits32=XHC_LOW_32BIT(TablePtr);
  RuntimeRegs->IR[InterrupterTarget].BasePtr.Bits32.HBits32=XHC_HIGH_32BIT(TablePtr);

  XHC_PEI_ERROR((EFI_D_INFO,"IR[%x].BasePtr=0x%x\n", InterrupterTarget, TablePtr));
 //(6)Record the Event ring attribute in the Xhc instance
  EventRing->ERSTBase=(VOID*)TablePtr;
  EventRing->EventInterrupter=InterrupterTarget;
  EventRing->EventRingSeg0 = (VOID *) RingHeadPtr;
  EventRing->EventRingEnqueue = (VOID *) RingHeadPtr;
  EventRing->EventRingDequeue = (VOID *) RingHeadPtr;
  EventRing->EventRingCCS = 1;
  
  return EFI_SUCCESS;
  
}


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
{
  return Xhc->AddressToSlot[DevAddr];
}

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
{
  UINT8 Index;
  
  
  if (EpAddr == 0){
    return 1;
  } else {
    Index = 2*EpAddr;
    if (Direction == EfiUsbDataIn) {
      Index++;
    }
    return Index;
  }
}



//
//URB related code 
//

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
{

  USB_ENDPOINT                  *Ep;
  EFI_STATUS                    Status;
  URB                           *Urb;



  Urb = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (URB));

  if (Urb == NULL)
    return NULL;
  
  ZeroMem((VOID *)(UINTN)Urb, sizeof (URB));
  
  


  Urb->Signature  = XHC_URB_SIG;
  InitializeListHead (&Urb->UrbList);

  Ep              = &Urb->Ep;
  Ep->DevAddr     = DevAddr;
  Ep->EpAddr      = EpAddr & 0x0F;
  Ep->Direction   = ((EpAddr & 0x80) ? EfiUsbDataIn : EfiUsbDataOut);
  Ep->DevSpeed    = DevSpeed;
  Ep->MaxPacket   = MaxPacket;
  Ep->Toggle      = Toggle;
  Ep->Type        = Type;
  Ep->PollRate    = XhcConvertPollRate (Interval);

  Urb->Request    = Request;
  Urb->Data       = Data;
  Urb->DataLen    = DataLen;
  Urb->Callback   = Callback;
  Urb->Context    = Context;
  
  Status = XhcCreateTransferTrb (Xhc, Urb);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return Urb;

ON_ERROR:
  UsbHcFreeMem (Xhc->MemPool, Urb, sizeof (URB));
  return NULL;
}

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
{
  
  if (SlotID == 0) {
    XhcWriteDBReg (Xhc, 0, 0);
  } else {
    XhcWriteDBReg (Xhc, SlotID * sizeof (UINT32), DCI);
  }

  return EFI_SUCCESS;
}

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
{


  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   Loop;
  BOOLEAN                 Finished;
  IN UINT8                SlotID;
  IN UINT8                Dci;

  Status    = EFI_SUCCESS;
  Loop      = (TimeOut * XHC_1_MILLISECOND / XHC_SYNC_POLL_INTERVAL) + 1;
  Finished  = FALSE;

  XHC_PEI_DEBUG((EFI_D_INFO,"Loop= %x\n", Loop));
  SlotID = XhcAddressToSlotID(Xhc, Urb->Ep.DevAddr);
  Dci = XhcEPToDci(Urb->Ep.EpAddr, Urb->Ep.Direction);
  XhcRefreshDequeuePointer(Xhc,(UINT8)Urb->InterrupterTarget);
  for (Index = 0; Index < Loop; Index++) 
  {
		    //XHC_DEBUG(("SlotID= %x,Dci=%x\n", SlotID,Dci));
		    XhcRingDoorBell (Xhc, SlotID, Dci);
		    Finished = XhcCheckUrbResult (Xhc, Urb);

		    if (Finished) {
		      break;
		    }
       MicroSecondDelay(XHC_SYNC_POLL_INTERVAL); 
  }
  
  if (!Finished) {
    XHC_PEI_ERROR((EFI_D_INFO,"XhcExecTransfer: transfer not finished in %dms\n", TimeOut));
    Status = EFI_TIMEOUT;

  } else 
  {
			  if (Urb->Result != EFI_USB_NOERROR) 
			  {
			    XHC_PEI_ERROR((EFI_D_INFO,"XhcExecTransfer: transfer failed with %x\n", Urb->Result));
			    Status = EFI_DEVICE_ERROR;
			  }
  	}
 XHC_PEI_DEBUG((EFI_D_INFO,"XhcExecTransfer Status: %r \n",Status));
  return Status;
  
}

EFI_STATUS 
EFIAPI
XhcEndpointHaltRecovery( 
 IN  USB3_HC_DEV         *Xhc,
 IN  URB                       *Urb
 )
  /*++

Routine Description:

			System software shall use a  Reset Endpoint Command (section 4.11.4.7) to remove the  Halted
			condition in the xHC. After the successful completion of the  Reset Endpoint Command, the Endpoint 
			Context is transitioned from the Halted to the  Stopped state and the Transfer Ring of the endpoint is 
			reenabled. The next write  to the Doorbell of the Endpoint will transition the Endpoint Context from the 
			Stopped to the Running state.
 
Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/
 {

     EFI_STATUS              Status;
     EVT_TRB_COMMAND       *EvtTrb; 
     EFI_USB_DEVICE_REQUEST    Request;
     PEI_USB3_DEVICE              *Dev;//
     UINT32                   Result;
     UINTN      Timeout;  
     UINTN     DataLength=0;
     
      XHC_PEI_ERROR((EFI_D_INFO,"Find Halted Slot=%x,Dci=%x\n", Urb->SlotID,Urb->Dci));
      Status=EFI_SUCCESS;
      
      Dev=Xhc->Devices[Urb->SlotID];
      
     //(1)Send Reset endpoint command to transite from halt to stop state
			Status = XhcSendCmd ( Xhc, TRB_TYPE_RESET_ENDPOINT, Urb->SlotID, 0, 0, 0,Urb->Dci,0);
			Status = XhcCheckEvent (
								         Xhc,
								         TRB_TYPE_COMMAND_COMPLT_EVENT,
								         0,
								         XHC_GENERIC_TIMEOUT,
								         (TRB **) &EvtTrb
								         );
			if (EFI_ERROR (Status)) {
			  return Status;
			}


  //(2) Send Clear endpoint halt request to clear the halt state in device
  Request.RequestType = 0x02;
  Request.Request     = USB_DEV_CLEAR_FEATURE;
  Request.Value       = 0;
  Request.Index       = Urb->Ep.EpAddr |0x80;
  Request.Length      = 0;
  Timeout              = 5000;
  if(Urb->Ep.Type !=XHC_CTRL_TRANSFER)
  {
         //EFI_DEADLOOP();
				  Status = XhcControlTransfer (
				                                       (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
																										  &(Xhc->Usb2HostControllerPpi),
																										  Urb->Ep.DevAddr,
																										  Urb->Ep.DevSpeed,
																										  Dev->MaxPacket0,
																										  &Request,
																										  EfiUsbNoData,
																										  NULL,
																										  &DataLength,
																										  Timeout,
																										  NULL,
																										  &Result
																										  );
				  XHC_PEI_ERROR((EFI_D_INFO,"XhcControlTransfer Status: %r \n",Status));
  	}
  //(2)Set dequeue pointer
    XhcSendCmd (Xhc,TRB_TYPE_SET_TR_DEQUE, Urb->SlotID,
    XHC_LOW_32BIT (Urb->EPRing->TransferRingEnqueue)|Urb->EPRing->TransferRingPCS,
    XHC_HIGH_32BIT (Urb->EPRing->TransferRingEnqueue),
    0,Urb->Dci,0);
		 Status = XhcCheckEvent (
										         Xhc,
										         TRB_TYPE_COMMAND_COMPLT_EVENT,
										         0,
										         XHC_GENERIC_TIMEOUT,
										         (TRB **) &EvtTrb
										         );
  //(3)Ring the doorbell to transite from stop to active 
  	 XhcRingDoorBell (Xhc, Urb->SlotID, Urb->Dci);

     return Status; 
    
 }

EFI_STATUS 
EFIAPI
XhcEndpointNoExecuteRecovery( 
 IN  USB3_HC_DEV         *Xhc,
 IN  URB                       *Urb
 )
  /*++

Routine Description:

			System software shall use a  Reset Endpoint Command (section 4.11.4.7) to remove the  Halted
			condition in the xHC. After the successful completion of the  Reset Endpoint Command, the Endpoint 
			Context is transitioned from the Halted to the  Stopped state and the Transfer Ring of the endpoint is 
			reenabled. The next write  to the Doorbell of the Endpoint will transition the Endpoint Context from the 
			Stopped to the Running state.
 
Arguments:

  Device      - The device to apply new Slot

Returns:

  EFI_NOT_FOUND        - There is no configuration with the index
  EFI_OUT_OF_RESOURCES - Failed to allocate resource
  EFI_SUCCESS          - The configuration is selected.

--*/ 
 {
     //
     // 1.stop endpoint 
     //
      EFI_STATUS              Status;
      EVT_TRB_COMMAND       *EvtTrb; 
      PEI_USB3_DEVICE              *Dev;
      DEVICE_CONTEXT     *OutputDevContxt;
     
      XHC_PEI_ERROR((EFI_D_INFO,"Find No Execute Slot=%x,Dci=%x\n", Urb->SlotID,Urb->Dci));
      Status=EFI_SUCCESS;
      Dev=Xhc->Devices[Urb->SlotID];
      OutputDevContxt= Dev->OutputDevContxt;
      XHC_PEI_ERROR((EFI_D_INFO,"Find No Execute Slot=%x,Dci=%x,EPState=%x\n", Urb->SlotID,Urb->Dci, OutputDevContxt->EP[Urb->Dci-1].EPState));
      //(1)Send Reset endpoint command to transite from halt to stop state
  		 Status = XhcSendCmd ( Xhc, TRB_TYPE_STOP_ENDPOINT, Urb->SlotID, 0, 0, 0,Urb->Dci,0);
  		 Status = XhcCheckEvent (
  								         Xhc,
  								         TRB_TYPE_COMMAND_COMPLT_EVENT,
  								         0,
  								         XHC_GENERIC_TIMEOUT,
  								         (TRB **) &EvtTrb
  								         );
  			if (EFI_ERROR (Status)) {
  	  	  XHC_PEI_ERROR((EFI_D_INFO,"stop endpoint cmd fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
  			  return Status;
  			}
       //(2)Set dequeue pointer
       XhcSendCmd (Xhc,TRB_TYPE_SET_TR_DEQUE, Urb->SlotID,
       XHC_LOW_32BIT (Urb->EPRing->TransferRingEnqueue)|Urb->EPRing->TransferRingPCS,
       XHC_HIGH_32BIT (Urb->EPRing->TransferRingEnqueue),
       0,Urb->Dci,0);
   		 Status = XhcCheckEvent (
   										         Xhc,
   										         TRB_TYPE_COMMAND_COMPLT_EVENT,
   										         0,
   										         XHC_GENERIC_TIMEOUT,
   										         (TRB **) &EvtTrb
   										         );
   		 	if (EFI_ERROR (Status)) {
   	  	  XHC_PEI_ERROR((EFI_D_INFO,"set dequeue point cmd fail, Completcode=%x!\n",((EVT_TRB_COMMAND*)EvtTrb)->Completcode));
   	  	  return Status;
   			}
       //(3)Ring the doorbell to transite from stop to active 
     	 XhcRingDoorBell (Xhc, Urb->SlotID, Urb->Dci);
       XHC_PEI_ERROR((EFI_D_INFO,"End No Execute Slot=%x,Dci=%x,EPState=%x\n", Urb->SlotID,Urb->Dci, OutputDevContxt->EP[Urb->Dci-1].EPState));
       return Status;
 }

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
{
        VOID                    *Buf;
        LNK_TRB                 *EndTrb;

        //
        //Allocate and initialize the Transfer Ring for the new Bulk Endpoint.
        // ps. UsbAllocatePool is 64 bytes alignment
        //

        Buf = UsbHcAllocateMem (Xhc, Xhc->MemPool, sizeof (TRB) * TR_RING_TRB_NUMBER);
        ZeroMem((VOID *)(UINTN)Buf, sizeof (TRB) * TR_RING_TRB_NUMBER);
        Device->EPRing[Dci-1].EPType=EPType;
        Device->EPRing[Dci-1].TransferRing = Buf;
        Device->EPRing[Dci-1].TransferRingEnqueue = Buf;
        Device->EPRing[Dci-1].TransferRingDequeue = Buf;
        Device->EPRing[Dci-1].TransferRingPCS = 1;  
        //
        // To form a ring (or circular queue) a Link TRB may be inserted at the end of a ring to point to the first TRB in
        // the ring.
        //
        EndTrb = (LNK_TRB*) ((UINTN)Buf + sizeof (TRB) * (TR_RING_TRB_NUMBER - 1));
        EndTrb->Type = TRB_TYPE_LINK;
        EndTrb->PtrLo = XHC_LOW_32BIT (Buf);
        EndTrb->PtrHi = XHC_HIGH_32BIT (Buf);
        EndTrb->TC = 1; //Toggle Cycle (TC). When set to ¡®1¡¯, the xHC shall toggle its interpretation of the Cycle bit.
        EndTrb->CycleBit = 0; //Set Cycle bit as other TRB PCS init value
        
        XHC_PEI_ERROR((EFI_D_INFO,"Dci-1=%x\n", Dci-1));  
        XHC_PEI_ERROR((EFI_D_INFO,"TransferRing=0x%x\n",Device->EPRing[Dci-1].TransferRing)); 
       return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
XhcTransferRingEnqueueAdvance (
  IN USB3_HC_DEV          *Xhc,
  IN ENDPOINT_RING   			*EPRing
)
{
  TRB                  *TempTrb;
  UINT8                TRBType;
   
 
  TempTrb=(TRB*)EPRing->TransferRingEnqueue;
  TempTrb++;
  //
  //Check the reach the Link TRB or not , if so toggle the PCS and update the CmdRingEnqueue 
  //
  TRBType = (UINT8) ((TempTrb->Dword4 >> 10) & 0x3f);
  if (TRBType == TRB_TYPE_LINK) 
  {
    if(TempTrb->Dword4 & 0x2) 
    {
    	//set cycle bit in Link TRB as normal
      TempTrb->Dword4 &= ~(0x01);
      TempTrb->Dword4 |=(EPRing->TransferRingPCS)?1:0;
      //Toggle PCS maintained by software.
      EPRing->TransferRingPCS= (EPRing->TransferRingPCS)? 0:1;
      EPRing->TransferRingEnqueue=(VOID*)(UINTN)(TempTrb->Dword1 & ~(0x0f));
    }
  }
  else
  {
  	EPRing->TransferRingEnqueue=(VOID*)TempTrb;
  }
  //Add full check!bugbug_zyq1
  ((TRB*)EPRing->TransferRingEnqueue)->Dword1=0;//clear previous TRB content
  ((TRB*)EPRing->TransferRingEnqueue)->Dword2=0;//clear previous TRB content
  ((TRB*)EPRing->TransferRingEnqueue)->Dword3=0;//clear previous TRB content
  ((TRB*)EPRing->TransferRingEnqueue)->Dword4  &=0x01;//clear previous TRB content ,except cycle bit 
  return EFI_SUCCESS;
}

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
{

  UINT8                         SlotID;
  UINT8                         Dci;
  UINT8                         EPType;
  DEVICE_CONTEXT                *OutputDevContxt;
  TRB                           *TrbStart;
  UINTN                         Len;
  UINTN                         TotalLen;
  UINTN                         TrbNum;
  PEI_USB3_DEVICE          *Device;
  ENDPOINT_RING   			        *EPRing;
  EFI_STATUS                    Status;
  
  Urb->TransferTrbNum = 0;
  SlotID = XhcAddressToSlotID(Xhc, Urb->Ep.DevAddr);
  Urb->SlotID=SlotID;
  OutputDevContxt = (DEVICE_CONTEXT*) (UINTN) Xhc->DCBAA[SlotID];  
  Urb->OutputDevContxt=OutputDevContxt;
  Dci = XhcEPToDci(Urb->Ep.EpAddr, Urb->Ep.Direction); //for doorbell use
  Urb->Dci=Dci;

  if(OutputDevContxt->EP[Dci-1].EPState ==2) 
  {
       XHC_PEI_ERROR((EFI_D_INFO,"endpoint halted!\n"));
       return EFI_DEVICE_ERROR;
  }
  //
  //Find the USB device Instance
  //
  Device=Xhc->Devices[SlotID];
  EPRing=(ENDPOINT_RING*)&(Device->EPRing[Dci-1]);  
  Urb->EPRing=EPRing;
  //
  //Record the Urb's first  TRB
  //
  Urb->TransferTrbStart = (TRB*)EPRing->TransferRingEnqueue; //Record the head TRB of this URB
  XHC_PEI_DEBUG((EFI_D_INFO,"SlotID=0x%x,OutputDevContxt=0x%x,TsfEq=0x%x,PCS=0x%x\n", SlotID,OutputDevContxt,EPRing->TransferRingEnqueue,EPRing->TransferRingPCS));

  EPType = (UINT8)EPRing->EPType;
  switch (EPType) {
    case ED_CONTROL_BIDIR:
      TrbStart=(TRB*)EPRing->TransferRingEnqueue; //Get the enqueue pointer!
      Urb->InterrupterTarget = Xhc->CtrlTrEventRing.EventInterrupter;
      //
      // Note that init the TRB items in structure defined sequence
      // Setup stage
      //
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->bmRequestType = Urb->Request->RequestType;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->bRequest = Urb->Request->Request;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->wValue = Urb->Request->Value;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->wIndex = Urb->Request->Index;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->wLength = Urb->Request->Length;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->Lenth = 8;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->RsvdZ1 = 0;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->IntTarget = (UINT32) Urb->InterrupterTarget;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->RsvdZ2 = 0;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->IOC = 1;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->IDT = 1;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->RsvdZ3 = 0;
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->Type = TRB_TYPE_SETUP_STAGE;
      if (Urb->Ep.Direction == EfiUsbDataIn) {
        ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->TRT = 3;
      } else if (Urb->Ep.Direction == EfiUsbDataOut) {
        ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->TRT = 2;
      } else {
        ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->TRT = 0;
      }
      ((TRANSFER_TRB_CONTROL_SETUP *) TrbStart)->RsvdZ4 = 0;
       
      //Update the cycle bit 
      TrbStart->Dword4 &= ~(0x1);
      TrbStart->Dword4 |= EPRing->TransferRingPCS;
      //Get Next TRB for data stage use 
      Status=XhcTransferRingEnqueueAdvance(Xhc,EPRing);
      Urb->TransferTrbNum++;
      
      //
      // Data stage
      //
      TrbStart=EPRing->TransferRingEnqueue;
      if (Urb->DataLen > 0) 
      {
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->TRBPtrLo = XHC_LOW_32BIT(Urb->Data);
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->TRBPtrHi = XHC_HIGH_32BIT(Urb->Data);
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->Lenth = (UINT32) Urb->DataLen;
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->TDSize = 0; //bugbug
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->IntTarget = (UINT32) Urb->InterrupterTarget;
        TrbStart->Dword4 &= 0x1; // clear all Dword4 except the cycle bit
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->ISP = 1;
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->IOC = 1;
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->IDT = 0;
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->CH=0;
        ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->Type = TRB_TYPE_DATA_STAGE;
        if (Urb->Ep.Direction == EfiUsbDataIn) {
          ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->DIR = 1;
        } else if (Urb->Ep.Direction == EfiUsbDataOut) {
          ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->DIR = 0;
        } else {
          ((TRANSFER_TRB_CONTROL_DATA *) TrbStart)->DIR = 0;
        }
      //Update the cycle bit 
      TrbStart->Dword4 &= ~(0x1);
      TrbStart->Dword4 |= EPRing->TransferRingPCS;
      
      //Get Next TRB for status stage use 
      Status=XhcTransferRingEnqueueAdvance(Xhc,EPRing); 
      Urb->TransferTrbNum++;  //Record the TRBs Number of this URB
    }
      //
      // Status stage
      //
      TrbStart=EPRing->TransferRingEnqueue;
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->RsvdZ1 = 0; 
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->RsvdZ2 = 0; 
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->RsvdZ3 = 0; 
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->IntTarget = (UINT32) Urb->InterrupterTarget;
      TrbStart->Dword4 &= 0x1; // clear all Dword4 except the cycle bit
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->IOC = 1;
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->CH=0;
      ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->Type = TRB_TYPE_STATUS_STAGE;
      if (Urb->Ep.Direction == EfiUsbDataIn) {
        ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->DIR = 0;
      } else if (Urb->Ep.Direction == EfiUsbDataOut) {
        ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->DIR = 1;
      } else {
        ((TRANSFER_TRB_CONTROL_STATUS *) TrbStart)->DIR = 0;
      }
      //Update the cycle bit 
      TrbStart->Dword4 &= ~(0x1);
      TrbStart->Dword4 |= EPRing->TransferRingPCS;
      Urb->TransferTrbEnd=TrbStart; //Record the end TRB of this URB
      //Update the enqueue pointer 
      Status=XhcTransferRingEnqueueAdvance(Xhc,EPRing);
      Urb->TransferTrbNum++;  //need to confirm!!!!
      XHC_PEI_DEBUG((EFI_D_INFO,"TrbNb=%x\n", Urb->TransferTrbNum));
      break;
    
    case ED_BULK_OUT:
    case ED_BULK_IN:
      Urb->InterrupterTarget = Xhc->BulkTrEventRing.EventInterrupter;
      TotalLen = 0;
      Len = 0;
      TrbNum = 0;
      TrbStart=EPRing->TransferRingEnqueue; //Only for compile
      while (TotalLen < Urb->DataLen) 
      {
        TrbStart=EPRing->TransferRingEnqueue;
        if ((TotalLen + 0x10000) >= Urb->DataLen) 
        {
          Len = Urb->DataLen - TotalLen;
        } else 
        {
          Len = 0x10000;
        }
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TRBPtrLo = XHC_LOW_32BIT((UINT8 *) Urb->Data + TotalLen);
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TRBPtrHi = XHC_HIGH_32BIT((UINT8 *) Urb->Data + TotalLen);
        ((TRANSFER_TRB_NORMAL *) TrbStart)->Lenth = (UINT32) Len;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TDSize = 0; //bugbug
        ((TRANSFER_TRB_NORMAL *) TrbStart)->IntTarget = (UINT32) Urb->InterrupterTarget;
        TrbStart->Dword4 &= 0x1; // clear all Dword4 except the cycle bit
        ((TRANSFER_TRB_NORMAL *) TrbStart)->ISP = 1;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->IOC = 1;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->Type = TRB_TYPE_NORMAL;
        
        //Update the cycle bit 
	      TrbStart->Dword4 &= ~(0x1);
	      TrbStart->Dword4 |= EPRing->TransferRingPCS;
	      //Get Next TRB for status stage use 
	      Status=XhcTransferRingEnqueueAdvance(Xhc,EPRing);
	      TrbNum++;  //Record the TRBs Number of this URB
        TotalLen += Len; // max buffer is 64K
      }
      
      Urb->TransferTrbEnd=TrbStart;  //Record the end TRB of this URB
      Urb->TransferTrbNum=TrbNum;
      XHC_PEI_DEBUG((EFI_D_INFO,"TrbNb=%x\n", Urb->TransferTrbNum));
      break;
    
    case ED_INTERRUPT_OUT:
    case ED_INTERRUPT_IN:
    	if(Urb->Ep.Type== XHC_INT_TRANSFER_ASYNC)
      Urb->InterrupterTarget = Xhc->AsynIntTrEventRing.EventInterrupter; 
    	if(Urb->Ep.Type== XHC_INT_TRANSFER_SYNC)
    	Urb->InterrupterTarget = Xhc->SynIntTrEventRing.EventInterrupter; 
    	
      TotalLen = 0;
      Len = 0;
      TrbNum = 0;
      TrbStart=EPRing->TransferRingEnqueue; //Only for compile
      while (TotalLen < Urb->DataLen) 
      {
        TrbStart=EPRing->TransferRingEnqueue;
        if ((TotalLen + 0x10000) >= Urb->DataLen) 
        {
          Len = Urb->DataLen - TotalLen;
        } else 
        {
          Len = 0x10000;
        }
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TRBPtrLo = XHC_LOW_32BIT((UINT8 *) Urb->Data + TotalLen);
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TRBPtrHi = XHC_HIGH_32BIT((UINT8 *) Urb->Data + TotalLen);
        ((TRANSFER_TRB_NORMAL *) TrbStart)->Lenth = (UINT32) Len;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->TDSize = 0; //bugbug
        ((TRANSFER_TRB_NORMAL *) TrbStart)->IntTarget = (UINT32) Urb->InterrupterTarget;
        TrbStart->Dword4 &= 0x1; // clear all Dword4 except the cycle bit
        ((TRANSFER_TRB_NORMAL *) TrbStart)->ISP = 1;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->IOC = 1;
        ((TRANSFER_TRB_NORMAL *) TrbStart)->Type = TRB_TYPE_NORMAL;
        
        //Update the cycle bit 
	      TrbStart->Dword4 &= ~(0x1);
	      TrbStart->Dword4 |= EPRing->TransferRingPCS;
	      //Get Next TRB for status stage use 
	      Status=XhcTransferRingEnqueueAdvance(Xhc,EPRing);
	      TrbNum++;  //Record the TRBs Number of this URB
        // ASSERT (TrbStart != XhcGetCmdTransferEnQueue (Xhc, Urb, &NextPCS));
        TotalLen += Len; // max buffer is 64K
      }
      
      Urb->TransferTrbEnd=TrbStart;  //Record the end TRB of this URB
      Urb->TransferTrbNum=TrbNum;
      XHC_PEI_DEBUG((EFI_D_INFO,"TrbNb=%x\n", Urb->TransferTrbNum));
      break;
    
    default:
      XHC_PEI_ERROR((EFI_D_INFO,"NonSupportEPType=0x%x\n",EPType));
      return EFI_UNSUPPORTED;
      break;
  }
  

  return EFI_SUCCESS;

}

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
{
  UINTN                   BitCount;

  if (Interval == 0) {
    return 1;
  }

  //
  // Find the index (1 based) of the highest non-zero bit
  //
  BitCount = 0;

  while (Interval != 0) {
    Interval >>= 1;
    BitCount++;
  }

  return (UINTN)1 << (BitCount - 1);
}


BOOLEAN 
	EFIAPI
	XhcUrbEventCheck( 
  IN  URB                       *Urb,
  IN  UINT32                   TRBPtrLo)
 {
     BOOLEAN Result;
     Result=FALSE;
     
     if(XHC_LOW_32BIT(Urb->TransferTrbStart)<XHC_LOW_32BIT(Urb->TransferTrbEnd))
     { 
        //start<end
        if((TRBPtrLo >= XHC_LOW_32BIT(Urb->TransferTrbStart))&&
  	      	  ((TRBPtrLo <= XHC_LOW_32BIT(Urb->TransferTrbEnd)))
  	      	)
  	      	Result=TRUE;
     }
     else
     {
          //start>end, toggle occured in Event TRBs caused by the Urb
          if((TRBPtrLo >= XHC_LOW_32BIT(Urb->TransferTrbStart))||
  	      	  ((TRBPtrLo <= XHC_LOW_32BIT(Urb->TransferTrbEnd)))
  	      	)
          Result= TRUE;
     }
     return Result;
     
 }

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
{

  BOOLEAN                 Finished;
  EVT_TRB_TRANSFER        *EvtTrb=(EVT_TRB_TRANSFER*)NULL;
  UINTN                   Index=0;
  UINT8                   TRBType; 
  EVENT_RING              *pEventRing=(EVENT_RING*)NULL;
  EFI_STATUS              Status;
  UINT32                  CCS;
  TRB                     *AsyIntTRB;   

  
  ASSERT ((Xhc != NULL) && (Urb != NULL)); 

  Finished        = FALSE;
  Urb->Completed  = 0;

  Urb->Result     = EFI_USB_NOERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    Urb->Result |= EFI_USB_ERR_SYSTEM;
    goto ON_EXIT;
  }

  //
  //(1) Get the EventRing 
  //
  switch(Urb->InterrupterTarget)
  {
  	 case 0:
  	    pEventRing = (EVENT_RING*)&(Xhc->CmdEventRing);
		    XHC_PEI_DEBUG((EFI_D_INFO,"CMD event Ring\n"));
  	   break;
  	 case 1:
  	    pEventRing = (EVENT_RING*)&(Xhc->CtrlTrEventRing);
		    XHC_PEI_DEBUG((EFI_D_INFO,"Ctrl\n"));
  	   break;
  	 case 2:
  	    pEventRing = (EVENT_RING*)&(Xhc->BulkTrEventRing);
		    XHC_PEI_DEBUG((EFI_D_INFO,"Bulk\n"));
  	   break;
  	 case 3:
  	    pEventRing = (EVENT_RING*)&(Xhc->AsynIntTrEventRing);
		    XHC_PEI_DEBUG((EFI_D_INFO,"AsynInt event Ring\n"));
  	     break;
  	 case 4:
  	    pEventRing = (EVENT_RING*)&(Xhc->SynIntTrEventRing);
  	    XHC_PEI_DEBUG((EFI_D_INFO,"SynInt event Ring\n"));
  	   break;
  	   default:
  	   pEventRing=(EVENT_RING*)NULL;
  	   break;
  }
  //(2)Get dequeue pointer
  EvtTrb = (EVT_TRB_TRANSFER *) pEventRing->EventRingDequeue;
  CCS = ((TRB*)EvtTrb)->Dword4 & 0x1;
  AsyIntTRB =(TRB*)(UINTN)(((TRB*)EvtTrb)->Dword1);
  XHC_PEI_DEBUG((EFI_D_INFO,"Dq= %x\n", EvtTrb));
  XHC_PEI_DEBUG((EFI_D_INFO,"no=%x\n", Urb->TransferTrbNum));
  
   //(3)check the Urb related Event TRB
   if(CCS==pEventRing->EventRingCCS)
  	{
  	      XHC_PEI_DEBUG((EFI_D_INFO,"TRBPtrLo= %x,Start=%x,End=%x\n", EvtTrb->TRBPtrLo,Urb->TransferTrbStart,Urb->TransferTrbEnd));
  	      if(XhcUrbEventCheck(Urb,EvtTrb->TRBPtrLo))
	  	     {
			        for (Index = 0; Index < Urb->TransferTrbNum; Index++) 
							  {
							       CCS = ((TRB*)EvtTrb)->Dword4 & 0x1;
							       if(CCS==pEventRing->EventRingCCS)
							       {
										    		if (EvtTrb->Type == TRB_TYPE_TRANS_EVENT) 
												    {
												      switch (EvtTrb->Completcode) {
												      case TRB_COMPLETION_STALL_ERROR:
												        Urb->Result |= EFI_USB_ERR_STALL;
												        XHC_PEI_ERROR((EFI_D_INFO,"EFI_USB_ERR_STALL\n")); 
												        Finished = TRUE;
												        goto ON_EXIT;  
												        break;
												        
												      case TRB_COMPLETION_BABBLE_ERROR:
												        Urb->Result |= EFI_USB_ERR_BABBLE;
												        XHC_PEI_ERROR((EFI_D_INFO,"TRB_COMPLETION_BABBLE_ERROR\n")); 
												        Finished = TRUE;
												        goto ON_EXIT;  
												        break;

												      case TRB_COMPLETION_DATA_BUFFER_ERROR:
												        Urb->Result |= EFI_USB_ERR_BUFFER;
												        XHC_PEI_ERROR((EFI_D_INFO,"TRB_COMPLETION_DATA_BUFFER_ERROR\n")); 
												        Finished = TRUE;
												        goto ON_EXIT;  
												        break;

												      case TRB_COMPLETION_USB_TRANSACTION_ERROR:
												        Urb->Result |= EFI_USB_ERR_TIMEOUT;
												        XHC_PEI_ERROR((EFI_D_INFO,"TRB_COMPLETION_USB_TRANSACTION_ERROR\n")); 
												        Finished = TRUE;
												        goto ON_EXIT;  
												        break;
															case TRB_COMPLETION_SHORT_PACKET:
																	      		 XHC_PEI_DEBUG((EFI_D_INFO,"AsyInt short packets\n")); 
												      case TRB_COMPLETION_SUCCESS:
												        TRBType = (UINT8) ((((TRB *)(UINTN)((EvtTrb->TRBPtrLo)))->Dword4 >> 10)) & 0x3f;
												        if (TRBType == TRB_TYPE_DATA_STAGE || 
												            TRBType == TRB_TYPE_NORMAL ||
												            TRBType == TRB_TYPE_ISOCH) {
												            Urb->Completed +=(Urb->DataLen- EvtTrb->Lenth);   
												            XHC_PEI_DEBUG((EFI_D_INFO,"Urb->Completed =%x\n",Urb->Completed));      
												            if(EvtTrb->Completcode==TRB_COMPLETION_SHORT_PACKET) XHC_PEI_ERROR((EFI_D_INFO,"short packets data!\n")); 
												        }
												        //
												        // finial Transfer TRB finish successfully
												        //
												        if (EvtTrb->TRBPtrLo == XHC_LOW_32BIT(Urb->TransferTrbEnd))
												        {
												    	        //update the dequeue pointer!
													  	          Status=XhcAdvanceDequeuePointer(Xhc,(UINT8)Urb->InterrupterTarget,(TRB*)EvtTrb);
													  	          Status=XhcSetEventDeQueue(Xhc, (UINT8) Urb->InterrupterTarget, (TRB *) pEventRing->EventRingDequeue);
												               Finished = TRUE;
												               XHC_PEI_DEBUG((EFI_D_INFO,"Transfer Event check successfull\n"));
												               if(EvtTrb->Completcode==TRB_COMPLETION_SHORT_PACKET)  XHC_PEI_ERROR((EFI_D_INFO,"short packets success!\n")); 
												               goto ON_EXIT;     
												        }
												        break;
														
														default:
														     XHC_PEI_ERROR((EFI_D_INFO,"Transfer Default Occur =%x!\n",EvtTrb->Completcode));
												        Urb->Result |= EFI_USB_ERR_TIMEOUT;
												        Finished = TRUE;
												        goto ON_EXIT;  
												        break;
												      }
													  
												    }
										    		else
										    		{
			                         XHC_PEI_ERROR((EFI_D_INFO,"_2_Abnormal Event TRB Occur!\n"));
										    		}
													 //
											    //Get Next event 
											    //
											    Status=XhcAdvanceDequeuePointer(Xhc,(UINT8)Urb->InterrupterTarget,(TRB*)EvtTrb);
											    Status=XhcSetEventDeQueue(Xhc, (UINT8) Urb->InterrupterTarget, (TRB *) pEventRing->EventRingDequeue);
											    EvtTrb = (EVT_TRB_TRANSFER *) pEventRing->EventRingDequeue;
							      }
							  }
	  	     }
  	       else//advance the Event TRBs that not caused by the URB 
  	       {
  	            XHC_PEI_ERROR((EFI_D_INFO,"_1_Abnormal Event TRB Occur!!!!\n")); 
  	            //EFI_DEADLOOP();//avoid abnormal event lead to system hang
  	            Status=XhcAdvanceDequeuePointer(Xhc,(UINT8)Urb->InterrupterTarget,(TRB*)EvtTrb);
  	            Status=XhcSetEventDeQueue(Xhc, (UINT8) Urb->InterrupterTarget, (TRB *) pEventRing->EventRingDequeue);
  	       }
  	}

  ON_EXIT:  
  //bugbug, we need re-enble the endpoint if transfer fail by  Reset Endpoint Command (14) and Set TR Dequeue Pointer Command (16)
  return Finished;
}


EFI_STATUS
EFIAPI
XhcCommandRingEnqueueAdvance (
  IN USB3_HC_DEV          *Xhc
)
{
    TRB                     *LinkTrb; 
    UINT8                   TRBType; 
    
    Xhc->CmdRing.CmdRingEnqueue++; 
    //
    //Check the reach the Link TRB or not , if so toggle the PCS and update the CmdRingEnqueue 
    //
    LinkTrb=Xhc->CmdRing.CmdRingEnqueue;
    TRBType = (UINT8) ((LinkTrb->Dword4 >> 10) & 0x3f);
    if (TRBType == TRB_TYPE_LINK) 
    {
		      if(LinkTrb->Dword4 & 0x2) 
		      	{
				      	 //set cycle bit in Link TRB as normal
				        LinkTrb->Dword4 &= ~(0x01);
				        LinkTrb->Dword4 |=(Xhc->CmdRing.CmdRingPCS)?1:0;
				        //Toggle PCS maintained by software.
				        Xhc->CmdRing.CmdRingPCS= (Xhc->CmdRing.CmdRingPCS)? 0:1;
				        Xhc->CmdRing.CmdRingEnqueue=(TRB*)(UINTN)(LinkTrb->Dword1 & ~(0x0f));
		      }
    }
    //Add full check!bugbug_zyq1
    
  ((TRB*)Xhc->CmdRing.CmdRingEnqueue)->Dword1=0;//clear previous TRB content
  ((TRB*)Xhc->CmdRing.CmdRingEnqueue)->Dword2=0;//clear previous TRB content
  ((TRB*)Xhc->CmdRing.CmdRingEnqueue)->Dword3=0;//clear previous TRB content
  ((TRB*)Xhc->CmdRing.CmdRingEnqueue)->Dword4  &=0x01;//clear previous TRB content ,except cycle bit 
    return EFI_SUCCESS;
}


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
{
  EFI_STATUS              Status;
  TRB                     *CmdTrb;  
  
  if (TRBType == 0 || TRBType > 39) {
    return EFI_UNSUPPORTED;
  }
  
  Status = EFI_SUCCESS;
  CmdTrb = Xhc->CmdRing.CmdRingEnqueue;
  XHC_PEI_DEBUG((EFI_D_INFO, "2.Xhc->CmdRingEnqueue=0x%x\n", (UINT64)Xhc->CmdRing.CmdRingEnqueue));
  
  switch (TRBType) {
  case TRB_TYPE_EN_SLOT:
    //
    // 4.6.3 Enable Slot
    //
    ((CMD_TRB_EN_SLOT *) CmdTrb)->RsvdZ0 = 0;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->Type = TRBType;
    ((CMD_TRB_EN_SLOT *) CmdTrb)->RsvdZ4 = 0;
      
    break;
  case TRB_TYPE_DIS_SLOT:
    if (SlotID > Xhc->MaxSlotsEn){
      return EFI_UNSUPPORTED;
    }
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->RsvdZ0 = 0;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->Type = TRBType;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->RsvdZ4 = 0;
    ((CMD_TRB_DIS_SLOT *) CmdTrb)->SlotID = SlotID;
    break;
  case TRB_TYPE_ADDRESS_DEV:
    if ((SlotID > Xhc->MaxSlotsEn) || (PtrLo == 0))  {
      return EFI_UNSUPPORTED;
    }
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->PtrLo = PtrLo;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->PtrHi = PtrHi;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->BSR = BSR;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->Type = TRBType;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_TRB_ADDR_DEV *) CmdTrb)->SlotID = SlotID;
    break;
  case TRB_TYPE_CON_ENDPOINT:
    if ((SlotID > Xhc->MaxSlotsEn) || (PtrLo == 0))  {
      return EFI_UNSUPPORTED;
    }
    ((CMD_CFG_ED *) CmdTrb)->PtrLo = PtrLo;
    ((CMD_CFG_ED *) CmdTrb)->PtrHi = PtrHi;
    ((CMD_CFG_ED *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_CFG_ED *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_CFG_ED *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_CFG_ED *) CmdTrb)->DC = 0; //bugbug, should support DC=1
    ((CMD_CFG_ED *) CmdTrb)->Type = TRBType;
    ((CMD_CFG_ED *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_CFG_ED *) CmdTrb)->SlotID = SlotID;  
    break;
  case TRB_TYPE_EVALU_CONTXT:
    if ((SlotID > Xhc->MaxSlotsEn) || (PtrLo == 0))  {
      return EFI_UNSUPPORTED;
    }
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->PtrLo = PtrLo;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->PtrHi = PtrHi;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->Type = TRBType;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_TRB_EVALU_CONTX *) CmdTrb)->SlotID = SlotID;  
    break;
  case TRB_TYPE_RESET_ENDPOINT:
  	 if (SlotID > Xhc->MaxSlotsEn){
      return EFI_UNSUPPORTED;
    }
    ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ0 = 0;
    ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_TRB_RESET_ED *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_TRB_RESET_ED *) CmdTrb)->TSP=TSP;
    ((CMD_TRB_RESET_ED *) CmdTrb)->Type = TRBType;
    ((CMD_TRB_RESET_ED *) CmdTrb)->EDID=Dci;
    ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ4 = 0;
    ((CMD_TRB_RESET_ED *) CmdTrb)->SlotID = SlotID;
    break;
  case TRB_TYPE_STOP_ENDPOINT:
    break;
  case TRB_TYPE_SET_TR_DEQUE:
    ((CMD_SET_TR_DEQ *) CmdTrb)->PtrLo = PtrLo;
    ((CMD_SET_TR_DEQ *) CmdTrb)->PtrHi = PtrHi;
    ((CMD_SET_TR_DEQ *) CmdTrb)->RsvdZ1 = 0;
    ((CMD_SET_TR_DEQ *) CmdTrb)->StreamID=0;
    ((CMD_SET_TR_DEQ *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_SET_TR_DEQ *) CmdTrb)->RsvdZ2 = 0;
    ((CMD_SET_TR_DEQ *) CmdTrb)->Type = TRBType;
    ((CMD_SET_TR_DEQ *) CmdTrb)->EDID=Dci;
     ((CMD_TRB_RESET_ED *) CmdTrb)->RsvdZ3 = 0;
    ((CMD_SET_TR_DEQ *) CmdTrb)->SlotID = SlotID;  
    break;
  case TRB_TYPE_RESET_DEV:
    break;
  case TRB_TYPE_GET_PORT_BANW:
    break;
  case TRB_TYPE_FORCE_HEADER:
    break;
  case TRB_TYPE_NO_OP_COMMAND:
    ((CMD_NO_OP_TRB *) CmdTrb)->RsvdZ0=0;
    ((CMD_NO_OP_TRB *) CmdTrb)->RsvdZ1=0;
    ((CMD_NO_OP_TRB *) CmdTrb)->RsvdZ2=0;
    ((CMD_NO_OP_TRB *) CmdTrb)->CycleBit = Xhc->CmdRing.CmdRingPCS;
    ((CMD_NO_OP_TRB *) CmdTrb)->RsvdZ3=0;  
    ((CMD_NO_OP_TRB *) CmdTrb)->Type = TRBType;
    ((CMD_NO_OP_TRB *) CmdTrb)->RsvdZ4=0;
    break;
  case TRB_TYPE_LINK:
    break;
    
  default:
    return EFI_UNSUPPORTED;
  }
  Xhc->CmdRing.CmdTrbPtr=CmdTrb; //Record the command Trb will be executed
  Status=XhcCommandRingEnqueueAdvance(Xhc);
  XhcRefreshDequeuePointer(Xhc,0);
  XhcRingDoorBell (Xhc, 0, 0);
  
  return Status;
}


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
{
   EFI_STATUS           Status;
   TRB                      *EvtTrb;
   UINT32                 CCS;
   TRB                      *CmdTrbPtr;  
   UINTN                   Loop;
   UINTN                   Index;
   
   Loop      = (TimeOut / XHC_SYNC_POLL_INTERVAL) + 1;
   for (Index = 0; Index < Loop; Index++) 
   {
			   //(1)Get the event ring dequeue pointer and CCS
			   EvtTrb = Xhc->CmdEventRing.EventRingDequeue;
			   //(2)check if Event generated
			   CmdTrbPtr =(TRB*)(UINTN)(((TRB*)EvtTrb)->Dword1);
			   CCS = ((TRB*)EvtTrb)->Dword4 & 0x1;
			   if(CCS==(Xhc->CmdEventRing.EventRingCCS))
			   	{     
			   	      //Event TRB is caused by the command TRB
			         if(CmdTrbPtr==Xhc->CmdRing.CmdTrbPtr)
			         	{
			               if (((EvtTrb->Dword4 >> 10) & 0x3f) == TRBType) 
			               { 
						               *Trb = EvtTrb;
						               if (((EvtTrb->Dword3 >> 24) & 0xff) == TRB_COMPLETION_SUCCESS) 
						               {
						                    //Command completed successfully,then advance the dequeue pointer
						                    XHC_PEI_DEBUG((EFI_D_INFO,"CMD complete Event\n"));
						                    Status=XhcAdvanceDequeuePointer(Xhc,0,(TRB*)EvtTrb);
			                         Status=XhcSetEventDeQueue(Xhc, 0, (TRB *) Xhc->CmdEventRing.EventRingDequeue); 
			                         return EFI_SUCCESS;
						               }
						               else
						               {
			              	           XHC_PEI_ERROR((EFI_D_INFO,"CMD fail Event\n"));
			                          return EFI_DEVICE_ERROR;
						               }
			         	      }
			          }
			     }
			    MicroSecondDelay(XHC_SYNC_POLL_INTERVAL);
  }
   return EFI_TIMEOUT;
}

