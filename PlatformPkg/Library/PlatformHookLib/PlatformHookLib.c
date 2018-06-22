
#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <PlatformDefinition.h>


#define SIO_Index_Port 0x4E
#define SIO_Data_Port SIO_Index_Port + 1

#define VART_BRL_REG		0x00	// baud rate low byte register
#define VART_BRH_REG		0x01	// baud rate high byte register
#define VART_IER_REG	       0x01	// interrupt enable register
#define VART_FCR_REG		0x02	// FIFO control register
#define VART_LCR_REG		0x03	// line control register
#define VART_MCR_REG		0x04	// modem control register


EFI_STATUS  SetUartHostCtrlLegacyReg(IN UINT16 BaseAddr)
{
     UINT8 TmpVal;
      
     //set LCR[7] to 0
     TmpVal = IoRead8(BaseAddr + VART_LCR_REG);
     TmpVal = TmpVal & 0x7f;  //set DLAB=0, bit 7 in LCR
     IoWrite8(BaseAddr + VART_LCR_REG, TmpVal);
  	
  	 //disable interrupts
     IoWrite8(BaseAddr + VART_IER_REG, 0);   //disable interrupt
		
     //set LCR[7] to 1 to set the divisor reg
     TmpVal = IoRead8(BaseAddr + VART_LCR_REG);
     TmpVal |= 0x80;
     IoWrite8(BaseAddr + VART_LCR_REG, TmpVal);
	
     //set divisor reg low, that is setting baud rate to 115200  
     IoWrite8(BaseAddr + VART_BRL_REG,0x01);
     //set divisor reg high
     IoWrite8(BaseAddr + VART_BRH_REG,0);
  
     //set LCR[7] to 0
     TmpVal = IoRead8(BaseAddr + VART_LCR_REG);
     TmpVal = TmpVal&0x7f;  //set DLAB=0, bit 7 in LCR
     IoWrite8(BaseAddr + VART_LCR_REG,TmpVal);
  
     //set LCR to 03 to select the frame format
     IoWrite8(BaseAddr + VART_LCR_REG,0x03);
 
     //set the FCR, FIFO Control Reg
     IoWrite8(BaseAddr + VART_FCR_REG,0x07);
  
     // set MCR 
     IoWrite8(BaseAddr + VART_MCR_REG,0x00);
	 
     return EFI_SUCCESS;
}


EFI_STATUS LpcUart_Init()
{
    UINT8 tmpval;
    UINT16 BaseAddr;
    
    // ;Enter config mode
    //;The config string is 83h/05h/55h/55h
    IoWrite8 (SIO_Index_Port, 0x83);
    IoWrite8 (SIO_Index_Port, 0x05);
    IoWrite8 (SIO_Index_Port, 0x55);
    IoWrite8 (SIO_Index_Port, 0x55);
    
    //;Now in configuration mode
    //;Select clock to 48MHz by clear Rx29[0]
    IoWrite8 (SIO_Index_Port, 0x29);
    tmpval = IoRead8(SIO_Data_Port);
    tmpval &= 0xFE;
    IoWrite8 (SIO_Data_Port, tmpval);
    
    //;Select LDN3
    IoWrite8 (SIO_Index_Port, 0x07);
    IoWrite8 (SIO_Data_Port, 0x03); 

    //;Set base address to 3F8h
    IoWrite8 (SIO_Index_Port, 0x60);
    IoWrite8 (SIO_Data_Port, 0x03);
    IoWrite8 (SIO_Index_Port, 0x61);
    IoWrite8 (SIO_Data_Port, 0xF8);

    //;Set IRQ=04h
    IoWrite8 (SIO_Index_Port, 0x70);
    IoWrite8 (SIO_Data_Port, 0x04);
    
    //;Enable UART
    IoWrite8 (SIO_Index_Port, 0x30);
    IoWrite8 (SIO_Data_Port, 0x01);

    //;Exit config mode by writing double 02
    IoWrite8 (SIO_Index_Port, 0x02);
    IoWrite8 (SIO_Index_Port, 0x02);
	
    BaseAddr = 0x3F8;		
    SetUartHostCtrlLegacyReg(BaseAddr);
		
    return EFI_SUCCESS;
  }

