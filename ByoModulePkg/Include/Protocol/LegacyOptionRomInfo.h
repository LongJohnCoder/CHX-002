
#ifndef __LEGACY_OPROM_INFO_H__
#define __LEGACY_OPROM_INFO_H__

typedef struct {
  UINT8  Bus;
  UINT8  Dev;
  UINT8  Func;
  UINT8  ClassCode[3];
  UINT8  *OpromData;
  UINTN  OpromDataSize;
} LEGACY_OPROM_INFO;


extern EFI_GUID gLegacyOptionRomInfoProtocolGuid;

#endif