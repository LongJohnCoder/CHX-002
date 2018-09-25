

#include <Uefi.h>
#include <Guid/SetupPassword.h>
#include <Guid/MtcVendor.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/VariableWrite.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciRootBridgeIo.h>                   // AsiaNbProtocol.h +
#include <Protocol/PciHostBridgeResourceAllocation.h>   // AsiaNbProtocol.h +
#include <Protocol/PciIo.h>                             // AsiaNbProtocol.h +
#include <PlatformDefinition.h>
#include <AsiaNbProtocol.h>
#include <CHX002Cfg.h>
#include <AsiaCpuProtocol.h>
#include <SetupVariable.h>
#include <Library/BaseCryptLib.h>
#include <Library/PlatformCommLib.h>
#include <PlatS3Record.h>
#include <Chx002/GENERAL.h>

extern EFI_GUID gIgdGopDepexProtocolGuid;
extern EFI_GUID gObLanUndiDxeDepexProtocolGuid;
CONST SETUP_DATA *gSetupHob;
TSESETUP gTse;
PLATFORM_S3_RECORD  *gS3Record;
CONST CHAR16 *gDefaultSetupAdminPassword = (CHAR16*)PcdGetPtr(PcdDefaultSetupAdminPassword);




/*
EFI_STATUS 
AllocAlignPageBelow4G (
  UINTN  Size, 
  UINTN  Alignment,
  VOID   **Memory
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;  
  UINTN                 TotalPages;
  EFI_STATUS            Status;
  UINTN                 AlignedAddr;
  UINTN                 Delta;
  UINTN                 PageEnd;


  if(Alignment & 0xFFF){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }  

  Address    = 0xFFFFFFFF;
  TotalPages = EFI_SIZE_TO_PAGES(Size + Alignment);
  Status     = gBS->AllocatePages(
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      TotalPages,
                      &Address
                      );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  DEBUG((EFI_D_INFO, "Pages(%X,%X) L:%X\n", (UINT32)Address, TotalPages, Size));


  AlignedAddr = (UINTN)ALIGN_VALUE(Address, Alignment);

  Delta = AlignedAddr - (UINTN)Address;
  if(Delta){
    Pages = EFI_SIZE_TO_PAGES(Delta);
    DEBUG((EFI_D_INFO, "FreePage(%X,%X)\n", Address, Pages));    
    gBS->FreePages(Address, Pages);
  }  
  
  Pages   = EFI_SIZE_TO_PAGES(Size);
  PageEnd = AlignedAddr + EFI_PAGES_TO_SIZE(Pages);
  Delta   = Address + EFI_PAGES_TO_SIZE(TotalPages) - PageEnd;
  if(Delta){
    Pages = EFI_SIZE_TO_PAGES(Delta);
    DEBUG((EFI_D_INFO, "FreePage(%X,%X)\n", PageEnd, Pages));    
    gBS->FreePages(PageEnd, Pages);
  }  

  *Memory = (VOID*)AlignedAddr;

ProcExit:
  return Status;
}
*/



STATIC
EFI_STATUS
ReadFileFromFv(
  IN  CONST EFI_GUID    *NameGuid,
  IN  EFI_SECTION_TYPE  SectionType,
  IN  UINTN             SectionInstance,
  OUT VOID              **FileData,
  OUT UINTN             *FileDataSize
    );

STATIC EFI_STATUS HandleXhciFw (
    CONST SETUP_DATA *SetupData
    )
{
    EFI_STATUS              Status = EFI_SUCCESS;
    VOID                    *Buffer;
    UINTN                   Size;
    EFI_PHYSICAL_ADDRESS    Address;
    UINTN                   Pages;
    UINT16                  tVID;
    UINT16                  tDID;
    BOOLEAN                 XhciEn   = (SetupData->UsbModeSelect != USB_MODE_SEL_DISABLE && SetupData->UsbModeSelect != USB_MODE_SEL_MODEA);
    EFI_PHYSICAL_ADDRESS    McuFw;
    UINT32                  McuFw_Lo, McuFw_Hi;


    // Init XhciMcuFw releated variable in gS3Record for S3 resume.
    gS3Record->XhciMcuFw_Lo  = 0;
    gS3Record->XhciMcuFw_Hi  = 0;
    gS3Record->XhciMcuFwSize = 0;

    // if one of the master or slave xHCI is enabled, we must load fw for it.
    if( XhciEn ) {
        DEBUG((EFI_D_INFO, "[CHX002_XHCI_FW]: xHCI has been enabled, catching firmware binary from flash to host memory...\n"));
        Status = ReadFileFromFv (
                 (EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile),
                 EFI_SECTION_RAW,
                 0,
                 &Buffer,
                 &Size);
        if(EFI_ERROR(Status)){
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Firmware binary catching failed!(%r)\n", Status));
            return Status;
        }

        DEBUG((EFI_D_INFO, "[CHX002_XHCI_FW]: Firmware loading for normal boot...\n"));

        tVID     = MmioRead16(PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG);
        tDID     = MmioRead16(PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_DID_REG);

        DEBUG((EFI_D_INFO, "                  [%02X|%02X|%02X](%08X) VID = 0x%04X, DID = 0x%04X\n", 0, CHX002_XHCI_DEV, 0, PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG, tVID, tDID));

        if( !( ((tVID == PCI_VID_ZX) || (tVID == PCI_VID_VIA)) && (tDID == XHCI_DEVICE_ID) ) ) {
            Status = EFI_DEVICE_ERROR;
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: [%02X|%02X|%02X](%08X) not exist!(%r)\n", 0, CHX002_XHCI_DEV, 0, PCI_DEV_MMBASE(0, CHX002_XHCI_DEV, 0) + PCI_VID_REG, Status));
            return Status;
        }

        DEBUG((EFI_D_INFO, "                  Allocate 64K-aligned buffer for xHCI...\n"));
        Address = 0xFFFFFFFFFFFFFFFF;
        Pages   = Size + SIZE_64KB;             // fw data size may be enlarged if using fw for some test cases
        Pages   = Pages + SIZE_64KB;            // need 64K align
        Pages   = EFI_SIZE_TO_PAGES(Pages);
        Status  = gBS->AllocatePages(
                      AllocateMaxAddress,
                      EfiReservedMemoryType,
                      Pages,
                      &Address
                  );
        if(EFI_ERROR(Status)){
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Allocate 64K-aligned buffer failed!(%r)\n", Status));
            return Status;
        }

        //JRZ add for XHCI RMRR table
        PcdSet64(PcdXhciFWAddr, (UINT64)Address);
        PcdSet32(PcdXhciFWSize, (UINT32)EFI_PAGES_TO_SIZE(Pages));

        McuFw = (EFI_PHYSICAL_ADDRESS)ALIGN_VALUE(Address, SIZE_64KB);

        McuFw_Lo = (UINT32)(McuFw & 0xFFFFFFFF);
        McuFw_Hi = (UINT32)(McuFw >> 32);

        if (McuFw_Hi != 0) {
            Status  = EFI_INVALID_PARAMETER;
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: Allocated 64K-aligned buffer beyond 4G!(%r)\n", Status));
            return Status;
        }

        gS3Record->XhciMcuFw_Lo = McuFw_Lo;
        gS3Record->XhciMcuFw_Hi = McuFw_Hi;

        DEBUG((EFI_D_INFO, "                  Firmware Address = 0x%016X  Firmware Size = %d[0x%X]Byte\n", McuFw, Size, Size));
        CopyMem((VOID *)McuFw, Buffer, Size);                   //Copy FW from Buffer to the space we allocated in DRAM

        gS3Record->XhciMcuFwSize  = (UINT32)Size;

        Status = LoadXhciFw(0, CHX002_XHCI_DEV, 0, McuFw_Lo, McuFw_Hi);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: xHCI firmware loading failed!(%r)\n", Status));
            return Status;
        }

        gBS->FreePool(Buffer);                                      //free the buffer memory
    } else {
        Status = EFI_SUCCESS;
        DEBUG((EFI_D_ERROR, "[CHX002_XHCI_FW]: xHCI has been disabled, skip firmware loading.\n"));
    }

    return Status;
}

