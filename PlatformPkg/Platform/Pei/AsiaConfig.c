//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2015 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************

#include "PlatformPei.h"
#include <Library/MultiPlatSupportLib.h>
#include <Library/PciLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>
#include <SetupVariable.h>
#include <RtcDef.h>

VOID SetAfterPowerLoss(UINT8 AfterPowerLoss)
{
  if(AfterPowerLoss == AFTER_POWER_LOSS_PREVIOUS || 
     AfterPowerLoss == AFTER_POWER_LOSS_ON){          // Last or On
    SetCmosVRT(FALSE);
  } else if(AfterPowerLoss == AFTER_POWER_LOSS_OFF){  // Off
    SetCmosVRT(TRUE);  
  }
}


VOID 
HandleRtcWake(
  IN EFI_BOOT_MODE          BootMode,
  IN SETUP_DATA             *SetupData
  )
{
  UINT8           Cmos0B;
  RTC_REGISTER_B  RegisterB;  
  EFI_STATUS      Status;

  if(BootMode == BOOT_ON_S3_RESUME || BootMode == BOOT_ON_S4_RESUME){		                    // Ignore Sleep Wake.
    goto ProcExit;
  }
  if(!(IoRead16(PMIO_REG(PMIO_STS_REG)) & IoRead16(PMIO_REG(PMIO_PM_EN)) & PMIO_STS_RTC)){  // Only Handle RTC wake.
    goto ProcExit;
  }
  if(SetupData->WakeOnRTC != RTC_WAKE_VAL_SINGLE_EVENT){                                    // Only Handle Single Event.
    goto ProcExit;
  }    

  Status = RtcWaitToUpdate();
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }   
  RegisterB.Data = CmosRead(RTC_ADDRESS_REGISTER_B);
  
  Cmos0B = RegisterB.Data;
  RegisterB.Bits.Set = 1;   // Updates inhibited
  RegisterB.Bits.Aie = 0;  
  RegisterB.Bits.Mil = 1;		// 24 hours
  RegisterB.Bits.Dm  = 0;		// BCD Format
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  if(CmosRead(RTC_ADDRESS_SECONDS_ALARM) == DecimalToBcd8(SetupData->RTCWakeupTime.Second) &&
     CmosRead(RTC_ADDRESS_MINUTES_ALARM) == DecimalToBcd8(SetupData->RTCWakeupTime.Minute) &&
     CmosRead(RTC_ADDRESS_HOURS_ALARM)   == DecimalToBcd8(SetupData->RTCWakeupTime.Hour)   &&
     CmosRead(RTC_ADDRESS_DATE_ALARM)    == DecimalToBcd8(SetupData->RTCWakeupDate.Day)    &&
     CmosRead(RTC_ADDRESS_MONTH_ALARM)   == DecimalToBcd8(SetupData->RTCWakeupDate.Month)  &&
     CmosRead(RTC_ADDRESS_YEAR)          <  DecimalToBcd8((UINT8)(SetupData->RTCWakeupDate.Year-2000))){
    
    DEBUG((EFI_D_ERROR, "RTC Relay\n"));
    
    CmosRead(RTC_ADDRESS_REGISTER_C);  // Read 0xC to clear pending RTC interrupts  	
    RegisterB.Bits.Aie = 1;
    RegisterB.Bits.Set = 0;
    CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
    
    IoWrite16(PMIO_REG(PMIO_STS_REG), PMIO_STS_RTC);///PMIO_Rx00[10] RTC Alarm Status
    IoOr16(PMIO_REG(PMIO_PM_EN), PMIO_PM_EN_RTC);   ///PMIO_Rx02[10] RTC Alarm Enable
    
    DEBUG((EFI_D_ERROR, "\n"));    
    SystemSoftOff();   
  }

  CmosWrite(RTC_ADDRESS_REGISTER_B, Cmos0B);

ProcExit:
  return;  
}


EFI_STATUS
EFIAPI
UpdateAsiaConfig (
  IN     EFI_BOOT_MODE                    BootMode,
  IN OUT ASIA_SB_CONFIGURATION            *SbCfg,
  IN OUT ASIA_NB_CONFIGURATION            *NbCfg,
  IN OUT ASIA_DRAM_CONFIGURATION          *DramCfg,
  IN     EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi
  )
{
  EFI_STATUS             Status;
  SETUP_DATA             *SetupData;
  SETUP_DATA             *VarHobSetupData;	
  UINTN                  Size;
  BOOLEAN                IsCmosBad;
  //UINT32                 RegEax, RegEbx, RegEcx, RegEdx;
  UINT8                  Data1,Data2;
	

  SetupData = BuildGuidHob(&gPlatformSetupVariableGuid, sizeof(SETUP_DATA));
  ASSERT(SetupData!=NULL);
  
// CMOS 0x0E is a normal storage. If RTC battery is LOST, this value will change to 0xFF.
// So we use this feature to judge RTC power loss or not.
// When setup variable has been sync from HOB, set it as ZERO.
  while(1)
  {
	  Data1=PciRead8(PCI_LIB_ADDRESS(0, 17, 0, 0x81))&0x04; //Rx81 bit2 Enable RTC Control Signal Gated with PSON (SUSC#) in Soft-Off Mode
	  Data2=PciRead8(PCI_LIB_ADDRESS(0, 17, 0, 0x82))&0x40; //Rx82 bit6 PSON (SUSC#) Current State
  if((Data1!=0)&&(Data2!=0)) break;
  }
  IsCmosBad = (CmosRead(0x0E) & BIT7)?TRUE:FALSE;
  DEBUG((EFI_D_INFO, "IsCmosBad:%d\n", IsCmosBad));
  IsCmosBad=0;//DLA_DBG_S Patch for a0
  if(IsCmosBad || BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
   goto LoadSetupDefault;
  }	 
  
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      SetupData
                      );
  if(!EFI_ERROR(Status)){
    goto GotSetupVariable;		
  } else {
    DEBUG((EFI_D_INFO, "GetSetupVar:%r\n", Status));
  }	
	
LoadSetupDefault:
  Status = CreateDefaultVariableHob(EFI_HII_DEFAULT_CLASS_STANDARD, 0, (VOID**)&VarHobSetupData);
  ASSERT_EFI_ERROR(Status);
  
// FCE tool has some limitation for gathering default value, so do override here.
  VarHobSetupData->RTCWakeupDate.Year   = 2016;
  VarHobSetupData->RTCWakeupDate.Month  = 1;
  VarHobSetupData->RTCWakeupDate.Day    = 1;
  VarHobSetupData->RTCWakeupTime.Hour   = 0;
  VarHobSetupData->RTCWakeupTime.Minute = 0;
  VarHobSetupData->RTCWakeupTime.Second = 0;
  if(VarHobSetupData->Csm){
    VarHobSetupData->UefiPxeIPV4 = 0;
    VarHobSetupData->UefiPxeIPV6 = 0;
  }
  
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      SetupData
                      );
  ASSERT_EFI_ERROR(Status);
  ASSERT(SetupData->RTCWakeupDate.Year == 2016);		


