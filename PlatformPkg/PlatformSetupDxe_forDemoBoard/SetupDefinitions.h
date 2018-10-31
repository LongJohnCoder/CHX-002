
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
#include <PlatformSetupDxe_forDemoBoard/Main/PlatformInfo.sd>

//
// Advanced
//
#include <PlatformSetupDxe_forDemoBoard/Advanced/Cpu/Cpu.sd>
#include <PlatformSetupDxe_forDemoBoard/Advanced/Dram/Dram.sd>
#include <PlatformSetupDxe_forDemoBoard/Advanced/Chipset/Chipset.sd >
#include <PlatformSetupDxe_forDemoBoard/Advanced/Raida/Raida.sd>
#include <PlatformSetupDxe_forDemoBoard/Advanced/Others/Others.sd>
#include <PlatformSetupDxe_forDemoBoard/Advanced/ConsoleRedirection/ConsoleRedirection.sd>
#include <PlatformSetupDxe_forDemoBoard/Advanced/Virtualization/Virtualization.sd>


//
// Devices
//
#include <PlatformSetupDxe_forDemoBoard/Devices/Sata/Sata.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Video/VideoSetup.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Audio/Audio.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Network/Network.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Pcie/Pcie.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Uart/Uart.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/SNMI/SNMI.sd>
#include <PlatformSetupDxe_forDemoBoard/Devices/Usb/UsbSetup.sd>
#ifdef IOE_EXIST
#include <PlatformSetupDxe_forDemoBoard/Devices/IOE/IOE_TOP.sd>
#endif


//
// Boot
//

//
// Exit & Save
//