// CJW_IOE_PORTING [20160922] - START
//CJW_IOE
#ifdef IOE_EXIST

STATIC EFI_STATUS  
HandleIoeMcuFwImp(
	UINT8 Bus,
	UINT8 Dev,
	UINT8 Func
)
{
  	EFI_STATUS            Status;
  	VOID                  *Buffer;
  	UINTN                 Size;
 	EFI_PHYSICAL_ADDRESS  Address;
  	UINTN                 Pages;
  	VOID                  *IoeMcuFw;
   	VOID                  *IoeMcuData;
  	UINT16 AutofillLen = SIZE_32KB;
	UINT16 AutofillAddr = 0;

	PLATFORM_S3_RECORD    *S3Record;	
	
	//Read file from Fv 
	//this function will allocate the buffer space for fw
	//we need to free it when we don't need it 
  	Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdIoeMcuFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  	ASSERT_EFI_ERROR(Status);

  	if(EFI_ERROR(Status)){
		goto ProcExit_Ioe;
	}

	//[1]:
  	//allocate mcu instruction space
  	//and we need to ensure the start address must be 64k align
  	//this align limit is from HW designer 
  	Address = 0xFFFFFFFF;
  	Pages   = Size + SIZE_64KB;      		// need 64K align
  	Pages   = EFI_SIZE_TO_PAGES(Pages);
  	Status  = gBS->AllocatePages(
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  	ASSERT_EFI_ERROR(Status);
#if 1      									
  	IoeMcuFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);		
#else
  	IoeMcuFw = (void*)((UINT64)0x20000000);			//Appoint address	
#endif
  	DEBUG((EFI_D_ERROR, "              IoeMcuFw = %x  FwSize = %d[0x%x]Byte\n",IoeMcuFw,Size,Size));
  	CopyMem(IoeMcuFw, Buffer, Size);				//Copy FW from Buffer to the space we allocated in DRAM
  	gBS->FreePool(Buffer);							//free the buffer memory 

	//Now FW have located in DRAM
	//[2]:
  	//Allocate Ioe mcu data space
  	Address = 0xFFFFFFFF;
  	Pages   = Size + SIZE_64KB;                     // need 64K align
  	Pages   = EFI_SIZE_TO_PAGES(Pages);
  	Status  = gBS->AllocatePages(			
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
 	ASSERT_EFI_ERROR(Status);
  	IoeMcuData = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);
  	gBS->SetMem(IoeMcuData,SIZE_64KB,0);

#if 1
	Status = LoadIoeMcuFw(Bus, IoeMcuFw,AutofillAddr,AutofillLen);
	ASSERT_EFI_ERROR(Status);	
#endif


	S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
	S3Record->PcieIoeEptrfcBusNum = (UINT8)Bus;
	S3Record->PcieIoeMcu = (UINT64)IoeMcuFw;
	S3Record->PcieIoeMcuAddr = (UINT16)AutofillAddr;
	S3Record->PcieIoeMcuLen = (UINT16)AutofillLen;

ProcExit_Ioe:
	return Status;
}

//CJW_IOE
STATIC EFI_STATUS HandleIoeMcuFw(VOID)
{

	#define mBusX 	1
	#define mBusXp1 2
	#define mBusXp2 3
	#define mBusXp3 4

	//
	//Those code for verfy on Haps - IOE under D7F0
	//
	#define VIDDID_IOE 0x071F1106
	#define DIDVID_EPTRFC 0x91011106
	#define VIDDID_IOE_ZX	0x071F1D17
	#define DIDVID_EPTRFC_ZX 0x91011D17
	
	EFI_STATUS Status=EFI_SUCCESS;			//CJW_IOE_0425

	UINT8 Tbus,Tdev,Tfunc;
	UINT32 Tex;
	UINT8 Tmpx,Tmpx1;
	UINT8 HideFlag = 0;

	//
	// Read Scratch register to determine whether to load fw
	//
	if (! (BIT0 & MmioRead8(PCI_DEV_MMBASE(0, 0, 6) + 0x47))) {
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] SKIP CND003 Init [HandleIoeMcuFw()]\n"));
		return EFI_SUCCESS;
	}


	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] IoeMcu Fw load...\n"));

	//
	// Scan whole system to search IOE
	// Here we only think the IOE connected to RP(Bus = 0)
	//
	Tbus = 0;
	for(Tdev = 0; Tdev < 0x32; Tdev++){
		for(Tfunc = 0; Tfunc<8; Tfunc++){
			Tex = MmioRead32(PCI_DEV_MMBASE(Tbus, Tdev, 0) + 0x00);
			if(Tex == 0xFFFFFFFF){
				break;
			}
			//assign temp bus number
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x01);
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x01);

			Tex = MmioRead32(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x00);	

			//clear temp bus number
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
			MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);	
			
			if((Tex == VIDDID_IOE)||(Tex ==VIDDID_IOE_ZX)){
				DEBUG((EFI_D_ERROR, "              Found Ioe [%d|%d|%d]\n",Tbus,Tdev,Tfunc));
				goto _FoundIoe;
			}
				
		}
	}
	DEBUG((EFI_D_ERROR, "              Can't find Ioe\n"));
	Status = EFI_SUCCESS;
	return Status;
	
