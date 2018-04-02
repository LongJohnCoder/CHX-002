/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SdLegacy.h

Abstract:
  SD card Model.

Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-16
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE: SD card feature support.
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/

#ifndef _SDLEGACY_H_
#define _SDLEGACY_H_

#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SDHostIo.h>
#include <Protocol/LegacySDInf.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Pci22.h>
#include <Protocol/SmmSwDispatch2.h>

// {968D9BB9-E625-47c7-831E-00B7F9029514}
#define EFI_SD_CARD_DATA_GUID \
  {\
    0x968d9bb9, 0xe625, 0x47c7, 0x83, 0x1e, 0x0, 0xb7, 0xf9, 0x2, 0x95, 0x14\
  }

extern EFI_GUID gSdCardDataGuid;

#define SMM_PCI_ADDRESS(bus, dev, func, reg) \
    ((UINT64) ((((UINTN) bus) << 24) + (((UINTN) dev) << 16) + (((UINTN) func) << 8) + ((UINTN) reg)))

typedef struct {
    UINT8   Register;
    UINT8   Function;
    UINT8   Device;
    UINT8   Bus;
    UINT32  ExtendedRegister;
} SMM_PCI_IO_ADDRESS;

#define PCI_SDIO_CLASS1                  0x08050000
#define PCI_SDIO_CLASS2                  0x08050100

#define   MAX_PCI_BUSS                   10
#define   MAX_PCI_DEVICES                32
#define   MAX_PCI_FUNCTIONS              8

//#include EFI_PROTOCOL_PRODUCER (LegacySDInf)

#define SD_LEGACY_SMI_VALUE               0x90
#define SD_SMI_VALUE                      0x91

#define BBS_SDMEM_RESERVATION_START_INDEX 0x38

#define BBS_DO_NOT_BOOT_FROM              0xFFFC
#define BBS_LOWEST_PRIORITY               0xFFFD
#define BBS_UNPRIORITIZED_ENTRY           0xFFFE
#define BBS_IGNORE_ENTRY                  0xFFFF

//
// define BBS Device Types
//
#define BBS_FLOPPY                        0x01
#define BBS_HARDDISK                      0x02
#define BBS_CDROM                         0x03
#define BBS_PCMCIA                        0x04
#define BBS_USB                           0x05
#define BBS_EMBED_NETWORK                 0x06
#define BBS_BEV_DEVICE                    0x80
#define BBS_UNKNOWN                       0xff

//
// Fixed Disk Error Codes
//
#define BE_NO_SENSE                       0x00  // No sense data
#define BE_BAD_COMMAND                    0x01  // Bad Command send to BIOS
#define BE_WRITE_PROTECT                  0x03  // Write Protect Fault
#define BE_BAD_ADDRESS_MARK               0x02  // Data Address Mark Not Found
#define BE_RECORD_NOT_FOUND               0x04  // ID field for sec not found
#define BE_RESET_ERROR                    0x05  // Hard Reset Failed
#define BE_MEDIA_CHANGE                   0x06  // Media Change
#define BE_INIT_ERROR                     0x07  // Bad drive # on get params
#define BE_BAD_BLOCK                      0x0A  // Bad block mark detected
#define BE_ECC_ERROR                      0x10  // Uncorrectable ECC occurred
#define BE_CORRECTED_DATA                 0x11  // Data corrected in ECC
#define BE_CONTROLLER_ERROR               0x20  // Controller failed diagnostics
#define BE_BAD_SEEK                       0x40  // Track 0 line not true
#define BE_TIMEOUT                        0x80  // Timeout error occurred
#define BE_CD_READ_ERROR                  0x90  // Error reading the CD-ROM boot info
#define BE_DRIVE_NOT_READY                0xAA  // Drive Not Ready
#define BE_INVALID_ERROR                  0xBB  // Undocumented error
#define BE_WRITE_FAULT                    0xCC  // Write Fault
#define BE_NO_ERROR                       0xE0  // No detectable error

#define FIRST_PARTITION_OFFSET            0x1be
#define PARTITION_SIG1_OFFSET             0x1fe
#define PARTITION_SIG2_OFFSET             0x1ff

#pragma pack(push, 1)

typedef struct _PARTITION_AREA {
  UINT8   BootIndicator;
  UINT8   StartingHead;
  //UINT16  StartingSecCyl;
  UINT8    StartSector;
  UINT8    StartCyl;
  UINT8   SystemID;
  UINT8   EndingHead;
  //UINT16  EndingsecCyl;
  UINT16  EndSector:6;
  UINT16  EndCyl:10;
  UINT32  PartitionStartLBA;
  UINT32  PartitionTotalLBA;
} PARTITION_AREA;

