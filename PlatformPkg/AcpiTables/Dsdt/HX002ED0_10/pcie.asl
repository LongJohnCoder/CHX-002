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

		Device (PE7S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}
