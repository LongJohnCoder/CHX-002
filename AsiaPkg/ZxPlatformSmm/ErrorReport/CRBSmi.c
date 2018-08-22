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
    
#if defined(PI_SPECIFICATION_VERSION)&&(PI_SPECIFICATION_VERSION>=0x0001000A)
#include <Protocol\SmmBase2.h>
#include <Protocol\SmmSwDispatch2.h>
#include <Protocol\SmmSxDispatch2.h>
#include <Protocol\SmmPowerButtonDispatch2.h>
#else
#include <Protocol\SmmBase.h>
#include <Protocol\SmmSwDispatch.h>
#include <Protocol\SmmSxDispatch.h>
#include <Protocol\SmmPowerButtonDispatch.h>
#endif

#include "CRBSmi.h"
#include "..\..\Asia\Porting\Include\CHX002Reg.h"
#include "..\..\Asia\Interface\CHX002Cfg.h"



EFI_SMM_VARIABLE_PROTOCOL       *mSmmVariable;

//----------------------------------------------------------------------------
// Constant, Macro and Type Definition(s)
//----------------------------------------------------------------------------
// Constant Definition(s)

#ifndef ACPI_SUPPORT
#define ACPI_SUPPORT                    0
#endif

#ifndef ERSMI_COMPORT_DEBUG_MESSAGES
#define ERSMI_COMPORT_DEBUG_MESSAGES    1
#endif  //;ERSMI_COMPORT_DEBUG_MESSAGES

// Macro Definition(s)

// Type Definition(s)

// Function Prototype(s)

//----------------------------------------------------------------------------
// Variable and External Declaration(s)
//----------------------------------------------------------------------------
// Variable Declaration(s)

EFI_BOOT_SCRIPT_SAVE_PROTOCOL   *gBootScriptSave;
EFI_EVENT                       mAcpiEvent;
VOID                            *mAcpiReg;
UINT8   gACLossAutoRestart;

// Error Reporting Setup-Data Usage
UINT8   gChipsetSERRNBControl;
BOOLEAN gChipsetLoopERSMIControl;
BOOLEAN gChipsetHIFErrControl;
UINT8 gChipsetDRAMErrControl;
//UINT8 gChipsetPEGErrControl;
UINT8 gChipsetPE0ErrControl;
UINT8 gChipsetPE1ErrControl;
UINT8 gChipsetPE2ErrControl;
UINT8 gChipsetPE3ErrControl;
UINT8 gChipsetPE5ErrControl;
UINT8 gChipsetPE4ErrControl;
UINT8 gChipsetPE5ErrControl;
UINT8 gChipsetPE6ErrControl;
UINT8 gChipsetPE7ErrControl;


// Chip Revision
UINT8   ChipRevision;

// GUID Definition(s)
#if defined(PI_SPECIFICATION_VERSION)&&(PI_SPECIFICATION_VERSION>=0x0001000A)
EFI_GUID gEfiSmmBase2ProtocolGuid      = EFI_SMM_BASE2_PROTOCOL_GUID;
EFI_SMM_BASE2_PROTOCOL  *gSmmBase2;
EFI_SMM_SYSTEM_TABLE2   *gSmst2;
EFI_GUID gSwDispatch2ProtocolGuid      = EFI_SMM_SW_DISPATCH2_PROTOCOL_GUID;
EFI_GUID gSxDispatch2ProtocolGuid      = EFI_SMM_SX_DISPATCH2_PROTOCOL_GUID;
EFI_GUID gPowerButtonDispatch2ProtocolGuid      = EFI_SMM_POWER_BUTTON_DISPATCH2_PROTOCOL_GUID;
#else
EFI_GUID gEfiSmmBaseProtocolGuid      = EFI_SMM_BASE_PROTOCOL_GUID;
EFI_GUID gSwDispatchProtocolGuid      = EFI_SMM_SW_DISPATCH_PROTOCOL_GUID;
EFI_GUID gSxDispatchProtocolGuid      = EFI_SMM_SX_DISPATCH_PROTOCOL_GUID;
EFI_GUID gPowerButtonDispatchProtocolGuid      = EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL_GUID;
#endif

//LGE-20151216 component out for compile error
//EFI_GUID gEfiBootScriptSaveGuid       = EFI_BOOT_SCRIPT_SAVE_GUID;
//EFI_GUID gSetupGuid = SETUP_GUID;
//LGE-20151216 component out for compile error





// Protocol Definition(s)

// External Declaration(s)

// Function Definition(s)

/* Memory space access
64bit address, 64bit data access supported.
Memory space access is implemented using flat mode pointers. */
// Memory read
UINT8 CrbMemoryRead8(UINT64 Address)
{ return *((volatile UINT8*)(UINTN)Address); }
UINT16 CrbMemoryRead16(UINT64 Address)
{ return *((volatile UINT16*)(UINTN)Address); }
UINT32 CrbMemoryRead32(UINT64 Address)
{ return *((volatile UINT32*)(UINTN)Address); }
UINT64 CrbMemoryRead64(UINT64 Address)
{ return *((volatile UINT64*)(UINTN)Address); }

// Memory write
VOID CrbMemoryWrite8(UINT64 Address, UINT8 Data)
{ *((volatile UINT8*)(UINTN)Address) = Data; }
VOID CrbMemoryWrite16(UINT64 Address, UINT16 Data)
{ *((volatile UINT16*)(UINTN)Address) = Data; }
VOID CrbMemoryWrite32(UINT64 Address, UINT32 Data)
{ *((volatile UINT32*)(UINTN)Address) = Data; }
VOID CrbMemoryWrite64(UINT64 Address, UINT64 Data)
{ *((volatile UINT64*)(UINTN)Address) = Data; }

// Memory modify
VOID CrbMemoryModify8(UINT64 Address, UINT8 Mask, UINT8 Value)
{
  UINT8 D8 = CrbMemoryRead8(Address);
  D8 = (D8 & (~Mask) | Value);
  CrbMemoryWrite8(Address, D8);
}
VOID CrbMemoryModify16(UINT64 Address, UINT16 Mask, UINT16 Value)
{
  UINT16 D16 = CrbMemoryRead16(Address);
  D16 = (D16 & (~Mask) | Value);
  CrbMemoryWrite16(Address, D16);
}
VOID CrbMemoryModify32(UINT64 Address, UINT32 Mask, UINT32 Value)
{
  UINT32 D32 = CrbMemoryRead32(Address);
  D32 = (D32 & (~Mask) | Value);
  CrbMemoryWrite32(Address, D32);
}
VOID CrbMemoryModify64(UINT64 Address, UINT64 Mask, UINT64 Value)
{
  UINT64 D64 = CrbMemoryRead64(Address);
  D64 = (D64 & (~Mask) | Value);
  CrbMemoryWrite64(Address, D64);
}

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT32 ExtendedRegister;
} CRB_PCI_ADDRESS;

UINT64
CrbGetPcieMmioAddress(UINT64 BaseAddress, UINT64 PciAddress)
{
  CRB_PCI_ADDRESS* pEfiAddress = (CRB_PCI_ADDRESS*)(&PciAddress);
  UINT32 Register;
  if(pEfiAddress->ExtendedRegister != 0)
    Register = (pEfiAddress->ExtendedRegister & 0x0FFF); // Just cut within 4k
  else
    Register = pEfiAddress->Register;
  return (BaseAddress | (pEfiAddress->Bus << 20) | (pEfiAddress->Device << 15) |
    (pEfiAddress->Function << 12) | Register);
}

// Pcie read
UINT8 CrbPcieRead8(UINT64 Bar, UINT64 Address)
{ return CrbMemoryRead8(CrbGetPcieMmioAddress(Bar, Address)); }
UINT16 CrbPcieRead16(UINT64 Bar, UINT64 Address)
{ return CrbMemoryRead16(CrbGetPcieMmioAddress(Bar, Address)); }
UINT32 CrbPcieRead32(UINT64 Bar, UINT64 Address)
{ return CrbMemoryRead32(CrbGetPcieMmioAddress(Bar, Address)); }
UINT64 CrbPcieRead64(UINT64 Bar, UINT64 Address)
{ return CrbMemoryRead64(CrbGetPcieMmioAddress(Bar, Address)); }

