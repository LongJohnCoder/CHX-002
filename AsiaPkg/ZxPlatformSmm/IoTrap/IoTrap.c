//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************


#include <Protocol/SmmBase2.h>
#include <Protocol/SmmControl2.h>
#include <Protocol\BootScriptSave.h>


#include "IoTrap.h"
#include "..\..\Asia\Porting\Include\CHX002Reg.h"
#include "..\..\Asia\Porting\Include\CHX002\REG_CHX002_D17F0_PMU.h" 
#include "..\..\Asia\Interface\CHX002Cfg.h"

EFI_SMM_VARIABLE_PROTOCOL       *mSmmVariable;


EFI_SMM_BASE2_PROTOCOL  *gSmmBase2;
EFI_SMM_SYSTEM_TABLE2   *gSmst2;


#define PCIRESET  6

//============================================
//    PCI cfg space r/w function
//
//============================================

VOID PCIWrite8(
	UINT64 Address,
	UINT8 Data)
{
	UINT16 Stride;
	UINT32 Cf8Val;
	EFI_PCI_CONFIGURATION_ADDRESS *PciAddr;

	PciAddr = (EFI_PCI_CONFIGURATION_ADDRESS *) (&Address);
	Cf8Val = (PciAddr->Bus << 16) | (PciAddr->Device << 11) | (PciAddr->Function << 8)| PciAddr->Register;

	Stride = ((UINT16)Address & 0x0003);
	Cf8Val &= 0xFFFFFFFC;

	IoWrite32(0xCF8, (Cf8Val|0x80000000));
	IoWrite8(0xCFC + Stride, Data);

}

VOID PCIWrite32(
	UINT64 Address,
	UINT32 Data)
{
	UINT16 Stride;
	UINT32 Cf8Val;
	EFI_PCI_CONFIGURATION_ADDRESS *PciAddr;

	PciAddr = (EFI_PCI_CONFIGURATION_ADDRESS *) (&Address);
	Cf8Val = (PciAddr->Bus << 16) | (PciAddr->Device << 11) | (PciAddr->Function << 8)| PciAddr->Register;

	Stride = ((UINT16)Address & 0x0003);
	Cf8Val &= 0xFFFFFFFC;

	IoWrite32(0xCF8, (Cf8Val|0x80000000));
	IoWrite32(0xCFC + Stride, Data);

}

UINT8 PCIRead8(
	UINT64 Address)
{
	UINT8 Data8;
	UINT16 Stride;
	UINT32 Cf8Val;
	EFI_PCI_CONFIGURATION_ADDRESS *PciAddr;

	PciAddr = (EFI_PCI_CONFIGURATION_ADDRESS *) (&Address);
	Cf8Val = (PciAddr->Bus << 16) | (PciAddr->Device << 11) | (PciAddr->Function << 8)| PciAddr->Register;

	Stride = ((UINT16)Address & 0x0003);
	Cf8Val &= 0xFFFFFFFC;

	IoWrite32(0xCF8,(Cf8Val|0x80000000));
	Data8=IoRead8(0xCFC + Stride);

	return Data8;
}

UINT32 PCIRead32(
	UINT64 Address)
{
	UINT32 Data32;
	UINT16 Stride;
	UINT32 Cf8Val;
	EFI_PCI_CONFIGURATION_ADDRESS *PciAddr;

	PciAddr = (EFI_PCI_CONFIGURATION_ADDRESS *) (&Address);
	Cf8Val = (PciAddr->Bus << 16) | (PciAddr->Device << 11) | (PciAddr->Function << 8)| PciAddr->Register;

	Stride = ((UINT16)Address & 0x0003);
	Cf8Val &= 0xFFFFFFFC;

	IoWrite32(0xCF8,(Cf8Val|0x80000000));
	Data32=IoRead32(0xCFC + Stride);

	return Data32;
}



UINT8 RwPci8 (
    IN UINT64   PciBusDevFunReg,
    IN UINT8    SetBit8,
    IN UINT8    ResetBit8 )
{
	UINT8	Buffer8;
	Buffer8 = PCIRead8(PciBusDevFunReg) & (~ResetBit8) | SetBit8;		//Reg -> NAND(Resetbit8) ->	OR(SetBit8)
	PCIWrite8(PciBusDevFunReg, Buffer8);
	return Buffer8;
}

