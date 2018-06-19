//************************************************//
//                                                //
//   for internal Ref                             //
//                                                //
//************************************************//

/*
Module Name:

  PeiDumpReg.h
    
Abstract:

 PEI DUMP REG PPI 
*/

#include "PeiDumpReg.h"

PEI_DUMP_REG_PPI mPeiDumpReg={
	DumpNBValue,
	DumpSBValue,
	DumpS3Reg
};

static dumpRegister SBRegList[] = {
   	{L"D17F0",								DUMP_CFG,				CHX002_DUMP_D17F0,		0x00, 		0xFF},
	{L"D17F0_MMIO", 						DUMP_MMIO,				CHX002_DUMP_D17F0_MMIO, 0x00,		0x12F},
    //{L"D10F1",								DUMP_CFG,				CHX002_DUMP_D10F1, 		0x00, 		0xFF},
	{L"D11F0", 						DUMP_CFG,				CHX002_DUMP_D11F0,		0x00,		0xFF},
	{L"D11F0_MMIO",							DUMP_MMIO,				CHX002_DUMP_D11F0_MMIO, 			0x00,		0x2FF}, 
    {L"D17F7",								DUMP_CFG,				CHX002_DUMP_D17F7,		0x00, 		0xFF},		//;josh D17F7
	//{L"D10F1_MMIO",							DUMP_MMIO,				CHX002_DUMP_D10F1_MMIO, 			0x00,		0x7F}, 
	{L"D15F0(IDE)", 						DUMP_CFG,				CHX002_DUMP_D15F0_SATA, 0x00,		0xFF},
	{L"D15F0_EPHY",					DUMP_IDX,				CHX002_DUMP_D15F0_EPHY,				0x00,		0x14F},
	{L"D15F0_MSI_XT",					DUMP_MMIO,				CHX002_DUMP_D15F0_MSI_XT,				0x00,		0x3F},	
	{L"D15F0_MSI_XP",					DUMP_MMIO,				CHX002_DUMP_D15F0_MSI_XP,				0x00,		0x0F},
	{L"D15F0_AHCI_MMIO", 					DUMP_MMIO,				CHX002_DUMP_D15F0_AHCI_MMIO, 			0x00,		0xFF},
	{L"D15F0_AHCI_MMIO_P0",					DUMP_MMIO,				CHX002_DUMP_D15F0_AHCI_MMIO_P0,				0x00,		0x47},
	{L"D15F0_AHCI_MMIO_P1", 				DUMP_MMIO,				CHX002_DUMP_D15F0_AHCI_MMIO_P1,				0x00,		0x47},
	//{L"D15F0_PRIMARY_CHANNEL_COMMAND_IO",	DUMP_IO,				CHX002_DUMP_D15F0_PRIMARY_CHANNEL_COMMAND_IO,					0x00,		0x07},
	//{L"D15F0_PRIMARY_CHANNEL_CONTROL_IO",	DUMP_IO,				CHX002_DUMP_D15F0_PRIMARY_CHANNEL_CONTROL_IO,					0x00,		0x03},	
	//{L"D15F0_BUS_MASTER_IDE",			DUMP_IO,				CHX002_DUMP_D15F0_BUS_MASTER_IDE_IO,					0x00,		0x17},	
	{L"HPET",								DUMP_MMIO,				CHX002_DUMP_HPET,		0x00,		0x15F}, 
	{L"SPI",								DUMP_MMIO,				CHX002_DUMP_SPI,		0x00,		0xFF},		
	{L"SMIO",								DUMP_IO,				CHX002_DUMP_SMIO,					0x00,		0x0F},	
	{L"PMIO",								DUMP_IO,				CHX002_DUMP_PMIO,					0x00,		0xFF},	
	{L"D14F0",						DUMP_CFG,				CHX002_DUMP_D14F0_USB,	0x00,		0xFF},
	{L"D14F7",						DUMP_CFG,				CHX002_DUMP_D14F7_USB,	0x00,		0xFF}, 
	{L"D16F0",						DUMP_CFG,				CHX002_DUMP_D16F0_USB,	0x00,		0xFF}, 
	{L"D16F1",						DUMP_CFG,				CHX002_DUMP_D16F1_USB,	0x00,		0xFF}, 
	{L"D16F7",						DUMP_CFG,				CHX002_DUMP_D16F7_USB,	0x00,		0xFF}, 
	{L"D18F0",							DUMP_CFG,			CHX002_DUMP_D18F0,		0x00,		0xFF}, 
	{L"D14F7_MMIO",					DUMP_MMIO,				CHX002_DUMP_D14F7_MMIO,				0x00,		0xBF}, 
	{L"D16F7_MMIO", 						DUMP_MMIO,				CHX002_DUMP_D16F7_MMIO, 			0x00,		0xBF}, 
	{L"D18F0_MMIO",						DUMP_MMIO,				CHX002_DUMP_D18F0_MMIO, 			0x00,		0x4BF}, 
	{L"D14F7_PHY_CTRL",				DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_PHY_CTRL,				0x00,		0x1F}, 
	{L"D14F7_P1_PHY",					DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_P1_PHY,				0x1000,		0x100F}, 
	{L"D14F7_P2_PHY", 				DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_P2_PHY, 			0x2000,		0x200F},
	{L"D14F7_PHY_TEST_CTRL", 			DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_PHY_TEST_CTRL, 			0x10000,	0x1000F},
	{L"D14F7_PHY_P1_TEST_CTRL",    		DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_PHY_P1_TEST_CTRL, 			0x11000,	0x1100F},
	{L"D14F7_PHY_P2_TEST_CTRL",			DUMP_USB2_D14F7_L2,		CHX002_DUMP_D14F7_PHY_P2_TEST_CTRL,				0x12000,	0x1200F},
	{L"D16F7_PHY_CTRL",				DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_CTRL,				0x00,		0x1F},
	{L"D16F7_PHY_P1_CTRL",					DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P1_CTRL,				0x1000,		0x100F},
	{L"D16F7_PHY_P2_CTRL",					DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P2_CTRL,				0x2000,		0x200F},	
	{L"D16F7_PHY_P3_CTRL", 				DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P3_CTRL,				0x3000,		0x300F},	
	{L"D16F7_PHY_P4_CTRL", 				DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P4_CTRL,				0x4000,		0x400F},	
	{L"D16F7_PHY_TEST_CTRL", 			DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_TEST_CTRL,				0x10000,	0x1000F},	//-10000	
	{L"D16F7_PHY_P1_TEST_CTRL", 			DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P1_TEST_CTRL,				0x11000,	0x1100F},//-10000		
	{L"D16F7_PHY_P2_TEST_CTRL", 			DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P2_TEST_CTRL,				0x12000,	0x1200F},//-10000		
	{L"D16F7_PHY_P3_TEST_CTRL", 			DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P3_TEST_CTRL,				0x13000,	0x1300F},//-10000		
	{L"D16F7_PHY_P4_TEST_CTRL", 			DUMP_USB2_D16F7_L2,		CHX002_DUMP_D16F7_PHY_P4_TEST_CTRL,				0x14000,	0x1400F},//-10000	
	{L"D18F0_OPTCFG_XHCI",					DUMP_USB3_D18F0_L2,		CHX002_DUMP_D18F0_OPTCFG_XHCI,				0x00,		0x16F},	//-10000	
	{L"D18F0_OPTCFG_HSBI",					DUMP_USB3_D18F0_L2,		CHX002_DUMP_D18F0_OPTCFG_HSBI,				0x10000,	0x1063F},//-10000	
	{L"D18F0_U3IP_SSCFG_C",				DUMP_USB3_D18F0_L2,		CHX002_DUMP_D18F0_U3IP_SSCFG_C,				0x20000,	0x200FF},	//-10000	
	{L"D18F0_U3IP_SSCFG_PA",				DUMP_USB3_D18F0_L2,		CHX002_DUMP_D18F0_U3IP_SSCFG_P,				0x20100,	0x201DF},//-10000		
	{L"D18F0_U3IP_SSCFG_PB",		 DUMP_USB3_D18F0_L2, CHX002_DUMP_D18F0_U3IP_SSCFG_P, 0x20200, 0x202DF}, //-10000	
	{L"D18F0_OPTCFG_MCU",					DUMP_USB3_D18F0_L2,		CHX002_DUMP_D18F0_OPTCFG_MCU,				0x30000,	0x3004F},		
	{L"D14F0_IO",							DUMP_IO,				CHX002_DUMP_D14F0_IO,		0x00,		0x1F},	
	{L"D16F0_IO", 							DUMP_IO,				CHX002_DUMP_D16F0_IO,		0x00,		0x1F},	
	{L"D16F1_IO", 							DUMP_IO,				CHX002_DUMP_D16F1_IO,		0x00,		0x1F},	
	{L"WT_MMIO",							DUMP_MMIO,				CHX002_DUMP_WT_MMIO,	0x00,		0xF},
	{L"D20F0",								DUMP_CFG,				CHX002_DUMP_D20F0,		0x00,		0x14F},
	{L"D20F0_MMIO",					DUMP_MMIO,				CHX002_DUMP_D20F0_MMIO, 			0x00,		0x2167},
	//{L"D20F0_MMIO_Part2",					DUMP_MMIO,				CHX002_DUMP_D20F0_MMIO, 			0x2030, 	0x2167},	
};