typedef struct _SD_MEMORY_CARD_INFO {
  CARD_DATA         *SDCardData;
  BOOLEAN             BootAsFixedDrive;
  BOOLEAN             BootAsRemovableDrive;
  UINT16              BytesPerSector;
  UINT8               TotalHeads;
  UINT8               SectorsPerHead;
  UINT16              Cylinder;
  UINT64              TotalLBA;
} SD_MEMORY_CARD_INFO;

typedef struct {
    UINT16    CF : 1;
    UINT16    Reserved1 : 1;
    UINT16    PF : 1;
    UINT16    Reserved2 : 1;
    UINT16    AF : 1;
    UINT16    Reserved3 : 1;
    UINT16    ZF : 1;
    UINT16    SF : 1;
    UINT16    TF : 1;
    UINT16    IF : 1;
    UINT16    DF : 1;
    UINT16    OF : 1;
    UINT16    IOPL : 2;
    UINT16    NT : 1;
    UINT16    Reserved4 : 1;
} FlagRegister;

typedef struct {
    UINT32    edi;
    UINT32    esi;
    UINT32    ebp;
    UINT32    esp;
    UINT8     bl, bh;
    UINT16    hWordEbx;
    UINT8     dl, dh;
    UINT16    hWordEdx;
    UINT8     cl, ch;
    UINT16    hWordEcx;
    UINT8     al, ah;
    UINT16    hWordEax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
} byteRegisterStackFrame;

typedef struct {
    UINT16    di;
    UINT16    hWordEdi;
    UINT16    si;
    UINT16    hWordEsi;
    UINT32    ebp;
    UINT32    esp;
    UINT16    bx;
    UINT16    hWordEbx;
    UINT16    dx;
    UINT16    hWordEdx;
    UINT16    cx;
    UINT16    hWordEcx;
    UINT16    ax;
    UINT16    hWordEax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
} wordRegisterStackFrame;

typedef struct {
    UINT32    edi;
    UINT32    esi;
    UINT32    ebp;
    UINT32    esp;
    UINT32    ebx;
    UINT32    edx;
    UINT32    ecx;
    UINT32    eax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
}dwordRegisterStackFrame;

typedef union{
    byteRegisterStackFrame    L;
    wordRegisterStackFrame    H;
    dwordRegisterStackFrame   X;
}registerStackFrame;

typedef struct {
    UINT8    bSize;
    UINT8    bRsvd;
    UINT16   wNumOfTran;
    UINT32   dwBuf;
    UINT64   qwLba;
} INT13_DISK_PACKET;

typedef struct {
    UINT16    wSize;
    UINT16    wInforFlag;
    UINT32    dwCylinder;
    UINT32    dwHeaders;
    UINT32    dwSector;
    UINT64    qwSectos;
    UINT16    wBytesInSector;
    VOID       * lpDpte;
} RESULT_BUF;

typedef struct {
  UINT16  OldPosition : 4;
  UINT16  Reserved1 : 4;
  UINT16  Enabled : 1;
  UINT16  Failed : 1;
  UINT16  MediaPresent : 2;
  UINT16  Reserved2 : 4;
} BBS_STATUS_FLAGS;

typedef struct {
  UINT16            BootPriority;
  UINT32            Bus;
  UINT32            Device;
  UINT32            Function;
  UINT8             Class;
  UINT8             SubClass;
  UINT16            MfgStringOffset;
  UINT16            MfgStringSegment;
  UINT16            DeviceType;
  BBS_STATUS_FLAGS  StatusFlags;
  UINT16            BootHandlerOffset;
  UINT16            BootHandlerSegment;
  UINT16            DescStringOffset;
  UINT16            DescStringSegment;
  UINT32            InitPerReserved;
  UINT32            AdditionalIrq13Handler;
  UINT32            AdditionalIrq18Handler;
  UINT32            AdditionalIrq19Handler;
  UINT32            AdditionalIrq40Handler;
  UINT8             AssignedDriveNumber;
  UINT32            AdditionalIrq41Handler;
  UINT32            AdditionalIrq46Handler;
  UINT32            IBV1;
  UINT32            IBV2;
} BBS_TABLE;

#pragma pack(pop)

#endif
