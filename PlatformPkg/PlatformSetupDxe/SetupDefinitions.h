
#ifdef FORM_SET_VARSTORE

  efivarstore SETUP_DATA, 
    varid     = SETUP_DATA_ID, 
    attribute = 0x7, 
    name      = Setup, 
    guid      = PLATFORM_SETUP_VARIABLE_GUID; 
  
  #ifdef ZX_TXT_SUPPORT
  efivarstore ASIA_VARIABLE,
    varid     = ASIA_VARIABLE_ID,
    attribute = 0x7,
    name      = AsiaVariable,
    guid      = ASIA_VARIABLE_GUID;
  #endif

  varstore SETUP_VOLATILE_DATA,
    varid = SETUP_VOLATILE_DATA_ID,
    name  = SetupVolatileData,
    guid  = PLATFORM_SETUP_VARIABLE_GUID;
    
  varstore TSESETUP, 
    varid = TSESETUP_DATA_ID, 
    name  = TSESetup, 
    guid  = TSESETUP_GUID;    
    
#endif



//
// Main
//
#include <PlatformSetupDxe/Main/PlatformInfo.sd>

//
// Advanced
//
#include <PlatformSetupDxe/Advanced/Cpu/Cpu.sd>
#include <PlatformSetupDxe/Advanced/Dram/Dram.sd>
#include <PlatformSetupDxe/Advanced/Chipset/Chipset.sd >
#include <PlatformSetupDxe/Advanced/Raida/Raida.sd>
#include <PlatformSetupDxe/Advanced/Others/Others.sd>
#include <PlatformSetupDxe/Advanced/ConsoleRedirection/ConsoleRedirection.sd>
#include <PlatformSetupDxe/Advanced/Virtualization/Virtualization.sd>


//
// Devices
//
#include <PlatformSetupDxe/Devices/Sata/Sata.sd>
#include <PlatformSetupDxe/Devices/Video/VideoSetup.sd>
#include <PlatformSetupDxe/Devices/Audio/Audio.sd>
#include <PlatformSetupDxe/Devices/Network/Network.sd>
#include <PlatformSetupDxe/Devices/Pcie/Pcie.sd>
#include <PlatformSetupDxe/Devices/Uart/Uart.sd>
#include <PlatformSetupDxe/Devices/SNMI/SNMI.sd>
#include <PlatformSetupDxe/Devices/Usb/UsbSetup.sd>
#include <PlatformSetupDxe/Devices/IOE/IOE_TOP.sd>


//
// Boot
//

//
// Exit & Save
//


