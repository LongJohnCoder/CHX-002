/** @file
  TPM instance guid, used for PcdTpmInstanceGuid.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TPM_INSTANCE_GUID_H__
#define __TPM_INSTANCE_GUID_H__

#define TPM_DEVICE_NULL           0
#define TPM_DEVICE_1_2            1
#define TPM_DEVICE_2_0_DTPM       2

#define TPM_DEVICE_SELECTED_GUID  \
  { 0x7f4158d3, 0x74d, 0x456d, { 0x8c, 0xb2, 0x1, 0xf9, 0xc8, 0xf7, 0x9d, 0xaa } }

extern EFI_GUID  gEfiTpmDeviceSelectedGuid;





#endif

