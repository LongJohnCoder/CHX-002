
           Device (USB0)
            {
                Name (_ADR, 0x000E0000)  
                OperationRegion (WAU3, PCI_Config, 0x84, One)
                Field (WAU3, ByteAcc, NoLock, Preserve)
                {
                    U384,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                Method (_PRW, 0, NotSerialized)  
                {
               
                  if(LEqual(\_SB.PCI0.NBF6.S4WP, One))
                      {
                       Return (GPRW (0x0E, 4))
                       } else {
                       Return (GPRW (0x0E, 3))
                        }
                }
   
            }
            
           Device (EHC1)
           {
           		Name (_ADR, 0x000E0007)  
                Method (_PRW, 0, NotSerialized) {        
                	if(LEqual(\_SB.PCI0.NBF6.S4WP, One)){
                    	Return (GPRW (0x0E, 4))
                    } else {
                    	Return (GPRW (0x0E, 3))
                    }
               	}
           }
           
           Device (USB1)
           {
                Name (_ADR, 0x00100000)  
                OperationRegion (WAU1, PCI_Config, 0x84, One)
                Field (WAU1, ByteAcc, NoLock, Preserve)
                {
                    U184,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                 Method (_PRW, 0, NotSerialized)  
                {
               
                  if(LEqual(\_SB.PCI0.NBF6.S4WP, One))
                      {
                       Return (GPRW (0x0E, 4))
                       } else {
                       Return (GPRW (0x0E, 3))
                        }
                }
            }

            Device (USB2)
            {
                Name (_ADR, 0x00100001)  
                OperationRegion (WAU2, PCI_Config, 0x84, One)
                Field (WAU2, ByteAcc, NoLock, Preserve)
                {
                    U284,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                Method (_PRW, 0, NotSerialized)  
                {
               
                  if(LEqual(\_SB.PCI0.NBF6.S4WP, One))
                      {
                       Return (GPRW (0x0E, 4))
                       } else {
                       Return (GPRW (0x0E, 3))
                        }
                }
            }
            
            Device (EHC2)
            {
                Name (_ADR, 0x00100007)  
                OperationRegion (WAEH, PCI_Config, 0x84, One)
                Field (WAEH, ByteAcc, NoLock, Preserve)
                {
                    EH84,   8
                }

                 Method (_PRW, 0, NotSerialized)  
                {
               
                  if(LEqual(\_SB.PCI0.NBF6.S4WP, One))
                      {
                       Return (GPRW (0x0E, 4))
                       } else {
                       Return (GPRW (0x0E, 3))
                        }
                }
            }

            Device (XHCI)
            {
                Name (_ADR, 0x00120000)

                Method (_PRW, 0, NotSerialized) {       // SCI Enable/Status Bit: 15, deepest wakeup state: S3/S4
                    if(LEqual(\_SB.PCI0.NBF6.S4WP, One)) {
                        Return (GPRW (0x0E, 4))
                    } else {
                        Return (GPRW (0x0E, 3))
                    }
                }

                Method (_S3D, 0) {                      // S3 device state
                    Return(3)
                }

                Method (_S4D, 0) {                      // S4 device state
                    Return(3)
                }

                Method (_S0W, 0, NotSerialized) {       // RTD3
                	if(LEqual(\_SB.PCI0.NBF6.RTDX, One)){
                    	Return(0x03)
                    } else {
                       	Return(0x0)
                    }
                }

                Device(RHUB) {
                    Name(_ADR, ZERO)

                    Device(HS1) {
                        Name( _ADR, 0x00000001)

                        Name( _UPC, Package() {
                            0x1,
                            0x3, // Connector type (N/A for non-visible ports)
                            0x0,
                            0x0
                        })

                        /*Name( _PLD, Package(1) {
                            Buffer(0x14) {
                                0x82,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,
                                0xE1,0x1D,0x00,0x00,
                                0x03,0x00,0x00,0x00,
                                0xFF,0xFF,0xFF,0xFF
                            }
                        })*/
                    }// Device( HS1)

                    Device(HS2) {
                        Name( _ADR, 0x00000002)

                        Name( _UPC, Package() {
                            0x1,
                            0x3, // Connector type (N/A for non-visible ports)
                            0x0,
                            0x0
                        })

                        /*Name( _PLD, Package(1) {
                            Buffer(0x14) {
                                0x82,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,
                                0x71,0x0c,0x80,0x01,
                                0x03,0x00,0x00,0x00,
                                0xFF,0xFF,0xFF,0xFF
                            }
                        })*/
                    }// Device( HS2)

                    Device( SS1) {
                        Name( _ADR, 0x00000003)

                        Name( _UPC, Package() {
                            0x1,
                            0x3, // Connector type (N/A for non-visible ports)
                            0x0,
                            0x0
                        })

                        /*Name( _PLD, Package(1) {
                            Buffer(0x14) {
                                0x82,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,
                                0xE1,0x1D,0x00,0x00,
                                0x03,0x00,0x00,0x00,
                                0xFF,0xFF,0xFF,0xFF
                            }
                        })*/
                    } // Device( SS1)

                    Device( SS2) {
                        Name( _ADR, 0x00000004)
                        
                        Name( _UPC, Package() {
                            0x1,
                            0x3, // Connector type (N/A for non-visible ports)
                            0x0,
                            0x0
                        })

                        /*Name( _PLD, Package(1) {
                            Buffer(0x14) {
                                0x82,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,
                                0x71,0x0c,0x80,0x01,
                                0x03,0x00,0x00,0x00,
                                0xFF,0xFF,0xFF,0xFF
                            }
                        })*/
                    } // Device( SS2)
                }
            }

            
