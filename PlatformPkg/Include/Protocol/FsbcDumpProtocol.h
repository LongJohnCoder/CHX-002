//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************

#ifndef _FSBC_DUMP_PROTOCOL_H_
#define _FSBC_DUMP_PROTOCOL_H_

#ifdef ZX_SECRET_CODE

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

#endif
