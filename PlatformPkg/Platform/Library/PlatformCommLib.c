
#include <Uefi.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Uefi/UefiSpec.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <Uefi/UefiAcpiDataTable.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PlatformCommLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PciIo.h>
#include <RtcDef.h>
#include <PlatformDefinition.h>
#include <PlatS3Record.h>


extern EFI_GUID gPlatformSetupVariableGuid;
extern EFI_GUID gEfiConsoleOutDeviceGuid;

UINT8 PcieRead8(UINTN PcieAddr)
{
  UINT8   Shift;
  UINT32  Data32;
  
  Data32 = MmioRead32(PcieAddr & ~(BIT0 + BIT1));
  Shift  = (UINT8)((PcieAddr & (BIT0 + BIT1))*8);
  
  return (UINT8)((Data32 >> Shift) & 0xFF);
}

/*
UINT8 Cmos72Read(UINT8 Address)
{
  IoWrite8(0x72, Address);
  return IoRead8(0x73);
}

VOID Cmos72Write(UINT8 Address, UINT8 Data)
{
  IoWrite8(0x72, Address);
  IoWrite8(0x73, Data);
}

VOID Cmos72AndOr(UINT8 Address, UINT8 AndData, UINT8 OrData)
{
  UINT8  OldData;

  IoWrite8(0x72, Address);
  OldData = IoRead8(0x73);
	IoWrite8(0x73, (OldData & AndData)|OrData);
}
*/

UINT8 CmosRead(UINT8 Address)
{
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, (UINT8)(Address|(UINT8)(IoRead8(PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  return IoRead8(PCAT_RTC_DATA_REGISTER);
}

VOID CmosWrite_Original(UINT8 Address, UINT8 Data)
{
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, (UINT8)(Address|(UINT8)(IoRead8(PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  IoWrite8(PCAT_RTC_DATA_REGISTER, Data);
}
VOID CmosWrite(UINT8 Address, UINT8 Data)
{
	RTC_REGISTER_A	RegisterA;
	RTC_REGISTER_B	RegisterB;


		//RTC patch for 0xA-OxD  
		if((Address>=0xA)&&(Address<=0xD)){
		//	DEBUG((EFI_D_ERROR, "DLA:[CmosWrite] [Address:0x%x]  = 0x%x\n",Address,Data));
		  RegisterB.Data	  = CmosRead (RTC_ADDRESS_REGISTER_B);

		  if(RegisterB.Bits.Set==0){

		  RegisterA.Data	= CmosRead (RTC_ADDRESS_REGISTER_A);
		
			//wait for uip=0
		  	while (RegisterA.Bits.Uip == 1 ) {
				MicroSecondDelay (10);
				RegisterA.Data = CmosRead (RTC_ADDRESS_REGISTER_A);
		  	}   
			}
			}
		CmosWrite_Original(Address,Data);		

}


UINT8 CheckAndConvertBcd8ToDecimal8(UINT8 Value)
{
  if ((Value < 0xa0) && ((Value & 0xf) < 0xa)) {
    return BcdToDecimal8(Value);
  }
  return 0xff;
}


EFI_STATUS RtcWaitToUpdate()
{
  RTC_REGISTER_A  RegisterA;
  RTC_REGISTER_D  RegisterD;
	UINTN           Timeout;

  Timeout = PcdGet32(PcdRealTimeClockUpdateTimeout);

  RegisterD.Data = CmosRead (RTC_ADDRESS_REGISTER_D);
  if (RegisterD.Bits.Vrt == 0) {
    return EFI_DEVICE_ERROR;
  }

  Timeout         = (Timeout / 10) + 1;
  RegisterA.Data  = CmosRead (RTC_ADDRESS_REGISTER_A);
  while (RegisterA.Bits.Uip == 1 && Timeout > 0) {
    MicroSecondDelay (10);
    RegisterA.Data = CmosRead (RTC_ADDRESS_REGISTER_A);
    Timeout--;
  }

  RegisterD.Data = CmosRead (RTC_ADDRESS_REGISTER_D);
  if (Timeout == 0 || RegisterD.Bits.Vrt == 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}



// W = (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400) mod 7
// 0 - Monday
UINT8 CaculateWeekDay(UINT16 y, UINT8 m, UINT8 d)
{
  UINT8 Weekday;
  
  if(m == 1 || m == 2){
    m += 12;
    y--;
  }
  Weekday = (d + 2*m + 3*(m + 1)/5 + y + y/4 - y/100 + y/400) % 7;
  return Weekday;
}  



BOOLEAN IsLeapYear(UINT16 Year)
{
  if (Year % 4 == 0) {
    if (Year % 100 == 0) {
      if (Year % 400 == 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}





// 0:POWER_OFF
// 1:POWER_PREVIOUS
// 2:POWER_ON 
//
// RTC Register Rx0D[7]
//   (0) Power Off  : always 1
//   (2) Power On   : always 0
//   (1) Last State : 0 in POST, 1 in S3/S4/S5.

VOID SetCmosVRT(BOOLEAN OnOff)
{
  UINT32          Data32;
  RTC_REGISTER_D  RegD;
  
  Data32 = MmioRead32(LPC_PCI_REG(LPC_SMMC1_REG));
  if(Data32 & LPC_SMMC1_RTC_D_WP){
    MmioAnd32(LPC_PCI_REG(LPC_SMMC1_REG), (UINT32)~LPC_SMMC1_RTC_D_WP);
  }
  
  RegD.Data = CmosRead(RTC_ADDRESS_REGISTER_D);
  RegD.Bits.Vrt = OnOff?1:0;
  CmosWrite(RTC_ADDRESS_REGISTER_D, RegD.Data);
  
  if(Data32 & LPC_SMMC1_RTC_D_WP){
    MmioOr32(LPC_PCI_REG(LPC_SMMC1_REG), LPC_SMMC1_RTC_D_WP);
  }  
}


VOID SystemSoftOff()
{
  UINT16   Data16;
  
  Data16  = IoRead16(PMIO_REG(PMIO_PM1_CNT_REG));///PMIO_Rx04[15:0] Power Management Control
  Data16 &= ~(PMIO_PM1_CNT_SLP_EN | PMIO_PM1_CNT_SLP_TYP);
  Data16 |= PMIO_PM1_CNT_S5;
  IoWrite16(PMIO_REG(PMIO_PM1_CNT_REG), Data16);///PMIO_Rx04[15:0] Power Management Control
  Data16 |= PMIO_PM1_CNT_SLP_EN;
  IoWrite16(PMIO_REG(PMIO_PM1_CNT_REG), Data16); ///PMIO_Rx04[15:0] Power Management Control
  
  CpuDeadLoop(); 
}








VOID *GetPlatformMemInfo(VOID)
{  
  VOID                  *MemInfo;
  EFI_PEI_HOB_POINTERS  GuidHob;    
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo = (VOID*)(GuidHob.Guid+1);
  
  return MemInfo;
}


VOID *GetPlatformDimmInfo(VOID)
{  
  VOID                  *DimmInfo;
  EFI_PEI_HOB_POINTERS  GuidHob;    
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatDimmInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  DimmInfo = (VOID*)(GuidHob.Guid+1);
  
  return DimmInfo;
}


VOID *GetSetupDataHobData(VOID)
{
  EFI_PEI_HOB_POINTERS  GuidHob;
  VOID                  *SetupData;

  GuidHob.Raw = GetFirstGuidHob(&gPlatformSetupVariableGuid);
  ASSERT(GuidHob.Raw != NULL);
  SetupData = (VOID*)(GuidHob.Guid+1);

  return SetupData;
}


VOID *GetAcpiRam(VOID)
{
  PLATFORM_S3_RECORD  *S3Record;
  EFI_ACPI_RAM_DATA   *AcpiRam;
  
  /// PcdS3RecordAddr : PlatformPkg.dec declare it.
  /// PlatformPeiEntry() will set its actual value.
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  AcpiRam  = &S3Record->AcpiRam;
  ASSERT(AcpiRam->Signature == ACPI_RAM_DATA_SIGNATURE);
  return (VOID*)AcpiRam;
}

VOID PlatRecordS3DebugData(CHAR8 *Name, UINT32 Data32)
{
  PLATFORM_S3_RECORD  *S3Record;
  UINT32              NameData;
  UINTN 	            Length;

  NameData = 0x20202020;
  Length = AsciiStrLen(Name);
  if(Length > 4){Length = 4;}
  CopyMem(&NameData, Name, Length);
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);
  
  if(S3Record->DebugDataIndex >= PLAT_DBG_DATA_DD_COUNT){
    return;
  }  

  S3Record->DebugData[S3Record->DebugDataIndex].Id   = NameData;
  S3Record->DebugData[S3Record->DebugDataIndex].Data = Data32;
  S3Record->DebugDataIndex++;
}

VOID *GetAcpiTableScat(VOID)
{
  PLATFORM_S3_RECORD               *S3Record;
  EFI_SMM_COMMUNICATION_ACPI_TABLE *Scat;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  Scat = (EFI_SMM_COMMUNICATION_ACPI_TABLE*)(UINTN)S3Record->ScatAddr;
  ASSERT(Scat->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE);
  return (VOID*)Scat;
}


VOID *GetS3MtrrTable()
{
  PLATFORM_S3_RECORD  *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE); 

  return (VOID*)&S3Record->MtrrTable;
}


VOID GetS3Cr3Stack(UINT32 *S3Cr3, UINT32 *S3StackBase, UINT32 *S3StackSize)
{
  PLATFORM_S3_RECORD               *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);  
  if(S3Cr3!=NULL){*S3Cr3 = S3Record->S3Cr3;}
  if(S3StackBase!=NULL){*S3StackBase = S3Record->S3StackBase;}  
  if(S3StackSize!=NULL){*S3StackSize = S3Record->S3StackSize;}
}

VOID *GetS3RecordTable()
{
  PLATFORM_S3_RECORD               *S3Record;
  
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(S3Record->Signature == PLAT_S3_RECORD_SIGNATURE);

  return (VOID*)S3Record;
}

BOOLEAN IsSmrrTypeSetWB()
{
  PLATFORM_S3_RECORD  *S3Record;

  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
  if(S3Record->SmrrType == CacheWriteBack){
    return TRUE;
  } else {
    return FALSE;
  }
}


VOID *GetCarTopData()
{
  EFI_PEI_HOB_POINTERS   GuidHob;
  CAR_TOP_DATA           *CarTopData;

  GuidHob.Raw = GetFirstGuidHob(&gCarTopDataHobGuid);
  ASSERT(GuidHob.Raw != NULL);  
  CarTopData  = (CAR_TOP_DATA*)(GuidHob.Guid+1);

  return (VOID*)CarTopData;
}
#ifdef IOE_EXIST
EFI_STATUS LoadIoeMcuFw(UINT8 BusOfEptrfc, VOID *FwAddr_alloc,UINT16 AutoFillAddr,UINT16 AutoFillLen)
{
	
	UINT8 Dev = 0;
	UINT8 Func = 0;
	UINT32	FwAddr_Hi,FwAddr_Lo;
	UINT8 TmpReg;
	UINT64 *FwAddr;
	
	//[3]Set the parameters into registers
	//Setting the FW instruction execution Start_address/Length in 8051
  	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_START_ADDR), (UINT32)AutoFillAddr);	//set firmware auto fill address
  	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_LENGTH), AutoFillLen<<1); 	 //Max 32KB - AutoLoad length

	//Setting the FW
	FwAddr = (UINT64 *)FwAddr_alloc;
	//FwAddr= (UINT64)((UINT32*)FwAddr_alloc);

	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] [Line:%d] FW address in DRAM = %x\n",__LINE__,FwAddr));
  	FwAddr_Lo= (UINT32)((UINT64)((UINT32*)FwAddr));
	FwAddr_Hi= (UINT32)(((UINT64)((UINT32*)FwAddr))>>32);
	if((FwAddr_Lo&0xFFFFFFC0) != FwAddr_Lo){
		ASSERT_EFI_ERROR(EFI_ACCESS_DENIED);
	}
	DEBUG((EFI_D_ERROR, "              [Line:%d] FwAddr_Lo=%x  FwAddr_Hi=%x\n",__LINE__,FwAddr_Lo, FwAddr_Hi));
	FwAddr_Hi = 0;
	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_FW_INSTRUCTION_BASEADDR_Lo), FwAddr_Lo);
	MmioWrite32((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_FW_INSTRUCTION_BASEADDR_Hi), FwAddr_Hi);
	
  	//Flush cache[xxx]
  	AsmWbinvd();
  	DEBUG((EFI_D_ERROR, "              [Line:%d] Flush Cache\n",__LINE__));


#if 1
	//Reset FW
	TmpReg = MmioRead8((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET));
	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg&0xFE);		//reset state
	//MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg|0x01);		//exit reset state	
