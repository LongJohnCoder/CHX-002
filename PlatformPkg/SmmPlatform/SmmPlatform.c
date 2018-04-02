/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmmPlatform.c

Abstract:
  Source file for the Smm platform driver.

Revision History:

**/

#include "SmmPlatform.h"
#include <PlatS3Record.h>
#include <Library/PciLib.h>

SMMCALL_ENTRY             *mSmmCallTablePtr = NULL;
UINT16                    mAcpiBaseAddr;
SETUP_DATA                gSetupData;
EFI_SMM_VARIABLE_PROTOCOL *mSmmVariable;
EFI_ACPI_RAM_DATA         *gAcpiRam;
PLATFORM_S3_RECORD        *gS3Record;


VOID ClearDateAlarm();
VOID SleepSmiDebug(UINT8 SleepType);

/// has checked for CHX001 by Tiger. 2016-06-23
EFI_STATUS
EnableAcpiCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE, 
    (EFI_SOFTWARE_SMM_DRIVER | BYO_ACPI_ENABLE)
    );

  //if(MmioRead16(EHCI_PCI_REG(PCI_VID_REG)) != 0xFFFF){
  //  MmioOr32(EHCI_PCI_REG(SB_EHCI_LEGEXT_CAP_REG), SB_EHCI_LEGEXT_CAP_HCOS);
  //}
  
  IoWrite16(0x80, 0);

  ClearDateAlarm(); 
  
  // Disable PM sources except power button
  IoWrite16(mAcpiBaseAddr + PMIO_PM_EN, PMIO_PM_EN_PWRBTN); ///PMIO_Rx02[8] Power Button Enable
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, 0xFFFF);        ///PMIO_Rx00[15:0] Power Management Status
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN_REG, 0);      ///PMIO Rx56[15:0] GPI SCI/RESUME Enable
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN1_REG, 0);    ///PMIO Rx58[15:0] General Purpose IO SCI/Resume Enable 1
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN3_REG, 0);    ///PMIO Rx5A[15:0] General Purpose IO SCI/Resume Enable 3

  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG, 0xFFFF);  ///PMIO Rx50[15:0] GPI Change Status
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG1, 0xFFFF);///PMIO Rx52[15:0] General Purpose IO Status 1  
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG3, 0xFFFF);///PMIO Rx54[15:0] General Purpose IO Status 3 

  // Disable GP2 & GP3 SMI, Disable GP2 & GP3 timer tick
  IoAnd16(mAcpiBaseAddr + PMIO_GBLEN_REG, (UINT16)~(PMIO_GBLEN_GP2SMI | PMIO_GBLEN_GP3SMI)); ///PMIO_Rx2A[13][12] GP3/2 Timer Timeout SMI Enable
  IoWrite16(mAcpiBaseAddr + PMIO_GBLSTS_REG, PMIO_GBLSTS_GP2SMI|PMIO_GBLSTS_GP3SMI);               ///PMIO_Rx28[13][12]  GP3/2 Timer Timeout Status
  MmioWrite8(LPC_PCI_REG(LPC_GP23TIMER_CTRL_REG), LPC_GP23TIMER_CTRL_REG_DEF_VALUE);             ///LPC_Rx98[7:0] GP2 / GP3 Timer Control

  // Enable SCI
  IoOr16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, PMIO_PM1_CNT_SCI_EN);  ///PMIO_Rx04[0] SCI / SMI Select

  // USB hand off
  if (mSmmCallTablePtr) {
    mSmmCallTablePtr[__ID_EndLegacyUsb]();
  }

  return EFI_SUCCESS;
}


EFI_STATUS
DisableAcpiCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  DEBUG((EFI_D_ERROR, "AcpiOff\n"));
  IoAnd16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, (UINT16)~PMIO_PM1_CNT_SCI_EN);   // Disable SCI ///PMIO_Rx04[0] SCI / SMI Select
  return EFI_SUCCESS;
}


EFI_STATUS
S3PeiEndCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  if(gS3Record->S3Sleep == PLAT_S3_SLEEP_SLEEP){
    REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PcdGet32(PcdProgressCodeS3SuspendEnd));
    gS3Record->S3Sleep = PLAT_S3_SLEEP_RESUME;
  }  
  return EFI_SUCCESS;
}




