/*++
Copyright (c) 2011 Byosoft Corporation. All rights reserved.

Module Name:

  SmiFlash.h

Abstract:

 This file contains the Includes, Definitions, typedefs,
 Variable and External Declarations, Structure and
 function prototypes needed for the SmiFlash driver
--*/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SetMemAttributeSmmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/Smbios.h>
#include <Library/BiosIdLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>

#define SW_SMI_FLASH_SERVICES	                0xec
#define   FUNC_UPDATE_SMBIOS_DATA               0x05
#define   FUNC_PROGRAM_FLASH                    0x09
#define   FUNC_PREPARE_BIOS_FLASH_ENV           0x0B
#define   FUNC_CLEAR_BIOS_FLASH_ENV             0x0C
#define   FUNC_CHECK_BIOS_ID                    0x0D


#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE        PcdGet32 (PcdFlashNvStorageSmbiosBase)
#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE        PcdGet32 (PcdFlashNvStorageSmbiosSize)
#define SMBIOS_BUFFER_SIZE        4096
#define END_HANDLE                0xFFFF
#define SMBIOS_REC_SIGNATURE      0x55AA
#define MAX_STRING_LENGTH         30


#pragma pack(1)
typedef struct {
    UINT16 Signature;
    UINT16 RecordLen;
} SMBIOS_REC_HEADER;

typedef struct {
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} SMBIOS_HEADER;

typedef struct {
  UINT8 Command;
  UINT8 FieldOffset;
  UINT32 ChangeMask;
  UINT32 ChangeValue;
  UINT16 DataLength;
  SMBIOS_HEADER StructureHeader;
  UINT8 StructureData[1];
} PNP_52_DATA_BUFFER;

typedef struct {
  UINT8               SubFun;
  PNP_52_DATA_BUFFER  Parameter;
} UPDATE_SMBIOS_PARAMETER;

#define EFI_PNP_52_SIGNATURE  SIGNATURE_32('_','p','n','p')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;
  SMBIOS_REC_HEADER   header;
  PNP_52_DATA_BUFFER  *pRecord;
} PNP_52_RECORD;

///
/// Identifies the structure-setting operation to be performed.
///
typedef enum {
  ByteChanged,
  WordChanged,
  DoubleWordChanged,
  AddChanged,
  DeleteChanged,
  StringChanged,
  BlockChanged,
  Reseved
} FUNC52_CMD;

typedef enum {
    UPDATE_UUID = 1,
    UPDATE_SERIAL_NUMBER,
    UPDATE_CHASSIS_ASSET_TAG,
    UPDATE_MODEL_NUMBER,
    UPDATE_BRAND_ID,
    UPDATE_LOCK_STATUS,
    UPDATE_BASE_BOARD_SERIAL_NUMBER,
    UPDATE_BASE_BOARD_ASSET_TAG,
    UPDATE_CHASSIS_SERIAL_NUMBER
} UPDATE_SMBIOS_TYPE;


typedef struct {
    UINT32    BiosAddr;
    UINT32    Size;
    UINT32    Buffer;
} BIOS_UPDATE_BLOCK_PARAMETER;



/*
EFI_CAPSULE_HEADER
  CapsuleGuid         // 16
  HeaderSize          // 4
  Flags               // 4
  CapsuleImageSize    // 4
-------------------------------------------------------------
  (+) PubkeySize      // 4            +28
  (+) SignSize        // 4            +32
  (+) RangeArraySize  // 4            +36

  (+) Pubkey          //              +40
  (+) Sign            //              +40+PubkeySize            align 4
  (+) Range[]         //              +40+PubkeySize+SignSize   align 4
-------------------------------------------------------------
FD                    // 16 align
*/

#pragma pack()


EFI_STATUS HandleSmbiosDataRequest(UPDATE_SMBIOS_PARAMETER *SmbiosPtr);
EFI_STATUS AllocDataBuffer();

extern NV_MEDIA_ACCESS_PROTOCOL  *mMediaAccess;




