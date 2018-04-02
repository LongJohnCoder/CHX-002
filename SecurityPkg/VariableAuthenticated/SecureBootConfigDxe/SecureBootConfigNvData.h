/** @file
  Header file for NV data structure definition.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SECUREBOOT_CONFIG_NV_DATA_H__
#define __SECUREBOOT_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/SecureBootConfigHii.h>

#define SECUREBOOT_CONFIGURATION_VARSTORE_ID  0x0002
#define SECUREBOOT_CONFIGURATION_FORM_ID      0x1000

#define KEY_SECURE_BOOT_ENABLE                0x1000
#define KEY_SECURE_BOOT_RESET                 0x1001
#define KEY_SECURE_BOOT_RESTORE               0x1002

typedef struct {
  UINT8    SecureBootState; // Secure Boot Disable/Enable;
  UINT8    SetupMode;       // Setup/User
} SECUREBOOT_CONFIGURATION;

#endif