void WaitxHCIPDU2U3 (void)
{
	//UINT32 Temp32;
	//Temp32 = MmioRead32(XHCI_PCI_REG(XHCI_FWSWMSG2_REG));
	//if (Temp32 == 0x0) MicroSecondDelay(150000);
}


//
//Trigger PME_Turn_Off Message Sending
//
VOID WaitL2L3Ready(UINT8 RootBus)
{
 	UINT8 Data8;
	UINT16 Data16;
	UINT16 I;
	UINT16 ExistRpList;
	UINT16 PortChecker = 0;

	DEBUG((EFI_D_ERROR, "[PCIE] PME_Turn_Off [%x]\n",RootBus));
	Data8 = PciRead8(PCI_LIB_ADDRESS(RootBus, 0, 5, 0xF0));	//RxF0
	Data8 |= BIT7;
	PciWrite8(PCI_LIB_ADDRESS(RootBus, 0, 5, 0xF0),Data8);	//RxF0

	//Get current exist RPs from D0F6 Scratch registers
	ExistRpList = PciRead16(PCI_LIB_ADDRESS(RootBus, 0, 6, 0x5A));
	
	if(!(ExistRpList & BIT0)){
		PortChecker |= BIT0;
	}
	if(!(ExistRpList & BIT1)){
		PortChecker |= BIT1;
	}
	if(!(ExistRpList & BIT2)){
		PortChecker |= BIT2;
	}
	if(!(ExistRpList & BIT3)){
		PortChecker |= BIT3;
	}
	if(!(ExistRpList & BIT4)){
		PortChecker |= BIT5;
	}
	if(!(ExistRpList & BIT5)){
		PortChecker |= BIT4;
	}
	if(!(ExistRpList & BIT6)){
		PortChecker |= BIT9;
	}
	if(!(ExistRpList & BIT7)){
		PortChecker |= BIT10;
	}

	//256 loops to check status 
	for(I=0;I<=0x100;I++){
		Data16=PciRead16(PCI_LIB_ADDRESS(RootBus, 0, 5, 0xA0));	//RxA0
		Data16 &= (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT9|BIT10);
		Data16 |= PortChecker;		//skip some port not exist
		if(Data16 == (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT9|BIT10))
			break;
	}
	DEBUG((EFI_D_ERROR, "[PCIE] %04xh\n",I));
 }
//
VOID ClearPMEStatus(UINT8 Bus, UINT8 Dev, UINT8 Func)
{
	UINT8 Data8;
	UINT32 Data32;

	Data32 = PciRead32(PCI_LIB_ADDRESS(Bus, Dev, Func, 0));
	if(Data32 == 0xFFFFFFFF) return;
	// Clear twice for RP PME pending status
		Data8 = PciRead8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x62));
		Data8 |= BIT0;
		PciWrite8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x62),Data8);		//Rx62
		IoWrite8(0xED, 0x00);
		PciWrite8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x62),Data8);		//Rx62
}
/* CJW-20170926 CHX001 BFT item, no need for CHX002 
//LNA-2016122701-S
VOID DisablePCIEOBFF(UINT8 Bus, UINT8 Dev, UINT8 Func)
{
	UINT8 Data8;
	UINT32 Data32;

	Data32 = PciRead32(PCI_LIB_ADDRESS(Bus, Dev, Func, 0));
	if(Data32 == 0xFFFFFFFF) return;
		Data8 = PciRead8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x69));
		Data8 &= ~(BIT5|BIT6);
		PciWrite8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x69),Data8);		
		Data8 = PciRead8(PCI_LIB_ADDRESS(Bus, Dev, Func, 0x69));
		DEBUG((EFI_D_ERROR, "DisablePCIEOBFF Rx69 = %x\n", Data8));

}
//LNA-2016122701-E
*/