#endif


	//Enable AutoFill fw
	TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN); 
	TmpReg |= 0x01;
  	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN,TmpReg); 				//auto fill enable



	DEBUG((EFI_D_ERROR, "              [Line:%d] Wait...\n",__LINE__));

	{
		UINT32 ddd,i;
		for(i=0;i<0xFF;i+=4){
			if((i&0xF)==0x0){
				DEBUG((EFI_D_ERROR, "\n              Rx0x%02x: ",i));
			}
			ddd = MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + i);
			DEBUG((EFI_D_ERROR, " 0x%08x ",ddd));
		}
		DEBUG((EFI_D_ERROR, "\n"));
	}

#if 1//ALJ20161030: for 4f hang issue ,remove for now.
	//Wait for autofill done
	do{
		TmpReg =  MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_DONE);

		//**
		//we need to add a timer to wait this bit to be cleard
		//and if timeout 
		//we should assert() an error break 
	}while((TmpReg&BIT0) == 0x00);
	DEBUG((EFI_D_ERROR, "              [Line:%d] FW AutoFill Done\n",__LINE__));
#else 
	{
		UINTN pp,xx;
		for(pp = 0;pp<0xFFFF ;pp++ )
			for(xx =0; xx<0xFFF; xx++);
	}
#endif
	

	//disable AutoFill fw
	TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN); 
	TmpReg &= 0xFE;
  	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_AUTOFILL_EN,TmpReg);


#if 1
	//Reset FW
	TmpReg = MmioRead8((PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET));
	//MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg&0xFE);		//reset state
	MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + IOE_MCU_SOFTWARE_RESET, TmpReg|0x01);		//exit reset state	
#endif



  return EFI_SUCCESS;	
}

EFI_STATUS
LoadIoeXhciFw(
  UINT8 BusOfEptrfc,
  VOID *FwAddr_alloc
)
{
  UINT8   Dev = 18;
  UINT8   Func = 0;
  UINT8   TmpReg;
  UINT16  TmpReg16;
  UINT32  TmpReg32;
  UINT32  FwAddr_Hi, FwAddr_Lo;

  UINT64 *FwAddr;
  
  UINT16  AutoFillAddr    = 0x500;
  UINT16  AutoFillLen     = 0x5000;   // 20KB

  AsmWbinvd();
  MicroSecondDelay (5);

  //Setting the FW
  FwAddr    = (UINT64 *)FwAddr_alloc;
  FwAddr_Lo = (UINT32)((UINT64)((UINT32*)FwAddr));
  FwAddr_Hi = (UINT32)(((UINT64)((UINT32*)FwAddr))>>32);
  if((FwAddr_Lo & 0xFFFF0000) != FwAddr_Lo) {
    return EFI_ACCESS_DENIED;
  }

  FwAddr_Hi = 0;
  DEBUG((EFI_D_INFO, "                  FwAddr_Lo=%x  FwAddr_Hi=%x\n", FwAddr_Lo, FwAddr_Hi));

  //enable bus master
  MmioOr8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_BUS_MASTER); 
  TmpReg=MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET);
  DEBUG((EFI_D_INFO, "                  Rx04 is %x\n", TmpReg));

  //operation enable
  MmioOr8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43, XHCI_OPT_CFG_EN); 
  TmpReg=MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43);
  DEBUG((EFI_D_INFO, "                  Rx43 is %x\n", TmpReg));

  //whether the value of Xhci Openrisc Software Reset Controlled by BIOS is 1
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x20);
  TmpReg = MmioRead8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_CFG_DAT);  
  DEBUG((EFI_D_INFO, "                  the default value of MCU software reset is %x\n", TmpReg));

  //MCU_ON_BOARD_APP=1 Rx04[0] = 1'b
  //SPIROM_ON_BOARD=0, MCU_INS_BUF_EN=1(Rx05[1:0]=10'b)
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x04);
  MmioWrite16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_DAT, 0x0201);
  TmpReg16 = MmioRead16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+ XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx30004  %x [0201]\n", TmpReg16));

  //Set Xhci Fw Lower Base Address in system memory
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x28);
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, (UINT32)(UINTN)FwAddr_Lo);
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx30028 FW Low Address  %x \n", TmpReg32));

  //Set Xhci Fw Upper Base Address in system memory
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x2C); 
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, FwAddr_Hi);   
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx3002C FW High Address  %x \n", TmpReg32));

  //Xhci Openrisc auto fill start address
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x0C);
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, AutoFillAddr); 
  TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT);
  DEBUG((EFI_D_INFO, "                  Rx3000C Autofill Start Address  %x \n", TmpReg32));

  //Xhci Openrisc auto fill length
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x08);
  MmioWrite16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT+2, AutoFillLen); 

  TmpReg16 =  MmioRead16(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT+2);
  DEBUG((EFI_D_INFO, "                  Rx30008 Autofill Length  %x \n", TmpReg16));

  //Xhci Openrisc Instruction Auto-fill Enable Rx08=1 
  MmioWrite32(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_ADR, XHCI_OPTCFG_MCU_BASE+0x08);
  MmioWrite8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func) + XHCI_OPT_CFG_DAT, 1);    

  //operation disable
  MmioAnd8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+XHCI_OPT_RX43, (UINT8)~XHCI_OPT_CFG_EN);

  DEBUG((EFI_D_INFO, "                  Auto-fill Enable and Wait...\n"));

  do {
    TmpReg32 =  MmioRead32(PCI_DEV_MMBASE(BusOfEptrfc, Dev, Func) + 0xB0);
  } while ( (TmpReg32 & BIT0) != BIT0 );
  DEBUG((EFI_D_INFO, "                  Xhci FW Init Done:%x\n", TmpReg32));

  //disable  bus master 
  MmioAnd8(PCI_DEV_MMBASE(BusOfEptrfc,Dev,Func)+PCI_COMMAND_OFFSET, (UINT8)~EFI_PCI_COMMAND_BUS_MASTER); 

  return EFI_SUCCESS;
}
#endif


