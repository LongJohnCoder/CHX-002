////////////////////////////////////////////////////////////////////
// following is for IOE

//	Name(PRX1, Package(){
// IOE SwitchUpBridge B1D0F0
//		Package(){0x0000FFFF, 0, LNKH, 0 },
//	})
//	Name(ARX1, Package(){
// IOE SwitchUpBridge B1D0F0
//		Package(){0x0000FFFF, 0, 0, 28 },	//CHX002 is 28
//	})

	Name(PRX2, Package(){
#if IOE_SHOW_XBCTL
// IOE XBCTL B2D0F0
		Package(){0x0000FFFF, 0, LNKH, 0 },
#endif
// IOE RC B2D1F0
		Package(){0x0001FFFF, 0, LNKH, 0 },
// IOE RC B2D5F0
		Package(){0x0005FFFF, 0, LNKH, 0 },
// IOE Virt SW DN Port B2D8F0
		Package(){0x0008FFFF, 0, LNKH, 0 },
	})
	
	Name(ARX2, Package(){
	
#if IOE_SHOW_XBCTL	
// IOE XBCTL B2D0F0
		Package(){0x0000FFFF, 0, 0, 28 },	//CHX002 is 28; 
#endif

// IOE RC B2D1F0
		Package(){0x0001FFFF, 0, 0, 29 },	//CHX002 is 29; 
// IOE RC B2D5F0
		Package(){0x0005FFFF, 0, 0, 29 },	//CHX002 is 29;
// IOE Virt SW DN Port B2D8F0
		Package(){0x0008FFFF, 0, 0, 28 },	//CHX002 is 28; 
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
		Package(){0x0000FFFF, 0, 0, 29 },	//CHX002 is 29; 
		Package(){0x0000FFFF, 1, 0, 30 },	//CHX002 is 30; 
		Package(){0x0000FFFF, 2, 0, 31 },	//CHX002 is 31; 
		Package(){0x0000FFFF, 3, 0, 28 },	//CHX002 is 28; 
	})



	Name(PRX7, Package(){
// IOE RC B2D5F0 Slot
		Package(){0x0000FFFF, 0, LNKH, 0 },
	})
	Name(ARX7, Package(){
// IOE RC B2D5F0 Slot
		Package(){0x0000FFFF, 0, 0, 29 },	//CHX002 is 29; 
	})
	

	Name(PRXA, Package(){
// IOE Virt SW DN Port B2D8F0 below device PCIEIF IRQ Routing Infromation
		Package(){0x0000FFFF, 0, LNKH, 0 },
	})
	Name(ARXA, Package(){ 
// IOE Virt SW DN Port B2D8F0 below device PCIEIF IRQ Routing Infromation
		Package(){0x0000FFFF, 0, 0, 28 },	//CHX002 is 28; CND001 is 2C
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
		Package(){0x0000FFFF, 0, 0, 28 }, 	//CHX002 is 28;
		Package(){0x000DFFFF, 0, 0, 29 },	//CHX002 is 29; 
		Package(){0x000EFFFF, 0, 0, 30 },	//CHX002 is 30; 
		Package(){0x000EFFFF, 1, 0, 31 },	//CHX002 is 31; 
		Package(){0x000EFFFF, 2, 0, 28 },	//CHX002 is 28; 
		Package(){0x000EFFFF, 3, 0, 29 },	//CHX002 is 29; 
		Package(){0x0010FFFF, 0, 0, 28 },	//CHX002 is 28; 
		Package(){0x0010FFFF, 1, 0, 29 },	//CHX002 is 29; 
		Package(){0x0010FFFF, 2, 0, 30 },	//CHX002 is 30; 
		Package(){0x0010FFFF, 3, 0, 31 },	//CHX002 is 31; 
		Package(){0x000FFFFF, 0, 0, 31 },	//CHX002 is 31; 
		Package(){0x0012FFFF, 0, 0, 30 },	//CHX002 is 30; 
	}) 	


	Name(PRXC, Package(){
                   Package(){0x0000FFFF, 0, LNKH, 0 },
                   Package(){0x0000FFFF, 1, LNKH, 0 },
                   Package(){0x0000FFFF, 2, LNKH, 0 },
                   Package(){0x0000FFFF, 3, LNKH, 0 }
         })
         
	Name(ARXC, Package(){
                   Package(){0x0000FFFF, 0, 0, 29 }, //CHX002 is 29; 
                   Package(){0x0000FFFF, 1, 0, 30 }, //CHX002 is 30; 
                   Package(){0x0000FFFF, 2, 0, 31 }, //CHX002 is 31; 
                   Package(){0x0000FFFF, 3, 0, 28 }, //CHX002 is 28; 
	   })