// SleepType: 3, 4, 5, 0xFF(SoftOff)
/// (has checked for CHX001 by Tiger. 2016-06-23)
VOID SleepCommonHandler(UINT8 SleepType)
{
  BOOLEAN  BiosUpdate = FALSE;
//  UINT16 Data;		//LNA-2016122701

/*  CJW-20170926 CHX001 BFT item, no need for CHX002 
  //LNA-2016122701-S
   Data = IoRead16(mAcpiBaseAddr + PMIO_STS_REG); ///PMIO_Rx00[15:0] Power Management Status
   DEBUG((EFI_D_ERROR, "[LNA_OBFF] PMIO Rx00:%04x\n", Data));
   //IoWrite16(mAcpiBaseAddr + 0xE4,0);

   DisablePCIEOBFF(0, 3, 0);
   DisablePCIEOBFF(0, 3, 1);
   DisablePCIEOBFF(0, 3, 2);
   DisablePCIEOBFF(0, 3, 3);
   DisablePCIEOBFF(0, 4, 0);
   DisablePCIEOBFF(0, 4, 1);
   DisablePCIEOBFF(0, 5, 0);
   DisablePCIEOBFF(0, 5, 1);
   Data = IoRead16(mAcpiBaseAddr + 0xE4);
   DEBUG((EFI_D_ERROR, "[LNA_OBFF] PMIO RxE4:%04x\n", Data));
   IoWrite16(mAcpiBaseAddr + 0x00,0x4000);
   Data = IoRead16(mAcpiBaseAddr + PMIO_STS_REG); ///PMIO_Rx00[15:0] Power Management Status
   DEBUG((EFI_D_ERROR, "[LNA_OBFF] PMIO Rx00:%04x\n", Data));
  //LNA-2016122701-E
*/


  ///if(SleepType == 3 || SleepType == 4 || SleepType == 5){
  ///  WaitxHCIPDU2U3(); // empty function for CHX001
  ///  IoAnd8(mAcpiBaseAddr + PMIO_SUS_PWR_DOMAIN_REG, (UINT8)~BIT7); //PMIO_Rx86
  ///}  
  
  SetAfterPowerLoss();
  TurnOffKbLed();

 //LNA-2016111501-S 
  //PCIE S3 PME_Turn_Off Message
  //[Note] Those code executed in S3/S4/S5
  	if(SleepType == 3 || SleepType == 4 || SleepType == 5){
  		if(SleepType == 5){
       		// Disable GP2 & GP3 SMI, Disable GP2 & GP3 timer tick
       		IoAnd16(mAcpiBaseAddr + PMIO_GBLEN_REG, (UINT16)~(PMIO_GBLEN_GP2SMI | PMIO_GBLEN_GP3SMI)); ///PMIO_Rx2A[13][12] GP3/2 Timer Timeout SMI Enable
       		IoWrite16(mAcpiBaseAddr + PMIO_GBLSTS_REG, PMIO_GBLSTS_GP2SMI|PMIO_GBLSTS_GP3SMI);    ///PMIO_Rx28[13][12]  GP3/2 Timer Timeout Status
       		MmioWrite8(LPC_PCI_REG(LPC_GP23TIMER_CTRL_REG), LPC_GP23TIMER_CTRL_REG_DEF_VALUE);  ///LPC_Rx98[7:0] GP2 / GP3 Timer Control
		}
  		WaitL2L3Ready(0);
	}

  	ClearPMEStatus(0, 3, 0); 
  	ClearPMEStatus(0, 3, 1); 
  	ClearPMEStatus(0, 3, 2); 
  	ClearPMEStatus(0, 3, 3); 
  	ClearPMEStatus(0, 4, 0);
  	ClearPMEStatus(0, 4, 1);
  	ClearPMEStatus(0, 5, 0);
  	ClearPMEStatus(0, 5, 1);


  //LNA-2016111501-E 

  if(SleepType == 5){
    IoWrite16(mAcpiBaseAddr + PMIO_GP_STS, PMIO_GP_STS_PME);  ///PMIO_Rx20[5] PME# Status
    BiosUpdate = IsBiosWantUpdate();
  }

  if(!BiosUpdate && (SleepType == 5 || SleepType == 0xFF)){
    EnableS5RtcWake();  // RTC/CMOS operation.
  }
  
//SetWakeOnLan(SleepType);

// Capsule RTC
  if(BiosUpdate){
    SetSleepTypeS3();    
    SetRtcWakeUpForCapsule(5);
  }  

}

//LNA-2016111501-S 
//Mike_20161115 Add S1CallBack_S