EFI_STATUS VT6576B_SIOUart_Init()
{
	UINT16 BaseAddr;
	
    //  VT6578B Uart Code.We can use 80Port which contain a COM port for Debug log Output.
    //  But it is necessary to disalbe ZX-internal uart0 beacuse there are resource conflicts problem.
    
    // ;Enter config mode by writting 0x77 to index port twice
    IoWrite8 (SIO_Index_Port, 0x77);
    IoWrite8 (SIO_Index_Port, 0x77);
    
    //;Select LDN=0,uart1
    IoWrite8 (SIO_Index_Port, 0x07);
    IoWrite8 (SIO_Data_Port, 0x00); 

    //;Set base address to 2F8h
    IoWrite8 (SIO_Index_Port, 0x60);
    IoWrite8 (SIO_Data_Port, 0x02);
    IoWrite8 (SIO_Index_Port, 0x61);
    IoWrite8 (SIO_Data_Port, 0xF8);

    //;Set IRQ=03h
    IoWrite8 (SIO_Index_Port, 0x70);
    IoWrite8 (SIO_Data_Port, 0x03);
    
    //;Enable UART1
    IoWrite8 (SIO_Index_Port, 0x30);
    IoWrite8 (SIO_Data_Port, 0x01);

    //;Exit config mode by writing 0xAA to Index Port.
    IoWrite8 (SIO_Index_Port, 0xAA);
	
    BaseAddr = 0x2F8;		
    SetUartHostCtrlLegacyReg(BaseAddr);
		
    return EFI_SUCCESS;
}


