#include "CpuMpDxeConfig.h"
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
 IN UINT32 Address,
 IN UINT32 Length
)
{ 
   EFI_STATUS Status;
   EFI_MP_SERVICES_PROTOCOL  *MpService;
   MSR_Format* Paddress = (MSR_Format*)(UINT64)Address;
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
   Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL,(VOID**)&MpService);
   ASSERT_EFI_ERROR(Status);
   //Get Processor Number
   Status = MpService->GetNumberOfProcessors(
   	                                MpService,
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
		SetNumofCore = (UINT32)NumberOfProcessors;
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
		 	 Status = MpService->StartupThisAP (
                            MpService, 
                            SetMSRCore, 
                            (UINTN)CoreIndex, 
                            NULL, 
                            0, 
                            (VOID *)Paddress,
                            NULL
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
IN MSR_PHASE    Phase
)
{  
   switch(Phase){
	case DXE:
		DEBUG((EFI_D_ERROR,"================Set MSR at DXE Phase===================\n"));
		SetMSR(PcdGet32(PcdMSRDXEBase),PcdGet32(PcdMSRDXESize));
		DEBUG((EFI_D_ERROR,"=======================================================\n"));
		break;
	case RDB:
		DEBUG((EFI_D_ERROR,"================Set MSR at RDB Phase===================\n"));
		SetMSR(PcdGet32(PcdMSRRDBBase),PcdGet32(PcdMSRRDBSize));
		DEBUG((EFI_D_ERROR,"=======================================================\n"));
		break;
	default: 
		DEBUG((EFI_D_ERROR,"Set MSR at undefined Phase!!!\n"));
		DEBUG((EFI_D_ERROR,"Please input the right Phase Parameter!!!\n"));
		return EFI_INVALID_PARAMETER;
   }
   return EFI_SUCCESS;
}
EFI_MP_CONFIG_PROTOCOL     mMpConfig = {
   MsrConfig
};

EFI_STATUS
EFIAPI
CpuMpConfigDxeInit (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS           Status;
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEfiCpuMpConfigProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpConfig
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
#endif

