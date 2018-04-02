
#include <PlatformDefinition2.h>

	include("HX001EK0_09_IOE_3C\pcitreeioe.asl")
	Device(B100) { // IOE SwitchUpBridge B1D0F0
		Name(_ADR, 0x00000000)
		Method(_PRT,0) {
			If(PICM) { Return(ARX2) }// APIC mode
			Return (PRX2) // PIC Mode
		} // end _PRT

#if IOE_SHOW_XBCTL
		Device(B200) { // IOE XBCTL B2D0F0
			Name(_ADR, 0x00000000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
		} // end "IOE XBCTL B2D0F0"
#endif

		Device(B210) { // IOE RC B2D1F0
			Name(_ADR, 0x00010000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			Method(_PRT,0) {
				If(PICM) { Return(ARX3) }// APIC mode
				Return (PRX3) // PIC Mode
			} // end _PRT

			Device(I10S) { // IOE RC B2D1F0 Slot
				Name(_ADR, 0x00000000)
				Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			} // end "IOE RC B2D1F0 Slot"
		} // end "IOE RC B2D1F0"

		Device(B220) { // IOE RC B2D2F0
			Name(_ADR, 0x00020000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			Method(_PRT,0) {
				If(PICM) { Return(ARX4) }// APIC mode
				Return (PRX4) // PIC Mode
			} // end _PRT
			//jenny add for hotplug 20170515-s
#if defined(PCAL6416A_PCIE_HOTPLUG_SUPPORT_IOE)
			//
			// this method called by _EJ01 method
			//
			Method(EJ01, 1, NotSerialized)
			{
			 // DBGC(1, "JNY_Method EJ01",0)
			  Sleep(300)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)   
			  And(Local3, 0xF9, Local3)
			  \_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
			  Or(Local3, 0x02, Local3)
			  \_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x02), Local3)
			  And(Local3, 0x36, Local3)
			  Or(Local3, 0x40, Local3)
			  \_SB.PCI0.SMBC.SMWB(0x40, 0x02, Local3)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
			  And(Local3, 0xFE, Local3)
			  \_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
			  And(Local3, 0xF9, Local3)
			  Or(Local3, 0x02, Local3)
			  \_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3)
			  Return(Zero)
			}																	
			//
			//this method called by _L0B method
			//
			Method(HPEH, 1, NotSerialized){					
	 //Yankui-Add-S
	 //Usage of GPIO bits
	 //PC00  Input port0
	 //   1  Att Button
	 //   2  Presence Detect
	 //   4  Fault
	 //PC02  Output port0
	 //   0  AUX On
	 //   3  ON
	 //   6  Power LED(LED3)
	 //   7  CKPWRGD_PD# to CLK Buffer
	 //PC03  Output port1
	 //   0  PERST# of Slot
	 //   1  Att LED(LED2)
	 //   2  Att LED(LED2)
	 //PC03.2	PC03.1	 LED2 mode
	 //  0		 0		  BLINK
	 //  1		 x		  ON
	 //  0		 1		  OFF
	 //PC4C  Port0 Interrupt Status
	 //Yankui-Add-E
			  Name(PC00, Zero)
			  Name(PC02, Zero)
			  Name(PC03, Zero)
			  Name(PC4C, Zero)

	//Jennychen add for IOE Hot plug path start
			OperationRegion (PE0L, SystemMemory, 0xE0210000, 0x1000)   //declare E0210000 is IOE D2F0
			Field (PE0L, ByteAcc,NoLock,Preserve) {
			Offset(0x1E8),
				 , 1,
				 PDL2, 1,
				 , 5,
				 IDPD, 1
				}
    //Jennychen add for Hot plug path end	 
			  Sleep(300)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4C), PC4C)	 // Interrupt status port 0 register
			  Sleep(400)
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x02), PC02)	 // Output port 0 register 
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), PC03)	 // Output port 1 register
			  Store(\_SB.PCI0.SMBC.SMRB(0x40, Zero), PC00)	 // Input port 0 register
			  
			  //DBGC(2, "4C:", PC4C)
			  //DBGC(2, "02:", PC02)
			  //DBGC(2, "03:", PC03)	
			  //DBGC(2, "00:", PC00)					  
			  
			  And(PC00, 0x04, Local0)				 // I0.2
			  And(PC03, 0x06, Local1)				 // O1.1, O1.2
			  And(PC4C, 0x04, Local2)				 // S0.2
			  
			  // I0.2 == 0 && O1.1 == 1 && S0.2 == 1
			  // Card is just plugging
			  If(LAnd(LEqual(Local0, Zero), LAnd(LEqual(Local1, 0x02), LEqual(Local2, 0x04)))){
				//DBGC(1, "JNY_JustPlugging",0)
				Sleep(300)
				
				//Jenny add for IOE hot plug path start
				
				Store(0x00, PDL2)
				Store(0x00, IDPD)
				Sleep(100)
				
				//Jenny add for IOE hot plug path end
				
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				And(Local3, 0xF9, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3) 		  // clear LED2, OFF-->Blinking
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				And(Local3, 0xFD, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3) 		  // clear "AttButton" mask
				Return(Zero)
			  }
			  
			  // I0.2 == 1 && O1.1 == O1.2 == 0 && S0.2 == 1, "remove"
			  If(LAnd(LEqual(Local0, 0x04), LAnd(LEqual(Local1, Zero), LEqual(Local2, 0x04)))){
				//DBGC(1, "JNY_Remove",0)
				Sleep(300)
				
			 //Jenny add for IOE hot plug path start
				
				Store(0x00, PDL2)
				Store(0x00, IDPD)
				Sleep(100)
				
			 //Jenny add for IOE hot plug path end
			 
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				And(Local3, 0xF9, Local3)				  // LED2
				Or(Local3, 0x02, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				Return(Zero)
			  }
			  
			  And(PC00, 0x02, Local0)					  // I0.1
			  And(PC03, 0x06, Local1)					  // O1.1, O1.2
			  And(PC4C, 0x02, Local2)					  // S0.1
			  //
			  If(LAnd(LEqual(Local0, Zero), LAnd(LEqual(Local1, Zero), LEqual(Local2, 0x02)))){
				//DBGC(1, "JNY_PluggingPress",0)
				Sleep(300)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				Or(Local3, 0x02, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3) 		  // mask button
				Sleep(5000)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, Zero), Local3)
				And(Local3, 0x02, Local3)
				If(LEqual(Local3, Zero)) {
				  //DBGC(1, "JNY_5s",0)
				  Sleep(300)
				  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				  And(Local3, 0xF9, Local3)
				  Or(Local3, 0x02, Local3)
				  \_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				  And(Local3, 0xFD, Local3)
				  \_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				  Return(Zero)
				}

				Sleep(300)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x02), Local3)
				And(Local3, 0x36, Local3)
				Or(Local3, 0x89, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x02, Local3)

				Sleep(800) //Yankui-add

				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				And(Local3, 0xF8, Local3)
				Or(Local3, 0x05, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				And(Local3, 0xFD, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3)
               //Yankui-add -S 
               Sleep(2000) 
				OperationRegion (PEGL, SystemMemory, 0xE0210000, 0x1000)   //declare E002_0000 is D4F0
				Field (PEGL, ByteAcc,NoLock,Preserve) {
				Offset(0x1C3),
					LTM, 8		//PHYLS LTSSM
				}
				Store(LTM, Local3)
				//Notify(H000, Zero)
				//DBGC(2,"JNY0",Local3)

				Notify(\_SB.PCI0.NPEG.B100.B220.H000, 0x0)

				//DBGC(2,"JNY1",Local3)

				Return(Zero)
			  }
			  
			  And(PC00, 0x02, Local0)
			  And(PC03, 0x04, Local1)
			  And(PC4C, 0x02, Local2)
			  If(LAnd(LEqual(Local0, Zero), LAnd(LEqual(Local1, 0x04), LEqual(Local2, 0x02))))
			  {
				//DBGC(1, "JNY_RemovalPress",0)
				Sleep(300)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				And(Local3, 0xF9, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				Or(Local3, 0x02, Local3)
				\_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3)
				Sleep(0x1388)
				Store(\_SB.PCI0.SMBC.SMRB(0x40, Zero), Local3)
				And(Local3, 0x02, Local3)
				If(LEqual(Local3, Zero))
				{
				
				  Store(0xD1, DBG8)
				  Sleep(300)
				  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x03), Local3)
				  And(Local3, 0xF9, Local3)
				  Or(Local3, 0x04, Local3)
				  \_SB.PCI0.SMBC.SMWB(0x40, 0x03, Local3)
				  Store(\_SB.PCI0.SMBC.SMRB(0x40, 0x4A), Local3)
				  And(Local3, 0xFD, Local3)
				  \_SB.PCI0.SMBC.SMWB(0x40, 0x4A, Local3)
				  Return(Zero)
				}
				Store(0xD2, DBG8)
				Sleep(300)
				Notify(H000, 0x03)
				Return(Zero)
			  }

			  Sleep(300)
			  Return(Zero)
			}

			Name(^HP01, Package(4) {
			  0x08, 	// CacheLineSize in unit DWORDS 
			  0x40, 	// LatencyTimer in PCI clocks
			  One,		// Enable SERR (Boolean) 
			  Zero		// Enable PERR (Boolean)
			  })
			Method(_HPP, 0, NotSerialized){    // Hot Plug Parameters
			  Return(HP01)
			}
			
			Device(H000)
			{
			  Name(_ADR, Zero)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H001)
			{
			  Name(_ADR, One)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H002)
			{
			  Name(_ADR, 0x02)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H003)
			{
			  Name(_ADR, 0x03)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H004)
			{
			  Name(_ADR, 0x04)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H005)
			{
			  Name(_ADR, 0x05)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H006)
			{
			  Name(_ADR, 0x06)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
			Device(H007)
			{
			  Name(_ADR, 0x07)
			  Name(_SUN, One)
			  Method(_EJ0, 1, NotSerialized)
			  {
				EJ01(Arg0)
			  }
			}
#else
      Device(I20S) { // IOE RC B2D2F0 Slot
      Name(_ADR, 0x00000000)
      Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
   }     // end "IOE RC B2D2F0 Slot"
#endif
		//jenny add for hotplug 20170515-E
}       //end "IOE RC B2D2F0"

		Device(B230) { // IOE RC B2D3F0
			Name(_ADR, 0x00030000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			Method(_PRT,0) {
				If(PICM) { Return(ARX5) }// APIC mode
				Return (PRX5) // PIC Mode
			} // end _PRT

			Device(I30S) { // IOE RC B2D3F0 Slot
				Name(_ADR, 0x00000000)
				Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			} // end "IOE RC B2D3F0 Slot"
		} // end "IOE RC B2D3F0"

		Device(B240) { // IOE RC B2D4F0
			Name(_ADR, 0x00040000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			Method(_PRT,0) {
				If(PICM) { Return(ARX6) }// APIC mode
				Return (PRX6) // PIC Mode
			} // end _PRT

			Device(I40S) { // IOE RC B2D4F0 Slot
				Name(_ADR, 0x00000000)
				Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			} // end "IOE RC B2D4F0 Slot"
		} // end "IOE RC B2D4F0"

		Device(B250) { // IOE RC B2D5F0
			Name(_ADR, 0x00050000)
			Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
			Method(_PRT,0) {
				If(PICM) { Return(ARX7) }// APIC mode
				Return (PRX7) // PIC Mode
			} // end _PRT

			Device(PTPB) { // ITE PCIE TO PCI Bridge
			Name(_ADR, 0x00000000)
			Method(_PRT,0) {
			     If(PICM) { Return(ARXC) }// APIC mode
			     Return (PRXC) // PIC Mode
			     
			     Device(ITES) { // ITE PCIE TO PCI Slot
				     Name(_ADR, 0x00000000)
				     Method(_PRW, 0) { Return(GPRW(0x10, 4)) } // can wakeup from S4 state
			    }// end "ITE PCIE TO PCI Slot"
			  }// end _PRT
		     } //end " ITE PCIE TO PCI Bridge"
	      }// end "IOE RC B2D5F0"

		Device(B280) { // IOE Virt SW DN Port B2D8F0
			Name(_ADR, 0x00080000)
			Method(_PRT,0) {
				If(PICM) { Return(ARXA) }// APIC mode
				Return (PRXA) // PIC Mode
			} // end _PRT

			Device(BA00) { // IOE PCIE2PCI Bridge B10D0F0
				Name(_ADR, 0x00000000)
				Method(_PRT,0) {
				If(PICM) { Return(ARXB) }// APIC mode
				Return (PRXB) // PIC Mode
			} // end _PRT


#if IOE_SHOW_EPTRFC
					Device(EPTR) { // IOE EPTRFC B11D0F0
						Name(_ADR, 0x00000000)
						Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
					} // end "IOE EPTRFC B11D0F0"
#endif
				
					Device(GNIC) { // IOE GNIC 
						Name(_ADR, 0x000D0000)
						Name(_S0W, 3)						
						Name(_PRW,Package(){0x10, 0x4})	
					} // end "IOE GNIC B11D13F0"

					Device (USB0){
		            	Name (_ADR, 0x00100000)  
		                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
		                {
		                	Return (0x03)
		                }
		                Method (_PRW, 0, NotSerialized) 
		                {
		                	if(LEqual(\_SB.PCI0.NBF6.US4C, One))
		                    {
		                    	Return (GPRW (0x10, 4))
		                	} else {
		                        Return (GPRW (0x10, 3))
		                    }
		                }
		            }

					Device (USB1){
		                Name (_ADR, 0x00100001)  
		                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
		                {
		                	Return (0x03)
		                }
		                Method (_PRW, 0, NotSerialized) 
		                {
			                if(LEqual(\_SB.PCI0.NBF6.US4C, One))
			                {
			                	Return (GPRW (0x10, 4))
			                } else {
			                	Return (GPRW (0x10, 3))
			                }
		                }
		            }

					Device (USB2){
		                Name (_ADR, 0x00100002)  
		                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
		                {
		                     Return (0x03)
		                }
		                Method (_PRW, 0, NotSerialized) 
		                {
		                     if(LEqual(\_SB.PCI0.NBF6.US4C, One))
		                     {
		                     	 Return (GPRW (0x10, 4))
		                     } else {
		                         Return (GPRW (0x10, 3))
		                     }
		                }
		            }

					Device (EHC1){
		                Name (_ADR, 0x00100007)  
		                Method (_PRW, 0, NotSerialized) 
		                {
		                     if(LEqual(\_SB.PCI0.NBF6.US4C, One))
		                     {
		                         Return (GPRW (0x10, 4))
		                     } else {
		                         Return (GPRW (0x10, 3))
		                     }
		                }
		             }
					
		            Device (USB3){
		                Name (_ADR, 0x000E0000)  
		                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
		                {
		                	Return (0x03)
                             }
                             Method (_PRW, 0, NotSerialized) 
                             {
                             if(LEqual(\_SB.PCI0.NBF6.US4C, One))
                             {
                              Return (GPRW (0x10, 4))
                             } else {
                               Return (GPRW (0x10, 3))
                               }
                             }
                         }
                         Device (USB4){
                             Name (_ADR, 0x000E0001)  
                             Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                             {
                               Return (0x03)
                             }
                             Method (_PRW, 0, NotSerialized) 
                             {
                             if(LEqual(\_SB.PCI0.NBF6.US4C, One))
                             {
                              Return (GPRW (0x10, 4))
                             } else {
                               Return (GPRW (0x10, 3))
                               }
                             }
                         }
                         Device (USB5){
                             Name (_ADR, 0x000E0002)  
                             Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                             {
                               Return (0x03)
                             }
                             Method (_PRW, 0, NotSerialized) 
                             {
                             if(LEqual(\_SB.PCI0.NBF6.US4C, One))
                             {
                              Return (GPRW (0x10, 4))
                             } else {
                               Return (GPRW (0x10, 3))
                               }
                             }
                         }
                         Device (EHC2){
                             Name (_ADR, 0x000E0007)  
                             Method (_PRW, 0, NotSerialized) 
                             {
                             if(LEqual(\_SB.PCI0.NBF6.US4C, One))
                             {
                              Return (GPRW (0x10, 4))
                             } else {
                               Return (GPRW (0x10, 3))
                               }
                             }
                         }
       		  Device(XHCI) { // IOE XHCI B11D18F0
                                     Name(_ADR, 0x00120000)
                                     Method (_PRW, 0, NotSerialized) 
                                     
                             {
                             if(LEqual(\_SB.PCI0.NBF6.US4C, One))
                             {
                              Return (GPRW (0x10, 4))
                             } else {
                               Return (GPRW (0x10, 3))
                               }
                             }      
    
                             
                             // can wakeup from S4 state
                                     Method (_S3D, 0) {
                                               Return(2)
                                     }

                                     Method (_S4D, 0) {
                                               Return(2)
                                     }
                                                                   
                                     Method (_S0W, 0, NotSerialized){
                                      	if(LEqual(\_SB.PCI0.NBF6.RTD3, One))
                                      	{
                                            Return(0x03)
                                      	} else {
                                      		Return(0x0)
                                      	}
                                     }
                                     
                                     Device(RHUB){
                                            Name(_ADR, ZERO)


                                            Device(HS1) {

                                                      Name( _ADR, 0x00000001)
                                                      Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})
                                               
                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, 
                                                                 0x00,0x00,0x00,0x00, 
                                                                 0xE1,0x1D,0x00,0x00, 
                                                                 0x03,0x00,0x00,0x00, 
                                                                 0xFF,0xFF,0xFF,0xFF}}) 
                                             }// Device( HS1)
                                            
                                             Device(HS2) {
                                                      Name( _ADR, 0x00000002)
                                                        Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x71,0x0c,0x80,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                            }// Device( HS2)

                                            Device(HS3) {
                                                      Name( _ADR, 0x00000003)
                                                        Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})
                                                        
                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0C,0x80,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                            }// Device( HS3)

                                            Device(HS4) {
                                                      Name( _ADR, 0x00000004)
                                                      Method(_UPC,0,Serialized)
                                                      {
                                                      		Name(UPCB, Package(4) {0x1,0x3,Zero,Zero})
                                                      		 if(LEqual(\_SB.PCI0.NBF6.P1TP, One)) 
                                                      		 {
                                                      		 	Store(0xA, Index(UPCB, 1))
                                                      		 } 
                                                      		 Return(UPCB)
                                                      }

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0c,0x80,0x00, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                            }// Device( HS4)

                                            Device(HS5) {
                                                      Name( _ADR, 0x00000005)
                                                      Method(_UPC,0,Serialized)
                                                      {
                                                      		Name(UPCB, Package(4) {0x1,0x3,Zero,Zero})
                                                      		 if(LEqual(\_SB.PCI0.NBF6.P2TP, One)) 
                                                      		 {
                                                      		 	Store(0xA, Index(UPCB, 1))
                                                      		 } 
                                                      		 Return(UPCB)
                                                      }

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0c,0x00,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                            }// Device( HS5)


                                            Device( SS1) {
                                                        Name( _ADR, 0x00000006)
                                                        
                                                        Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, 
                                                                 0x00,0x00,0x00,0x00, 
                                                                 0xE1,0x1D,0x00,0x00, 
                                                                 0x03,0x00,0x00,0x00, 
                                                                 0xFF,0xFF,0xFF,0xFF}}) 

                                               } // Device( SS1)

                                               Device( SS2) {
                                                        Name( _ADR, 0x00000007)
                                                        Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x71,0x0c,0x80,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                               } // Device( SS2)

                                               Device( SS3) {
                                                        Name( _ADR, 0x00000008)
                                                        Name( _UPC, Package(){
                                                        0x1, 
                                                        0x3, // Connector type (N/A for non-visible ports)
                                                        0x0, 
                                                        0x0})

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0C,0x80,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                               } // Device( SS3)

                                               Device( SSP1) {
                                                        Name( _ADR, 0x00000009)
                                                      	Method(_UPC,0,Serialized)
                                                      	{
                                                      		Name(UPCB, Package(4) {0x1,0x3,Zero,Zero})
                                                      		 if(LEqual(\_SB.PCI0.NBF6.P1TP, One)) 
                                                      		 {
                                                      		 	Store(0xA, Index(UPCB, 1))
                                                      		 } 
                                                      		 Return(UPCB)
                                                      	}

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0c,0x80,0x00, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                               } // Device( SSP1)

                                               Device( SSP2) {
                                                        Name( _ADR, 0x0000000A)
                                                      	Method(_UPC,0,Serialized)
                                                      	{
                                                      		Name(UPCB, Package(4) {0x1,0x3,Zero,Zero})
                                                      		 if(LEqual(\_SB.PCI0.NBF6.P2TP, One)) 
                                                      		 {
                                                      		 	Store(0xA, Index(UPCB, 1))
                                                      		 } 
                                                      		 Return(UPCB)
                                                      	}

                                                        Name( _PLD, Package(1) {
                                                                 Buffer(0x14) {
                                                                 0x82,0x00,0x00,0x00, // Revision 2, Ignore color
                                                                 // Color (ignored), width and height not
                                                                 0x00,0x00,0x00,0x00, // required as this is a standard USB 'A' type
                                                                 // connector
                                                                 0x69,0x0c,0x00,0x01, // User visible, Back panel, Vertical
                                                                 // Center, shape = vert. rectangle
                                                                 0x03,0x00,0x00,0x00, // ejectable, requires OPSM eject assistance
                                                                 0xFF,0xFF,0xFF,0xFF}})// Vert. and Horiz. Offsets not supplied
                                               } // Device( SSP2)
                                               
                                   }        
                                      
                            } // end "IOE XHCI B11D18F0"         

							
						} // end "IOE PCIE2PCI Bridge B10D0F0"
					} // end "IOE Virt SW DN Port B2D8F0"
				} // end "IOE SwitchUpBridge B1D0F0"


