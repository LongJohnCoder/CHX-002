#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/TimerLib.h>
#include <Ppi/Smbus.h>
#include <PlatformDefinition.h>
#include <CHX002Reg.h>

#define SioBaseAddress   0x290
#define SioIndexPort     (SioBaseAddress + 05)
#define SioDataport      (SioBaseAddress + 06)

VOID InitSioUart1Uart2()
{
	//;EnterCfg Mode
	IoWrite8(0x2E,0x87);
	IoWrite8(0x2E,0x01);
	IoWrite8(0x2E,0x55);
	IoWrite8(0x2E,0x55);
	//;Cfg GPIO
	IoWrite8 (0x2E, 0x07);
	IoWrite8 (0x2F, 0x07); 
	//;Other Cfg
	IoWrite8 (0x2E, 0x25);
	IoWrite8 (0x2F, 0x00); 
	IoWrite8 (0x2E, 0x26);
	IoWrite8 (0x2F, 0x00);
	
	//;Cfg SIO Uart1 LDN=1
	IoWrite8(0x2E,0x07);
	IoWrite8(0x2F,0x01);
    //;IO Base = 0x3F8
	IoWrite8(0x2E,0x60);
	IoWrite8(0x2F,0x03);
	IoWrite8(0x2E,0x61);
	IoWrite8(0x2F,0xF8);
	//;Irq = 0x04 
	IoWrite8(0x2E,0x70);
	IoWrite8(0x2F,0x04);
	//;Other Cfg
	IoWrite8(0x2E,0xF0);
	IoWrite8(0x2F,0x01);  // IVS-20170410 share edge trigger
	IoWrite8(0x2E,0xF1);
	IoWrite8(0x2F,0x50);	
	//;Enable Uart1
	IoWrite8(0x2E,0x30);
	IoWrite8(0x2F,0x01);

	
	//;Cfg SIO Uart2 LDN=2
	IoWrite8(0x2E,0x07);
	IoWrite8(0x2F,0x02);
    //;IO Base = 0x2F8
	IoWrite8(0x2E,0x60);
	IoWrite8(0x2F,0x02);
	IoWrite8(0x2E,0x61);
	IoWrite8(0x2F,0xF8);
	//;Irq = 0x03 
	IoWrite8(0x2E,0x70);
	IoWrite8(0x2F,0x03);
	//;Other Cfg
	IoWrite8(0x2E,0xF0);
	IoWrite8(0x2F,0x01);   // IVS-20170410 share edge trigger
	IoWrite8(0x2E,0xF1);
	IoWrite8(0x2F,0x50);	
	//;Enable Uart2
	IoWrite8(0x2E,0x30);
	IoWrite8(0x2F,0x01);

	//ExitCfg Mode
	IoWrite8(0x2E,0x02);
	IoWrite8(0x2F,0x02);

}



//Ives_20161207
VOID InitSioParallel()   // IVS-20170410 Enable 80Port ,disable LPT
{
	// EnterCfg Mode
	IoWrite8(0x2E,0x87);
	IoWrite8(0x2E,0x01);
	IoWrite8(0x2E,0x55);
	IoWrite8(0x2E,0x55);

	IoWrite8(0x2E,0x07);
	IoWrite8(0x2F,0x03);

	IoWrite8(0x2E,0x60);
	IoWrite8(0x2F,0x03);
	
	IoWrite8(0x2E,0x61);
	IoWrite8(0x2F,0x78);

	IoWrite8(0x2E,0x62);
	IoWrite8(0x2F,0x07);
	
	IoWrite8(0x2E,0x63);
	IoWrite8(0x2F,0x78);
	
	IoWrite8(0x2E,0x70);
	IoWrite8(0x2F,0x07);

	IoWrite8(0x2E,0x74);
	IoWrite8(0x2F,0x03);
	
	IoWrite8(0x2E,0xF0); //Enable Port 80
	IoWrite8(0x2F,0x00);
	
	IoWrite8(0x2E,0x30);//Disable LPT
	IoWrite8(0x2F,0x00);

	
	//ExitCfg Mode
	IoWrite8(0x2E,0x02);
	IoWrite8(0x2F,0x02);

}