// Pcie write
VOID CrbPcieWrite8(UINT64 Bar, UINT64 Address, UINT8 Data)
{ CrbMemoryWrite8(CrbGetPcieMmioAddress(Bar, Address), Data); }
VOID CrbPcieWrite16(UINT64 Bar, UINT64 Address, UINT16 Data)
{ CrbMemoryWrite16(CrbGetPcieMmioAddress(Bar, Address), Data); }
VOID CrbPcieWrite32(UINT64 Bar, UINT64 Address, UINT32 Data)
{ CrbMemoryWrite32(CrbGetPcieMmioAddress(Bar, Address), Data); }
VOID CrbPcieWrite64(UINT64 Bar, UINT64 Address, UINT64 Data)
{ CrbMemoryWrite64(CrbGetPcieMmioAddress(Bar, Address), Data); }

// Pcie modify
VOID CrbPcieModify8(UINT64 Bar, UINT64 Address, UINT8 Mask, UINT8 Value)
{ CrbMemoryModify8(CrbGetPcieMmioAddress(Bar, Address), Mask, Value); }
VOID CrbPcieModify16(UINT64 Bar, UINT64 Address, UINT16 Mask, UINT16 Value)
{ CrbMemoryModify16(CrbGetPcieMmioAddress(Bar, Address), Mask, Value); }
VOID CrbPcieModify32(UINT64 Bar, UINT64 Address, UINT32 Mask, UINT32 Value)
{ CrbMemoryModify32(CrbGetPcieMmioAddress(Bar, Address), Mask, Value); }
VOID CrbPcieModify64(UINT64 Bar, UINT64 Address, UINT64 Mask, UINT64 Value)
{ CrbMemoryModify64(CrbGetPcieMmioAddress(Bar, Address), Mask, Value); }



