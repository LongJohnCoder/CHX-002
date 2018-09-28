//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2015 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//********************************************************************** 

#ifdef ZX_SECRET_CODE   

#include <PlatformDefinition.h>
#include <Library/CpuDebugLib.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>

STATIC MSR_CONFIG_INFO_DUMP		  FsbcConfigInfo[20];

STATIC UINT32	TracerMsrGroup[]= {
	0x120f,0x1203,0x144a,0x144b,0x160f,0x1625,0x1200,
	0x1204,0x1621,0x1504,0x1506,0x120e,0x1259,0x1255,
	0x1301,0x1302,0x1303,0x1304,0x1305,0x1306,0x1307};

STATIC MSR_CONFIG_INFO_DUMP TracerConfigInfo[0x20*16]={0};

VOID DumpTracerMsr(){
	UINT8 i = 0;
	UINT64 MsrValue;
	UINT32		Eax, Ebx;
	UINT32		cpuid;
	UINT8       SocketId;
	
	// LAPIC ID
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;
	IoWrite8(0x80,(UINT8)cpuid);
	
	//Socket ID
	SocketId = (AsmReadMsr64(0x1610)>>3)&0x01;
	IoWrite8(0x80,(UINT8)SocketId);
	
	for(i=0;i<21;i++){
		MsrValue = AsmReadMsr64(TracerMsrGroup[i]);
		TracerConfigInfo[cpuid*0x20+i].SocketId= SocketId;
		TracerConfigInfo[cpuid*0x20+i].ApicId = cpuid;
		TracerConfigInfo[cpuid*0x20+i].MsrAddress = TracerMsrGroup[i];
		TracerConfigInfo[cpuid*0x20+i].MsrValue	= MsrValue; 
	} 
}
VOID DumpTracerMsr1()
{
	UINT8 i = 0;
	DEBUG((EFI_D_ERROR,"Begin to Dump TRACER MSR\n"));
	DEBUG((EFI_D_ERROR,"============================================\n"));
	for(i=0;i<0x20*4;i++){
		if(TracerConfigInfo[i].MsrAddress!=0){
			DEBUG((EFI_D_ERROR,"Core %d,socket %d,TRACER_MSR[%x]:%llx\n",
						TracerConfigInfo[i].ApicId,
						TracerConfigInfo[i].SocketId,
						TracerConfigInfo[i].MsrAddress,
						TracerConfigInfo[i].MsrValue
						));
		}
	} 
	DEBUG((EFI_D_ERROR,"============================================\n"));
}

VOID DumpFsbcMsr1()
{
	UINT8 i = 0;
	DEBUG((EFI_D_ERROR,"Begin to Dump FSBC MSR\n"));
	DEBUG((EFI_D_ERROR,"============================================\n"));
	for(i=0;i<20;i++){
		if(FsbcConfigInfo[i].MsrAddress!=0){
			DEBUG((EFI_D_ERROR,"Core %d,socket %d,FSBC_MSR[%x]:%llx\n",
						FsbcConfigInfo[i].ApicId,
						FsbcConfigInfo[i].SocketId,
						FsbcConfigInfo[i].MsrAddress,
						FsbcConfigInfo[i].MsrValue
						));
		}
	} 
	DEBUG((EFI_D_ERROR,"============================================\n"));
}