//Ives_20161207
VOID InitSioPs2KBMS()
{
	//disable internal keyboard & mouse, and enable LPC Keyboard.
	MmioWrite8(LPC_PCI_REG(0x51),MmioRead8(LPC_PCI_REG(0x51))&(~BIT0)&(~BIT2));   
	MmioWrite8(LPC_PCI_REG(0x59),MmioRead8(LPC_PCI_REG(0x59))|(BIT4)); 

    // super io for keyobard not need to config,but mouse needs.
      
#if 1
	// EnterCfg Mode
	IoWrite8(0x2E,0x87);
	IoWrite8(0x2E,0x01);
	IoWrite8(0x2E,0x55);
	IoWrite8(0x2E,0x55);

	IoWrite8(0x2E,0x07);
	IoWrite8(0x2F,0x06);

	IoWrite8(0x2E,0x70);
	IoWrite8(0x2F,0x0C);

	IoWrite8(0x2E,0x71);
	IoWrite8(0x2F,0x02);

	IoWrite8(0x2E,0xF0);
	IoWrite8(0x2F,0x00);	
		
	IoWrite8(0x2E,0x30);
	IoWrite8(0x2F,0x01);

	//ExitCfg Mode
	IoWrite8(0x2E,0x02);
	IoWrite8(0x2F,0x02);

#endif

}
VOID SioWrite(UINT8 address,UINT8 data)
{
	IoWrite8(SioIndexPort,address);
	IoWrite8(SioDataport,data);
}

UINT8 SioRead(UINT8 address)
{
	IoWrite8(SioIndexPort,address);
	return IoRead8(SioDataport);
}
VOID SetExternalTemperatureSensorCfg()
{
	// Start  Rx00[0]=1;  default = 0x18
	SioWrite(0x00,0x19); 
	//Fan_TAC3-1 Enable
	SioWrite(0x13,0x70);
	//Fan 1,2,3 base clock set 6MHz(default)
	SioWrite(0x14,0xC0);  //FAN_CTL Polarity  bit7 1:High  , 0:Low
	SioWrite(0x55,0x40); //default
	//TMPIN_1 Diode mode; TMPIN_2 Resistor Mode
	SioWrite(0x51,0x11); 
	//SioWrite(0x51,0x38); // set TMPIN1,2,3 to reisistor mode

	//Fan 1, 2 channel(29h,,2Ah)
	SioWrite(0x15,0x80);  //FAN_CTL 1 -> bit7=1:Automatic Operation, bit[5:3]=000(TMPIN1 ( 29h ) ),
	SioWrite(0x16,0x88);  //FAN_CTL 2 -> bit7=1:Automatic Operation, bit[5:3]=001(TMPIN1 ( 2Ah ) ),

	SioWrite(0x40,0x55); // TMPIN1 High Limit  0x55--85 degree
	SioWrite(0x41,0x0F); // TMPIN1 Low  Limit  0x0F--15 degree
	SioWrite(0x42,0x55); // TMPIN2 High Limit  0x55--85 degree
	SioWrite(0x43,0x0F); // TMPIN2 Low  Limit  0x0F--15 degree
	
	
#if 0
	// Extra Vector Register
	SioWrite(0x06,0x40);//go bank2
	// A
	SioWrite(0x11,0xAF);// Enable Vector;Slope Value
	SioWrite(0x10,0x0F);// Start Limit
	SioWrite(0x12,0x00);// Temperature Input
	SioWrite(0x13,0x00);// Positive Slope,Range 00
	// B
	SioWrite(0x15,0xAF);// Enable Vector;Slope Value
	SioWrite(0x14,0x0F);// Start Limit
	SioWrite(0x16,0x00);// Temperature Input
	SioWrite(0x17,0x00);// Positive Slope,Range 00
	// C
	SioWrite(0x19,0xAF);// Enable Vector;Slope Value
	SioWrite(0x18,0x0F);// Start Limit
	SioWrite(0x1A,0x00);// Temperature Input
	SioWrite(0x1B,0x00);// Positive Slope,Range 00
	SioWrite(0x1C,0x32);// Full Limit

	// A
	SioWrite(0x21,0xAF);// Enable Vector;Slope Value
	SioWrite(0x20,0x0F);// Start Limit
	SioWrite(0x22,0x20);// Temperature Input
	SioWrite(0x23,0x00);// Positive Slope,Range 00
	// B
	SioWrite(0x25,0xAF);// Enable Vector;Slope Value
	SioWrite(0x24,0x0F);// Start Limit
	SioWrite(0x26,0x20);// Temperature Input
	SioWrite(0x27,0x00);// Positive Slope,Range 00
	// C
	SioWrite(0x29,0xAF);// Enable Vector;Slope Value
	SioWrite(0x28,0x0F);// Start Limit
	SioWrite(0x2A,0x20);// Temperature Input
	SioWrite(0x2B,0x00);// Positive Slope,Range 00
	SioWrite(0x2C,0x32);// Full Limit

	SioWrite(0x06,0x00);//out bank2
#endif

	// Extra Vector Register
	SioWrite(0x06,0x40);//go bank2
	SioWrite(0x1D,0x10);  // Bank2, 1Dh[7:4]=0001   ---- TMPIN1(29h),TMPIN2(2Ah)
	SioWrite(0x06,0x00);//out bank2
		
	//Fan 1
	SioWrite(0x60,0x0A);  //automatic mode temperature Limit of OFF     10 degree = 0x0Ah
	SioWrite(0x61,0x0F);  //automatic mode temperature Limit of start   15 degree = 0x0Fh
	SioWrite(0x62,0x32);  //automatic mode temperature Limit of FULL Speed   50 degree = 32h
	SioWrite(0x63,0x33);  //Minimum PWM    20%
	SioWrite(0x64,0x2F);  //Slope of PWM
	SioWrite(0x65,0x40); //Temperature Interval set to 0

	//Fan 2
	SioWrite(0x68,0x0A);  //automatic mode temperature Limit of OFF     10 degree = 0x0Ah
	SioWrite(0x69,0x0F);  //automatic mode temperature Limit of start   15 degree = 0x0Fh
	SioWrite(0x6A,0x32);  //automatic mode temperature Limit of FULL Speed   50 degree = 32h
	SioWrite(0x6B,0x33);  //Minimum PWM    20%
	SioWrite(0x6C,0x2F);  //Slope of PWM
	SioWrite(0x6D,0x40); //Temperature Interval set to 0

	//IVS-20181030 Offset Adjustment For THERMD[+|-],SIO need to decrease 10 degree
	SioWrite(0x5C,SioRead(0x5C)|BIT7); //BIT7 need set to 1 to enable 0x56 Write Access.
	SioWrite(0x56,0xF4); // F4 is 10's complement value.

}