//ALJ-CHX002- FWLOAD 
EFI_STATUS LoadXhciFw(
    UINT32  FwAddr_Lo,
    UINT32  FwAddr_Hi
)
{
    UINT16      AutoFillAddr    = 0x500;
    UINT16      AutoFillLen     = 0x1800;   // 6KB
    UINT32      Data;

    AsmWbinvd();
    MicroSecondDelay (5);

    //
    // 0. Check Fw Address if 64KB-aligned
    //
    if (FwAddr_Hi != 0) {
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware address beyond 4G!\n"));
        return EFI_INVALID_PARAMETER;
    }

    if ((FwAddr_Lo & 0xFFFF0000) != FwAddr_Lo) {
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware address is not 64KB-aligned!\n"));
        return EFI_ACCESS_DENIED;
    }

    DEBUG((EFI_D_INFO, "                  Firmware Address Low = 0x%08X  Firmware Address High = 0x%08X\n", FwAddr_Lo, FwAddr_Hi));

    //
    // 1. Enable bus master and OPTCFG access
    //
    MmioOr8(XHCI_PCI_REG(PCI_COMMAND_OFFSET), EFI_PCI_COMMAND_BUS_MASTER);
    Data    = MmioRead8(XHCI_PCI_REG(PCI_COMMAND_OFFSET));
    DEBUG((EFI_D_INFO, "                  PCI Command(Rx04[2]) is %X\n", ((Data & BIT2) >> 2)));
    MmioOr8(XHCI_PCI_REG(XHCI_OPT_RX43), XHCI_OPT_CFG_EN);
    Data    = MmioRead8(XHCI_PCI_REG(XHCI_OPT_RX43));
    DEBUG((EFI_D_INFO, "                  Rx43[0] is %X\n", (Data & BIT0)));

    //
    // 2. Peek path for MCU DMA cycle
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), 0xC0);
    Data    = MmioRead32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  MCUDMASEL(RxC0[5]) is %Xb  (0b for non-snoop, 1b for snoop)\n", ((Data & BIT5) >> 5)));

    //
    // 3. Config Base Address of MCU Firmware in System Memory
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x28);
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT), FwAddr_Lo);
    Data    = MmioRead32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  Base Address of MCU Firmware in System Memory Low(Rx30028):  0x%08X \n", Data));
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x2C);
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT), FwAddr_Hi);
    Data    = MmioRead32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  Base Address of MCU Firmware in System Memory High(Rx3002C):  0x%08X \n", Data));

    //
    // 4. Config Start Address of Auto-fill Instruction
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x0C);
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT), AutoFillAddr);
    Data    = MmioRead32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  Start Address of Auto-fill Instruction(Rx3000C)  0x%08X \n", Data));

    //
    // 5. Config MCU Instruction Auto-fill Length
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x08);
    MmioWrite16(XHCI_PCI_REG(XHCI_OPT_CFG_DAT) + 2, AutoFillLen);
    Data    = MmioRead32(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Length(Rx3000A)  0x%04X \n", (UINT16)(Data >> 16)));

    //
    // 6. Start MCU Instruction Auto-fill
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x08);
    MmioWrite8(XHCI_PCI_REG(XHCI_OPT_CFG_DAT), 0x1);
    Data    = MmioRead8(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Enable(Rx30008[0])  %X\n", (Data & BIT0)));

    //
    // 7. Wait for autofill done
    //
    DEBUG((EFI_D_INFO, "                  Wait autofill done...\n", __LINE__));
    do {
        MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x08);
        Data    = MmioRead8(XHCI_PCI_REG(XHCI_OPT_CFG_DAT) + 0x1);
    } while ((Data & BIT0) == 0x00);
    DEBUG((EFI_D_INFO, "                  MCU Instruction Auto-fill Done(Rx30009[0])  %X\n", (Data & BIT0)));

    //
    // 8. Config MCU Software Reset
    //
    MmioWrite32(XHCI_PCI_REG(XHCI_OPT_CFG_ADR), XHCI_OPTCFG_MCU_BASE + 0x20);
    MmioWrite8(XHCI_PCI_REG(XHCI_OPT_CFG_DAT), 0x1);
    // Data    = MmioRead8(XHCI_PCI_REG(XHCI_OPT_CFG_DAT));
    // DEBUG((EFI_D_INFO, "                  MCU Software Reset(Rx30020[0]) is %X\n", (Data & BIT0)));

    //
    // 9. Disable bus master and OPTCFG access
    //
    MmioAnd8(XHCI_PCI_REG(XHCI_OPT_RX43), (UINT8)~XHCI_OPT_CFG_EN);
    Data    = MmioRead8(XHCI_PCI_REG(XHCI_OPT_RX43));
    DEBUG((EFI_D_INFO, "                  Rx43[0] is %X\n", (Data & BIT0)));

    //
    // 10. Wait FW init done
    //
    DEBUG((EFI_D_INFO, "                  Wait FW init done...\n", __LINE__));
    do  {
        Data    = MmioRead32(XHCI_PCI_REG(XHCI_FWSWMSG0_REG));
    } while ( (Data & XHCI_FWSWMSG0_INITDONE) != XHCI_FWSWMSG0_INITDONE );
    DEBUG((EFI_D_INFO, "                  MCU Init Done(RxB0[0]) is %X\n", (Data & BIT0)));

    // MicroSecondDelay (60*1000);

    MmioAnd8(XHCI_PCI_REG(PCI_COMMAND_OFFSET), (UINT8)~EFI_PCI_COMMAND_BUS_MASTER);
    Data    = MmioRead8(XHCI_PCI_REG(PCI_COMMAND_OFFSET));
    DEBUG((EFI_D_INFO, "                  PCI Command(Rx04[2]) is %X\n", ((Data & BIT2) >> 2)));

    return EFI_SUCCESS;
}


/**
@CJW-20171114
@Param:
	PeMcuFw: Memory base address for saving FW
	IsDoEQ: Do EQ or not, if DO EQ, then this function 
	TP_Value: If DO EQ, will request TP_Value as the Tx preset for Endpoint
	TargetBus: For single socket, this should be 0h, for dual socket, 'TargetBus' means BusNumber of target socket
@Return:
	
**/
EFI_STATUS LoadPeMcuFw(
	VOID *PeMcuFw, 	
    VOID *PeMcuData,
	UINT8 IsDoEQ, 
	UINT8 TP_Value, 
	UINT8 TargetBus)
{
  EFI_STATUS  	Status;
  UINTN 		PCIEPhyMMIOBase;
  UINT32    	FwAddr;
  UINT32         DataAddr;  
  UINT8 		TmpReg;
  UINT16		TmpReg16;
  
  Status  = EFI_SUCCESS;

  //gBS = (EFI_BOOT_SERVICES*)BootServices;

  //
  // Get EPHY Base Address 
  //
  PCIEPhyMMIOBase=(UINTN)MmioRead32((PCI_DEV_MMBASE(TargetBus,0,5)+D0F5_PCIE_EPHY_BASE_ADR)); 	 
  PCIEPhyMMIOBase = PCIEPhyMMIOBase<<8;
  DEBUG((DEBUG_ERROR," PCIEPhyMMIOBase: %x\n", PCIEPhyMMIOBase));

  //
  // Autofill start address of FW
  //
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR),0);						 
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR));   
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR: %x\n", TmpReg16));

  //
  //Autofill Lenth (Max 16K for CHX002, All zero means 16K)
  //
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN),SIZE_16KB);		 
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN));  
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_LEN: %x\n", TmpReg16));

  //
  //Reformat MemoryAddressBase of PEMCU FW
  //
  FwAddr= (UINT32)((UINT64)((UINT32*)PeMcuFw));
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  PcdSet64(PcdPEMCUFWAddr, (UINT64)FwAddr);  			//For TA RMRR tbl - jerry add
  PcdSet32(PcdPEMCUFWSize,(UINT32)(0 + SIZE_64KB));		//For TA RMRR tbl - jerry add
  FwAddr=FwAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION),FwAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION));   

  //
  //Reformat MemoryAddressBase of PEMCU XDATA 
  //
  DataAddr= (UINT32)((UINT64)((UINT32*)PeMcuData));
  DEBUG((EFI_D_ERROR, "[Line:%d] DataAddr = %x\n",__LINE__,DataAddr));
  PcdSet64(PcdPEMCUDATAAddr, (UINT64)DataAddr);			//For TA RMRR tbl - jerry add
  PcdSet32(PcdPEMCUDATASize,(UINT32)(0 + SIZE_64KB));	//For TA RMRR tbl - jerry add
  DataAddr=DataAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] DataAddr = %x\n",__LINE__,DataAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_DATA_SPACE),DataAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_DATA_SPACE));   


  //
  //Flush cache[xxx]
  //
  AsmWbinvd();
  DEBUG((EFI_D_ERROR, "[Line:%d] Flush Cache\n",__LINE__));

  //
  //Autofill Enable - start to load FW from system memory
  //
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_INST_AUTOFILL_EN); 			//auto fill enable

  //
  //Loop to wait Autofill Done
  //
  TmpReg=0;
  while((TmpReg&0x02)==0)  
  {
  	TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  }
  TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  DEBUG((EFI_D_ERROR, "[Line:%d] Fill done status = %X\n",__LINE__, TmpReg));  
  DEBUG((EFI_D_ERROR, "[Line:%d] FW AutoFill Done\n",__LINE__));

  //
  //Reset MCU to run - Write 0 to reset then write 1 to run
  //
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),0x00); 	
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_MCU_RST); //reset mcu


  ////////////////////////////////////////////////////////
  ///For EQ
  ///////////////////////////////////////////////////////