EFI_STATUS
S1SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
	SleepCommonHandler(1);
	return EFI_SUCCESS;
}
//Mike_20161115 Add S1CallBack_E
//LNA-2016111501-E 

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S3SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PcdGet32(PcdProgressCodeS3SuspendStart));
  gS3Record->S3Sleep = PLAT_S3_SLEEP_SLEEP;

  SleepCommonHandler(3);
  //[ALJ20180315]
  //;ALWDBG030218-S
   //With USB mode 2, connect ASUS AC1300 USB WiFi device and run ACPI S4 burn-in would cause black screen and hang at post "40" in WIN10 x64 
	PciAndThenOr8(PCI_LIB_ADDRESS(0, 0xE, 0x7, 0x54), (UINT8)~0x20, 0x0);
   //;ALWDBG030218-E

/*
// Add GFX Power gating feature settings.
  if((UINT16)MmioRead32(IGD_PCI_REG(PCI_VID_REG)) != 0xFFFF){
    DEBUG((EFI_D_INFO, "IGD PG\n"));
    // Disable BMU/VPP/S0EUX/S0TUF before go to STR
    // Disable S0EUX
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_SOEUX_POWER_GATING_REG, (UINT8)~BIT4);
    // Disable  S0TUF
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_SOTUF_POWER_GATING_REG, (UINT8)~BIT4);
    // Keep BMU always being powered on after R25 for S3/S4 burn-in hang issue
    // Disable  BMU
    //IoWrite8(PmioBar|GFX_BMU_POWER_GATING, (IoRead8(PmioBar|GFX_BMU_POWER_GATING)&0xF7));
    // Disable  VPP
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_VPP_POWER_GATING_REG, (UINT8)~BIT4);
  } else {
      // Enable DIU before go to STR
      // Enable  DIU
      // Always keep DIU enabled after R23. Therefore, it is not necessary to enable again here.
      // IoWrite8(PmioBar|GFX_DIU_POWER_GATING, (IoRead8(PmioBar|GFX_DIU_POWER_GATING)|POWER_CONTROL_FOR_DIU));
  }
*/

// D0F4Rx71[7] = 0, Disable MCKE dynamic assertion
  MmioAnd32(NPMC_PCI_REG(DRAM_DYNCLK_CTRL_REG), (UINT32)~BIT15);  // Dynamic CKE When DRAM Is Idle (disable)

// Patch F0000 readable when S3 resume 
// (0,0,3,83)[5:4] - F0000-FFFFFh Memory Space Access Control 
  MmioAnd32(HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG), (UINT32)~((BIT4|BIT5)<<24));


// Set D0F3 Rx63[0]=1 to fix S3 resume fail issue. (CHX001 has deprecated this bit!!)
  //if(MmioRead8(NPMC_PCI_REG(D0F4_INTERNAL_REV_ID)) == 0x00){
  //  MmioOr8(DRAM_PCI_REG(IBV_D0F3_RESERVED_Z4), D0F3_REF_STP); // 
  //}

#ifndef MDEPKG_NDEBUG	
  SleepSmiDebug(3);
#endif  
    
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S4SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  SleepCommonHandler(4);
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S5SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  SleepCommonHandler(5);
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
PowerButtonCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  UINT16  Buffer;
  //UINT16  BaseAddress;
  //UINT16  Index;

  SleepCommonHandler(0xFF);

// Clear Sleep Type Enable
  IoAnd16(mAcpiBaseAddr + PMIO_GBLEN_REG,  (UINT16)~PMIO_GBLEN_SLPSMI); ///PMIO_Rx2A[10] SMI SLP_EN Write

// Clear Power Button Status
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, PMIO_STS_PWRBTN); // PMIO_Rx00[8] Power Button Status

// Stop all UHCI controllers before trying to shut down
  //for (Index = 0; Index < 3; Index++) {
  //  BaseAddress = MmioRead16(UHCI_PCI_REG(Index, 0x20)) & ~BIT0;
  //  if(BaseAddress == 0xFFFE){continue;}
  //  IoWrite8(BaseAddress, 0);
  //}

// Shut it off now
  Buffer = IoRead16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG) & (~(PMIO_PM1_CNT_SLP_EN | PMIO_PM1_CNT_SLP_TYP)); /// PMIO_Rx04[13:10} Sleep Enable and Type
  Buffer |= PMIO_PM1_CNT_S5;
  IoWrite16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG, Buffer); ///PMIO_Rx04[15:0] Power Management Control
  Buffer |= PMIO_PM1_CNT_SLP_EN;
  IoWrite16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG, Buffer); ///PMIO_Rx04[15:0] Power Management Control

  return EFI_SUCCESS;
}

