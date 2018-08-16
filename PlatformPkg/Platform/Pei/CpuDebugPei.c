#ifdef ZX_SECRET_CODE  

#include "PlatformPei.h"

#include <PiPei.h>
#include <Ppi/MpServices.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/CpuDebugLib.h>
#include <SetupVariable.h>

//for pei code£¬can not use global Var

VOID
EFIAPI
EnableFsbc(	
	IN EFI_PEI_MP_SERVICES_PPI	  *ptMpSvr,
	IN UINTN					   NumberOfProcessors,
	IN FSBC_CONFIG_PARA			   *pFsbcConfig
)
{
	EFI_STATUS				  Status;
	UINTN                     ProcessorIndex=0;
	UINT32					  Index = 1;
	
	Status = EFI_SUCCESS;
	
	//BSP config - master 
	if(pFsbcConfig->IsMasterEn){
		IoWrite8(0x80,0x81);
		fsbc_init(pFsbcConfig);
	}
	
	if(!pFsbcConfig->IsSlaveEn) return;
	
	//AP config
	IoWrite8(0x80,0x82);
	
	for(Index=1;Index<NumberOfProcessors;Index++){
		ProcessorIndex = Index;

		 Status = ptMpSvr->StartupThisAP (
 						   (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                            ptMpSvr, 
                            fsbc_init, 
                            ProcessorIndex, 
                            0, 
                            (VOID*)pFsbcConfig

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
	IN EFI_PEI_MP_SERVICES_PPI	  *ptMpSvr,
	IN UINTN					   NumberOfProcessors,
	IN EFI_PHYSICAL_ADDRESS        MasterBase,
	IN EFI_PHYSICAL_ADDRESS        SlaveBase,
	IN UINT32					   NumOfInstPer2Dump
	)
{
  EFI_STATUS				 Status;
  UINTN                     ProcessorIndex;
  UINT32                    Index = 1;
  CPU_CONTEXT               context;

  //Set 120f 1203 144a 144b for all core_S
  
  IoWrite8(0x80,0x61);
  SetAllMSRFunc_Before(NULL);
  
  for(Index=1;Index<NumberOfProcessors;Index++){
  ProcessorIndex = Index;
  Status = ptMpSvr->StartupThisAP (
 						   (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                            ptMpSvr, 
                            SetAllMSRFunc_Before, 
                            ProcessorIndex, 
                            0, 
                            (VOID *)ProcessorIndex
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
							(EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                            ptMpSvr, 
                            SetMSR, 
                            ProcessorIndex, 
                            0, 
                            (VOID *)&context
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
  						    (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                            ptMpSvr, 
                            SetAllMSRFunc_After, 
                            ProcessorIndex, 
                            0, 
                            (VOID *)ProcessorIndex
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
 CpuDebugPei(	 
	 IN EFI_PEI_SERVICES		   **PeiServices,
	 IN EFI_PEI_MP_SERVICES_PPI    *MpSvr,
	 IN SETUP_DATA					*SetupHob
)
{
	EFI_STATUS				SpcStatus;
	UINTN    				MasterAddress,SlaveAddress;
	FSBC_CONFIG_PARA		FsbcConfig;
	FSBC_TRIGGER_CONDITION  FsbcTrigger;
	UINT64 					MsrValue;
	UINTN					  NumberOfProcessors;
    UINTN					  NumberOfEnabledProcessors;  
	UINT32					   NumOfInstPer2Dump;
	
	DEBUG((EFI_D_ERROR,"CpuDebugPei enter\n"));	
	DEBUG((EFI_D_ERROR,"PeiSer:%x,%x;MpSer:%x,%x\n",PeiServices,*PeiServices,MpSvr,*MpSvr));
	
	MasterAddress = SlaveAddress = 0;
	FsbcConfig.MasterFsbcBase = FsbcConfig.SlaveFsbcBase = 0;
	FsbcConfig.IsMasterEn	  = FsbcConfig.IsSlaveEn     = FALSE;
	FsbcConfig.FsbcSize = UC_SIZE_512MB;
	
	SpcStatus = MpSvr->GetNumberOfProcessors(
				(EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
  				 MpSvr,
  				 &NumberOfProcessors, 
  				 &NumberOfEnabledProcessors
  				 );
	
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
	
	FsbcConfig.IsMasterEn	  = SetupHob->CPU_MASTER_FSBC_EN;
//	FsbcConfig.IsMasterEn	  = TRUE;
//	FsbcConfig.IsSlaveEn	  = SetupHob->CPU_SLAVE_FSBC_EN;	
    FsbcConfig.IsSlaveEn	  = FALSE;
	
	//master socket
	IoWrite8(0x80,0xab);
	//MasterAddress = SetupHob->CPU_TRACER_DUMP_MEMORY_BASE*SIZE_256MB;
	MasterAddress = 0x40000000;
	
//		MasterBase=AllocateReservedAndUcMemory(MasterAddress,UC_SIZE_512MB);	
	FsbcConfig.MasterFsbcBase = (UINTN)MasterAddress;
	
	IoWrite8(0x80,(AsmReadMsr64(0x1610)&0x03));
	DEBUG((EFI_D_ERROR,"Dual Socket or not:%x\n",(AsmReadMsr64(0x1610)&0x03)));
	
	//dual socket or not: MSR 0x1610 bit1/2
	if((AsmReadMsr64(0x1610)&0x03)==0x03){		
		//slave socket
		IoWrite8(0x80,0xac);
		SlaveAddress = (((UINT64)(MmioRead32(HIF_PCI_REG(0xA4)))>>12)<<20);
		SlaveAddress-=0x40000000;
//			SlaveBase=AllocateReservedAndUcMemory(SlaveAddress,UC_SIZE_512MB);
 //       SlaveAddress = 0x400000000;	
		FsbcConfig.SlaveFsbcBase  = SlaveAddress;
	}
	
	if(SetupHob->CPU_TRACER_EN){
		IoWrite8(0x80,0xae);
#ifdef CHX002_PXP
    NumOfInstPer2Dump = 0x00000000004000;
#else
    NumOfInstPer2Dump = (UINT32)SetupHob->CPU_TRACER_INSTRUCTION_INTERVAL;
#endif
		EnableMPTracer(MpSvr,NumberOfProcessors,MasterAddress,SlaveAddress,NumOfInstPer2Dump);
		
		FsbcConfig.MasterFsbcBase = (UINTN)(MasterAddress+SIZE_256MB);
		FsbcConfig.SlaveFsbcBase  = (UINTN)(SlaveAddress+SIZE_256MB);
		FsbcConfig.FsbcSize = UC_SIZE_256MB;
	}
	
	if(!(FsbcConfig.IsMasterEn||FsbcConfig.IsSlaveEn)) return EFI_SUCCESS;
	
   // 20170921-hxz add for chx002: msr 0x16a7 bit1-enable fsbc
    
    if((AsmReadMsr64(0x16a7)&0x01)==0){
	    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7, AsmReadMsr64(0x16a7)));
  		AsmWriteMsr64(0x16a7, (AsmReadMsr64(0x16a7)|0x01));
    }
    
    MsrValue = AsmReadMsr64(0x16a7);
    DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x16a7,MsrValue));
    IoWrite8(0x80,(UINT8)(MsrValue));
       	
	FsbcTrigger.IsDumpToPCIE = SetupHob->CPU_FSBC_PCIE_ON;
	FsbcTrigger.PciePortNum	 = SetupHob->CPU_FSBC_TOPCIE;
	FsbcTrigger.IsSoccapEn   = SetupHob->CPU_FSBC_SOCCAP_ON;
	FsbcTrigger.IsStreamModeEn = SetupHob->CPU_FSBC_STREAM_EN;
	FsbcTrigger.FsbcTriggerPosition =  EnumFsbcTriggerPosition50;  // for mem dump 
	FsbcTrigger.IsWriteTriggerTransaction = FALSE;
	FsbcTrigger.TriggerTransaction		  = 0;
	FsbcTrigger.TriggerType 			  = FSBC_TRIGGER_TYPE_TRANSACTION;
	FsbcTrigger.IsNeedConfigTriggerCondition = FALSE;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_MISSPACKE_EN = SetupHob->CPU_FSBC_MISSPACKE_EN;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_IFSBCSTP_EN = SetupHob->CPU_FSBC_IFSBCSTP_EN;
	FsbcTrigger.OtherTriggerMethod.CPU_FSBC_TIGPULSE_EN = SetupHob->CPU_FSBC_TIGPULSE_EN;
	FsbcConfig.FsbcTriggerCondition = &FsbcTrigger;

	IoWrite8(0x80,0xaf);
	EnableFsbc(MpSvr,NumberOfProcessors,&FsbcConfig);
	
	//DumpTracerMsr1();
	//DumpFsbcMsr1();

		   //debug tracer
	{
		UINT32 Index;

		for(Index=0;Index<100;Index++){
		DEBUG((EFI_D_ERROR,"%d Read MSR 0x1307:%llx\n",Index,AsmReadMsr64(0x1307)));
		}
	}
		   
	IoWrite8(0x80,0xaa);
	
	return EFI_SUCCESS;
}

#endif