_FoundIoe:
	
	//
	// Assign temp bus number
	//
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x01);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x04);

	//
	// distinguish BIOS/SPI mode
	//
	DEBUG((EFI_D_ERROR, "              Rx1EA=0x%04x\n", MmioRead16(PCI_DEV_MMBASE(1, 0, 0) + 0x1EA)));
	if( BIT15 & MmioRead16(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x1EA) ){
		DEBUG((EFI_D_ERROR, "              In BIOS mode\n"));
	}else{
		DEBUG((EFI_D_ERROR, "              In SPI mode - Exit\n"));
		MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
		MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);		
		return Status;
	}

#if 0	
	//Since EPTRFC was hidden before, now let it visiable temporarily 
	MmioWrite32(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x10, 0xFE028000);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04, 0x07); 	//Mem enable
	MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x20, 0xFE02); //Base mem
	MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x22, 0xFE02); //limit mem
	MmioWrite8(0xFE028000+0x1100+0x52, (MmioRead8(0xFE028000+0x1100+0x52)&(~(BIT7)))); 
#endif

	//For BusX
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x18, 0x01);
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x19, 0x02);
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x1A, 0x04);
	//For BusX+1
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x18, 0x02);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x19, 0x03);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x1A, 0x04);
	//For PCIEIF
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x18, 0x03);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x19, 0x04);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x1A, 0x04);	

	//
	// Read VID DID
	//
	Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp3, 0, 0) + 0x00);
	DEBUG((EFI_D_ERROR, "              EPTRFC's DIDVID = 0x%08X (Should be 0x91011106)\n",Tex));
	Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x00);
	DEBUG((EFI_D_ERROR, "              PCIEIF's DIDVID = 0x%08X (Should be 0x07221106)\n",Tex));
	Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x00);
	DEBUG((EFI_D_ERROR, "              PESB's DIDVID = 0x%08X (Should be 0x07211106)\n",Tex));
	Tex = MmioRead32(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x00);
	DEBUG((EFI_D_ERROR, "              PEEP's DIDVID = 0x%08X (Should be 0x071F1106)\n",Tex));
	
	//
	// Enable BusMaster Enable(Rx04[2])  of PCIEIF
	//
	Tmpx = MmioRead8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x04);
	DEBUG((EFI_D_ERROR, "              The value of PCIEIF Rx04 is 0x%02x\n",Tmpx));
	Tmpx1 = ((Tmpx & (~BIT2))|BIT2);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x04, Tmpx1);
	DEBUG((EFI_D_ERROR, "              PCIEIF Rx04 will be set as 0x%02x\n",Tmpx1));
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x04, 0x07);
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x04, 0x07);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04, 0x07);
	DEBUG((EFI_D_ERROR, "              [0|7|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(0, 7, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(0, 7, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "              [1|0|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(1, 0, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(1, 0, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "              [2|8|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(2, 8, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(2, 8, 0) + 0x04)));
	DEBUG((EFI_D_ERROR, "              [3|0|0][%04x] Rx04=0x%02x\n",MmioRead16(PCI_DEV_MMBASE(3, 0, 0) + 0x02),MmioRead8(PCI_DEV_MMBASE(3, 0, 0) + 0x04)));

	//
	// If EPTRFC was hiden before, show it temporary
	//
	MmioWrite32(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x20, 0xFE02FE02);
	MmioWrite32(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x10, 0xFE028000);
	if( MmioRead8(0xFE028000+0x1100+0x52) & BIT7 ){
		HideFlag = 1;
		MmioWrite8(0xFE028000+0x1100+0x52,  MmioRead8(0xFE028000+0x1100+0x52)&(~(BIT7)));
	}else{
		HideFlag = 0;
	}
	
	Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp3, 0, 0) + 0x00);
	DEBUG((EFI_D_ERROR, "              EPTRFC's DIDVID = 0x%08X (Should be 0x91011106)\n",Tex));
		

#if 1	
	////
	//This code path is the formal path
	//and the next part is just for debug
	////

	Status=HandleIoeMcuFwImp(mBusXp3,0,0);

#else
//CJW_IOE:
//Those code is for Burn In test
//BIOS load fw and fw will write EPHY RxC70-C73 to 0x12345678
//Then BIOS will wait several seconds to guarantee the finish of fw's action
//Then BIOS will check whether the value was set in or not
//FW requirement:
//	-read B0D0F0 to ensure the BAR have a valid value(Non-Zero)
//	-write EPHY RxC70-C73 to 0x12345678
// 	Note: althrough we have wirte PEEP's Rx18 to 0x1, but 
//			we still need to use BUS0 to access PEEP from debug path
{
	UINTN xp,xq,xx,xs=0;

	//// CJW_AUTOFILL_TEST
	//Those test code is for IOE FW autofill 
	////
	
	//Assign the bar/base/limit
	MmioWrite16(PCI_DEV_MMBASE(0, 7, 0) + 0x20, 0xFC10);
	MmioWrite16(PCI_DEV_MMBASE(0, 7, 0) + 0x22, 0xFC10);
	MmioWrite32(PCI_DEV_MMBASE(1, 0, 0) + 0x10, 0xFC100000);
	//Clear the scratch regsiter value
	//to confirm the default value can be zero
	MmioWrite32(0xFC100C70, 0);
	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] We clear the EPHY 0xC70 to (0x%08x)\n",MmioRead32(0xFC100C70)));
	
	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] [xxx] MMIO bar is [%08x]\n",MmioRead32(PCI_DEV_MMBASE(1, 0, 0) + 0x10)));
	IoWrite8(0x80, 0x78);
	
	//4 FW load 
	Status=HandleIoeMcuFwImp(4,0,0);

	//Delay
	IoWrite8(0x80, 0x79);
	for(xp = 0; xp < 0x1000;xp++){
		for(xq = 0;xq<0x1000;xq++){
			for(xx = 0; xx<90;xx++){
				xs++;	
				if(xs >= 0xFFFF){
					xs = 0;
				}
		
			}
		}
	}

	//Check the value of EPHY registers 
	IoWrite8(0x80, 0x80);
	DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] EPHY C70 - 0x%02x\n",MmioRead8(0xFC100C70)));

	if(0x12 != MmioRead8(0xFC100C70)){
		IoWrite8(0x80, 0x81);
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] EPHY C70 - 0x%02x\n",MmioRead8(0xFC100C70)));
		while(1);
	}
	if(0x34 != MmioRead8(0xFC100C71)){
		IoWrite8(0x80, 0x82);
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] EPHY C71 - 0x%02x\n",MmioRead8(0xFC100C71)));
		while(1);
	}
	if(0x56 != MmioRead8(0xFC100C72)){
		IoWrite8(0x80, 0x83);
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] EPHY C72 - 0x%02x\n",MmioRead8(0xFC100C72)));
		while(1);
	}
	if(0x78 != MmioRead8(0xFC100C73)){
		IoWrite8(0x80, 0x84);
		DEBUG((EFI_D_ERROR, "[CJW_IOE_MCU] EPHY C73 - 0x%02x\n",MmioRead8(0xFC100C73)));
		while(1);
	}

	
	//clear the temp bar/base/limit
	MmioWrite16(PCI_DEV_MMBASE(0, 7, 0) + 0x20, 0x0000);
	MmioWrite16(PCI_DEV_MMBASE(0, 7, 0) + 0x22, 0x0000);
	MmioWrite32(PCI_DEV_MMBASE(1, 0, 0) + 0x10, 0x00000000);	
	IoWrite8(0x80, 0x85);

}
#endif

	//
	// Clear bus number assignment
	//
	//For PCIEIF
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x18, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x19, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x1A, 0x00);	
	//For BusX+1
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x18, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x19, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x1A, 0x00);
	//For BusX
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x18, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x19, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x1A, 0x00);


	// Hide EPTRFC after Autofill
	DEBUG((EFI_D_ERROR, "              HideFlag = %d\n",HideFlag));
	if(HideFlag){
		MmioWrite8(0xFE028000+0x1100+0x52, (MmioRead8(0xFE028000+0x1100+0x52)|(BIT7))); 
	}
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x10, 0x00000000);
	

	//MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04, 0x07); //Mem enable
	MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x20, 0x0000); //Base mem
	MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x22, 0x0000); //limit mem
	//switch upstream port
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
	MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);

			
	


	return Status;
}

