
#include "PlatformPei.h"
#include <ByoCapsuleExt.h>
#include <Library/PerformanceLib.h>
#include <SetupVariable.h>


#define RES_IO_BASE   0x1000
#define RES_IO_LIMIT  0xFFFF

#define DDR3_SPD_MIN_CYCLE_TIME_OFFSET       12
#define DDR4_SPD_MIN_CYCLE_TIME_OFFSET       18

#ifdef IOE_EXIST
EFI_STATUS
HandleIoeMcuXhciFwPei(
  PLATFORM_S3_RECORD *S3Record,
  SETUP_DATA         *SetupData  
	);
#endif

EFI_STATUS 
HandlePemcuFwPei(
  PLATFORM_S3_RECORD *S3Record
  );

STATIC EFI_MEMORY_TYPE_INFORMATION gDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   0x40   },     // ASL
  { EfiACPIMemoryNVS,       0x100  },     // ACPI NVS (including S3 related)
  { EfiReservedMemoryType,  0x100  },     // BIOS Reserved (including S3 related)
  { EfiRuntimeServicesData, 0x50   },     // Runtime Service Data
  { EfiRuntimeServicesCode, 0x50   },     // Runtime Service Code
  { EfiBootServicesData,    0x100  },     // Boot Service Data
  { EfiBootServicesCode,    0x100  },     // Boot Service Code
  { EfiMaxMemoryType,       0      }
};



/**
   Validate variable data for the MemoryTypeInformation. 

   @param MemoryData       Variable data.
   @param MemoryDataSize   Variable data length.

   @return TRUE            The variable data is valid.
   @return FALSE           The variable data is invalid.

**/
STATIC
BOOLEAN
ValidateMemoryTypeInfoVariable (
  IN EFI_MEMORY_TYPE_INFORMATION      *MemoryData,
  IN UINTN                            MemoryDataSize
  )
{
  UINTN                       Count;
  UINTN                       Index;

  // Check the input parameter.
  if (MemoryData == NULL) {
    return FALSE;
  }

  // Get Count
  Count = MemoryDataSize / sizeof (*MemoryData);

  // Check Size
  if (Count * sizeof(*MemoryData) != MemoryDataSize) {
    return FALSE;
  }

  // Check last entry type filed.
  if (MemoryData[Count - 1].Type != EfiMaxMemoryType) {
    return FALSE;
  }

  // Check the type filed.
  for (Index = 0; Index < Count - 1; Index++) {
    if (MemoryData[Index].Type >= EfiMaxMemoryType) {
      return FALSE;
    }
  }

  return TRUE;
}



// 20 - 800
// 15 - 1066
// 12 - 1333
// 10 - 1600
STATIC UINT16 GetMemSpdFreq(UINT8 Tck)
{
  UINT16  Freq;

  switch(Tck){
    case 20:
      Freq = 800;
      break;
			
    case 15:
      Freq = 1066;
      break;
			
    case 12:
      Freq = 1333;
      break;
			
    case 10:
      Freq = 1600;
      break;

    case 9:
      Freq = 1866;
      break;
      
  	case 8:
  	  Freq = 2133;
      break;
      
  	case 7:
  	  Freq = 2400;
      break;
      
  	case 6:
  	  Freq = 2666;
      break;
      
  	case 5:
  	  Freq = 3200;
      break;
			
    default:
      Freq = 0;
      break;
  }
  
  return Freq;
}


UINT32 gFvAddrList[] = {
  _PCD_VALUE_PcdFlashFvMainBase,    // PcdGet32(PcdFlashFvMainBase)
  _PCD_VALUE_PcdFlashFvMain2Base,
  _PCD_VALUE_PcdFlashNvLogoBase
};


