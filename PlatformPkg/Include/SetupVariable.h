/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SetupVarible.h

Abstract:
  The declaration header file for SYSTEM_CONFIGURATION_GUID.

Revision History:

$END--------------------------------------------------------------------

**/

#ifndef _SETUP_VARIABLE_H
#define _SETUP_VARIABLE_H

#define TSE_SETUP_ENTER_GUID \
  { 0x71202EEE, 0x5F53, 0x40d9, 0xAB, 0x3D, 0x9E, 0x0C, 0x26, 0xD9, 0x66, 0x57 }

#define EFI_IFR_FRONT_PAGE_GUID \
  { 0xe58809f8, 0xfbc1, 0x48e2, { 0x88, 0x3a, 0xa3, 0xf, 0xdc, 0x4b, 0x44, 0x1e } }


#ifdef CHX002_PXP
#define DEFAULT_BOOT_TIMEOUT                          0
#else
#define DEFAULT_BOOT_TIMEOUT                          1
#endif
#define PASSWORD_MAX_SIZE                             20
#define PLATFORM_SETUP_VARIABLE_NAME                  L"Setup"
#define SETUP_VOLATILE_VARIABLE_NAME                  L"SetupVolatileData"
#define SETUP_TEMP_VARIABLE_NAME                      L"SetupTemporary"

#define PLATFORM_SETUP_VARIABLE_GUID \
  { \
     0xEC87D643, 0xEBA4, 0x4BB5, 0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9 \
  }

extern EFI_GUID gPlatformSetupVariableGuid;


#define RTC_WAKE_VAL_SINGLE_EVENT    1
#define RTC_WAKE_VAL_DAILY_EVENT     2
#define RTC_WAKE_VAL_WEEKLY_EVENT    3
#define RTC_WAKE_VAL_USER_DEFINED    4
#define RTC_WAKE_VAL_PERIOD_MINUTES    5
#define RTC_WAKE_VAL_PERIOD_SECONDS    6
#define RTC_WAKE_VAL_DISABLE         0

#define SMRR_TYPE_DISABLE            0
#define SMRR_TYPE_WT                 1
#define SMRR_TYPE_WB                 2

#define AFTER_POWER_LOSS_OFF         0
#define AFTER_POWER_LOSS_PREVIOUS    1
#define AFTER_POWER_LOSS_ON          2

#define DISPLAY_PRIMARY_PCIE         0
#define DISPLAY_PRIMARY_IGD          1

#pragma pack(1)

