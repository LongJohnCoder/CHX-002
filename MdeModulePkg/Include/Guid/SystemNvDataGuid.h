/** @file  
  This file defines NvDataFv GUID and FTW working block structure header.
  This guid can be used as FileSystemGuid in EFI_FIRMWARE_VOLUME_HEADER if 
  this FV image contains NV data, such as NV variable data.
  This guid can also be used as the signature of FTW working block header.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SYSTEM_NV_DATA_GUID_H__
#define __SYSTEM_NV_DATA_GUID_H__

#define EFI_SYSTEM_NV_DATA_FV_GUID \
  {0xfff12b8d, 0x7696, 0x4c8b, {0xa9, 0x85, 0x27, 0x47, 0x7, 0x5b, 0x4f, 0x50} }

///
/// An NvDataFv GUID used as the signature of FTW working block header.
///
extern EFI_GUID gEfiSystemNvDataFvGuid;

#define WORKING_BLOCK_VALID   0x1
#define WORKING_BLOCK_INVALID 0x2


#define FTW_VALID_STATE     0
#define FTW_INVALID_STATE   1


#define HEADER_ALLOCATED  0x1
#define WRITES_ALLOCATED  0x2
#define WRITES_COMPLETED  0x4

//
// EFI Fault tolerant block update write queue entry
//
typedef struct {                                   // size: 0x28
  UINT8     HeaderAllocated : 1;
  UINT8     WritesAllocated : 1;
  UINT8     Complete : 1;
  UINT8     Reserved : 5;
  EFI_GUID  CallerId;
  UINT64    NumberOfWrites;
  UINT64    PrivateDataSize;
} EFI_FAULT_TOLERANT_WRITE_HEADER;


///
/// The EDKII Fault tolerant working block header.
/// The header is immediately followed by the write queue data.
///
typedef struct {                                   // size: 0x20
  ///
  /// System Non Volatile FV Guid.
  ///
  EFI_GUID  Signature;
  ///
  /// 32bit CRC caculated for this header.
  ///
  UINT32    Crc;
  ///
  /// Working block valid bit.
  ///
  UINT8     WorkingBlockValid : 1;
  UINT8     WorkingBlockInvalid : 1;
  UINT8     Reserved : 6;
  UINT8     Reserved3[3];
  ///
  /// Total size of the following write queue range.
  ///
  UINT64    WriteQueueSize;
  ///
  /// Write Queue data.
  /// UINT8  WriteQueue[WriteQueueSize];
  ///
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER;




#define BOOT_BLOCK_UPDATE 0x1
#define SPARE_COMPLETED   0x2
#define DEST_COMPLETED    0x4

//
// EFI Fault tolerant block update write queue record
//
typedef struct {                                     // 0x28
  UINT8   BootBlockUpdate : 1;
  UINT8   SpareComplete : 1;
  UINT8   DestinationComplete : 1;
  UINT8   Reserved : 5;
  EFI_LBA Lba;
  UINT64   Offset;
  UINT64   Length;
  EFI_PHYSICAL_ADDRESS  FvBaseAddress;
  //
  // UINT8                PrivateData[PrivateDataSize]
  //
} EFI_FAULT_TOLERANT_WRITE_RECORD;


#define FTW_RECORD_SIZE(PrivateDataSize)  (sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD) + (UINTN) PrivateDataSize)

#define FTW_WRITE_TOTAL_SIZE(NumberOfWrites, PrivateDataSize) \
    ( \
      sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER) + (UINTN) (NumberOfWrites) * \
      (sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD) + (UINTN) PrivateDataSize) \
    )


#endif