//------------------------------------------------------------------------------
GotSetupVariable:

  DEBUG((EFI_D_ERROR, "SetupData->Csm = %x \n",SetupData->Csm));
  //chipset sb
  SbCfg->ApplicationModeSelection= SetupData->ApplicationModeSelection;
  ///
  SbCfg->SBSPEValue= SetupData->SBSPEValue;
  SbCfg->SATASPEValue= SetupData->SATASPEValue;
  SbCfg->VARTSPEValue= SetupData->VARTSPEValue;
  SbCfg->ESPISPEValue   = SetupData->ESPISPEValue;	  
  SbCfg->BusCtrlSPEValue= SetupData->BusCtrlSPEValue;
  SbCfg->PMUSPEValue= SetupData->PMUSPEValue;
  SbCfg->PCCASPEValue= SetupData->PCCASPEValue;
  SbCfg->HDACSPEValue= SetupData->HDACSPEValue;   
  SbCfg->SPISPEValue=SetupData->SPISPEValue;
  SbCfg->SOCXHCISPEValue=SetupData->SOCXHCISPEValue;
  SbCfg->SOCEHCISPEValue=SetupData->SOCEHCISPEValue;
  SbCfg->SOCUHCISPEValue=SetupData->SOCUHCISPEValue;
  // SbCfg->P2PBSPEValue = 2; 
  
  //this varible is needed when some TACTL linkage configuration is added in the SBInit.c
  SbCfg->IOVEnable= SetupData->IOVEnable;
  
  SbCfg->SbApicID	= 9;
  //SbCfg->AHCIGen=SetupData->AHCIGen;  

  //UART
  if(PcdGetBool(PcdBiosDebugUsePciUart) == TRUE) {  
    /// default should come here (PlatformPkg.dec)	
    SbCfg->OnChipUartMode= SetupData->OnChipUartMode;
    SbCfg->UartModuleSelection= SetupData->UartModuleSelection;
    SbCfg->Uart0Enable= SetupData->Uart0Enable;
    SbCfg->Uart0IoBaseSelection= SetupData->Uart0IoBaseSelection;
    SbCfg->Uart0IRQSelection= SetupData->Uart0IRQSelection;
	
    SbCfg->Uart1Enable= SetupData->Uart1Enable;
    SbCfg->Uart1IoBaseSelection= SetupData->Uart1IoBaseSelection;
    SbCfg->Uart1IRQSelection= SetupData->Uart1IRQSelection;
	
    SbCfg->Uart2Enable= SetupData->Uart2Enable;
    SbCfg->Uart2IoBaseSelection= SetupData->Uart2IoBaseSelection;
    SbCfg->Uart2IRQSelection= SetupData->Uart2IRQSelection;
	
    SbCfg->Uart3Enable= SetupData->Uart3Enable;
    SbCfg->Uart3IoBaseSelection= SetupData->Uart3IoBaseSelection;
    SbCfg->Uart3IRQSelection= SetupData->Uart3IRQSelection;   
    SbCfg->UartFLREn= SetupData->UartFLREn;   
	

  } else {
    SbCfg->OnChipUartMode = ON_CHIP_UART_MODE_DISABLED;
  }
  
  SbCfg->ESPI = SetupData->ESPI;
  SbCfg->SPIBus0ClockSelect= SetupData->SPIBus0ClockSelect;
  SbCfg->PmioBar = PcdGet16(AcpiIoPortBaseAddress);
  SbCfg->MobileCenterControl= SetupData->MobileCenterControl;
  SbCfg->ACAdapterControl= SetupData->ACAdapterControl;
  SbCfg->MsiSupport= SetupData->MsiSupport;
  ///
  SbCfg->SBDynamicClkControl  = SetupData->SBDynamicClkControl;  
  SbCfg->TMRCDynamicClkControl  = SetupData->TMRCDynamicClkControl;
  SbCfg->GoNonSnoopPath       = SetupData->GoNonSnoopPath; 

  SbCfg->eBMCSettleTime       = SetupData->eBMCSettleTime; 
  SbCfg->KBDCLegacySelControl       = SetupData->KBDCLegacySelControl; 
  SbCfg->INTCLegacySelControl       = SetupData->INTCLegacySelControl; 
  SbCfg->DMACLegacySelControl      = SetupData->DMACLegacySelControl; 
  SbCfg->TMRCLegacySelControl       = SetupData->TMRCLegacySelControl; 
  
  //PMU_ACPI
  SbCfg->ProcessorCState= SetupData->CpuCState;
  SbCfg->ConditionalC4= SetupData->ConditionalC4;

  //JYZ_Added_SVID 
	SbCfg->SVIDMVCLKControl 	  = SetupData->SVIDMVCLKControl;
	SbCfg->SVIDMPeriodicIoutControl 	  = SetupData->SVIDMPeriodicIoutControl;
	SbCfg->SVIDMC3Control		= SetupData->SVIDMC3Control; 
	SbCfg->SVIDMC4C5Control 	  = SetupData->SVIDMC4C5Control; 
	SbCfg->VRMStableBD		 = SetupData->VRMStableBD; 
	SbCfg->SVIDMC4SetPS 	  = SetupData->SVIDMC4SetPS; 
	SbCfg->CPU2SVIDCmdGate		 = SetupData->CPU2SVIDCmdGate; 
	SbCfg->CPUPStateTOCounter		= SetupData->CPUPStateTOCounter; 
	SbCfg->CPUPStateSetVIDDone		 = SetupData->CPUPStateSetVIDDone; 
	SbCfg->VRM0VIDFSSelect			= SetupData->VRM0VIDFSSelect;
	SbCfg->SVIDMC4SetPSSel			= SetupData->SVIDMC4SetPSSel;
	SbCfg->VRM0IoutTimer			= SetupData->VRM0IoutTimer;
	SbCfg->SVIDMC4DecayOP			= SetupData->SVIDMC4DecayOP;
	SbCfg->SVIDMC4SetVIDSel 		= SetupData->SVIDMC4SetVIDSel;
  
  //AsmCpuid(1, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  //if CNQ001 or new processor, then support C5.
  //if ((((RegEax & 0xFFFFF) >= 0x6FD) && ((RegEax & 0xFF000) == 0x0)) || ((RegEax & 0xFFFFF) >= 0x10690))
  //{
		SbCfg->C5Control = SetupData->C5Control;
  //}else
  //{
  //	SbCfg->C5Control = 0;
  //}

  SbCfg->VidSelect = SetupData->VID_SEL;
  
  ////
  SbCfg->C4BusMasterIdleTimer= SetupData->C4BusMasterIdleTimer;
  SbCfg->C3BusMasterIdleTimer= SetupData->C3BusMasterIdleTimer;
  SbCfg->ShortC3C4Mode= SetupData->ShortC3C4Mode;
  SbCfg->DPSLPtoSLP= SetupData->DPSLPtoSLP;
  SbCfg->VRDSLPtoDPSLP= SetupData->VRDSLPtoDPSLP;
  SbCfg->FixedFreeCxLatency= SetupData->FixedFreeCxLatency;
  SbCfg->DynamicT05= SetupData->DynamicT05;
  ///

  //SATA
  SbCfg->SataEn               = SetupData->SataEn;  
  SbCfg->SataCfg              = SetupData->SataCfg;
  SbCfg->IDEGen               = SetupData->IDEGen;
  SbCfg->IDECapSelect         = SetupData->IDECapSelect;
  SbCfg->IDEHIPMEn            = SetupData->IDEHIPMEn;
  SbCfg->AHCIGen              = SetupData->AHCIGen;
  SbCfg->AHCIMSI              = SetupData->AHCIMSI;
  SbCfg->AHCIMSIX             = SetupData->AHCIMSIX;
  SbCfg->AHCIHotplugEn        = SetupData->AHCIHotplugEn;
  SbCfg->AHCIALPMEn           = SetupData->AHCIALPMEn;
  SbCfg->FuncLevelResetEn     = SetupData->FuncLevelResetEn;
  SbCfg->GBFlushendEn         = SetupData->GBFlushendEn;
  //USB
  SbCfg->UsbModeSelect          = SetupData->UsbModeSelect;
  SbCfg->UsbS4WakeupCtrl        = SetupData->UsbS4WakeupCtrl;
  SbCfg->UsbOCCtrl              = SetupData->UsbOCCtrl;
  SbCfg->USBCFLRCtrl            = SetupData->USBCFLRCtrl;
  SbCfg->XhcMcuDmaPathCtrl     = SetupData->XhcMcuDmaPathCtrl;
  SbCfg->XhcTRBCacheBypassCtrl = SetupData->XhcTRBCacheBypassCtrl;
  SbCfg->XhcBurstCtrl       = SetupData->XhcBurstCtrl;
  SbCfg->XhcU1U2Ctrl           = SetupData->XhcU1U2Ctrl;
  SbCfg->XhcMsiFlushCtrl       = SetupData->XhcMsiFlushCtrl;
  SbCfg->XhcUartCtrl           = SetupData->XhcUartCtrl;
  SbCfg->XhcFLRCtrl            = SetupData->XhcFLRCtrl;
  SbCfg->XhcRTD3Ctrl           = SetupData->XhcRTD3Ctrl;  //DLA_DBG
  SbCfg->XhcC4BlockCtrl        = SetupData->XhcC4BlockCtrl;
  SbCfg->XhcEITRNCtrl          = SetupData->XhcEITRNCtrl;

  //CND003 USB	
  //SbCfg->IOEUsbModeSelectMode = SetupData->IOEUsbModeSelectMode;
  SbCfg->IOEUsbModeSelect       = SetupData->IOEUsbModeSelect;
  SbCfg->IOETRBCacheBypass      = SetupData->IOETRBCacheBypass;
  SbCfg->IOEXhciOutBurstEn      = SetupData->IOEXhciOutBurstEn;
  SbCfg->IOEXhciMaxBurstSize    = SetupData->IOEXhciMaxBurstSize;
  SbCfg->IOEUSBCFLRControl      = SetupData->IOEUSBCFLRControl;
  SbCfg->IOEXHCIFLRControl      = SetupData->IOEXHCIFLRControl;  
  SbCfg->IOERTD3Control         = SetupData->IOERTD3Control;  //DLA_DBG
  SbCfg->IOEUsbS4Wakeup         = SetupData->IOEUsbS4Wakeup;
  SbCfg->IOEUsbC4Block          = SetupData->IOEUsbC4Block;
  SbCfg->IOEUsb10GSupport       = SetupData->IOEUsb10GSupport;
  
  SbCfg->IOEUsbOCEn  = SetupData->IOEUsbOCEn;
  SbCfg->IOEUsbEITRN  = SetupData->IOEUsbEITRN;
  //HDAC
  SbCfg->Azalia= SetupData->ObAudioEn;
  SbCfg->HDACFLREn=SetupData->HDACFLREn;

  //SNMI
  SbCfg->IsoLPC= SetupData->IsoLPC;
  SbCfg->IsoVART= SetupData->IsoVART;
  SbCfg->IsoAZALIA= SetupData->IsoAZALIA;
  SbCfg->IsoESPI= SetupData->IsoESPI;  
  SbCfg->IsoSPI= SetupData->IsoSPI;
  
  NbCfg->PrimaryGraphicAdapter = SetupData->VideoPrimaryAdapter; // Setup UI's default value = PCIE GFX Card
  NbCfg->DualVGA               = SetupData->VideoDualVga; // Setup UI's default value = Disabled (0)

  NbCfg->PcieFPGAMode = TRUE;
  
  //RAIDA
  NbCfg->RAIDA0Enable = SetupData->RAIDA0En;
  NbCfg->RAIDA1Enable = SetupData->RAIDA1En;

  NbCfg->ApplicationModeSelection= SetupData->ApplicationModeSelection;
  NbCfg->NBSPEValue= SetupData->NBSPEValue;
  NbCfg->D0F0SPEValue= SetupData->D0F0SPEValue;
  NbCfg->D0F1SPEValue= SetupData->D0F1SPEValue;
  NbCfg->D0F2SPEValue= SetupData->D0F2SPEValue;
  NbCfg->D0F3SPEValue= SetupData->D0F3SPEValue;
  NbCfg->D0F4SPEValue= SetupData->D0F4SPEValue;
  NbCfg->D0F5SPEValue= SetupData->D0F5SPEValue;
  NbCfg->D0F6SPEValue= SetupData->D0F6SPEValue;
