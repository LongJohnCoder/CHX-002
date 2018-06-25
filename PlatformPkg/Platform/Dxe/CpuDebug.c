#ifdef ZX_SECRET_CODE   

#include "PlatformDxe.h"
#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>
#include <Library/TimerLib.h>

#define UC_SIZE_256MB  0x0
#define UC_SIZE_512MB  0x1
#define UC_SIZE_1024MB 0x2
#define UC_SIZE_2048MB 0x3

#define FSBC_C2P        BIT0
#define FSBC_P2C        BIT1
#define FSBC_C2M        BIT2
#define FSBC_Asyc       BIT4
#define FSBC_Debug      BIT5
#define FSBC_All        BIT7

#define FSBC_TRIGGER_TYPE_SOCCAP		BIT0
#define FSBC_TRIGGER_TYPE_TRANSACTION	BIT1
#define FSBC_TRIGGER_TYPE_REQUEST_COUNT	BIT2
#define FSBC_TRIGGER_TYPE_NO_REQUEST	BIT3
#define FSBC_TRIGGER_TYPE_APIC			BIT4
#define FSBC_TRIGGER_TYPE_GPIO			BIT5
#define FSBC_TRIGGER_TYPE_USER_STOP		BIT6
#define FSBC_TRIGGER_TYPE_HIF_HANGE		BIT7

#define FSBC_TRIGGER_POSITION_SNAPSHOT_MODE  0x000

typedef enum {
    EnumFsbcTriggerPositionSnapShotMode,
    EnumFsbcTriggerPosition12_5,
    EnumFsbcTriggerPosition25,
    EnumFsbcTriggerPosition37_5,
    EnumFsbcTriggerPosition50,
    EnumFsbcTriggerPosition62_5,
    EnumFsbcTriggerPosition75,
    EnumFsbcTriggerPosition87_5
} FSBC_TRIGGER_POSITION;

typedef enum {
    EnumFsbcTransactionC2P,
    EnumFsbcTransactionC2M,
    EnumFsbcTransactionP2C,
    EnumFsbcTransactionVPITX,
    EnumFsbcTransactionVPIRX
} FSBC_TRIGGER_TRANSACTION;

typedef struct _FSBC_TRIGGER_CONDITION{
	BOOLEAN						IsDumpToPCIE;
	BOOLEAN						IsNeedConfigTriggerCondition;
	FSBC_TRIGGER_POSITION		FsbcTriggerPosition;
	UINT16						TriggerType;
	UINT8						TriggerTransaction;
	BOOLEAN						IsWriteTriggerTransaction;
}FSBC_TRIGGER_CONDITION;

typedef struct _FSBC_CONFIG_PARA{
	FSBC_TRIGGER_CONDITION		*FsbcTriggerCondition;
	EFI_PHYSICAL_ADDRESS        MasterFsbcBase;
	EFI_PHYSICAL_ADDRESS        SlaveFsbcBase;
	UINT8						FsbcSize;
	BOOLEAN						IsMasterEn;
	BOOLEAN						IsSlaveEn;	
}FSBC_CONFIG_PARA;

typedef struct _FSBC_CONFIG_INFO{
	UINT32		ApicId;
	UINT8       SocketId;
	UINT64      MsrAddress;
	UINT64	    MsrValue;
}FSBC_CONFIG_INFO;

EFI_STATUS
EFIAPI
EnableFsbcSnapShotMode(
	IN EFI_FSBC_DUMP_PROTOCOL  *This
);

EFI_FSBC_DUMP_PROTOCOL mFsbcDump={
	EnableFsbcSnapShotMode
};

EFI_PHYSICAL_ADDRESS      MasterBase = 0;    //hxz add for spc flow
EFI_PHYSICAL_ADDRESS      SlaveBase = 0;    //hxz add for spc flow

EFI_MP_SERVICES_PROTOCOL  *ptMpSvr;
UINTN					  NumberOfProcessors;
UINTN					  NumberOfEnabledProcessors;  
FSBC_CONFIG_INFO		  FsbcConfigInfo[20];

