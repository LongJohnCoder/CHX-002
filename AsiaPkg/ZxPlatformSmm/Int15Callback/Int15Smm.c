//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************


#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>

     

#define CHIPSET_INT15_SW_SMI  0x5F
EFI_SMM_CPU_PROTOCOL                *mSmmCpu;
EFI_SMM_VARIABLE_PROTOCOL           *mSmmVariable;

VOID BootDeviceSet(
	IN UINT8	DP1,
	IN UINT8	DP2,
	IN UINT8	DVO,
	IN UINT8	CRT,
	OUT UINT32	*SmmEbx,
	OUT UINT32	*SmmEcx
)
{
	//UINT8 DisDev, i = 0;
	UINT32 tmpBootUpDev = (UINT32) 0;
	UINT32 tmpDetailedBootUpDev = (UINT32) 0;
	

	if(DP1)
		tmpBootUpDev |= 0X8000;
	if(DP2)
		tmpBootUpDev |= 0X0200;
	if(DVO)
		tmpBootUpDev |= 0X0020;
	if(CRT)
			tmpBootUpDev |= 0x0001;
	
	tmpDetailedBootUpDev = (DP2<<16)|(DP1<<8)|DVO;
	

	*SmmEbx = tmpDetailedBootUpDev;
	*SmmEcx = tmpBootUpDev;
}

EFI_STATUS
EFIAPI
ChipsetInt15SmmDispatcher (
  IN EFI_HANDLE                        DispatchHandle,
  IN CONST EFI_SMM_SW_REGISTER_CONTEXT *Context,
  IN OUT EFI_SMM_SW_CONTEXT            *SwContext,
  IN OUT UINTN                         *CommBufferSize
  )
{
    EFI_STATUS             Status;
	UINTN                  CpuIndex = SwContext->SwSmiCpuIndex;
	UINTN                  SetupDataSize;
    UINT32				   SmmEax, SmmEbx, SmmEcx, SmmEdx, SmmEsi, SmmEdi;
    UINT32                 TempIndex, TempData, TempData2;
    SETUP_DATA   SetupData;

    Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RBP,
                      CpuIndex,
                      &SmmEax
                      );