#if 1	 ///Add for Redo-EQ in DXE stage.  	

  if(IsDoEQ == 0){
  
	///For Debug - Add by ChrisJWang 2015.07.31 
	///[Possible Bug:When No UART debug message output, Pemcu AutoFill Failed]
	///Those Code can check whether FW autofill success 
	MicroSecondDelay(100);					//Wait 100us
    TmpReg = MmioRead8(PCIEPhyMMIOBase+PEMCU_RESET_AUTOFILL_EN);
    if(TmpReg == 0x00){
  	  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Failed\n"));
	  while(1){
		IoWrite8(0x80,0x52); 						//80 Port show 0x52	
	  }
    } 
    MicroSecondDelay(1000);	         // 1 msec delay  
	
	//
  	//CMD stage ,CMD Tx Preset = 3
  	//
  	DEBUG((EFI_D_ERROR,"TxPreset = SetupData->EQTxPreset - Start \n"));
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z6),0x03); 	
  	MicroSecondDelay(1000);	        // 1 msec delay  
  	// Run CMD (DB=1)
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 	
  	do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	
	}while(TmpReg == 0x01);

	//
  	//ARG stage - Get from Setup
  	//
  	TmpReg16 = TP_Value;
  	MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z6),TmpReg16); 	
  	MicroSecondDelay(1000);	         // 1 msec delay  
  	// Run CMD (DB=1)
  	MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 		
  	do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	
   	}while(TmpReg == 0x01);

  	DEBUG((EFI_D_ERROR," TxPreset = SetupData->EQTxPreset - end \n")); 
  
}

#endif

  return Status;  
}





//------------------------------------ DEBUG -----------------------------------
VOID DumpMem32(VOID *Base, UINTN Size)
{
  UINTN  Index;
	UINTN  Addr;

  Addr = (UINTN)Base;
  Addr &= ~0x3;

  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));
  for (Index=0; Index < Size; Index+=4) {
    if(Index%16==0){
      DEBUG((EFI_D_ERROR, "%08X: ", Addr+Index));
    }
    DEBUG((EFI_D_ERROR, "%08X ", MmioRead32(Addr+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }

  DEBUG((EFI_D_ERROR, "\n"));	
}


VOID DumpMem8(VOID *Base, UINTN Size)
{
  UINT8  *Data8;
  UINTN  Index;
  
  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));  
  Data8 = (UINT8*)Base;
  for(Index=0; Index<Size; Index++){
    DEBUG((EFI_D_ERROR, "%02X ", Data8[Index]));
    if(((Index+1)%16)==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));
}


VOID DumpCmos()
{
  UINTN  Index;
  
  DEBUG((EFI_D_ERROR, "%a()\n", __FUNCTION__));    
  for(Index=0;Index<128;Index++){
    IoWrite8(0x70, (UINT8)Index);
    DEBUG((EFI_D_ERROR, "%02X ", IoRead8(0x71)));
    if((Index+1)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }  
}

////
VOID DumpDramPci()
{
  UINTN   Index;
  UINTN   Base;


  Base = PCI_DEV_MMBASE(0, 0, 3);
  DEBUG((EFI_D_ERROR,"Device(%d, %d, %d):\n   ",0, 0, 3));
  for(Index = 0; Index < 16; Index++) DEBUG((EFI_D_ERROR," %02X", Index));
  ///
  DEBUG((EFI_D_ERROR,"\n  +"));
  ///
  for(Index = 0; Index <= 3*16; Index++) DEBUG((EFI_D_ERROR,"-"));
  ///
  for(Index=0;Index<0x300;Index++){
  	if(Index%16==0) DEBUG((EFI_D_ERROR,"\n%02X|", Index/16));
    DEBUG((EFI_D_ERROR, " %02X", MmioRead8(Base+Index)));
  }  
  DEBUG((EFI_D_ERROR,"\n"));

}
VOID DumpPcie(UINT8 Bus, UINT8 Dev, UINT8 Func,UINT32 Rang)
{
  UINTN   Index;
  UINTN   Base;


  Base = PCI_DEV_MMBASE(Bus, Dev, Func);
  DEBUG((EFI_D_ERROR,"Device(%d, %d, %d):\n   ",Bus, Dev, Func));
  for(Index = 0; Index < 16; Index++) DEBUG((EFI_D_ERROR," %02X", Index));
  ///
  DEBUG((EFI_D_ERROR,"\n  +"));
  ///
  for(Index = 0; Index <= 3*16; Index++) DEBUG((EFI_D_ERROR,"-"));
  ///
  for(Index=0;Index<Rang;Index++){
  	if(Index%16==0) DEBUG((EFI_D_ERROR,"\n%02X|", Index/16));
    DEBUG((EFI_D_ERROR, " %02X", MmioRead8(Base+Index)));
  }  
  DEBUG((EFI_D_ERROR,"\n"));

}
////

VOID DumpPci(UINT8 Bus, UINT8 Dev, UINT8 Func)
{
  UINTN   Index;
  UINTN   Base;

#if 0
  Base = PCI_DEV_MMBASE(Bus, Dev, Func);
  DEBUG((EFI_D_ERROR, "(%02X,%02X,%02X)\n", Bus, Dev, Func));
  for(Index=0;Index<256;Index+=4){
    DEBUG((EFI_D_ERROR, "%08X ", MmioRead32(Base+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }  
#else
	 Base = PCI_DEV_MMBASE(Bus, Dev, Func);
	 DEBUG((EFI_D_ERROR,"Device(%d, %d, %d):\n	 ",Bus, Dev, Func));
	 for(Index = 0; Index < 16; Index++) DEBUG((EFI_D_ERROR," %02X", Index));
	 DEBUG((EFI_D_ERROR,"\n  +"));
	 for(Index = 0; Index <= 3*16; Index++) DEBUG((EFI_D_ERROR,"-"));
	// DEBUG((EFI_D_ERROR, "\n"));
	// DEBUG((EFI_D_ERROR, "PciDev(%02X,%02X,%02X)\n", Bus, Dev, Func));
	 for(Index=0;Index<256;Index++){
	   if(Index%16==0) DEBUG((EFI_D_ERROR,"\n%02X|", Index/16));
	   DEBUG((EFI_D_ERROR, " %02X", MmioRead8(Base+Index)));
	   //if((Index+4)%16==0){
	   //  DEBUG((EFI_D_ERROR, "\n"));
	   //}
	 }	
	 DEBUG((EFI_D_ERROR,"\n"));
         ////
#endif
}

VOID DumpIo4(UINTN Base, UINT16 Size)
{
  UINT16  Index;
  
  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));    
  for (Index=0; Index < Size; Index+=4) {
    DEBUG((EFI_D_ERROR, "%08X ", IoRead32(Base+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));  
}


VOID DumpAllPciIntLinePin()
{
  UINTN   Base;
  UINT8   Bus, Dev, Func;
  UINT8   IntPin;

  for(Bus=0;Bus<64;Bus++){
    for(Dev=0;Dev<32;Dev++){
      for(Func=0;Func<8;Func++){
        Base = PCI_DEV_MMBASE(Bus, Dev, Func);
        if(MmioRead16(Base+0) == 0xFFFF){    // not present
          if(Func == 0){
            Func = 7;
          }
          continue;
        }
        IntPin = MmioRead8(Base+0x3D);
        if(IntPin != 0){
          DEBUG((EFI_D_ERROR, "(%02X,%02X,%02X) %02X(%c) -> %02X\n", 
                               Bus, Dev, Func, 
                               IntPin,
                               IntPin + 'A' - 1,
                               MmioRead8(Base+0x3C)
                               ));
        }
        if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
          Func = 7;
        }
      }
    }
  }
}


VOID DumpAllPci()
{
  UINTN   Base;
  UINT8   Bus, Dev, Func;
  
  for(Bus=0;Bus<64;Bus++){                 // assume max bus number is 64
    for(Dev=0;Dev<32;Dev++){
      for(Func=0;Func<8;Func++){
        Base = PCI_DEV_MMBASE(Bus, Dev, Func);
        if(MmioRead16(Base+0) == 0xFFFF){    // not present
          if(Func == 0){
            Func = 7;
          }
          continue;
        }
        
        DumpPci(Bus, Dev, Func);
        
        if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
          Func = 7;
        }
      }
    }
  }
  DEBUG((EFI_D_ERROR, "\n"));  
}

///
VOID ZX_DumpPciDevSetting()
{
//Yankui-dbg-s
  UINTN   Base, Index;
  UINT8   Bus, Dev, Func;
  UINT8 Bus0Dev[] = {0,1,2,3,4,5,6,7,9,10,12,15,16,17,18,20,21};
  UINT32  MmioBase;
//All devices on bus0
  Bus = 0;
  for(Index = 0; Index < sizeof(Bus0Dev)/sizeof(Bus0Dev[0]); Index++) {
      Dev = Bus0Dev[Index];
	  for(Func = 0; Func < 8; Func++) {
    	  Base = PCI_DEV_MMBASE(Bus, Dev, Func);
          if(MmioRead16(Base+0) == 0xFFFF){    // not present
              if(Func == 0){
                Func = 7;
              }
              continue;
            }

            /// Tiger - 2016-01-28+S
	    if( 0 == Bus  && 0 == Dev && 3 == Func )		
	    {
	       DumpDramPci(); // should dump 0 - 0x300 range.
	    }
		else if(0==Bus && 4==Dev && 0==Func)
		{
		  DumpPcie(Bus,Dev,Func,0x300);
		}
		else if(0==Bus && 0==Dev&& 0==Func)
		{
		   DumpPcie(Bus,Dev,Func,0x420);
		}
		else if(0==Bus && 0==Dev && 5==Func)
		{
		  DumpPcie(Bus,Dev,Func,0x300);
		}
	    else
	    {
               DumpPci(Bus, Dev, Func);
	    }
            /// Tiger - 2016-01-28+E
            
            if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
              Func = 7;
            }
      }
  }
  //Other devices behind P2P bridges(i.e. D2F0~D7F0)
  for(Index = 2; Index <= 7; Index++) {
  	Dev = Bus0Dev[Index];
  	Base = PCI_DEV_MMBASE(0, Dev, 0);
	if(MmioRead16(Base+0) == 0xFFFF) continue;
	if((Bus = MmioRead8(Base + 0x19)) == 0) continue;
    //OK, a valid bus number
	Dev = 0;
	for(Func = 0; Func < 8; Func++) {
      Base = PCI_DEV_MMBASE(Bus, Dev, Func);
      if(MmioRead16(Base+0) == 0xFFFF){    // not present
        if(Func == 0){
            Func = 7;
        }
          continue;
       }
            
       DumpPci(Bus, Dev, Func);
            
       if(Func == 0 && !(MmioRead8(Base + 0xE) & BIT7)){
          Func = 7;
        }
      }
  }

 // MmioBase = MmioRead32(PCI_DEV_MMBASE(0, 17, 0) + 0xBC);
 // MmioBase &= 0xfffff0;
  MmioBase = 0xFED12000; 
  DEBUG((EFI_D_ERROR,"======Dump D17F0 MMIO Space==========\n"));
  DumpMem8((VOID*)(UINTN)MmioBase, 0x58);
  DEBUG((EFI_D_ERROR,"======Dump PMIO Space==========\n"));
  DumpIo4(0x800, 0x14);
  DumpIo4(0x820, 0xD0);
//Yankui-dbg-e
//DumpAllPci();

// PM 14 ~ 18  CState
//    19 ~ 1F  Reserved
//DumpIo4(0x800, 0x14);
//DumpIo4(0x820, 0x70);  
}

///
VOID DumpHob()
{
  EFI_PEI_HOB_POINTERS  Hob;

  for (Hob.Raw = GetHobList(); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
	  DEBUG((EFI_D_ERROR, "T:%d A:%X\n", GET_HOB_TYPE(Hob), Hob));
  }
}



//---------------------------------- PS2 ---------------------------------------
#define  CMD_READ_OUTPUT_PORT     0xD0
#define  CMD_WRITE_OUTPUT_PORT    0xD1
#define    OUTPUT_PORT_GATEA20      BIT1
#define  KBC_DISKEYBD_INF        0xAD
#define  KBC_ENAKEYBD_INF        0xAE
#define  KBC_WRITMSD_BYTE        0xD4
#define  MS_DISABLE_REPORT       0xF5
#define  KB_ENABLE_SCANING       0xF4
#define  KBC_ACK_STATUS          0xFA

#define KBC_STS_ERR0   0
#define KBC_STS_ERR1   0xFF
#define KBC_CMD_PORT   0x64
#define KBC_STS_PORT   0x64
#define KBC_OUT_BUFFUL BIT0
#define KBC_IN_BUFFUL  BIT1  
#define KBC_DAT_PORT   0x60
#define KBC_TIMEOUT    65536   // 0.07s

EFI_STATUS KbcWaitInputBufferFree()
{
  UINT32  TimeOut;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (!(IoRead8(KBC_STS_PORT) & KBC_IN_BUFFUL)) {
      break;
    }
    MicroSecondDelay(30);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_TIMEOUT;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcWaitOutputBufferFull()
{
  UINT32  TimeOut;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (IoRead8(KBC_STS_PORT) & KBC_OUT_BUFFUL) {
      break;
    }
    MicroSecondDelay(30);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_TIMEOUT;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcWaitOutputBufferFree()
{
  UINT32  TimeOut;
  UINT8   Data8;
  
  for (TimeOut = 0; TimeOut < KBC_TIMEOUT; TimeOut += 30) {
    if (!(IoRead8(KBC_STS_PORT) & KBC_OUT_BUFFUL)) {
      break;
    }
    Data8 = IoRead8(KBC_DAT_PORT);
  }
  if(TimeOut == KBC_TIMEOUT){
    return EFI_DEVICE_ERROR;
  }else{
    return EFI_SUCCESS;
  }
}

EFI_STATUS KbcSendCmd(UINT8 Cmd)
{
  EFI_STATUS  Status;

  Status = KbcWaitInputBufferFree();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  IoWrite8(KBC_CMD_PORT, Cmd);
  Status = KbcWaitInputBufferFree();
  
ProcExit:
  return Status;  
}


BOOLEAN CheckKbcPresent(VOID)
{
  UINT8   KbcSts;

  KbcSts = IoRead8 (KBC_CMD_PORT);

  if (KbcSts == KBC_STS_ERR0) {
    return FALSE;
  }

  if (KbcSts == KBC_STS_ERR1) {
    return FALSE;
  }

  return TRUE;
}


EFI_STATUS
KbcCmdReadData (
  IN  UINT8  Cmd,
  OUT UINT8  *Data  OPTIONAL
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IntState;
  UINT8       Data8;

  IntState = SaveAndDisableInterrupts();  
  
  if(!CheckKbcPresent()){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Status = KbcWaitOutputBufferFree();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  Status = KbcSendCmd(Cmd);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  Status = KbcWaitOutputBufferFull();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  Data8 = IoRead8(KBC_DAT_PORT);
  if(Data != NULL){
    *Data = Data8;
  }

ProcExit: 
  SetInterruptState(IntState);
  return Status;
}



BOOLEAN CheckKbPresent(VOID)
{
  UINT8   KbcSts;
  BOOLEAN KeyboardFlag;
  EFI_STATUS  Status;
  UINT8       Loop = 4;

  KeyboardFlag = FALSE;
  while (Loop) {
    KbcSts = IoRead8 (KBC_CMD_PORT);
    if ((KbcSts & KBC_OUT_BUFFUL) == KBC_OUT_BUFFUL) {
      KbcSts = IoRead8 (KBC_DAT_PORT);
    }
  
    //
    // Disable keyboard interface of Kbc.
    //
    Status = KbcSendCmd(KBC_DISKEYBD_INF);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    } 
  
    //
    // Send the keyboard enable scaning command.
    //
    IoWrite8 (KBC_DAT_PORT, KB_ENABLE_SCANING);
  
    Status = KbcWaitOutputBufferFull();
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }
    KbcSts = IoRead8 (KBC_DAT_PORT);
    if (KbcSts == KBC_ACK_STATUS) {
      KeyboardFlag = TRUE;
      break;
    }

    KbcSendCmd (KBC_ENAKEYBD_INF);

  ProcExit:
    Loop --;
  }
  return KeyboardFlag;
}

BOOLEAN CheckMsPresent(VOID)
{
  UINT8   KbcSts;
  BOOLEAN MouseFlag;
  EFI_STATUS  Status;
  UINT8       Loop = 4;

  MouseFlag = FALSE;
  while (Loop) {
    KbcSts = IoRead8 (KBC_CMD_PORT);
    if ((KbcSts & KBC_OUT_BUFFUL) == KBC_OUT_BUFFUL) {
      KbcSts = IoRead8 (KBC_DAT_PORT);
    }
  
    //
    // Disable keyboard interface of Kbc.
    //
    Status = KbcSendCmd(KBC_DISKEYBD_INF);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    } 
    //
    // Send the mouse disable report command.
    //
    Status = KbcSendCmd(KBC_WRITMSD_BYTE);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    } 
  
    IoWrite8 (KBC_DAT_PORT, MS_DISABLE_REPORT);
  
    Status = KbcWaitOutputBufferFull();
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }
  
    KbcSts = IoRead8 (KBC_DAT_PORT);
    if (KbcSts == KBC_ACK_STATUS) {
      MouseFlag = TRUE;
      break;
    }
  
    KbcSendCmd(KBC_ENAKEYBD_INF);
  
  ProcExit:
    Loop --;
  }
  return MouseFlag;
}