// Pci write
VOID CRB_PCIWrite8(
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

//Pci read
UINT8 CRB_PCIRead8(
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


#if 0
//**********************************************************************
//                                                              Delay
//**********************************************************************
#if defined(EFI64) || defined(EFIx64)
UINT64
DivU64x32 (
  IN UINT64   Dividend,
  IN UINTN    Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
{
    UINT64  Result = Dividend/Divisor;
    if (Remainder) *Remainder=Dividend%Divisor;
    return Result;
}

#else
UINT64 
DivU64x32 (
    IN UINT64   Dividend,
    IN UINTN    Divisor,    //Can only be 31 bits.
    OUT UINTN   *Remainder OPTIONAL
    )
{
    UINT64  Result;
    UINT32  Rem;
    __asm
    {
        mov     eax, dword ptr Dividend[0]
        mov     edx, dword ptr Dividend[4]
        mov     esi, Divisor
        xor     edi, edi                    ; Remainder
        mov     ecx, 64                     ; 64 bits
Div64_loop:
        shl     eax, 1                      ;Shift dividend left. This clears bit 0.
        rcl     edx, 1    
        rcl     edi, 1                      ;Shift remainder left. Bit 0 = previous dividend bit 63.

        cmp     edi, esi                    ; If Rem >= Divisor, dont adjust
        cmc                                 ; else adjust dividend and subtract divisor.
        sbb     ebx, ebx                    ; if Rem >= Divisor, ebx = 0, else ebx = -1.
        sub     eax, ebx                    ; if adjust, bit 0 of dividend = 1
        and     ebx, esi                    ; if adjust, ebx = Divisor, else ebx = 0. 
        sub     edi, ebx                    ; if adjust, subtract divisor from remainder.
        loop    Div64_loop

        mov     dword ptr Result[0], eax
        mov     dword ptr Result[4], edx
        mov     Rem, edi
    }

    if (Remainder) *Remainder = Rem;

    return Result;
}

#endif
VOID 
FixedDelayMicroSecond (
    UINTN           Usec                           
 )
{
    UINTN   Counter, i;
    UINT32  Data32, PrevData;
    UINTN   Remainder;

    Counter = (UINTN)DivU64x32 ((Usec * 10), 3, &Remainder);

    if (Remainder != 0) {
        Counter++;
    }

    //
    // Call WaitForTick for Counter + 1 ticks to try to guarantee Counter tick
    // periods, thus attempting to ensure Microseconds of stall time.
    //
    if (Counter != 0) {

        PrevData = IoRead32(PM_BASE_ADDRESS + 8);
        for (i=0; i < Counter; ) {
            Data32 = IoRead32(PM_BASE_ADDRESS + 8);    
            if (Data32 < PrevData) {        // Reset if there is a overlap
                PrevData=Data32;
                continue;
            }
            i += (Data32 - PrevData);        
            PrevData = Data32;
        }
    }
    return;
}  
#endif
//;modify this Routine by using IO-Cycle Delay Counting for BYO BIOS temporarily - start
VOID 
FixedDelayMicroSecond (
    UINTN           Usec                           
 )
{
    UINTN   Counter = 0;

	while(Counter != Usec){
		IoRead8(0x88);
		Counter++;
	}

    return;
} 
//;modify this Routine by using IO-Cycle Delay Counting for BYO BIOS temporarily - end

/*
VOID 
FixedDelayMicroSecond (
    UINTN           Usec                           
 )
{
    UINTN   Counter = 0, i;
    UINT32  Data32, PrevData;
    UINTN   Remainder = 0;

    //LGE-20151221 comment out Temporarily
    //Counter = (UINTN)DivU64x32 ((Usec * 10), 3, &Remainder);

    if (Remainder != 0) {
        Counter++;
    }

    //
    // Call WaitForTick for Counter + 1 ticks to try to guarantee Counter tick
    // periods, thus attempting to ensure Microseconds of stall time.
    //
    if (Counter != 0) {

        PrevData = IoRead32(PM_BASE_ADDRESS + 8);
        for (i=0; i < Counter; ) {
            Data32 = IoRead32(PM_BASE_ADDRESS + 8);    
            if (Data32 < PrevData) {        // Reset if there is a overlap
                PrevData=Data32;
                continue;
            }
            i += (Data32 - PrevData);        
            PrevData = Data32;
        }
    }
    return;
}  
*/
EFI_STATUS GetSetupData(VOID)
{
    EFI_STATUS             Status;
        SETUP_DATA   SetupData;
    UINTN                  SetupDataSize;

    DEBUG((EFI_D_ERROR,"SmmGetVariable()\n")); 
	//// 2016041301-TGR patched for EMU Variable.
	if(PcdGetBool(PcdVarServiceUseEmu)){
	   DEBUG((EFI_D_INFO, "(Smm)UseEmuVarService, CRBSmi.c - GetSetupData return directly \n"));	   
	   return EFI_SUCCESS;
	}
	////
	
  //
  // Locate SmmVariableProtocol
  //
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
    
    gACLossAutoRestart = SetupData.AfterPowerLoss;
    gChipsetSERRNBControl = SetupData.SERRNBControl;
        //LGE-20160115
    gChipsetLoopERSMIControl = SetupData.LoopERSMIControl;
    gChipsetHIFErrControl = SetupData.HIFErrControl;
    gChipsetDRAMErrControl = SetupData.DRAMErrControl;          
//	gChipsetPEGErrControl = SetupData.PEGErrControl;
    gChipsetPE0ErrControl = SetupData.PE0ErrControl;
    gChipsetPE1ErrControl = SetupData.PE1ErrControl;
    gChipsetPE2ErrControl = SetupData.PE2ErrControl;
    gChipsetPE3ErrControl = SetupData.PE3ErrControl;
    gChipsetPE4ErrControl = SetupData.PE4ErrControl;
	gChipsetPE5ErrControl = SetupData.PE5ErrControl;
    gChipsetPE6ErrControl = SetupData.PE6ErrControl;
	gChipsetPE7ErrControl = SetupData.PE7ErrControl;
    ChipRevision = CrbPcieRead8(0xE0000000, CHX002_NPMC|((UINT64)(D0F4_INTERNAL_REV_ID) << 32));        //Get Chipset Revision Number   

    DEBUG((EFI_D_ERROR,"BIOS SETUP gChipsetDRAMErrControl = %02x\n", gChipsetDRAMErrControl));
    DEBUG((EFI_D_ERROR,"BIOS SETUP gChipsetHIFErrControl = %02x\n", gChipsetHIFErrControl));
        
        return EFI_SUCCESS;
}


//----------------------------------------------------------------------------
//
// Procedure:   GetCRBErrSmiContext
//
// Description: This is a template CRB Error SMI GetContext for Porting.
//
// Parameters:  None
//
// Returns:     None
//
// Modified:
//
// Referrals:
//
// Notes:       Here is the control flow of this function:
//              1. Check if CRB Smi source.
//              2. If yes, return TRUE.
//              3. If not, return FALSE.
//
//----------------------------------------------------------------------------

BOOLEAN GetCRBErrSmiContext()
{

//lna_pcie_0419-start
        if((IoRead8(PM_BASE_ADDRESS + PMIO_EXTEND_SMI_IO_TRAP_STA) & PMIO_NB2SB_SMI_STS) && 
                (IoRead8(PM_BASE_ADDRESS + PMIO_GENERAL_PURPOSE_SMI_ENABLE) & PMIO_NB2SB_SMI_SM)) {   //For SERR_NB SMI
                return TRUE;
        }
//lna_pcie_0419 - end
//LGE20160309 - E

        return FALSE;
}


typedef struct {
        UINT8 ErrSrc;
        UINT8 Correctable;
        UINT8 ErrDetail1;
        UINT8 ErrDetail2;
        UINT32 ErrDetail3;      //;Additional Error Information
        UINT32 ErrDetail4;      //;Additional Error Information
        UINT8 ErrDetail5;       //;Additional Error Information
        UINT8 ErrDetail6;       //;Additional Error Information
        UINT8 NextErr;          //;Indicate whether continous Error occur
} REPO_SERR;



//----------------------------------------------------------------------------
//
// Procedure:   ChkV4IFSerr
//
// Description: Check SERR from V4IF
//
// Parameters:  None
//
// Returns:     None
//
//----------------------------------------------------------------------------

VOID ChkV4IFSerr (
        IN UINT64       pciebase,
        IN OUT REPO_SERR* SERR
)
{       
//LGE20160308 CHX001 Without V4IF
#if 0
        if(SERR->ErrSrc == 0xFF){       //;No previous Error occurred
                if((CrbPcieRead8(pciebase, CHX002_ERRC|((UINT64)(D0F1_CPU_BUS_ERR_STA) << 32)) & D0F1_HAP_ERR) != 0){           //CPU BUS Address Parity Error detected
                        if(gChipsetV4IFErrControl == TRUE)
                                SERR->ErrSrc = 0x01;

                        SERR->Correctable = 0x0;
                        CrbPcieModify8(pciebase, CHX002_ERRC|((UINT64)(D0F1_CPU_BUS_ERR_STA) << 32), D0F1_HAP_ERR, D0F1_HAP_ERR);       //Clear CPU BUS Address Parity Error Status BIT
                }
        }
#endif
}




//----------------------------------------------------------------------------
//
// Procedure:   ChkDRAMSerr
//
// Description: Check SERR from DRAM ECC,Partiy,CRC
//
// Parameters:  None
//
// Returns:     None
//
//----------------------------------------------------------------------------

VOID ChkDRAMSerr (
        IN UINT64       pciebase,
        IN OUT REPO_SERR* SERR
)
{       

        //UINT8 EccError;
		//UINT32 ParCRCError;
	

        //if(SERR->ErrSrc == 0xFF){       //;No previous Error occurred

			////TODO RKD remove for pxp code parpare
                ////EccError = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32));
				
				////LGE20160505 read D0F3 Rx17C, check parity status and CRC error status
			////TODO RKD remove for pxp code parpare
				////ParCRCError = CrbPcieRead32(pciebase, CHX002_DRAM|((UINT64)(D0F3_CHN_B_RETRY_ERR_LOG) << 32));
				
        
                //if((EccError & (D0F3_RSEFS_CHA + D0F3_RSEFS_CHB)) != 0){           //;Correctable (Single-BIT) Error

				 //DEBUG((EFI_D_ERROR,"Ecc Single bit error\n"));

                        //if((gChipsetDRAMErrControl == ERR_Correctable) || (gChipsetDRAMErrControl == ERR_Both)){
                                //SERR->ErrSrc = 0x02;
                        
                                //SERR->Correctable = 0x01;

                                //if((EccError & D0F3_RSEFS_CHA) != 0){      //Correctable-Error occurred in Channel A
                                ////LGE20160505 
                                        //SERR->ErrDetail1 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_RANK_INF_FOR_CHA) << 32)) & D0F3_SDBKS_2_0_CHA;
                
                                //SERR->ErrDetail3 = CrbPcieRead32(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_ADR_CHA) << 32));  //;ECC Error Address in CHA
                                        //SERR->ErrDetail5 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_PATTERN + 1) << 32));       //;ECC Syndrome BIT Address in CHA
                                        //CrbPcieModify8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), D0F3_RSEFS_CHA, D0F3_RSEFS_CHA);       //Clear DRAM CHA Correctable (Single Bit) Error Status BIT

                                 
                                 //DEBUG((EFI_D_ERROR,"Ecc CHA Single bit Error Address = %08x\n", SERR->ErrDetail3 << 3));
								 //DEBUG((EFI_D_ERROR,"Ecc CHA Single bit Error Syndrome = %08x\n", SERR->ErrDetail5));
								 //DEBUG((EFI_D_ERROR,"Ecc CHA Single bit Error Rank = %08x\n", SERR->ErrDetail1));
								//}

                                //if((EccError & D0F3_RSEFS_CHB) != 0){      //Correctable-Error occurred in Channel B
                                ////LGE20160505 Wait for  IRS Update
                                    //SERR->ErrDetail2 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_RANK_INF_FOR_CHB) << 32)) & D0F3_SDBKS_2_0_CHB;
                    
                                //SERR->ErrDetail4 = CrbPcieRead32(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_ADR_CHB) << 32));  //;ECC Error Address in CHB
                                        //SERR->ErrDetail6 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_PATTERN) << 32));   //;ECC Syndrome BIT Address in CHB
                                        //CrbPcieModify8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), D0F3_RSEFS_CHB, D0F3_RSEFS_CHB);       //Clear DRAM CHB Correctable (Single Bit) Error Status BIT

                                 //DEBUG((EFI_D_ERROR,"Ecc CHB Single bit Error Address = %08x\n", SERR->ErrDetail4 << 3));
								 //DEBUG((EFI_D_ERROR,"Ecc CHB Single bit Error Syndrome = %08x\n", SERR->ErrDetail6));
								 //DEBUG((EFI_D_ERROR,"Ecc CHB Single bit Error Rank = %08x\n", SERR->ErrDetail2));
								//}                                               

                                ////Check whether ECC still being generated
                                //EccError = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32));
                                //if ((EccError & (D0F3_RSEFS_CHA + D0F3_RSEFS_CHB)))
                                        //SERR->NextErr = 1;

                        //}
                //}

                //if((EccError & (D0F3_RMEFS_CHA + D0F3_RMEFS_CHB)) != 0){           //;Uncorrectable (Multi-BIT) Error

					   //DEBUG((EFI_D_ERROR,"Ecc Multi bit error\n"));
                        //if((gChipsetDRAMErrControl == ERR_Uncorrectable) || (gChipsetDRAMErrControl == ERR_Both)){

                                //SERR->ErrSrc = 0x02;
                        
                                //SERR->Correctable = 0x00;

                                //if((EccError & D0F3_RMEFS_CHA) != 0){      //Uncorrectable-Error occurred in Channel A
                                ////LGE20160505 
                                        //SERR->ErrDetail1 = (CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_RANK_INF_FOR_CHA) << 32)) & D0F3_MCDBKS_2_0_CHA) >> 3;
                                        //SERR->ErrDetail3 = CrbPcieRead32(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_Z2) << 32));      //;ECC Error Address in CHA
                 
                                //SERR->ErrDetail5 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_PATTERN + 1) << 32)); //;ECC Syndrome BIT Address in CHA
                                        //CrbPcieModify8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), D0F3_RMEFS_CHA, D0F3_RMEFS_CHA);       //Clear DRAM CHA Uncorrectable (Multi Bit) Error Status BIT
                                 //DEBUG((EFI_D_ERROR,"Ecc CHA Multi bit Error Address = %08x\n", SERR->ErrDetail3 << 3));
								 //DEBUG((EFI_D_ERROR,"Ecc CHA Multi bit Error Syndrome = %08x\n", SERR->ErrDetail5));
								 //DEBUG((EFI_D_ERROR,"Ecc CHA Multi bit Error Rank = %08x\n", SERR->ErrDetail1)); 
								//}

                                //if((EccError & D0F3_RMEFS_CHB) != 0){      //Uncorrectable-Error occurred in Channel B
                   
                                        //SERR->ErrDetail2 = (CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_RANK_INF_FOR_CHB) << 32)) & (D0F3_MCDBKS_2_0_CHB)) >> 3;
                                        //SERR->ErrDetail4 = CrbPcieRead32(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_Z6) << 32));      //;ECC Error Address in CHB
                   
                                   //SERR->ErrDetail6 = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_REPORT_PATTERN) << 32));  //;ECC Syndrome BIT Address in CHB
                                        //CrbPcieModify8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), D0F3_RMEFS_CHB, D0F3_RMEFS_CHB);       //Clear DRAM CHB Uncorrectable (Multi Bit) Error Status BIT
                                //DEBUG((EFI_D_ERROR,"Ecc CHB Multi bit Error Address = %08x\n", SERR->ErrDetail4 << 3));
								 //DEBUG((EFI_D_ERROR,"Ecc CHB Multi bit Error Syndrome = %08x\n", SERR->ErrDetail6));
								 //DEBUG((EFI_D_ERROR,"Ecc CHB Multi bit Error Rank = %08x\n", SERR->ErrDetail2));
								//}                       

                                ////Check whether ECC still being generated
                                //EccError = CrbPcieRead8(pciebase, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32));
                                //if ((EccError & (D0F3_RMEFS_CHA + D0F3_RMEFS_CHB)))
                                        //SERR->NextErr = 1;

                        //}
                //} 


				////Parity error
                //if(ParCRCError &(D0F3_RERRSTS_PAR_CHA | D0F3_RERRSTS_PAR_CHB))
                //{
                  //DEBUG((EFI_D_ERROR,"Parity error\n"));

				  ////LGE20160710 Clear Parity error status  D0F3_RERRCLR_PAR,  Write 0 Then Write 1
				  //CrbPcieModify32(pciebase, CHX002_DRAM|((UINT64)(D0F3_CHN_B_RETRY_ERR_LOG) << 32), D0F3_RERRCLR_PAR, 0);
				  //CrbPcieModify32(pciebase, CHX002_DRAM|((UINT64)(D0F3_CHN_B_RETRY_ERR_LOG) << 32), D0F3_RERRCLR_PAR, D0F3_RERRCLR_PAR);
                //}
				////CRC error
                //if(ParCRCError &(D0F3_RERRSTS_CRC_CHA | D0F3_RERRSTS_CRC_CHB))
                //{
                  //DEBUG((EFI_D_ERROR,"CRC error\n")); 
				  ////LGE20160710 Clear CRC error status, Write 0 Then Write 1  
				  //CrbPcieModify32(pciebase, CHX002_DRAM|((UINT64)(D0F3_CHN_B_RETRY_ERR_LOG) << 32), D0F3_RERRCLR_CRC, 0);
				  //CrbPcieModify32(pciebase, CHX002_DRAM|((UINT64)(D0F3_CHN_B_RETRY_ERR_LOG) << 32), D0F3_RERRCLR_CRC, D0F3_RERRCLR_CRC);
                //}
				
        //}

////LGE-20160505 Clear error status some more times for temp -s
//#if 1
    //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
        //CrbPcieWrite8(0xE0000000, CHX002_DRAM|((UINT64)(D0F3_ECC_ERR_STA) << 32), 0x0F);
////LGE-20160505 Clear error status some more times for temp -s
//#endif  
}

//
typedef struct {
        UINT8 AERType;  //; 0: uncorrectable; 1: correctable
        UINT8 BITLoc;   //; BIT Location - for indicate Error exactly
        UINT32 MaskBit; //; Mask Bit
} AERCodeTable;

AERCodeTable AERCode[] = {

//      AERType,        BITLoc,         MaskBit
        0x00,           0x00,           BIT0,
        0x00,           0x04,           BIT4,
        0x00,           0x05,           BIT5,
        0x00,           0x0C,           BIT12,
        0x00,           0x0D,           BIT13,
        0x00,           0x0E,           BIT14,
        0x00,           0x0F,           BIT15,
        0x00,           0x10,           BIT16,
        0x00,           0x11,           BIT17,
        0x00,           0x12,           BIT18,
        0x00,           0x13,           BIT19,
        0x00,           0x14,           BIT20,
        0x00,           0x15,           BIT21,
        0x01,           0x00,           BIT0,
        0x01,           0x06,           BIT6,
        0x01,           0x07,           BIT7,
        0x01,           0x08,           BIT8,
        0x01,           0x0C,           BIT12,
        0x01,           0x0D,           BIT13
        
};
//

//----------------------------------------------------------------------------
//
// Procedure:   ChkPESource
//
// Description: Check PCIE Error Source and return the defined Error Source Number
//
// Parameters:  None
//
// Returns:     Error Source Number
//
//----------------------------------------------------------------------------

UINT8 ChkPESource (     
        IN UINT64       RPAddr
)
{
        UINT8 ErrSrc;

        switch (RPAddr){
 //               case CHX002_PEG:{
  //                      ErrSrc = 0x03;
  //              } break;

                case CHX002_PE0:{
                        ErrSrc = 0x04;
                } break;                

                case CHX002_PE1:{
                        ErrSrc = 0x05;
                } break;

                case CHX002_PE2:{
                        ErrSrc = 0x06;
                } break;

                case CHX002_PE3:{
                        ErrSrc = 0x07;
                } break;

				case CHX002_PE4:{
                        ErrSrc = 0x08;
                } break;

				case CHX002_PE5:{
						ErrSrc = 0x09;
				} break;
				case CHX002_PE6:{
						ErrSrc = 0x0A;
				} break;
				case CHX002_PE7:{
						ErrSrc = 0x0B;
				} break;
                default:{
                        ErrSrc = 0xFF;
                } break;
        }

        return ErrSrc;

}

//----------------------------------------------------------------------------
//
// Procedure:   ChkPESerr
//
// Description: Check PCIE SERR from RP
//
// Parameters:  None
//
// Returns:     None
//
//----------------------------------------------------------------------------

VOID ChkPESerr (
        IN UINT64       pciebase,
        IN UINT64       RPAddr,
        IN OUT REPO_SERR* SERR,
        IN UINT8 PCIE_SETUP_DATA
)
{

        UINT8 i;
        UINT32 AER;
        UINT32 Temp32;
		UINT64 RcrbhBaseAddr;

		RcrbhBaseAddr = (UINT64)CrbPcieRead32(pciebase, CHX002_APIC|((UINT64)(D0F5_RCRB_H_BASE_ADR) << 32));
		RcrbhBaseAddr = RcrbhBaseAddr&D0F5_RXRCRBHA_OUT_39_12;
		RcrbhBaseAddr = RcrbhBaseAddr<<12;
			
		
        if(SERR->ErrSrc == 0xFF){       //;No previous Error occurred
                if(CrbPcieRead16(pciebase, RPAddr|((UINT64)(D3D5F1_VID) << 32)) != 0xFFFF){     //;only check when RP exist (VID != 0xFFFF)
                        if((CrbPcieRead8(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32)) & D3D5F1_RESSF) != 0){ //Uncorrectable Error

                                if((PCIE_SETUP_DATA == ERR_Uncorrectable) || (PCIE_SETUP_DATA == ERR_Both))
                                        SERR->ErrSrc = ChkPESource(RPAddr);

                                SERR->Correctable = 0x00;

                                AER = CrbPcieRead32(pciebase, RPAddr|((UINT64)(D3D5F1_UNCORRECTABLE_ERR_STA) << 32));   //Get the UC AER Status Register in current RP
                                
                                if (AER != 0){ //the UC AER occurred within the RP
                                
                                    for(i=0; i<sizeof(AERCode)/sizeof(AERCodeTable); i++)       //Check the Error Table for detecting exactly Error information
                                    {
                                                Temp32 = AER & AERCode[i].MaskBit;      //;the UC AER BIT in current RP
                                        
                                        if(AERCode[i].AERType == 0x00 && Temp32 != 0){  //;UC AER detected       within the RP                                          
                                                
                                                        SERR->ErrDetail2 = AERCode[i].BITLoc;

                                                        CrbPcieWrite32(pciebase, RPAddr|((UINT64)(D3D5F1_UNCORRECTABLE_ERR_STA) << 32), Temp32);        //Clear detected AER Status BIT                                         
                                                        CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), (D3D5F1_RESSF + D3D5F1_RESMF + D3D5F1_RESFFRV), (D3D5F1_RESSF + D3D5F1_RESMF + D3D5F1_RESFFRV));        //Clear detected UC AER Root Error Status BIT                                                   

                                                        //Check Severity Register to determine Non-Fatal or Fatal
                                                        if ((CrbPcieRead32(pciebase, RPAddr|((UINT64)(D3D5F1_UNCORRECTABLE_ERR_SEVERITY) << 32)) & AERCode[i].MaskBit) == 0){   //this Error is Non-Fatal
                                                                SERR->ErrDetail1 = 0x22;
                                                                CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), D3D5F1_RESNFRV, D3D5F1_RESNFRV);        //Clear detected Non-Fatal AER Root Error Status BIT
                                                                CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_DEV_STA_1) << 32), D3D5F1_DSNFED, D3D5F1_DSNFED);      //Clear detected Non-Fatal Error Device Status BIT
                                                        }else{
                                                                SERR->ErrDetail1 = 0x33;
                                                                CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), D3D5F1_RESFRV, D3D5F1_RESFRV);  //Clear detected Fatal AER Root Error Status BIT
                                                                CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_DEV_STA_1) << 32), D3D5F1_DSFED, D3D5F1_DSFED);        //Clear detected Fatal Error Device Status BIT
                                                        }

                                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_PCI_STA) << 32), D3D5F1_SERRS, D3D5F1_SERRS); //Clear detected SERR PCI Status BIT
                                                      // CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_PCIE_ROOT_PORT_ERR_STA_D4F0) << 32), D3D5F1_PE_ERR, D3D5F1_PE_ERR);    //Clear PE_ERR (PCIE SERR_NB) Status BIT
                                              	CrbMemoryModify8(RcrbhBaseAddr + RCRBH_PCIE_ROOT_PORT_ERR_STA,RCRBH_PE_ERR,RCRBH_PE_ERR); //Clear PE_ERR (PCIE SERR_NB) Status BIT- CJW_20170612 
                                        }
                                    }                           
                                }else{  //the UC AER coming from EP of the RP

                                        SERR->ErrDetail1 = 0x44;
                                        SERR->ErrDetail2 = 0xFF;

                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_SECONDARY_STA) << 32), D3D5F1_SSERRS, D3D5F1_SSERRS); //Clear detected Received Error in P2P Bridge Secondary Status
                                        CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), (D3D5F1_RESSF + D3D5F1_RESMF + D3D5F1_RESFFRV), (D3D5F1_RESSF + D3D5F1_RESMF + D3D5F1_RESFFRV));        //Clear detected UC AER Root Error Status BIT
                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_PCI_STA) << 32), D3D5F1_SERRS, D3D5F1_SERRS); //Clear detected SERR PCI Status BIT
                                       //CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_PCIE_ROOT_PORT_ERR_STA_D4F0) << 32), D3D5F1_PE_ERR, D3D5F1_PE_ERR);    //Clear PE_ERR (PCIE SERR_NB) Status BIT
										CrbMemoryModify8(RcrbhBaseAddr + RCRBH_PCIE_ROOT_PORT_ERR_STA,RCRBH_PE_ERR,RCRBH_PE_ERR); //Clear PE_ERR (PCIE SERR_NB) Status BIT- CJW_20170612
								}
                        }

                        if((CrbPcieRead8(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32)) & D3D5F1_RESSC) != 0){         //Correctable Error

                                if((PCIE_SETUP_DATA == ERR_Correctable) || (PCIE_SETUP_DATA == ERR_Both))
                                        SERR->ErrSrc = ChkPESource(RPAddr);
                                
                                SERR->Correctable = 0x01;

                                AER = CrbPcieRead32(pciebase, RPAddr|((UINT64)(D3D5F1_CORRECTABLE_ERR_STA) << 32));     //Get the Correctable AER Status Register in current RP

                                if (AER != 0){ //the Correctable AER occurred within the RP

                                    for(i=0; i<sizeof(AERCode)/sizeof(AERCodeTable); i++)       //Check the Error Table for detecting exactly Error information
                                    {
                                                Temp32 = AER & AERCode[i].MaskBit;      //;the Correctable AER BIT in current RP
                                    
                                        if((AERCode[i].AERType == 0x01) && (Temp32 != 0)){      //;Correctable AER detected      within the RP
                                                        SERR->ErrDetail2 = AERCode[i].BITLoc;                                   
                                                        SERR->ErrDetail1 = 0x11;

                                                        CrbPcieWrite32(pciebase, RPAddr|((UINT64)(D3D5F1_CORRECTABLE_ERR_STA) << 32), Temp32);  //Clear detected AER Status BIT                                         
                                                        CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), (D3D5F1_RESSC + D3D5F1_RESMC), (D3D5F1_RESSC + D3D5F1_RESMC));  //Clear detected Correctable AER Root Error Status BIT                                                  
                                                        CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_DEV_STA_1) << 32), D3D5F1_DSCED, D3D5F1_DSCED);        //Clear detected Correctable Error Device Status BIT
                                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_PCI_STA) << 32), D3D5F1_SERRS, D3D5F1_SERRS); //Clear detected SERR PCI Status BIT
                                                       // CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_PCIE_ROOT_PORT_ERR_STA_D4F0) << 32), D3D5F1_PE_ERR, D3D5F1_PE_ERR);    //Clear PE_ERR (PCIE SERR_NB) Status BIT
												CrbMemoryModify8(RcrbhBaseAddr + RCRBH_PCIE_ROOT_PORT_ERR_STA,RCRBH_PE_ERR,RCRBH_PE_ERR); //Clear PE_ERR (PCIE SERR_NB) Status BIT- CJW_20170612
										}
                                    }
                                }else{  //the Correctable AER coming from EP of the RP

                                        SERR->ErrDetail1 = 0x44;
                                        SERR->ErrDetail2 = 0xFF;

                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_SECONDARY_STA) << 32), D3D5F1_SSERRS, D3D5F1_SSERRS); //Clear detected Received Error in P2P Bridge Secondary Status
                                        CrbPcieModify32(pciebase, RPAddr|((UINT64)(D3D5F1_ROOT_ERR_STA) << 32), (D3D5F1_RESSC + D3D5F1_RESMC), (D3D5F1_RESSC + D3D5F1_RESMC));  //Clear detected Correctable AER Root Error Status BIT
                                        CrbPcieModify16(pciebase, RPAddr|((UINT64)(D3D5F1_PCI_STA) << 32), D3D5F1_SERRS, D3D5F1_SERRS); //Clear detected SERR PCI Status BIT
                                      // CrbPcieModify8(pciebase, RPAddr|((UINT64)(D3D5F1_PCIE_ROOT_PORT_ERR_STA_D4F0) << 32), D3D5F1_PE_ERR, D3D5F1_PE_ERR);    //Clear PE_ERR (PCIE SERR_NB) Status BIT
										CrbMemoryModify8(RcrbhBaseAddr + RCRBH_PCIE_ROOT_PORT_ERR_STA,RCRBH_PE_ERR,RCRBH_PE_ERR); //Clear PE_ERR (PCIE SERR_NB) Status BIT - CJW_20170612
								}
                        }                               
                }
        }
}


