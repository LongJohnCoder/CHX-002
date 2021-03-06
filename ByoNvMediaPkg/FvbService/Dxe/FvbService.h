/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  FvbService.h

Abstract: 

Revision History:

**/
//
// This file contains an 'Intel Peripheral Driver' and is
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may
// be modified by the user, subject to additional terms of the
// license agreement
//
/*++

Copyright (c) 2010, Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  FvbService.h

Abstract:

  Firmware volume block driver for FWH or SPI device.
  It depends on which Flash Device Library to be linked with this driver.

--*/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H


#include <PiDxe.h>

#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Protocol/NvMediaAccess.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>

#define SF_SECTOR_SIZE    0x1000

//
// Define two helper macro to extract the Capability field or Status field in FVB
// bit fields
//
#define EFI_FVB2_CAPABILITIES (EFI_FVB2_READ_DISABLED_CAP | \
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )

#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)


typedef struct {
    UINTN                       FvBase;
    UINTN                       NumOfBlocks;
    //
    // Note!!!: VolumeHeader must be the last element
    // of the structure.
    //
    EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
} EFI_FW_VOL_INSTANCE;


typedef struct {
    EFI_FW_VOL_INSTANCE         *FvInstance;
    UINT32                      NumFv;
} FWB_GLOBAL;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE       SIGNATURE_32('F','V','B','C')

typedef struct {
    MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
    EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
    MEMMAP_DEVICE_PATH          MemMapDevPath;
    EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
    UINT32                                Signature;
    EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
    UINTN                                 Instance;
    EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
    IN  EFI_PHYSICAL_ADDRESS              FvBaseAddress,
    OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
);

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    OUT EFI_FVB_ATTRIBUTES_2                      *Attributes
);

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN OUT EFI_FVB_ATTRIBUTES_2                   *Attributes
);

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    OUT EFI_PHYSICAL_ADDRESS               *Address
);

EFI_STATUS
FvbProtocolGetBlockSize (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
    IN  EFI_LBA                            Lba,
    OUT UINTN                              *BlockSize,
    OUT UINTN                              *NumOfBlocks
);

EFI_STATUS
EFIAPI
FvbProtocolRead (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    OUT UINT8                                *Buffer
);

EFI_STATUS
EFIAPI
FvbProtocolWrite (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
    IN EFI_LBA                              Lba,
    IN UINTN                                Offset,
    IN OUT UINTN                            *NumBytes,
    IN UINT8                                *Buffer
);

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
    IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
    ...
);

#endif

