
#include "PlatformDxe.h"
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiTable.h>


#define EFI_ACPI_DMA_REMAPPING_TABLE_SIGNATURE  SIGNATURE_32('D', 'M', 'A', 'R')
#define EFI_ACPI_DMA_REMAPPING_TABLE_REVISION   1

#define DMAR_FLAG_INTR_REMAP          BIT0
#define DMAR_FLAG_X2APIC_OPT_OUT      BIT1

#define DMAR_DRHD_FLAG_INC_PCI_ALL    BIT0

#define DMAR_DEV_SCOPE_TYPE_PCI_EP    				1
#define DMAR_DEV_SCOPE_TYPE_PCI_BRIDGE    			2
#define DMAR_DEV_SCOPE_TYPE_IOAPIC    				3
#define DMAR_DEV_SCOPE_TYPE_HPET    				4
#define DMAR_DEV_SCOPE_TYPE_ACPI_NAMESPACE_DEVICE   5

#define  REMAPPING_TYPE_DRHD		0
#define  REMAPPING_TYPE_RMRR		1
#define  REMAPPING_TYPE_ATSR		2
#define  REMAPPING_TYPE_RHSA		3
#define  REMAPPING_TYPE_ANDD		4

#define PLATFORM_SEGMENT_NUM 0x0
#define PLATFORM_START_BUS	 0x0

#define NBIOAPIC_DEV 0x00
#define NBIOAPIC_FUN 0x05

#define SBIOAPIC_DEV 0x11
#define SBIOAPIC_FUN 0x00

#define HPET_EnumID		0x0
#define HPET_START_BUS	0x80
#define HPET_DEV		0x10
#define HPET_FUN		0x0

#define PEMCU_EnumID	0x1
#define PEMCU_DEV		0x0
#define PEMCU_FUN		0x7

#define SPIC_EnumID		0x3
#define SPIC_DEV		0x11
#define SPIC_FUN		0x0

#pragma pack(1)

typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT16    Reserved;
  UINT8     EnumID;
  UINT8     StartBusNo;
  UINT8     Path[2];
} DMAR_DEVICE_SCOPE;

typedef struct {
  UINT16			  Type;
  UINT16			  Length;
  UINT8 			  Flags;
  UINT8 			  Reserved;
  UINT16			  SegmentNo;
  UINT64			  BaseAddr;
} DMAR_DRHD_TABLE_HEADER;

typedef struct {
	UINT16 	Type;
	UINT16  Length;
  	UINT8   Reserved[2];
  	UINT16  SegmentNo;
    UINT64  Reserved_mem_base_addr;
    UINT64  Reserved_mem_limit_addr;
} DMAR_RMRR_TABLE_HEADER;

typedef struct {
  UINT16              Type;
  UINT16              Length;
  UINT8               Reserved[3];
  UINT8               ACPIDeviceNumber;
  UINT8				  ACPIObjectName[24];
} DMAR_ANDD_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER 	Header;
  UINT8                         HostAddrWidth;        // +36
  UINT8                         Flags;                // +37
  UINT8                         Reserved[10];         // +38
} ACPI_TABLE_DMAR_HEADER;

#pragma pack()