VOID DumpFsbcMsr()
{
	UINT8 i = 0;
	UINT64 MsrValue;
	UINT32		Eax, Ebx;
	UINT32		cpuid;
	UINT8       SocketId;
	
	// LAPIC ID
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;
	IoWrite8(0x80,(UINT8)cpuid);
	
	//Socket ID
	SocketId = (AsmReadMsr64(0x1610)>>3)&0x01;
	IoWrite8(0x80,(UINT8)SocketId);
	
	////
	for(i=0;i<9;i++){
		MsrValue = AsmReadMsr64(0x1604+i);
		FsbcConfigInfo[SocketId*9+i].SocketId= SocketId;
		FsbcConfigInfo[SocketId*9+i].ApicId = cpuid;
		FsbcConfigInfo[SocketId*9+i].MsrAddress = 0x1604+i;
		FsbcConfigInfo[SocketId*9+i].MsrValue   = MsrValue;	

		if(((0x1604+i)==0x1609)||((0x1604+i)==0x160B)){
			/// Debug purpose, Meaningless for A0 Chip, only for PXP debug purpose.
			IoWrite8(0x80,(UINT8)(4+i));
			IoWrite8(0x80,(UINT8)(MsrValue));
			IoWrite8(0x80,(UINT8)(MsrValue>>8));
			IoWrite8(0x80,(UINT8)(MsrValue>>16));
			IoWrite8(0x80,(UINT8)(MsrValue>>24));
			IoWrite8(0x80,(UINT8)(MsrValue>>32));
			IoWrite8(0x80,(UINT8)(MsrValue>>40));
			IoWrite8(0x80,(UINT8)(MsrValue>>48));
			IoWrite8(0x80,(UINT8)(MsrValue>>56));
		}
	} 
}

VOID InitPcie(UINT8 RootBusNum,UINT8 PciePortNum,BOOLEAN IsSoccapEn)
{ 
    UINT8  Value8;
    UINT32 Value32;
    UINT64 Value64;
    UINTN Address;	
    UINT16	Pmio = 0x800;
    
	//Dump to PCIE
	Address = PCI_DEV_MMBASE(RootBusNum, 17, 0)+D17F0_PMU_PM_IO_BASE;
	Pmio = MmioRead16(Address)&0xff00;
//	DEBUG((EFI_D_ERROR,"Addr:%x, PMIO BAR=%x\n",Address,Pmio));
	
	//D0F4 Rx42 bit 4
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 4)+D0F4_CHIP_TEST_MODE;
	Value8 = MmioRead8(Address);
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_D0F4_Rx42=%x\n",Address,Value8));
	Value8 = Value8 | D0F4_RFSBCDBG;
	MmioWrite8(Address,Value8);
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_D0F4_Rx42=%x\n",Address,MmioRead8(Address)));
	
	//D0F4 Rx47 bit 7
	
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 4)+D0F4_DEBUG_SEL_SIGNAL_0+3;
	Value8 = MmioRead8(Address);
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_D0F4_Rx47=%x\n",Address,Value8));
	if(IsSoccapEn)
		Value8 = Value8 & (~BIT7);
	else
		Value8 = Value8 | BIT7;
	
	MmioWrite8(Address,Value8);
//	DEBUG((EFI_D_ERROR,"Addr:%x,Af_D0F4_Rx47=%x\n",Address,MmioRead8(Address)));
	
	//D0F5 Read Rx268 for RCRB-H
	
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 5)+D0F5_RCRB_H_BASE_ADR;
	Value32 = MmioRead32(Address);
//	DEBUG((EFI_D_ERROR,"Addr:%x,D0F5_Rx268=%x\n",Address,Value32));
	
	Value64 = (UINT64)(Value32 & 0x0FFFFFFF);
	Value64 = LShiftU64(Value64, 12);
	
	Value64 += (UINT64)RCRBH_MISC_0; //0x260      CJW_20170512  PEG2 changed to PE6(D5F0)   
//	DEBUG((EFI_D_ERROR,"RPEGDBG_PEXC_Address=%llx\n",Value64));
	
	Value8 = MmioRead8((UINTN)Value64);
//	DEBUG((EFI_D_ERROR,"Bf_RPEGDBG_PEXC=%x\n",Value8));
	if(PciePortNum==0)  //select PE0(D0F3,Haps) as the output port
	{
		//Set bit0, clear bit1

		Value8 |= RCRBH_RPE0DBG_PEXC;
		Value8 &= ~RCRBH_RPE4DBG_PEXC;
		Address = PCI_DEV_MMBASE(RootBusNum, 3, 0)+D3D5F1_BRIDGE_CTL;
	}
	else
	{
		//sellect PE4(D4F0 ,PXP)
		//Set bit1, clear bit0
		Value8 |= RCRBH_RPE4DBG_PEXC;
		Value8 &= ~RCRBH_RPE0DBG_PEXC;
		Address = PCI_DEV_MMBASE(RootBusNum, 4, 0)+D3D5F1_BRIDGE_CTL;
	}
    
	MmioWrite8((UINTN)Value64,Value8);