//--------------------------------- CRC32 --------------------------------------
STATIC UINT32 gCrcTable[256] = {
  0x00000000, 
  0x77073096, 
  0xEE0E612C, 
  0x990951BA, 
  0x076DC419, 
  0x706AF48F, 
  0xE963A535, 
  0x9E6495A3, 
  0x0EDB8832, 
  0x79DCB8A4, 
  0xE0D5E91E, 
  0x97D2D988, 
  0x09B64C2B, 
  0x7EB17CBD, 
  0xE7B82D07, 
  0x90BF1D91, 
  0x1DB71064, 
  0x6AB020F2, 
  0xF3B97148, 
  0x84BE41DE, 
  0x1ADAD47D, 
  0x6DDDE4EB, 
  0xF4D4B551, 
  0x83D385C7, 
  0x136C9856, 
  0x646BA8C0, 
  0xFD62F97A, 
  0x8A65C9EC, 
  0x14015C4F, 
  0x63066CD9, 
  0xFA0F3D63, 
  0x8D080DF5, 
  0x3B6E20C8, 
  0x4C69105E, 
  0xD56041E4, 
  0xA2677172, 
  0x3C03E4D1, 
  0x4B04D447, 
  0xD20D85FD, 
  0xA50AB56B, 
  0x35B5A8FA, 
  0x42B2986C, 
  0xDBBBC9D6, 
  0xACBCF940, 
  0x32D86CE3, 
  0x45DF5C75, 
  0xDCD60DCF, 
  0xABD13D59, 
  0x26D930AC, 
  0x51DE003A, 
  0xC8D75180, 
  0xBFD06116, 
  0x21B4F4B5, 
  0x56B3C423, 
  0xCFBA9599, 
  0xB8BDA50F, 
  0x2802B89E, 
  0x5F058808, 
  0xC60CD9B2, 
  0xB10BE924, 
  0x2F6F7C87, 
  0x58684C11, 
  0xC1611DAB, 
  0xB6662D3D, 
  0x76DC4190, 
  0x01DB7106, 
  0x98D220BC, 
  0xEFD5102A, 
  0x71B18589, 
  0x06B6B51F, 
  0x9FBFE4A5, 
  0xE8B8D433, 
  0x7807C9A2, 
  0x0F00F934, 
  0x9609A88E, 
  0xE10E9818, 
  0x7F6A0DBB, 
  0x086D3D2D, 
  0x91646C97, 
  0xE6635C01, 
  0x6B6B51F4, 
  0x1C6C6162, 
  0x856530D8, 
  0xF262004E, 
  0x6C0695ED, 
  0x1B01A57B, 
  0x8208F4C1, 
  0xF50FC457, 
  0x65B0D9C6, 
  0x12B7E950, 
  0x8BBEB8EA, 
  0xFCB9887C, 
  0x62DD1DDF, 
  0x15DA2D49, 
  0x8CD37CF3, 
  0xFBD44C65, 
  0x4DB26158, 
  0x3AB551CE, 
  0xA3BC0074, 
  0xD4BB30E2, 
  0x4ADFA541, 
  0x3DD895D7, 
  0xA4D1C46D, 
  0xD3D6F4FB, 
  0x4369E96A, 
  0x346ED9FC, 
  0xAD678846, 
  0xDA60B8D0, 
  0x44042D73, 
  0x33031DE5, 
  0xAA0A4C5F, 
  0xDD0D7CC9, 
  0x5005713C, 
  0x270241AA, 
  0xBE0B1010, 
  0xC90C2086, 
  0x5768B525, 
  0x206F85B3, 
  0xB966D409, 
  0xCE61E49F, 
  0x5EDEF90E, 
  0x29D9C998, 
  0xB0D09822, 
  0xC7D7A8B4, 
  0x59B33D17, 
  0x2EB40D81, 
  0xB7BD5C3B, 
  0xC0BA6CAD, 
  0xEDB88320, 
  0x9ABFB3B6, 
  0x03B6E20C, 
  0x74B1D29A, 
  0xEAD54739, 
  0x9DD277AF, 
  0x04DB2615, 
  0x73DC1683, 
  0xE3630B12, 
  0x94643B84, 
  0x0D6D6A3E, 
  0x7A6A5AA8, 
  0xE40ECF0B, 
  0x9309FF9D, 
  0x0A00AE27, 
  0x7D079EB1, 
  0xF00F9344, 
  0x8708A3D2, 
  0x1E01F268, 
  0x6906C2FE, 
  0xF762575D, 
  0x806567CB, 
  0x196C3671, 
  0x6E6B06E7, 
  0xFED41B76, 
  0x89D32BE0, 
  0x10DA7A5A, 
  0x67DD4ACC, 
  0xF9B9DF6F, 
  0x8EBEEFF9, 
  0x17B7BE43, 
  0x60B08ED5, 
  0xD6D6A3E8, 
  0xA1D1937E, 
  0x38D8C2C4, 
  0x4FDFF252, 
  0xD1BB67F1, 
  0xA6BC5767, 
  0x3FB506DD, 
  0x48B2364B, 
  0xD80D2BDA, 
  0xAF0A1B4C, 
  0x36034AF6, 
  0x41047A60, 
  0xDF60EFC3, 
  0xA867DF55, 
  0x316E8EEF, 
  0x4669BE79, 
  0xCB61B38C, 
  0xBC66831A, 
  0x256FD2A0, 
  0x5268E236, 
  0xCC0C7795, 
  0xBB0B4703, 
  0x220216B9, 
  0x5505262F, 
  0xC5BA3BBE, 
  0xB2BD0B28, 
  0x2BB45A92, 
  0x5CB36A04, 
  0xC2D7FFA7, 
  0xB5D0CF31, 
  0x2CD99E8B, 
  0x5BDEAE1D, 
  0x9B64C2B0, 
  0xEC63F226, 
  0x756AA39C, 
  0x026D930A, 
  0x9C0906A9, 
  0xEB0E363F, 
  0x72076785, 
  0x05005713, 
  0x95BF4A82, 
  0xE2B87A14, 
  0x7BB12BAE, 
  0x0CB61B38, 
  0x92D28E9B, 
  0xE5D5BE0D, 
  0x7CDCEFB7, 
  0x0BDBDF21, 
  0x86D3D2D4, 
  0xF1D4E242, 
  0x68DDB3F8, 
  0x1FDA836E, 
  0x81BE16CD, 
  0xF6B9265B, 
  0x6FB077E1, 
  0x18B74777, 
  0x88085AE6, 
  0xFF0F6A70, 
  0x66063BCA, 
  0x11010B5C, 
  0x8F659EFF, 
  0xF862AE69, 
  0x616BFFD3, 
  0x166CCF45, 
  0xA00AE278, 
  0xD70DD2EE, 
  0x4E048354, 
  0x3903B3C2, 
  0xA7672661, 
  0xD06016F7, 
  0x4969474D, 
  0x3E6E77DB, 
  0xAED16A4A, 
  0xD9D65ADC, 
  0x40DF0B66, 
  0x37D83BF0, 
  0xA9BCAE53, 
  0xDEBB9EC5, 
  0x47B2CF7F, 
  0x30B5FFE9, 
  0xBDBDF21C, 
  0xCABAC28A, 
  0x53B39330, 
  0x24B4A3A6, 
  0xBAD03605, 
  0xCDD70693, 
  0x54DE5729, 
  0x23D967BF, 
  0xB3667A2E, 
  0xC4614AB8, 
  0x5D681B02, 
  0x2A6F2B94, 
  0xB40BBE37, 
  0xC30C8EA1, 
  0x5A05DF1B, 
  0x2D02EF8D
};


