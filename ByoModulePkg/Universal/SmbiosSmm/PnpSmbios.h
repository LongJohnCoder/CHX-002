/*++
====================================================================
      NOTICE: Copyright (c) 2006 - 2012 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
====================================================================
Module Name:

  PnpSmbios.h

Abstract:

Revision History:

--*/

#ifndef _PNP_SMBIOS_H_
#define _PNP_SMBIOS_H_

#include <SmBiosSmm.h>
#include <Guid/SmBios.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SetMemAttributeSmmLib.h>
#include <../PnpCommand.h>

#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE        PcdGet32 (PcdFlashNvStorageSmbiosBase)
#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE        PcdGet32 (PcdFlashNvStorageSmbiosSize) / 2
#define SMBIOS_BUFFER_SIZE        4096
#define END_HANDLE                0xFFFF
#define SMBIOS_REC_SIGNATURE      0x55AA

#pragma pack(1)
typedef struct {
    UINT16 Signature;
    UINT16 RecordLen;
} SMBIOS_REC_HEADER;

//
// Please note that SMBIOS structures can be odd byte aligned since the
//  unformated section of each record is a set of arbitrary size strings.
//
typedef struct {
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} SMBIOS_HEADER;
//
// Params list for PNP function 50-52
//
typedef struct {
  UINT16 Function;
  UINT32 DmiBiosRevisionPtr;
  UINT32 NumStructuresPtr;
  UINT32 StructureSizePtr;
  UINT32 DmiStorageBasePtr;
  UINT32 DmiStorageSizePtr;
  UINT16 BiosSelector;
} PNP_PARAMS_50;

typedef struct {
  UINT16 Function;
  UINT32 StructurePtr;
  UINT32 DmiStrucBufferPtr;
  UINT16 DmiSelector;
  UINT16 BiosSelector;
} PNP_PARAMS_51;

typedef struct {
  UINT16 Function;
  UINT32 DmiDataBufferPtr;
  UINT32 DmiWorkBufferPtr;
  UINT8   Control;
  UINT16 DmiSelector;
  UINT16 BiosSelector;
} PNP_PARAMS_52;

typedef struct {
  UINT8 Command;
  UINT8 FieldOffset;
  UINT32 ChangeMask;
  UINT32 ChangeValue;
  UINT16 DataLength;
  SMBIOS_HEADER StructureHeader;
  UINT8 StructureData[1];
} PNP_52_DATA_BUFFER;

#define EFI_PNP_52_SIGNATURE  SIGNATURE_32('_','p','n','p')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;
  SMBIOS_REC_HEADER   header;
  PNP_52_DATA_BUFFER  *pRecord;
} PNP_52_RECORD;

typedef struct {
  EFI_SMBIOS_TABLE_HEADER  Header;
  UINT8                    Tailing[2];
} EFI_SMBIOS_TABLE_END_STRUCTURE;

#pragma pack()

UINT16
PnpGetSmbiosInformation (
  IN VOID *ParameterBuffer
);

UINT16
PnpGetSmbiosStructure(
  IN VOID *ParameterBuffer
);

UINT16
PnpSetSmbiosStructure (
  IN VOID *ParameterBuffer
);

EFI_STATUS 
Mem2RecordList (
  IN UINT8 *pBase, 
  IN UINTN size
);

VOID 
GetFromFlash ();

#endif

