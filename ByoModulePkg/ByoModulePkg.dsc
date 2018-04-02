## @file  
# EFI/PI Reference Module Package for All Architectures
#
# Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
# All rights reserved.This software and associated documentation (if any)
# is furnished under a license and may only be used or copied in 
# accordance with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be reproduced, 
# stored in a retrieval system, or transmitted in any form or by any 
# means without the express written consent of Byosoft Corporation.
#
#	File Name:
#  	ByoModulePkg.dsc
#
#	Abstract: 
#  	ByoSoft Core Module Package Configuration File
#
#	Revision History:
#
#	Bug 2204:   Created Byosoft specific package "ByoModulePkg" to 
#             include Byosoft specific functionality
#	TIME:       2011-5-30
#	$AUTHOR:    Chen Daolin    
#	$REVIEWERS: 
#	$SCOPE:     Sugar Bay and Tunnel Customer Refernce Board
#	$TECHNICAL: 
#   1. ByoModulePkg is created to include the features as follows:
#        USB (Legacy/Native/Crisis Recovery)
#        SD (Legacy/Native)
#        Security including HDD password, TCG/TPM, TCM and Pre-boot 
#          Authentication 
#        Setup Browser (Text/Graphic mode)
#        Debugging 
#        Binary Image (CSM16/AHCI Option Rom)
#   2. Both of DSC and DEC in Sugar Bay and Tunnel Creek are updated 
#      to refer to the new location of drivers in ByoModulePkg.
#   3. Reserved 630-640k for the usage of BIOS stack in int 13h handler. 
#      Previously this stack is allocated via EBDA. Sometimes this stack
#      could be corrupted by OptionRom image's code.
#	$END------------------------------------------------------------------
#
##

[Defines]
  PLATFORM_NAME                  = ByoModule
  PLATFORM_GUID                  = D9FBEF68-0995-47A4-9EA8-73FAD163715E
  PLATFORM_VERSION               = 0.01
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ByoModule
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  
[LibraryClasses]
  #
  # Entry point
  #
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  
   
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  
  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  
  #
  # Framework
  #
  S3BootScriptLib|MdePkg\Library\BaseS3BootScriptLibNull\BaseS3BootScriptLibNull.inf
  
  #
  # TPM Library
  #
  TpmCommLib|ByoModulePkg\Library\TpmCommLibNull\TpmCommLibNull.inf
  
  #
  # Misc
  #
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf  
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  
  
[LibraryClasses.common.PEIM]
  BiosIdLib|ByoModulePkg/Library/BiosIdLib/Pei/BiosIdPeiLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  BiosIdLib|ByoModulePkg/Library/BiosIdLib/Dxe/BiosIdDxeLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
 
[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  
###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
[Components]

  ByoModulePkg/Library/BiosIdLib/Dxe/BiosIdDxeLib.inf
  ByoModulePkg/Library/BiosIdLib/Pei/BiosIdPeiLib.inf
  ByoModulePkg/Library/TpmCommLibNull/TpmCommLibNull.inf
  
  #
  # USB Modules for Legacy, Native and Crisis Recovery
  #
  ByoModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  ByoModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  ByoModulePkg/Bus/Pci/XhciPei/XhciPei.inf
  ByoModulePkg/Bus/Pci/OhciDxe/OhciDxe.inf
  ByoModulePkg/Bus/Pci/OhciPei/OhciPei.inf
  ByoModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  ByoModulePkg/Bus/Usb/LegacyFreeKbDxe/LegacyFreeKbDxe.inf
  ByoModulePkg/Bus/Usb/LegacyFreeMsDxe/LegacyFreeMsDxe.inf
  ByoModulePkg/Bus/Usb/LegacyUsbSmm/LegacyUsbSmm.inf
  ByoModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf

  #
  # ATA Bus Module for Crisis Recovery
  #
  ByoModulePkg/Bus/Ata/AtaBusPei/AtaBusPei.inf
  ByoModulePkg/Universal/Disk/CDExpressPei/CdExpressPei.inf
  ByoModulePkg/Universal/Disk/FatPei/FatPei.inf
  
  #
  # Crisis Recovery Modules
  #
  ByoModulePkg/CrisisRecovery/FlashUpdateDxe/FlashUpdateDxe.inf
  ByoModulePkg/CrisisRecovery/ModuleRecoveryPei/ModuleRecoveryPei.inf
  ByoModulePkg/CrisisRecovery/RecoveryJudgePei/IsRecoveryPei.inf
  
  #
  # SD Modules for Legacy and Native
  #
  ByoModulePkg/Bus/Pci/SdHostDxe/SdHostDxe.inf
  ByoModulePkg/Bus/Sd/LegacySdSmm/SdLegacySmm.inf
  ByoModulePkg/Bus/Sd/MediaDeviceDxe/MediaDeviceDxe.inf
  
  #
  # HDD Password Modules
  #
  ByoModulePkg/Security/HddPassword/Dxe/HddPasswordDxe.inf
  ByoModulePkg/Security/HddPassword/Pei/HddPasswordPei.inf
  ByoModulePkg/Security/HddPassword/Smm/HddPasswordSmm.inf
  
  #
  # TPM Modules
  #
  ByoModulePkg/Security/Tpm/MemoryOverwriteControlSmm/MemoryOverwriteControSmm.inf
  ByoModulePkg/Security/Tpm/PhysicalPresenceSmm/PhysicalPresenceSmm.inf
  #ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmm16.inf
  ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmm.inf
  ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmmInstallInt1A.inf
  
  #
  # Setup
  #
  ByoModulePkg/Setup/TextBrowserDxe/SetupBrowserDxe.inf
  
  #
  # Graphics Modules
  #
  ByoModulePkg/Graphics/JpegDecoderDxe/JpegDecoder.inf
  