VOID
ReportResourceForDxe (
  IN  EFI_PEI_SERVICES      **PeiServices,
  IN  PLATFORM_MEMORY_INFO  *MemInfo,
  IN  EFI_BOOT_MODE         BootMode
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE      Attributes;  
  UINTN                            DataSize;
  UINT32                           FvAddr;
  UINT32                           FvSize;
  VOID                             *FvData;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;  
  EFI_MEMORY_TYPE_INFORMATION      MemoryData[EfiMaxMemoryType + 1];
  EFI_STATUS                       Status;
  UINTN                            Index;	
  EFI_ASIA_DRAM_PPI                *DramPpi;
  ASIA_DRAM_INFO                   DramInfo;	
  PLAT_DIMM_INFO                   *DimmInfo;
  UINTN                            DimmInfoSize;
  UINT32                           PmmBase;
  UINT32                           PmmSize;
  UINT32                           UsableSize;
  CONST UINT8                      *Spd;
  UINT32                           Address;
  UINT32                           RangeSize;
  EFI_FIRMWARE_VOLUME_HEADER       *FvHdr;


  GetAsiaPpi(NULL, NULL, &DramPpi, NULL, NULL);
  Status = DramPpi->DramGetInfo(PeiServices, DramPpi, &DramInfo);
  ASSERT_EFI_ERROR(Status);
  ASSERT(ASIA_MAX_SOCKETS >= 1);
  
  DimmInfoSize = sizeof(PLAT_DIMM_INFO) + (ASIA_MAX_SOCKETS - 1)*sizeof(DIMM_SPD_INFO);
  DimmInfo = BuildGuidHob(&gEfiPlatDimmInfoGuid, DimmInfoSize);
  ASSERT(DimmInfo != NULL);
  ZeroMem(DimmInfo, DimmInfoSize);  

  DimmInfo->DimmCount = ASIA_MAX_SOCKETS;
  DimmInfo->DimmFreq  = DramInfo.DramFreq;
  for(Index=0;Index<(UINTN)DimmInfo->DimmCount;Index++){	
    DimmInfo->SpdInfo[Index].DimmSize = DramInfo.RankInfo[Index*2].RankSize + 
                                        DramInfo.RankInfo[Index*2+1].RankSize;
    DimmInfo->DimmTotalSizeMB += DimmInfo->SpdInfo[Index].DimmSize;
    if(DramInfo.Spd[Index].SpdPresent){	
      Spd = DramInfo.Spd[Index].Buffer;

      //LGE20160624 Distinguish  ddr3/ddr4
	  if(Spd[2] == 0x0B){
	  //DDR3	
       DimmInfo->SpdInfo[Index].DimmSpeed = GetMemSpdFreq(Spd[DDR3_SPD_MIN_CYCLE_TIME_OFFSET]);
       DimmInfo->SpdInfo[Index].Sn = (UINT32)((Spd[122]<<24)|(Spd[123]<<16)|(Spd[124]<<8)|Spd[125]);
      	   
	  	}
	  else if(Spd[2] == 0x0C)
	  {
      //DDR4
      DimmInfo->SpdInfo[Index].DimmSpeed = GetMemSpdFreq(Spd[DDR4_SPD_MIN_CYCLE_TIME_OFFSET]);
       DimmInfo->SpdInfo[Index].Sn = (UINT32)((Spd[325]<<24)|(Spd[326]<<16)|(Spd[327]<<8)|Spd[328]);
	  }
      CopyMem(DimmInfo->SpdInfo[Index].PartNo, &Spd[128], 18);

	  // Fill the module manufacturer ID code.
      DimmInfo->SpdInfo[Index].Spd320 = DramInfo.Spd[Index].Buffer[320];
      DimmInfo->SpdInfo[Index].Spd321 = DramInfo.Spd[Index].Buffer[321];
  		
      DEBUG((DEBUG_ERROR, "Dimm%d Speed :%d\n", Index, DimmInfo->SpdInfo[Index].DimmSpeed));	
    }			
  }	

  
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);  


   
// Report FV  
  if(BootMode != BOOT_IN_RECOVERY_MODE){

    PERF_START (NULL,"FvMain", NULL, 0);	

    for(Index=0;Index<sizeof(gFvAddrList)/sizeof(gFvAddrList[0]);Index++){
      FvAddr = gFvAddrList[Index];
      if(FvAddr == 0){
        continue;
      }
      FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)FvAddr;
      if(FvHdr->Signature != EFI_FVH_SIGNATURE){
        continue;
      }
      FvSize = (UINT32)FvHdr->FvLength;

// TPM/TCM will Measure FV Data at the callback of FvInfoPpi
// So here we should copy it to memory before, then DxeCore will 
// not read it again to save boot time.
      FvData = AllocatePages(EFI_SIZE_TO_PAGES(FvSize));
      ASSERT(FvData!=NULL);
      CopyMem(FvData, (VOID*)FvHdr, FvSize);
      DEBUG((EFI_D_INFO, "FvMem(%X,%X)\n", FvData, FvSize));
    
      BuildFvHob((UINTN)FvData, FvSize);
      PeiServicesInstallFvInfoPpi(
        NULL,
        FvData,
        FvSize,
        NULL,
        NULL
        ); 
    }
    
    PERF_END(NULL,"FvMain", NULL, 0);	
  }		


  
