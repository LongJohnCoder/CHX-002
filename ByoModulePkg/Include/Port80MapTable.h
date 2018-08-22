/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Port80MapTable.h

Abstract: 
  Port80 status code definition.

Revision History:

Bug 2909:   Add some port 80 status codes into EDKII code.
TIME:       2011-09-23
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     All Platforms
$TECHNICAL: 
  1. Improve Port80 map table.
  2. Add Port80 status codes in the corresponding position to report status code.
  3. Change the seconed REPORT_STATUS_CODE_WITH_EXTENDED_DATA macro's parameter
     to EFI_SW_PC_INIT_END from EFI_SW_PC_INIT_BEGIN.
$END--------------------------------------------------------------------

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/
//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Port80 status code definition.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef __PORT_80_MAP_TABLE_H__
#define __PORT_80_MAP_TABLE_H__

#include <ByoStatusCode.h>


STATUS_CODE_TO_DATA_MAP  mProgressPort80MapTable[] = {
  //
  // SEC (0x0 - 0xF)
  //
  
  //
  // PEI (0x10 - 0x4F)
  //
  // Regular boot
  { PEI_CORE_STARTED,             0x10 },           // PeiCore() in PeiMain.c
  { PEI_CAR_CPU_INIT,             0x11 },           // PeimInitializeCpu() in CpuPeim.c
  // { Reserved for CPU,          0x12 - 0x14
  { PEI_CAR_SB_INIT,              0x15 },           // PchInitialize() in PchInitPeim.c
  // reserved for SB:             0x16 - 0x18
  { PEI_CAR_NB_INIT,              0x19 },           // SaInitPeiEntryPoint() in SaInitPeim.c
  // reserved for NB:             0x1A - 0x1C
  //Add  by jenny for pcie
  //{ PEI_PCIE_INIT,                0x12 },           // PeiNbPreMemoryInit()  in NbInit.c 
  //{ PEI_PCIE_RELEASE_LTSSM,       0x13 },           // PeiNbPcieInit()  in NbPcie.c 
  //{ PEI_PCIE_IOE_INIT,          0x14 },           // PeiNbPcieInit()  in NbPcie.c    
  //{ PEI_PCIE_BEFORE_CHANGE_MAX_SPEED,            0x1A },           // PeiNbPcieInit()  in NbPcie.c  
  //{ PEI_PCIE_END,        0x1B },           // PeiNbPcieInit()  in NbPcie.c  
  
  { PEI_MEMORY_SPD_READ,          0x1D },           // MRC_GetSpdData() in MRC_SpdDriver.c
  // { PEI_MEMORY_PRESENCE_DETECT,0x1E },           // TBD
  // { PEI_MEMORY_TIMING,         0x1F},            // TBD
  { PEI_MEMORY_CONFIGURING,       0x20 },           // PeiMemoryInit() in Memoryinit.c
  { PEI_MEMORY_INIT,              0x21 },           // PeiMemoryInit() in Memoryinit.c
  // reserved for OEM use:        0x22 - 0x2F
  // reserved for AML use:        0x30
  { PEI_MEMORY_INSTALLED,         0x31 },           // PeiCore() in PeiMain.c
  // { PEI_MEM_NB_INIT,           0x32 },           // TBD
  // reserved for NB:             0x33 - 0x3A
  //add by jenny for pcie s3
  //{ PEI_SB_POST_MEMORY_S3          0x38 },           //peiSbPostMemoryInitS3() in SbInit.c
  //{ PEI_NB_POST_MEMORY_S3          0x39 },          //peiNbPostMemoryInitS3() in NbInit.c
  //{ PEI_MEM_SB_INIT,            0x3B },           // TBD
  // reserved for SB:             0x3C - 0x3E
  // reserved for OEM use:        0x3F - 0x4E
  { PEI_DXE_IPL_STARTED,          0x4F },           // DxeLoad() in DxeLoad.c
  
  //
  // DXE (0x60 - 0xCF)
  //
  { DXE_CORE_STARTED,             0x60 },           // DxeMain() in DxeMain.c
  { DXE_NB_HB_INIT,               0x61 },           // PciHostBridgeEntryPoint() in PciHostBridge.c
  { DXE_CPU_SMM_INIT,             0x62 },           // SmmIplEntry() in PiSmmIpl.c
  { DXE_VARIABLE_INIT,            0x63 },           // VariableSmmRuntimeInitialize() in VariableSmmRuntimeDxe.c
                                                    // VariableServiceInitialize() in VariableDxe.c
  { DXE_CPU_INIT,                 0x64 },           // InitializeCpu() in Cpu.c
  { DXE_CPU_CACHE_INIT,           0x65 },           // InitializeCpu() in Cpu.c
  { DXE_CPU_BSP_SELECT,           0x66 },           // SwitchToLowestFeatureProcess() in LeastFeatures.c
  { DXE_CPU_AP_INIT,              0x67 },           // InitializeMpSystemData() in MpService.c
  { DXE_SB_SMM_INIT,              0x68 },           // InitializePchSmmDispatcher() in PchSmmCore.c
  { DXE_NB_SMM_INIT,              0x69 },           // SaLateInitSmmEntryPoint() in SaLateInitSmm.c
  { DXE_NB_INIT,                  0x6A },           // SaInitEntryPoint() in SaInit.c
  // reserved for NB:             0x6B - 0x6F
  //add by jenny for PCIE&IOE
  //{ PCIE_EaryDXE_PEMCU_FW_LOAD,   0x6B},             //PlatformEarlyDxeEntry () PlatformEarlyDxe.c
  //{ DXE_NB_PCIE_PRE_INIT,         0x6C},             //AsiaNbDxePrePciInit() in AsiaNbDxeInit.c
  //{ DXE_NB_PCIE_D3D5F1_SPE        0x6D},             //PCIeCommonSpeSetting() in NbPcieCommon.c
  //{ DXE_NB_PCIE_RCRB_SPE,         0x6E},             //PCIeCommonSpeSetting() in NbPcieCommon.c
  //{ DXE_NB_PCIE_EPHY_SPE,         0x6F},             //PCIeCommonSpeSetting() in NbPcieCommon.c
  //{ DXE_NB_IOE_PRE_INIT,          0x71},             //AsiaNbDxePrePciInit() in AsiaNbDxeInit.c
  //{ DXE_NB_PCIE_AGGRESSIVE_PM,    0x72},             //PciePrePciInit in NbDxePciePrePci.c
  //{ DXE_NB_PCIE_POWER_CTL,        0x73},             //PciePrePciInit in NbDxePciePrePci.c
  
  { DXE_SB_INIT,                  0x70 },           // PchInitEntryPoint() in PchInit.c
    // reserved for SB:           0x71 - 0x75
  // { DXE_NB_IOV_SNOOP_CHECK,    0x74 },           //JRZ-20180518 AsiaNbDxePreBootInit() in AsiaNbDxeInit.c
  // { DXE_SB_DEVICES_INIT,       0x76 },           // TBD
  { DXE_SIO_INIT,                 0x77 },           // SioDriverEntryPoint() in SioDriver.c
  // { DXE_ACPI_INIT,             0x78 },           // TBD
  { DXE_CSM_INIT,                 0x79 },           // LegacyBiosInstall() in LegacyBios.c
  // reserved for IBV use:        0x7A - 0x7F
  // reserved fpr OEM use:        0x80 - 0x8F
  { DXE_BDS_STARTED,              0x90 },           // DxeMain() in DxeMain.c
  { DXE_BDS_CONNECT_DRIVERS,      0x91 },           // PlatformBootManagerBeforeConsole() in BdsPlatform.c
  { DXE_PCI_BUS_BEGIN,            0x92 },           // PciBusDriverBindingStart() in PciBus.c
  { DXE_PCI_BUS_HPC_INIT,         0x93 },           // PciRootBridgeP2CProcess() in PciLib.c
  { DXE_PCI_BUS_ENUM,             0x94 },           // PciRootBridgeEnumerator() in PciEnumerator.c
  { DXE_PCI_BUS_REQUEST_RESOURCES,0x95 },           // PciHostBridgeResourceAllocator() in PciLib.c
  { DXE_PCI_BUS_ASSIGN_RESOURCES, 0x96 },           // PciHostBridgeResourceAllocator() in PciLib.c
  { DXE_CON_OUT_CONNECT,          0x97 },           // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  { DXE_CON_IN_CONNECT,           0x98 },           // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  { DXE_USB_BEGIN,                0x99 },           // StartLegacyUsb() in StartLegacyUsb.c
  { DXE_USB_RESET,                0x9A },           // StartLegacyUsb() in Uhci.c/Ehci.c/Xhci.c/Ohci.c
  { DXE_USB_DETECT,               0x9B },           // StartLegacyUsb() in StartLegacyUsb.c
  { DXE_USB_HOTPLUG,              0x9C },           // UsbCreateDevice() in UsbEnumer.c
  { DXE_USB_ENABLE,               0x9D },           // StartLegacyUsb() in StartLegacyUsb.c
  { DXE_USB_HOTPLUG_OUT,          0x9E },           // UsbEnumeratePort() in UsbEnumer.c
  // reserved for IBV use:        0x9E - 0x9F

  { DXE_ATA_PASS_THRU_BEGIN,      0xA0 },           // AtaAtapiPassThruStart() in AtaAtapiPassThru.c
  { DXE_ATA_PASS_THRU_RESET,      0xA1 },           // AhciModeInitialization() in AhciMode.c
                                                    // IdeModeInitialization() in IdeMode.c
  { DXE_ATA_PASS_THRU_ENUMERATION,0xA2 },           // AhciModeInitialization() in AhciMode.c
                                                    // IdeModeInitialization() in IdeMode.c   
  { DXE_ATA_BEGIN,                0xA4 },           // AtaBusDriverBindingStart() in AtaBus.c
  { DXE_ATA_DETECT,               0xA6 },           // RegisterAtaDevice()in AtaBus.c
  { DXE_ATA_ENABLE,               0xA7 },           // RegisterAtaDevice() in AtaBus.c
  
  { DXE_SCSI_BEGIN,               0xA8 },           // SCSIBusDriverBindingStart() in ScsiBus.c
  { DXE_SCSI_RESET,               0xA9 },           // ScsiResetBus() in ScsiBus.c
  { DXE_SCSI_DETECT,              0xAA },           // DiscoverScsiDevice() in ScsiBus.c
  { DXE_SCSI_ENABLE,              0xAB },           // DiscoverScsiDevice() in ScsiBus.c

  { DXE_MOUSE_ENABLE,             0xA5 },
  
  { DXE_SETUP_START,              0xAC },           // ByoSetUpManager() in FrontPage.c
  { DXE_READY_TO_BOOT,            0xAD },           // BdsLibBootViaBootOption() in BdsBoot.c
  { DXE_LEGACY_BOOT,              0xAE },           // GenericLegacyBoot() in LegacyBootSupport.c
  { DXE_EXIT_BOOT_SERVICES,       0xAF },           // CoreExitBootServices() in DxeMain.c
  { RT_SET_VIRTUAL_ADDRESS_MAP_BEGIN, 0xB0 },       // RuntimeDriverSetVirtualAddressMap() in Runtime.c
  // { RT_SET_VIRTUAL_ADDRESS_MAP_END, 0xB1 },      // TBD
  { DXE_LEGACY_OPROM_INIT,        0xB2 },           // LegacyBiosInstallRom() in LegacyPci.c
  { DXE_RESET_SYSTEM,             0xB3 },           // IntelPchResetSystem() and PchExtendedReset() in PchReset.c
  // { DXE_PCI_BUS_HOTPLUG,       0xB4 },           // TBD
  { DXE_VARIABLE_CLEAN_UP,        0xB5 },           // Reclaim() in Variable.c
  // { DXE_CONFIGURATION_RESET,   0xB6 },           // TBD
  { DXE_ACPI_ENABLE,              0xB7 },           // EnableAcpiCallback() in SmmPlatform.c
  { DXE_USB_HAND_OFF,             0xB8 },           // UsbOwnershipHandoff() in UsbBus.c
  // reserved for IBV use:        0xB9 - 0xBF
  // reserved for OEM use:        0xC0 - 0xCF
  
  //
  // S3 (0xE0 - 0xE7)
  //
  { PEI_S3_SUSPEND_STARTED,       0xE0 },           // S3SleepEntryCallBack() in SmmPlatform.c
  { PEI_S3_SUSPEND_ENDED,         0xE1 },           // PchSmmSxGoToSleep() in PchSmmSx.c
  // reserved for suspend:        0xE2 - 0xE3      
  { PEI_S3_RESUME_STARTED,        0xE4 },           // S3RestoreConfig2() in S3Resume.c
  { PEI_S3_BOOT_SCRIPT,           0xE5 },           // S3ResumeExecuteBootScript() in S3Resume.c
  // { PEI_S3_VIDEO_REPOST,       0xE6 },           // Obsolete as video OpRom repost is not supported
  { PEI_S3_OS_WAKE,               0xE7 },           // S3ResumeBootOs() in S3Resume.c

  //
  // Recovery (0xF0 - 0xF7)
  //
  { PEI_RECOVERY_AUTO,            0xF0 },           // PlatformStage1InitBootMode() in BootMode.c
  { PEI_RECOVERY_USER,            0xF1 },           // PlatformStage1InitBootMode() in BootMode.c
  { PEI_RECOVERY_STARTED,         0xF2 },           // PlatformRecoveryModule() in Recovery.c
  { PEI_RECOVERY_CAPSULE_FOUND,   0xF3 },           // PlatformRecoveryModule() in Recovery.c
  { PEI_RECOVERY_CAPSULE_LOADED,  0xF4 },           // PlatformRecoveryModule() in Recovery.c
   

  {0,0}
};