EFI_PHYSICAL_ADDRESS
EFIAPI
AllocateReservedAndUcMemory(
	EFI_PHYSICAL_ADDRESS	 Address,
	UINT8 Size
)
{
	EFI_STATUS				 Status;
	EFI_CPU_ARCH_PROTOCOL	 *pCpuArch;
	UINT32	Length = 0;
	
	switch (Size)
	{
		case UC_SIZE_256MB:
	            Length = 0x10000000;
	            break;
	    case UC_SIZE_512MB:
	            Length = 0x20000000;
	            break;
		case UC_SIZE_1024MB:
				Length = 0x40000000;
				break;
		case UC_SIZE_2048MB:
				Length = 0x80000000;
				break;
	    default:
	            Length = 0x10000000;
	            break;
	}
		
	DEBUG((EFI_D_ERROR,"Mike_Address:%llx Length:%llx\n",Address,Length));
    IoWrite8(0x80,(UINT8)(Address>>24));
    IoWrite8(0x80,(UINT8)(Address>>32));
    IoWrite8(0x80,(UINT8)(Length>>24));
	
	if(Address<SIZE_4GB){
	  IoWrite8(0x80,0x70);
	  Status =gBS->AllocatePages(
	  	                AllocateAddress,
	                    EfiReservedMemoryType,
	                    EFI_SIZE_TO_PAGES (Length),
	                    &Address);
	  if(Status!=0){
	  	DEBUG((EFI_D_ERROR,"Mike_AllocateFailed:%d,address:%llx\n",Status,Address));
		IoWrite8(0x80,0xDE);
		IoWrite8(0x80,0xAD);
		IoWrite8(0x80,0xE7);
		CpuDeadLoop();
	  }
  	}
	
	//set UC
    Status = gBS->LocateProtocol (
                      &gEfiCpuArchProtocolGuid,
                      NULL,
                      &pCpuArch
                      );
  	ASSERT_EFI_ERROR (Status);
	
	IoWrite8(0x80,0x71);
 	Status = pCpuArch->SetMemoryAttributes (
	                      pCpuArch,
	                      Address,
	                      Length, 
	                      EFI_MEMORY_UC
	                      );
  
   if(Status!=0){
  	 DEBUG((EFI_D_ERROR,"Mike_SetMemoryAttribtuesFaild:%d\n",Status));
	 IoWrite8(0x80,0xDE);
	 IoWrite8(0x80,0xAD);
	 IoWrite8(0x80,0xE8);
	 CpuDeadLoop();
   }
	
	return Address;
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
	
	for(i=0;i<9;i++){
		MsrValue = AsmReadMsr64(0x1604+i);
		FsbcConfigInfo[SocketId*9+i].SocketId= SocketId;
		FsbcConfigInfo[SocketId*9+i].ApicId = cpuid;
		FsbcConfigInfo[SocketId*9+i].MsrAddress = 0x1604+i;
		FsbcConfigInfo[SocketId*9+i].MsrValue   = MsrValue;		
		if(((0x1604+i)==0x1609)||((0x1604+i)==0x160B)){
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
VOID TestMmioConf(UINT8 RootBusNum)
{ 
    UINT8  Value8;
    UINT32 Value32;
    UINT64 Value64;
    UINT64 Address;	
    UINT16 Pmio;
    
	Address = PCI_DEV_MMBASE(RootBusNum, 17, 0)+D17F0_PMU_PM_IO_BASE;
	Pmio = MmioRead16(Address)&0xff00;
	DEBUG((EFI_D_ERROR,"Addr:%x, PMIO BAR=%x\n",Address,Pmio));
	
	//D0F4 Rx42 bit 4
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 4)+D0F4_CHIP_TEST_MODE;
	Value8 = MmioRead8(Address);
	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_D0F4_Rx42=%x\n",Address,Value8));
	
	//D0F4 Rx47 bit 7
	
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 4)+D0F4_DEBUG_SEL_SIGNAL_0+3;
	Value8 = MmioRead8(Address);
	DEBUG((EFI_D_ERROR,"Addr:%x,Bf_D0F4_Rx47=%x\n",Address,Value8));

	
	//D0F5 Read Rx268 for RCRB-H
	
	Address = PCI_DEV_MMBASE(RootBusNum, 0, 5)+D0F5_RCRB_H_BASE_ADR;
	Value32 = MmioRead32(Address);
	DEBUG((EFI_D_ERROR,"Addr:%x,D0F5_Rx268=%x\n",Address,Value32));
	
	Value64 = (UINT64)(Value32 & 0x0FFFFFFF);
	Value64 = LShiftU64(Value64, 12);
	
	Value64 += (UINT64)RCRBH_MISC_0; //0x260      CJW_20170512  PEG2 changed to PE6(D5F0)   
	DEBUG((EFI_D_ERROR,"RPEGDBG_PEXC_Address=%llx\n",Value64));
	
	Value8 = MmioRead8(Value64);
	DEBUG((EFI_D_ERROR,"Bf_RPEGDBG_PEXC=%x\n",Value8));

    //Delay 
	MicroSecondDelay(20000);	

	return;
}

VOID InitPcie(UINT8 RootBusNum)
{ 
    UINT8  Value8;
    UINT32 Value32;
    UINT64 Value64;
    UINT64 Address;	
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
	if(gSetupData->CPU_FSBC_SOCCAP_ON==1)
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
	
	Value8 = MmioRead8(Value64);
//	DEBUG((EFI_D_ERROR,"Bf_RPEGDBG_PEXC=%x\n",Value8));
	if(gSetupData->CPU_FSBC_TOPCIE==0)  //select PE0(D0F3,Haps) as the output port
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
    
	MmioWrite8(Value64,Value8);
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
	UINTN 		FSBCBaseAddress,
	UINT8		FsbcBufferSize,
	UINT8		CycleType,
	FSBC_TRIGGER_CONDITION	*FsbcTrigger,
	UINT8		RootBusNum
)
{
   UINT64 MsrValue;
   UINT16 Pmio = 0x800;
   UINT64 Address; 
   UINT32  Value32;
   
    if((AsmReadMsr64(0x16a7)&0x01)==0){
//	    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7, AsmReadMsr64(0x16a7)));
  		AsmWriteMsr64(0x16a7, (AsmReadMsr64(0x16a7)|0x01));
    }
	
	//Dump to PCIE
	Address = PCI_DEV_MMBASE(RootBusNum, 17, 0)+D17F0_PMU_PM_IO_BASE;
	Pmio = MmioRead16(Address)&0xff00;
//	DEBUG((EFI_D_ERROR,"Addr:%x, PMIO BAR=%x\n",Address,Pmio));

   	if(gSetupData->CPU_FSBC_MISSPACKE_EN)
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

	if(gSetupData->CPU_FSBC_TIGPULSE_EN)
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
	
	if(gSetupData->CPU_FSBC_IFSBCSTP_EN)
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


	if(gSetupData->CPU_FSBC_SOCCAP_ON==1)
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

   if(FsbcTrigger->IsDumpToPCIE){
        InitPcie(RootBusNum);
   }		 
   
   MsrValue = AsmReadMsr64(0x160B);
   if(!gSetupData->CPU_FSBC_STREAM_EN){
		// snap shot mode(no selective trigger mode) or not,Dump to Dram
		
		if(FsbcTrigger->FsbcTriggerPosition!=EnumFsbcTriggerPositionSnapShotMode){
			if(FsbcTrigger->IsNeedConfigTriggerCondition){
				MsrValue |=(UINT64)FsbcTrigger->FsbcTriggerPosition<<44;
				MsrValue |=(UINT64)FsbcTrigger->TriggerType<<49;
				MsrValue |=(UINT64)FsbcTrigger->IsWriteTriggerTransaction<<60;
				MsrValue |=(UINT64)FsbcTrigger->TriggerTransaction<<61;
			}else if(FsbcTrigger->IsDumpToPCIE){
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
   if(!gSetupData->CPU_FSBC_STREAM_EN){
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
}

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

VOID
EnableFsbc(	
	FSBC_CONFIG_PARA		*FsbcConfig
)
{
	EFI_STATUS				  Status;
	BOOLEAN                   Finished;
	UINTN                     ProcessorIndex=0;
	UINT32					  Index = 1;
	
	//BSP config - master 
	if(FsbcConfig->IsMasterEn){
		IoWrite8(0x80,0x81);
		TestMmioConf(0);
		fsbc_init(FsbcConfig);
	}
	
	if(!FsbcConfig->IsSlaveEn) return;
	
	//AP config
	IoWrite8(0x80,0x82);
	TestMmioConf(0x80);
	
	for(Index=1;Index<NumberOfProcessors;Index++){
		ProcessorIndex = Index;
		Status = ptMpSvr->StartupThisAP (
	                        ptMpSvr, 
	                        fsbc_init, 
	                        ProcessorIndex, 
	                        NULL, 
	                        0, 
	                        FsbcConfig,
	                        &Finished
	                        );
		if(EFI_ERROR(Status)){
			DEBUG((EFI_D_ERROR,"Process:%x error\n",ProcessorIndex));
			IoWrite8(0x80,0xDE);
			IoWrite8(0x80,0xAD);
			IoWrite8(0x80,(UINT8)ProcessorIndex);
			IoWrite8(0x80,0xE6);
			CpuDeadLoop();
		}
	}

	return;
}

typedef struct _CPU_CONTEXT{
  UINTN Processor;
  EFI_PHYSICAL_ADDRESS Address;
}CPU_CONTEXT;

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
	 IoWrite8(0x80,0xAD);
	 IoWrite8(0x80,0xAD);
	 IoWrite8(0x80,(UINT8)(Address64));
	 IoWrite8(0x80,(UINT8)(Address64>>8));
	 IoWrite8(0x80,(UINT8)(Address64>>16));
	 IoWrite8(0x80,(UINT8)(Address64>>24));
	 IoWrite8(0x80,0xAD);
	 IoWrite8(0x80,0xAD);

 	 AsmWriteMsr64(0x1621,Address64);
	 
	 SwitchBevoClk();
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
  
  //MSR 0x144b bit6
  Value64 = AsmReadMsr64(0x144b);
  Value64|=(UINT64)(1<<6);
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
		Address =(EFI_PHYSICAL_ADDRESS)(SlaveBase+(cpuid-0x08)*0x1000000);
	}else{
		Address =(EFI_PHYSICAL_ADDRESS)(MasterBase+cpuid*0x1000000);
	}
	
	IoWrite8(0x80,(UINT8)(Address>>24));
	IoWrite8(0x80,(UINT8)(Address>>32));
	
	//MSR 0x1301,BSP or AP??? dual socket???
	
	if(cpuid==0x0){
		AsmWriteMsr64(0x1301,  0x00000000d003ffde);
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
		AsmWriteMsr64(0x1307,0x0000000004000);//Tr70
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


VOID
EnableMPTracer(EFI_PHYSICAL_ADDRESS Address)
{
  EFI_STATUS				Status;
  BOOLEAN					Finished;
  UINTN 					ProcessorIndex;
  UINT32					Index = 1;
  CPU_CONTEXT				context;
 
  //Set 120f 1203 144a 144b for all core_S
  
  IoWrite8(0x80,0x61);
  SetAllMSRFunc_Before(NULL);
  
  for(Index=1;Index<NumberOfProcessors;Index++){
	ProcessorIndex = Index;
	Status = ptMpSvr->StartupThisAP (
							ptMpSvr, 
							SetAllMSRFunc_Before, 
							ProcessorIndex, 
							NULL, 
							0, 
							(VOID *)ProcessorIndex,
							&Finished
							);
	if(Status!=0){
			DEBUG((EFI_D_ERROR,"Process:%x error\n",ProcessorIndex));
			IoWrite8(0x80,0xDE);
			IoWrite8(0x80,0xAD);
			IoWrite8(0x80,(UINT8)ProcessorIndex);
			IoWrite8(0x80,0xE3);
			CpuDeadLoop();
	}
  }
  
  //Set 120f 1203 144a 144b for all core_E
  IoWrite8(0x80,0x62);
  for(Index=(UINT32)(NumberOfProcessors-1);Index>0;Index--){
	context.Processor = (UINTN)Index;
	context.Address = Address;
	ProcessorIndex = Index;
	Status = ptMpSvr->StartupThisAP (
							ptMpSvr, 
							SetMSR, 
							ProcessorIndex, 
							NULL, 
							0, 
							(VOID *)&context,
							&Finished
							);
	  if(Status!=0){
			DEBUG((EFI_D_ERROR,"Process:%x error\n",ProcessorIndex));
			IoWrite8(0x80,0xDE);
			IoWrite8(0x80,0xAD);
			IoWrite8(0x80,(UINT8)ProcessorIndex);
			IoWrite8(0x80,0xE4);
			CpuDeadLoop();
	}
  }
  
  context.Processor = 0;
  context.Address = Address;
  SetMSR((VOID*)&context);
  
  IoWrite8(0x80,0x63);
  SetAllMSRFunc_After(NULL);
  for(Index=1;Index<NumberOfProcessors;Index++){
  ProcessorIndex = Index;
  Status = ptMpSvr->StartupThisAP (
							ptMpSvr, 
							SetAllMSRFunc_After, 
							ProcessorIndex, 
							NULL, 
							0, 
							(VOID *)ProcessorIndex,
							&Finished
							);
	if(Status!=0){
			DEBUG((EFI_D_ERROR,"Process:%x error\n",ProcessorIndex));
			IoWrite8(0x80,0xDE);
			IoWrite8(0x80,0xAD);
			IoWrite8(0x80,(UINT8)ProcessorIndex);
			IoWrite8(0x80,0xE5);
			CpuDeadLoop();
	}
  }
}

EFI_STATUS
EFIAPI
EnableFsbcSnapShotMode(
	IN EFI_FSBC_DUMP_PROTOCOL  *This
)
{	
	return EFI_SUCCESS;
}

VOID
EFIAPI
CpuDebug()
{
	EFI_STATUS				SpcStatus;
	EFI_PHYSICAL_ADDRESS    MasterAddress,SlaveAddress;
	FSBC_CONFIG_PARA		FsbcConfig;
	FSBC_TRIGGER_CONDITION  FsbcTrigger;
	UINT64 					MsrValue;
	
	MasterAddress = SlaveAddress = 0;
	FsbcConfig.MasterFsbcBase = FsbcConfig.SlaveFsbcBase = 0;
	FsbcConfig.IsMasterEn	  = FsbcConfig.IsSlaveEn     = FALSE;
	FsbcConfig.FsbcSize = UC_SIZE_512MB;
	
	FsbcTrigger.FsbcTriggerPosition = EnumFsbcTriggerPositionSnapShotMode;
	
	SpcStatus = gBS->LocateProtocol(
					&gEfiMpServiceProtocolGuid,
					NULL,
					(VOID**)&ptMpSvr
					);
	
	if (EFI_ERROR (SpcStatus)){
		DEBUG((EFI_D_ERROR,"mike_LocateMpProtocol_error\n"));
		IoWrite8(0x80,0xDE);
		IoWrite8(0x80,0xAD);
		IoWrite8(0x80,0xE1);
		CpuDeadLoop();
	}
	
	SpcStatus = ptMpSvr->GetNumberOfProcessors(ptMpSvr, &NumberOfProcessors, &NumberOfEnabledProcessors);
	if (EFI_ERROR (SpcStatus)) {
		DEBUG((EFI_D_ERROR,"mike_GetNumberOfProcessors_error\n"));	
		IoWrite8(0x80,0xDE);
		IoWrite8(0x80,0xAD);
		IoWrite8(0x80,0xE2);
		CpuDeadLoop();
	}
	
	DEBUG((EFI_D_ERROR,"mike_NumberOfProcessors:%x\n",NumberOfProcessors));
	
	// Core Counter
	IoWrite8(0x80,0xaa);
	IoWrite8(0x80,(UINT8)NumberOfProcessors);
	
	FsbcConfig.IsMasterEn	  = gSetupData->CPU_MASTER_FSBC_EN;
//	FsbcConfig.IsSlaveEn	  = gSetupData->CPU_SLAVE_FSBC_EN;	
    FsbcConfig.IsSlaveEn	  = FALSE;
	
	//master socket
	if(FsbcConfig.IsMasterEn){
		IoWrite8(0x80,0xab);
		//MasterAddress = gSetupData->CPU_TRACER_DUMP_MEMORY_BASE*SIZE_256MB;
		MasterAddress = 0x40000000;
		MasterBase=AllocateReservedAndUcMemory(MasterAddress,UC_SIZE_512MB);	
		FsbcConfig.MasterFsbcBase = MasterBase;
	}
	
	IoWrite8(0x80,(AsmReadMsr64(0x1610)&0x03));
	DEBUG((EFI_D_ERROR,"Dual Socket or not:%x\n",(AsmReadMsr64(0x1610)&0x03)));
	
	//dual socket or not: MSR 0x1610 bit1/2
	if((AsmReadMsr64(0x1610)&0x03)==0x03){		
		//slave socket
		if(FsbcConfig.IsSlaveEn){
			IoWrite8(0x80,0xac);
			SlaveAddress = (((UINT64)(MmioRead32(HIF_PCI_REG(0xA4)))>>12)<<20)- 0x40000000;
			SlaveBase=AllocateReservedAndUcMemory(SlaveAddress,UC_SIZE_512MB);	
			FsbcConfig.SlaveFsbcBase  = SlaveBase;
		}
	}
	
	if(gSetupData->CPU_TRACER_EN){
		IoWrite8(0x80,0xae);
		EnableMPTracer(MasterAddress);
		
		FsbcConfig.MasterFsbcBase = MasterBase+SIZE_256MB;
		FsbcConfig.SlaveFsbcBase  = SlaveBase+SIZE_256MB;
		FsbcConfig.FsbcSize = UC_SIZE_256MB;
	}
	
	if(!(FsbcConfig.IsMasterEn||FsbcConfig.IsSlaveEn)) return;
	
   // 20170921-hxz add for chx002: msr 0x16a7 bit1-enable fsbc
    
    if((AsmReadMsr64(0x16a7)&0x01)==0){
	    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7, AsmReadMsr64(0x16a7)));
  		AsmWriteMsr64(0x16a7, (AsmReadMsr64(0x16a7)|0x01));
    }
    
    MsrValue = AsmReadMsr64(0x16a7);
    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7,MsrValue));
    IoWrite8(0x80,(UINT8)(MsrValue));
       	
	FsbcTrigger.IsDumpToPCIE = gSetupData->CPU_FSBC_PCIE_ON||gSetupData->CPU_FSBC_SOCCAP_ON;
	FsbcTrigger.FsbcTriggerPosition =  EnumFsbcTriggerPosition50;  // for mem dump 
	FsbcTrigger.IsWriteTriggerTransaction = FALSE;
	FsbcTrigger.TriggerTransaction		  = 0;
	FsbcTrigger.TriggerType 			  = FSBC_TRIGGER_TYPE_TRANSACTION;
	FsbcTrigger.IsNeedConfigTriggerCondition = FALSE;

	FsbcConfig.FsbcTriggerCondition = &FsbcTrigger;

	IoWrite8(0x80,0xaf);
	EnableFsbc(&FsbcConfig);
	DumpFsbcMsr1();
	IoWrite8(0x80,0xaa);
	
	return;
}

#endif