//  NbCfg->D2F0SPEValue= SetupData->D2F0SPEValue;
  NbCfg->D3F0SPEValue= SetupData->D3F0SPEValue;
  NbCfg->D3F1SPEValue =  SetupData->D3F1SPEValue;
  NbCfg->D3F2SPEValue =  SetupData->D3F2SPEValue;
  NbCfg->D3F3SPEValue =  SetupData->D3F3SPEValue;
  ///
  NbCfg->D4F0SPEValue= SetupData->D4F0SPEValue;
  NbCfg->D4F1SPEValue =  SetupData->D4F1SPEValue;
  NbCfg->D5F0SPEValue= SetupData->D5F0SPEValue;
  NbCfg->D5F1SPEValue =  SetupData->D5F1SPEValue;
  NbCfg->D8F0SPEValue =  SetupData->D8F0SPEValue;
  NbCfg->D9F0SPEValue =  SetupData->D9F0SPEValue;
  NbCfg->RCRBHSPEValue= SetupData->RCRBHSPEValue;
  NbCfg->PCIEEPHYSPEValue= SetupData->PCIEEPHYSPEValue;
  NbCfg->D1F0SPEValue= SetupData->D1F0SPEValue; // 2 : means Energy Value

  //PCIE related
  NbCfg->PcieRst= SetupData->PcieRst;
//  NbCfg->PcieRstPEG= SetupData->PcieRstPEG;
  NbCfg->PcieRstPE0= SetupData->PcieRstPE0;
  NbCfg->PcieRstPE1= SetupData->PcieRstPE1;
  NbCfg->PcieRstPE2= SetupData->PcieRstPE2;
  NbCfg->PcieRstPE3= SetupData->PcieRstPE3;
  NbCfg->PcieRstPE4= SetupData->PcieRstPE4;
  NbCfg->PcieRstPE5= SetupData->PcieRstPE5;
  NbCfg->PcieRstPE6= SetupData->PcieRstPE6;
  NbCfg->PcieRstPE7= SetupData->PcieRstPE7;

  NbCfg->PcieLinkSpeed= SetupData->PcieLinkSpeed;
//  NbCfg->PciePEGLinkSpeed= SetupData->PciePEGLinkSpeed;		//RP Speed Select
  NbCfg->PciePE0LinkSpeed= SetupData->PciePE0LinkSpeed;
  NbCfg->PciePE1LinkSpeed= SetupData->PciePE1LinkSpeed;
  NbCfg->PciePE2LinkSpeed= SetupData->PciePE2LinkSpeed;
  NbCfg->PciePE3LinkSpeed= SetupData->PciePE3LinkSpeed;
  NbCfg->PciePE4LinkSpeed= SetupData->PciePE4LinkSpeed;
  NbCfg->PciePE5LinkSpeed= SetupData->PciePE5LinkSpeed;
  NbCfg->PciePE6LinkSpeed= SetupData->PciePE6LinkSpeed;
  NbCfg->PciePE7LinkSpeed= SetupData->PciePE7LinkSpeed;
  ///  
  NbCfg->PcieRP= SetupData->PcieRP;  