STATIC EFI_STATUS  
HandleIoeXhciFwImp(
  UINT8 Bus,
  UINT8 Dev,
  UINT8 Func
)
{
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;
  VOID                  *IoeXhciFw;

  PLATFORM_S3_RECORD    *S3Record;

  //Read file from Fv 
  //this function will allocate the buffer space for fw
  //we need to free it when we don't need it 
  Status = ReadFileFromFv (
    (EFI_GUID*)PcdGetPtr(PcdIoeXhciFwFile),
    EFI_SECTION_RAW,
    0,
    &Buffer,
    &Size
  );
  ASSERT_EFI_ERROR(Status);

  if(EFI_ERROR(Status)){
    goto ProcExit_Ioe;
  }

  //[1]:
  //allocate mcu instruction space
  //and we need to ensure the start address must be 64k align
  //this align limit is from HW designer 
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;           // fw data size may be enlarged if using fw for some test cases
  Pages   = Pages + SIZE_64KB;          // need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(
                AllocateMaxAddress,
                EfiReservedMemoryType,
                Pages,
                &Address
                );
  ASSERT_EFI_ERROR(Status);

  PcdSet64(PcdIoeXhciFWAddr, (UINT64)Address);
  PcdSet32(PcdIoeXhciFWSize, (UINT32)EFI_PAGES_TO_SIZE(Pages));

  IoeXhciFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);

  DEBUG((EFI_D_INFO, "                  IoeXhciFw = %x  FwSize = %d[0x%x]Byte\n",IoeXhciFw, Size, Size));

  CopyMem(IoeXhciFw, Buffer, Size);     //Copy FW from Buffer to the space we allocated in DRAM
  gBS->FreePool(Buffer);                //free the buffer memory 

  Status = LoadIoeXhciFw(Bus, IoeXhciFw);

  ASSERT_EFI_ERROR(Status);

  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);

  S3Record->PcieIoeEptrfcBusNum = (UINT8)Bus;
  S3Record->PcieIoeXhci         = (UINT64)IoeXhciFw;

ProcExit_Ioe:
  return Status;
}

STATIC EFI_STATUS
HandleIoeXhciFw(
  VOID
)
{
#define mBusX   1
#define mBusXp1 2
#define mBusXp2 3
#define mBusXp3 4

  //
  //Those code for verfy on Haps - IOE under D7F0
  //
  //#define VIDDID_IOE 0x01221C28
#define VIDDID_IOE        0x071F1106
#define DIDVID_EPTRFC     0x91011106
#define VIDDID_IOE_ZX     0x071F1D17
#define DIDVID_EPTRFC_ZX  0x91011D17

  EFI_STATUS Status=EFI_SUCCESS;

  UINT8   Tbus, Tdev, Tfunc;
  UINT32  Tex;
  UINT8   Tmpx, Tmpx1;
  UINT8   HideFlag = 0;

  //
  // Read Scratch register to determine whether to load fw
  //
  if (! (BIT0 & MmioRead8(PCI_DEV_MMBASE(0, 0, 6) + 0x47))) {
    DEBUG((EFI_D_ERROR, "[CND003_XHCI_FW]: SKIP CND003 Init [HandleIoeXhciFw()]\n"));
    return EFI_SUCCESS;
  }

  DEBUG((EFI_D_INFO, "[CND003_XHCI_FW]: IoeXhci Fw load...\n"));

  //
  // Scan whole system to search IOE
  // Here we only think the IOE connected to D7F0 
  //
  Tbus = 0;
  for(Tdev = 0; Tdev < 0x32; Tdev++) {
    for(Tfunc = 0; Tfunc < 8; Tfunc++) {
      Tex = MmioRead32(PCI_DEV_MMBASE(Tbus, Tdev, 0) + 0x00);
      if(Tex == 0xFFFFFFFF) {
        break;
      }
      //assign temp bus number
      MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
      MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x01);
      MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x01);

      Tex = MmioRead32(PCI_DEV_MMBASE(1, 0, 0) + 0x00);

      //clear temp bus number
      MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
      MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);

      if((Tex == VIDDID_IOE)||(Tex == VIDDID_IOE_ZX)) {
        DEBUG((EFI_D_INFO, "                  Found Ioe [%d|%d|%d]\n",Tbus,Tdev,Tfunc));
        goto _FoundIoe;
      }
    }
  }
  DEBUG((EFI_D_ERROR, "                  Can't find Ioe\n"));
  Status = EFI_SUCCESS;
  return Status;