// Report CPU FV
  BuildCpuHob(MemInfo->PhyAddrBits, 16);
  
  Attributes = 
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    Attributes,
    0,
    0xA0000
    );


  UsableSize = MemInfo->LowMemSize;

  PmmSize = PcdGet32(PcdHighPmmMemorySize);
  if(PmmSize){
    PmmSize = ALIGN_VALUE(PmmSize, EFI_PAGE_SIZE);
    UsableSize -= PmmSize;
    PmmBase     = UsableSize;
    ASSERT((PmmBase & (EFI_PAGE_SIZE - 1)) == 0);
    MemInfo->PmmBase = PmmBase;
    MemInfo->PmmSize = PmmSize;
  } 
  
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    Attributes,
    SIZE_1MB,
    UsableSize - SIZE_1MB    // Memory above 4G will be reported at ReadyToBoot.
    );    
  
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    Attributes,
    MemInfo->LowMemSize,
    S3_PEI_MEMORY_SIZE + S3_DATA_RECORD_SIZE + MemInfo->TSegSize
    );  

//Eric debug for dual socket
  BuildResourceDescriptorHob (
    EFI_RESOURCE_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
    RES_IO_BASE,
    RES_IO_LIMIT - RES_IO_BASE + 1
    );    

  Attributes = EFI_RESOURCE_ATTRIBUTE_PRESENT | 
               EFI_RESOURCE_ATTRIBUTE_INITIALIZED | 
               EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE | 
               EFI_RESOURCE_ATTRIBUTE_TESTED;

  Address = (UINT32)PcdGet64(PcdPciExpressBaseAddress);
  ASSERT(Address >= MemInfo->Tolum);  
  RangeSize = Address - MemInfo->Tolum;
  if(RangeSize){
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      Attributes,
      MemInfo->Tolum,
      RangeSize
      );
  }
  
  Address += SIZE_256MB;
  RangeSize = PCI_MMIO_TOP_ADDRESS - Address;
  if(RangeSize){
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      Attributes,
      Address,
      RangeSize 
      );  
  }
  
  if(MemInfo->Pci64Size) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_MEMORY_MAPPED_IO,
        Attributes,
        MemInfo->Pci64Base,
        MemInfo->Pci64Size
        );
  }
  
  DataSize = sizeof (MemoryData);
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                      &gEfiMemoryTypeInformationGuid,
                      NULL,
                      &DataSize,
                      &MemoryData
                      );
  DEBUG((EFI_D_INFO, "GMTIV:%r\n", Status));	
  if (EFI_ERROR (Status) || !ValidateMemoryTypeInfoVariable(MemoryData, DataSize)) {
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      gDefaultMemoryTypeInformation,
      sizeof(gDefaultMemoryTypeInformation)
      );
  }
}  

EFI_STATUS
EFIAPI
HandleCapsuleBeforeMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo,
  OUT      UINT32                *PeiMemAddr
  )  
{
  EFI_STATUS        Status;
  PEI_CAPSULE_PPI   *Capsule;
  VOID              *CapsuleBuffer;
  UINTN             CapsuleBufferLength;
  UINT32            Memory;
  UINT32            MemEnd;
 
  MemInfo->CapsuleBase = 0;
  MemInfo->CapsuleSize = 0;
  *PeiMemAddr = 0;

  Status = (*PeiServices)->LocatePpi(PeiServices, &gPeiCapsulePpiGuid, 0, NULL, &Capsule);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  CapsuleBuffer = (VOID*)(UINTN)0;
  CapsuleBufferLength = MemInfo->LowMemSize;
  DEBUG((EFI_D_INFO, "CapsuleBuffer(%X,%X)\n", CapsuleBuffer, CapsuleBufferLength));  
  Status = Capsule->Coalesce((EFI_PEI_SERVICES**)PeiServices, &CapsuleBuffer, &CapsuleBufferLength);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  DEBUG((EFI_D_INFO, "Capsule(%X,%X)\n", CapsuleBuffer, CapsuleBufferLength));

  MemInfo->CapsuleBase = (UINT32)(UINTN)CapsuleBuffer;
  MemInfo->CapsuleSize = (UINT32)CapsuleBufferLength;

  MemEnd = MemInfo->LowMemSize - PEI_MEMORY_SIZE;
  for(Memory = SIZE_1MB; Memory < MemEnd; Memory += PEI_MEMORY_SIZE){
    if(Memory + PEI_MEMORY_SIZE < MemInfo->CapsuleBase ||
       MemInfo->CapsuleBase + MemInfo->CapsuleSize < Memory){
      *PeiMemAddr = Memory;
      break;
    }   
  }  

  if(*PeiMemAddr == 0){          // not found
    DEBUG((EFI_D_ERROR, "[ERROR] Could not find PeiMemRange at BIOS_UPDATE mode\n"));
    *PeiMemAddr = MemEnd;
  }  
  
ProcExit:  
  return Status;
}