//  NbCfg->PciePEG= SetupData->PciePEG;
  NbCfg->PciePE0= SetupData->PciePE0;
  NbCfg->PciePE1= SetupData->PciePE1;
  NbCfg->PciePE2= SetupData->PciePE2;
  NbCfg->PciePE3= SetupData->PciePE3;
  NbCfg->PciePE4= SetupData->PciePE4;
  NbCfg->PciePE5= SetupData->PciePE5;
  NbCfg->PciePE6= SetupData->PciePE6;
  NbCfg->PciePE7= SetupData->PciePE7; 
  NbCfg->PcieForceLinkWidth = SetupData->PcieForceLinkWidth;

  NbCfg->PcieEQ=SetupData->PcieEQ;
  NbCfg->EQTxPreset=SetupData->EQTxPreset;

  NbCfg->PcieEQTS2 = SetupData->PcieEQTS2;
  NbCfg->PcieCrsMech = SetupData->PcieCrsMech;
  NbCfg->PcieOptimalTLS = SetupData->PcieOptimalTLS;
  
  NbCfg->PcieEqCtlOrgValL0=SetupData->PcieEqCtlOrgValL0;
  NbCfg->PcieEqCtlOrgValL1=SetupData->PcieEqCtlOrgValL1;
  NbCfg->PcieEqCtlOrgValL2=SetupData->PcieEqCtlOrgValL2;
  NbCfg->PcieEqCtlOrgValL3=SetupData->PcieEqCtlOrgValL3;
  NbCfg->PcieEqCtlOrgValL4=SetupData->PcieEqCtlOrgValL4;
  NbCfg->PcieEqCtlOrgValL5=SetupData->PcieEqCtlOrgValL5;
  NbCfg->PcieEqCtlOrgValL6=SetupData->PcieEqCtlOrgValL6;
  NbCfg->PcieEqCtlOrgValL7=SetupData->PcieEqCtlOrgValL7;
  NbCfg->PcieEqCtlOrgValL8=SetupData->PcieEqCtlOrgValL8;
  NbCfg->PcieEqCtlOrgValL9=SetupData->PcieEqCtlOrgValL9;
  NbCfg->PcieEqCtlOrgValL10=SetupData->PcieEqCtlOrgValL10;
  NbCfg->PcieEqCtlOrgValL11=SetupData->PcieEqCtlOrgValL11;
  NbCfg->PcieEqCtlOrgValL12=SetupData->PcieEqCtlOrgValL12;
  NbCfg->PcieEqCtlOrgValL13=SetupData->PcieEqCtlOrgValL13;
  NbCfg->PcieEqCtlOrgValL14=SetupData->PcieEqCtlOrgValL14;
  NbCfg->PcieEqCtlOrgValL15=SetupData->PcieEqCtlOrgValL15;

  ////
  NbCfg->PEMCU_LoadFW_WhenBoot=SetupData->PEMCU_LoadFW_WhenBoot;
  NbCfg->SelectableDeEmphasis=SetupData->SelectableDeEmphasis;
  //NbCfg->PEMCU_RstSys_WhenFail=SetupData->PEMCU_RstSys_WhenFail;   

  NbCfg->PcieHotReset= SetupData->PcieHotReset;
  NbCfg->MaxPayloadSize= SetupData->MaxPayloadSize; // Setup UI's default value = 256 bytes
  //MKE_20170220 MaxPayload need 128(PXP limitation)_S
  #ifdef CHX002_PXP
  NbCfg->MaxPayloadSize= 0;
  #endif
  //MKE_20170220 MaxPayload need 128(PXP limitation)_E
  NbCfg->PcieASPM= SetupData->PcieASPM; // default = disabled
  NbCfg->RelaxedOrder= SetupData->RelaxedOrder;
  NbCfg->ExtTag= SetupData->ExtTag;
  NbCfg->ExtSync= SetupData->ExtSync; 

//  NbCfg->PEGErrControl= SetupData->PEGErrControl;
  NbCfg->PE0ErrControl= SetupData->PE0ErrControl;
  NbCfg->PE1ErrControl= SetupData->PE1ErrControl;
  NbCfg->PE2ErrControl= SetupData->PE2ErrControl;
  NbCfg->PE3ErrControl= SetupData->PE3ErrControl;
  NbCfg->PE4ErrControl= SetupData->PE4ErrControl;
  NbCfg->PE5ErrControl= SetupData->PE5ErrControl;
  NbCfg->PE6ErrControl= SetupData->PE6ErrControl;
  NbCfg->PE7ErrControl= SetupData->PE7ErrControl; 

  NbCfg->PcieRoutingCtrl = SetupData->PcieRoutingCtrl; 

 // NbCfg->PEG_Msgc2PcieIntx = SetupData->PEG_Msgc2PcieIntx;
  NbCfg->PE0_Msgc2PcieIntx = SetupData->PE0_Msgc2PcieIntx;
  NbCfg->PE1_Msgc2PcieIntx = SetupData->PE1_Msgc2PcieIntx;
  NbCfg->PE2_Msgc2PcieIntx = SetupData->PE2_Msgc2PcieIntx;
  NbCfg->PE3_Msgc2PcieIntx = SetupData->PE3_Msgc2PcieIntx;
  ///
  NbCfg->PE4_Msgc2PcieIntx = SetupData->PE4_Msgc2PcieIntx;
  NbCfg->PE5_Msgc2PcieIntx = SetupData->PE5_Msgc2PcieIntx;
  NbCfg->PE6_Msgc2PcieIntx = SetupData->PE6_Msgc2PcieIntx;
  NbCfg->PE7_Msgc2PcieIntx = SetupData->PE7_Msgc2PcieIntx;
  ///
//  NbCfg->PEG_PcieIntx2Nb2sbIntx = SetupData->PEG_PcieIntx2Nb2sbIntx;
  NbCfg->PE0_PcieIntx2Nb2sbIntx = SetupData->PE0_PcieIntx2Nb2sbIntx;
  NbCfg->PE1_PcieIntx2Nb2sbIntx = SetupData->PE1_PcieIntx2Nb2sbIntx;
  NbCfg->PE2_PcieIntx2Nb2sbIntx = SetupData->PE2_PcieIntx2Nb2sbIntx;
  NbCfg->PE3_PcieIntx2Nb2sbIntx = SetupData->PE3_PcieIntx2Nb2sbIntx;
  ///
  NbCfg->PE4_PcieIntx2Nb2sbIntx = SetupData->PE4_PcieIntx2Nb2sbIntx;
  NbCfg->PE5_PcieIntx2Nb2sbIntx = SetupData->PE5_PcieIntx2Nb2sbIntx;
  NbCfg->PE6_PcieIntx2Nb2sbIntx = SetupData->PE6_PcieIntx2Nb2sbIntx;
  NbCfg->PE7_PcieIntx2Nb2sbIntx = SetupData->PE7_PcieIntx2Nb2sbIntx;
  ///
  //NbCfg->PegApciIrq = SetupData->PegApciIrq;
  NbCfg->Pe6ApciIrq = SetupData->Pe6ApciIrq;
  NbCfg->Pe7ApciIrq = SetupData->Pe7ApciIrq;
  NbCfg->PcieASPMBootArch = SetupData->PcieASPMBootArch;
  //LNA-2016122701-S
  NbCfg->PcieOBFFCtrl_PCIE = SetupData->PcieOBFFCtrl_PCIE;
  NbCfg->PcieOBFFCtrl_PMU = SetupData->PcieOBFFCtrl_PMU;
  //LNA-2016122701-E
#ifdef ZX_SECRET_CODE
  NbCfg->CPU_FSBC_EN	= SetupData->CPU_MASTER_FSBC_EN;
  NbCfg->CPU_FSBC_PCIE_ON=SetupData->CPU_FSBC_PCIE_ON;
  NbCfg->CPU_FSBC_TOPCIE=SetupData->CPU_FSBC_TOPCIE;

  NbCfg->CPU_FSBC_MISSPACKE_EN=SetupData->CPU_FSBC_MISSPACKE_EN;
  NbCfg->CPU_FSBC_TIGPULSE_EN=SetupData->CPU_FSBC_TIGPULSE_EN;
  NbCfg->CPU_FSBC_IFSBCSTP_EN=SetupData->CPU_FSBC_IFSBCSTP_EN;

  NbCfg->CPU_FSBC_STREAM_EN=SetupData->CPU_FSBC_STREAM_EN;
