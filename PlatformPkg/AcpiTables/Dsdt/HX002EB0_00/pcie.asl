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

		Device (PE3S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
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


