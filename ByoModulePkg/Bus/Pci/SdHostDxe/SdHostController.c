/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SDHostController.c

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
/*++
  This file contains an 'Intel Peripheral Driver' and uniquely
  identified as Intel "Mobile Silicon Support Code" and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
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

  SDHostController.c

Abstract:

  SD Host I/O protocol implementation

**/
#include "SdHostDriver.h"

extern EFI_SMM_BASE2_PROTOCOL    *gSMM2;
extern EFI_CPU_IO2_PROTOCOL      *mCpuIo2;
extern volatile SD_SMMCALL_COMM  *mSmmCallComm;
extern SMMCALL_ENTRY             *mSmmCallTablePtr;
extern EFI_SMM_CONTROL2_PROTOCOL *mSmmControl;
extern UINT8                      mArgBufferSize;
extern UINT8                      mArgBuffer;

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SendHCommand \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_9(SendHCommand, &Ret, &SdHostData->SdHostIo, CommandIndex, Argument, DataType, Buffer, BufferSize, ResponseType, TimeOut, ResponseData);

  return Ret;
}

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
  IN  UINT32                     MaxFrequency
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetClockFrequency \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetClockFrequency, &Ret, &SdHostData->SdHostIo, MaxFrequency);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetBusWidth \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetBusWidth, &Ret, &SdHostData->SdHostIo, BusWidth);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetHostVoltage \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetHostVoltage, &Ret, &SdHostData->SdHostIo, Voltage);
  return Ret;
}

EFI_STATUS
EFIAPI
SetHostDdrMode(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     DdrMode                       
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetHostDdrMode \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetHostDdrMode, &Ret, &SdHostData->SdHostIo, DdrMode);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native ResetSDHost \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(ResetSdHost, &Ret, &SdHostData->SdHostIo, ResetType);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native EnableAutoStopCmd \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(EnableAutoStopCmd, &Ret, &SdHostData->SdHostIo, Enable);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetBlockLength \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetBlockLength, &Ret, &SdHostData->SdHostIo, BlockLength);

  return Ret;
}

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
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;

  //DEBUG ((EFI_D_ERROR, "Native DetectCardAndInitHost \n"));
  gBS->Stall (5 * 1000);//workaround for 4G SD card cannot be detected under shell
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_1(DetectCardAndInitHost, &Ret, &SdHostData->SdHostIo);

  return Ret;
}

EFI_STATUS
EFIAPI
SetHostSpeedMode(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     HighSpeed
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;
  //DEBUG ((EFI_D_ERROR, "Native SetHostSpeedMode \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_2(SetHostSpeedMode, &Ret, &SdHostData->SdHostIo, HighSpeed);

  return Ret;
}

EFI_STATUS
EFIAPI
SetupDevice (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  )
{
  SDHOST_DATA            *SdHostData;
  EFI_STATUS             Ret;

  //DEBUG ((EFI_D_ERROR, "Native SetupDevice \n"));
  SdHostData = SDHOST_DATA_FROM_THIS (This);
  SMMCALL_1(SetupDevice, &Ret, &SdHostData->SdHostIo);

  return Ret;
}