EFI_STATUS 
EFIAPI
LibCalcCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
{
  UINT32  Crc;
  UINTN   Index;
  UINT8   *Ptr;

  if (Data == NULL || DataSize == 0 || CrcOut == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Crc = 0xffffffff;
  for (Index = 0, Ptr = Data; Index < DataSize; Index++, Ptr++) {
    Crc = (Crc >> 8) ^ gCrcTable[(UINT8) Crc ^ *Ptr];
  }

  *CrcOut = Crc ^ 0xffffffff;
  return EFI_SUCCESS;
}

BOOLEAN LibVerifyDataCrc32(VOID *Data, UINTN DataSize, UINTN CrcOffset, UINT32 *CalcCrc32 OPTIONAL)
{
  UINT32      OldCrc32;
  UINT32      Crc32;
  EFI_STATUS  Status;
  BOOLEAN     rc;
  UINT32      *CrcPos;
  
  CrcPos   = (UINT32*)((UINT8*)Data+CrcOffset);
  OldCrc32 = *CrcPos;
  *CrcPos  = 0;
  rc       = FALSE;
  
  Status  = LibCalcCrc32(Data, DataSize, &Crc32);
  *CrcPos = OldCrc32;
  if(CalcCrc32 != NULL){
    *CalcCrc32 = Crc32;
  }
    
  if((!EFI_ERROR(Status)) && (Crc32==OldCrc32)){
    rc = TRUE;
  }

  return rc;
}






//-------------------------------- Azalia --------------------------------------
#define REG_HDA_GCAP            0x00      // 2
#define REG_HDA_VMIN            0x02      // 1
#define REG_HDA_VMAJ            0x03      // 1
#define REG_HDA_OUTPAY          0x04      // 2
#define REG_HDA_INPAY           0x06      // 2
#define REG_HDA_GCTL            0x08      // 4
#define   REG_HDA_GCTL_RESET      BIT0
#define REG_HDA_WAKEEN          0x0C      // 2
#define REG_HDA_STATESTS        0x0E      // 2
#define REG_HDA_GSTS            0x10      // 2
#define REG_HDA_IC              0x60      // 4
#define REG_HDA_IR              0x64      // 4
#define REG_HDA_IRS             0x68      // 4
#define   REG_HDA_IRS_BUSY        BIT0
#define   REG_HDA_IRS_VALID       BIT1
#define REG_HDA_OSD0CTL         0x100     // 3
#define REG_HDA_OSD0CBL         0x108     // 4
#define REG_HDA_OSD0LVI         0x10C     // 2
#define REG_HDA_OSD0FIFOS       0x110     // 2
#define REG_HDA_OSD0FMT         0x112     // 2
#define REG_HDA_OSD0BDPL        0x118     // 4
#define REG_HDA_OSD0BDPU        0x11C     // 4

#define ROOT_NODE_ID		                0x00

#define VERB_GET_PARAMETER  		        0xF00
#define PARAMETER_ID_VENDOR_ID          0x00

#define CREATE_VERB12_FOEMAT(CAd, NodeID, VerbID, PayLoad) \
  (UINT32)((((CAd)&0x0F)<<28) | (((NodeID)&0xFF)<<20) | (((VerbID)&0xFFF)<<8) | ((PayLoad)&0xFF))


  
STATIC 
EFI_STATUS 
AzaliaSendCommand (
  UINTN   HdaBar, 
  UINT32  Verb, 
  UINT32  *Res  OPTIONAL
  )
{
  UINTN       TimeOut;
  UINTN       Address;
  EFI_STATUS  Status;

//DEBUG((EFI_D_INFO, "Verb:%08X\n", Verb));  
  Status  = EFI_SUCCESS;  
  TimeOut = AZALIA_MAX_LOOP_TIME;
  Address = HdaBar + REG_HDA_IRS;
  while(TimeOut--){
    if(!(MmioRead32(Address)&REG_HDA_IRS_BUSY)){
      break;
    }
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }
  if(MmioRead32(Address)&REG_HDA_IRS_BUSY){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;
  }
        
  MmioWrite32(Address, MmioRead32(Address)|REG_HDA_IRS_VALID);  // clear
  MmioWrite32(HdaBar+REG_HDA_IC, Verb);
  MmioWrite32(Address, MmioRead32(Address)|REG_HDA_IRS_BUSY);   // start
  
  TimeOut = AZALIA_MAX_LOOP_TIME;
  while(TimeOut--){
    if(MmioRead32(Address)&REG_HDA_IRS_VALID){
      break;
    }
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }
  if(!(MmioRead32(Address)&REG_HDA_IRS_VALID)){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }
  
  if(Res!=NULL){
    *Res = MmioRead32(HdaBar+REG_HDA_IR);
  }

ProcExit:
  return Status;    
}


STATIC EFI_STATUS AzaliaReset(UINTN HdaBar)
{
  UINTN       TimeOut;
  UINTN       Address;
  EFI_STATUS  Status;
  
  Status = EFI_SUCCESS;
  
// 1. clear STATESTS
  Address = HdaBar + REG_HDA_STATESTS;
  MmioOr8(Address, AZALIA_MAX_SID_MASK);

// 2. reset controller
  Address = HdaBar + REG_HDA_GCTL;
  MmioWrite32(Address, MmioRead32(Address) & ~REG_HDA_GCTL_RESET);
	TimeOut = AZALIA_MAX_LOOP_TIME;
	while((MmioRead32(Address)&REG_HDA_GCTL_RESET) && TimeOut--){
		MicroSecondDelay(AZALIA_WAIT_PERIOD);
	}
  if(MmioRead32(Address)&REG_HDA_GCTL_RESET){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }

  MicroSecondDelay(AZALIA_RESET_WAIT_TIME);  
  
// 3. Bring controller out of reset
  Address = HdaBar + REG_HDA_GCTL;
  MmioWrite32(Address, MmioRead32(Address) | REG_HDA_GCTL_RESET);
	TimeOut = AZALIA_MAX_LOOP_TIME;
	while((!(MmioRead32(Address)&REG_HDA_GCTL_RESET)) && TimeOut--){
		MicroSecondDelay(AZALIA_WAIT_PERIOD);
	}
  if(!(MmioRead32(Address)&REG_HDA_GCTL_RESET)){
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }
  
ProcExit:
  return Status;
}



STATIC UINTN GetHdaBar()
{
  DATA_64  Data64;
  UINTN    HdaBar;

  Data64.Uint64 = 0;
  Data64.Uint32.Lower32 = MmioRead32(HDAC_PCI_REG(0x10));

  if((Data64.Uint32.Lower32 & (BIT1 + BIT2)) == BIT2){         // 64bit Address.
    Data64.Uint32.Upper32 = MmioRead32(HDAC_PCI_REG(0x14));
  }
  HdaBar = (UINTN)(Data64.Uint64 & ~(BIT0 + BIT1 + BIT2 + BIT3));
  ASSERT(HdaBar!=0);
  ASSERT(HdaBar < SIZE_4GB);
  
  return HdaBar;
}


// Only handle the first found codec.
EFI_STATUS 
AzaliaLoadVerbTable(
  IN OEM_VERB_TABLE  *OemVerbTable, 
  IN UINTN           OemVerbTableSize
  )
{
	EFI_STATUS		      Status;
  UINTN               HdaBar;
  UINT16              CodecMask;  
  UINT8               CAd;
  UINT32              Verb;
  UINT32              RetVal;
  UINTN               TableIndex;
  UINTN               TableCount;  
  UINTN               VerbIndex;
  UINTN               VerbCount;
  UINT32              *VerbData;
  UINTN               LoopTime;
  
  
  ASSERT(OemVerbTable!=NULL && OemVerbTableSize!=0);

  if(MmioRead16(HDAC_PCI_REG(PCI_VID_REG))==0xFFFF){
    DEBUG((EFI_D_INFO, "Azalia Not Present\n"));
    Status = EFI_SUCCESS;
    goto ProcExit;		
  }		

  HdaBar = GetHdaBar();
  DEBUG((EFI_D_INFO, "HdaBar:%X\n", HdaBar));  
  Status = AzaliaReset(HdaBar);
  ASSERT_EFI_ERROR(Status);
  
  CodecMask = 0;
  for (LoopTime = 0; LoopTime < AZALIA_MAX_LOOP_TIME; LoopTime++) {
    CodecMask = MmioRead16(HdaBar + REG_HDA_STATESTS) & AZALIA_MAX_SID_MASK;
    if(CodecMask!=0){
      break;
    } 
    MicroSecondDelay(AZALIA_WAIT_PERIOD);
  }  
  if(!CodecMask){
    DEBUG((EFI_D_ERROR, "No Codec Detect!\n"));
    goto ProcExit;
  }

  TableCount = OemVerbTableSize/sizeof(OEM_VERB_TABLE);  
  for(CAd=0;CAd<AZALIA_MAX_SID_NUMBER;CAd++){
    if(!((UINT16)(1<<CAd) & CodecMask)){
      continue;
    }
    Verb   = CREATE_VERB12_FOEMAT(CAd, ROOT_NODE_ID, VERB_GET_PARAMETER, PARAMETER_ID_VENDOR_ID);
    Status = AzaliaSendCommand(HdaBar, Verb, &RetVal);
    ASSERT_EFI_ERROR(Status);
    DEBUG((EFI_D_ERROR, "SDI[%d] %08X\n", CAd, RetVal));
    
    for(TableIndex=0;TableIndex<TableCount;TableIndex++){
      if(RetVal != OemVerbTable[TableIndex].Hdr.Vdid){
        continue;
      }
      VerbCount = OemVerbTable[TableIndex].VerbDataSize/sizeof(UINT32);
      VerbData  = OemVerbTable[TableIndex].VerbData;
      for(VerbIndex=0;VerbIndex<VerbCount;VerbIndex++){
        if(VerbData[VerbIndex] == 0 || VerbData[VerbIndex] == 0xFFFFFFFF){
          continue;
        }
        Verb   = (VerbData[VerbIndex] & 0x0FFFFFFF) | (CAd<<28);
        Status = AzaliaSendCommand(HdaBar, Verb, &RetVal);
        ASSERT_EFI_ERROR(Status);
      }
      break;
    }
    if(TableIndex>=TableCount){
      DEBUG((EFI_D_ERROR, "VerbTable Not Found!\n"));
    }
    break;
  }
  
ProcExit:
  return Status;  
}


/*
(0, F, 0, A0)[0] - PM device detected
(0, F, 0, A1)[0] - PS device detected
(0, F, 0, A2)[0] - SM device detected
(0, F, 0, A3)[0] - SS device detected
*/ 
// assume this host is under IDE mode.
UINT8 WaitIdeDeviceReady(UINTN Bus, UINTN Dev, UINTN Func, UINTN DetectTimeOutInms)
{
        UINT8       PortStatus[4];
        UINT16      CmdPort[2];
        UINTN       Count;
  CONST UINTN       TimeOut = 3000;
        UINT8       Interface;
        UINTN       SataPciBase;
        UINT8       ChannelCount;
        UINT8       Ch0, Ch1;	
        BOOLEAN     DeviceDetected;


  SataPciBase = PCI_DEV_MMBASE(Bus, Dev, Func);

  Interface = MmioRead8(SataPciBase + PCI_CLASSCODE_OFFSET);
  if(Interface & BIT0){
    CmdPort[0] = MmioRead16(SataPciBase + PCI_BASE_ADDRESSREG_OFFSET) & ((UINT16)~BIT0);
  } else {
    CmdPort[0] = 0x1F0;
  }
  if(Interface & BIT2){
    CmdPort[1] = MmioRead16(SataPciBase + PCI_BASE_ADDRESSREG_OFFSET + 8) & ((UINT16)~BIT0);
  } else {
    CmdPort[1] = 0x170;
  }	
  DEBUG((EFI_D_INFO, "CmdPort: %04X, %04X\n", CmdPort[0], CmdPort[1]));

  MmioOr8 (SataPciBase + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_IO_SPACE);

  DeviceDetected = FALSE;
  Count = DetectTimeOutInms;
  while(Count--){
    *(UINT32*)&PortStatus[0] = MmioRead32(SataPciBase + 0xA0);
    if((PortStatus[0] & BIT0) || (PortStatus[1] & BIT0) || 
       (PortStatus[2] & BIT0) || (PortStatus[3] & BIT0)){   // Device Detected
      DeviceDetected = TRUE; 
      MicroSecondDelay(100000);
      break;
    }
    MicroSecondDelay(1000);
  }
  *(UINT32*)&PortStatus[0] = MmioRead32(SataPciBase + 0xA0);  

  if(!DeviceDetected) {
    DEBUG((EFI_D_INFO, "(%X,%X,%X) No IDE Device Found!\n", Bus, Dev, Func));
		ChannelCount = 0;
    goto ProcExit;
  }
	
  if(PortStatus[0] & BIT0){                     // port 0 present
    IoWrite8(CmdPort[0]+6, 0xE0);
    MicroSecondDelay(1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[0]+7) & BIT7)){      // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "PM_Sts:%X <%dms\n", IoRead8(CmdPort[0]+7), TimeOut-Count));
  }
  
  if(PortStatus[1] & BIT0){                     // port 1 present
    IoWrite8(CmdPort[0]+6, 0xF0);
    MicroSecondDelay (1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[0]+7) & BIT7)){      // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "PS_Sts:%X <%dms\n", IoRead8(CmdPort[0]+7), TimeOut-Count));
  }

  if(PortStatus[2] & BIT0){                     // port 2 present
    IoWrite8(CmdPort[1]+6, 0xE0);
    MicroSecondDelay(1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[1]+7) & BIT7)){      // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "SM_Sts:%X <%dms\n", IoRead8(CmdPort[1]+7), TimeOut-Count));
  }
  
  if(PortStatus[3] & BIT0){                     // port 3 present
    IoWrite8(CmdPort[1]+6, 0xF0);
    MicroSecondDelay (1000);
    Count = TimeOut;
    while(Count--){
      if(!(IoRead8(CmdPort[1]+7) & BIT7)){      // not busy
        break;
      }
      MicroSecondDelay (1000);
    }
    DEBUG((EFI_D_ERROR, "SS_Sts:%X <%dms\n", IoRead8(CmdPort[1]+7), TimeOut-Count));
  }

  Ch0 = (PortStatus[0] & BIT0) || (PortStatus[1] & BIT0);
  Ch1 = (PortStatus[2] & BIT0) || (PortStatus[3] & BIT0);