_FoundIoe:
  //
  //Assign temp bus number
  //
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x01);
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x04);

  //
  // distinguish BIOS/SPI mode
  //
  DEBUG((EFI_D_INFO, "                  Rx1EA=0x%04x\n", MmioRead16(PCI_DEV_MMBASE(1, 0, 0) + 0x1EA)));
  if( BIT15 & MmioRead16(PCI_DEV_MMBASE(1, 0, 0) + 0x1EA) ) {
    DEBUG((EFI_D_INFO, "                  In BIOS mode\n"));
  } else {
    DEBUG((EFI_D_INFO, "                  In SPI mode - Exit\n"));
    MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
    MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);
    return Status;
  }

  //For BusX
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x18, 0x01);
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x19, 0x02);
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x1A, 0x04);
  //For BusX+1
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x18, 0x02);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x19, 0x03);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x1A, 0x04);
  //For PCIEIF
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x18, 0x03);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x19, 0x04);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x1A, 0x04);	

  //
  // Read VID DID
  //
  Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp3, 0, 0) + 0x00);
  DEBUG((EFI_D_INFO, "                  EPTRFC's DIDVID = 0x%08X (Should be 0x91011D17)\n",Tex));
  Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x00);
  DEBUG((EFI_D_INFO, "                  PCIEIF's DIDVID = 0x%08X (Should be 0x07221D17)\n",Tex));
  Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x00);
  DEBUG((EFI_D_INFO, "                  PESB's DIDVID = 0x%08X (Should be 0x07211D17)\n",Tex));
  Tex = MmioRead32(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x00);
  DEBUG((EFI_D_INFO, "                  PEEP's DIDVID = 0x%08X (Should be 0x071F1D17)\n",Tex));

  //
  // Enable BusMaster Enable(Rx04[2])  of PCIEIF
  //
  Tmpx = MmioRead8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x04);
  DEBUG((EFI_D_INFO, "                  The value of PCIEIF Rx04 is 0x%02x\n",Tmpx));
  Tmpx1 = ((Tmpx & (~BIT2))|BIT2);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x04, Tmpx1);
  DEBUG((EFI_D_INFO, "                  PCIEIF Rx04 will be set as 0x%02x\n",Tmpx1));
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x04, 0x07);
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x04, 0x07);
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04, 0x07);
  DEBUG((EFI_D_INFO, "                  [%x|%x|%x][%04x] Rx04=0x%02x\n", Tbus, Tdev, Tfunc, MmioRead16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x02), MmioRead8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04)));
  DEBUG((EFI_D_INFO, "                  [%x|0|0][%04x] Rx04=0x%02x\n", mBusX, MmioRead16(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x02), MmioRead8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x04)));
  DEBUG((EFI_D_INFO, "                  [%x|8|0][%04x] Rx04=0x%02x\n", mBusXp1, MmioRead16(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x02), MmioRead8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x04)));
  DEBUG((EFI_D_INFO, "                  [%x|0|0][%04x] Rx04=0x%02x\n", mBusXp2, MmioRead16(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x02), MmioRead8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x04)));

  //
  // If EPTRFC was hiden before, show it temporary
  //
  MmioWrite32(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x20, 0xFE02FE02);
  MmioWrite32(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x10, 0xFE028000);
  if( MmioRead8(0xFE028000 + 0x1100 + 0x52) & BIT7 ) {
    HideFlag = 1;
    MmioWrite8(0xFE028000 + 0x1100 + 0x52,  MmioRead8(0xFE028000 + 0x1100 + 0x52) & (~(BIT7)));
  } else {
    HideFlag = 0;
  }

  Tex = MmioRead32(PCI_DEV_MMBASE(mBusXp3, 0, 0) + 0x00);
  DEBUG((EFI_D_INFO, "                  EPTRFC's DIDVID = 0x%08X (Should be 0x91011106)\n",Tex));
  ////
  //This code path is the formal path
  //and the next part is just for debug
  ////

  Status = HandleIoeXhciFwImp(4, 0, 0);

  //
  // Clear bus number assignment
  //
  //For PCIEIF
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x18, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x19, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp2, 0, 0) + 0x1A, 0x00);	
  //For BusX+1
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x18, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x19, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusXp1, 8, 0) + 0x1A, 0x00);
  //For BusX
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x18, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x19, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(mBusX, 0, 0) + 0x1A, 0x00);


  // Hide EPTRFC after Autofill
  DEBUG((EFI_D_INFO, "                  HideFlag = %d\n",HideFlag));
  if(HideFlag){
    MmioWrite8(0xFE028000 + 0x1100 + 0x52, (MmioRead8(0xFE028000 + 0x1100 + 0x52) | (BIT7))); 
  }
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x10, 0x00000000);



  //MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x04, 0x07); //Mem enable
  MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x20, 0x0000); //Base mem
  MmioWrite16(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x22, 0x0000); //limit mem
  //switch upstream port
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x18, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x19, 0x00);
  MmioWrite8(PCI_DEV_MMBASE(Tbus, Tdev, Tfunc) + 0x1A, 0x00);

  return Status;
}
#endif


VOID
AsiaNbPtCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_ASIA_NB_PROTOCOL  *ptAsiaNb;
  ASIA_NB_CONFIGURATION *AsiaNbCfg;
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;

  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid, NULL, &ptAsiaNb);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  gBS->CloseEvent(Event);

  if(MmioRead16(IGD_PCI_REG(PCI_VID_REG)) == 0xFFFF){
    goto ProcExit;
  }  
  
  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdNbVcpFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){
    return;
  }

  AsiaNbCfg = (ASIA_NB_CONFIGURATION*)ptAsiaNb->NbCfg;
  ASSERT((UINT64)(UINTN)Buffer < SIZE_4GB);
  AsiaNbCfg->VcpFileBaseAddr = (UINT32)(UINTN)Buffer;   // ASIA will free this.
  AsiaNbCfg->VcpFileSize     = (UINT16)Size;
  
ProcExit:
  return;  
}