//	ASSERT_EFI_ERROR (Status);	//WMC changed to avoid assert error 20150807
	if(EFI_ERROR(Status))
		return Status;


    SmmEdx = SmmEax >> 16;
	SmmEax = SmmEax & 0xFFFF;
	DEBUG((EFI_D_ERROR,"Enter INT15 Callback, fun=%x\n", SmmEax&0xff));

	Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RBX,
                      CpuIndex,
                      &SmmEbx
                      );
	Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RCX,
                      CpuIndex,
                      &SmmEcx
                      );
	Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RSI,
                      CpuIndex,
                      &SmmEsi
                      );
	Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RDI,
                      CpuIndex,
                      &SmmEdi
                      );
  SetupDataSize = sizeof(SETUP_DATA);  

  Status = mSmmVariable->SmmGetVariable (
                           PLATFORM_SETUP_VARIABLE_NAME,
                           &gPlatformSetupVariableGuid,
                           NULL,
                           &SetupDataSize,
                           &SetupData
                           );
    ASSERT_EFI_ERROR (Status);


    if(((SmmEax >> 8) & 0xff) == CHIPSET_INT15_SW_SMI){
      switch(SmmEax & 0xff) {
	        case 0x1:
				// Panel type is no longer in use in CHX002.
				SmmEax = CHIPSET_INT15_SW_SMI;
		  	    break;

			case 0x8:
			    TempData = 0xFFFF & SmmEbx;

				TempIndex = 0;
				TempIndex = 0x1|0x2|0x4|0x20|0x40;
				SetupData.DP1 = 0;
				SetupData.DP2 = 0;
				SetupData.DVO = 0;
				SetupData.CRT = 0;
				
				if(TempData&0x1)
				{
					SetupData.CRT = 1;
				}
				if(TempData&0x20)
				{
					SetupData.DVO = (UINT8) TempIndex;
				}
				if(TempData&0x200)
				{
					SetupData.DP2 = (UINT8) TempIndex;
				}
				if(TempData&0x8000)
				{
					SetupData.DP1 = (UINT8) TempIndex;
				}

			    mSmmVariable->SmmSetVariable (
   					              PLATFORM_SETUP_VARIABLE_NAME, 
   					              &gPlatformSetupVariableGuid,
   					              EFI_VARIABLE_NON_VOLATILE |
   					              EFI_VARIABLE_BOOTSERVICE_ACCESS |
   					              EFI_VARIABLE_RUNTIME_ACCESS,
   					              SetupDataSize, 
   					              &SetupData );
                  
              SmmEax &= (~0xFFFF);
              SmmEax |= CHIPSET_INT15_SW_SMI;
              break;

			case 0x18:
				//Query memory data rate and get frame buffer size from CMOS setting
				TempData=0;					 
				//TODO RKD remove for pxp code parpare
				TempData = MmioRead16(DRAM_PCI_REG(D0F3_PLLINDDR_CTL_Z1));
				TempData = TempData & D0F3_RDIV_M_PLLINDDR_7_0;
				TempData >>= 8;
				
				if (TempData < 6)//800
					TempData = 0x09;
				else if (TempData < 8 )//1066
					TempData = 0x0A;
				else if (TempData < 10 )//1333
					TempData = 0x0B;
				else if (TempData < 12 )//1600
					TempData = 0x0C;
				else if (TempData < 14 )//1866
					TempData = 0x0D;
				else if (TempData < 16 )//2133
					TempData = 0x0E;
				// else if (TempData < 18)//2400
				// 	TempData =     ;
				// else if (TempData < 20)//2666
				// 	TempData =     ;
				// else if (TempData < 22)//2933
				// 	TempData =     ;
				else					 //2400   //3200
					TempData = 0x0F;

				TempData <<= 4;	
				
				SmmEbx = (UINT8) TempData;


				//Get Frame Buffer Size
				TempData=0;
				TempData2=0;
				TempData = MmioRead32(DRAM_PCI_REG(D0F3_CPU_DIRECT_ACCESS_FRAME_BUFFER_CTL_Z1));

				if ((TempData & D0F3_RGFXEN) != D0F3_RGFXEN)
			    {
					TempData2 = 0;
			    }
			    else
			    {
			    	TempData2 = (TempData & D0F3_RFBSZ_2_0)>>4;
					if (TempData2 > 0x4)
						TempData2 = ((TempData2&0x3) + 2); //UMAsize = 2^FBSizeTmp (MB)
					else
						TempData2 = (TempData2 + 5); //UMAsize = 2^FBSizeTmp (MB)
			    }

				SmmEbx |= (UINT8) TempData2;
				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
				
				break;
			case 0x28:
				// Panel type 2 is no longer in use in CHX002.
				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
				break;
			case 0x2A:
				SmmEcx = (UINT16) 0;
				SmmEax = (UINT16) 0;

				//For DP1
				TempData = (UINT16) SetupData.DP1SSCEn;
				TempData &= 0x01;
				SmmEcx |= TempData;

				if(TempData == 1) //If DP1SSCEn Enable
				{
					TempData = (UINT16) SetupData.DP1SSCMode;
					TempData &= 0x03;
					SmmEcx |= TempData << 1;
				}

				//For DP2
				TempData = (UINT16) SetupData.DP2SSCEn;
				TempData &= 0x01;
				SmmEcx |= TempData << 8;

				if(TempData == 1) //If DP2SSCEn Enable
				{
					TempData = (UINT16) SetupData.DP2SSCMode;
					TempData &= 0x03;
					SmmEcx |= TempData << 9;
				}

				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
			    break;
			case 0x33:
				TempData = (UINT32) 0;
				TempData = 0xF & SmmEbx;
				SmmEbx = (UINT32) 0;
				SmmEbx = SmmEbx|(TempData << 28);
				SmmEbx = SmmEbx|(1 << 27);
				if((TempData == 0x2) && (SetupData.VCLKCtrl)) //VCLK
				{			
					TempData = (UINT16) SetupData.VCLKFreq;
					TempData = TempData & 0xFFF;
					SmmEbx = SmmEbx|TempData;
				}
				else if((TempData == 0x1) && (SetupData.ICLKCtrl))//ICLK
				{
					TempData = (UINT16) SetupData.ICLKFreq;
					TempData = TempData & 0xFFF;
					SmmEbx = SmmEbx|TempData;
				}
				else if((TempData == 0x0) && (SetupData.ECLKCtrl))
				{
					TempData = (UINT16) SetupData.ECLKFreq;
					TempData = TempData & 0xFFF;
					SmmEbx = SmmEbx|TempData;
				}
						
				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
				break;
			case 0x30:
				//If selectDisplaydevice is "Manual"
				if(SetupData.SelectDisplayDevice != 0)
				{
					BootDeviceSet(SetupData.DP1, SetupData.DP2, SetupData.DVO, SetupData.CRT, &SmmEbx, &SmmEcx);
				}else
				//If selectDisplaydevice is "AUTO"
				{
					SmmEcx = (UINT16) 0;
					SmmEbx = (UINT32) 0;
				}
				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
				break;
			case 0x32:
				SmmEbx = 0x0; 
				SmmEbx |=(UINT8)SetupData.PWM0OutputEn;
				TempData =  (UINT8)SetupData.PWM1OutputEn;
				TempData <<= 1;
				SmmEbx |=TempData;
				TempData =  (UINT8)SetupData.PWM0Frequency;
				TempData <<=2;
				SmmEbx |=TempData;
				TempData =  (UINT8)SetupData.PWM1Frequency;
				TempData <<=5;
				SmmEbx |=TempData;
				SmmEcx = (UINT32)SetupData.PWM0DutyCycle ;
				SmmEdx = (UINT32)SetupData.PWM1DutyCycle;
				SmmEax = (UINT16) CHIPSET_INT15_SW_SMI;
				break;
			default:
			    SmmEax &= (~0xFFFF);
                SmmEax |= ((0x86 << 8) + 0x86);			
      }
  }

	    Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RAX,
                      CpuIndex,
                      &SmmEax
                      );
	    Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RBX,
                      CpuIndex,
                      &SmmEbx
                      );
	    Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RCX,
                      CpuIndex,
                      &SmmEcx
                      );
	    Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RDX,
                      CpuIndex,
                      &SmmEdx
                      );

        Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RSI,
                      CpuIndex,
                      &SmmEsi
                      );

         Status = mSmmCpu->WriteSaveState (
                      mSmmCpu,
                      4,
                      EFI_SMM_SAVE_STATE_REGISTER_RDI,
                      CpuIndex,
                      &SmmEdi
                      );

	return Status;
}