// CH0  CH1  Count
//   0    0      0
//   0    1      2
//   1    0      1
//   1    1      2
  if(Ch1){
    ChannelCount = 2;
  } else {
    ChannelCount = 1;
  }

ProcExit:
  DEBUG((EFI_D_INFO, "%a(%X,%X,%X) ChCount:%d\n", __FUNCTION__, Bus, Dev, Func, ChannelCount));	
  return ChannelCount;
}


// I am a common library, I can not use Arch dependent library.
// So below gBS should be passed from caller.
VOID *LibGetGOP(EFI_BOOT_SERVICES *pBS)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *ptGO;
  VOID                                  *Interface;
  EFI_STATUS                            Status;  
  EFI_HANDLE                            *HandleBuffer;
  UINTN                                 HandleCount;
  UINTN                                 Index;  

  HandleBuffer = NULL;
  ptGO         = NULL;
  
  Status = pBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiGraphicsOutputProtocolGuid,
              NULL,
              &HandleCount,
              &HandleBuffer
              );
  if(EFI_ERROR(Status) || HandleCount==0){
    goto ProcExit;
  }  
              
  for(Index=0;Index<HandleCount;Index++){
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiConsoleOutDeviceGuid, 
               &Interface
               );
    if(EFI_ERROR(Status)){
      continue;
    }
    Status = pBS->HandleProtocol(
               HandleBuffer[Index], 
               &gEfiGraphicsOutputProtocolGuid, 
               &ptGO
               );
    ASSERT(!EFI_ERROR(Status) && ptGO!=NULL);               
  }