//----------------------------------------------------------------------------
//
// Procedure:   CRBErrSmiHandler
//
// Description: This is a template CRB Error SMI Handler for Porting.
//
// Parameters:  None
//
// Returns:     None
//
//----------------------------------------------------------------------------

VOID CRBErrSmiHandler ()
{
        UINT64 pciebase = 0xE0000000;
        UINT8 debug_port = 0x80;
        REPO_SERR SERR_NB = {0};

#if ERSMI_COMPORT_DEBUG_MESSAGES
        UINT8 Buffer8;
        UINT16 Buffer16;
#endif  //; ERSMI_COMPORT_DEBUG_MESSAGES        

#if ERSMI_COMPORT_DEBUG_MESSAGES

        //;Set Baud Rate for COM Port output
        //;======================
        //; set Baud rate = 115200
        //;======================
        //; Enable DLAB
        Buffer8 = IoRead8(0x3FB);
        Buffer8 = Buffer8 |0x80;                
        IoWrite8(0x3FB,Buffer8);
        //; Save Baud rate
        Buffer16 = IoRead16(0x3F8);
        //; Set Baud rate (LSB)
        IoWrite8(0x3F8,0x01);
        //; Set Baud rate (MSB)
        IoWrite8(0x3F9,0x00);
        //; Disable DLAB
        Buffer8 = IoRead8(0x3FB);
        Buffer8 = Buffer8 & 0x7F;               
        IoWrite8(0x3FB,Buffer8);

        DEBUG((EFI_D_ERROR,"\n===================== Enter Error Handling SMI ============================\n"));

#endif  //; ERSMI_COMPORT_DEBUG_MESSAGES        
     
        do{

                SERR_NB.ErrSrc = (UINT8)-1;
                SERR_NB.Correctable = (UINT8)-1;
                SERR_NB.ErrDetail1 = (UINT8)-1;
                SERR_NB.ErrDetail2 = (UINT8)-1;
                SERR_NB.ErrDetail3 = (UINT8)-1;
                SERR_NB.ErrDetail4 = (UINT8)-1;
                SERR_NB.ErrDetail5 = (UINT8)-1;
                SERR_NB.ErrDetail6 = (UINT8)-1;
                SERR_NB.NextErr = 0;

                
                //Check Error Type
                //ChkV4IFSerr(pciebase, &SERR_NB);

                ChkDRAMSerr(pciebase, &SERR_NB);

//                ChkPESerr(pciebase, CHX002_PEG, &SERR_NB, gChipsetPEGErrControl);
                ChkPESerr(pciebase, CHX002_PE0, &SERR_NB, gChipsetPE0ErrControl);                
                ChkPESerr(pciebase, CHX002_PE1, &SERR_NB, gChipsetPE1ErrControl);                
                ChkPESerr(pciebase, CHX002_PE2, &SERR_NB, gChipsetPE2ErrControl);                
                ChkPESerr(pciebase, CHX002_PE3, &SERR_NB, gChipsetPE3ErrControl);                
                ChkPESerr(pciebase, CHX002_PE4, &SERR_NB, gChipsetPE4ErrControl);
                ChkPESerr(pciebase, CHX002_PE5, &SERR_NB, gChipsetPE5ErrControl);
                ChkPESerr(pciebase, CHX002_PE6, &SERR_NB, gChipsetPE6ErrControl);
                ChkPESerr(pciebase, CHX002_PE7, &SERR_NB, gChipsetPE7ErrControl);

                //Previous error occurred
                if (SERR_NB.ErrSrc != 0xFF){

                        //Loop Error Code Report according to Setup-Item
                        do{

                                //Printing Error SMI Signature to IO Port 0x80
                                IoWrite8(debug_port, 0xEE);             
                                FixedDelayMicroSecond(1000000);   // 1s delay           

                                IoWrite8(debug_port, 0x5A);                             
                                FixedDelayMicroSecond(1000000);   // 1s delay

                                //Printing Types and Details of Error Source
                                IoWrite8(debug_port, SERR_NB.ErrSrc);           
                                FixedDelayMicroSecond(1000000);   // 1s delay                   
                                IoWrite8(debug_port, SERR_NB.Correctable);              
                                FixedDelayMicroSecond(1000000);   // 1s delay           
                                IoWrite8(debug_port, SERR_NB.ErrDetail1);               
                                FixedDelayMicroSecond(1000000);   // 1s delay                   
                                IoWrite8(debug_port, SERR_NB.ErrDetail2);               
                                FixedDelayMicroSecond(1000000);   // 1s delay                   

                                IoWrite32(debug_port, SERR_NB.ErrDetail3);              
                                FixedDelayMicroSecond(1000);      // 1ms delay  
                                IoWrite32(debug_port, SERR_NB.ErrDetail4);              
                                FixedDelayMicroSecond(1000);      // 1ms delay
                                IoWrite8(debug_port, SERR_NB.ErrDetail5);               
                                FixedDelayMicroSecond(1000);      // 1ms delay  
                                IoWrite8(debug_port, SERR_NB.ErrDetail6);               
                                FixedDelayMicroSecond(1000);      // 1ms delay          

                                //End of Error Log Message
                                IoWrite8(debug_port, 0xA5);             
                                FixedDelayMicroSecond(1000000);   // 1s delay   

#if ERSMI_COMPORT_DEBUG_MESSAGES

                                //;Also print Error-Code through COM Port here
                                DEBUG((EFI_D_ERROR,"\n== Error Information - Check the Error Code Document for more details ==\n"));
                                DEBUG((EFI_D_ERROR,"Error Source: %X; Correctable: %X\n",SERR_NB.ErrSrc, SERR_NB.Correctable));
                                DEBUG((EFI_D_ERROR,"Error Details 1: %X\n",SERR_NB.ErrDetail1));
                                DEBUG((EFI_D_ERROR,"Error Details 2: %X\n",SERR_NB.ErrDetail2));
                                DEBUG((EFI_D_ERROR,"Error Details 3: %X\n",SERR_NB.ErrDetail3));
                                DEBUG((EFI_D_ERROR,"Error Details 4: %X\n",SERR_NB.ErrDetail4));
                                DEBUG((EFI_D_ERROR,"Error Details 5: %X\n",SERR_NB.ErrDetail5));
                                DEBUG((EFI_D_ERROR,"Error Details 6: %X\n",SERR_NB.ErrDetail6));

#endif  //; ERSMI_COMPORT_DEBUG_MESSAGES

                        }while(gChipsetLoopERSMIControl);

                }

                
                
                //Clear SERR_NB SMI Status (PMIO Rx40[6])again to avoid continous error occur
                IoWrite8(PM_BASE_ADDRESS + PMIO_EXTEND_SMI_IO_TRAP_STA, PMIO_NB2SB_SMI_STS); //clear SERR_NB status (PMIO_Rx39[1])
                CrbPcieModify8(pciebase, CHX002_SCRCH|((UINT64)(0x78)) <<32, BIT2, BIT2);       //set d0f6rx78[2] = 1 //Lana for SMI backdoor
        }while(SERR_NB.ErrSrc == 0);

#if ERSMI_COMPORT_DEBUG_MESSAGES

        DEBUG((EFI_D_ERROR,"\n===================== Exit Error Handling SMI ============================\n"));

        //;Restore Baud Rate for COM Port output        
        //;======================
        //; Restore Baud rate
        //;======================
        //; Enable DLAB
        Buffer8 = IoRead8(0x3FB);
        Buffer8 = Buffer8 |0x80;                
        IoWrite8(0x3FB,Buffer8);        
        //; Restore Baud rate (LSB)
        IoWrite8(0x3F8,(UINT8)(Buffer16 & 0xFF));
        //; Restore Baud rate (MSB)
        IoWrite8(0x3F9,(UINT8)((Buffer16 & 0xFF00) >> 0x8));
        //; Disable DLAB
        Buffer8 = IoRead8(0x3FB);
        Buffer8 = Buffer8 & 0x7F;               
        IoWrite8(0x3FB,Buffer8);
        
#endif  //; ERSMI_COMPORT_DEBUG_MESSAGES

}





