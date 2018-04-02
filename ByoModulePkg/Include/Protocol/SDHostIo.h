/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SDHostIo.h

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

  SDHostIo.h

Abstract:

  Interface definition for EFI_SD_HOST_IO_PROTOCOL

**/

#ifndef _SD_HOST_IO_H
#define _SD_HOST_IO_H

// Global ID for the EFI_SD_HOST_IO_PROTOCOL
// {B63F8EC7-A9C9-4472-A4C0-4D8BF365CC51}
//
#define EFI_SD_HOST_IO_PROTOCOL_GUID \
  { \
    0xb63f8ec7, 0xa9c9, 0x4472, 0xa4, 0xc0, 0x4d, 0x8b, 0xf3, 0x65, 0xcc, 0x51 \
  }
///
/// Forward reference for pure ANSI compatability
///
typedef struct _EFI_SD_HOST_IO_PROTOCOL EFI_SD_HOST_IO_PROTOCOL;

typedef enum {
  ResponseNo = 0,
  ResponseR1,
  ResponseR1b,
  ResponseR2,
  ResponseR3,
  ResponseR4,
  ResponseR5,
  ResponseR5b,
  ResponseR6,
  ResponseR7
}RESPONSE_TYPE;

typedef enum {
  NoData = 0,
  InData,
  OutData
}TRANSFER_TYPE;

typedef enum {
  Reset_Auto = 0,
  Reset_DAT,
  Reset_CMD,
  Reset_DAT_CMD,
  Reset_All,
  Reset_HW
}RESET_TYPE;



#define PCI_SUBCLASS_SD_HOST_CONTROLLER   0x05
#define PCI_IF_STANDARD_HOST_NO_DMA       0x00
#define PCI_IF_STANDARD_HOST_SUPPORT_DMA  0x01  

//
//MMIO Registers definition for MMC/SDIO controller
//
#define  MMIO_DMAADR                     0x00
#define  MMIO_BLKSZ                      0x04
#define  MMIO_BLKCNT                     0x06
#define  MMIO_CMDARG                     0x08
#define  MMIO_XFRMODE                    0x0C
#define  MMIO_SDCMD                      0x0E
#define  MMIO_RESP                       0x10
#define  MMIO_BUFDATA                    0x20
#define  MMIO_PSTATE                     0x24
#define  MMIO_HOSTCTL                    0x28
#define  MMIO_PWRCTL                     0x29
#define  MMIO_BLKGAPCTL                  0x2A
#define  MMIO_WAKECTL                    0x2B
#define  MMIO_CLKCTL                     0x2C
#define  MMIO_TOCTL                      0x2E
#define  MMIO_SWRST                      0x2F
#define  MMIO_NINTSTS                    0x30
#define  MMIO_ERINTSTS                   0x32
#define  MMIO_NINTEN                     0x34
#define  MMIO_ERINTEN                    0x36
#define  MMIO_NINTSIGEN                  0x38
#define  MMIO_ERINTSIGEN                 0x3A
#define  MMIO_AC12ERRSTS                 0x3C
#define  MMIO_HOST_CTL2                  0x3E
#define  MMIO_CAP                        0x40
#define  MMIO_MCCAP                      0x48
#define  MMIO_SLTINTSTS                  0xFC
#define  MMIO_CTRLRVER                   0xFE

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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SEND_COMMAND) (
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_CLOCK_FREQUENCY) (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     MaxFrequency          
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_BUS_WIDTH) (
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_HOST_VOLTAGE) (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     Voltage                       
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_RESET_SD_HOST) (
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_ENABLE_AUTO_STOP_CMD) (
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_DETECT_CARD_AND_INIT_HOST) (
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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_BLOCK_LENGTH) (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BlockLength   
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SETUP_DEVICE)(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_HOST_SPEED_MODE) (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     HighSpeed
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SD_HOST_IO_PROTOCOL_SET_HOST_DDR_MODE) (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     DdrMode                       
  );


#define EFI_SD_HOST_IO_PROTOCOL_REVISION_01          0x01 

typedef struct {
  UINT32  HighSpeedSupport:    1;  //High speed supported
  UINT32  V18Support:          1;  //1.8V supported
  UINT32  V30Support:          1;  //3.0V supported
  UINT32  V33Support:          1;  //3.3V supported
  UINT32  SDR50Support:        1;
  UINT32  SDR104Support:       1;
  UINT32  DDR50Support:        1;
  UINT32  Reserved0:           1;
  UINT32  BusWidth4:           1;  // 4 bit width
  UINT32  BusWidth8:           1;  // 8 bit width
  UINT32  Reserved1:           6;
  UINT32  SDMASupport:         1;
  UINT32  ADMA2Support:        1;
  UINT32  DmaMode:             2;
  UINT32  ReTuneTimer:         4;
  UINT32  ReTuneMode:          2;
  UINT32  Reserved2:           6;
  UINT32  BoundarySize;
} HOST_CAPABILITY;

//
// Interface structure for the PCI I/O Protocol
//
typedef struct _EFI_SD_HOST_IO_PROTOCOL {
  UINT32                                             Revision; 
  HOST_CAPABILITY                                    HostCapability;
  UINT16                                             PFA; 
  UINT32                                             Slot0RegMap;
  EFI_SD_HOST_IO_PROTOCOL_SEND_COMMAND               SendHCommand;
  EFI_SD_HOST_IO_PROTOCOL_SET_CLOCK_FREQUENCY        SetClockFrequency;
  EFI_SD_HOST_IO_PROTOCOL_SET_BUS_WIDTH              SetBusWidth;
  EFI_SD_HOST_IO_PROTOCOL_SET_HOST_VOLTAGE           SetHostVoltage;  
  EFI_SD_HOST_IO_PROTOCOL_SET_HOST_DDR_MODE          SetHostDdrMode;  
  EFI_SD_HOST_IO_PROTOCOL_RESET_SD_HOST              ResetSdHost; 
  EFI_SD_HOST_IO_PROTOCOL_ENABLE_AUTO_STOP_CMD       EnableAutoStopCmd;  
  EFI_SD_HOST_IO_PROTOCOL_DETECT_CARD_AND_INIT_HOST  DetectCardAndInitHost;
  EFI_SD_HOST_IO_PROTOCOL_SET_BLOCK_LENGTH           SetBlockLength;
  EFI_SD_HOST_IO_PROTOCOL_SETUP_DEVICE               SetupDevice;
  EFI_SD_HOST_IO_PROTOCOL_SET_HOST_SPEED_MODE        SetHostSpeedMode;
}EFI_SD_HOST_IO_PROTOCOL;

extern EFI_GUID gEfiSdHostIoProtocolGuid;

#endif                 