EFI_STATUS
EFIAPI
InitializeChipsetInt15Smm (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL     *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT       SwContext;
  EFI_HANDLE                        SwHandle;
  EFI_BOOT_MODE                     BootMode;
  UINT32                           IfCallINT15;


  DEBUG((EFI_D_INFO, "(L%d) %a()\n", __LINE__, __FUNCTION__));

  //IVS-20181015 Judge whether to Call INT15
  IfCallINT15 = MmioRead32(SCRCH_PCI_REG(D0F6_BIOS_SCRATCH_REG_12))&BIT0;
  DEBUG((EFI_D_INFO, "[IVES] IfCallINT15 = 0x%x\n",IfCallINT15));

  BootMode = GetBootModeHob();
  if(BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
    return EFI_SUCCESS;
  }  
  
  //
  //  Get the Sw dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID**)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if(IfCallINT15)
  {
	  SwContext.SwSmiInputValue = CHIPSET_INT15_SW_SMI;
	  Status = SwDispatch->Register (SwDispatch, ChipsetInt15SmmDispatcher, &SwContext, &SwHandle);
	  DEBUG((EFI_D_ERROR, "(L%d) %a() %r\n", __LINE__, __FUNCTION__, Status));

	  ASSERT_EFI_ERROR (Status);
	  if (EFI_ERROR (Status)) {
	    return Status;
	  }
  }
   //
  // Locate SmmVariableProtocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID**)&mSmmVariable
                    );
  ASSERT_EFI_ERROR (Status);

  
  //
  // Get SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid, 
                    NULL, 
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status); 
  return EFI_SUCCESS;
}