EFI_STATUS
EFIAPI
HandleCapsuleAfterMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo
  )  
{
  UINT16                           PmEnable;
  EFI_STATUS                       Status;
  PEI_CAPSULE_PPI                  *Capsule;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;
  BYO_CAPSULE_EXTEND               *CapExt;
  UINTN                            Size;


// disable power button SMI with flash update mode
  PmEnable  = IoRead16 (PMIO_REG (PMIO_PM_ENABLE));///PMIO_Rx02[15:0] Power Management Enable
  PmEnable &= (~PMIO_PBTN_EN);
  IoWrite16 (PMIO_REG (PMIO_PM_ENABLE), PmEnable);///PMIO_Rx02[15:0] Power Management Enable

  Status = (*PeiServices)->LocatePpi(PeiServices, &gPeiCapsulePpiGuid, 0, NULL, &Capsule);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  if(MemInfo->CapsuleSize == 0){
    goto ProcExit;
  }  

  Status = Capsule->CreateState((EFI_PEI_SERVICES**)PeiServices, (VOID*)(UINTN)MemInfo->CapsuleBase, MemInfo->CapsuleSize);
  DEBUG((EFI_D_INFO, "CreateState:%r\n", Status));
  
  CapExt = BuildGuidHob(&gEfiCapsuleVendorGuid, sizeof(BYO_CAPSULE_EXTEND));
  ASSERT(CapExt != NULL);
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);
  Size   = sizeof(BYO_CAPSULE_EXTEND);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      EFI_CAPSULE_EXT_VARIABLE_NAME,
                      &gEfiCapsuleVendorGuid,
                      NULL,
                      &Size,
                      CapExt
                      );

ProcExit:  
  return Status;  
}




// RP(0,5,0) by (0,0,5,F4)[10] = 1
VOID 
DisableObLan (
  EFI_ASIA_NB_PPI        *NbPpi,
  EFI_ASIA_SB_PPI        *SbPpi,
  UINT8                  ObLanEn
  )
{
  UINT32                 Base;
  UINT32                 SubBase;
  ASIA_NB_CONFIGURATION  *NbCfg;
  ASIA_SB_CONFIGURATION  *SbCfg;
  UINT16                 PmioBase;
  UINT32                  Data32;
  

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  PmioBase = MmioRead16(PCI_DEV_MMBASE(0,17,0)|D17F0_PMU_PM_IO_BASE) & D17F0_PMU_PMIOBA;  
  NbCfg = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg); 
  if(ObLanEn || !NbCfg->PciePE6 || SbCfg->XhcUartCtrl){
    Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4e[7] 
    Data32 |= BIT23;
    IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx4E[7] = 1
    return;
  }
  
  
  Data32 = IoRead32(PmioBase + PMIO_CR_GPIO_PAD_CTL);//PMIO GPIO 12 RxB4[26:24] = 3'b000
  Data32 &= (~PMIO_PAD_GPIO12_2_1_0);
  IoWrite32(PmioBase + PMIO_CR_GPIO_PAD_CTL, Data32);

  Base = PEG2_PCI_REG(0);
  if(MmioRead16(Base+PCI_VID_REG) == 0xFFFF){
    DEBUG((EFI_D_INFO, "PE6 not found!\n"));
    return;
  }
  
  MmioWrite32(Base+PCI_PBN_REG, 0x000E0E00);
  SubBase = PCI_DEV_MMBASE(0xE, 0, 0);
  
  if(MmioRead32(SubBase+PCI_VID_REG) != 0x816810EC){
    DEBUG((EFI_D_INFO, "ObLan not found!\n"));
    return;
  }

  MmioWrite32(SubBase+PCI_BAR0_REG, 0xD000);
  MmioWrite8(SubBase+PCI_CMD_REG, PCI_CMD_IO_EN);
  MmioWrite16(Base+0x1C, 0xD0D0);
  MmioWrite8(Base+PCI_CMD_REG, PCI_CMD_IO_EN);

  DEBUG((EFI_D_INFO, "[44]:%X, [DA]:%X\n", IoRead8(0xD000 + 0x44), IoRead16(0xD000 + 0xDA)));
  IoAnd8(0xD000 + 0x44, (UINT8)~BIT0);
  IoWrite16(0xD000 + 0xDA, 0);
  DEBUG((EFI_D_INFO, "[44]:%X, [DA]:%X\n", IoRead8(0xD000 + 0x44), IoRead16(0xD000 + 0xDA)));

