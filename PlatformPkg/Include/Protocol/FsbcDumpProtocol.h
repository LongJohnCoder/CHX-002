/** @file
  CPU Architectural Protocol as defined in PI spec Volume 2 DXE

  This code abstracts the DXE core from processor implementation details.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _FSBC_DUMP_PROTOCOL_H_
#define _FSBC_DUMP_PROTOCOL_H_

//
// Global ID for the EFI_FSBC_DUMP_PROTOCOL_GUID.
//

#define EFI_FSBC_DUMP_PROTOCOL_GUID \
  { \
    0xCF5B5993,  0x992C, 0x4B3A, {0x8D, 0x1A, 0xC5, 0x81, 0x80, 0xD1, 0x14, 0xB4} \
  }

typedef struct _EFI_FSBC_DUMP_PROTOCOL EFI_FSBC_DUMP_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_EN_FSBC)(
  IN EFI_FSBC_DUMP_PROTOCOL  *This
  );

struct _EFI_FSBC_DUMP_PROTOCOL {
  EFI_EN_FSBC  			EnableFsbcSnapShotMode;
};

extern EFI_GUID gEfiFsbcDumpProtocolGuid;

#endif