EFI_STATUS InstallAcpiTableDmar()
{
  EFI_ACPI_TABLE_PROTOCOL  	*AcpiTable;
  EFI_STATUS               	Status;
  UINT64                   	OemTableId;
  UINTN                    	TableKey;

  //JRZ add for preparation of memory allocation
  EFI_PHYSICAL_ADDRESS		DmarMem;
  UINTN						SizePages;
  UINTN						SizeBytes; 

  UINT32	DmarTableSize = 0;

  ACPI_TABLE_DMAR_HEADER	*DmarTableHeaderPointer = NULL;
  DMAR_DRHD_TABLE_HEADER	*DrhdTableHeaderPointer = NULL;
  DMAR_RMRR_TABLE_HEADER	*RmrrTableHeaderPointer = NULL;
  DMAR_ANDD_TABLE			*AnddTablePointer = NULL;
  DMAR_DEVICE_SCOPE			*DeviceScopePointer = NULL;
  UINT8						*TablePointer = NULL;

  UINT8 PEMCU_RMRR_flag = 0;
  UINT8 XHCI_RMRR_flag = 0;
  UINT8 SPIC_flag = 1;

  if((gAsiaSbCfg->UsbModeSelect | SETUPVALUE_MASK) == USB_MODE_SEL_MODEB || 
  	 (gAsiaSbCfg->UsbModeSelect | SETUPVALUE_MASK) == USB_MODE_SEL_MODEC || 
  	 (gAsiaSbCfg->UsbModeSelect | SETUPVALUE_MASK) == USB_MODE_SEL_MODED )
  	XHCI_RMRR_flag = 1;
  else
  	XHCI_RMRR_flag = 0;

  if(gAsiaNbCfg->PEMCU_LoadFW_WhenBoot == 1)
  	PEMCU_RMRR_flag = 1;
  else
  	PEMCU_RMRR_flag = 0;

  if(!gAsiaNbCfg->IOVEnable){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }  

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, (VOID**)&AcpiTable);
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  
//[1]the total size of DMAR table will be calculated accurately.
  DmarTableSize += sizeof(ACPI_TABLE_DMAR_HEADER);

  DmarTableSize += sizeof(DMAR_DRHD_TABLE_HEADER);
  DmarTableSize += sizeof(DMAR_DEVICE_SCOPE) * 3;//for NBIOAPIC/SBIOAPIC/HPET device scope
  if(PEMCU_RMRR_flag)//if PEMCU will be used, a device scope must be reported in DRHD table in form of ACPI namespace device.
  	DmarTableSize += sizeof(DMAR_DEVICE_SCOPE);

  if(SPIC_flag)
  	DmarTableSize += sizeof(DMAR_DEVICE_SCOPE);

  //if XHCI will be enabled, a RMRR table for xhci mcu must be reported.
  if(XHCI_RMRR_flag)
  {
  	DmarTableSize += sizeof(DMAR_RMRR_TABLE_HEADER);
	DmarTableSize += sizeof(DMAR_DEVICE_SCOPE);
  }

  if(PEMCU_RMRR_flag)//if PEMCU will be used, a RMRR table and a andd table for PEMCU must be reported.
  {
    DmarTableSize += sizeof(DMAR_RMRR_TABLE_HEADER);
	DmarTableSize += sizeof(DMAR_DEVICE_SCOPE);

	DmarTableSize += sizeof(DMAR_ANDD_TABLE);
  }
  
  if(SPIC_flag)
  	DmarTableSize += sizeof(DMAR_ANDD_TABLE);

//[2]a memory buffer will be allocated according to the DMAR table size.
  SizeBytes = DmarTableSize;
  SizePages = EFI_SIZE_TO_PAGES(SizeBytes);
  DEBUG((EFI_D_ERROR,"[JRZ]DmarTableSize      = 0x%08X\n", DmarTableSize));
  DEBUG((EFI_D_ERROR,"[JRZ]SizePages          = 0x%08X pages.\n", SizePages));
  
  Status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, SizePages, &DmarMem);
  ASSERT_EFI_ERROR(Status);
  SetMem((VOID *)(UINTN)DmarMem, EFI_PAGES_TO_SIZE(SizePages), 0);
  DEBUG((EFI_D_ERROR,"[JRZ]pages allocation success! base addr = 0x%016X.\n", DmarMem));

  TablePointer = (UINT8 *)(UINTN)DmarMem;

