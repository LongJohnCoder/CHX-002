/** @file

  Copyright (c) ,  Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <Protocol/SmmBase2.h>
#include <Protocol/SmmControl2.h>
#include <Protocol\BootScriptSave.h>


#include "McaSmi.h"
#include "..\..\Asia\Porting\Include\CHX002Reg.h"
#include "..\..\Asia\Porting\Include\CHX002\REG_CHX002_D17F0_PMU.h" 
#include "..\..\Asia\Interface\CHX002Cfg.h"


#define ReadMsr(addr)  AsmReadMsr64((addr))
#define WriteMsr(addr, val)  AsmWriteMsr64((addr), (val))

EFI_SMM_VARIABLE_PROTOCOL       *mSmmVariable;


EFI_SMM_BASE2_PROTOCOL  *gSmmBase2;
EFI_SMM_SYSTEM_TABLE2   *gSmst2;


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
    UINT8   Buffer8;
    Buffer8 = PCIRead8(PciBusDevFunReg) & (~ResetBit8) | SetBit8;       //Reg -> NAND(Resetbit8) -> OR(SetBit8)
    PCIWrite8(PciBusDevFunReg, Buffer8);
    return Buffer8;
}

VOID
ModifyMsr (
  IN  UINT32      Input,
  IN  UINT64      Mask,
  IN  UINT64      Value
  )
{
    WriteMsr(Input, ReadMsr(Input) & ~Mask | Value & Mask );
}

// ===================================================================
// Function:        ProgramCOMDBG()
// 
// Description: This function is used to program COM port for Debugger
// 
// Input:       PeiServices Pointer to the PEI services table
// 
// Output:      None
// ===================================================================

VOID ProgramCOMDBG()
{
    UINT8 Buffer8;
    UINT32 Buffer32;

	 //Enable  4Pin UART1
	 //Buffer32=IoRead32(0x800 + PMIO_CR_GPIO_PAD_CTL);
	 //Buffer32&=~(0x3F000000);
	 //Buffer32|=0x13000000;
	 //IoWrite32(0x800 + PMIO_CR_GPIO_PAD_CTL,  Buffer32);

	 Buffer32=IoRead32(0x800 + PMIO_GPIO_PAD_CTL);
	 Buffer32&=~(0x3F3F0007);
	 Buffer32|=0x242D0000;
	 IoWrite32(0x800 + PMIO_GPIO_PAD_CTL,  Buffer32);

    // set D17F0RxB2[3:0]=2h for UART1 IRQ=IRQ3
    RwPci8(CHX002_BUSC|D17F0_PCI_UART_IRQ_ROUTING_LOW, 0x30, 0xF0); //;Set bit, clear bit
    // set D17F0RxB3=7Fh to set IO Base Address configuration of UART1
    RwPci8(CHX002_BUSC|D17F0_PCI_UART_1_IO_BASE_ADR, 0xDF, 0x7F);   //;Set bit, clear bit
    // set D17F0RxB3[7]=1 to UART0 to legacy mode
    RwPci8(CHX002_BUSC|D17F0_PCI_UART_1_IO_BASE_ADR, 0x80, 0x80);   //;Set bit, clear bit
    // D17F0 Rx48[1]=1b to enable UART1
    RwPci8(CHX002_BUSC|D17F0_APIC_FSB_DATA_CTL, 0x02, 0x02);    //;Set bit, clear bit

    //;======================
    //; set Word length=8
    //;======================
    Buffer8 = IoRead8(0x2FB);
    Buffer8 = Buffer8 |0x03;
    IoWrite8(0x2FB,Buffer8);
    //;======================
    //; set Baud rate = 115200
    //;======================
    //; Enable DLAB
    Buffer8 = IoRead8(0x2FB);
    Buffer8 = Buffer8 |0x80;
    IoWrite8(0x2FB,Buffer8);
    //; Set Baud rate (LSB)
    IoWrite8(0x2F8,0x01);
    //; Set Baud rate (MSB)
    IoWrite8(0x2F9,0x00);
    //; Disable DLAB
    Buffer8 = IoRead8(0x2FB);
    Buffer8 = Buffer8 & 0x7F;
    IoWrite8(0x2FB,Buffer8);
    //;=====================
    //; init UART0
    //;=====================
    IoWrite8(0x2FA,0x00);
    IoWrite8(0x2FA,0x02);   //; clear RCV FIFO
    IoWrite8(0x2FA,0x04);   //; clear XMT FIFO
    IoWrite8(0x2FA,0x01);   //; enable RCV FIFO & XMT FIFO
    IoWrite8(0x2FA,0xC1);   //; RCV trigger level = 14 Bytes
}

// ===================================================================
// Function:        ProgramCOM4DBG()
// 
// Description: This function is used to program COM3 port for Debugger
// 
// Input:       PeiServices Pointer to the PEI services table
// 
// Output:      None
// ===================================================================
VOID ProgramCOM4DBG()
{
    UINT8 Buffer8;
    UINT32 Buffer32;

  Buffer32=IoRead32(PM_BASE_ADDRESS + PMIO_UART_PAD_CTL); 
    Buffer32&=~(0x004700FF);
    Buffer32|=0x00010000;

    IoWrite32(PM_BASE_ADDRESS + PMIO_UART_PAD_CTL, Buffer32);

    // set D17F0Rx48[3]=1 to enable UART3
    RwPci8(CHX002_BUSC|D17F0_APIC_FSB_DATA_CTL, 0x08, 0x08);    //;Set bit, clear bit
    // set D17F0RxB1[7:4]=3h for UART3 IRQ=IRQ3
    ///RwPci8(CHX002_BUSC|D17F0_PCI_UART_IRQ_ROUTING_HIGH, 0x30, 0xF0); //;Set bit, clear bit
    // set D17F0RxB6=5Dh to set IO Base Address configuration of UART3
    ///RwPci8(CHX002_BUSC|D17F0_PCI_UART_3_IO_BASE_ADR, 0x5D, 0x7F);    //;Set bit, clear bit
    // set D17F0RxB6[7]=1 to UART3 to legacy mode
    ///RwPci8(CHX002_BUSC|D17F0_PCI_UART_3_IO_BASE_ADR, 0x80, 0x80);    //;Set bit, clear bit
    // D17F0 Rx48[3]=1b to enable UART3
    RwPci8(CHX002_BUSC|D17F0_APIC_FSB_DATA_CTL, 0x08, 0x08);    //;Set bit, clear bit

    //;======================
    //; set Word length=8
    //;======================
    Buffer8 = IoRead8(0x2EB);
    Buffer8 = Buffer8 |0x03;
    IoWrite8(0x2EB,Buffer8);
    //;======================
    //; set Baud rate = 115200
    //;======================
    //; Enable DLAB
    Buffer8 = IoRead8(0x2EB);
    Buffer8 = Buffer8 |0x80;
    IoWrite8(0x2EB,Buffer8);
    //; Set Baud rate (LSB)
    IoWrite8(0x2E8,0x01);
    //; Set Baud rate (MSB)
    IoWrite8(0x2E9,0x00);
    //; Disable DLAB
    Buffer8 = IoRead8(0x2EB);
    Buffer8 = Buffer8 & 0x7F;
    IoWrite8(0x2EB,Buffer8);
    //;=====================
    //; init UART0
    //;=====================
    IoWrite8(0x2EA,0x00);
    IoWrite8(0x2EA,0x02);   //; clear RCV FIFO
    IoWrite8(0x2EA,0x04);   //; clear XMT FIFO
    IoWrite8(0x2EA,0x01);   //; enable RCV FIFO & XMT FIFO
    IoWrite8(0x2EA,0xC1);   //; RCV trigger level = 14 Bytes
}

void InitUart()
{
        // Program COM4 port for Debugger or COM port display
        //ProgramCOM4DBG();

        // Program COM port for Debugger
        ProgramCOMDBG();
}


UINT8   _gMca_Msmi_En;
UINT8   _gMca_Csmi_En;

//============================================
//   MCA related Code
//
//============================================

enum BANK_ID_LIST {
  BANK_0 = 0,
  BANK_6 = 6,
  BANK_8 = 8,
  BANK_9 = 9,
  BANK_10 = 10,
  BANK_17 = 17,
};


void BankHandler(UINT8 BankId)
{
        DEBUG(( EFI_D_ERROR, " ***** BankID %X ****************************\n", BankId));
        DEBUG(( EFI_D_ERROR, " CTL    : %llX\n", ReadMsr(MSR_IA32_MCx_CTL(BankId))  ));
        DEBUG(( EFI_D_ERROR, " Status : %llX\n", ReadMsr(MSR_IA32_MCx_STATUS(BankId))));
        DEBUG(( EFI_D_ERROR, " Addr   : %llX\n", ReadMsr(MSR_IA32_MCx_ADDR(BankId))  ));
        DEBUG(( EFI_D_ERROR, " Misc   : %llX\n", ReadMsr(MSR_IA32_MCx_MISC(BankId))  ));
        DEBUG(( EFI_D_ERROR, " CTL2   : %llX\n", ReadMsr(MSR_IA32_MCx_CTL2(BankId))  ));

        WriteMsr(MSR_IA32_MCx_STATUS(BankId), 0);
        WriteMsr(MSR_IA32_MCx_ADDR(BankId), 0);
        WriteMsr(MSR_IA32_MCx_MISC(BankId), 0);
        //WriteMsr(MSR_IA32_MCx_CTL(BankId), 0);
}


EFI_STATUS
EFIAPI
McaHandler(
  IN EFI_HANDLE             SmmImageHandle,
  IN     CONST VOID         *ContextData,            OPTIONAL
  IN OUT VOID               *CommunicationBuffer,    OPTIONAL
  IN OUT UINTN              *SourceSize              OPTIONAL
  )
{
    UINT8  McaSmiAssert = FALSE;
    UINT64 Val64;
    UINT8  BankId, Idx;
    UINT8  SupportMsmiBankIdList[] = {BANK_0, BANK_6, BANK_8, BANK_9, BANK_10, BANK_17};
    UINT8  SupportCsmiBankIdList[] = {BANK_9, BANK_10, BANK_17};

   //
   //  Enable bit & Status bit for MCA Smi should be 1
   //
   if ((IoRead8( PM_BASE_ADDRESS+ PMIO_GENERAL_PURPOSE_STA_Z1 ) & PMIO_CPUMCA_STS ) == 0 ||
        (IoRead8( PM_BASE_ADDRESS+ PMIO_GENERAL_PURPOSE_SMI_ENABLE) & PMIO_CPUMCA_SM ) == 0) {//0419
       return EFI_SUCCESS;
   }

#if ERSMI_COMPORT_DEBUG_MESSAGES
    InitUart();
#endif

    DEBUG((EFI_D_ERROR, "bf MSR_IA32_MCG_STATUS = 0x%X\n",ReadMsr(MSR_IA32_MCG_STATUS)));
    WriteMsr(MSR_IA32_MCG_STATUS,0);
    DEBUG((EFI_D_ERROR, "af MSR_IA32_MCG_STATUS = 0x%X\n",ReadMsr(MSR_IA32_MCG_STATUS)));

    do
    {
        McaSmiAssert = FALSE;

        //
        // Handle MSMI
        //
        for (Idx=0; Idx < sizeof(SupportMsmiBankIdList)/sizeof(UINT8); Idx++)
        {
            if (_gMca_Msmi_En == 0)
                break;

            BankId = SupportMsmiBankIdList[Idx];

            Val64 = ReadMsr(MSR_IA32_MCx_STATUS(BankId));

            if ( (Val64 & MCI_STS_VAL_BIT) != MCI_STS_VAL_BIT)
                continue; // Not valid error

            if ( (Val64 & MCI_STS_UC_BIT) != MCI_STS_UC_BIT)
                continue; // not MSMI (Uncorrected Error)
                
            DEBUG(( EFI_D_ERROR, " Handle MSMI \n" ));
            
            BankHandler(BankId);

            McaSmiAssert = TRUE;
        }

        //
        // Handle CSMI
        //

        for (Idx=0; Idx < sizeof(SupportCsmiBankIdList)/sizeof(UINT8); Idx++)
        {

            if (_gMca_Csmi_En == 0)
                break;

            BankId = SupportCsmiBankIdList[Idx];

            Val64 = ReadMsr(MSR_IA32_MCx_STATUS(BankId));

            if ( (Val64 & MCI_STS_VAL_BIT) != MCI_STS_VAL_BIT)
                continue; // Not valid error

            if ( (Val64 & MCI_STS_UC_BIT) != 0)
                continue; // not CSMI (Corrected Error)
            
            DEBUG(( EFI_D_ERROR, " Handle CSMI \n" ));
            
            BankHandler(BankId);

            McaSmiAssert = TRUE;
        }
    } while (McaSmiAssert == TRUE);
    
    DEBUG(( EFI_D_ERROR, " Clear PMIO_GENERAL_PURPOSE_STA_Z1 \n" ));
    // Clear Mca Smi status
    IoWrite8( PM_BASE_ADDRESS+PMIO_GENERAL_PURPOSE_STA_Z1, IoRead8( PM_BASE_ADDRESS+PMIO_GENERAL_PURPOSE_STA_Z1) | (UINT8)PMIO_CPUMCA_STS );

    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
McaSmiInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS         Status;
    EFI_HANDLE         Handle = NULL;
    SETUP_DATA         SetupData;
    UINTN              SetupDataSize;

    DEBUG((EFI_D_ERROR,"McaSmiInit()\n"));
    
    //// 2016041301-TGR patched for EMU Variable.
    if(PcdGetBool(PcdVarServiceUseEmu)){
       DEBUG((EFI_D_INFO, "(Smm)UseEmuVarService, Not do McaSmiInit()\n"));    
       return EFI_SUCCESS;
    }

    //
    // Enable MCE and Inject Thermal error if Thermal status in PMIO is High
    //
    if (IoRead16( PM_BASE_ADDRESS + PMIO_GENERAL_PURPOSE_STA) & PMIO_THRM_STS )  { //Clear Thermal status in PMIO if Thermal status High
        
        IoWrite16( PM_BASE_ADDRESS + PMIO_GENERAL_PURPOSE_STA, PMIO_THRM_STS );

        //EnableMachineCheck();  // Enable Cr4.Mce
        
        ModifyMsr(BJ_MCA_ZX_CTRL2_MSR, 1, 1);

        ModifyMsr(MSR_IA32_MCx_STATUS(0), 0xF0000000FFFFFFFF, 0xB000000041000001); //Set Thermal Type error source
        ModifyMsr(MSR_IA32_MCx_STATUS(0), 0x8000000000000000, 0x8000000000000000); //Inject Thermal error

        ModifyMsr(BJ_MCA_ZX_CTRL2_MSR, 1, 0);
        //DisableMachineCheck(); // Disable Cr4.Mce

    } 
    
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
    
    _gMca_Msmi_En = SetupData.Mca_Msmi_En;
    _gMca_Csmi_En = SetupData.Mca_Csmi_En;


    if ( (_gMca_Msmi_En || _gMca_Csmi_En) == 0) // No need to install MCA SMI Handler
        return EFI_SUCCESS;
        
    //
    // Enable Smi On Cpu/Mca Smi Assertion
    //
    IoWrite8( PM_BASE_ADDRESS+PMIO_GENERAL_PURPOSE_SMI_ENABLE,  IoRead8( PM_BASE_ADDRESS+PMIO_GENERAL_PURPOSE_SMI_ENABLE) | PMIO_CPUMCA_SM  ); //0419

    //
    // Enable MSMI
    //
    if (_gMca_Msmi_En)
    {
        WriteMsr ( MSR_IA32_MCx_CTL2(0),    ReadMsr(MSR_IA32_MCx_CTL2(0)) | MCI_CTL2_MSMI_EN_BIT  );
        WriteMsr ( MSR_IA32_MCx_CTL2(5),    ReadMsr(MSR_IA32_MCx_CTL2(5)) | MCI_CTL2_MSMI_EN_BIT  );
        WriteMsr ( MSR_IA32_MCx_CTL2(6),    ReadMsr(MSR_IA32_MCx_CTL2(6)) | MCI_CTL2_MSMI_EN_BIT  );
        WriteMsr ( MSR_IA32_MCx_CTL2(8),    ReadMsr(MSR_IA32_MCx_CTL2(8)) | MCI_CTL2_MSMI_EN_BIT  );
        WriteMsr ( MSR_IA32_MCx_CTL2(9),    ReadMsr(MSR_IA32_MCx_CTL2(9)) | MCI_CTL2_MSMI_EN_BIT   );
        WriteMsr ( MSR_IA32_MCx_CTL2(10),   ReadMsr(MSR_IA32_MCx_CTL2(10)) | MCI_CTL2_MSMI_EN_BIT   );
        WriteMsr ( MSR_IA32_MCx_CTL2(17),   ReadMsr(MSR_IA32_MCx_CTL2(17)) | MCI_CTL2_MSMI_EN_BIT   );
    }

    //
    //  Enable CSMI
    //
    if (_gMca_Csmi_En)
    {
        WriteMsr ( MSR_IA32_MCx_CTL2(5),    ReadMsr(MSR_IA32_MCx_CTL2(5)) | MCI_CTL2_CSMI_EN_BIT   );
        WriteMsr ( MSR_IA32_MCx_CTL2(9),    ReadMsr(MSR_IA32_MCx_CTL2(9)) | MCI_CTL2_CSMI_EN_BIT   );
        WriteMsr ( MSR_IA32_MCx_CTL2(10),   ReadMsr(MSR_IA32_MCx_CTL2(10)) | MCI_CTL2_CSMI_EN_BIT  );
        WriteMsr ( MSR_IA32_MCx_CTL2(17),   ReadMsr(MSR_IA32_MCx_CTL2(17)) | MCI_CTL2_CSMI_EN_BIT  );
    }

    Status = gSmst->SmiHandlerRegister (McaHandler, NULL,  &Handle);    
    ASSERT_EFI_ERROR (Status);

    return EFI_SUCCESS;
}
