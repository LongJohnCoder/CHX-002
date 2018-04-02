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

#ifndef _BYO_PLATFORM_BDS_H_
#define _BYO_PLATFORM_BDS_H_

//why #include "EfiHii.h"

#define BYO_PLATFORM_BDS_PROTOCOL_GUID \
  { \
    0xD8242D52, 0x9AA6, 0x47AB, 0x97, 0x78, 0x7A, 0x21, 0x1E, 0x9B, 0xB5, 0x47 \
  }

//why //
//why // Forward reference for pure ANSI compatability
//why //
//why EFI_FORWARD_DECLARATION (BYO_PLATFORM_BDS_PROTOCOL);
//why 
//why typedef struct _BYO_PLATFORM_BDS_PROTOCOL BYO_PLATFORM_BDS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *BYO_BDS_BOOTMAINT) (
  );


typedef struct _BYO_PLATFORM_BDS_PROTOCOL {
  BYO_BDS_BOOTMAINT          BdsBootMaint;
} BYO_PLATFORM_BDS_PROTOCOL;

extern EFI_GUID gByoPlatformBdsProtocolGuid;

#endif
