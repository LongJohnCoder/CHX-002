/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ByoPlatformSetup.h

Abstract:

--*/

#ifndef _BYO_PLATFORM_SETUP_H_
#define _BYO_PLATFORM_SETUP_H_


#define BYO_PLATFORM_SETUP_PROTOCOL_GUID \
  { \
    0x44b0dc51, 0xaf0e, 0x4dc5, 0x88, 0xc1, 0xff, 0x4f, 0x39, 0x69, 0xbb, 0xa4 \
  }
  

#define FORMSET_STANDARD_BIOS_GUID    \
  { \
    0xa4f3e22a, 0x541, 0x401f, 0xa8, 0xa2, 0xd8, 0x31, 0x61, 0xcf, 0xb5, 0x9e \
  }

#define FORMSET_ADVANCED_BIOS_GUID    \
  { \
    0x7a0cc1fc, 0xa02c, 0x45b8, 0xa7, 0x38, 0xa8, 0x76, 0xdc, 0x28, 0x73, 0xe8 \
  }

#define FORMSET_ADVANCED_CHIPSET_GUID    \
  { \
    0xd1de1061, 0x1753, 0x4e7a, 0x89, 0xe8, 0x94, 0xc3, 0xf7, 0x73, 0xfc, 0xcc \
  }

#define FORMSET_BOOT_CONFIG_GUID    \
  { \
    0x2147959d, 0xf051, 0x4901, 0x9a, 0x9d, 0xf2, 0x2f, 0xd, 0x49, 0x1c, 0xfd \
  }

#define FORMSET_POWER_MANAGER_GUID    \
  { \
    0x26e33c1f, 0xdea1, 0x418b, 0x83, 0xf4, 0x96, 0xa4, 0x70, 0x2a, 0x39, 0x1b \
  }

#define FORMSET_PNP_PCI_GUID    \
  { \
    0x3aca1970, 0xbaf9, 0x467b, 0x91, 0xe, 0xc1, 0xb9, 0x13, 0xf6, 0x21, 0xc \
  }

#define FORMSET_PC_HEALTH_GUID    \
  { \
    0x9bee9ff2, 0x82a1, 0x4562, 0x9e, 0xc6, 0xbe, 0x6f, 0x51, 0xc3, 0xa0, 0xba \
  }

#define FORMSET_BIOS_SECURITY_GUID    \
  { \
    0x6537a20c, 0x3511, 0x4341, 0x98, 0xb8, 0x72, 0x75, 0xb5, 0x33, 0x6e, 0xae \
  }


typedef
EFI_STATUS
(EFIAPI *BYO_LOAD_DEFALUT) (
  );

typedef
EFI_STATUS
(EFIAPI *BYO_EXIT_WITH_SAVE) (
  );

typedef
EFI_STATUS
(EFIAPI *BYO_EXIT_WITHOUT_SAVE) (
  );

typedef
EFI_STATUS
(EFIAPI *BYO_PCHEALTH_UPDATE)(
  IN EFI_HII_HANDLE                          HiiHandle
  );

typedef struct _BYO_PLATFORM_SETUP_PROTOCOL {
  BYO_LOAD_DEFALUT      LoadDefault;
  BYO_EXIT_WITH_SAVE    ExitWithSave;
  BYO_EXIT_WITHOUT_SAVE ExitWithoutSave;
  BYO_PCHEALTH_UPDATE   UpdatePcHealth;
} BYO_PLATFORM_SETUP_PROTOCOL;

extern EFI_GUID gByoPlatformSetupProtocolGuid;
extern EFI_GUID gStandardBiosFormsetGuid;
extern EFI_GUID gAdvancedBiosFormsetGuid;
extern EFI_GUID gAdvancedChipsetFormsetGuid;
extern EFI_GUID gBootConfigFormsetGuid;
extern EFI_GUID gPowerManagerFormsetGuid;
extern EFI_GUID gPnpPciFormsetGuid;
extern EFI_GUID gPcHealthFormsetGuid;
extern EFI_GUID gBiosSecurityFormsetGuid;

#endif