typedef struct {
  UINT8  Revision;

  // Main Page
  UINT8  Language;

  UINT8  Numlock;
  UINT8  BootTimeout;
  UINT8  OpromMessage;

  UINT8  UCREnable;
  UINT8  TerminalType;
  UINT8  SerialPortSelect;
  UINT8  SerialBaudrate;

  UINT8  PrimaryDisplay;
//  UINT8  UmaFbSize;	

  UINT8  DisplayBootMode;
  UINT8  Csm;
  UINT8  BootModeType;
  
  //
  //Controls whether to display the hotkey 
  //
  UINT8  LegacyUsbEnable;
  UINT8  XhcSupport; 
  //UINT8 IOVEnable;//NBconfig

 // UINT8 Pmon;
  UINT8 EpsEnable;
	UINT8 SMRREnable;
#ifdef ZX_SECRET_CODE
  UINT8 KillAp;
  UINT8 FixCpuRatio;
  //UINT8	PartialResetEn;
  UINT8 MCASupport;
  UINT8 C2P2FlushC2M;
  UINT8 FMS107B0;
#endif  
  ////
  UINT8 IOVEnable;
  UINT8	IOVQIEnable;
  UINT8	IOVINTREnable;
  UINT8  CpuCState;
  UINT8 ConditionalC4;
  UINT8 C5Control;
  UINT8 C5L2sizingControl;
  UINT8 CxIntFilterControl;
  UINT8 CxEEnable;
  UINT8 RoundRobin;
  UINT8 TPREcho;
  UINT8  Msr3A;
  UINT8  TSC_Deadline_mode;
  UINT8  ExecuteDisable;
  ////
  //YKN-20160627 -S
  //UINT8  CpuCoreCount;
  //UINT8  CpuC1E;	
  //UINT8  CpuVT;
  //YKN-20160627 -E
  /// 
  // DRAM
   UINT8 DramClk;
  
   //UINT8 IoTimingMethod;
   UINT8 RxIoTimingMethod;
   UINT8 DQSIByRank;
   UINT8 TxIoTimingMethod; 
   UINT8 LimitRankSize;
   UINT8 DqsiAdjustEn;//DLA_ADD_0406
   UINT8 BankIntlv;
   UINT8 MemoryChipODTDebug;	
   UINT8 MemoryChipODTWRDebug;
   UINT8 MemoryChipODTParkDebug;
   UINT8 VRInlv;
  // UINT8 ChannelIntlv;
   UINT8 CHdecode;
   UINT8 BankGroupBit0Decode;
   //UINT8 RA0Select;
   //UINT8 RA1Select;
   UINT8 BASelect;
   //UINT8 BaScmb;
   
   UINT8 CRCParRetryEn;
   UINT8 ParErrControl;
   UINT8 CRCErrControl;
   UINT8 EccRetry;
   UINT8 EccPatrolScrub;
   // UINT8 SwapByte78;//DLA_Rename_0406
   UINT8 SwapChAB; //DLA_Rename_0406
   UINT8 ScanIOTiming; //LGE Add for scan dram timing
   UINT8 DramECC;
   //UINT8 ASPDI;//DLA_ADD_0406
   UINT8 VDD; //DLA_Rename_0406
   UINT8 DataScmb;
   //UINT8 DQMPinOut;
   UINT8 RequestInorder;  
   UINT8 RFW2W;
   UINT8 RFR2R;
   UINT8 RFR2W;
   UINT8 RDFW2R;
   UINT8 BL; //DLA_Rename_0406
   UINT8 CmdRate;
   UINT8 VGAShareMemory;
   UINT8 DramInitMethod;
   UINT8 DramSelfRefresh;
   UINT8 DynamicCKE;
   UINT8 RemapEn;
   UINT8 DramFastBoot;
	UINT8 DramRxTxTimingHardCode;	
	UINT8 TNIHighPulse;//daisy add
   UINT8 CRCEn;
   UINT8 CAParEn;
   UINT8 CAParPerEn;
   UINT8 WPREA;
   UINT8 RPREA;
   UINT8 CALEn;
   UINT8 ACTimingOption;
   UINT8 DramVoltageControl;
   UINT8 DramCL;
   UINT8 DramTrcd;
   UINT8 DramTrp;
   UINT8 DramTras;
   UINT8 RxVref;
   UINT16 TxVref;
   UINT8 TxVrefAllByte;
   UINT8 RRankRDelay;
   UINT8 RRankWDelay;
   ////
   ////
   // Graphic
   UINT8 UMAEn;
   UINT8 DisableHDAC1;
   UINT8 DisableHDAC2;
   UINT8 SelectDisplayDevice;
   UINT8 DP1;
   UINT8 DP2;
   UINT8 DVO; 
   UINT8 CRT;
   UINT8 ECLKCtrl;
   UINT16 ECLKFreq;
   UINT8 VCLKCtrl;
   UINT16 VCLKFreq;
   UINT8 ICLKCtrl;
   UINT16 ICLKFreq;
   UINT8  DP1SSCEn;
   UINT8  DP2SSCEn;
   UINT8 DP1SSCMode;
   UINT8 DP2SSCMode;

   UINT8 LinuxVideoIP;
   UINT8 PWM0OutputEn;
   UINT8 PWM1OutputEn;
   UINT8 PWM0Frequency;
   UINT8 PWM1Frequency;
   UINT16 PWM0DutyCycle;
   UINT16 PWM1DutyCycle;
   ///
   UINT8 PcieFPGAMode;	
   ///
   //UINT8 SwapByte7_Byte8;
   //UINT8 SwapChannelA_ChannelB;
   //UINT8 ECCClearMemory;
   //UINT8 VDDSel;
   //UINT8 BurstLength;

   ///
  UINT8 ApplicationModeSelection;  
  UINT8 NBSPEValue;
  UINT8 D0F0SPEValue;
  UINT8 D0F1SPEValue;
  UINT8 D0F2SPEValue;
  UINT8 D0F3SPEValue;
  UINT8 D0F4SPEValue;
  UINT8 D0F5SPEValue;
  UINT8 D0F6SPEValue;
  ///
//  UINT8 D2F0SPEValue;
  UINT8 D3F0SPEValue;
  UINT8 D3F1SPEValue;	 //PE1
  UINT8 D3F2SPEValue;	 //PE2
  UINT8 D3F3SPEValue;	 //PE3
  ///
  UINT8 D4F0SPEValue;
  UINT8 D4F1SPEValue;	 //PE5  
  UINT8 D5F0SPEValue;
  UINT8 D5F1SPEValue;	 //PE7    
  ///
  UINT8 RCRBHSPEValue;
  UINT8 PCIEEPHYSPEValue;
  UINT8 D1F0SPEValue;
  UINT8 D8F0SPEValue;
  UINT8 D9F0SPEValue;

  ////
  UINT8 SBSPEValue;
  UINT8 SATASPEValue;
  UINT8 VARTSPEValue;
  UINT8 ESPISPEValue;  
  UINT8 BusCtrlSPEValue;
  UINT8 PMUSPEValue;
  UINT8 PCCASPEValue;
  UINT8 HDACSPEValue;	 
  UINT8 SPISPEValue; 
  UINT8 SOCXHCISPEValue;
  UINT8 SOCEHCISPEValue;
  UINT8 SOCUHCISPEValue; 

	 
	 //NB DEBUG
	  UINT8 CpuClockControl;
	//  UINT8 CpuSSCControl;
	//  UINT8 PcieClockControl;
	 //UINT8 PcieSSCControl;
	 //UINT8 SkwClockControl;
	 //UINT8 ChipsetCpuBclkControl;
	 //UINT8 CPUSlewRate;
	 // UINT8 OnBoard1394ClockControl;
   
	  //UINT8 CpuBclkControl;
	  UINT8 SERRNBControl;
	  UINT8 LoopERSMIControl;
	  UINT8 V4IFErrControl;
	  UINT8 DRAMErrControl;
//	  UINT8 PEGErrControl;
	  UINT8 PE0ErrControl;
	  UINT8 PE1ErrControl;
	  UINT8 PE2ErrControl;
	  UINT8 PE3ErrControl;
	  UINT8 PE4ErrControl;
	  UINT8 PE5ErrControl;
      UINT8 PE6ErrControl;
      UINT8 PE7ErrControl;    

	 //UINT8 DID_VID_LOCK;
	 UINT8 VID_SEL;
	
	//UART
	 UINT8 OnChipUartMode;
	 UINT8 UartModuleSelection;
	 UINT8 Uart0Enable;
	 UINT8 Uart0IoBaseSelection;
	 UINT8 Uart0IRQSelection;
	 UINT8 Uart1Enable;
	 UINT8 Uart1IoBaseSelection;
	 UINT8 Uart1IRQSelection;
	 UINT8 Uart2Enable;
	 UINT8 Uart2IoBaseSelection;
	 UINT8 Uart2IRQSelection;
	 UINT8 Uart3Enable;
	 UINT8 Uart3IoBaseSelection;
	 UINT8 Uart3IRQSelection;  
	 UINT8 UartFLREn; 	 

	 //RAIDA
	 UINT8	RAIDA0En;
	 UINT8	RAIDA1En;
	   
	 //debug signal
	 UINT8 DebugMode;
	 UINT8 DebugOutputSelect;
	// UINT8 OutPadSelect;
	 UINT8 DebugSignalSelect0;
	 UINT8 DebugSignalSelect1;
	 UINT8 DebugModuleSelect0;
	 UINT8 DebugModuleSelect1;
	 UINT8	 NBGroup0TopSelcet;
	 UINT32  NBGroup0ModuleSubSelect1;
	 UINT32  NBGroup0ModuleSubSelect2;
	 UINT8	 NBGroup1TopSelcet;
	 UINT32  NBGroup1ModuleSubSelect1;
	 UINT32  NBGroup1ModuleSubSelect2;
   
	 UINT8	 DebugSBSelectByNB;
	 UINT8	 SBTopDbgMux1;	 
	 UINT32  SBGroup0ModuleSubSelect;
	 UINT8	  SBTopDbgMux2;
	UINT32	SBGroup1ModuleSubSelect;
	
	 UINT8	 TOPGroup0TopSelcet;
	 //UINT8	 TOPGroup0XhciModuleSelcet;
	 UINT8  TOPGroup0XhciModule0Selcet;
	 UINT8  TOPGroup0XhciModule1Selcet;
	 UINT16 TOPGroup0XhciGroup0Selcet;
	 UINT16 TOPGroup0XhciGroup1Selcet;
	 UINT8	TOPGroup0XhciMCUSelcet;
	 UINT16	TOPGroup0SUSXhciSelcet;
	 UINT32	TOPGroup0ModuleSubSelcet;
	 UINT8	 TOPGroup1TopSelcet;
	 UINT8  TOPGroup1XhciModule2Selcet;
	 UINT8  TOPGroup1XhciModule3Selcet;
	 UINT16 TOPGroup1XhciGroup2Selcet;
	 UINT16 TOPGroup1XhciGroup3Selcet;
	 UINT8	TOPGroup1XhciMCUSelcet;
	 UINT16	TOPGroup1SUSXhciSelcet;

	 //UINT8	 TOPGroup1XhciModuleSelcet;
	 UINT32	TOPGroup1ModuleSubSelcet;
 	 UINT8	 FSBCGroup0TopSelcet;
	 UINT8	 FSBCGroup1TopSelcet;  
	 
#ifdef ZX_SECRET_CODE
	 UINT8	 CPU_MASTER_FSBC_EN;//GRW-add-1127
	 UINT8	 CPU_SLAVE_FSBC_EN;//GRW-add-1127
	 UINT8	 CPU_TRACER_EN;//GRW-add-1127
	 UINT64	 CPU_TRACER_INSTRUCTION_INTERVAL;//GRW-add-1127
	 UINT8	 CPU_TRACER_DUMP_MEMORY_BASE;//GRW-add-1127
	 UINT8	 CPU_FSBC_PCIE_ON;
	 UINT8	 CPU_FSBC_TOPCIE;
	 UINT8	 CPU_FSBC_DRAM_ON;
	 UINT8	 CPU_FSBC_STREAM_EN;
	 UINT8	 CPU_FSBC_P2CC2PONLY;
	 UINT8	 CPU_FSBC_SOCCAP_ON;//GRW-add-1127	
	 UINT8	 CPU_FSBC_MISSPACKE_EN;
	 UINT8	 CPU_FSBC_TIGPULSE_EN;
	 UINT8	 CPU_FSBC_IFSBCSTP_EN;
#endif
	 UINT64   CPU_FSBC_DBG_04;
	 UINT64  CPU_FSBC_DBG_05;
	 UINT64  CPU_FSBC_DBG_06;
	 UINT64  CPU_FSBC_DBG_07;
	 UINT64  CPU_FSBC_DBG_08;
	 UINT64   CPU_FSBC_DBG_09;
	 UINT64  CPU_FSBC_DBG_0A;
	 UINT64  CPU_FSBC_DBG_0B;
	 UINT64  CPU_FSBC_DBG_0C;
	 UINT8	 SOCCAP_Mem_Size;

   	 UINT8	 SB_HS_DBG_SEL;
	 UINT8	 SB_HS_PCIE_PORT_SEL;	  
	 UINT8   SB_HS_DBG_CH_SEL;  
	//VDD OFF debug signal
	UINT8	VDD_OFF_EN;
	UINT8	VDD_OFF_Module_Sel;
	UINT8	VDD_OFF_Group_Sel;
   //SATA
   UINT8	 SataEn;
   UINT8	 SataCfg;
   UINT8	 IDEGen;
   UINT8	 IDECapSelect;
   UINT8	 IDEHIPMEn;
   UINT8	 AHCIGen;
   UINT8	 AHCIMSI;
   UINT8	 AHCIMSIX;
   UINT8	 AHCIHotplugEn;
   UINT8	 AHCIALPMEn;
   UINT8	 FuncLevelResetEn;
   UINT8     GBFlushendEn;
   //USB
   UINT8   UsbModeSelect;
   UINT8   UsbS4WakeupCtrl;
   UINT8   UsbOCCtrl;
   UINT8   USBCFLRCtrl;
   UINT8   XhcMcuDmaPathCtrl;
   UINT8   XhcTRBCacheBypassCtrl;
   UINT8   XhcBurstCtrl;
   UINT8   XhcPerfModeCtrl;
   UINT8   XhcU1U2Ctrl;
   UINT8   XhcMsiFlushCtrl;
   UINT8   XhcUartCtrl;
   UINT8   XhcFLRCtrl;
   UINT8   XhcRTD3Ctrl;
   UINT8   XhcC4BlockCtrl;
   UINT8   XhcEITRNCtrl;
   	//IOE_USB
	//UINT8   IOEUsbModeSelectMode;
	UINT8	IOEUsbModeSelect;
	UINT8	IOETRBCacheBypass;
    UINT8   IOEXhciOutBurstEn;
    UINT8   IOEXhciMaxBurstSize;
	UINT8	IOEUSBCFLRControl;
	UINT8	IOEXHCIFLRControl;
	UINT8	IOERTD3Control;
	UINT8	IOEUsbS4Wakeup;
    UINT8   IOEUsbC4Block;
    UINT8   IOEUsb10GSupport;
    UINT8   IOEUsbOCEn;
	UINT8   IOEUsbEITRN;
   //HDAC
   UINT8 GoNonSnoopPath;
   UINT8 HDACFLREn;
   
   //SNMI
   UINT8 IsoLPC;
   UINT8 IsoVART;
   UINT8 IsoAZALIA;
   UINT8 IsoESPI;
   UINT8 IsoSPI;
   UINT8 IsoAPIC;

   //PMU_ACPI
   UINT8 C3BusMasterIdleTimer;
   UINT8 C4BusMasterIdleTimer;
   UINT8 FixedFreeCxLatency;
   UINT8 ShortC3C4Mode;
   UINT8 DPSLPtoSLP;
   UINT8 VRDSLPtoDPSLP;
   UINT8 DynamicT05;
   UINT8 SBDynamicClkControl;
   UINT8 TMRCDynamicClkControl;
   UINT8 MobileCenterControl;
   UINT8 ACAdapterControl;
   UINT8 CpuFanStartTemperature;
   UINT8 MsiSupport;
   
   UINT8 eBMCSettleTime;
   UINT8 KBDCLegacySelControl;
   UINT8 INTCLegacySelControl;
   UINT8 DMACLegacySelControl;
   UINT8 TMRCLegacySelControl;
   
   //OTHERS
	UINT8 EnableMultimediaTimer;
	UINT8 MultimediaTimerMode; 
	UINT8 EnableMultimediaTimerMsi;
	UINT8 WatchDogTimer;
	UINT8 WatchDogTimerRunStop;
	UINT8 WatchDogTimerAction;
	UINT8 WatchDogTimerCount;
	UINT8 KBMouseWakeupControl;
	UINT8 SMBusControllerUnderOS;
	UINT8 SMBusHostClockFrequencySelect;
	UINT8 SMBusHostClockFrequency;
	UINT8 SPIBus0ClockSelect;
    UINT8 ESPI;
//HYL-2018062901-start
#ifdef ZX_SECRET_CODE
    UINT8 WDTClear;	
#endif
//HYL-2018062901-end 

	//JYZ_ADD_SVID
	UINT8 SVIDMVCLKControl;
	UINT8 SVIDMPeriodicIoutControl;
	UINT8 SVIDMC3Control;
	UINT8 SVIDMC4C5Control;
	UINT8 VRMStableBD;
	UINT8 SVIDMC4SetPS;
	UINT8 CPU2SVIDCmdGate;
	UINT8 CPUPStateTOCounter;
	UINT8 CPUPStateSetVIDDone;
	UINT8 VRM0VIDFSSelect;
	UINT8 SVIDMC4SetPSSel;
	UINT8 VRM0IoutTimer;
	UINT8 SVIDMC4DecayOP;
	UINT8 SVIDMC4SetVIDSel;
	   
   //DLA_ADD_0406_E
  
  //Controls whether to display the hotkey 
  //
  UINT8  VideoDualVga;  
  UINT8  VideoPrimaryAdapter;  

  UINT8  AfterPowerLoss;
  UINT8  WakeOnLan;  
  UINT8  WakeOnRTC;  
  EFI_HII_DATE  RTCWakeupDate;
  EFI_HII_TIME  RTCWakeupTime;
  UINT8  UserDefMon;
  UINT8  UserDefTue;
  UINT8  UserDefWed;
  UINT8  UserDefThu;
  UINT8  UserDefFri;
  UINT8  UserDefSat;
  UINT8  UserDefSun;
  UINT8  AlarmWeekDay;
  UINT8	 RTCWakeupTimeMinuteIncrease;
  UINT8	 RTCWakeupTimeSecondIncrease;
  UINT8  OnboardLan;
  UINT8  LegacyPxeBoot;
  UINT8  UefiPxeIPV4;
  UINT8  UefiPxeIPV6;

  UINT8  ObAudioEn;
  UINT8  AcpiSleepStatus;

  //MCA
  UINT8 Mca_Msmi_En;
  UINT8 Mca_Csmi_En;

  //DLA_ADD_S
  UINT8 SPECIALLY_SI_SETTING;
  UINT8 PcieRoutingCtrl;
//  UINT8 PEG_Msgc2PcieIntx;
  UINT8 PE0_Msgc2PcieIntx;
  UINT8 PE1_Msgc2PcieIntx;
  UINT8 PE2_Msgc2PcieIntx;	  
  UINT8 PE3_Msgc2PcieIntx;
  UINT8 PE4_Msgc2PcieIntx;
  UINT8 PE5_Msgc2PcieIntx;
  UINT8 PE6_Msgc2PcieIntx;
  UINT8 PE7_Msgc2PcieIntx;
  
//  UINT8 PEG_PcieIntx2Nb2sbIntx;
  UINT8 PE0_PcieIntx2Nb2sbIntx;
  UINT8 PE1_PcieIntx2Nb2sbIntx;
  UINT8 PE2_PcieIntx2Nb2sbIntx;
  UINT8 PE3_PcieIntx2Nb2sbIntx;
  UINT8 PE4_PcieIntx2Nb2sbIntx;
  UINT8 PE5_PcieIntx2Nb2sbIntx;
  UINT8 PE6_PcieIntx2Nb2sbIntx;
  UINT8 PE7_PcieIntx2Nb2sbIntx;
  
 // UINT8 MsiEnable;
  UINT8 PcieASPMBootArch;//dla_add_061107
  //UINT8 PegApciIrq;
  UINT8 Pe6ApciIrq;
  UINT8 Pe7ApciIrq;  
  //DLA_ADD_E

  //reset link when fail
  UINT8 PcieRst;
//  UINT8 PcieRstPEG;
  UINT8 PcieRstPE0;
  UINT8 PcieRstPE1;
  UINT8 PcieRstPE2;
  UINT8 PcieRstPE3;
  UINT8 PcieRstPE4;
  UINT8 PcieRstPE5;
  UINT8 PcieRstPE6;
  UINT8 PcieRstPE7;
  //rp 
  UINT8 PcieRP;
//  UINT8 PciePEG;
  UINT8 PciePE0;
  UINT8 PciePE1;
  UINT8 PciePE2;
  UINT8 PciePE3;
  UINT8 PciePE4;
  UINT8 PciePE5;
  UINT8 PciePE6;
  UINT8 PciePE7;
  //
  UINT8 PcieHotReset;
  UINT8 PcieForceLinkWidth;
  //Fmw related
  UINT8 PEMCU_LoadFW_WhenBoot;
  UINT8 PEMCU_RstSys_WhenFail;
  UINT8 SelectableDeEmphasis;
  //Link Speed
  UINT8 PcieLinkSpeed;
//  UINT8 PciePEGLinkSpeed;
  UINT8 PciePE0LinkSpeed;
  UINT8 PciePE1LinkSpeed;
  UINT8 PciePE2LinkSpeed;
  UINT8 PciePE3LinkSpeed;
  UINT8 PciePE4LinkSpeed;
  UINT8 PciePE5LinkSpeed;
  UINT8 PciePE6LinkSpeed;
  UINT8 PciePE7LinkSpeed;
  //OBFF 
  UINT8 PcieOBFFCtrl_PCIE;
  UINT8 PcieOBFFCtrl_PMU;
  //EQ related
  UINT8 PcieEQ;
	  	  UINT8 EQTxPreset;
		UINT8 PcieEQPE0;
		UINT8 PcieEQPE1;
		UINT8 PcieEQPE2;
		UINT8 PcieEQPE3;
		UINT8 PcieEQPE4;
		UINT8 PcieEQPE5;
		UINT8 PcieEQPE6;
		UINT8 PcieEQPE7;
		UINT8 PcieEMEQDebug;
		UINT8 PcieEMEQScanTime;	  
			UINT8 PcieEQTS2;
			UINT8 PcieCrsMech;
			UINT8 PcieOptimalTLS;
		  UINT16 PcieEqCtlOrgValL0;
		  UINT16 PcieEqCtlOrgValL1;
		  UINT16 PcieEqCtlOrgValL2;
		  UINT16 PcieEqCtlOrgValL3;
		  UINT16 PcieEqCtlOrgValL4;
		  UINT16 PcieEqCtlOrgValL5;
		  UINT16 PcieEqCtlOrgValL6;
		  UINT16 PcieEqCtlOrgValL7;
		  UINT16 PcieEqCtlOrgValL8;
		  UINT16 PcieEqCtlOrgValL9;
		  UINT16 PcieEqCtlOrgValL10;
		  UINT16 PcieEqCtlOrgValL11;
		  UINT16 PcieEqCtlOrgValL12;
		  UINT16 PcieEqCtlOrgValL13;
		  UINT16 PcieEqCtlOrgValL14;
		  UINT16 PcieEqCtlOrgValL15;

		  UINT8 PciePHYA_SSC_EN;
		  UINT8 PciePHYB_SSC_EN;

  //Else
  UINT8 MaxPayloadSize;
  UINT8 PcieASPM;
  UINT8 RelaxedOrder;
  UINT8 ExtTag;
  UINT8 ExtSync;
  UINT8 NoSnoop;
  
  //pcie
  UINT8 Pci64;           // winddy +
  UINT8 Pci64Location;
	
//NB DEBUG
//LGE20160308-START
	 UINT8 HIFErrControl;
	///	
  UINT8	ZxeDualSocket;

  UINT8 ShellEn;

//IOE_PCIE_PTN
//JNY add for IOE -S
	UINT8 Cnd003PhyCfg;
	UINT8 Cnd003PhyACfg;
	UINT8 Cnd003PhyBCfg;
	UINT8 Cnd003EpCap;
	UINT8 Cnd003CapPEA0;
	UINT8 Cnd003CapPEA1;
	UINT8 Cnd003CapPEA2;
	UINT8 Cnd003CapPEA3;
	UINT8 Cnd003CapPEA4;
	UINT8 Cnd003CapPEB0;
	UINT8 Cnd003CapPEB1;
	UINT8 Cnd003CapPESB;  //CJW_IOE-add
	UINT8 IoeDnPortCtl;
	UINT8 IoePEA0Ctl;
	UINT8 IoePEA1Ctl;
	UINT8 IoePEA2Ctl;
	UINT8 IoePEA3Ctl;
	UINT8 IoePEA4Ctl;
	UINT8 IoePEB0Ctl;
	UINT8 IoePEB1Ctl;
	UINT8 IoeDnPortPEXRST;
	UINT8 IoePEA0PEXRST;
	UINT8 IoePEA1PEXRST;
	UINT8 IoePEA2PEXRST;
	UINT8 IoePEA3PEXRST;
	UINT8 IoePEA4PEXRST;
	UINT8 IoePEB0PEXRST;
	UINT8 IoePEB1PEXRST;
	UINT8 Cnd003PcieRstTest;
	UINT8 Cnd003PEA0RstTest;
	UINT8 Cnd003PEA1RstTest;
	UINT8 Cnd003PEA2RstTest;
	UINT8 Cnd003PEA3RstTest;
	UINT8 Cnd003PEA4RstTest;
	UINT8 Cnd003PEB0RstTest;
	UINT8 Cnd003PEB1RstTest;
	UINT8 Cnd003LinkWidth;
  	UINT8 Cnd003PcieEq;
  	UINT8 Cnd003Autofill;
	UINT8 Cnd003BootErrorClear;
	UINT8 Cnd003ForceMPS128B;
	UINT8 Cnd003SwCtlDnPortLinkup;

	//IOE_TOP
	UINT8	Cnd003ModeSel;		// 0:BIOS mode   1:SPI mode
	UINT8	Cnd003HideXbctl;
	UINT8	Cnd003HideEptrfc;
	UINT8	Cnd003TCxFavor;
	UINT8	Cnd003DebugFromCpu;
	UINT8	Cnd003ModuleG0;
	UINT8	Cnd003ModuleG1;
	UINT16	Cnd003DebugG0;
	UINT16	Cnd003DebugG1;
	UINT16	Cnd003DebugG2;
	UINT16	Cnd003DebugG3;
    UINT16	Cnd003DebugG0_XHCI;
	UINT16	Cnd003DebugG1_XHCI;
	UINT16	Cnd003DebugG2_XHCI;
	UINT16	Cnd003DebugG3_XHCI;
	UINT8	Cnd003HighSpeed;
	UINT8	Cnd003DebugSuspend;
	UINT8	Cnd003HighSpeedModuleSel;
	UINT16	Cnd003HighSpeedDbgSel;
	UINT8 Cnd003CorePowerOff;
	UINT8 Cnd003DbgPadG0En;
	UINT8 Cnd003DbgPadG1En;
	UINT8 Cnd003DbgPadG2En;
	UINT8 Cnd003DbgPadG3En;

	UINT8 Cnd003UartPinEn;
#ifdef ZX_SECRET_CODE
	UINT8 Cnd003DIDVIDChoose;
#endif
	UINT8 Cnd003ChangeSIDForISB;
	UINT32 Cnd003SwDbg1;
	UINT32 Cnd003SwDbg2;
	UINT8	CND003PLLPCIEASSCEn;
	UINT8	CND003PLLPCIEASSCMagnitude;
	UINT8	CND003CKGSRCPLLPCIEASSCSpread;
	UINT8	CND003PLLPCIEBSSCEn;
	UINT8	CND003PLLPCIEBSSCMagnitude;
	UINT8	CND003CKGSRCPLLPCIEBSSCSpread;
	UINT8	CND003USBCLKSEL;
	UINT8	CND003SATACLKSEL;
	UINT16 	CND003_RPCIE_PU_IO;
	UINT16 	CND003_RPCIE_TNO_IO;
	
	UINT8 Cnd003DC;			//DBGCAP
	UINT8 Cnd003DCModule;
	UINT8 Cnd003DCTrans;
	UINT8 Cnd003DCCfgPath;
	UINT8 Cnd003DCChannel;
	UINT32 Cnd003DCDbg0Mask;
	UINT32 Cnd003DCDbg1Mask;
	UINT16 Cnd003DCTriggerMode;
	UINT32 Cnd003DCReqPtn;
	UINT32 Cnd003DCReqPtnMask;
	UINT32 Cnd003DCDataPtn;
	UINT32 Cnd003DCDataPtnMask;
	UINT32 Cnd003DCPlusTimer;
	UINT32 Cnd003DCStartTimer;
	UINT32 Cnd003DCPeriodTimer;
	UINT32 Cnd003DCReqCnt;
	UINT8 Cnd003DCOutput;
  //JNY add for IOE -E	
  
  //Add for IOE
  
  //IOE_SATA
	UINT8	IOESataEn;
	UINT8	IOESataCfg;
	//UINT8	IOESataMode;
	UINT8	IOESataGen;
	UINT8	IOEAHCIGen;
	UINT8	IOEAHCI_MMSI;
	UINT8	IOEAHCI_MSIX;
	UINT8	IOESataCapSelect;
	UINT8	IOESataHIPMEn;
	UINT8	IOESataHpcpEn;
	UINT8   IOESataALPMEn;
	UINT8	IOEFunctionLevelEnabled;
	
	//IOE_GNIC
	UINT8	IOEGnicEn;
    UINT8 	IOEGnicVEEPROM;
	UINT8	IOEGnicTXDCS;
	UINT8	IOEGnicTXDC0;
	UINT8	IOEGnicTXDC1;
	UINT8	IOEGnicTXDC2;
	UINT8	IOEGnicRXDCS;
	UINT8	IOEGnicRXDC0;
	UINT8	IOEGnicRXDC1;
	UINT8	IOEGnicRXDC2;
	UINT8	IOEGnicDPDC64;
	UINT8	IOEGnicDPDC128;
	UINT8	IOEGnicDPDC256;
	UINT8	IOEGnicForceSizeEn;
	UINT8	IOEGnicForceRDR;
	UINT8	IOEGnicForcePayload;
	UINT8	IOEGnicEventCtrl;
	UINT8	IOEGnicPendingCtrl;
	UINT8 	IOEGnicMsiCtrl;
	UINT8	IOEGnicMsiXCtrl;
	UINT8	IOEGnicD0PME;

	//IOE_SPE
	UINT8 IOESPEValue;
	UINT8 IOEXSPEValue;
	UINT8 IOEXp1D0F0SPEValue;
	UINT8 IOEXp1D1F0SPEValue;
	UINT8 IOEXp1D2F0SPEValue;
	UINT8 IOEXp1D3F0SPEValue;
	UINT8 IOEXp1D4F0SPEValue;
	UINT8 IOEXp1D5F0SPEValue;
	UINT8 IOEXp1D6F0SPEValue;
	UINT8 IOEXp1D7F0SPEValue;
	UINT8 IOEXp1D8F0SPEValue;
	UINT8 IOEXp2D0F0SPEValue;
	UINT8 IOEXp3D0F0SPEValue;
	UINT8 IOEEphySPEValue;
	UINT8 IOEMmioISBSPEValue;

	UINT8 IOEGNICSPEValue;
    UINT8 IOESATASPEValue;
	UINT8 IOEXHCISPEValue; //TTP_IOE_02
	UINT8 IOEEHCISPEValue; //TTP_IOE_02
	UINT8 IOEUHCISPEValue; //TTP_IOE_02
	
	//Add for IOE end
  
}SETUP_DATA;

typedef struct {
  UINT8  PlatId;
  UINT8  TpmFormsetPresent;
  UINT8  TcmFormsetPresent;
  UINT8  TcFormsetPresent;  
  UINT8  Tpm2FormsetPresent;
  UINT8  HdpFormsetPresent;
  UINT8  SecureBootFormsetPresent;	
} SETUP_VOLATILE_DATA;

#pragma pack()


#endif  // #ifndef _SETUP_VARIABLE_H

