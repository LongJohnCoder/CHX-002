/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtIo.h

Abstract:

--*/

#ifndef _BYO_SETUP_BROWSER_H_
#define _BYO_SETUP_BROWSER_H_

#define BYO_SETUP_BROWSER_PROTOCOL_GUID \
  { \
    0x1693f046, 0x1f22, 0x4445, 0xbe, 0x5c, 0xa4, 0x4d, 0x7c, 0xe5, 0xc3, 0xa3 \
  }


typedef
EFI_STATUS
(EFIAPI *BYO_CREATE_DIALOG) (
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *KeyValue,
  IN  CHAR16                          *String,
  ...
  );

typedef
UINTN
(EFIAPI *BYO_PRINT_STRING) (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       *String
  );

typedef struct _BYO_SETUP_BROWSER_PROTOCOL {
  BYO_CREATE_DIALOG CreateDialog;
  BYO_PRINT_STRING  PrintStr;
} BYO_SETUP_BROWSER_PROTOCOL;

extern EFI_GUID gByoSetupBrowserProtocolGuid;

#endif