STATIC
EFI_STATUS
ReadFileFromFv(
  IN  CONST EFI_GUID    *NameGuid,
  IN  EFI_SECTION_TYPE  SectionType,
  IN  UINTN             SectionInstance,
  OUT VOID              **FileData,
  OUT UINTN             *FileDataSize
    )
{
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv2;
	UINT32                        AuthenticationStatus;
  EFI_FV_FILETYPE               FoundType;
  UINT32                        FvStatus;
  UINT32                        Attributes;
  VOID                          *MyFileData;
  UINTN                         MyFileDataSize;
  EFI_STATUS                    Status;
  UINTN                         Index;


  Fv2 = NULL;
	FvHandleCount  = 0;
	FvHandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvHandleCount,
                  &FvHandleBuffer
                  );
  if(EFI_ERROR(Status) || FvHandleCount==0){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  for (Index = 0; Index < FvHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    FvHandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv2
                    );
    ASSERT(!EFI_ERROR(Status));
    Status = Fv2->ReadFile (
                  Fv2,
                  NameGuid,
                  NULL,
                  &MyFileDataSize,
                  &FoundType,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (!EFI_ERROR(Status)) {
      break;
    }
  }
  if(Index >= FvHandleCount){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  MyFileData     = NULL;  //!!! NULL means memory allocated by callee.
  MyFileDataSize = 0;
  Status = Fv2->ReadSection (
                    Fv2,
                    NameGuid,
                    SectionType,
                    SectionInstance,
                    &MyFileData,
                    &MyFileDataSize,
                    &FvStatus
                    );
  if (!EFI_ERROR(Status)) {
    *FileData     = MyFileData;
    *FileDataSize = MyFileDataSize;
  }

ProcExit:
  if(FvHandleBuffer != NULL){
    gBS->FreePool(FvHandleBuffer);
  }
  return Status;
}

// JIH-Add PEMCU Option - s
STATIC EFI_STATUS HandlePeMcuFw(VOID)
{
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;
 EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;
  VOID                  *PeMcuFw;
   VOID                  *PeMcuData;
  UINT32                Crc32;


  /* VOID                  *McuFw;
  PLATFORM_NV_INFO      *NvInfo;
  EFI_HOB_GUID_TYPE     *GuidHob;
  UINT64                Tsc;*/

  DEBUG((EFI_D_ERROR, "[JNY-DXE]HandlePeMcu_1\n"));


  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdPeMcuFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_ERROR, "[JNY-DXE]HandlePeMcu_2:Status= %x\n",Status));

  if(EFI_ERROR(Status)){goto ProcExit;}
  
  //allocate mcu instruction space
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           		// need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  
  PeMcuFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);		
  DEBUG((EFI_D_ERROR, " PeMcuFw = %x  FwSize = %d[0x%x]Byte\n",PeMcuFw,Size,Size));

  CopyMem(PeMcuFw, Buffer, Size);								//Copy FW from Buffer to the space we allocated
  gBS->FreePool(Buffer);

  LibCalcCrc32(PeMcuFw, Size, &Crc32);
  gS3Record->PeMcuFwCrc32 = Crc32;
  gS3Record->PeMcuFwSize  = (UINT32)Size;  

  ASSERT((UINT64)(UINTN)PeMcuFw < SIZE_4GB);

  gS3Record->PeMcuFw = (UINT32)(UINTN)PeMcuFw;

  //allocate mcu data space
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           		// need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(			
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  PeMcuData = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);
  DEBUG((EFI_D_ERROR, " PeMcuData = %x  DataSize = %d[0x%x]Byte\n",PeMcuData,Size,Size));
  

  gBS->SetMem(PeMcuData,SIZE_64KB,0);

  LibCalcCrc32(PeMcuData, Size, &Crc32);
  gS3Record->PeMcuDataCrc32 = Crc32;
  gS3Record->PeMcuDataSize  = (UINT32)Size;  

  ASSERT((UINT64)(UINTN)PeMcuData < SIZE_4GB);

  gS3Record->PeMcuData = (UINT32)(UINTN)PeMcuData;

#if 1

 gS3Record->PeMcuDoEq = gSetupHob->PcieEQ;
 gS3Record->PeMcuTx = gSetupHob->EQTxPreset;
  Status = LoadPeMcuFw(PeMcuFw,PeMcuData, gSetupHob->PcieEQ, gSetupHob->EQTxPreset, 0);
  DEBUG((EFI_D_ERROR, "[JNY-DXE]HandlePeMcu_3 LoadPeMcuFw Status = %x\n",Status));
#else
  // Jimmy Debug: set pcie ephy base address
  //MmioWrite32((0xE0005000|PEMCU_MMIO_BAR_ADDR),0x00FE0140); //TGR del it, ASIA has set it in PEIM.

  PCIEPhyMMIOBase=(UINTN)MmioRead32((PCI_DEV_MMBASE(0,0,5)+D0F5_PCIE_EPHY_BASE_ADR)); 	//get base address
  PCIEPhyMMIOBase = PCIEPhyMMIOBase<<8;
  // PCIEPhyMMIOBase = 0xFE014000;
  DEBUG((DEBUG_ERROR," PCIEPhyMMIOBase: %x\n", PCIEPhyMMIOBase));
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR),0);						//set firmware auto fill address
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR));  // Jimmy Debug - For cache flush
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR: %x\n", TmpReg16));
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN),SIZE_8KB);					//set firmware auto fill length 8k?
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN));  // Jimmy Debug - For cache flush
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_LEN: %x\n", TmpReg16));
  
  FwAddr= (UINT32)((UINT64)((UINT32*)PeMcuFw));
  //FwAddr= (UINT32)(*(UINT32*)PeMcuFw);
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  FwAddr=FwAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION),FwAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION));  // Jimmy Debug - For cache flush
  //Flush cache[xxx]
  AsmWbinvd();

  DEBUG((EFI_D_ERROR, "[Line:%d] Flush Cache\n",__LINE__));

  
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_INST_AUTOFILL_EN); 			//auto fill enable
  TmpReg=0;
  while((TmpReg&0x02)==0) //wait for auto fill ready
  {
  	TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  }
  TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  DEBUG((EFI_D_ERROR, "[Line:%d] Fill done status = %X\n",__LINE__, TmpReg)); // Jimmy Debug - For cache flush

  DEBUG((EFI_D_ERROR, "[Line:%d] FW AutoFill Done\n",__LINE__));
  
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),0x00); 	
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_MCU_RST); //reset mcu

