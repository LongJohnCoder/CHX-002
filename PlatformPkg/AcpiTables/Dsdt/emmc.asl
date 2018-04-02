  Device(EMMC) {
    Name(_ADR, 0x000C0000)

    Method(_DSW, 3){}
    
    Method(_STA, 0, NotSerialized)
    {
        Return (0x0F)
    }

    Device (CARD)
    {
      Name (_ADR, 0x08)
      Method(_RMV, 0x0, NotSerialized)
      {
        Return (0)
      }
    }
  }