//	DEBUG((EFI_D_ERROR,"af_RPEGDBG_PEXC=%x\n",MmioRead8(Value64)));
	
	//PCIE hot reset 0->1->0 dxf0 rx3e bit 6
	Value8 = MmioRead8(Address);
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_DxF0_Rx3e=%x\n",Address,Value8));
	Value8 |= D3D5F1_RSRST; //Bit 6
	MmioWrite8(Address,Value8);//write 1
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_DxF0_Rx3e=%x\n",Address,MmioRead8(Address)));
	
	MicroSecondDelay(20000);
	
	Value8 &= ~D3D5F1_RSRST;
	MmioWrite8(Address,Value8);//write 0
//	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_DxF0_Rx3e=%x\n",Address,MmioRead8(Address)));
		
	// wait for PCIE link stable
    //Delay 
	MicroSecondDelay(20000);	

	return;
}

VOID ConfigFsbc(
	EFI_PHYSICAL_ADDRESS 		FSBCBaseAddress,
	UINT8		FsbcBufferSize,
	UINT8		CycleType,
	FSBC_TRIGGER_CONDITION	*FsbcTrigger,
	UINT8		RootBusNum
)
{
   UINT64 MsrValue;
   UINT16 Pmio = 0x800;
   UINTN Address; 
   UINT32  Value32;
   BOOLEAN IsStreamModeEn;
   BOOLEAN IsSoccapEn;
   BOOLEAN IsDumpToPCIE;

   IsDumpToPCIE = FsbcTrigger->IsDumpToPCIE;
   IsStreamModeEn  = FsbcTrigger->IsStreamModeEn;
   IsSoccapEn	 = FsbcTrigger->IsSoccapEn;
   
   MsrValue = AsmReadMsr64(0x16a7);
    if((MsrValue&0x01)==0){
//	    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7, AsmReadMsr64(0x16a7)));
  		AsmWriteMsr64(0x16a7, (MsrValue|0x01));
    }
	
	//Dump to PCIE
	Address = PCI_DEV_MMBASE(RootBusNum, 17, 0)+D17F0_PMU_PM_IO_BASE;
	Pmio = MmioRead16(Address)&0xff00;
//	DEBUG((EFI_D_ERROR,"Addr:%x, PMIO BAR=%x\n",Address,Pmio));

   	if(FsbcTrigger->OtherTriggerMethod.CPU_FSBC_MISSPACKE_EN)
	{
		//PMIO Rx94
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 &= 0xF0000000;
		//Value32 |= PMIO_PAD_WR;
		Value32 |= BIT16;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
		
		//PMIO Rx8C
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA);
		Value32 &= (~BIT29);
		Value32 |= BIT28;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);

		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 |= PMIO_PAD_WR;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
		Value32 &= (~PMIO_PAD_WR);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
	}

	if(FsbcTrigger->OtherTriggerMethod.CPU_FSBC_TIGPULSE_EN)
	{
		//FSBC_TIGPULSE
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 &= 0xF0000000;
		//Value32 |= PMIO_PAD_WR;
		Value32 |= BIT17;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);

		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA);
		Value32 &= (~BIT12);
		Value32 &= (~BIT13);
		Value32 |= BIT11;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);

		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 |= PMIO_PAD_WR;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
		Value32 &= (~PMIO_PAD_WR);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
	}
	
	if(FsbcTrigger->OtherTriggerMethod.CPU_FSBC_IFSBCSTP_EN)
	{
		//FSBC_IFSBCSTP
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 &= 0xF0000000;
		//Value32 |= PMIO_PAD_WR;
		Value32 |= BIT17;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);

		Value32=IoRead32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA);
		Value32 &= (~BIT18);
		Value32 &= (~BIT17);
		Value32 |= BIT16;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);

		Value32=IoRead32(Pmio + PMIO_PAD_CTL_REG);
		Value32 |= PMIO_PAD_WR;
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
		Value32&=(~PMIO_PAD_WR);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,Value32);
	}


	if(IsSoccapEn==1)
    {
						
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_READ_DATA);
		Value32 &= ~(0x00000038);
		Value32 |= 0x00000018; 
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x80040000);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);

		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_READ_DATA);
		Value32 &= ~(0x00000007);
		Value32 |= 0x00000002; 
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x80040000);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);
		//user need set D0F4 Rx50,Rx52,RSOCCAP_DUMPBS,RSOCCAP_DUMPSZ,RSOCCAP_DUMP_EN
	}else{
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_READ_DATA);
		Value32 &= ~(0x00000038);
		Value32 |= 0x00000000; 
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);
	    IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x80040000);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);						
		
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);
		Value32 = IoRead32(Pmio + PMIO_PAD_CTL_REG_READ_DATA);
		Value32 &= ~(0x00000007);
		Value32 |= 0x000000000; 
		IoWrite32(Pmio + PMIO_PAD_CTL_REG_WRITE_DATA,Value32);
	    IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x80040000);
		IoWrite32(Pmio + PMIO_PAD_CTL_REG,0x00040000);
	  }
				   
   // Config FSBC base address of data buffer and size
   
   MsrValue = AsmReadMsr64(0x160B);
   MsrValue |= (FSBCBaseAddress|((UINT64)((UINT64)FsbcBufferSize<<40)));
   AsmWriteMsr64(0x160B, MsrValue);

   if(IsDumpToPCIE||IsSoccapEn){
        InitPcie(RootBusNum,FsbcTrigger->PciePortNum,IsSoccapEn);
   }		 
   
   MsrValue = AsmReadMsr64(0x160B);
   if(!IsStreamModeEn){
		// snap shot mode(no selective trigger mode) or not,Dump to Dram
		
		if(FsbcTrigger->FsbcTriggerPosition!=EnumFsbcTriggerPositionSnapShotMode){
			if(FsbcTrigger->IsNeedConfigTriggerCondition){
				MsrValue |=(UINT64)FsbcTrigger->FsbcTriggerPosition<<44;
				MsrValue |=(UINT64)FsbcTrigger->TriggerType<<49;
				MsrValue |=(UINT64)FsbcTrigger->IsWriteTriggerTransaction<<60;
				MsrValue |=(UINT64)FsbcTrigger->TriggerTransaction<<61;
			}else if(IsDumpToPCIE){
				MsrValue |= 0x1004C00000000000;    //wrap around, 50% , trigger mode, pcie dump
			}else{
				MsrValue |= 0x1004400000000000;		// wrap around, 50% trigger
			}
		}
   }else{
		//CPU FSBC Stream Mode enable ,Dump to pcie, stream mode , snapshot mode
		
		MsrValue |= ((UINT64)0x1<<47|(UINT64)0x1<<48);//dump to PCIE,value:0x0001800050000000=
   }
   
   AsmWriteMsr64(0x160B, MsrValue);
   
   MsrValue = AsmReadMsr64(0x1609);
   MsrValue |=(((UINT64)CycleType)<<8);

   // for mem dump,160 trigger mode
   if(!IsStreamModeEn){
   		MsrValue |=((UINT64)0xB)<<29;   // for mem dump
   }
   
   AsmWriteMsr64(0x1609, MsrValue);
   
   return;
}