#endif
  
  /// Synched here.
  //OTHERS
  SbCfg->EnableMultimediaTimer= SetupData->EnableMultimediaTimer;
  SbCfg->EnableMultimediaTimerMsi= SetupData->EnableMultimediaTimerMsi;
  SbCfg->MultimediaTimerMode= SetupData->MultimediaTimerMode;
  SbCfg->WatchDogTimer= SetupData->WatchDogTimer;
  SbCfg->WatchDogTimerRunStop= SetupData->WatchDogTimerRunStop;
  SbCfg->WatchDogTimerAction= SetupData->WatchDogTimerAction;
  SbCfg->WatchDogTimerCount= SetupData->WatchDogTimerCount;
  SbCfg->KBMouseWakeupControl= SetupData->KBMouseWakeupControl;
  SbCfg->SMBusControllerUnderOS= SetupData->SMBusControllerUnderOS;
  SbCfg->SMBHostClockFrequencySelect= SetupData->SMBusHostClockFrequencySelect;
  SbCfg->SMBHostClockFrequency= SetupData->SMBusHostClockFrequency;

  
  //Chipset NB  
  NbCfg->NbApicID = 10; 


  NbCfg->CpuClockControl= SetupData->CpuClockControl;
  //NbCfg->CpuSSCControl= SetupData->CpuSSCControl;
  //NbCfg->PcieClockControl= SetupData->PcieClockControl;
  //NbCfg->CpuBclkControl= SetupData->CpuBclkControl;

  NbCfg->SERRNBControl=SetupData->SERRNBControl; 
  NbCfg->HIFErrControl=SetupData->HIFErrControl; 


  NbCfg->UMAEn                 = SetupData->UMAEn;
  NbCfg->SelectDisplayDevice   = SetupData->SelectDisplayDevice;
  NbCfg->DP1        = SetupData->DP1;
  NbCfg->DP2        = SetupData->DP2;
  NbCfg->DVO        = SetupData->DVO;
  NbCfg->CRT        = SetupData->CRT;
  NbCfg->ECLKCtrl= SetupData->ECLKCtrl;
  NbCfg->ECLKFreq= SetupData->ECLKFreq; 
  NbCfg->VCLKCtrl= SetupData->VCLKCtrl;
  NbCfg->VCLKFreq= SetupData->VCLKFreq;
  NbCfg->ICLKCtrl= SetupData->ICLKCtrl;
  NbCfg->ICLKFreq= SetupData->ICLKFreq;
  NbCfg->DP1SSCEn= SetupData->DP1SSCEn;
  NbCfg->DP1SSCMode= SetupData->DP1SSCMode;
  NbCfg->DP2SSCEn= SetupData->DP2SSCEn;
  NbCfg->DP2SSCMode= SetupData->DP2SSCMode;
  NbCfg->IOVEnable= SetupData->IOVEnable;
  NbCfg->IOVQIEnable= SetupData->IOVQIEnable;
  NbCfg->IOVINTREnable= SetupData->IOVINTREnable;
  NbCfg->RoundRobin = SetupData->RoundRobin;

  //DRAM
  	 DramCfg->DramClk = SetupData->DramClk;    
	 DramCfg->RxIoTimingMethod= SetupData->RxIoTimingMethod;
	 DramCfg->DQSIByRank= SetupData->DQSIByRank; 
     DramCfg->TxIoTimingMethod= SetupData->TxIoTimingMethod;
	 DramCfg->LimitRankSize= SetupData->LimitRankSize;
     DramCfg->BankIntlv= SetupData->BankIntlv;
     DramCfg->MemoryChipODTDebug= SetupData->MemoryChipODTDebug;    
     DramCfg->MemoryChipODTWRDebug= SetupData->MemoryChipODTWRDebug;
	 DramCfg->MemoryChipODTParkDebug = SetupData->MemoryChipODTParkDebug;
     DramCfg->VRIntlv= SetupData->VRInlv;    
     DramCfg->ChannelIntlv= SetupData->ChannelIntlv;	
	 DramCfg->DRAMECC= SetupData->DramECC;
	 DramCfg->DataScmb= SetupData->DataScmb;	
     DramCfg->BurstLength= SetupData->BL;
     DramCfg->CmdRate= SetupData->CmdRate;    
     DramCfg->VGAShareMemory= SetupData->VGAShareMemory;    
     DramCfg->DramInitMethod= SetupData->DramInitMethod;        
     DramCfg->DramSelfRefresh= SetupData->DramSelfRefresh;
     DramCfg->DynamicCKE= SetupData->DynamicCKE;
     DramCfg->RemapEn= SetupData->RemapEn;
	 DramCfg->DramFastBoot= SetupData->DramFastBoot;
DramCfg->DramRxTxTimingHardCode= SetupData->DramRxTxTimingHardCode;
	 DramCfg->TNIHighPulse = SetupData->TNIHighPulse;

	 DramCfg->CHdecode = SetupData->CHdecode;   //DKS -20161212 add
	 DramCfg->CRCEn= SetupData->CRCEn;   //CRC Enable
	 DramCfg->CAParEn= SetupData->CAParEn;  //CA Parity check Enable
	 DramCfg->CAParPerEn= SetupData->CAParPerEn; //CA Parity persistent Enable
	 DramCfg->WPREA= SetupData->WPREA;  //Write preamble	    
	 DramCfg->RPREA= SetupData->RPREA;  //read preambleR	   
	 DramCfg->CALEn= SetupData->CALEn; //CAL Latency enable
	 DramCfg->ACTimingOption = SetupData->ACTimingOption;	
	 DramCfg->DramVoltageControl = SetupData->DramVoltageControl;  // IVES-20180517
	 DramCfg->DramCL= SetupData->DramCL;	
	 DramCfg->DramTrcd= SetupData->DramTrcd;	
	 DramCfg->DramTrp= SetupData->DramTrp;	
	 DramCfg->DramTras= SetupData->DramTras;	
	 DramCfg->CRCParRetryEn= SetupData->CRCParRetryEn;
 	 DramCfg->ParErrControl= SetupData->ParErrControl;   
	 DramCfg->CRCErrControl= SetupData->CRCErrControl;		
	 DramCfg->EccRetry= SetupData->EccRetry;	
	 DramCfg->BASelect= SetupData->BASelect;
	 DramCfg->EccPatrolScrub= SetupData->EccPatrolScrub;
	 DramCfg->DRAMErrControl= SetupData->DRAMErrControl;
	 DramCfg->CPU_FSBC_ON= SetupData->CPU_FSBC_DRAM_ON; //FSBC Debug Mode
	 DramCfg->TxVref = SetupData->TxVref;
	 DramCfg->TxVrefAllByte = SetupData->TxVrefAllByte;
     DramCfg->SwapChAB = SetupData->SwapChAB;
     DramCfg->ScanIOTiming = SetupData->ScanIOTiming;
     DramCfg->RxVref = SetupData->RxVref;
     DramCfg->RRankRDelay = SetupData->RRankRDelay;
     DramCfg->RRankWDelay = SetupData->RRankWDelay;
     DramCfg->BankGroupBit0Decode = SetupData->BankGroupBit0Decode;