static dumpRegister NBRegList[] = {
  	{L"D0F0",		DUMP_CFG,	CHX002_DUMP_D0F0,		0x00, 	0x4FF},
	{L"D0F1",		DUMP_CFG,	CHX002_DUMP_D0F1,		0x00,	0xFF},	
	{L"D0F2",		DUMP_CFG,	CHX002_DUMP_D0F2,		0x00,	0xFF},
	{L"D0F3",		DUMP_CFG,	CHX002_DUMP_D0F3,		0x00,	0xFFF},		
	{L"D0F4",		DUMP_CFG,	CHX002_DUMP_D0F4,		0x00,	0xFF},
	{L"D0F5",		DUMP_CFG,	CHX002_DUMP_D0F5,		0x00,	0x2FF},
	{L"D0F6",		DUMP_CFG,	CHX002_DUMP_D0F6,		0x00,	0xFF},
    {L"D3F0",		DUMP_CFG,	CHX002_DUMP_D3F0,		0x00, 	0x2EF},
 	//{L"D3F1",		DUMP_CFG,	CHX002_DUMP_D3F1,		0x00,	0x2EF},	
 	{L"D3F2",		DUMP_CFG,	CHX002_DUMP_D3F2,		0x00,	0x2EF},
 	{L"D3F3",		DUMP_CFG,	CHX002_DUMP_D3F3,		0x00,	0x2EF},		
	{L"D4F0",		DUMP_CFG,	CHX002_DUMP_D4F0,		0x00,	0x2EF},
 	//{L"D4F1",		DUMP_CFG,	CHX002_DUMP_D4F1,		0x00,	0x2EF},
 	{L"D5F0",		DUMP_CFG,	CHX002_DUMP_D5F0,		0x00,	0x2EF},	
 	{L"D5F1",		DUMP_CFG,	CHX002_DUMP_D5F1,		0x00,	0x2EF},	
	{L"D7F0",		DUMP_CFG,	CHX002_DUMP_D7F0,		0x00,	0x84F}, 
	{L"D8F0",		DUMP_CFG,	CHX002_DUMP_D8F0,		0x00,	0xFF},
	{L"D9F0",		DUMP_CFG,	CHX002_DUMP_D9F0,		0x00,	0xFF},
	{L"D8F0_MMIO",	DUMP_MMIO,	CHX002_DUMP_D9F0_MMIO,				0x00,	0xFF},	
	{L"D9F0_MMIO",	DUMP_MMIO,	CHX002_DUMP_D8F0_MMIO,				0x00,	0xFF},
	{L"RCRB_H",  	DUMP_MMIO,	CHX002_DUMP_RCRB_H,	   		 	0x00,	0x2BF},
	{L"PCIE_PHY",	DUMP_MMIO,	CHX002_DUMP_PCIE_EPHY,	0x00,	0x3FFF},
	//{L"PEMCU",		DUMP_MMIO,	CHX002_DUMP_PEMCU,		0x00,	0x2F},
	{L"TACTL",  	DUMP_MMIO,	CHX002_DUMP_TACTL,	    		0x00,	0x2FF},
	{L"CRMCA",		DUMP_MMIO,	CHX002_DUMP_CRMCA,		0x00,	0xFF},
};

