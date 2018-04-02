
#include <PlatformDefinition2.h>

include("HX002EC0_01_IOE_33\pcitreeioe.asl")

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

		      Device(I20S) { // IOE RC B2D2F0 Slot
		      Name(_ADR, 0x00000000)
		      Method(_PRW, 0) { Return(GPRW(0x10, 4)) }	// can wakeup from S4 state
		   }     // end "IOE RC B2D2F0 Slot"
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

			Device(I50S) { // IOE RC B2D5F0 Slot
				  Name(_ADR, 0x00000000)
				  Method(_PRW, 0) { Return(GPRW(0x10, 4)) }   // can wakeup from S4 state
			}

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


