	Device (NPE0)
	{
		Name (_ADR, 0x00030000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR01)
			}

			Return (PR01)
		}

		Device (PE0S)
		{
			Name(_ADR, Zero)
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}
	
	Device (NPE2)
	{
		Name (_ADR, 0x00030002)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR03)
			}

			Return (PR03)
		}

		Device (PE2S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}


	Device (NPE3)
	{
		Name (_ADR, 0x00030003)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR04)
			}

			Return (PR04)
		}

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

	}

	Device (NPE4)
	{
		Name (_ADR, 0x00040000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR05)
			}

			Return (PR05)
		}

		Device (PE4S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPE6)
	{
		Name (_ADR, 0x00050000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR07)
			}

			Return (PR07)
		}

		Device (PE6S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPE7)
	{
		Name (_ADR, 0x00050001)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR08)
			}

			Return (PR08)
		}
		//add for CHX D4F0 HP through PCAL6416A
#if PCAL6416A_PCIE_HOTPLUG_SUPPORT_CHX002
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
				
							  OperationRegion (PE7L, SystemMemory, 0xE0029000, 0x1000)	 //declare E001_8000 is D3F0
							  Field (PE7L, ByteAcc,NoLock,Preserve) {
							  Offset(0x1E8),
								, 1,
								PDL2, 1,
									, 5,
								IDPD, 1
							  }
							  
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
				
								Store(0x00, PDL2)
								Store(0x00, IDPD)
								Sleep(100)
				
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
				
								Store(0x01, PDL2)
								Store(0x01, IDPD)
								Sleep(100)
								
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
								Field (PE7L, ByteAcc,NoLock,Preserve) {
								Offset(0x1C3),
									LTM, 8		//PHYLS LTSSM
								}
								Store(LTM, Local3)
								//Notify(H000, Zero)
								//DBGC(2,"JNY0",Local3)
				
								Notify(\_SB.PCI0.NPE7.H000, 0x0)
				
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

		Device (PE7S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
#endif
	}