STATUS_CODE_TO_DATA_MAP  mErrorPort80MapTable[] = {
  //
  // PEI (0x50 - 0x5F)
  //
  // { PEI_MEMORY_INVALID_TYPE,   0x50 },           // TBD
  // { PEI_MEMORY_INVALID_SPEED,  0x50 },           // TBD
  // { PEI_MEMORY_SPD_FAIL,       0x51 },           // TBD
  // { PEI_MEMORY_INVALID_SIZE,   0x52 },           // TBD
  // { PEI_MEMORY_MISMATCH,       0x52 },           // TBD
  { PEI_MEMORY_NOT_DETECTED,      0x53 },           // PeimMemoryInit() in MemoryInit.c
  // { PEI_MEMORY_NONE_USEFUL,    0x53 },           // TBD
  { PEI_MEMORY_ERROR,             0x54 },           // PeimMemoryInit() in MemoryInit.c
  { PEI_MEMORY_NOT_INSTALLED,     0x55 },           // PeiCore() in PeiMain.c
  // { PEI_RESET_NOT_AVAILABLE,   0x56 },           // TBD
  // reserved for IBV use:        0x57 - 0x5F
  
  //
  // DXE (0xD0 - 0xDF)
  //
  { DXE_CPU_SELF_TEST_FAILED,     0xD0 },           // InitializeMpSystemData() in MpService.c
  // { DXE_CPU_INVALID_TYPE,      0xD1 },           // TBD
  // { DXE_CPU_INVALID_SPEED,     0xD1 },           // TBD
  // { DXE_CPU_MISMATCH,          0xD1 },           // TBD
  // { DXE_CPU_CACHE_ERROR,       0xD2 },           // TBD
  { DXE_CPU_MICROCODE_UPDATE_FAILED, 0xD3 },        // CheckMicrocodeUpdate() in Microcode.c
  { DXE_CPU_NO_MICROCODE,         0xD3 },           // CheckMicrocodeUpdate() in Microcode.c
  // { DXE_CPU_INTERNAL_ERROR,    0xD4 },           // TBD
  // { DXE_CPU_ERROR,             0xD4 },           // TBD
  // { DXE_NB_ERROR,              0xD5 },           // TBD
  // { DXE_SB_ERROR,              0xD6 },           // TBD
  { DXE_ARCH_PROTOCOL_NOT_AVAILABLE, 0xD7 },        // DxeMain() in DxeMain.c
  { DXE_PCI_BUS_OUT_OF_RESOURCES, 0xD8 },           // PciHostBridgeAdjustAllocation() in PciEnumerator.c
  { DXE_LEGACY_OPROM_NO_SPACE,    0xD9 },           // LegacyBiosInstallPciRom() in LegacyPci.c
  { DXE_NO_CON_OUT,               0xDA },           // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  { DXE_NO_CON_IN,                0xDB },           // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  // { DXE_INVALID_PASSWORD,      0xDC },           // TBD
  { DXE_BOOT_OPTION_LOAD_ERROR,   0xDD },           // EfiBootManagerBoot() in BdsBoot.c
  { DXE_BOOT_OPTION_FAILED,       0xDE },           // EfiBootManagerBoot() in BdsBoot.c
  { DXE_FLASH_UPDATE_FAILED,      0xDF },           // PlatformBootManagerAfterConsole() in BdsPlatform.c
    
  
  //
  // S3 (0xE8 - 0xEF)
  //
  { PEI_MEMORY_S3_RESUME_FAILED,  0xE8 },           // TBD
  { PEI_S3_RESUME_PPI_NOT_FOUND,  0xE9 },           // DxeLoadCore() in DxeLoad.c
  { PEI_S3_BOOT_SCRIPT_ERROR,     0xEA },           // TBD
  { PEI_S3_OS_WAKE_ERROR,         0xEB },           // TBD
  // reserved for IBV use:        0xEC - 0xEF

  //
  // Recovery (0xF8 - 0xFF)
  //
  { PEI_RECOVERY_PPI_NOT_FOUND,   0xF8 },           // DxeLoadCore() in DxeLoad.c
  { PEI_RECOVERY_NO_CAPSULE,      0xF9 },           // PlatformRecoveryModule() in Recovery.c
  { PEI_RECOVERY_INVALID_CAPSULE, 0xFA },           // PlatformRecoveryModule() in Recovery.c
  // reserved for IBV use:        0xFB - 0xFF
  
  {0,0}
};
#endif