VOID StartFsbc(BOOLEAN IsStartDump)
{
   UINT64 MsrValue;
   
   MsrValue = AsmReadMsr64(0x1609);
   
   if(IsStartDump){
  	 MsrValue |= ((UINT64)0x07);
   }else{
  	 MsrValue &= ((UINT64)~0x07);
   }
   
   AsmWriteMsr64(0x1609, MsrValue);   
   DumpFsbcMsr();
   //for debug
   MsrValue = AsmReadMsr64(0x160B);
//   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x160B,MsrValue));
   MsrValue = AsmReadMsr64(0x1609);
//   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x1609,MsrValue));
   MsrValue = AsmReadMsr64(0x160C);
//   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x160C,MsrValue));
}


//// For FSBC init, every socket only need init one core, not need init all APs.
//// So, this function will do FSBC init based on cpuid.
VOID 
EFIAPI
fsbc_init (
  IN OUT VOID  *Buffer
)
{
	UINT32		Eax, Ebx;
	UINT32		cpuid;
	BOOLEAN     IsSlaveSocket;
	EFI_PHYSICAL_ADDRESS    FsbcAddress=0;
	UINT8	FsbcSize = 0;
	UINT8   RootBusNum=0;
	FSBC_TRIGGER_CONDITION	*FsbcTrigger;
	FSBC_CONFIG_PARA *Arg;
	
	Arg = (FSBC_CONFIG_PARA*)Buffer;
	FsbcSize = Arg->FsbcSize;
	FsbcTrigger = Arg->FsbcTriggerCondition;
	
	// LAPIC ID
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;
	IoWrite8(0x80,(UINT8)cpuid);
	
	//Socket ID
	IsSlaveSocket = (AsmReadMsr64(0x1610)>>3)&0x01;
	IoWrite8(0x80,(UINT8)IsSlaveSocket);

	if((cpuid==0x00)||(cpuid==0x08)){
		if(cpuid==0x00){
			RootBusNum =0;
			FsbcAddress = Arg->MasterFsbcBase;		
		}else if(cpuid==0x08){
			RootBusNum =0x80;
			FsbcAddress = Arg->SlaveFsbcBase;	
		}
		ConfigFsbc(FsbcAddress,FsbcSize,FSBC_All,FsbcTrigger,RootBusNum);
		StartFsbc(TRUE);		
	}
	
	return;
}