ProcExit:
  pBS->FreePool(HandleBuffer);
  return (VOID*)ptGO;
}



EFI_STATUS BltSaveAndRetore(VOID *BootServices, BOOLEAN Save)
{
         EFI_STATUS                     Status = EFI_SUCCESS;
         EFI_GRAPHICS_OUTPUT_PROTOCOL   *ptGO;
  STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *gBltBuffer = NULL;
         UINTN                          Width;
         UINTN                          Height;
         EFI_BOOT_SERVICES              *pBS;      
  

  pBS = (EFI_BOOT_SERVICES*)BootServices;
  ASSERT(pBS->Hdr.Signature == EFI_BOOT_SERVICES_SIGNATURE);
  
  ptGO = (EFI_GRAPHICS_OUTPUT_PROTOCOL*)LibGetGOP(pBS);
  if(ptGO == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }    
  
  Width  = ptGO->Mode->Info->HorizontalResolution;
  Height = ptGO->Mode->Info->VerticalResolution;

  if(Save){
    if(gBltBuffer != NULL){
      pBS->FreePool(gBltBuffer);
      gBltBuffer = NULL;
    }  
    Status = pBS->AllocatePool(
                    EfiBootServicesData,
                    sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)*Width*Height,
                    &gBltBuffer
                    );
    if(EFI_ERROR(Status)){
      Status = EFI_OUT_OF_RESOURCES;
      goto ProcExit;		
    }	

    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltVideoToBltBuffer,
                    0,
                    0,
                    0,
                    0,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)   // the number of bytes in a row of the BltBuffer.
                    );  
    DEBUG((EFI_D_INFO, "BltSave:%r\n", Status));    
  } else {
    
    if(gBltBuffer == NULL){
      Status = EFI_INVALID_PARAMETER;
      goto ProcExit;	      
    }
    Status = ptGO->Blt(
                    ptGO,
                    gBltBuffer,
                    EfiBltBufferToVideo,
                    0,
                    0,
                    0,
                    0,
                    Width,
                    Height,
                    Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                    );
    DEBUG((EFI_D_INFO, "BltRestore:%r\n", Status));
    pBS->FreePool(gBltBuffer);
    gBltBuffer = NULL;    
  }
	
ProcExit:
  return Status;	
}



VOID 
AcpiTableUpdateChksum (
  IN VOID *AcpiTable
  )
{
  UINTN  ChecksumOffset;
  UINT8  *Buffer;
  EFI_ACPI_COMMON_HEADER  *Hdr;

  Hdr = (EFI_ACPI_COMMON_HEADER*)AcpiTable;
  if(Hdr->Signature == EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE){
    return;
  }  

  Buffer = (UINT8*)Hdr;
  ChecksumOffset = OFFSET_OF(EFI_ACPI_DESCRIPTION_HEADER, Checksum);
  Buffer[ChecksumOffset] = 0;
  Buffer[ChecksumOffset] = CalculateCheckSum8(Buffer, Hdr->Length);
}

#define gPciRootBridge \
  { \
    ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID (0x0A03), 0 \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 \
  } 

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2;

STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2 gObLanDpData = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    ONBOARD_LAN_BRIDGE_DEV_NUM
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

VOID  *gObLanDp    = (VOID*)&gObLanDpData;
UINTN gObLanDpSize = sizeof(PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2);


UINTN
MyGetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!(DevicePath->Type == END_DEVICE_PATH_TYPE && 
           DevicePath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)DevicePath + (DevicePath->Length[1]<<8) + DevicePath->Length[0]);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN) DevicePath - (UINTN)Start) + (DevicePath->Length[1]<<8) + DevicePath->Length[0];
}


typedef struct {
  UINTN  Bus;
  UINTN  Dev;
  UINTN  Func;
  UINT16  IndexBase;
} SATA_PORT_INDEX_LIST;


SATA_PORT_INDEX_LIST gSataPortIndexList[] = {
  {0, 15, 0, 0},
};  

UINT16 GetSataPortIndexBase(UINTN Bus, UINTN Dev, UINTN Func)
{
  UINTN  ListIndex;

  for(ListIndex=0;ListIndex<sizeof(gSataPortIndexList)/sizeof(gSataPortIndexList[0]);ListIndex++){
    if(gSataPortIndexList[ListIndex].Bus == Bus &&
       gSataPortIndexList[ListIndex].Dev == Dev &&
       gSataPortIndexList[ListIndex].Func == Func){
      return gSataPortIndexList[ListIndex].IndexBase;
    }     
  } 

  return 0xFFFF;
}


UINT16 
GetOnboardSataPortIndexFromDiskInfo (
  IN EFI_HANDLE          Handle,
  IN EFI_BOOT_SERVICES   *pBS
  )
{
  EFI_STATUS                    Status;
  UINT16                        DevIndex = 0xFFFF;
  EFI_DISK_INFO_PROTOCOL        *DiskInfo;
  EFI_PCI_IO_PROTOCOL           *PciIo;  
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  EFI_DEVICE_PATH_PROTOCOL      *TempDp;  
  EFI_HANDLE                    PciHandle;
  UINT32                        IdeChannel;	
  UINT32                        IdeDevice;
  UINT8                         SataMode;
  UINTN                         DpSize;
  UINTN                         Seg, Bus, Dev, Func;
  UINT16                        PortBase;
  
  Status = pBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID**)&DiskInfo
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)){
    SataMode = 1;
  } else if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)){
    SataMode = 6;
  } else {
    goto ProcExit;
  }

  Status = DiskInfo->WhichIde (
                       DiskInfo,
                       &IdeChannel,
                       &IdeDevice
                       );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }	  
  DEBUG((EFI_D_INFO, "IDE(%d,%d)\n", IdeChannel, IdeDevice));

  Status = pBS->HandleProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  &PciIo
                  );
  if(!EFI_ERROR(Status)){
    goto PciIoFound;
  }
  
  Status = pBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&DevicePath
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  PciHandle = NULL;
  DpSize = MyGetDevicePathSize(DevicePath);
  Status = pBS->AllocatePool(EfiBootServicesData, DpSize, &Dp);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  CopyMem(Dp, DevicePath, DpSize);
  TempDp = Dp;
  while(!(TempDp->Type == END_DEVICE_PATH_TYPE && TempDp->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
    if(TempDp->Type == MESSAGING_DEVICE_PATH &&
       (TempDp->SubType == MSG_ATAPI_DP || TempDp->SubType == MSG_SATA_DP)) {
      TempDp->Type = END_DEVICE_PATH_TYPE;
      TempDp->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
      TempDp->Length[0] = END_DEVICE_PATH_LENGTH;
      TempDp->Length[1] = 0;  
      TempDp = Dp;
      pBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &TempDp, &PciHandle);
      break;
    }
    TempDp = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)TempDp + (TempDp->Length[1]<<8) + TempDp->Length[0]);
  }
  pBS->FreePool(Dp);
  
  if (PciHandle == NULL) {
    DEBUG((EFI_D_ERROR, "Can not found PciIo in DP\n"));
    goto ProcExit;
  }
  Status = pBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  &PciIo
                  );

PciIoFound:
  PciIo->GetLocation(PciIo, &Seg, &Bus, &Dev, &Func);
  PortBase = GetSataPortIndexBase(Bus, Dev, Func);
  if(PortBase == 0xFFFF){
    goto ProcExit;
  }

  if(SataMode == 1){
    DevIndex = (UINT16)(PortBase + IdeChannel * 2 + IdeDevice);
  } else {             // must AHCI
    DevIndex = (UINT16)(PortBase + IdeChannel);
  }

ProcExit:
  return DevIndex;  
}