BOOLEAN GetCRBSmiContext()
{
/* //HYL-20160503	
//HYL-20160501-start
	UINT64 pciebase = 0xE0000000;
	UINT32 espibase,espidata,espidata1,espidata2,espidata3;
	

	espibase=CrbPcieRead32(pciebase, CHX001_ESPI|((UINT64)(0x10)) <<32);
	espidata=CrbMemoryRead32(espibase+0x110)&0x00FFFFFFF;
	espidata1=CrbMemoryRead32(espibase+0x114)&0x007A0FFB;
	espidata2=CrbMemoryRead32(espibase+0x118)&0x00000FFE;
	espidata3=CrbMemoryRead32(espibase+0x11C)&0x01FE3F00;
	
		if((espidata!=0)||(espidata1!=0)||(espidata2!=0)||(espidata3!=0))
		{
		return TRUE;
		
		}
//HYL-20160501-end
*/ //HYL-20160503	
        return FALSE;
}


VOID CRBSmiHandler ( )
{
/* //HYL-20160503	
//HYL-20160501-start
#if ERSMI_COMPORT_DEBUG_MESSAGES
	UINT8 Buffer8;
	UINT16 Buffer16;
#endif	//; ERSMI_COMPORT_DEBUG_MESSAGES
	UINT64 pciebase = 0xE0000000;
	UINT32 espibase,espidata,espidata1,espidata2,espidata3;

#if ERSMI_COMPORT_DEBUG_MESSAGES

	//;Set Baud Rate for COM Port output
	//;======================
	//; set Baud rate =  
	//;======================
	//; Enable DLAB
	Buffer8 = IoRead8(0x3FB);
	Buffer8 = Buffer8 |0x80;		
	IoWrite8(0x3FB,Buffer8);
	//; Save Baud rate
	Buffer16 = IoRead16(0x3F8);
	//; Set Baud rate (LSB)
	IoWrite8(0x3F8,0x01);
	//; Set Baud rate (MSB)
	IoWrite8(0x3F9,0x00);
	//; Disable DLAB
	Buffer8 = IoRead8(0x3FB);
	Buffer8 = Buffer8 & 0x7F;		
	IoWrite8(0x3FB,Buffer8);

	DEBUG((EFI_D_ERROR,"\n===================== Enter Error Handling SMI ============================\n"));

#endif	//; ERSMI_COMPORT_DEBUG_MESSAGES
	espibase=CrbPcieRead32(pciebase, CHX001_ESPI|((UINT64)(0x10)) <<32);
	espidata=CrbMemoryRead32(espibase+0x110);
	espidata1=CrbMemoryRead32(espibase+0x114);
	espidata2=CrbMemoryRead32(espibase+0x118);
	espidata3=CrbMemoryRead32(espibase+0x11C);

	if((espidata&0x00000001)==0x00000001)
	DEBUG((EFI_D_ERROR,"\nSW triggered transaction (by writing MMIO Rx04h register) done on espi bus status\n"));
	if((espidata&0x00000002)==0x00000002)
	DEBUG((EFI_D_ERROR,"\nInvalid response code status for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000004)==0x00000004)
	DEBUG((EFI_D_ERROR,"\nInvalid cycle type status for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000008)==0x00000008)
	DEBUG((EFI_D_ERROR,"\nResponse CRC error status for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000010)==0x00000010)
	DEBUG((EFI_D_ERROR,"\nNo Response status (After initialization phase) for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000020)==0x00000020)
	DEBUG((EFI_D_ERROR,"\nFATAL_ERROR response code status for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000040)==0x00000040)
	DEBUG((EFI_D_ERROR,"\nNONFATAL_ERROR response code status for sw triggered transaction  (by writing MMIO Rx04h register)\n"));	
	if((espidata&0x00000080)==0x00000080)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel Payload length > Max Payload Size (aligned) Status\n"));	
	if((espidata&0x00000100)==0x00000100)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel Read request size > Max Read Request Size (aligned) Status\n"));	
	if((espidata&0x00000200)==0x00000200)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel (Address + Length) crosses 4KB (aligned) boundary Status\n"));	
	if((espidata&0x00000400)==0x00000400)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel Unsuccessful completion received Status\n"));	
	if((espidata&0x00000800)==0x00000800)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel completion with invalid tag received Status\n"));	
	if((espidata&0x00001000)==0x00001000)
	DEBUG((EFI_D_ERROR,"\nDMA write done status\n"));	
	if((espidata&0x00002000)==0x00002000)
	DEBUG((EFI_D_ERROR,"\nDMA read done status\n"));	
	if((espidata&0x00004000)==0x00004000)
	DEBUG((EFI_D_ERROR,"\nPC_FREE assert status\n"));	
	if((espidata&0x00008000)==0x00008000)
	DEBUG((EFI_D_ERROR,"\nNP_FREE assert status\n"));	
	if((espidata&0x00010000)==0x00010000)
	DEBUG((EFI_D_ERROR,"\nPC_AVAIL assert status\n"));	
	if((espidata&0x00020000)==0x00020000)
	DEBUG((EFI_D_ERROR,"\nNP_AVAIL assert status\n"));	
	if((espidata&0x00040000)==0x00040000)
	DEBUG((EFI_D_ERROR,"\nPC_AVAIL timeout status\n"));	
	if((espidata&0x00080000)==0x00080000)
	DEBUG((EFI_D_ERROR,"\nPC_FREE timeout status\n"));	
	if((espidata&0x00100000)==0x00100000)
	DEBUG((EFI_D_ERROR,"\nData ready status for PIO read\n"));	
	if((espidata&0x00200000)==0x00200000)
	DEBUG((EFI_D_ERROR,"\nESPI SW get ownership status\n"));	
	if((espidata&0x00400000)==0x00400000)
	DEBUG((EFI_D_ERROR,"\nsw triggered Peripheral Channel unexpected completion (completion without non-posted request) received Status\n"));

	if((espidata1&0x00000001)==0x00000001)
	DEBUG((EFI_D_ERROR,"\nGPI receive done status\n"));
	if((espidata1&0x00000002)==0x00000002)
	DEBUG((EFI_D_ERROR,"\nInvalid response code status for vw channel transaction\n"));	
	if((espidata1&0x00000008)==0x00000008)
	DEBUG((EFI_D_ERROR,"\nResponse CRC error status for vw channel transaction\n"));	
	if((espidata1&0x00000010)==0x00000010)
	DEBUG((EFI_D_ERROR,"\nNo Response status (After initialization phase) for vw channel transaction\n"));	
	if((espidata1&0x00000020)==0x00000020)
	DEBUG((EFI_D_ERROR,"\nFATAL_ERROR response code status for vw channel transaction\n"));	
	if((espidata1&0x00000040)==0x00000040)
	DEBUG((EFI_D_ERROR,"\nNONFATAL_ERROR response code status for vw channel transaction\n"));	
	if((espidata1&0x00000080)==0x00000080)
	DEBUG((EFI_D_ERROR,"\nVirtual Wire Channel  Count > Max Virtual Wire Count Status\n"));	
	if((espidata1&0x00000010)==0x00000010)
	DEBUG((EFI_D_ERROR,"\nReceive ERROR FATAL Virtual Wire Status\n"));	
	if((espidata1&0x00000020)==0x00000020)
	DEBUG((EFI_D_ERROR,"\nReceive ERROR NON FATAL Virtual Wire Status\n"));	
	if((espidata1&0x00000040)==0x00000040)
	DEBUG((EFI_D_ERROR,"\nSw triggered vw transaction done on espi bus status\n"));	
	if((espidata1&0x00000080)==0x00000080)
	DEBUG((EFI_D_ERROR,"\nvw transaction done on espi bus status\n"));	
	if((espidata1&0x00002000)==0x00002000)
	DEBUG((EFI_D_ERROR,"\nInvalid response code status for hw auto issued GET_STATUS transaction\n"));	
	if((espidata1&0x00008000)==0x00008000)
	DEBUG((EFI_D_ERROR,"\nResponse CRC error status for hw auto issued GET_STATUS transaction\n"));	
	if((espidata1&0x00010000)==0x00010000)
	DEBUG((EFI_D_ERROR,"\nNo Response status (After initialization phase) for hw auto issued GET_STATUS transaction\n"));	
	if((espidata1&0x00020000)==0x00020000)
	DEBUG((EFI_D_ERROR,"\nFATAL_ERROR response code status for hw auto issued GET_STATUS transaction\n"));	
	if((espidata1&0x00040000)==0x00040000)
	DEBUG((EFI_D_ERROR,"\nNONFATAL_ERROR response code status for hw auto issued GET_STATUS transaction\n"));	
	


	if((espidata2&0x00000002)==0x00000002)
	DEBUG((EFI_D_ERROR,"\nInvalid response code status for flash channel transaction\n"));	
	if((espidata2&0x00000004)==0x00000004)
	DEBUG((EFI_D_ERROR,"\nInvalid cycle type status for flash channel transaction\n"));	
	if((espidata2&0x00000008)==0x00000008)
	DEBUG((EFI_D_ERROR,"\nResponse CRC error status for flash channel transaction\n"));	
	if((espidata2&0x00000010)==0x00000010)
	DEBUG((EFI_D_ERROR,"\nNo Response status (After initialization phase) for flash channel transaction\n"));	
	if((espidata2&0x00000020)==0x00000020)
	DEBUG((EFI_D_ERROR,"\nFATAL_ERROR response code status for flash channel transaction\n"));	
	if((espidata2&0x00000040)==0x00000040)
	DEBUG((EFI_D_ERROR,"\nNONFATAL_ERROR response code status for flash channel transaction\n"));	
	if((espidata2&0x00000080)==0x00000080)
	DEBUG((EFI_D_ERROR,"\nFlash Access Channel Payload length > Max Payload Size Status\n"));	
	if((espidata2&0x00000100)==0x00000100)
	DEBUG((EFI_D_ERROR,"\nFlash Access Channel Read request size > Max Read Request Size Status\n"));	
	if((espidata2&0x00000200)==0x00000200)
	DEBUG((EFI_D_ERROR,"\nFlash erase length is more than 2\n"));	
	if((espidata2&0x00000400)==0x00000400)
	DEBUG((EFI_D_ERROR,"\nflash Channel Unsuccessful completion received Status\n"));	
	if((espidata2&0x00000800)==0x00000800)
	DEBUG((EFI_D_ERROR,"\nflash Channel completion with invalid tag received Status\n"));



	if((espidata3&0x00000100)==0x00000100)
	DEBUG((EFI_D_ERROR,"\nEspi reset# pin is asserted by espi slave status\n"));	
	if((espidata3&0x00000200)==0x00000200)
	DEBUG((EFI_D_ERROR,"\nSw triggered transfer fail  because espi slave assert espi reset# pin\n"));	
	if((espidata3&0x00000400)==0x00000400)
	DEBUG((EFI_D_ERROR,"\nEspi virtual wire channel transfer fail  because espi slave assert espi reset# pin\n"));	
	if((espidata3&0x00000800)==0x00000800)
	DEBUG((EFI_D_ERROR,"\nEspi flash channel transfer fail  because espi slave assert espi reset# pin\n"));	
	if((espidata3&0x00001000)==0x00001000)
	DEBUG((EFI_D_ERROR,"\nSW transparent cycle fail because espi slave assert espi reset# pin\n"));	
	if((espidata3&0x00002000)==0x00002000)
	DEBUG((EFI_D_ERROR,"\nslave de-assert espi reset# pin\n"));	
	if((espidata3&0x00020000)==0x00020000)
	DEBUG((EFI_D_ERROR,"\nInvalid response code status for SW transparent cycle\n"));	
	if((espidata3&0x00040000)==0x00040000)
	DEBUG((EFI_D_ERROR,"\nInvalid cycle type status for SW transparent cycle\n"));	
	if((espidata3&0x00080000)==0x00080000)
	DEBUG((EFI_D_ERROR,"\nResponse CRC error status for SW transparent cycle\n"));	
	if((espidata3&0x00100000)==0x00100000)
	DEBUG((EFI_D_ERROR,"\nNo Response status (After initialization phase) for SW transparent cycle\n"));
	if((espidata3&0x00200000)==0x00200000)
	DEBUG((EFI_D_ERROR,"\nFATAL_ERROR response code status for SW transparent cycle\n"));	
	if((espidata3&0x00400000)==0x00400000)
	DEBUG((EFI_D_ERROR,"\nNONFATAL_ERROR response code status for SW transparent cycle\n"));
	if((espidata3&0x00800000)==0x00800000)
	DEBUG((EFI_D_ERROR,"\nSW transparent cycle unsuccessful completion received Status\n"));	
	if((espidata3&0x01000000)==0x01000000)
	DEBUG((EFI_D_ERROR,"\nSW transparent cycle PC_AVAIL timeout status\n"));
	if((espidata3&0x02000000)==0x02000000)
	DEBUG((EFI_D_ERROR,"\nSW transparent cycle PC_FREE timeout status\n"));
	if((espidata3&0x04000000)==0x04000000)
	DEBUG((EFI_D_ERROR,"\nSW transparent cycle NP_FREE timeout status\n"));	





#if ERSMI_COMPORT_DEBUG_MESSAGES

	DEBUG((EFI_D_ERROR,"\n===================== Exit Error Handling SMI ============================\n"));

	//;Restore Baud Rate for COM Port output	
	//;======================
	//; Restore Baud rate
	//;======================
	//; Enable DLAB
	Buffer8 = IoRead8(0x3FB);
	Buffer8 = Buffer8 |0x80;		
	IoWrite8(0x3FB,Buffer8);	
	//; Restore Baud rate (LSB)
	IoWrite8(0x3F8,(UINT8)(Buffer16 & 0xFF));
	//; Restore Baud rate (MSB)
	IoWrite8(0x3F9,(UINT8)((Buffer16 & 0xFF00) >> 0x8));
	//; Disable DLAB
	Buffer8 = IoRead8(0x3FB);
	Buffer8 = Buffer8 & 0x7F;		
	IoWrite8(0x3FB,Buffer8);
	
#endif	//; ERSMI_COMPORT_DEBUG_MESSAGES

//Clear ESPI Interrupt

	CrbMemoryWrite32(espibase+0x110,0xFFFFFFFF);
	CrbMemoryWrite32(espibase+0x114,0xFFFFFFFF);
	CrbMemoryWrite32(espibase+0x118,0xFFFFFFFF);
	CrbMemoryWrite32(espibase+0x11C,0xFFFFFFFF);

//HYL-20160501-end
*/ //HYL-20160503	
}