EFI_STATUS
LegacyUsbCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                    Status;
  EFI_LEGACY_USB_INF_PROTOCOL   *LegacyUsbInf;

  Status = gSmst->SmmLocateProtocol (
                  &gEfiLegacyUsbInfProtocolGuid,
                  NULL,
                  &LegacyUsbInf
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mSmmCallTablePtr = LegacyUsbInf->SmmCallTablePtr;
  return EFI_SUCCESS;
}

EFI_STATUS
InitializePlatformSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS                                Status;
  EFI_HANDLE                                PowerButtonHandle = NULL;
  EFI_HANDLE                                SwHandle = NULL;
  EFI_HANDLE                                S4SleepEntryHandle;  
  EFI_HANDLE                                S5SleepEntryHandle;
  EFI_HANDLE                                S3SleepEntryHandle;
  EFI_HANDLE                                S1SleepEntryHandle;
  EFI_SMM_POWER_BUTTON_DISPATCH2_PROTOCOL   *PowerButtonDispatch;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_SMM_POWER_BUTTON_REGISTER_CONTEXT     PowerButtonContext;
  EFI_SMM_SW_REGISTER_CONTEXT               SwContext;
  EFI_SMM_SX_DISPATCH2_PROTOCOL             *SxDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT               EntryDispatchContext;
  UINTN                                     VariableSize;
  SETUP_DATA	                              *SetupHob;
  VOID                                      *Registration;


  gAcpiRam = (EFI_ACPI_RAM_DATA*)GetAcpiRam();
  gS3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
  
  Status = gSmst->SmmLocateProtocol(&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&mSmmVariable);
  if(EFI_ERROR(Status)){
    mSmmVariable = NULL;		
    SetupHob = (SETUP_DATA*)GetSetupDataHobData();
    CopyMem(&gSetupData, SetupHob, sizeof(SETUP_DATA));		
  } else {
    VariableSize = sizeof(SETUP_DATA);  
    Status = mSmmVariable->SmmGetVariable (
                             PLATFORM_SETUP_VARIABLE_NAME,
                             &gPlatformSetupVariableGuid,
                             NULL,
                             &VariableSize,
                             &gSetupData
                             );  
    ASSERT_EFI_ERROR(Status);
  }

  mAcpiBaseAddr = PcdGet16(AcpiIoPortBaseAddress);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmPowerButtonDispatch2ProtocolGuid,
                    NULL,
                    &PowerButtonDispatch
                    );
  ASSERT_EFI_ERROR (Status);

  PowerButtonContext.Phase = PowerButtonEntry;
  Status = PowerButtonDispatch->Register (
                                  PowerButtonDispatch,
                                  PowerButtonCallback,
                                  &PowerButtonContext,
                                  &PowerButtonHandle
                                  );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_ENABLE_SW_SMI; // 0xF0
  Status = SwDispatch->Register (
                         SwDispatch,
                         EnableAcpiCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_DISABLE_SW_SMI; // 0xF1
  Status = SwDispatch->Register (
                         SwDispatch,
                         DisableAcpiCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_S3_PEI_END_SW_SMI; // 0xF2
  Status = SwDispatch->Register (
                         SwDispatch,
                         S3PeiEndCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);
  

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSxDispatch2ProtocolGuid,
                    NULL,
                    &SxDispatch
                    );
  if (EFI_ERROR(Status)) {
    return Status;
  }
   //LNA-2016111501-S  
   //Mike_20161115 Add S1CallBack_S

   EntryDispatchContext.Type  = SxS1;
   EntryDispatchContext.Phase = SxEntry;
   Status = SxDispatch->Register (
						  SxDispatch,
						  S1SleepEntryCallBack,
						  &EntryDispatchContext,
						  &S1SleepEntryHandle
						  );
   //Mike_20161115 Add S1CallBack_E
   //LNA-2016111501-S 

  EntryDispatchContext.Type  = SxS3;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S3SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S3SleepEntryHandle
                         );

  EntryDispatchContext.Type  = SxS4;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S4SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S4SleepEntryHandle
                         );

  EntryDispatchContext.Type  = SxS5;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S5SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S5SleepEntryHandle
                         );

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiLegacyUsbInfProtocolGuid,
                    LegacyUsbCallback,
                    &Registration
                    );
  LegacyUsbCallback (NULL, NULL, NULL);

  return EFI_SUCCESS;
}