//LNA-2017061301-S:pcie patch
//
//Trigger PME_Turn_Off Message Sending
//JNY20180115 add for CHX002 signal socket-S 
VOID WaitL2L3Ready(VOID)
{
 	UINT8 Data8;
	UINT32 Data32, PexPresentData32;
	UINT16 I;
 
	Data8 = PCIRead8(CHX002_APIC|0xF0);	//RxF0
	Data8 |= BIT7;
	PCIWrite8((CHX002_APIC|0xF0),Data8);	//RxF0

	PexPresentData32 = 0x00000000;
       if (PCIRead8(CHX002_PE0|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)
		PexPresentData32 |= D0F5_RPMEACK_PE0;
       if (PCIRead8(CHX002_PE1|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)
		PexPresentData32 |= D0F5_RPMEACK_PE1;
       if (PCIRead8(CHX002_PE2|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)
		PexPresentData32 |= D0F5_RPMEACK_PE2;
       if (PCIRead8(CHX002_PE3|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)
		PexPresentData32 |= D0F5_RPMEACK_PE3;
       if (PCIRead8(CHX002_PE4|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)    //for pe4
		PexPresentData32 |= D0F5_RPMEACK_PEG0;
       if (PCIRead8(CHX002_PE5|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)    //for pe5
		PexPresentData32 |= D0F5_RPMEACK_PEG1;
       if (PCIRead8(CHX002_PE6|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)    //for PE6
		PexPresentData32 |= (D0F5_RPMEACK_PEG2 << 8);
       if (PCIRead8(CHX002_PE7|D3D5F1_SLOT_STA_1) && D3D5F1_SPDC)     //for PE7
		PexPresentData32 |= (D0F5_RPMEACK_PEG3 << 8);

	if(PexPresentData32 == 0x00000000)
		return;
	
	for(I=0;I<=0x100;I++){
		Data32=PCIRead32(CHX002_APIC|0xA0);	//RxA0
				
		Data32 &= PexPresentData32;

		if(Data32 == PexPresentData32)
			break;
	}
}


VOID ClearPMEStatus(UINT64 Addr)
{
	UINT8 Data8;
	UINT32 Data32;

	//Data32 = PCIRead32(PCI_LIB_ADDRESS(Bus, Dev, Func, 0));
	Data32 = PCIRead32(Addr);
	if(Data32 == 0xFFFFFFFF) return;
	// Clear twice for RP PME pending status
		Data8 = PCIRead8(Addr|0x62);
		Data8 |= BIT0;
		PCIWrite8(Addr|0x62,Data8);		//Rx62
		IoWrite8(0xED, 0x00);
		PCIWrite8(Addr|0x62,Data8);		//Rx62
}
//LNA-2017061301-E
//JNY20180115 add for CHX002 signal socket-E 

VOID BuildIOTrapSMI()
{
	UINT32 mmiobase;
	UINT8 data;
	mmiobase = (PCIRead32(CHX002_BUSC|0xBC))<<8;
	DEBUG(( EFI_D_ERROR, "  mmio base=0x%x \n",mmiobase));
	//Trap IO Port 0xCF9 Write cycle.
	*(volatile UINT32 *)(UINTN)(mmiobase+0x54)=0xCF928;

	
	//clear status;
	data = IoRead8( PM_BASE_ADDRESS+ 0x20 );
	IoWrite8( PM_BASE_ADDRESS+ 0x20 ,data|0x08);
	//data = IoRead8( PM_BASE_ADDRESS+ 0x2d );
	//IoWrite8( PM_BASE_ADDRESS+ 0x2d ,data|0x01);
	
	DEBUG(( EFI_D_ERROR, "  Enable IO trap SMI \n"));
	data = IoRead8( PM_BASE_ADDRESS+ 0x24 );
	data |= 0x08;
	IoWrite8( PM_BASE_ADDRESS+ 0x24 ,data);
	
	DEBUG(( EFI_D_ERROR, " IO Trap =0x%x \n",*(volatile UINT32 *)(UINTN)(mmiobase+0x54)));
	DEBUG(( EFI_D_ERROR, " 824 =0x%x \n",IoRead8( PM_BASE_ADDRESS+ 0x24 )));

	return;
}


EFI_STATUS
EFIAPI
IoTrapHandler(
  IN EFI_HANDLE             SmmImageHandle,
  IN     CONST VOID         *ContextData,            OPTIONAL
  IN OUT VOID               *CommunicationBuffer,    OPTIONAL
  IN OUT UINTN              *SourceSize              OPTIONAL
  )

{
    UINT8  Idx ;
	
	if(((IoRead8( PM_BASE_ADDRESS+ 0x20 ))&0x08) == 0)
		return EFI_SUCCESS;
	
	if (((IoRead8( PM_BASE_ADDRESS+ 0x24 ))&0x08) == 0)
		return EFI_SUCCESS;

	

	//IoWrite8( 0x84,0xEE );
	DEBUG(( EFI_D_ERROR, " GP SMI Enable 0x824=0x%x \n",IoRead16(0x824)));
	DEBUG(( EFI_D_ERROR, " GP status 0x820=0x%x \n",IoRead16(0x820)));
	//DEBUG(( EFI_D_ERROR, " Global SMI Status/Enable 0x828=0x%x \n",IoRead32(0x828)));
	//DEBUG(( EFI_D_ERROR, " Extend SMI Status/Enable 0x840=0x%x \n",IoRead32(0x840)));
	Idx = IoRead8(0xcf9);
	DEBUG(( EFI_D_ERROR, " There is IO Trap SMI handler \n" ));
	DEBUG(( EFI_D_ERROR, "	0xcf9=0x%x \n",Idx));
	DEBUG(( EFI_D_ERROR, "	0xcf8=0x%x \n",IoRead32(0xcf8)));
		
		//Clear Io trap status
		IoWrite8( PM_BASE_ADDRESS+0x20, (IoRead8( PM_BASE_ADDRESS+0x20)) | 0x08 );
		//DEBUG(( EFI_D_ERROR, "  0x820=0x%x \n",IoRead8( PM_BASE_ADDRESS+0x20)));
		IoWrite8( PM_BASE_ADDRESS+ 0x2d ,(IoRead8( PM_BASE_ADDRESS+ 0x2d ))|0x01);
		//DEBUG(( EFI_D_ERROR, "  0x82d=0x%x \n",IoRead8( PM_BASE_ADDRESS+0x2d)));

	//JNY20180115 add for CHX002 signal socket-S 
	#if 1
	if(Idx == PCIRESET) {
		//LNA-2017061301-S: pcie patch
		WaitL2L3Ready();
		DEBUG(( EFI_D_ERROR, "[QLP_IOTRAP]PCIE WaitL2L3Ready \n"));
		
		ClearPMEStatus(CHX002_PE0); 
		ClearPMEStatus(CHX002_PE1); 
		ClearPMEStatus(CHX002_PE2); 
		ClearPMEStatus(CHX002_PE3); 
		ClearPMEStatus(CHX002_PE4);
		ClearPMEStatus(CHX002_PE5);
		ClearPMEStatus(CHX002_PE6);
		ClearPMEStatus(CHX002_PE7);
		DEBUG((EFI_D_ERROR, "[QLP_IOTRAP]PCIE Clear PME Status\n"));
	}
	#endif
	//JNY20180115 add for CHX002 signal socket-E 

		
		{
			UINT32 data;
			UINT8 data8;
			UINT32 mmiobase;
			mmiobase = (PCIRead32(CHX002_BUSC|0xBC))<<8;
			if((Idx==4)||(Idx==6)||(Idx==0xF)||(Idx==0xE))
				*(volatile UINT32 *)(UINTN)(mmiobase+0x54)=0;
			
			data = *(volatile UINT32 *)(UINTN)(0xE0003124);
			if(data&0x800000){
				DEBUG(( EFI_D_ERROR, " Set 0x3C2 = 0 \n" ));
				data8 = IoRead8(0x3C2);
				data8 &= ~0x04;	
				IoWrite8( 0x3C2, data8 );
				DEBUG(( EFI_D_ERROR, "	IO 0x3C2=0x%x \n",IoRead8(0x3C2)));
			}
		}
		if((Idx==4)||(Idx==6)||(Idx==0xF)||(Idx==0x0E))
			IoWrite8( 0xCF9, Idx );
	
		return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
IoTrapInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
	//return EFI_SUCCESS;//Disable IO trap
#if 1

    EFI_STATUS                              Status;
    EFI_HANDLE                              Handle = NULL;


	SETUP_DATA				SetupData;
    UINTN                  SetupDataSize;

    DEBUG((EFI_D_ERROR,"IoTrapInit()\n")); 
    
     
    Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID**)&mSmmVariable
                    );
    ASSERT_EFI_ERROR (Status);


    SetupDataSize = sizeof(SETUP_DATA); 
    Status = mSmmVariable->SmmGetVariable (
                           PLATFORM_SETUP_VARIABLE_NAME,
                           &gPlatformSetupVariableGuid,
                           NULL,
                           &SetupDataSize,
                           &SetupData
                           );    
	ASSERT_EFI_ERROR (Status);
	
	if(SetupData.IoTrapEn) {	
		BuildIOTrapSMI();

	    Status = gSmst->SmiHandlerRegister (IoTrapHandler, NULL,  &Handle);	
	    ASSERT_EFI_ERROR (Status);
	}
	
 	return EFI_SUCCESS;
  
#endif
}