VOID EfiPciAddr2PciCfgAddr (
    IN EFI_PCI_CONFIGURATION_ADDRESS    *PciAddr )
{
	UINT32	PciCfg32 = ( 0x80000000	| (PciAddr->Bus	<< 16) | \
						 (PciAddr->Device << 11) | \
						 (PciAddr->Function	<< 8) |	\
						 (PciAddr->Register	& 0xfc)	);
//	DEBUG((EFI_D_ERROR, "DLA:EfiPciAddr2PciCfgAddr()=%x\n",PciCfg32));	

	IoWrite32(0xcf8, PciCfg32);
}
UINT8 ReadPci8 (
    IN UINT64   PciBusDevFunReg )
{
//	DEBUG((EFI_D_ERROR, "DLA:ReadPci8(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	
	return IoRead8(0xcfc | (UINT8)(PciBusDevFunReg & 3));
}
UINT16 ReadPci16 (
    IN UINT64   PciBusDevFunReg )
{
//	DEBUG((EFI_D_ERROR, "DLA:ReadPci16(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	return IoRead16(0xcfc |	(UINT8)(PciBusDevFunReg	& 2));
}
UINT32 ReadPci32 (
    IN UINT64   PciBusDevFunReg )
{
//	DEBUG((EFI_D_ERROR, "DLA:PciBusDevFunReg(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	return IoRead32(0xcfc);
}
VOID WritePci8 (
    IN UINT64   PciBusDevFunReg,
    IN UINT8    WriteValue8 )
{
//	DEBUG((EFI_D_ERROR, "DLA:WritePci8(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	IoWrite8(0xcfc | (UINT8)(PciBusDevFunReg & 3), WriteValue8);
}
VOID WritePci16 (
    IN UINT64   PciBusDevFunReg,
    IN UINT16   WriteValue16 )
{
//	DEBUG((EFI_D_ERROR, "DLA:WritePci16(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	IoWrite16(0xcfc	| (UINT8)(PciBusDevFunReg &	2),	WriteValue16);
}
VOID WritePci32 (
    IN UINT64   PciBusDevFunReg,
    IN UINT32   WriteValue32 )
{
//	DEBUG((EFI_D_ERROR, "DLA:WritePci32(%x)\n",PciBusDevFunReg));	

	EfiPciAddr2PciCfgAddr((EFI_PCI_CONFIGURATION_ADDRESS *)&PciBusDevFunReg);
	IoWrite32(0xcfc, WriteValue32);
}
UINT8 RwPci8 (
    IN UINT64   PciBusDevFunReg,
    IN UINT8    SetBit8,
    IN UINT8    ResetBit8 )
{
	UINT8	Buffer8;
	
//	DEBUG((EFI_D_ERROR, "DLA:RwPci8(%x)\n",PciBusDevFunReg));	
	Buffer8 = ReadPci8(PciBusDevFunReg) & (~ResetBit8) | SetBit8;		//Reg -> NAND(Resetbit8) ->	OR(SetBit8)
	WritePci8(PciBusDevFunReg, Buffer8);
	return Buffer8;
}

void Print_Binary(
	IN		UINT8 data)
{
        UINT8 i;
	
        for(i = 7; i > 0; i--)
        {
			if((data >> i) & 0x01) 
				
				DEBUG((EFI_D_ERROR,"1"));
			else
				DEBUG((EFI_D_ERROR,"0"));				
        }
		if(data & 0x01)			
			DEBUG((EFI_D_ERROR,"1"));
		else
			DEBUG((EFI_D_ERROR,"0"));				
			
			DEBUG((EFI_D_ERROR," "));				
 }