VOID SwitchBevoClk()
{
	UINT64 Value64;
	UINT64 TscThreshold;

	//MSR 0x1438 bit30,  dis tweeter req
	
	Value64 = AsmReadMsr64(0x1438);
	Value64|=(UINT64)(1<<30);
	AsmWriteMsr64(0x1438,Value64);

	//MSR 0x16a8[63:32],save Tsc thresld then write Tsc thresld to 0

	Value64 = AsmReadMsr64(0x16a8);
	TscThreshold = (Value64>>32)&0xFFFFFFFF;
	Value64&=~((UINT64)0xFFFFFFFF<<32);
	AsmWriteMsr64(0x16a8,Value64);

	MicroSecondDelay(6);

	//MSR 0x143a[2], BEVO Clk =100MHz

	Value64 = AsmReadMsr64(0x143a);
	Value64&=~((UINT64)1<<2);
	AsmWriteMsr64(0x143a,Value64);

	//MSR 0x16a8[0],switch send Tsc cnt(100Mhz)

	Value64 = AsmReadMsr64(0x16a8);
	Value64|=(UINT64)(0x01<<0);
	AsmWriteMsr64(0x16a8,Value64);

	//MSR 0x16a8[23:8],Disable TSC offset Modify

	Value64 = AsmReadMsr64(0x16a8);
	Value64&=~((UINT64)0xFFFF<<8);
	Value64|=(UINT64)(0x5A<<8);
	AsmWriteMsr64(0x16a8,Value64);

	//MSR 0x16a8[63:32],recover Tsc thresld

	Value64 = AsmReadMsr64(0x16a8);	
	Value64|=(UINT64)(TscThreshold<<32);
	AsmWriteMsr64(0x16a8,Value64);
	
	//MSR 0x1438 bit30, recover tweeter req
	Value64 = AsmReadMsr64(0x1438);
	Value64&=~((UINT64)1<<30);
	AsmWriteMsr64(0x1438,Value64);
}

