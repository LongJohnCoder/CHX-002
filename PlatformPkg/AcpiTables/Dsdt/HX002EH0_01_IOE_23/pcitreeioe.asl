




////////////////////////////////////////////////////////////////////
// following is for IOE

//	Name(PRX1, Package(){
// IOE SwitchUpBridge B1D0F0
//		Package(){0x0000FFFF, 0, LNKH, 0 },
//	})
//	Name(ARX1, Package(){
// IOE SwitchUpBridge B1D0F0
//		Package(){0x0000FFFF, 0, 0, 32 },	//CHX001 is 32
//	})

	Name(PRX2, Package(){
#if IOE_SHOW_XBCTL
// IOE XBCTL B2D0F0
		Package(){0x0000FFFF, 0, LNKH, 0 },
#endif
// IOE RC B2D1F0
		Package(){0x0001FFFF, 0, LNKH, 0 },
// IOE RC B2D2F0
		Package(){0x0002FFFF, 0, LNKH, 0 },
// IOE RC B2D3F0
		Package(){0x0003FFFF, 0, LNKH, 0 },
// IOE RC B2D4F0
		Package(){0x0004FFFF, 0, LNKH, 0 },
// IOE RC B2D5F0
		Package(){0x0005FFFF, 0, LNKH, 0 },
// IOE Virt SW DN Port B2D8F0
		Package(){0x0008FFFF, 0, LNKH, 0 },
	})
	
	Name(ARX2, Package(){
	
#if IOE_SHOW_XBCTL	
// IOE XBCTL B2D0F0
		Package(){0x0000FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 24
#endif

// IOE RC B2D1F0
		Package(){0x0001FFFF, 0, 0, 33 },	//CHX001 is 33; CND001 is 25
// IOE RC B2D2F0
		Package(){0x0002FFFF, 0, 0, 34 },	//CHX001 is 34; CND001 is 26
// IOE RC B2D3F0
		Package(){0x0003FFFF, 0, 0, 35 },	//CHX001 is 35; CND001 is 27
// IOE RC B2D4F0
		Package(){0x0004FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 24
// IOE RC B2D5F0
		Package(){0x0005FFFF, 0, 0, 33 },	//CHX001 is 33; CND001 is 25
// IOE Virt SW DN Port B2D8F0
		Package(){0x0008FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 24
	})

	
	Name(PRX3, Package(){
// IOE RC B2D1F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },
		Package(){0x0000FFFF, 1, LNKH, 0 },
		Package(){0x0000FFFF, 2, LNKH, 0 },
		Package(){0x0000FFFF, 3, LNKH, 0 },
	})
	Name(ARX3, Package(){
// IOE RC B2D1F0 Slot
		Package(){0x0000FFFF, 0, 0, 33 },	//CHX001 is 32; CND001 is 24
		Package(){0x0000FFFF, 1, 0, 34 },	//CHX001 is 33; CND001 is 25
		Package(){0x0000FFFF, 2, 0, 35 },	//CHX001 is 34; CND001 is 26
		Package(){0x0000FFFF, 3, 0, 32 },	//CHX001 is 35; CND001 is 27
	})
	Name(PRX4, Package(){
// IOE RC B2D2F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },	
		Package(){0x0000FFFF, 1, LNKH, 0 },	
		Package(){0x0000FFFF, 2, LNKH, 0 },	
		Package(){0x0000FFFF, 3, LNKH, 0 },	
	})
	Name(ARX4, Package(){
// IOE RC B2D2F0 Slot
		Package(){0x0000FFFF, 0, 0, 34 },	//CHX001 is 32; CND001 is 24
		Package(){0x0000FFFF, 1, 0, 35 },	//CHX001 is 33; CND001 is 25
		Package(){0x0000FFFF, 2, 0, 32 },	//CHX001 is 34; CND001 is 26
		Package(){0x0000FFFF, 3, 0, 33 },	//CHX001 is 35; CND001 is 27
	})
	Name(PRX5, Package(){
// IOE RC B2D3F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },
		Package(){0x0000FFFF, 1, LNKH, 0 },
		Package(){0x0000FFFF, 2, LNKH, 0 },
		Package(){0x0000FFFF, 3, LNKH, 0 },
	})
	Name(ARX5, Package(){
// IOE RC B2D3F0 Slot
		Package(){0x0000FFFF, 0, 0, 35 },	//CHX001 is 32; CND001 is 24
		Package(){0x0000FFFF, 1, 0, 32 },	//CHX001 is 33; CND001 is 25
		Package(){0x0000FFFF, 2, 0, 33 },	//CHX001 is 34; CND001 is 26
		Package(){0x0000FFFF, 3, 0, 34 },	//CHX001 is 35; CND001 is 27
	})
	Name(PRX6, Package(){
// IOE RC B2D4F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },	
		Package(){0x0000FFFF, 1, LNKH, 0 },	
		Package(){0x0000FFFF, 2, LNKH, 0 },	
		Package(){0x0000FFFF, 3, LNKH, 0 }	
	})
	Name(ARX6, Package(){
// IOE RC B2D4F0 Slot
		Package(){0x0000FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 24
		Package(){0x0000FFFF, 1, 0, 33 },	//CHX001 is 33; CND001 is 25
		Package(){0x0000FFFF, 2, 0, 34 },	//CHX001 is 34; CND001 is 26
		Package(){0x0000FFFF, 3, 0, 35 },	//CHX001 is 35; CND001 is 27
	})
	Name(PRX7, Package(){
// IOE RC B2D5F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },
	})
	Name(ARX7, Package(){
// IOE RC B2D5F0 Slot
		Package(){0x0000FFFF, 0, 0, 33 },	//CHX001 is 32; CND001 is 24
	})
	

	Name(PRXA, Package(){
// IOE Virt SW DN Port B2D8F0 below device PCIEIF IRQ Routing Infromation
		Package(){0x0000FFFF, 0, LNKH, 0 },
	})
	Name(ARXA, Package(){ 
// IOE Virt SW DN Port B2D8F0 below device PCIEIF IRQ Routing Infromation
		Package(){0x0000FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 2C
	})

// IOE PCIE2PCI Bridge B10D0F0 below device ISB IRQ Routing Infromation
	Name(PRXB, Package(){
		Package(){0x0000FFFF, 0, LNKH, 0 },
		Package(){0x000DFFFF, 0, LNKH, 0 },
		Package(){0x000EFFFF, 0, LNKH, 0 },
		Package(){0x000EFFFF, 1, LNKH, 0 },
		Package(){0x000EFFFF, 2, LNKH, 0 },
		Package(){0x000EFFFF, 3, LNKH, 0 },
		Package(){0x0010FFFF, 0, LNKH, 0 },
		Package(){0x0010FFFF, 1, LNKH, 0 },
		Package(){0x0010FFFF, 2, LNKH, 0 },
		Package(){0x0010FFFF, 3, LNKH, 0 },
		Package(){0x000FFFFF, 0, LNKH, 0 },
		Package(){0x0012FFFF, 0, LNKH, 0 },
	})
	
// IOE PCIE2PCI Bridge B10D0F0	below device ISB IRQ Routing Infromation
	Name(ARXB, Package(){
		Package(){0x0000FFFF, 0, 0, 32 }, 	//CHX001 is 32; CND001 is 2C
		Package(){0x000DFFFF, 0, 0, 33 },	//CHX001 is 33; CND001 is 2D
		Package(){0x000EFFFF, 0, 0, 34 },	//CHX001 is 34; CND001 is 2E
		Package(){0x000EFFFF, 1, 0, 35 },	//CHX001 is 35; CND001 is 2F
		Package(){0x000EFFFF, 2, 0, 32 },	//CHX001 is 32; CND001 is 2C
		Package(){0x000EFFFF, 3, 0, 33 },	//CHX001 is 33; CND001 is 2D
		Package(){0x0010FFFF, 0, 0, 32 },	//CHX001 is 32; CND001 is 2C
		Package(){0x0010FFFF, 1, 0, 33 },	//CHX001 is 33; CND001 is 2D
		Package(){0x0010FFFF, 2, 0, 34 },	//CHX001 is 34; CND001 is 2E
		Package(){0x0010FFFF, 3, 0, 35 },	//CHX001 is 35; CND001 is 2F
		Package(){0x000FFFFF, 0, 0, 35 },	//CHX001 is 35; CND001 is 2F
		Package(){0x0012FFFF, 0, 0, 34 },	//CHX001 is 34; CND001 is 2E
	}) 	


	Name(PRXC, Package(){
                   Package(){0x0000FFFF, 0, LNKH, 0 },
                   Package(){0x0000FFFF, 1, LNKH, 0 },
                   Package(){0x0000FFFF, 2, LNKH, 0 },
                   Package(){0x0000FFFF, 3, LNKH, 0 }
         })
         
	Name(ARXC, Package(){
	                   Package(){0x0000FFFF, 0, 0, 33 },
	                   Package(){0x0000FFFF, 1, 0, 34 },
	                   Package(){0x0000FFFF, 2, 0, 35 },
	                   Package(){0x0000FFFF, 3, 0, 32 },
	   })
























