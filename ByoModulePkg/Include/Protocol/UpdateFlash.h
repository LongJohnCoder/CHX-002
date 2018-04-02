/*++

Module Name:

  UpdateFlash.h

Abstract:
	This file provide a protocol for update flash processing under 
	recovery mode or flash update mode

--*/

#ifndef _UPDATE_FLASH_H_
#define _UPDATE_FLASH_H_

#define UPDATE_FLASH_PROTOCOL_GUID \
		{ \
			0xC647D33A, 0x2977, 0x464F,0xFF, 0xFF, 0xFF, 0xFF, 0x35, 0x80, 0x10, 0x97 \
		}
		
typedef
EFI_STATUS
(EFIAPI *PROCESS_FLASH) (
  EFI_BOOT_MODE     BootMode
  );


typedef struct {
  PROCESS_FLASH   ProcessFlash;
} UPDATE_FLASH_PROTOCOL;

extern EFI_GUID gUpdateFlashProtocolGuid;		

#endif