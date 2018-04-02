
#include <PlatformDefinition2.h>

#ifndef MDEPKG_NDEBUG      // debug mode
#define ASL_COM(x)   DBGC x
#else
#define ASL_COM(x)
#endif

DefinitionBlock ("SSDT.aml", "SSDT", 2, "_BYO_ ", "ZX_SPIC", 0x00000001)
{
  External (\_SB.PCI0.SBRG.SPIC, DeviceObj)

  Device (\_SB.PCI0.SBRG.SPIC) //SPI Host Controller
  { 
	  //Name (_ADR, 0)
	  Name(_HID,  EisaId("SPI0001")) 
	  //Name (_CID, "PNP0001")
	  Name (_UID, 1)
						
	  Method (_STA, 0, NotSerialized) 
	  {
		  Return (0x0B)
	  }
						
	  Method (_CRS, 0, Serialized)
	  {
		  Name (BUF0, ResourceTemplate ()
		  {
			  Memory32Fixed (ReadWrite,
							 0xFEB30000,	  // Address Base	==> mmio_bas_add	  
							 0x00000100,	  // Address Length
							)
			  IRQ (Level, ActiveLow, Shared, ) //IRQ Number   ==> irq_no 
			  {5}
		  })
				 
		  Return (BUF0)
	  } 	 
	  
	  Device (SLV1)//SPI Slave Device 1
	  { 	   
		  //Name (_ADR, 8)
		  Name(_HID, EisaId("SPI0002"))// ==> device name 
		  Name(_STR, Unicode("sst26vf064b"))
		  Name (BUF1, ResourceTemplate ()
		  {
			  SPISerialBus (0,						  //Device selection ==> chip_select	
							PolarityLow,			  //Device selection polarity
							FourWireMode,			  //WireMode
							8,						  //DataBitLength
							ControllerInitiated,	  //SlaveMode
							1000,					  //ConnectionSpeed ==> max_speed_hz
							ClockPolarityHigh,		  //ClockPolarity
							ClockPhaseSecond,		  //ClockPhase
							"\\_SB.PCI0.SPIC", 
							0,
							ResourceConsumer,
							,
							)
		  })	 
	  
		  Method (_STA, 0, NotSerialized) 
		  {
			  Return (0x0B)
		  }
						 
		  Method (_CRS, 0, Serialized)
		  {
			  Return (BUF1)
		  }
	  }   
	  
	  Device (SLV2)//SPI Slave Device 2
	  { 	   
		  //Name (_ADR, 8)
		  Name(_HID, EisaId("SPI0003"))  // ==> device name 
		  Name(_STR, Unicode("sst26vf064b")) 
		  Name (BUF1, ResourceTemplate ()
		  {
			  SPISerialBus (  1,								  //Device selection ==> chip_select	
							  PolarityLow,						  //Device selection polarity
							  FourWireMode, 					  //WireMode
							  8,								  //DataBitLength
							  ControllerInitiated,				  //SlaveMode
							  1000, 							  //ConnectionSpeed ==> max_speed_hz
							  ClockPolarityHigh,				  //ClockPolarity
							  ClockPhaseSecond, 				  //ClockPhase
							  "\\_SB.PCI0.SPIC", 
							  0,
							  ResourceConsumer,
							  ,
							  )
		  })	 
	  
		  Method (_STA, 0, NotSerialized) 
		  {
			  Return (0x0B)
		  }
						
		  Method (_CRS, 0, Serialized)
		  {
	  
					 
			 Return (BUF1)
		  }
	  } 

  }
}


