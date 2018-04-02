/** @file
Private Include file for IdeBus PEIM.

Copyright (c) 2006 - 2010, BYOSoft Corporation. All rights
reserved.<BR> This software and associated documentation (if
any) is furnished under a license and may only be used or copied
in accordance with the terms of the license. Except as permitted
by such license, no part of this software or documentation may
be reproduced, stored in a retrieval system, or transmitted in
any form or by any means without the express written consent of
BYOSoft Corporation.


Module Name:

    AtaPeim.h

Abstract:


Revision History
**/
#ifndef _ATA_PEIM_H_
#define _ATA_PEIM_H_

#include <PiPei.h>
#include <Ppi\BlockIo.h>
#include <Ppi\AtaController.h>
#include <Base.h>
#include <Library\DebugLib.h>


#define MAX_IDE_CHANNELS    4  // Ide and Sata Primary, Secondary Channel.
#define MAX_IDE_DEVICES     8  // Ide, Sata Primary, Secondary and Master, Slave device.

//
// Time Out Value For IDE Device Polling
//
// ATATIMEOUT is used for waiting time out for ATA device
//
#define ATATIMEOUT  1000  // 1 second

#define STALL_1_MILLI_SECOND  1000  // stall 1 ms

#define ATA_BLK_IO_DEV_SIGNATURE  SIGNATURE_32 ('a', 't', 'i', 'o')

#define PEI_RECOVERY_ATA_FROM_BLKIO_THIS(a) CR (a, ATA_BLK_IO_DEV, AtaBlkIo, ATA_BLK_IO_DEV_SIGNATURE)

typedef struct {
  UINTN                   DevicePosition;
  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
} PEI_ATAPI_DEVICE_INFO;

typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  IdeMaster     = 0,
  IdeSlave      = 1,
  IdeMaxDevice  = 2
} EFI_IDE_DEVICE;


//
// IDE Registers
//
typedef union {
  UINT16  Command;        /* when write */
  UINT16  Status;         /* when read */
} IDE_CMD_OR_STATUS;

typedef union {
  UINT16  Error;          /* when read */
  UINT16  Feature;        /* when write */
} IDE_ERROR_OR_FEATURE;

typedef union {
  UINT16  AltStatus;      /* when read */
  UINT16  DeviceControl;  /* when write */
} IDE_ALTSTATUS_OR_DEVICECONTROL;


//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  IDE_ERROR_OR_FEATURE            Reg1;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  IDE_CMD_OR_STATUS               Reg;

  IDE_ALTSTATUS_OR_DEVICECONTROL  Alt;
  UINT16                          DriveAddress;
} IDE_BASE_REGISTERS;


typedef struct {
  UINTN                           Signature;

  EFI_PEI_RECOVERY_BLOCK_IO_PPI   AtaBlkIo;
  EFI_PEI_PPI_DESCRIPTOR          PpiDescriptor;
  PEI_ATA_CONTROLLER_PPI          *AtaControllerPpi;

  UINTN                           DeviceCount;
  PEI_ATAPI_DEVICE_INFO           DeviceInfo[MAX_IDE_DEVICES];   //for max 8 device
  IDE_BASE_REGISTERS              IdeIoPortReg[MAX_IDE_CHANNELS]; //for max 4 channel.
} ATA_BLK_IO_DEV;

typedef struct _EFI_ATA_COMMAND_BLOCK {
  UINT8 Reserved1[2];
  UINT8 AtaCommand;
  UINT8 AtaFeatures;
  UINT8 AtaSectorNumber;
  UINT8 AtaCylinderLow;
  UINT8 AtaCylinderHigh;
  UINT8 AtaDeviceHead;
  UINT8 AtaSectorNumberExp;
  UINT8 AtaCylinderLowExp;
  UINT8 AtaCylinderHighExp;
  UINT8 AtaFeaturesExp;
  UINT8 AtaSectorCount;
  UINT8 AtaSectorCountExp;
  UINT8 Reserved2[6];
} EFI_ATA_COMMAND_BLOCK;

#endif
