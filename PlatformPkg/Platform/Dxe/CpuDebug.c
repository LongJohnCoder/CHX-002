#ifdef ZX_SECRET_CODE   

#include "PlatformDxe.h"
#include <Library/CpuDebugLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>

EFI_MP_SERVICES_PROTOCOL  *ptMpSvr;
UINTN					  NumberOfProcessors;
UINTN					  NumberOfEnabledProcessors;  

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
		fsbc_init(FsbcConfig);
	}
	
	if(!FsbcConfig->IsSlaveEn) return;
	
	//AP config
	IoWrite8(0x80,0x82);
	
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

VOID
EnableMPTracer(
	IN UINT32		     NumOfInstPer2Dump,
	IN EFI_PHYSICAL_ADDRESS        MasterBase,
	IN EFI_PHYSICAL_ADDRESS        SlaveBase
)
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
  context.MasterAddress = MasterBase;
  context.SlaveAddress = SlaveBase;
  context.NumOfInstPer2Dump = NumOfInstPer2Dump;
  for(Index=(UINT32)(NumberOfProcessors-1);Index>0;Index--){
	context.Processor = (UINTN)Index;

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
  	 DumpTracerMsr1();
}

EFI_STATUS
EFIAPI
EnableFsbcSnapShotMode(
	IN EFI_FSBC_DUMP_PROTOCOL  *This
)
{	
	return EFI_SUCCESS;
}

EFI_FSBC_DUMP_PROTOCOL mFsbcDump={
	EnableFsbcSnapShotMode
};

VOID
EFIAPI
CpuDebug()
{
	EFI_STATUS				SpcStatus;
	EFI_PHYSICAL_ADDRESS    MasterAddress,SlaveAddress;
	FSBC_CONFIG_PARA		FsbcConfig;
	FSBC_TRIGGER_CONDITION  FsbcTrigger;
	UINT64 					MsrValue;
	UINT32					NumOfInstPer2Dump;
	
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
	IoWrite8(0x80,0xab);
//	MasterAddress = 0x40000000;
    MasterAddress = gSetupData->CPU_TRACER_DUMP_MEMORY_BASE;
	FsbcConfig.MasterFsbcBase=AllocateReservedAndUcMemory(MasterAddress,UC_SIZE_512MB);	
	
	IoWrite8(0x80,(AsmReadMsr64(0x1610)&0x03));
	DEBUG((EFI_D_ERROR,"Dual Socket or not:%x\n",(AsmReadMsr64(0x1610)&0x03)));
	
	//dual socket or not: MSR 0x1610 bit1/2
	if((AsmReadMsr64(0x1610)&0x03)==0x03){		
		//slave socket
		IoWrite8(0x80,0xac);
		SlaveAddress = (((UINT64)(MmioRead32(HIF_PCI_REG(0xA4)))>>12)<<20)- 0x40000000;
		FsbcConfig.SlaveFsbcBase=AllocateReservedAndUcMemory(SlaveAddress,UC_SIZE_512MB);	
	}
	
	if(gSetupData->CPU_TRACER_EN){
		IoWrite8(0x80,0xae);
		NumOfInstPer2Dump = (UINT32)gSetupData->CPU_TRACER_INSTRUCTION_INTERVAL;
		EnableMPTracer(NumOfInstPer2Dump,MasterAddress,SlaveAddress);
		
		FsbcConfig.MasterFsbcBase +=SIZE_256MB;
		FsbcConfig.SlaveFsbcBase  +=SIZE_256MB;
		FsbcConfig.FsbcSize = UC_SIZE_256MB;
	}
	   //debug tracer
	{
		UINT32 Index;

		for(Index=0;Index<100;Index++){
		DEBUG((EFI_D_ERROR,"%d Read MSR 0x1307:%llx\n",Index,AsmReadMsr64(0x1307)));
		}
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
       	
	FsbcTrigger.IsDumpToPCIE = gSetupData->CPU_FSBC_PCIE_ON;
	FsbcTrigger.PciePortNum	 = gSetupData->CPU_FSBC_TOPCIE;
	FsbcTrigger.IsSoccapEn   = gSetupData->CPU_FSBC_SOCCAP_ON;
	FsbcTrigger.IsStreamModeEn = gSetupData->CPU_FSBC_STREAM_EN;
	FsbcTrigger.FsbcTriggerPosition =  EnumFsbcTriggerPosition50;  // for mem dump 
	FsbcTrigger.IsWriteTriggerTransaction = FALSE;
	FsbcTrigger.TriggerTransaction		  = 0;
	FsbcTrigger.TriggerType 			  = FSBC_TRIGGER_TYPE_TRANSACTION;
	FsbcTrigger.IsNeedConfigTriggerCondition = FALSE;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_MISSPACKE_EN = gSetupData->CPU_FSBC_MISSPACKE_EN;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_IFSBCSTP_EN = gSetupData->CPU_FSBC_IFSBCSTP_EN;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_TIGPULSE_EN = gSetupData->CPU_FSBC_TIGPULSE_EN;
	FsbcConfig.FsbcTriggerCondition = &FsbcTrigger;

	IoWrite8(0x80,0xaf);
	EnableFsbc(&FsbcConfig);
	DumpFsbcMsr1();
	IoWrite8(0x80,0xaa);
	
	return;
}

#endif
