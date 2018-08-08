

Scope (\_GPE)
{
#if defined(PCAL6416A_PCIE_HOTPLUG_SUPPORT_CHX002)

  //JNY-20180808 add _Lxx Method to service PCIe Hotplug SCI
  Method(_L0B, 0, NotSerialized)   //gpe block 0 bit11
  {
	   //for CHX_D4F0
	   Store(\_SB.PCI0.NPE7.HPEH(One), Local0)
	   Notify(\_SB.PCI0.NPE7.H000, Local0)
	   Notify(\_SB.PCI0.NPE7.H001, Local0)
	   Notify(\_SB.PCI0.NPE7.H002, Local0)
	   Notify(\_SB.PCI0.NPE7.H003, Local0)
	   Notify(\_SB.PCI0.NPE7.H004, Local0)
	   Notify(\_SB.PCI0.NPE7.H005, Local0)
	   Notify(\_SB.PCI0.NPE7.H006, Local0)
	   Notify(\_SB.PCI0.NPE7.H007, Local0)

  }

#endif
}