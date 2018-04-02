## @file
#
# Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
# All rights reserved.This software and associated documentation (if any)
# is furnished under a license and may only be used or copied in
# accordance with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be reproduced,
# stored in a retrieval system, or transmitted in any form or by any
# means without the express written consent of Byosoft Corporation.
#
# File Name:
#   ByoNvMediaPkg.dsc
#
# Abstract:
#
# Revision History:
# 
# Bug 2501:  Support NV Media Access function on UNC codebase.
# TIME:       2011-7-19
# $AUTHOR:    Xin Shujun
# $REVIEWERS:  
# $SCOPE:     Sugar Bay Customer Refernce Board.
# $TECHNICAL: 
#   Unify NV Media Access interface, include Flash chip and CMOS.
#   Consumers are Variable service Flash utility, etc.
# $END--------------------------------------------------------------------
#
##

[Defines]
  PLATFORM_NAME                  = ByoNvMedia
  PLATFORM_GUID                  = 2620A963-2E8F-4dd8-A4DF-42B8139AD8E2
  PLATFORM_VERSION               = 1.00
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ByoNvMedia
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses]

  SpiFlashLib|ByoNvMediaPkg/Library/PchSpiFlashLib/PchSpiFlashLib.inf
  
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
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  
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
  # Misc
  #
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  
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
    
[Components]
  ByoNvMediaPkg/FlashDevice/ATMEL25DF161/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/ATMEL25DF161/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/ATMEL26DF321/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/ATMEL26DF321/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/CMOS/Dxe/Cmos.inf
  ByoNvMediaPkg/FlashDevice/CMOS/Smm/Cmos.inf
  ByoNvMediaPkg/FlashDevice/MXIC25L16/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/MXIC25L16/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/NumonyxM25PX16/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/NumonyxM25PX16/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST25VF016B/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST25VF016B/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q16/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q16/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q32/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q32/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q64/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q64/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q128/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/WINBOND25Q128/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FvbService/Dxe/FvbService.inf
  ByoNvMediaPkg/FvbService/Smm/FvbService.inf
  ByoNvMediaPkg/Library/IchTptSpiFlashLib/IchTptSpiFlashLib.inf
  ByoNvMediaPkg/Library/IvbSpiFlashLib/IvbSpiFlashLib.inf
  ByoNvMediaPkg/Library/PchSpiFlashLib/PchSpiFlashLib.inf
  ByoNvMediaPkg/Library/SpiFlashLibNull/SpiFlashLibNull.inf
  ByoNvMediaPkg/NvMediaAccess/Dxe/NvMediaAccess.inf
  ByoNvMediaPkg/NvMediaAccess/Smm/NvMediaAccess.inf
  ByoNvMediaPkg/PlatformAccess/Dxe/PlatformAccess.inf
  ByoNvMediaPkg/PlatformAccess/Smm/PlatformAccess.inf