//ProcExit:
  MmioWrite8(SubBase+PCI_CMD_REG, 0);
  MmioWrite32(SubBase+PCI_BAR0_REG, 0);
  MmioWrite8(Base+PCI_CMD_REG, 0);  
  MmioWrite16(Base+0x1C, 0);
  MmioWrite32(Base+PCI_PBN_REG, 0);

  Data32 = IoRead32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT );//PMIO Rx4e[7] 
  Data32 &= (~BIT23);
  IoWrite32(PmioBase + PMIO_GENERAL_PURPOSE_OUTPUT, Data32);//PMIO Rx4E[7] = 0
  return;
}



EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )  
{
  EFI_STATUS                       Status;
  ASIA_SB_CONFIGURATION            *SbCfg;
  EFI_ASIA_SB_PPI                  *SbPpi;
  EFI_ASIA_NB_PPI                  *NbPpi;
  EFI_ASIA_DRAM_PPI                *DramPpi;
  EFI_ASIA_CPU_PPI_PROTOCOL        *CpuPpi;
  EFI_BOOT_MODE                    BootMode;  
  PLATFORM_MEMORY_INFO             *MemInfo; 
  EFI_HOB_GUID_TYPE                *GuidHob;
  SETUP_DATA                       *SetupData;
  PLATFORM_S3_RECORD               *S3Record;

  DEBUG((EFI_D_INFO, "MemCallBack, &Status:%X\n", (UINTN)&Status));

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);  
  
  GetAsiaPpi(&SbPpi, &NbPpi, &DramPpi, &CpuPpi, NULL); 
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);

  GuidHob = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob!=NULL);	
  MemInfo = (PLATFORM_MEMORY_INFO*)GET_GUID_HOB_DATA(GuidHob); 

  SetupData = (SETUP_DATA*)GetSetupDataHobData();

  Status = CpuCachePpiInit();
  ASSERT_EFI_ERROR(Status); 


//JNY Porting for IOE MCU-E
  PERF_START(NULL, "CACHE", NULL, 0);		
  SetCacheMtrr(PeiServices, MemInfo);
  PERF_END  (NULL, "CACHE", NULL, 0);		
  
  if(BootMode == BOOT_ON_S3_RESUME){
	S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
	HandlePemcuFwPei(S3Record);
	
    Status = SbPpi->PostMemoryInitS3(PeiServices, SbPpi);
    ASSERT_EFI_ERROR(Status); 
    Status = NbPpi->PostMemoryInitS3(PeiServices, NbPpi);
    ASSERT_EFI_ERROR(Status);
    Status = SmmAccessPpiInstall(PeiServices);
    ASSERT_EFI_ERROR(Status);			
    Status = SmmControlPpiInstall(PeiServices);
    ASSERT_EFI_ERROR(Status);

	
	//JNY Porting for IOE MCU -S   
#ifdef IOE_EXIST    
	Status = HandleIoeMcuXhciFwPei(S3Record, SetupData);
	ASSERT_EFI_ERROR(Status);   
#endif
	
  } else {
    Status = SbPpi->PostMemoryInit(PeiServices, SbPpi);
    ASSERT_EFI_ERROR(Status); 
    Status = NbPpi->PostMemoryInit(PeiServices, NbPpi);
    ASSERT_EFI_ERROR(Status); 
    ReportResourceForDxe(PeiServices, MemInfo, BootMode);
    if(BootMode == BOOT_IN_RECOVERY_MODE){
      PeimInitializeRecovery((EFI_PEI_SERVICES**)PeiServices);
    } else if(BootMode == BOOT_ON_FLASH_UPDATE){	
      HandleCapsuleAfterMemInstall(PeiServices, MemInfo);
    }
  }

//UpdateSsid();
  #ifdef CHX002_A0
  #ifndef HX002EB0
  DisableObLan(NbPpi,SbPpi,SetupData->OnboardLan);
  #endif
  #endif
  
  return Status;
}