EFI_STATUS PciUart_Init()
{

 ///CND001-R01-MTN-01-DEL-20141118-1100
  //UINT16  DataSave16;
  ///UINT8   DataSave8;

  ///if(MmioRead8(LPC_PCI_REG(LPC_APIC_FSB_DATA_CTRL_REG)) & PCI_UART0_EN){     // UART0 Enabled, Init before, exit
  ///  goto ProcExit;
  ///}

  ///DataSave16 = MmioRead16(DRAM_PCI_REG(CPU_DAFBC_REG));
  ///DataSave8  = MmioRead8(DRAM_PCI_REG(D0F3_MISC_CTRL_REG));
  
  ///MmioWrite16(DRAM_PCI_REG(CPU_DAFBC_REG), 0x8000);     // default
  ///MmioWrite8(DRAM_PCI_REG(D0F3_MISC_CTRL_REG), 0x70);   // default  
  ///MmioOr8(IGD_PCI_REG(PCI_CMD_REG), PCI_CMD_IO_EN);
  
// VGA Enable, 3C3[0]=1, 3C2 = 67H, CR38 = 48H , CR39 = A5H, SR08 = 06H
  ///IoAndThenOr8(0x03C3, 0xFF, 0x01);
  ///IoWrite8(0x3C2, 0x67);
  ///IoWrite16(0x3D4, 0x4838);
  ///IoWrite16(0x3D4, 0xA539);
  ///IoWrite16(0x3C4, 0x0608);

  ///MmioAndThenOr8(LPC_PCI_REG(LPC_APIC_FSB_DATA_CTRL_REG), (UINT8)~UART_SHARE_PCIPAD_ONLY, PCI_UART0_EN);///R48h
  ///MmioWrite8(LPC_PCI_REG(LPC_UART0_BASE_ADDR_REG), UART0_LEG_MODE_EN|(0x3F8>>3));///RxB2h
  ///MmioAndThenOr8(LPC_PCI_REG(LPC_UART_IRQR_LOW_REG), (UINT8)~PCI_UART0_IRQR_MASK, 4);///RxB2h
  ///MmioOr8(LPC_PCI_REG(LPC_APIC_C4P_CTRL_REG), UART_MUX_DVP_PAD_EN);
//MmioOr8(LPC_PCI_REG(LPC_PCI_CPU_IF_CTRL_REG), PCI_DEBUG_MODE_EN);
  
  ///IoWrite8(0x3C4, 0x32);
  ///IoWrite8(0x3C5, IoRead8(0x3C5)|BIT4);

  ///MmioAnd8(IGD_PCI_REG(PCI_CMD_REG), (UINT8)~PCI_CMD_IO_EN); 
  ///MmioWrite16(DRAM_PCI_REG(CPU_DAFBC_REG), DataSave16);
  ///MmioWrite8(DRAM_PCI_REG(D0F3_MISC_CTRL_REG), DataSave8);
  ///CND001-R01-MTN-01-END-20141118-1100
  
 //CND001-R01-MTN-01-ADD-20141118-1100, PCI UART Chipset Initialize
 //UINT8 Buffer8;
 //UINT16 Buffer16;
 UINT32 Buffer32;

 //Enable 8Pin UART0
 //Buffer16=IoRead16(0x800+PMIO_PAD_CTL_REG_Z1);
 //Buffer16&=~(0x07);
 //Buffer16|=0x04;
 //IoWrite16(0x800+PMIO_PAD_CTL_REG_Z1,Buffer16);
 //Buffer32=IoRead32(0x800 + PMIO_CR_GPIO_PAD_CTL);
 //Buffer32&=~(0x3F000000);
 //Buffer32|=0x13000000;
 //IoWrite32(0x800 + PMIO_CR_GPIO_PAD_CTL,  Buffer32);
 //Buffer32=IoRead32(0x800 + PMIO_GPIO_PAD_CTL);
 //Buffer32&=~(0x3F3F0007);
 //Buffer32|=0x09090002;
 //IoWrite32(0x800 + PMIO_GPIO_PAD_CTL,  Buffer32);
 
 //Enable 4Pin UART0
 //Buffer16=IoRead16(0x800+PMIO_PAD_CTL_REG_Z1);
 //Buffer16&=~(0x07);
 //Buffer16|=0x04;
 //IoWrite16(0x800+PMIO_PAD_CTL_REG_Z1,Buffer16);
 //Buffer32=IoRead32(0x800 + PMIO_CR_GPIO_PAD_CTL);
 //Buffer32&=~(0x3F000000);
 //Buffer32|=0x13000000;
 //IoWrite32(0x800 + PMIO_CR_GPIO_PAD_CTL,  Buffer32);
 //Buffer32=IoRead32(0x800 + PMIO_GPIO_PAD_CTL);
 //Buffer32&=~(0x00000007);
 //Buffer32|=0x00000002;
 //IoWrite32(0x800 + PMIO_GPIO_PAD_CTL,  Buffer32);
 
 //Enable 4Pin UART1
 Buffer32=IoRead32(0x800 + PMIO_GPIO_PAD_CTL);
 Buffer32&=~(0x3F3F0000);
 Buffer32|=0x242D0000;
 IoWrite32(0x800 + PMIO_GPIO_PAD_CTL,  Buffer32);

 //Uart0
 //MmioAndThenOr8(LPC_PCI_REG(LPC_UART_IRQR_LOW_REG), (UINT8)~PCI_UART0_IRQR_MASK, 4);///RxB2h
 //MmioAndThenOr8(LPC_PCI_REG(LPC_UART0_BASE_ADDR_REG), (UINT8)~0x7F, 0x7F);///RxB3h
 //MmioAndThenOr8(LPC_PCI_REG(LPC_UART0_BASE_ADDR_REG), (UINT8)~0x80, 0x80);///RxB3h
 //MmioAndThenOr8(LPC_PCI_REG(LPC_APIC_FSB_DATA_CTRL_REG), (UINT8)~PCI_UART0_EN, PCI_UART0_EN);///R48h
 //SetUartHostCtrlLegacyReg(0x03F8);
 
 //Uart1
 MmioAndThenOr8(LPC_PCI_REG(LPC_UART_IRQR_LOW_REG), (UINT8)~PCI_UART1_IRQR_MASK, 0x30);///RxB2h
 MmioAndThenOr8(LPC_PCI_REG(LPC_UART1_BASE_ADDR_REG), (UINT8)~0x7F, 0xDF);///RxB4h
 MmioAndThenOr8(LPC_PCI_REG(LPC_UART1_BASE_ADDR_REG), (UINT8)~0x80, 0x80);///RxB4h
 MmioAndThenOr8(LPC_PCI_REG(LPC_APIC_FSB_DATA_CTRL_REG), (UINT8)~PCI_UART1_EN, PCI_UART1_EN);///R48h
 SetUartHostCtrlLegacyReg(0x02F8);

 return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
	// For CHX002 EVBs, UART0's pin mux with PCIE-RST, so default use onboard UART1 as DEBUG BIOS's log output port.
	// UART0 not initialize anymore, keep related registers' default setting. 
	// IOBASE setting ---- UART0 : 3F8h ; UART1 : 2F8h ; UART2 : 3E8h ; UART3:2E8h 
	// We can choose BIOS Debuging port by changing PcdBiosDebugUsePciUart
	//       TRUE  : Internal Uart1  (Default)
	//       FALSE : LPC Uart VT6576B
    // Both of them use the same IO Base address = 0x2F8 and irq number = 3.
    // When using LPC Uart VT6576B(PcdBiosDebugUsePciUat==FALSE),Internal Uart will auto be disable.
	
	if(PcdGetBool(PcdBiosDebugUsePciUart) == TRUE) {
		PciUart_Init();
	} else {
		//LpcUart_Init();
		VT6576B_SIOUart_Init();
	}
  return EFI_SUCCESS;
}