// do override here.
//-----------------------------------------------------------
  //NbCfg->PciePE1 = SetupData->OnboardLan; // should check CHX001 EVB Board.

  ////
  NbCfg->DebugMode= SetupData->DebugMode;
  NbCfg->DebugOutputSelect= SetupData->DebugOutputSelect;
  NbCfg->DebugSignalSelect0= SetupData->DebugSignalSelect0;
  NbCfg->DebugSignalSelect1= SetupData->DebugSignalSelect1;  
  NbCfg->DebugModuleSelect0= SetupData->DebugModuleSelect0;
  NbCfg->DebugModuleSelect1= SetupData->DebugModuleSelect1;  

  NbCfg->NBGroup0TopSelect= SetupData->NBGroup0TopSelcet;
  NbCfg->NBGroup0ModuleSubSelect1= SetupData->NBGroup0ModuleSubSelect1;
  NbCfg->NBGroup0ModuleSubSelect2= SetupData->NBGroup0ModuleSubSelect2;
  NbCfg->NBGroup1TopSelect= SetupData->NBGroup1TopSelcet;
  NbCfg->NBGroup1ModuleSubSelect1= SetupData->NBGroup1ModuleSubSelect1;
  NbCfg->NBGroup1ModuleSubSelect2= SetupData->NBGroup1ModuleSubSelect2;
  NbCfg->DebugSBselectByNB= SetupData->DebugSBSelectByNB;
  NbCfg->SBTopDbgMux1= SetupData->SBTopDbgMux1;
  NbCfg->SBGroup0ModuleSubSelect= SetupData->SBGroup0ModuleSubSelect;
 
  NbCfg->SBTopDbgMux2= SetupData->SBTopDbgMux2;
  NbCfg->SBGroup1ModuleSubSelect= SetupData->SBGroup1ModuleSubSelect;
  

  NbCfg->TOPGroup0TopSelcet= SetupData->TOPGroup0TopSelcet;  
  NbCfg->TOPGroup0ModuleSubSelcet= SetupData->TOPGroup0ModuleSubSelcet;  
 // NbCfg->TOPGroup0XhciModuleSelcet= SetupData->TOPGroup0XhciModuleSelcet;  
 NbCfg->TOPGroup0XhciModule0Selcet= SetupData->TOPGroup0XhciModule0Selcet;	
 NbCfg->TOPGroup0XhciModule1Selcet= SetupData->TOPGroup0XhciModule1Selcet;	
 NbCfg->TOPGroup0XhciGroup0Selcet= SetupData->TOPGroup0XhciGroup0Selcet;	
 NbCfg->TOPGroup0XhciGroup1Selcet= SetupData->TOPGroup0XhciGroup1Selcet;	
 NbCfg->TOPGroup0XhciMCUSelcet= SetupData->TOPGroup0XhciMCUSelcet;	
 

  NbCfg->TOPGroup1TopSelcet= SetupData->TOPGroup1TopSelcet;
  NbCfg->TOPGroup1ModuleSubSelcet= SetupData->TOPGroup1ModuleSubSelcet;  
 // NbCfg->TOPGroup1XhciModuleSelcet= SetupData->TOPGroup1XhciModuleSelcet;  
  NbCfg->TOPGroup1XhciModule2Selcet= SetupData->TOPGroup1XhciModule2Selcet; 
  NbCfg->TOPGroup1XhciModule3Selcet= SetupData->TOPGroup1XhciModule3Selcet; 
  NbCfg->TOPGroup1XhciGroup2Selcet= SetupData->TOPGroup1XhciGroup2Selcet;	
  NbCfg->TOPGroup1XhciGroup3Selcet= SetupData->TOPGroup1XhciGroup3Selcet;	
  NbCfg->TOPGroup1XhciMCUSelcet= SetupData->TOPGroup1XhciMCUSelcet;  

  NbCfg->SB_HS_DBG_SEL= SetupData->SB_HS_DBG_SEL;
  NbCfg->SB_HS_PCIE_PORT_SEL= SetupData->SB_HS_PCIE_PORT_SEL;
  NbCfg->SB_HS_DBG_CH_SEL= SetupData->SB_HS_DBG_CH_SEL;
  //vdd off debug signal
  NbCfg->VDD_OFF_EN= SetupData->VDD_OFF_EN;
  NbCfg->VDD_OFF_Module_Sel= SetupData->VDD_OFF_Module_Sel;
  NbCfg->VDD_OFF_Group_Sel= SetupData->VDD_OFF_Group_Sel;

  // JNY Porting IOE TOP/PCIE -S
  // CND003 TOP/PCIE
  // CND003 PCIE_PTN
  NbCfg->Cnd003PhyCfg = SetupData->Cnd003PhyCfg;
  NbCfg->Cnd003PhyACfg = SetupData->Cnd003PhyACfg;
  NbCfg->Cnd003PhyBCfg = SetupData->Cnd003PhyBCfg;
  NbCfg->Cnd003EpCap	=SetupData->Cnd003EpCap;
  NbCfg->Cnd003CapPEA0	=SetupData->Cnd003CapPEA0;
  NbCfg->Cnd003CapPEA1	=SetupData->Cnd003CapPEA1;
  NbCfg->Cnd003CapPEA2	=SetupData->Cnd003CapPEA2;
  NbCfg->Cnd003CapPEA3	=SetupData->Cnd003CapPEA3;
  NbCfg->Cnd003CapPEA4	=SetupData->Cnd003CapPEA4;
  NbCfg->Cnd003CapPEB0	=SetupData->Cnd003CapPEB0;
  NbCfg->Cnd003CapPEB1	=SetupData->Cnd003CapPEB1;
  NbCfg->Cnd003CapPESB  =SetupData->Cnd003CapPESB;
  
	NbCfg->Cnd003PcieRstTest = SetupData->Cnd003PcieRstTest;
	NbCfg->Cnd003PEA0RstTest = SetupData->Cnd003PEA0RstTest;
	NbCfg->Cnd003PEA1RstTest = SetupData->Cnd003PEA1RstTest;
	NbCfg->Cnd003PEA2RstTest = SetupData->Cnd003PEA2RstTest;
	NbCfg->Cnd003PEA3RstTest = SetupData->Cnd003PEA3RstTest;
	NbCfg->Cnd003PEA4RstTest = SetupData->Cnd003PEA4RstTest;
	NbCfg->Cnd003PEB0RstTest = SetupData->Cnd003PEB0RstTest;
	NbCfg->Cnd003PEB1RstTest = SetupData->Cnd003PEB1RstTest;	
  
  NbCfg->Cnd003LinkWidth = SetupData->Cnd003LinkWidth;  
  // CND003 DN port control
  NbCfg->IoeDnPortCtl = SetupData->IoeDnPortCtl;
  NbCfg->IoePEA0Ctl = SetupData->IoePEA0Ctl;
  NbCfg->IoePEA1Ctl = SetupData->IoePEA1Ctl;
  NbCfg->IoePEA2Ctl = SetupData->IoePEA2Ctl;
  NbCfg->IoePEA3Ctl = SetupData->IoePEA3Ctl;
  NbCfg->IoePEA4Ctl = SetupData->IoePEA4Ctl;
  NbCfg->IoePEB0Ctl = SetupData->IoePEB0Ctl;
  NbCfg->IoePEB1Ctl = SetupData->IoePEB1Ctl;
  NbCfg->IoeDnPortPEXRST= SetupData->IoeDnPortPEXRST;
  NbCfg->IoePEA0PEXRST= SetupData->IoePEA0PEXRST;
  NbCfg->IoePEA1PEXRST= SetupData->IoePEA1PEXRST;
  NbCfg->IoePEA2PEXRST= SetupData->IoePEA2PEXRST;
  NbCfg->IoePEA3PEXRST= SetupData->IoePEA3PEXRST;
  NbCfg->IoePEA4PEXRST= SetupData->IoePEA4PEXRST;
  NbCfg->IoePEB0PEXRST= SetupData->IoePEB0PEXRST;
  NbCfg->IoePEB1PEXRST= SetupData->IoePEB1PEXRST;

  NbCfg->Cnd003PcieEq = SetupData->Cnd003PcieEq;
  NbCfg->Cnd003Autofill = SetupData->Cnd003Autofill;
  NbCfg->Cnd003BootErrorClear = SetupData->Cnd003BootErrorClear;
 	NbCfg->Cnd003ForceMPS128B = SetupData->Cnd003ForceMPS128B;
	NbCfg->Cnd003SwCtlDnPortLinkup = SetupData->Cnd003SwCtlDnPortLinkup;
  
  //CND003 TOP
  NbCfg->Cnd003ModeSel = SetupData->Cnd003ModeSel;
  SbCfg->Cnd003ModeSel = SetupData->Cnd003ModeSel;
  NbCfg->Cnd003HideXbctl= SetupData->Cnd003HideXbctl;
  NbCfg->Cnd003HideEptrfc= SetupData->Cnd003HideEptrfc;
  SbCfg->Cnd003HideXbctl = SetupData->Cnd003HideXbctl;
  SbCfg->Cnd003HideEptrfc = SetupData->Cnd003HideEptrfc;
  NbCfg->Cnd003TCxFavor = SetupData->Cnd003TCxFavor;
  //Cnd003 DebugSignal
  NbCfg->Cnd003DebugFromCpu	=SetupData->Cnd003DebugFromCpu;
  NbCfg->Cnd003ModuleG0	=SetupData->Cnd003ModuleG0;
  NbCfg->Cnd003ModuleG1	=SetupData->Cnd003ModuleG1;
  NbCfg->Cnd003DebugG0	=SetupData->Cnd003DebugG0;
  NbCfg->Cnd003DebugG1	=SetupData->Cnd003DebugG1;
  NbCfg->Cnd003DebugG2	=SetupData->Cnd003DebugG2;
  NbCfg->Cnd003DebugG3	=SetupData->Cnd003DebugG3;
  NbCfg->Cnd003DebugG0_XHCI	=SetupData->Cnd003DebugG0_XHCI;
  NbCfg->Cnd003DebugG1_XHCI	=SetupData->Cnd003DebugG1_XHCI;
  NbCfg->Cnd003DebugG2_XHCI	=SetupData->Cnd003DebugG2_XHCI;
  NbCfg->Cnd003DebugG3_XHCI	=SetupData->Cnd003DebugG3_XHCI;
  NbCfg->Cnd003DebugSuspend=SetupData->Cnd003DebugSuspend;
  NbCfg->Cnd003HighSpeed=SetupData->Cnd003HighSpeed;
  NbCfg->Cnd003HighSpeedModuleSel=SetupData->Cnd003HighSpeedModuleSel;
  NbCfg->Cnd003HighSpeedDbgSel=SetupData->Cnd003HighSpeedDbgSel;
  NbCfg->Cnd003CorePowerOff = SetupData->Cnd003CorePowerOff;
  NbCfg->Cnd003DbgPadG0En = SetupData->Cnd003DbgPadG0En;
  NbCfg->Cnd003DbgPadG1En = SetupData->Cnd003DbgPadG1En;
  NbCfg->Cnd003DbgPadG2En = SetupData->Cnd003DbgPadG2En;
  NbCfg->Cnd003DbgPadG3En = SetupData->Cnd003DbgPadG3En;
  //Cnd003 DebugSignal
  SbCfg->Cnd003DebugFromCpu	=SetupData->Cnd003DebugFromCpu;
  SbCfg->Cnd003ModuleG0	=SetupData->Cnd003ModuleG0;
  SbCfg->Cnd003ModuleG1	=SetupData->Cnd003ModuleG1;
  SbCfg->Cnd003DebugG0	=SetupData->Cnd003DebugG0;
  SbCfg->Cnd003DebugG1	=SetupData->Cnd003DebugG1;
  SbCfg->Cnd003DebugG2	=SetupData->Cnd003DebugG2;
  SbCfg->Cnd003DebugG3	=SetupData->Cnd003DebugG3;
  SbCfg->Cnd003DebugG0_XHCI	=SetupData->Cnd003DebugG0_XHCI;
  SbCfg->Cnd003DebugG1_XHCI	=SetupData->Cnd003DebugG1_XHCI;
  SbCfg->Cnd003DebugG2_XHCI	=SetupData->Cnd003DebugG2_XHCI;
  SbCfg->Cnd003DebugG3_XHCI	=SetupData->Cnd003DebugG3_XHCI;
  SbCfg->Cnd003DebugSuspend=SetupData->Cnd003DebugSuspend;
  SbCfg->Cnd003HighSpeed=SetupData->Cnd003HighSpeed;
  SbCfg->Cnd003HighSpeedModuleSel=SetupData->Cnd003HighSpeedModuleSel;
  SbCfg->Cnd003HighSpeedDbgSel=SetupData->Cnd003HighSpeedDbgSel;  
  SbCfg->Cnd003CorePowerOff = SetupData->Cnd003CorePowerOff;
  SbCfg->Cnd003DbgPadG0En = SetupData->Cnd003DbgPadG0En;
  SbCfg->Cnd003DbgPadG1En = SetupData->Cnd003DbgPadG1En;
  SbCfg->Cnd003DbgPadG2En = SetupData->Cnd003DbgPadG2En;
  SbCfg->Cnd003DbgPadG3En = SetupData->Cnd003DbgPadG3En;

  SbCfg->Cnd003UartPinEn = SetupData->Cnd003UartPinEn;
  NbCfg->Cnd003UartPinEn = SetupData->Cnd003UartPinEn;
  