VOID
SetAllMSRFunc_Before(IN OUT VOID  *Buffer)
{
  UINT64 Value64;
  UINT32		Eax, Ebx;
  UINT32		cpuid;
  UINT64        Address64 = 0; 
  UINT8         RootBusNum=0;
	
	// LAPIC ID
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;

  IoWrite8(0x80,(UINT8)cpuid);
  
  if((cpuid==0x00)||(cpuid==0x08)){
  	
	 if(cpuid==0x00){
		RootBusNum =0;
	 }else if(cpuid==0x08){
		RootBusNum =0x80;
	 }
	  
  	 Address64 = PCI_DEV_MMBASE(RootBusNum, 0, 4)+D0F4_VDD_OFF_DEBUG_CTL;
	// IoWrite8(0x80,0xAD);
	 //IoWrite8(0x80,0xAD);
	 ///
	 /// Debug purpose, Meaningless for A0 Chip, only for PXP debug purpose.
	 IoWrite8(0x80,(UINT8)(Address64));
	 IoWrite8(0x80,(UINT8)(Address64>>8));
	 IoWrite8(0x80,(UINT8)(Address64>>16));
	 IoWrite8(0x80,(UINT8)(Address64>>24));
	 IoWrite8(0x80,0xF1);
	 IoWrite8(0x80,0xF2);

 	 AsmWriteMsr64(0x1621,Address64);
#ifdef CHX002_PXP
	 SwitchBevoClk();
#endif
  }	
  
  //MSR 0x120f bit18/bit42/bit4
  Value64 = AsmReadMsr64(0x120f);
  Value64|=(UINT64)0x40000000000;
  Value64|=(UINT64)(1<<18);
  Value64|=(UINT64)(1<<4);
  AsmWriteMsr64(0x120f,Value64);
  
  //MSR 0x1204 BIT24
  AsmWriteMsr64(0x1523,0x69bcb2964735c3a5);
  Value64 = AsmReadMsr64(0x1204);
  Value64|=(UINT64)(1<<24);
  AsmWriteMsr64(0x1204,Value64);

  //MSR 0x1203 bit27
  Value64 = AsmReadMsr64(0x1203);
  Value64|=(UINT64)(1<<27);
  AsmWriteMsr64(0x1203,Value64);
  
  //MSR 0x144a bit0/bit21
  Value64 = AsmReadMsr64(0x144a);
  Value64|=(UINT64)(1<<21|1);
  AsmWriteMsr64(0x144a,Value64);
  
  //MSR 0x144b bit6/4/7
  Value64 = AsmReadMsr64(0x144b);
  Value64|=(UINT64)(1<<6);
#ifndef CHX002_PXP
  Value64|=(UINT64)(1<<4);
  Value64|=(UINT64)(1<<7);
#endif
  AsmWriteMsr64(0x144b,Value64);
  
  //MSR 0x160f bit2
  Value64 = AsmReadMsr64(0x160f);
  Value64|=(UINT64)(1<<2);
  AsmWriteMsr64(0x160f,Value64);

  //MSR 0x1625 bit27
  Value64 = AsmReadMsr64(0x1625);
  Value64|=(UINT64)((UINT64)1<<27);
  AsmWriteMsr64(0x1625,Value64);
	
  //
  //[20160712]ALJ add CPUID off,jimmy request- MSR 0x1200 bit56,clear
  //
  Value64 = AsmReadMsr64(0x1200);
  Value64&=~((UINT64)1<<56);
  AsmWriteMsr64(0x1200,Value64);
#ifndef CHX002_PXP
  //MSR 0x1504 bit0,clear
    Value64 = AsmReadMsr64(0x1504);
  Value64&=~((UINT64)1<<0);
  AsmWriteMsr64(0x1504,Value64);

    //MSR 0x1506 bit0
  Value64 = AsmReadMsr64(0x1506);
  Value64|=(UINT64)((UINT64)1<<0);
  AsmWriteMsr64(0x1506,Value64);
#endif
  return;
  
}