//[3]the memory buffer is filled with the corresponding tables(DRHD/RMRR/ANDD) according to function status.

  //fill the memory buffer with DMAR table header
  DmarTableHeaderPointer = (ACPI_TABLE_DMAR_HEADER *)TablePointer;
  DmarTableHeaderPointer->Header.Signature 		= EFI_ACPI_DMA_REMAPPING_TABLE_SIGNATURE;
  DmarTableHeaderPointer->Header.Length 		= 0;//this field will be updated later
  DmarTableHeaderPointer->Header.Revision 		= EFI_ACPI_DMA_REMAPPING_TABLE_REVISION;
  DmarTableHeaderPointer->Header.Checksum 		= 0;//this field will be updated later
  CopyMem(DmarTableHeaderPointer->Header.OemId, PcdGetPtr(PcdAcpiDefaultOemId), sizeof(DmarTableHeaderPointer->Header.OemId));
  OemTableId = PcdGet64(PcdAcpiDefaultOemTableId);
  CopyMem(&(DmarTableHeaderPointer->Header.OemTableId), &OemTableId, sizeof(UINT64));
  DmarTableHeaderPointer->Header.OemRevision    = PcdGet32(PcdAcpiDefaultOemRevision);
  DmarTableHeaderPointer->Header.CreatorId      = PcdGet32(PcdAcpiDefaultCreatorId);
  DmarTableHeaderPointer->Header.CreatorRevision= PcdGet32(PcdAcpiDefaultCreatorRevision);
  DmarTableHeaderPointer->HostAddrWidth 		= 0x27;
  DmarTableHeaderPointer->Flags 				= DMAR_FLAG_INTR_REMAP;
  //DmarTableHeaderPointer->Reserved;//Reserved field has been zero already when memory was cleared successfully, just skip this step.
  TablePointer += sizeof(ACPI_TABLE_DMAR_HEADER);

  //fill the memory buffer with DRHD table
  DrhdTableHeaderPointer = (DMAR_DRHD_TABLE_HEADER *)TablePointer;
  DrhdTableHeaderPointer->Type 		= REMAPPING_TYPE_DRHD;
  DrhdTableHeaderPointer->Length 	= 0;//this field will be updated later
  DrhdTableHeaderPointer->Flags 	= DMAR_DRHD_FLAG_INC_PCI_ALL;
  //Skip initialization of Reserved field
  DrhdTableHeaderPointer->SegmentNo = PLATFORM_SEGMENT_NUM;
  DrhdTableHeaderPointer->BaseAddr 	= gAsiaNbCfg->RcrbvBar;
  TablePointer += sizeof(DMAR_DRHD_TABLE_HEADER);
  
	  DeviceScopePointer = (DMAR_DEVICE_SCOPE *)TablePointer;
	  DeviceScopePointer->Type 		= DMAR_DEV_SCOPE_TYPE_IOAPIC;
	  DeviceScopePointer->Length 	= sizeof(DMAR_DEVICE_SCOPE);
	  DeviceScopePointer->EnumID 	= gAsiaSbCfg->SbApicID;
	  DeviceScopePointer->StartBusNo= PLATFORM_START_BUS;
	  DeviceScopePointer->Path[0] 	= SBIOAPIC_DEV;
	  DeviceScopePointer->Path[1] 	= SBIOAPIC_FUN;
	  TablePointer += sizeof(DMAR_DEVICE_SCOPE);  

	  DeviceScopePointer = (DMAR_DEVICE_SCOPE *)TablePointer;
	  DeviceScopePointer->Type 		= DMAR_DEV_SCOPE_TYPE_IOAPIC;
	  DeviceScopePointer->Length 	= sizeof(DMAR_DEVICE_SCOPE);
	  DeviceScopePointer->EnumID 	= gAsiaNbCfg->NbApicID;
	  DeviceScopePointer->StartBusNo= PLATFORM_START_BUS;
	  DeviceScopePointer->Path[0] 	= NBIOAPIC_DEV;
	  DeviceScopePointer->Path[1] 	= NBIOAPIC_FUN;
	  TablePointer += sizeof(DMAR_DEVICE_SCOPE);

	  DeviceScopePointer = (DMAR_DEVICE_SCOPE *)TablePointer;
	  DeviceScopePointer->Type 		= DMAR_DEV_SCOPE_TYPE_HPET;
	  DeviceScopePointer->Length 	= sizeof(DMAR_DEVICE_SCOPE);
	  DeviceScopePointer->EnumID 	= HPET_EnumID;
	  DeviceScopePointer->StartBusNo= HPET_START_BUS;
	  DeviceScopePointer->Path[0] 	= HPET_DEV;
	  DeviceScopePointer->Path[1] 	= HPET_FUN;
	  TablePointer += sizeof(DMAR_DEVICE_SCOPE);  

	  if(PEMCU_RMRR_flag)
	  {
		  DeviceScopePointer = (DMAR_DEVICE_SCOPE*)TablePointer;
		  DeviceScopePointer->Type 		= DMAR_DEV_SCOPE_TYPE_ACPI_NAMESPACE_DEVICE;
		  DeviceScopePointer->Length 	= sizeof(DMAR_DEVICE_SCOPE);
		  DeviceScopePointer->EnumID 	= PEMCU_EnumID;
		  DeviceScopePointer->StartBusNo= PLATFORM_START_BUS;
		  DeviceScopePointer->Path[0] 	= PEMCU_DEV;
		  DeviceScopePointer->Path[1] 	= PEMCU_FUN;
		  TablePointer += sizeof(DMAR_DEVICE_SCOPE);
	  }
	  
	  if(SPIC_flag)
	  {
		  DeviceScopePointer = (DMAR_DEVICE_SCOPE*)TablePointer;
		  DeviceScopePointer->Type		= DMAR_DEV_SCOPE_TYPE_ACPI_NAMESPACE_DEVICE;
		  DeviceScopePointer->Length	= sizeof(DMAR_DEVICE_SCOPE);
		  DeviceScopePointer->EnumID	= SPIC_EnumID;
		  DeviceScopePointer->StartBusNo= PLATFORM_START_BUS;
		  DeviceScopePointer->Path[0]	= SPIC_DEV;
		  DeviceScopePointer->Path[1]	= SPIC_FUN;
		  TablePointer += sizeof(DMAR_DEVICE_SCOPE);
	  }

  DrhdTableHeaderPointer->Length = (UINT16)(TablePointer - (UINT8 *)DrhdTableHeaderPointer);

  //fill the memory buffer with RMRR table for mcu in xhci if it is necessary.
  if(XHCI_RMRR_flag)
  {
    RmrrTableHeaderPointer = (DMAR_RMRR_TABLE_HEADER *)TablePointer;
	RmrrTableHeaderPointer->Type 					= REMAPPING_TYPE_RMRR;
	RmrrTableHeaderPointer->Length 					= 0;//this field will be updated later
	//Skip initialization of Reserved field
	RmrrTableHeaderPointer->SegmentNo 				= PLATFORM_SEGMENT_NUM;
	RmrrTableHeaderPointer->Reserved_mem_base_addr 	= PcdGet64(PcdxhciFWAddr);
	RmrrTableHeaderPointer->Reserved_mem_limit_addr = RmrrTableHeaderPointer->Reserved_mem_base_addr + PcdGet32(PcdxhciFWSize);
	if(RmrrTableHeaderPointer->Reserved_mem_limit_addr % 0x1000 == 0)//This range must be 4K aligned according to VT-d Spec, and limit must be the last address in a 4K range.
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr--;
	else
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr = ((RmrrTableHeaderPointer->Reserved_mem_limit_addr + 0x1000) & 0xFFFFFFFFFFFFF000) - 1;
	TablePointer += sizeof(DMAR_RMRR_TABLE_HEADER);
	
	DEBUG((EFI_D_ERROR,"[JRZ]xchi RmrrTableBase = 0x%016X\n", RmrrTableHeaderPointer->Reserved_mem_base_addr));
	DEBUG((EFI_D_ERROR,"[JRZ]xchi RmrrTableLimit = 0x%016X\n", RmrrTableHeaderPointer->Reserved_mem_limit_addr));

	DeviceScopePointer = (DMAR_DEVICE_SCOPE *)TablePointer;
	DeviceScopePointer->Type 			= DMAR_DEV_SCOPE_TYPE_PCI_EP;
	DeviceScopePointer->Length 			= sizeof(DMAR_DEVICE_SCOPE);
	//Skip initialization of Reserved field
	DeviceScopePointer->EnumID 			= 0;
	DeviceScopePointer->StartBusNo 		= PLATFORM_START_BUS;
	DeviceScopePointer->Path[0] 		= 18;
	DeviceScopePointer->Path[1] 		= 0;
	TablePointer += sizeof(DMAR_DEVICE_SCOPE);

	RmrrTableHeaderPointer->Length = (UINT16)(TablePointer - (UINT8 *)RmrrTableHeaderPointer);
  }

  //fill the memory buffer with RMRR table for PEMCU if it is necessary.
  if(PEMCU_RMRR_flag)
  {
    RmrrTableHeaderPointer = (DMAR_RMRR_TABLE_HEADER *)TablePointer;
	RmrrTableHeaderPointer->Type 		= REMAPPING_TYPE_RMRR;
	RmrrTableHeaderPointer->Length 		= 0;//this field will be updated later
	//Skip initialization of Reserved field
	RmrrTableHeaderPointer->SegmentNo 	= PLATFORM_SEGMENT_NUM;

	DEBUG((EFI_D_ERROR,"[JRZ]PcdPEMCUFWAddr = 0x%016X\n", PcdGet64(PcdPEMCUFWAddr)));
	DEBUG((EFI_D_ERROR,"[JRZ]PcdPEMCUFWSize = 0x%08X\n", PcdGet32(PcdPEMCUFWSize)));
	DEBUG((EFI_D_ERROR,"[JRZ]PcdPEMCUDATAAddr = 0x%016X\n", PcdGet64(PcdPEMCUDATAAddr)));
	DEBUG((EFI_D_ERROR,"[JRZ]PcdPEMCUDATASize = 0x%08X\n", PcdGet32(PcdPEMCUDATASize)));

	if(PcdGet64(PcdPEMCUFWAddr) < PcdGet64(PcdPEMCUDATAAddr))//this RMRR table will cover the data and instruction range of PEMCU in DRAM.
	{
	  RmrrTableHeaderPointer->Reserved_mem_base_addr = PcdGet64(PcdPEMCUFWAddr);
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr = PcdGet64(PcdPEMCUDATAAddr) + PcdGet32(PcdPEMCUDATASize);
	}
	else
	{
	  RmrrTableHeaderPointer->Reserved_mem_base_addr = PcdGet64(PcdPEMCUDATAAddr);
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr = PcdGet64(PcdPEMCUFWAddr) + PcdGet32(PcdPEMCUFWSize);
	}
	
	if((RmrrTableHeaderPointer->Reserved_mem_limit_addr % 0x1000) == 0)//This range must be 4K aligned according to VT-d Spec, and limit must be the last address in a 4K range.
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr--;
	else
	  RmrrTableHeaderPointer->Reserved_mem_limit_addr = ((RmrrTableHeaderPointer->Reserved_mem_limit_addr + 0x1000) & 0xFFFFFFFFFFFFF000) - 1;
	TablePointer += sizeof(DMAR_RMRR_TABLE_HEADER);

	DEBUG((EFI_D_ERROR,"[JRZ]PEMCU RmrrTableBase = 0x%016X\n", RmrrTableHeaderPointer->Reserved_mem_base_addr));
	DEBUG((EFI_D_ERROR,"[JRZ]PEMCU RmrrTableLimit = 0x%016X\n", RmrrTableHeaderPointer->Reserved_mem_limit_addr));	

	DeviceScopePointer = (DMAR_DEVICE_SCOPE *)TablePointer;
	DeviceScopePointer->Type 		= DMAR_DEV_SCOPE_TYPE_ACPI_NAMESPACE_DEVICE;
	DeviceScopePointer->Length 		= sizeof(DMAR_DEVICE_SCOPE);
	//Skip initialization of Reserved field
	DeviceScopePointer->EnumID 		= PEMCU_EnumID;
	DeviceScopePointer->StartBusNo 	= PLATFORM_START_BUS;
	DeviceScopePointer->Path[0] 	= PEMCU_DEV;
	DeviceScopePointer->Path[1] 	= PEMCU_FUN;
	TablePointer += sizeof(DMAR_DEVICE_SCOPE);

	RmrrTableHeaderPointer->Length = (UINT16)(TablePointer - (UINT8 *)RmrrTableHeaderPointer);
  }  

  //fill the memory buffer with ANDD table for PEMCU if it is necessary.
  if(PEMCU_RMRR_flag)
  {
	AnddTablePointer = (DMAR_ANDD_TABLE *)TablePointer;
    AnddTablePointer->Type 				= REMAPPING_TYPE_ANDD;
	AnddTablePointer->Length 			= sizeof(DMAR_ANDD_TABLE);
	//Skip initialization of Reserved field
	AnddTablePointer->ACPIDeviceNumber 	= PEMCU_EnumID;
	CopyMem(AnddTablePointer->ACPIObjectName, "\\_SB.MPEC", strlen("\\_SB.MPEC"));
	TablePointer += sizeof(DMAR_ANDD_TABLE);
  }
  
  //fill the memory buffer with ANDD table for SPIC if it is necessary.
  if(SPIC_flag)
  {
	AnddTablePointer = (DMAR_ANDD_TABLE *)TablePointer;
    AnddTablePointer->Type 				= REMAPPING_TYPE_ANDD;
	AnddTablePointer->Length 			= sizeof(DMAR_ANDD_TABLE);
	//Skip initialization of Reserved field
	AnddTablePointer->ACPIDeviceNumber 	= SPIC_EnumID;
	CopyMem(AnddTablePointer->ACPIObjectName, "\\_SB.PCI0.SBRG.SPIC", strlen("\\_SB.PCI0.SBRG.SPIC"));
	TablePointer += sizeof(DMAR_ANDD_TABLE);
  }

  DmarTableHeaderPointer->Header.Length = (UINT16)(TablePointer - (UINT8 *)DmarTableHeaderPointer);
  DEBUG((EFI_D_ERROR,"[JRZ]DmarTableHeaderPointer->Header.Length = 0x%04X\n", DmarTableHeaderPointer->Header.Length));

  AcpiTableUpdateChksum(DmarTableHeaderPointer);
 
  //debug
  //for(UINT32 index = 0; index < DmarTableHeaderPointer->Header.Length; index++)
  //	DEBUG((EFI_D_ERROR,"%02X ", TablePointer[index]));
  //DEBUG((EFI_D_ERROR,"\n"));
 
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        DmarTableHeaderPointer,
                        DmarTableHeaderPointer->Header.Length,
                        &TableKey
                        ); 
  
  Status = gBS->FreePages(DmarMem, SizePages);
  DEBUG((EFI_D_ERROR,"[JRZ]pages free success!\n"));
  
ProcExit:
  return Status;  
}