#ifdef ZX_SECRET_CODE
  NbCfg->Cnd003DIDVIDChoose = SetupData->Cnd003DIDVIDChoose;
#endif
  NbCfg->Cnd003ChangeSIDForISB = SetupData->Cnd003ChangeSIDForISB;
  NbCfg->Cnd003SwDbg1 = SetupData->Cnd003SwDbg1;
  	NbCfg->Cnd003SwDbg2 = SetupData->Cnd003SwDbg2;
	
	SbCfg->Cnd003DC = SetupData->Cnd003DC;			//DBGCAP
	SbCfg->Cnd003DCModule = SetupData->Cnd003DCModule;
	SbCfg->Cnd003DCTrans = SetupData->Cnd003DCTrans;
	SbCfg->Cnd003DCCfgPath = SetupData->Cnd003DCCfgPath;
	SbCfg->Cnd003DCChannel = SetupData->Cnd003DCChannel;
	SbCfg->Cnd003DCDbg0Mask = SetupData->Cnd003DCDbg0Mask;
	SbCfg->Cnd003DCDbg1Mask = SetupData->Cnd003DCDbg1Mask;
	SbCfg->Cnd003DCTriggerMode = SetupData->Cnd003DCTriggerMode;
	SbCfg->Cnd003DCReqPtn = SetupData->Cnd003DCReqPtn;
	SbCfg->Cnd003DCReqPtnMask = SetupData->Cnd003DCReqPtnMask;
	SbCfg->Cnd003DCDataPtn = SetupData->Cnd003DCDataPtn;
	SbCfg->Cnd003DCDataPtnMask = SetupData->Cnd003DCDataPtnMask;
	SbCfg->Cnd003DCPlusTimer = SetupData->Cnd003DCPlusTimer;
	SbCfg->Cnd003DCStartTimer = SetupData->Cnd003DCStartTimer;
	SbCfg->Cnd003DCPeriodTimer = SetupData->Cnd003DCPeriodTimer;
	SbCfg->Cnd003DCReqCnt = SetupData->Cnd003DCReqCnt;
	SbCfg->Cnd003DCOutput = SetupData->Cnd003DCOutput;

	NbCfg->Cnd003DC = SetupData->Cnd003DC;			//DBGCAP
	NbCfg->Cnd003DCModule = SetupData->Cnd003DCModule;
	NbCfg->Cnd003DCTrans = SetupData->Cnd003DCTrans;
	NbCfg->Cnd003DCCfgPath = SetupData->Cnd003DCCfgPath;
	NbCfg->Cnd003DCChannel = SetupData->Cnd003DCChannel;
	NbCfg->Cnd003DCDbg0Mask = SetupData->Cnd003DCDbg0Mask;
	NbCfg->Cnd003DCDbg1Mask = SetupData->Cnd003DCDbg1Mask;
	NbCfg->Cnd003DCTriggerMode = SetupData->Cnd003DCTriggerMode;
	NbCfg->Cnd003DCReqPtn = SetupData->Cnd003DCReqPtn;
	NbCfg->Cnd003DCReqPtnMask = SetupData->Cnd003DCReqPtnMask;
	NbCfg->Cnd003DCDataPtn = SetupData->Cnd003DCDataPtn;
	NbCfg->Cnd003DCDataPtnMask = SetupData->Cnd003DCDataPtnMask;
	NbCfg->Cnd003DCPlusTimer = SetupData->Cnd003DCPlusTimer;
	NbCfg->Cnd003DCStartTimer = SetupData->Cnd003DCStartTimer;
	NbCfg->Cnd003DCPeriodTimer = SetupData->Cnd003DCPeriodTimer;
	NbCfg->Cnd003DCReqCnt = SetupData->Cnd003DCReqCnt;  
	NbCfg->Cnd003DCOutput = SetupData->Cnd003DCOutput;

	NbCfg->CND003PLLPCIEASSCEn = SetupData->CND003PLLPCIEASSCEn;
	NbCfg->CND003PLLPCIEASSCMagnitude = SetupData->CND003PLLPCIEASSCMagnitude;
	NbCfg->CND003CKGSRCPLLPCIEASSCSpread = SetupData->CND003CKGSRCPLLPCIEASSCSpread;
	NbCfg->CND003PLLPCIEBSSCEn = SetupData->CND003PLLPCIEBSSCEn;
	NbCfg->CND003PLLPCIEBSSCMagnitude = SetupData->CND003PLLPCIEBSSCMagnitude;
	NbCfg->CND003CKGSRCPLLPCIEBSSCSpread = SetupData->CND003CKGSRCPLLPCIEBSSCSpread;
	NbCfg->CND003USBCLKSEL = SetupData->CND003USBCLKSEL;
	NbCfg->CND003SATACLKSEL = SetupData->CND003SATACLKSEL;
	NbCfg->CND003_RPCIE_PU_IO = SetupData->CND003_RPCIE_PU_IO;
	NbCfg->CND003_RPCIE_TNO_IO = SetupData->CND003_RPCIE_TNO_IO;

