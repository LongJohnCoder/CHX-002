#include "CpuMpPeiConfig.h"
#ifdef ZX_SECRET_CODE

VOID
SetMSRCore (
  IN  VOID  *Buffer
  )
{
    UINT64 MsrVal;
	UINT64 Value64;
	UINT32 RegEbx;
	MSR_Format* Paddress = (MSR_Format*)Buffer;
    AsmCpuid(0x1,NULL,&RegEbx,NULL,NULL);
	//OperateType:1: | OrMask   
	if(Paddress->OperateType){
     MsrVal = AsmReadMsr64(Paddress->Address);
	 MsrVal =MsrVal&(~(Paddress->AndMask));
	 MsrVal =MsrVal|(Paddress->OrMask);
     DEBUG((EFI_D_ERROR,"Core:%02x	msr:%04x	",RegEbx>>24,Paddress->Address));
     DEBUG((EFI_D_ERROR,"AndMask:%08lx	OrMask:%08lx	Value:%08lx\n",Paddress->AndMask,Paddress->OrMask,MsrVal));
	 AsmWriteMsr64(Paddress->Address,MsrVal);
	}else{
	 //OperateType: 0: +/- OrMask    
	 Value64 = AsmReadMsr64(Paddress->Address);
	 MsrVal = Value64&(Paddress->AndMask);
	 if(MsrVal==0x0){
	 	DEBUG((EFI_D_ERROR,"Core:%02x	msr:%04x Value:%08lx(Can't +/-)\n",RegEbx>>24,Paddress->Address,Value64));
	 	return;
	 }
	 //AddOrSub: 0: + OrMask
	 if(!Paddress->AddOrSub){
	 	MsrVal = (MsrVal+Paddress->OrMask)&(Paddress->AndMask);
	 }else{
	   //AddOrSub:1: - OrMask
	   MsrVal = (MsrVal-Paddress->OrMask)&(Paddress->AndMask);
	 }
	 MsrVal=MsrVal|(Value64&(~Paddress->AndMask));
     AsmWriteMsr64(Paddress->Address,MsrVal);
	 DEBUG((EFI_D_ERROR,"Core:%02x	msr:%04x	",RegEbx>>24,Paddress->Address));
     DEBUG((EFI_D_ERROR,"AndMask:%08lx	%s:%08lx	Value:%08lx\n",Paddress->AndMask,Paddress->AddOrSub?"-":"+",Paddress->OrMask,MsrVal));
	}
}

EFI_STATUS
EFIAPI
SetMSR(
 IN EFI_PEI_SERVICES  **PeiServices,
 IN UINT32 Address,
 IN UINT32 Length
)
{ 
   EFI_STATUS Status;
   EFI_PEI_MP_SERVICES_PPI    *MpSvr;
   MSR_Format* Paddress = (MSR_Format*)Address;
   UINTN Index = 0;
   UINTN NumberOfProcessors;
   UINTN NumberOfEnabledProcessors;
   UINT8 Core[32];
   UINT32 SetNumofCore=0;
   UINT32 CoreIndex;

   if(Paddress->Address ==0xFFFFFFFF)
   	return EFI_SUCCESS;
   //Clear Core Arrary
   for(Index=0;Index<32;Index++){
   	  Core[Index] = 0;
   	}
   //Locate PeiMpServicesPpi
   Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID**)&MpSvr
             );
   ASSERT_EFI_ERROR(Status);
   //Get Processor Number
   Status = MpSvr->GetNumberOfProcessors(
   	                                PeiServices,
   	                                MpSvr,
   	                                &NumberOfProcessors,
   	                                &NumberOfEnabledProcessors
   	                                );
   ASSERT_EFI_ERROR(Status);
   //0xFFFFFFFF Or 0x00000000 is the End Signal
   while(Paddress->Address!=0xFFFFFFFF){
   	//Find which Core need Set MSR
   	SetNumofCore = 0;
   	for(Index=0;Index<NumberOfProcessors;Index++){
		if((Paddress->CoreMask>>Index)&0x1){
			Core[SetNumofCore] = (UINT8)Index;
		    SetNumofCore++;
			
		}
   	}
	//Max Core Can Set is System Max Core
	if(SetNumofCore>=NumberOfProcessors){
		SetNumofCore = NumberOfProcessors;
	}
   	for(Index=0;Index<SetNumofCore;Index++){
		 //Set MSR Order
		 if(Paddress->SetOrder&0x1){
		 	CoreIndex = Core[Index];
		 }
		 //Set MSR ReversOrder
		 else{
		 	CoreIndex = Core[SetNumofCore-Index-1];
		 }
		 //BSP
		 if(CoreIndex==0){
		 	SetMSRCore((VOID*)Paddress);
		 }
		 //APs
		 else{
		    Status = MpSvr->StartupThisAP (
                            PeiServices, 
                            MpSvr,
                            SetMSRCore, 
                            (UINTN)CoreIndex, 
                            0,
                            (VOID *)Paddress
                            );
			ASSERT_EFI_ERROR(Status);
		 }
   	}
	//MSR Can't Above the Max Area Range
	if(Paddress>=(MSR_Format*)(Address+Length-sizeof(MSR_Format))){
		  break;
		}
    Paddress++;
	DEBUG((EFI_D_ERROR,"\n"));
   	}
   return EFI_SUCCESS;
}
EFI_STATUS 
EFIAPI 
MsrConfig(
IN EFI_PEI_SERVICES  **PeiServices,
IN MSR_PHASE    phase
)
{  
   switch(phase){
	case PEI0:
		DEBUG((EFI_D_ERROR,"================Set MSR at PEI0 Phase=================\n"));
		SetMSR(PeiServices,PcdGet32(PcdMSRPEI0Base),PcdGet32(PcdMSRPEI0Size));
		DEBUG((EFI_D_ERROR,"======================================================\n"));
        break;
	case PEI1:
		DEBUG((EFI_D_ERROR,"================Set MSR at PEI1 Phase=================\n"));
		SetMSR(PeiServices,PcdGet32(PcdMSRPEI1Base),PcdGet32(PcdMSRPEI1Size));
		DEBUG((EFI_D_ERROR,"=======================================================\n"));
		break;
	default:
		DEBUG((EFI_D_ERROR,"Set MSR at undefined Phase!!!\n"));
		DEBUG((EFI_D_ERROR,"Please input the right Phase Parameter!!!\n"));
		return EFI_INVALID_PARAMETER;
   }
   return EFI_SUCCESS;
}

EFI_CPU_CONFIG_PPI_PROTOCOL   gPeiCpuConfigPpi =
{
    MsrConfig
};

static EFI_PEI_PPI_DESCRIPTOR       gCpuConfigPpiDesc=
{
    (EFI_PEI_PPI_DESCRIPTOR_PPI |EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gCpuConfigPpiGuid,
    &gPeiCpuConfigPpi
};

EFI_STATUS
EFIAPI
CpuMpConfigPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS           Status;
  Status = PeiServicesInstallPpi(&gCpuConfigPpiDesc);
  ASSERT_EFI_ERROR (Status);
  return Status;
}
#endif