VOID
SetAllMSRFunc_After(IN OUT VOID  *Buffer)
{
  UINT64 Value64;
  UINT32		Eax, Ebx;
  UINT32		cpuid;
	
	// LAPIC ID
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;

  IoWrite8(0x80,(UINT8)cpuid);
  //MSR 0x1403 BIT1
  AsmWriteMsr64(0x1523,0xff48ebed44d20a2b);
  Value64 = AsmReadMsr64(0x1403);
  Value64|=(UINT64)(0x1<1);
  AsmWriteMsr64(0x1403,Value64);

  //MSR 0x1204 BIT4
  AsmWriteMsr64(0x1523,0x69bcb2964735c3a5);
  Value64 = AsmReadMsr64(0x1204);
  Value64|=(UINT64)(1<<4);
  AsmWriteMsr64(0x1204,Value64);
#ifndef CHX002_PXP

  //MSR 0x120e BIT0/3
  Value64 = AsmReadMsr64(0x120e);
  Value64|=(UINT64)(1<<0);
  Value64|=(UINT64)(1<<3);
  AsmWriteMsr64(0x120e,Value64);

    //MSR 0x1259 BIT17/18
  Value64 = AsmReadMsr64(0x1259);
  Value64|=(UINT64)(1<<17);
  Value64|=(UINT64)(1<<18);
  AsmWriteMsr64(0x1259,Value64);

    //MSR 0x1204 BIT24
  AsmWriteMsr64(0x1523,0x69bcb2964735c3a5);
  Value64 = AsmReadMsr64(0x1204);
  Value64|=(UINT64)(1<<24);
  AsmWriteMsr64(0x1204,Value64);

    //MSR 0x120f bit54
  Value64 = AsmReadMsr64(0x120f);
  Value64|=(UINT64)(((UINT64)1)<<54);
  AsmWriteMsr64(0x120f,Value64);

  //MSR 0x1255 bit51
  Value64 = AsmReadMsr64(0x1255);
  Value64|=(UINT64)(((UINT64)1)<<51);
  AsmWriteMsr64(0x1255,Value64);
#endif
  DumpTracerMsr();
  return;
}


VOID 
EFIAPI
SetMSR (
  IN OUT VOID  *Buffer
  )
{

	UINT32		Eax, Ebx;
	UINT32		cpuid;
	UINT64		Value64;
	CPU_CONTEXT  *context;
	EFI_PHYSICAL_ADDRESS Address;

	context = (CPU_CONTEXT*)(Buffer);
	
	//LAPIC ID
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;
	IoWrite8(0x80,(UINT8)cpuid);
	if(cpuid>=8){
		Address =(EFI_PHYSICAL_ADDRESS)(context->SlaveAddress+(cpuid-0x08)*0x1000000);
	}else{
		Address =(EFI_PHYSICAL_ADDRESS)(context->MasterAddress+cpuid*0x1000000);
	}
	
	/// for PXP debug purpose, because it's easy to watch waveform on PXP platform.
	IoWrite8(0x80,(UINT8)(Address>>24));
	IoWrite8(0x80,(UINT8)(Address>>32));
	
	//MSR 0x1301,BSP or AP??? dual socket???
	
	if(cpuid==0x0){
		AsmWriteMsr64(0x1301,  0x00000000d003ffde);
//	AsmWriteMsr64(0x1301,  0x000000001203ffde);
	}
	else{
		 AsmWriteMsr64(0x1301, 0x000000005003ffde);
	}
	
	//MSR 0x1302
	
	AsmWriteMsr64(0x1302, 0x0100000577fbf2ac);
	
	//MSR 0x1303,BSP or AP??? dual socket???
	if(cpuid==0x0){
		AsmWriteMsr64(0x1303,	0x000b0003000bb249);
	}
	else{
		AsmWriteMsr64(0x1303, 0x000b0003000b3249);
	}

	AsmWriteMsr64(0x1305, Address);
	AsmWriteMsr64(0x1306, 0x00000000000009a4);
	//enable Tracer
	AsmWriteMsr64(0x1523, 0x956c32fddb02b09e);
	AsmWriteMsr64(0x317c, 0x0000000000000005);

	//MSR 0x1307,BSP or AP??? dual socket???
	if(cpuid==0x0){
		AsmWriteMsr64(0x1307,context->NumOfInstPer2Dump);//Tr70
	}
	else{
		AsmWriteMsr64(0x1307,0x0000080000000);
	}

	//MSR 0x120f BIT32
	AsmWriteMsr64(0x1523,0x3675828b73ac1757);
	Value64 = AsmReadMsr64(0x120f);
	Value64|=(UINT64)0x100000000;
	AsmWriteMsr64(0x120f,Value64);

	return;
}

#endif