VOID InitVoltageMonitor()
{
	SioWrite(0x50,0xFF); // ADC Voltage Channel Enable,
#if 1
	SioWrite(0x30,0xFF); //VIN0 High
	SioWrite(0x31,0x00); //VIN0 Low
	SioWrite(0x32,0xFF); //VIN1 High
	SioWrite(0x33,0x00); //VIN1 Low
	SioWrite(0x34,0xFF); //VIN2 High
	SioWrite(0x35,0x00); //VIN2 Low
	SioWrite(0x36,0xFF); //VIN3 High
	SioWrite(0x37,0x00); //VIN3 Low
	SioWrite(0x38,0xFF); //VIN4 High
	SioWrite(0x39,0x00); //VIN4 Low
	SioWrite(0x3A,0xFF); //VIN5 High
	SioWrite(0x3B,0x00); //VIN5 Low
	SioWrite(0x3C,0xFF); //VIN6 High
	SioWrite(0x3D,0x00); //VIN6 Low
	SioWrite(0x3E,0xFF); //VIN7 High
	SioWrite(0x3F,0x00); //VIN7 Low
#endif

}
VOID InitSioEnvironment()
{
	// EnterCfg Mode
	IoWrite8(0x2E,0x87);
	IoWrite8(0x2E,0x01);
	IoWrite8(0x2E,0x55);
	IoWrite8(0x2E,0x55);

	IoWrite8(0x2E,0x07);
	IoWrite8(0x2F,0x04);

	IoWrite8(0x2E,0x60);
	IoWrite8(0x2F,0x02);
	
	IoWrite8(0x2E,0x61);
	IoWrite8(0x2F,0x90);

	IoWrite8(0x2E,0x62);
	IoWrite8(0x2F,0x02);
	
	IoWrite8(0x2E,0x63);
	IoWrite8(0x2F,0x30);
	
	IoWrite8(0x2E,0x70);
	IoWrite8(0x2F,0x06);  // interrupt routing irq6

	IoWrite8(0x2E,0xF0);
	IoWrite8(0x2F,0x00); // In S0,disalbe kb/ms wakeup event.

	IoWrite8(0x2E,0xF1);
	IoWrite8(0x2F,0xFF); // clear status.
	
	IoWrite8(0x2E,0xF2);
	IoWrite8(0x2F,0x44);// In S0,PME# OutPut Control is disable.
	
	IoWrite8(0x2E,0xF3);
	IoWrite8(0x2F,0x01); //irq  normal mode ,high edge trigger 
	
	IoWrite8(0x2E,0xF4);
	IoWrite8(0x2F,0x60);

	IoWrite8(0x2E,0xFB);
	IoWrite8(0x2F,0x30); // one click of left or right key
	
	IoWrite8(0x2E,0x30);
	IoWrite8(0x2F,0x01);
		
	//ExitCfg Mode
	IoWrite8(0x2E,0x02);
	IoWrite8(0x2F,0x02);

	SetExternalTemperatureSensorCfg();
	InitVoltageMonitor();

}






VOID SioInit()
{
#if defined(HX002EH0_01)||defined(HX002EL0_05)

// For SuperIO Uart---Ives add 201703071752
  InitSioUart1Uart2();
// For SuperIO Keyboard & mouse---Ives add 201612071041
  InitSioPs2KBMS();   
// For SuperIO 80Port---Ives add 201612071041
  InitSioParallel();
// For SuperIO Environment Controller
  InitSioEnvironment();
  
#endif
}
