/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SDHostDriver.h

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
/** @file

Copyright (c) 2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


File Name:

  SDHostDriver.h

Abstract:

  Header file for driver.

**/

#ifndef _SD_HOST_DRIVER_H
#define _SD_HOST_DRIVER_H

#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/SDHostIo.h>
#include <Protocol/LegacySDInf.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Pci22.h>

#include "SDCard.h"

#define SDHOST_DATA_SIGNATURE  SIGNATURE_32 ('s', 'd', 'h', 's')

#define SDHOST_DATA_FROM_THIS(a) \
    CR(a, SDHOST_DATA, SdHostIo, SDHOST_DATA_SIGNATURE)

#define BLOCK_SIZE          0x200
#define TIME_OUT_1S         1000
#define SD_SMI_VALUE        0x91

#define INTEL_VENDOR_ID     0x8086
#define IOH_FUNC_SDIO1      0x8809
#define IOH_FUNC_SDIO2      0x880A

#define BUFFER_CTL_REGISTER 0x84


#pragma pack(1)
//
// PCI Class Code structure
//
typedef struct {
  UINT8 PI;
  UINT8 SubClassCode;
  UINT8 BaseCode;
} PCI_CLASSC;

#pragma pack()

typedef struct {
  UINTN                      Signature;
  EFI_SD_HOST_IO_PROTOCOL    SdHostIo;
  EFI_PCI_IO_PROTOCOL        *PciIo;
  UINT16                     PciVid;
  UINT16                     PciDid;
  BOOLEAN                    IsAutoStopCmd;
  BOOLEAN                    IsEmmc;
  BOOLEAN                    EnableVerboseDebug;
  UINT32                     BaseClockInMHz;
  UINT32                     CurrentClockInKHz; 
  UINT32                     BlockLength;
  EFI_UNICODE_STRING_TABLE   *ControllerNameTable;
  UINT32                     ControllerVersion;
} SDHOST_DATA;

/**
  Starting the SD Host Driver.

  @param  Controller              A pointer to EFI_BLOCK_IO_PROTOCOL.
  @param  PciIo                   A pointer to EFI_PCI_IO_PROTOCOL¡£

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Unsupport.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SdHostDriverStart (
  IN UINT8  BusNum,
  IN UINT8  DeviceNum,
  IN UINT8  FunNum,
  IN UINT32 MemAddress,
  IN UINT32 PciDidVid
  );

/**
  The main function used to send the command to the card inserted into the SD host slot.
  It will assemble the arguments to set the command register and wait for the command
  and transfer completed until timeout. Then it will read the response register to fill
  the ResponseData.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  CommandIndex            The command index to set the command index field of command register.
  @param  Argument                Command argument to set the argument field of command register.
  @param  DataType                TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                  Contains the data read from / write to the device.
  @param  BufferSize              The size of the buffer.
  @param  ResponseType            RESPONSE_TYPE.
  @param  TimeOut                 Time out value in 1 ms unit.
  @param  ResponseData            Depending on the ResponseType, such as CSD or card status.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SendHCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,    
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData OPTIONAL
  );

/**
  Set max clock frequency of the host, the actual frequency
  may not be the same as MaxFrequencyInKHz. It depends on
  the max frequency the host can support, divider, and host
  speed mode.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  MaxFrequency            Max frequency in HZ.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetClockFrequency (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     MaxFrequencyInKHz          
  );

/**
  Set bus width of the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  BusWidth                Bus width in 1, 4, 8 bits.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetBusWidth (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BusWidth          
  );

/**
  Set voltage which could supported by the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  Voltage                 Units in 0.1 V.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetHostVoltage (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     Voltage                       
  );

EFI_STATUS
EFIAPI
SetHostDdrMode (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN UINT32                      DdrMode
);

EFI_STATUS
EFIAPI
SetHostSpeedMode(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     HighSpeed                       
  );

/**
  Reset the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  ResetAll                TRUE to reset all.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
ResetSdHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  RESET_TYPE                 ResetType   
  );

/**
  Enable auto stop command.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  Enable                  TRUE to enable, FALSE to disable.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
EnableAutoStopCmd (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable   
  );

/**
  Find whether these is a card inserted into the slot. If so
  init the host. If not, return EFI_NOT_FOUND.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.

  @retval EFI_NOT_FOUND           Not find device.
  @retval EFI_SUCCESS             Success.

**/
EFI_STATUS
EFIAPI
DetectCardAndInitHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  );

/**
  Set the Block length.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  BlockLength             card supportes block length.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetBlockLength (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BlockLength   
  );

EFI_STATUS 
EFIAPI    
SetupDevice(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  );


EFI_STATUS
SdMemMapAccess (
  UINT32                    Address,
  BOOLEAN                   ReadWriteOp,
  UINTN                     Offset,
  EFI_PCI_IO_PROTOCOL_WIDTH WidthOperation,
  UINTN                     Count,
  VOID                      *Buffer
  );

#endif