void SB_beforeDump(UINT32 baseAddress) {
	UINT32 buffer32;
	UINT8 buffer8;
	UINT16 buffer16;
	switch (baseAddress) {		
		case CHX002_DUMP_D10F1:
			RwPci8(CHX002_BUSC|0x48, 0x02, 0);	//;Set bit, clear bit
			RwPci8(CHX002_BUSC|0XB4, 0x00, 0x80);	//;Set bit, clear bit	
			break;
		case CHX002_DUMP_D10F1_MMIO:
			RwPci8(CHX002_BUSC|0X48, 0x02, 0);	//;Set bit, clear bit
			RwPci8(CHX002_BUSC|0XB4, 0x00, 0x80);	//;Set bit, clear bit	
			buffer32=CHX002_DUMP_D10F1_MMIO;
			WritePci32(CHX002_UART1|0x10, buffer32);
			RwPci8(CHX002_UART1|0x4, 0x02, 0);	//;Set bit, clear bit			
			break;
		case CHX002_DUMP_D11F0:
			buffer8=IoRead8(0x800+0xE7);
			buffer8=buffer8|0x01;
			IoWrite8(0x800+0xE7, buffer8);		
			break;
		case CHX002_DUMP_D11F0_MMIO:			
			RwPci8(CHX002_ESPI|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer32=CHX002_DUMP_D11F0_MMIO;
			WritePci32(CHX002_ESPI|0x10, buffer32);
			break;
		case CHX002_DUMP_D14F0_USB:
		case CHX002_DUMP_D14F7_USB:
		case CHX002_DUMP_D16F0_USB:		
		case CHX002_DUMP_D16F1_USB: 	
		case CHX002_DUMP_D16F7_USB:
			RwPci8(CHX002_BUSC|0x50, BIT2|BIT4|BIT5, 0);	//;Set bit, clear bit
			buffer8=MmioRead8(CHX002_DUMP_D17F0_MMIO+0x40);
			buffer8|=BIT0|BIT3;
			MmioWrite8(CHX002_DUMP_D17F0_MMIO+0x40, buffer8);	
			break;
		case CHX002_DUMP_D18F0: 	
			//FOR XHCI
			buffer8=IoRead8(0x800+0x80);
			buffer8|=BIT3;
			IoWrite8(0x800+0x80, buffer8);			
			//DEBUG((EFI_D_ERROR,"DLA debug. AF:IO 0x880)=0x%x\n",IoRead8(0x800+0x80)));
			//D0F0: CICFG::RxF7-F6[15:14] =11b
			RwPci8(CHX002_GFXAX|0xF7, 0xC0, 0);	//;Set bit, clear bit			
			//DEBUG((EFI_D_ERROR,"DLA debug. AF:D0F0 0xF7)=0x%x\n",ReadPci8(CHX002_GFXAX|0xF7)));
			break;

		case CHX002_DUMP_D14F7_MMIO:			
			RwPci8(CHX002_D14F7EHCI|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer32=CHX002_DUMP_D14F7_MMIO;
			WritePci32(CHX002_D14F7EHCI|0x10, buffer32);
			break;
		case CHX002_DUMP_D15F0_MSI_XT:
			WritePci8(CHX002_SATA|0x04,0x07);
			WritePci8(CHX002_SATA|D15F0_MISC_CTL_2, 0x80);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x06);
			WritePci32(CHX002_SATA|D15F0_SATA_MSIXT_BASE_ADR, CHX002_DUMP_D15F0_MSI_XT);
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af sata MSIXT bar(%2X)\n",ReadPci32(CHX002_SATA|D15F0_SATA_MSIXT_BASE_ADR)));
			break;		
		case CHX002_DUMP_D15F0_MSI_XP:
			WritePci8(CHX002_SATA|D15F0_MISC_CTL_2, 0x80);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x06);
			WritePci32(CHX002_SATA|D15F0_SATA_MSIXP_BASE_ADR, CHX002_DUMP_D15F0_MSI_XP);
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af sata MSIXP bar(%2X)\n",ReadPci32(CHX002_SATA|D15F0_SATA_MSIXP_BASE_ADR)));
			break;	
		case CHX002_DUMP_D15F0_AHCI_MMIO:		
		case CHX002_DUMP_D15F0_AHCI_MMIO_P0:
		case CHX002_DUMP_D15F0_AHCI_MMIO_P1:
			WritePci8(CHX002_SATA|D15F0_MISC_CTL_2, 0x80);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x06);
			WritePci32(CHX002_SATA|D15F0_SATA_MMIO_GLOBAL_BASE_ADR, CHX002_DUMP_D15F0_AHCI_MMIO);
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af sata mmio bar(%2X)\n",ReadPci32(CHX002_SATA|D15F0_SATA_MMIO_GLOBAL_BASE_ADR)));
			break;		

		case CHX002_DUMP_D16F7_MMIO:
			RwPci8(CHX002_EHCI|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer32=CHX002_DUMP_D16F7_MMIO;
			WritePci32(CHX002_EHCI|0x10, buffer32);
			break;			


		case CHX002_DUMP_D17F0_MMIO:
			buffer32 = ReadPci32(CHX002_BUSC|D17F0_MMIO_SPACE_BASE_ADR);
			buffer32 = (buffer32 & 0xFF000000) | (CHX002_DUMP_D17F0_MMIO >> 8);
			WritePci32(CHX002_BUSC|0xBC, buffer32);
			break;

		case CHX002_DUMP_WT_MMIO:
			buffer32 = CHX002_DUMP_WT_MMIO;
			WritePci32(CHX002_BUSC| 0xE8, buffer32);
			buffer8 = 0x03;
			WritePci8(CHX002_BUSC| 0xEC, buffer8);
			break;

		case CHX002_DUMP_D18F0_MMIO:
			RwPci8(CHX002_XHCI|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer32=CHX002_DUMP_D18F0_MMIO;
			WritePci32(CHX002_XHCI|0x10, buffer32);
			break;			

		case CHX002_DUMP_D20F0:
			RwPci8(CHX002_PCCA|0xD1, 0x00, 0x04);	//;Set bit, clear bit
			RwPci8(CHX002_PCCA|0x74, 0x20, 0x20);	//;Set bit, clear bit
			break;
			
		case CHX002_DUMP_D20F0_MMIO:
			//DEBUG((EFI_D_ERROR,"DLA debug. CHX002_DUMP_D20F0_MMIO\n"));
			RwPci8(CHX002_PCCA|0xD1, 0x00, 0x04);	//;Set bit, clear bit
			buffer32=CHX002_DUMP_D20F0_MMIO;
			WritePci32(CHX002_HDAC|0x10, buffer32);
			RwPci8(CHX002_HDAC|0x4, 0x02, 0x03);	//;Set bit, clear bit			
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af D20F0 Rx10(%2X)\n",ReadPci32(CHX002_HDAC|0x10)));
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af D20F0 Rx04(%2X)\n",ReadPci32(CHX002_HDAC|0x4)));
			break;	

		case CHX002_DUMP_HPET:
			buffer32 = ReadPci32(CHX002_BUSC|0x68);
			buffer32 = ((buffer32) | (0x80|(CHX002_DUMP_HPET)));
			WritePci32(CHX002_BUSC|0x68, buffer32);
			break;

		case CHX002_DUMP_SPI:
			buffer32 = ReadPci32(CHX002_BUSC|0xBC);
			buffer32 = (buffer32 & 0xFF000000) | (CHX002_DUMP_D17F0_MMIO >> 8);
			WritePci32(CHX002_BUSC|0xBC, buffer32);
			MmioWrite32(CHX002_DUMP_D17F0_MMIO, CHX002_DUMP_SPI);
			buffer32=MmioRead32(CHX002_DUMP_D17F0_MMIO);
			buffer32|=0x01;
			MmioWrite32(CHX002_DUMP_D17F0_MMIO, buffer32);			
			break;
			

		case CHX002_DUMP_D14F0_IO:			
			RwPci8(CHX002_D14F0UHCI|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer16 = CHX002_DUMP_D14F0_IO;
			WritePci16(CHX002_D14F0UHCI|0x20, buffer16);
			break;

		case CHX002_DUMP_D16F0_IO:
			
			RwPci8(CHX002_UHC0|0x04, 0x03, 0);	//;Set bit, clear bit
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-bf D16F0 Rx20=0x%x\n",ReadPci32(CHX002_UHC0|0x20)));
			buffer32 = CHX002_DUMP_D16F0_IO;
			WritePci32(CHX002_UHC0|0x20, buffer32);
			//DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af D16F0 Rx20=0x%x\n",ReadPci32(CHX002_UHC0|0x20)));
		 	buffer32=ReadPci32(CHX002_EHCI|0xA0);
/*			DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-bf D16F7 RxA0=0x%x\n",ReadPci32(CHX002_EHCI|0xA0)));
			buffer32=0xBBBBBB00| (0xFF & buffer32); 
	        WritePci32 ((CHX002_EHCI|0xA0), buffer32); 
			DEBUG((EFI_D_ERROR,"DLA debug. DumpReg-af D16F7 RxA0=0x%x\n",ReadPci32(CHX002_EHCI|0xA0)));
*/			break;

		case CHX002_DUMP_D16F1_IO:			
			RwPci8(CHX002_UHC1|0x04, 0x03, 0);	//;Set bit, clear bit
			buffer16 = CHX002_DUMP_D16F1_IO;
			WritePci16(CHX002_UHC1|0x20, buffer16);
			break;


		case CHX002_DUMP_PMIO:
			buffer16 = CHX002_DUMP_PMIO;
			WritePci16(CHX002_BUSC|0x88, buffer16);
			buffer8 = ReadPci8(CHX002_BUSC| 0x81);
			buffer8 |= BIT7;
			WritePci8(CHX002_BUSC| 0xD2, buffer8);		
			break;			

		case CHX002_DUMP_SMIO:
			buffer16 = CHX002_DUMP_SMIO;
			WritePci16(CHX002_BUSC|0xD0, buffer16);
			buffer8 = 0x01;
			WritePci8(CHX002_BUSC| 0xD2, buffer8);		
			break;				

/*		case CHX002_DUMP_D15F0_PRIMARY_CHANNEL_COMMAND_IO:
			RwPci8(CHX002_SATA|D15F0_MISC_CTL_2, BIT7,0);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x01);
			WritePci32(CHX002_SATA|0x10, CHX002_DUMP_D15F0_PRIMARY_CHANNEL_COMMAND_IO);
			break;	

		case CHX002_DUMP_D15F0_PRIMARY_CHANNEL_CONTROL_IO:
			RwPci8(CHX002_SATA|D15F0_MISC_CTL_2, BIT7,0);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x01);
			WritePci32(CHX002_SATA|0x14, CHX002_DUMP_D15F0_PRIMARY_CHANNEL_CONTROL_IO);
			break;

		case CHX002_DUMP_D15F0_BUS_MASTER_IDE_IO:
			RwPci8(CHX002_SATA|D15F0_MISC_CTL_2, BIT7,0);
			WritePci8(CHX002_SATA|D15F0_SUB_CLASS_CODE, 0x01);
			WritePci32(CHX002_SATA|0x20, CHX002_DUMP_D15F0_BUS_MASTER_IDE_IO);
			break;	
	*/	default :
			break;
	}
}
void SB_afterDump(UINT32 baseAddress){
	UINT32 buffer32;
	switch (baseAddress) {
		case CHX002_DUMP_D10F1:
			RwPci8(CHX002_BUSC|0x48, 0x00, 0x02);	//;Set bit, clear bit
			RwPci8(CHX002_BUSC|0xB4, 0x00, 0x80);	//;Set bit, clear bit	
			break;
		case CHX002_DUMP_D10F1_MMIO:
			RwPci8(CHX002_UART1|0x4, 0x00, 0x03);	//;Set bit, clear bit			
			buffer32=0;
			WritePci32(CHX002_UART1|0x10, buffer32);
			break;
		case CHX002_DUMP_D20F0:
			RwPci8(CHX002_PCCA|0xD1, 0x04, 0x04);	//;Set bit, clear bit
			break;
		case CHX002_DUMP_D20F0_MMIO:
			RwPci8(CHX002_HDAC|0x4, 0x00, 0x03);	//;Set bit, clear bit			
			buffer32=0;
			WritePci32(CHX002_HDAC|0x10, buffer32);
			break;
	default :
			break;
	}	
}

void NB_beforeDump(UINT32 baseAddress) {
	UINT32 Buffer32;
	UINT8 Buffer8;
	switch (baseAddress) {
		case CHX002_DUMP_D3F0:
		case CHX002_DUMP_D3F1:
		case CHX002_DUMP_D3F2:
		case CHX002_DUMP_D4F0:
		case CHX002_DUMP_D4F1:
		case CHX002_DUMP_D5F0:
		case CHX002_DUMP_D5F1:
			RwPci8(CHX002_D0F0|0xF7,BIT7,0);
			MicroSecondDelay(1000);
			break;
			
		case CHX002_DUMP_D8F0_MMIO:
			RwPci8(CHX002_RAIIDA_08|0x04, 0x00, 0xFF);	//;Set bit, clear bit
			Buffer32=CHX002_DUMP_D8F0_MMIO;
			WritePci32(CHX002_RAIIDA_08|0x10, Buffer32);
			RwPci8(CHX002_RAIIDA_08|0x04, 0x07, 0);	//;Set bit, clear bit
			break;
		case CHX002_DUMP_D9F0_MMIO:
			RwPci8(CHX002_RAIIDA_09|0x04, 0x00, 0xFF);	//;Set bit, clear bit		
			Buffer32=CHX002_DUMP_D9F0_MMIO;
			WritePci32(CHX002_RAIIDA_09|0x10, Buffer32);
			RwPci8(CHX002_RAIIDA_09|0x04, 0x07, 0);	//;Set bit, clear bit
			break;			
		case CHX002_DUMP_PCIE_EPHY:
			MmioWrite32(0xE0005138, CHX002_DUMP_PCIE_EPHY>>8);
			//DEBUG((EFI_D_ERROR, "DUMP_PCIE_EPHY(Rx138=0x%08x)\n",MmioRead32(0xE0005138)));  
			break;
		case CHX002_DUMP_RCRB_H:
			MmioWrite32(0xE0005268, CHX002_DUMP_RCRB_H>>12);
			//DEBUG((EFI_D_ERROR, "DUMP_RCRB_H(Rx268=0x%08x)\n",MmioRead32(0xE0005268)));  
			break;
		case CHX002_DUMP_TACTL: 		
			MmioWrite32(0xE000526C, CHX002_DUMP_TACTL>>12);
			//DEBUG((EFI_D_ERROR, "DUMP_TACTL(Rx26C=0x%08x)\n",MmioRead32(0xE000526C)));  
			break;		
		case CHX002_DUMP_CRMCA:
			MmioWrite32(0xE000514C, CHX002_DUMP_CRMCA>>12);
			//DEBUG((EFI_D_ERROR, "DUMP_CRMCA(Rx14C=0x%08x)\n",MmioRead32(0xE000514C)));  
			Buffer8 =MmioRead8(0xE000514F);
			Buffer8 = Buffer8|0x30;
			MmioWrite8(0xE000514F, Buffer8);
			//DEBUG((EFI_D_ERROR, "DUMP_CRMCA(Rx14F=0x%08x)\n",MmioRead8(0xE000514F)));  
			break;	
		default:
			break;
		}


}

void NB_afterDump(UINT32 baseAddress) {
	switch (baseAddress) {
		case CHX002_DUMP_D8F0_MMIO:
			RwPci8(CHX002_RAIIDA_08|0x04, 0x00, 0xFF);	//;Set bit, clear bit
			break;		
		case CHX002_DUMP_D9F0_MMIO:
			RwPci8(CHX002_RAIIDA_09|0x04, 0x00, 0xFF);	//;Set bit, clear bit
			break;
		default:
			break;
		}
}

VOID PrintRegArray(
	UINTN Base,
	UINTN LowBound,
	UINTN HighBound,
	BOOLEAN IsBit_Layout,
	BOOLEAN RegType
  )
{
    UINTN  Index;
	UINT8 data=0;
	UINT32 data_temp=0;
	UINT32 Size;
	UINT32 TmpBase;
	UINT16 IdxOffset;
	UINT32 USB3_Lev2_Off;
	
	Size=(UINT32)(HighBound-LowBound);
	TmpBase=Base+LowBound;
		
    //DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, TmpBase, Size));  

    for(Index=0; Index<=Size; Index++)
	{
		if(RegType==TYPE_MEM){
			data = MmioRead8(TmpBase+Index);
		}
		else if(RegType==TYPE_IO){
			if(((TmpBase+Index)<0x818)&&((TmpBase+Index)>0x813))
				data=0x0E;//Error
			else
				data = IoRead8((TmpBase+Index));
		}
		
		else if(RegType==TYPE_IDX){
			IdxOffset=(UINT16)((TmpBase+Index)-((TmpBase+Index)%4));
			WritePci16(VISA_PCI_ADDRESS(0, 15, 0, D15F0_SATA_EPHY_CTL_REG_PTR),IdxOffset);
			data = ReadPci8(VISA_PCI_ADDRESS(0, 15, 0, D15F0_SATA_EPHY_CTL_REG_DATA_0+((TmpBase+Index)%4)));
		}
		
		else if(RegType==TYPE_USB2_D14F7_L2){
			//RwPci8(VISA_PCI_ADDRESS(0, 14, 7, 0x43),BIT0,0);
			USB3_Lev2_Off=(TmpBase+Index)-((TmpBase+Index)%4);
			WritePci32(VISA_PCI_ADDRESS(0, 14, 7, 0x78),USB3_Lev2_Off);
			data = ReadPci8(VISA_PCI_ADDRESS(0, 14, 7, 0x7C+((TmpBase+Index)%4)));
			//RwPci8(VISA_PCI_ADDRESS(0, 14, 7, 0x43),0,BIT0);
			}
		
		else if(RegType==TYPE_USB2_D16F7_L2){
			//RwPci8(VISA_PCI_ADDRESS(0, 16, 7, 0x43),BIT0,0);
			USB3_Lev2_Off=(TmpBase+Index)-((TmpBase+Index)%4);
			WritePci32(VISA_PCI_ADDRESS(0, 16, 7, 0x78),USB3_Lev2_Off);
			data = ReadPci8(VISA_PCI_ADDRESS(0, 16, 7, 0x7C+((TmpBase+Index)%4)));
			//RwPci8(VISA_PCI_ADDRESS(0, 16, 7, 0x43),0,BIT0);
			}
		
		else if(RegType==TYPE_USB3_D18F0_L2){
			RwPci8(VISA_PCI_ADDRESS(0, 18, 0, 0x43),BIT0,0);
			USB3_Lev2_Off=(TmpBase+Index)-((TmpBase+Index)%4);
			WritePci32(VISA_PCI_ADDRESS(0, 18, 0, 0x78),USB3_Lev2_Off);
			data = ReadPci8(VISA_PCI_ADDRESS(0, 18, 0, 0x7C+((TmpBase+Index)%4)));
			RwPci8(VISA_PCI_ADDRESS(0, 18, 0, 0x43),0,BIT0);
			}
		if(Index%16==0) 
			DEBUG((EFI_D_ERROR,  "\n%03x:", Index));
		if(IsBit_Layout){
			Print_Binary(data);
			}
		else{
			
			data_temp=data_temp+(data<<(8*(Index%4)));
			
			if(Index%4==3)
				{
				DEBUG((EFI_D_ERROR,  "%08x", data_temp));
				data_temp=0;
				}
			if(((Index+1)%4) == 0)
				DEBUG((EFI_D_ERROR,  " "));
	    	}
		}
    DEBUG((EFI_D_ERROR, "\n"));
}

EFI_STATUS
EFIAPI
DumpPciDevSetting(
 	IN dumpRegister *RegToDump,
 	IN BOOLEAN IsBit_Layout
 	)
{
	UINT32 PciDevBase;
	UINT32 HighBound,LowBound;
	UINT32 SizeToDump;
    UINT8  Bus,Dev,Func;

	if(RegToDump==NULL){
		return EFI_INVALID_PARAMETER;
	}
	
	PciDevBase = RegToDump->baseAddress;
	LowBound   = RegToDump->lowerBound;
	HighBound  = RegToDump->highBound;
	SizeToDump = HighBound-LowBound;
	
 	 //PCI device not Present
	if(MmioRead16(PciDevBase)==0xFFFF){ 
		DEBUG((EFI_D_ERROR,"\n%s not Present\n",RegToDump->label));
		return EFI_UNSUPPORTED;	
	}
	
	Bus=(UINT8)(PciDevBase>>20);
	Dev=((UINT8)(PciDevBase>>15))&0x1F;
	Func=((UINT8)(PciDevBase>>12))&0x07;
	
	DEBUG((EFI_D_ERROR,"\n# PCI: %s",RegToDump->label));
	PrintRegArray(PciDevBase,LowBound,HighBound,IsBit_Layout,TYPE_MEM);
	
    return EFI_SUCCESS;
}
EFI_STATUS
EFIAPI
DumpMmioReg(
	IN dumpRegister *RegToDump,
	IN BOOLEAN IsBit_Layout
	)
{
	UINT32 MmioBase;
	UINT32 HighBound,LowBound;
    UINT32 SizeToDump;
	
//	DEBUG((EFI_D_ERROR,"HXZ debug. DumpMmioReg\n"));
	if(RegToDump==NULL){
		return EFI_INVALID_PARAMETER;
	}
	LowBound   = RegToDump->lowerBound;
	HighBound  = RegToDump->highBound;
	MmioBase   = RegToDump->baseAddress+LowBound;
	SizeToDump = HighBound-LowBound;
	
	if( MmioBase==0xFFFFFFFF){ 
		DEBUG((EFI_D_ERROR,"\n %s  not Present\n",RegToDump->label));
		return EFI_UNSUPPORTED;	
	}
    
	DEBUG((EFI_D_ERROR,  "\n# MMIO: %s", RegToDump->label));
	PrintRegArray(MmioBase,LowBound,HighBound,IsBit_Layout,TYPE_MEM);
	
    return EFI_SUCCESS;
}
EFI_STATUS
EFIAPI
DumpIoReg(
	IN dumpRegister *RegToDump,
	IN BOOLEAN IsBit_Layout
	)
{
	UINT16 IoBase;
	UINT16 HighBound,LowBound;
    UINT16 SizeToDump;
	
	if(RegToDump==NULL){
			return EFI_INVALID_PARAMETER;
		}
	
	LowBound   = (UINT16)RegToDump->lowerBound;
	HighBound  = (UINT16)RegToDump->highBound;
	IoBase     = (UINT16)RegToDump->baseAddress+LowBound;
	SizeToDump = (UINT16)(HighBound-LowBound);
	
	if( IoBase==0xFFFF){
		return EFI_UNSUPPORTED;	
	}
	if(IoBase+SizeToDump>=0xFFFF){
		DEBUG((EFI_D_ERROR,"border-crossing in IO space\n"));
		return EFI_ABORTED;
	}
	
	DEBUG((EFI_D_ERROR,  "\n# IO: %s", RegToDump->label));
	PrintRegArray(IoBase,LowBound,HighBound,IsBit_Layout,TYPE_IO);

    return EFI_SUCCESS;
}
EFI_STATUS
EFIAPI
DumpIDX_USB_Reg(
	IN dumpRegister *RegToDump,
	IN BOOLEAN IsBit_Layout////true:bit; false:byte
	)
{
	UINT32 Base;
	UINT32 HighBound,LowBound;
    UINT32 SizeToDump;
	
	if(RegToDump==NULL){
		return EFI_INVALID_PARAMETER;
	}
	
	LowBound   = RegToDump->lowerBound;
	HighBound  = RegToDump->highBound;
	SizeToDump = HighBound-LowBound;
	Base=0;

	DEBUG((EFI_D_ERROR,  "\n# %s", RegToDump->label));

    if(RegToDump->type==DUMP_IDX)
		PrintRegArray(Base,LowBound,HighBound,IsBit_Layout,TYPE_IDX);
	else if(RegToDump->type==DUMP_USB2_D14F7_L2)
		PrintRegArray(Base,LowBound,HighBound,IsBit_Layout,TYPE_USB2_D14F7_L2);
	else if(RegToDump->type==DUMP_USB2_D16F7_L2)
		PrintRegArray(Base,LowBound,HighBound,IsBit_Layout,TYPE_USB2_D16F7_L2);
	else if(RegToDump->type==DUMP_USB3_D18F0_L2)
		PrintRegArray(Base,LowBound,HighBound,IsBit_Layout,TYPE_USB3_D18F0_L2);
	
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DumpReg(
	IN	 dumpRegister *dumpRegisters,
	IN   UINT8	       Index,
	IN   BOOLEAN       IsBit_Layout
	)
{
  UINT32  TypeToDump;

  if(dumpRegisters==NULL)
  {
  	DEBUG((EFI_D_ERROR,"invalid reg list\n"));
  	return EFI_INVALID_PARAMETER;
  }
  
  TypeToDump=dumpRegisters[Index].type;

		
		
  switch (TypeToDump){
		case DUMP_CFG:
			DumpPciDevSetting(dumpRegisters+Index,IsBit_Layout);
		    break;
		case DUMP_IO:
			DumpIoReg(dumpRegisters+Index,IsBit_Layout);
			break;
		case DUMP_MMIO:
			DumpMmioReg(dumpRegisters+Index,IsBit_Layout);
			break;		
		case DUMP_IDX:
		case DUMP_USB2_D14F7_L2:
		case DUMP_USB2_D16F7_L2:
		case DUMP_USB3_D18F0_L2:
			DumpIDX_USB_Reg(dumpRegisters+Index,IsBit_Layout);
			break;
		}
  
  return EFI_SUCCESS;
}
VOID DumpNBValue (
  IN     PEI_DUMP_REG_PPI  *This,
  IN     BOOLEAN           DefaultValue,
  IN 	 BOOLEAN			IsBit_Layout
  )
{
	UINT32	Buffer32;
	
	UINT8  buffer1=0,buffer2=0;
	UINT8 CountToDump;
	UINT8 Index;
	
	CountToDump=sizeof(NBRegList)/sizeof(dumpRegister);
    //DEBUG((EFI_D_ERROR,"DLA:dumpNBDefaultValue():CountToDump(%d)\n",CountToDump));
	

   Buffer32 = ReadPci32(CHX002_BUSC|D17F0_MMIO_SPACE_BASE_ADR)&D17F0_F0MMIOBA_31_12;
  // DEBUG((EFI_D_ERROR, "DLA:MMIO BA= %x\n",Buffer32));    

//Init MMIO +Start
	
	if(DefaultValue){
		buffer1 = ReadPci8(CHX002_APIC|0x261);
		WritePci8(CHX002_APIC|0x261, 0x0E);
		buffer2 = ReadPci8(CHX002_APIC|0x260);
		WritePci8(CHX002_APIC|0x260, buffer2|0x03);
	}
	
	for(Index=0;Index<=CountToDump;Index++){

		if (((NBRegList[Index].baseAddress==CHX002_DUMP_D8F0_MMIO)&&(ReadPci8(CHX002_RAIIDA_08)==0xFF))\
		||	((NBRegList[Index].baseAddress==CHX002_DUMP_D9F0_MMIO)&&(ReadPci8(CHX002_RAIIDA_09)==0xFF)))
				continue;

		if(DefaultValue){
			NB_beforeDump(NBRegList[Index].baseAddress);
			}

		DumpReg(NBRegList,Index,IsBit_Layout);


		if(DefaultValue){
			NB_afterDump(NBRegList[Index].baseAddress);
			}
		}; 	
	if(DefaultValue){
		WritePci8(CHX002_APIC|0x261, buffer1);
		WritePci8(CHX002_APIC|0x260, buffer2);
	}	

}
VOID DumpSBValue (
  IN     PEI_DUMP_REG_PPI  *This,
  IN     BOOLEAN           DefaultValue ,  
  IN BOOLEAN			  IsBit_Layout
  )
 {
    UINT8 CountToDump;
	UINT8 Index;
	UINT8 buffer1=0, buffer2=0, buffer3=0;
	
 	CountToDump=sizeof(SBRegList)/sizeof(dumpRegister);
	
	if(DefaultValue){
		buffer1 = ReadPci8(CHX002_APIC|0x261);
		WritePci8(CHX002_APIC|0x261, 0x0E);
		buffer2 = ReadPci8(CHX002_APIC|0x260);
		WritePci8(CHX002_APIC|0x260, buffer2|0x03);
		buffer3 = ReadPci8(CHX002_PCCA|0x4F);
		WritePci8(CHX002_PCCA|0x4F, buffer3|0x02);
	}
	for (Index= 0; Index < CountToDump; Index++){
		
		if ((SBRegList[Index].baseAddress==CHX002_DUMP_D11F0_MMIO)&&(ReadPci8(CHX002_ESPI)==0xFF))
			continue;

		if(DefaultValue){
			SB_beforeDump(SBRegList[Index].baseAddress);
		}
		
			DumpReg(SBRegList,Index,IsBit_Layout);
				
		if(DefaultValue){
			SB_afterDump(SBRegList[Index].baseAddress);
		}
	}
	
	if(DefaultValue){
		WritePci8(CHX002_APIC|0x261, buffer1);
		WritePci8(CHX002_APIC|0x260, buffer2);
	}
}

VOID DumpS3Reg(
  IN     PEI_DUMP_REG_PPI  *This,  
  IN BOOLEAN			  IsBit_Layout
  )
{
	UINT8 CountToDumpSB;
	UINT8 CountToDumpNB;
 	CountToDumpSB=sizeof(SBRegList)/sizeof(dumpRegister);
    CountToDumpNB=sizeof(NBRegList)/sizeof(dumpRegister);
    DEBUG((EFI_D_ERROR,"HXZ debug. dump S3 Reg \n"));
  	DEBUG((EFI_D_ERROR,"HXZ debug. DumpReg-CountToDump(%d)\n",CountToDumpSB));
 	//DumpReg(SBRegList,CountToDumpSB,IsBit_Layout);
	
  	DEBUG((EFI_D_ERROR,"HXZ debug. DumpReg-CountToDump(%d)\n",CountToDumpNB));
 	//DumpReg(NBRegList,CountToDumpNB,IsBit_Layout);
	ZX_DumpPciDevSetting();
}