//----------------------------------------------------------------------------
//
// Procedure:   CRBChildDispatcher
//
// Description: Chipset Reference Board SMM Child Dispatcher Handler.
//
// Parameters:  SmmImageHandle - EFI Handler
//              *CommunicationBuffer - OPTIONAL
//              *SourceSize - OPTIONAL
//
// Returns:     EFI_STATUS
//
// Modified:
//
// Referrals:   EfiSmmSwDispatch EfiSmmSxDispatch 
//
// Notes:
//  Here is the control flow of this function:
//  1. Read SMI source status registers.
//  2. If source, call handler.
//  3. Repeat #2 for all sources registered.
//

EFI_STATUS
EFIAPI
CRBChildDispatcher(
  IN EFI_HANDLE             SmmImageHandle,
  IN     CONST VOID         *ContextData,            OPTIONAL
  IN OUT VOID               *CommunicationBuffer,    OPTIONAL
  IN OUT UINTN              *SourceSize              OPTIONAL
  )

{
        if (GetCRBErrSmiContext() && (gChipsetSERRNBControl == ERR_SMI)) CRBErrSmiHandler();
    if (GetCRBSmiContext()) CRBSmiHandler();

    return EFI_HANDLER_SUCCESS;
}



EFI_STATUS
EFIAPI
CRBChildDispatcherInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                              Status;
  EFI_HANDLE                              ErrorReportHandle;

  DEBUG((EFI_D_ERROR,"%a()\r\n",__FUNCTION__));

   
  //LGE-20151222 
  GetSetupData();   
  ErrorReportHandle = NULL;
  Status = gSmst->SmiHandlerRegister (CRBChildDispatcher, NULL,  &ErrorReportHandle);   
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

