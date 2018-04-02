
#include <PlatformDefinition2.h>

#ifndef MDEPKG_NDEBUG      // debug mode
#define ASL_COM(x)   DBGC x
#else
#define ASL_COM(x)
#endif

DefinitionBlock ("SSDT.aml", "SSDT", 2, "_BYO_ ", "ZX_PEMCU", 0x00000001)
{
	Scope(\_SB)
	{
#ifdef ZX_DUALSOCKET
		Device (SPEC)
		{
			Name(_HID, EISAID("PNP0C01"))
			Name(_STR, Unicode("Slave PEMCU Device"))
			Name(_UID, 0xF1)

			Method(_STA, 0)
			{
				Return (0x0f)
			}
		}
#endif

		Device (MPEC)
		{
			Name(_HID, EISAID("PNP0C01"))
			Name(_STR, Unicode("Master PEMCU Device"))
			Name(_UID, 0xF0)

			Method(_STA, 0)
			{
				Return (0x0f)
			}
		}
	}
}