//JNY Porting IOE TOP/PCIE -E

#ifdef IOE_EXIST
  //CND003 SATA
  SbCfg->IOESataEn               =SetupData->IOESataEn;  
  SbCfg->IOESataCfg              =SetupData->IOESataCfg;
  SbCfg->IOESataGen              =SetupData->IOESataGen;
  SbCfg->IOEAHCIGen              =SetupData->IOEAHCIGen;
  SbCfg->IOEAHCI_MSI             =SetupData->IOEAHCI_MMSI;
  SbCfg->IOEAHCI_MSIX            =SetupData->IOEAHCI_MSIX;
  SbCfg->IOESataCapSelect        =SetupData->IOESataCapSelect;
  SbCfg->IOESataHIPMEn           =SetupData->IOESataHIPMEn;
  SbCfg->IOESataHpcpEn           =SetupData->IOESataHpcpEn;
  SbCfg->IOESataALPMEn           =SetupData->IOESataALPMEn;
  SbCfg->IOEFunctionLevelEnabled =SetupData->IOEFunctionLevelEnabled;

  //IOE_GNIC
  SbCfg->IOEGnicEn = SetupData->IOEGnicEn ;
  SbCfg->IOEGnicVEEPROM = SetupData->IOEGnicVEEPROM;
  SbCfg->IOEGnicTXDCS = SetupData->IOEGnicTXDCS ;
  SbCfg->IOEGnicTXDC0 = SetupData->IOEGnicTXDC0 ;
  SbCfg->IOEGnicTXDC1 = SetupData->IOEGnicTXDC1 ;
  SbCfg->IOEGnicTXDC2 = SetupData->IOEGnicTXDC2 ;
  SbCfg->IOEGnicRXDCS = SetupData->IOEGnicRXDCS ;
  SbCfg->IOEGnicRXDC0 = SetupData->IOEGnicRXDC0 ;
  SbCfg->IOEGnicRXDC1 = SetupData->IOEGnicRXDC1 ;
  SbCfg->IOEGnicRXDC2 = SetupData->IOEGnicRXDC2 ;
  SbCfg->IOEGnicDPDC64 = SetupData->IOEGnicDPDC64 ;
  SbCfg->IOEGnicDPDC128 = SetupData->IOEGnicDPDC128 ;
  SbCfg->IOEGnicDPDC256 = SetupData->IOEGnicDPDC256 ;
  SbCfg->IOEGnicForceSizeEn = SetupData->IOEGnicForceSizeEn ;
  SbCfg->IOEGnicForceRDR = SetupData->IOEGnicForceRDR ;
  SbCfg->IOEGnicForcePayload = SetupData->IOEGnicForcePayload ;
  SbCfg->IOEGnicEventCtrl = SetupData->IOEGnicEventCtrl ;
  SbCfg->IOEGnicPendingCtrl = SetupData->IOEGnicPendingCtrl ;
  SbCfg->IOEGnicMsiCtrl = SetupData->IOEGnicMsiCtrl ;
  SbCfg->IOEGnicMsiXCtrl = SetupData->IOEGnicMsiXCtrl ;
  SbCfg->IOEGnicD0PME = SetupData->IOEGnicD0PME ;

   // IOE_SPE value
  NbCfg->IOESPEValue = SetupData->IOESPEValue;
  SbCfg->IOESPEValue= SetupData->IOESPEValue;
  
  NbCfg->IOEXSPEValue = SetupData->IOEXSPEValue;
  NbCfg->IOEXp1D0F0SPEValue = SetupData->IOEXp1D0F0SPEValue;
  NbCfg->IOEXp1D1F0SPEValue = SetupData->IOEXp1D1F0SPEValue;
  NbCfg->IOEXp1D2F0SPEValue = SetupData->IOEXp1D2F0SPEValue;
  NbCfg->IOEXp1D3F0SPEValue = SetupData->IOEXp1D3F0SPEValue;
  NbCfg->IOEXp1D4F0SPEValue = SetupData->IOEXp1D4F0SPEValue;
  NbCfg->IOEXp1D5F0SPEValue = SetupData->IOEXp1D5F0SPEValue;
  NbCfg->IOEXp1D6F0SPEValue = SetupData->IOEXp1D6F0SPEValue;
  NbCfg->IOEXp1D7F0SPEValue = SetupData->IOEXp1D7F0SPEValue;
  NbCfg->IOEXp1D8F0SPEValue = SetupData->IOEXp1D8F0SPEValue;
  NbCfg->IOEXp2D0F0SPEValue = SetupData->IOEXp2D0F0SPEValue;
  NbCfg->IOEXp3D0F0SPEValue = SetupData->IOEXp3D0F0SPEValue;
  NbCfg->IOEEphySPEValue = SetupData->IOEEphySPEValue;
  NbCfg->IOEMmioISBSPEValue = SetupData->IOEMmioISBSPEValue;
  
  SbCfg->IOEGNICSPEValue= SetupData->IOEGNICSPEValue;  
  SbCfg->IOESATASPEValue= SetupData->IOESATASPEValue;
  SbCfg->IOEXHCISPEValue= SetupData->IOEXHCISPEValue;
  SbCfg->IOEEHCISPEValue= SetupData->IOEEHCISPEValue;
  SbCfg->IOEUHCISPEValue= SetupData->IOEUHCISPEValue;

#endif



  if(BootMode == BOOT_IN_RECOVERY_MODE){
    SbCfg->SataEn = TRUE;
    SbCfg->SataCfg = SATA_CFG_IDE;
    //SbCfg->UsbModeSelect = USB_MODE_SEL_MODEB;
  } 
    
  SetAfterPowerLoss(SetupData->AfterPowerLoss);
  HandleRtcWake(BootMode, SetupData);
  DEBUG((EFI_D_ERROR, "DLA_HandleRtcWake() pass\n"));
  
#ifdef ZX_SECRET_CODE
  PcdSet8(PcdCpuCoreEnabled, SetupData->KillAp);
  NbCfg->C2P2FlushC2M = SetupData->C2P2FlushC2M;
//YKN-2016112201 -s
//YKN-2016112402 -s
/*
{
  UINT16 FidVid;
  UINT64 ALTPSRUP;

  if(SetupData->FixCpuRatio == 18)
  {
    FidVid = 0x2463;
    ALTPSRUP = 0x2463246300000001;
  }else if(SetupData->FixCpuRatio == 20) {
     FidVid = 0x2863;
     ALTPSRUP =  0x2863286300000001;
   }else {
    FidVid = 0x1057;
    ALTPSRUP=0x1057105700000001;
   }
   
    AsmWriteMsr64(0x1440, 0x80000000|FidVid);

	AsmWriteMsr64(0x1441, 0x88000000|FidVid);
    AsmWriteMsr64(0x1442, 0);
    AsmWriteMsr64(0x1443, 0);
    AsmWriteMsr64(0x1444, 0);
    AsmWriteMsr64(0x80000198, ALTPSRUP); 
    AsmWriteMsr64(0x1404, AsmReadMsr64(0x1404)|0x04);
    AsmWriteMsr64(0x1404, AsmReadMsr64(0x1404)|0x01);
	DEBUG((EFI_D_ERROR, "af AsmWriteMsr64(0x1404) pass\n"));
    AsmWriteMsr64(0x199, FidVid);
	DEBUG((EFI_D_ERROR, "af AsmWriteMsr64(0x199) pass\n"));
}
*/
//YKN-2016112402 -e
 //YKN-2016112201 -e
#endif
  #ifdef ZX_SECRET_CODE
  PcdSet8(PcdFMS107B0,SetupData->FMS107B0);
  #endif
  return Status;  
}  