#if 1								 ///Add for Redo-EQ in DXE stage.  	

   if(gSetupHob->PcieEQ == 0){
  
  
  ///For Debug - Add by ChrisJWang 2015.07.31 
  ///[Possible Bug:When No UART debug message output, Pemcu AutoFill Failed]
  ///Those Code can check whether FW autofill success 
  gBS->Stall(100); 					//Wait 100us
  TmpReg = MmioRead8(PCIEPhyMMIOBase+PEMCU_RESET_AUTOFILL_EN);
  if(TmpReg == 0x00){
  	DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Failed\n"));
	while(1){
		IoWrite8(0x80,0x52); 						//80 Port show 0x52	
	}
  } 

  //After loaded,FW shold set Rx9D0=0x22
  /*
  TmpReg = 0x00;
  while(TmpReg != 0x22){
  	  //wait until it turn to 0x22(It mean FW runing successful)
	  TmpReg = MmioRead8(PCIEPhyMMIOBase + PCIEPHYCFG_PEMCU2BIOS_SCRATCH);
	  //DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass But  RxCCD!=0x22\n"));
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass But FW not running\n"));
	  IoWrite8(0x80,0x53);							//80 Port show 0x53
  }
  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass and FW running success\n"));
  */
  gBS->Stall(1000);         // 1 msec delay  


  DEBUG((EFI_D_ERROR,"TxPreset = SetupData->EQTxPreset - Start \n"));

  //CMD stage ,CMD Tx Preset = 3
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_BIOS2PEMCU_SCRATCH),0x03); 	

  gBS->Stall(1000);         // 1 msec delay  
  
  // Run CMD (DB=1)
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 	
		
  do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	

        }while(TmpReg == 0x01);

  //ARG stage
  TmpReg16 = gSetupHob->EQTxPreset;
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_BIOS2PEMCU_SCRATCH),TmpReg16); 	
	
  gBS->Stall(1000);         // 1 msec delay  


  // Run CMD (DB=1)
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3),PCIEPHYCFG_BIOS2PEMCU_DB); 	
		
  do{
		TmpReg = MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_EQ_INT_TO_MCU_Z3)); 	

        }while(TmpReg == 0x01);


  DEBUG((EFI_D_ERROR," TxPreset = SetupData->EQTxPreset - end \n")); 


  //Trigger Redo EQ  -Add by ChrisJWang 2015.08.31
  /*
  MmioWrite8(0xE0010000 + 0x70,0x02);
  do{
  	  //wait 
	  TmpReg = MmioRead8(0xE0010000 + 0x52);
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] Waiting for speed to Gen2\n"));
	  IoWrite8(0x80,0x54);							//80 Port show 0x53
  }while((TmpReg&0xF) != 0x02);
  DEBUG((EFI_D_ERROR, "[AutoFill-001] Speed to Gen2\n"));

  //gBS->Stall(5000000);
  
  MmioWrite8(0xE0010000 + 0x70,0x03);  //speed to gen3 : RedoEQ
  do{
  	  //wait 
	  TmpReg = MmioRead8(0xE0010000 + 0x52);
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] Waiting for speed to Gen3\n"));
	  IoWrite8(0x80,0x55);							//80 Port show 0x53
  }while((TmpReg&0xF) != 0x03);
  DEBUG((EFI_D_ERROR, "[AutoFill-001] Speed to Gen3\n"));
  */
  
}

#endif
#endif
  DEBUG((EFI_D_ERROR, "[Line:%d] Exit %a()\n", __LINE__, __FUNCTION__));
 
  ProcExit:
  return Status;
}
// JIH-Add PEMCU Option - e

VOID ClearCmosBad()
{
  UINT8  Cmos0E;

  Cmos0E = CmosRead(0xE);
  if(Cmos0E & BIT7){
    CmosWrite(0xE, Cmos0E & (UINT8)~BIT7);
  }
}


EFI_STATUS
EFIAPI
Sha256Hash (
  IN  CONST VOID        *Data,
  IN        UINTN       DataLen,
  OUT       UINT8       *Digest,
  IN        UINTN       DigestSize  
  )
{
  VOID     *Sha256Ctx;
  UINTN    CtxSize;

  ASSERT(Data != NULL && DataLen != 0 && Digest != NULL && DigestSize >= 32);

  CtxSize = Sha256GetContextSize();
  Sha256Ctx = AllocatePool(CtxSize);
  ASSERT(Sha256Ctx != NULL);

  Sha256Init (Sha256Ctx);
  Sha256Update (Sha256Ctx, Data, DataLen);
  Sha256Final (Sha256Ctx, Digest);

  FreePool(Sha256Ctx);

  return EFI_SUCCESS;
}



VOID
VariableWriteReadyCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  VOID                 *Interface;
  EFI_STATUS           Status;
  UINTN                VarSize;
  TSESETUP             *Tse;
  BOOLEAN              IsCmosBad;


  Status = gBS->LocateProtocol(&gEfiVariableWriteArchProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);


//------------------------------------------------------------------------------
  if(PcdGetBool(PcdVarServiceUseEmu)){
    Status = gRT->SetVariable (
                    PLATFORM_SETUP_VARIABLE_NAME,
                    &gPlatformSetupVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(SETUP_DATA),
                    (VOID*)gSetupHob
                    );
    DEBUG((EFI_D_INFO, "[EMU] Sync SetupData:%r\n"));		
  }

  IsCmosBad = (CmosRead(0x0E) & BIT7)?TRUE:FALSE;

  VarSize = sizeof(TSESETUP);
  Tse     = &gTse;
  Status  = gRT->GetVariable (
                  SETUP_PASSWORD_VARTSE_NAME,
                  &gSetupPasswordVariableTseGuid,
                  NULL,
                  &VarSize,
                  Tse
                  );
  if(Status == EFI_NOT_FOUND || IsCmosBad){
    VarSize = sizeof(TSESETUP);
    ZeroMem(Tse, VarSize);
    
    if(gDefaultSetupAdminPassword[0]){
      Sha256Hash(
        gDefaultSetupAdminPassword, 
        StrSize(gDefaultSetupAdminPassword), 
        Tse->AdminPasswdHash, 
        sizeof(Tse->AdminPasswdHash)
        );
      Tse->AdminPasswdPresent = TRUE;
    }

    Status = gRT->SetVariable (
                    SETUP_PASSWORD_VARTSE_NAME,
                    &gSetupPasswordVariableTseGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VarSize,
                    Tse
                    );
  }
  ASSERT_EFI_ERROR(Status);






  if(IsCmosBad){ClearCmosBad();}	
}


typedef struct {
  UINT32  StartAddr;
  UINT32  RangeSize;
}	EFI_RESERVED_MEMORY_LIST;

/*
[037]    MMIO 00000000FE011000 00000000FE012FFF 0000000000002000 UC RT
[038]    MMIO 00000000FE014000 00000000FE015FFF 0000000000002000 UC RT
[039]    MMIO 00000000FEC00000 00000000FEC3FFFF 0000000000040000 UC RT
[040]    MMIO 00000000FECC0000 00000000FED7FFFF 00000000000C0000 UC RT
[041]    MMIO 00000000FEE00000 00000000FEEFFFFF 0000000000100000 UC RT
[042]    MMIO 00000000FF800000 00000000FFFFFFFF 0000000000800000 UC RT
*/
STATIC EFI_RESERVED_MEMORY_LIST gRsvdMemList[] = {
  {0xFE011000, 0x00002000},   // RCRB V+H
  {0xFE014000, 0x00004000},   // PcieEPHY 
  {0xFE020000, 0x00001000},// CRMCA Bar  
  {0xFEC00000, 0x00001000},   // ApicBar
  {0xFECC0000, 0x00001000},   // NbApicBaseAddress
  {0xFEE00000, 0x00100000},   // CPU_MSI_MEMORY_BASE
  {0xFEF00000, 0X00100000},  // EX-IPI range
  {_PCD_VALUE_PcdPciExpressBaseAddress, SIZE_256MB}, // E000 0000 -- F000 0000
  {_PCD_VALUE_PcdFlashAreaBaseAddress, _PCD_VALUE_PcdFlashAreaSize},   // bios, FFC0 0000, Size = 4MB
};  


VOID SetReservedMmio()
{
  UINTN       Index;  
  EFI_STATUS  Status;
  
  for(Index = 0; Index < sizeof(gRsvdMemList)/sizeof(gRsvdMemList[0]); Index++){
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeReserved,
                    gRsvdMemList[Index].StartAddr,
                    gRsvdMemList[Index].RangeSize,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
                    gRsvdMemList[Index].StartAddr, 
                    gRsvdMemList[Index].RangeSize,
                    EFI_MEMORY_RUNTIME
                    );
    DEBUG((EFI_D_INFO, "[%d] %X %X %r(%lX)\n", Index, \
      gRsvdMemList[Index].StartAddr, gRsvdMemList[Index].RangeSize, Status, (UINT64)Status));    
    ASSERT_EFI_ERROR(Status);    
  }
}

VOID
PCIE_EarlyDXE_80_PORT(UINT8 Data)
 {
	UINT8 PostCodeValue=Data;
	IoWrite8(0x80,PostCodeValue);
	DEBUG((EFI_D_ERROR, "POSTCODE=<%02x>\n", IoRead8(0x80)));
 }

EFI_STATUS
EFIAPI
PlatformEarlyDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  VOID                          *Registration;
  EFI_BOOT_MODE                 BootMode;
  EFI_HANDLE                    Handle = NULL;
  UINT8                         Data8;	
  UINT32                        SystemMiscCfg;


  SetReservedMmio();

  BootMode = GetBootModeHob();
  DEBUG((EFI_D_INFO, "BootMode:%X\n", BootMode));

  gSetupHob = GetSetupDataHobData();
  gS3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(gS3Record->Signature == PLAT_S3_RECORD_SIGNATURE);
#ifdef IOE_EXIST
  ////JNY-CND003 FW Autofill -S
	  //;cjw-A0-debug 20161103
	  if(gSetupHob->Cnd003Autofill == 1){
		  DEBUG((DEBUG_ERROR,"[CJW_IOE_FW] Setup: Enable CND003 Autofill\n")); 
		  if (BootMode != BOOT_IN_RECOVERY_MODE) {
			  Status= HandleIoeMcuFw();
			  ASSERT_EFI_ERROR(Status);
		  }
	  }else{
		  DEBUG((DEBUG_ERROR,"[CJW_IOE_FW] Setup: Skip CND003 Autofill\n")); 
	  }
	  //;cjw-A0-debug END

    if (BootMode != BOOT_IN_RECOVERY_MODE) {
        Status= HandleIoeXhciFw();
        ASSERT_EFI_ERROR(Status);
    }
////JNY-CND003 FW autofill -E	  
#endif

// JIH-Add PEMCU Option - s
#if 1
//open this for enable EQ FW load
if(gSetupHob->PEMCU_LoadFW_WhenBoot){
	DEBUG((DEBUG_ERROR,"\nPCIe PeMcu FW...\n"));  //ChrisJWang Add for Debug
	DEBUG((DEBUG_ERROR,"\n[JNY-DXE]PCIe PeMcu FW Handle start...\n")); 
  	Status=HandlePeMcuFw();
	DEBUG((DEBUG_ERROR,"\n[JNY-DXE]PCIe PeMcu FW Status:%x\n",Status)); 
       ASSERT_EFI_ERROR(Status);
}
#endif
  // JIH-Add PEMCU Option - e
 PCIE_EarlyDXE_80_PORT(PCIE_EaryDXE_PEMCU_FW_LOAD);

  if (BootMode != BOOT_IN_RECOVERY_MODE) {
    Status = HandleXhciFw(gSetupHob); // for Normal Boot, will load XHCI FW.
    ASSERT_EFI_ERROR(Status);
  }

  PcdSet8(PcdLegacyBiosSupport, gSetupHob->Csm);
  PcdSet8(PcdDisplayOptionRomMessage, gSetupHob->OpromMessage);
  PcdSet8(PcdUCREnable, gSetupHob->UCREnable);
  PcdSetBool(PcdSerialTerminalSupport, gSetupHob->UCREnable);	


  SystemMiscCfg = PcdGet32(PcdSystemMiscConfig);
  if(gSetupHob->VideoDualVga){
    SystemMiscCfg |= SYS_MISC_CFG_DUAL_VGA;
  }  
  if(gSetupHob->VideoPrimaryAdapter == DISPLAY_PRIMARY_IGD){
    SystemMiscCfg |= SYS_MISC_CFG_PRI_VGA_IGD;
  }
  if(gSetupHob->Pci64){
    SystemMiscCfg |= SYS_MISC_CFG_PCI64;
  }  
  PcdSet32(PcdSystemMiscConfig, SystemMiscCfg);

  

  switch(gSetupHob->SMRREnable){
    case SMRR_TYPE_DISABLE:
      gS3Record->SmrrType = 0xFF;
      break;
      
    default:
    case SMRR_TYPE_WT:
      gS3Record->SmrrType = CacheWriteThrough;
      break;

    case SMRR_TYPE_WB:
      gS3Record->SmrrType = CacheWriteBack;
      break;       
  }

// 0 - all
// 1 - Uefi only
// 2 - Legacy only
  Data8 = gSetupHob->BootModeType;
  if(!gSetupHob->Csm){Data8 = 1;}
  PcdSet8(PcdBiosBootModeType, Data8);

// default FALSE.
  if(BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
    PcdSetBool(PcdVarServiceUseEmu, TRUE);
  }	

  if (!gSetupHob->Csm) {              // EFI only
    gBS->InstallProtocolInterface (
           &Handle,
           &gIgdGopDepexProtocolGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
    DEBUG((EFI_D_INFO, "InsIgdGopDpx\n"));
  }

// csm LegacyPXE
//   0       N/A    (v)
//   1         0    (v)
//   1         1    (x)
  if(gSetupHob->OnboardLan && 
    !(gSetupHob->Csm && gSetupHob->LegacyPxeBoot) && 
    (gSetupHob->UefiPxeIPV4 || gSetupHob->UefiPxeIPV6)){
    gBS->InstallProtocolInterface (
           &Handle,
           &gObLanUndiDxeDepexProtocolGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
  }    

  PcdSet8(PcdInternalShellEnable, gSetupHob->ShellEn);

  EfiCreateProtocolNotifyEvent (
    &gAsiaNbProtocolGuid,
    TPL_CALLBACK,
    AsiaNbPtCallBack,
    NULL,
    &Registration
    );

  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    VariableWriteReadyCallBack,
    NULL,
    &Registration
    );

  DEBUG((EFI_D_INFO, "PlatformEarlyDxe Exit\n"));

  return EFI_SUCCESS;
}



