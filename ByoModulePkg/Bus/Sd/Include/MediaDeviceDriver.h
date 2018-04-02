/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  MediaDeviceDriver.h

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

  MediaDeviceDriver.h

Abstract:

  Media Device Driver header

**/

#ifndef _MEDIA_DEVICE_DRIVER_H
#define _MEDIA_DEVICE_DRIVER_H

#include <Uefi.h>

#include <Protocol/PciIo.h>
#include <Protocol/BlockIo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Pci22.h>
#include <Protocol/SDHostIo.h>

#include "MMC.h"
#include "CEATA.h"
#include "SDCard.h"


extern EFI_COMPONENT_NAME_PROTOCOL  gMediaDeviceComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gMediaDeviceComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL  gMediaDeviceDriverBinding;

#ifdef EFI_DEBUG
extern UINT32  gMediaDeviceDebugLevel;
#endif


#define CARD_DATA_SIGNATURE  SIGNATURE_32 ('c', 'a', 'r', 'd')
#define CARD_PARTITION_SIGNATURE  SIGNATURE_32 ('c', 'p', 'a', 'r')

#define CARD_PARTITION_DATA_FROM_THIS(a) \
    CR(a, MMC_PARTITION_DATA, BlockIo, CARD_PARTITION_SIGNATURE)

#define CARD_DATA_FROM_THIS(a) \
    ((CARD_PARTITION_DATA_FROM_THIS(a))->CardData)

#define CARD_DATA_PARTITION_NUM(p) \
    ((((UINTN) p) - ((UINTN) &(p->CardData->Partitions))) / sizeof (*p))

//
// Command timeout will be max 100 ms
//
#define  TIMEOUT_COMMAND     100
#define  TIMEOUT_DATA        5000

typedef enum{
  UnknownCard = 0,
  MMCCard,                // MMC card
  CEATACard,              // CE-ATA device 
  SDMemoryCard,           // SD 1.1 card
  SDMemoryCard2,          // SD 2.0 or above standard card
  SDMemoryCard2High       // SD 2.0 or above high capacity card
}CARD_TYPE;

typedef struct _CARD_DATA CARD_DATA;

typedef struct {
  //
  //BlockIO
  //
  UINT32                    Signature;

  EFI_HANDLE                Handle;

  BOOLEAN                   Present;

  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  EFI_BLOCK_IO_PROTOCOL     BlockIo;

  EFI_BLOCK_IO_MEDIA        BlockIoMedia;

  CARD_DATA                 *CardData;

} MMC_PARTITION_DATA;


#define MAX_NUMBER_OF_PARTITIONS 8


struct _CARD_DATA {
  //
  //BlockIO
  //
  UINT32                    Signature;

  EFI_HANDLE                Handle;

  MMC_PARTITION_DATA        Partitions[MAX_NUMBER_OF_PARTITIONS];

  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
  CARD_TYPE                 CardType;

  UINT8                     CurrentBusWidth; 
  BOOLEAN                   DualVoltage;
  BOOLEAN                   NeedFlush;    
  UINT8                     Reserved[3];

  UINT16                    Address;
  UINT32                    BlockLen;
  UINT32                    MaxFrequency;
  UINT64                    BlockNumber;
  //
  //Common used
  //
  CARD_STATUS               CardStatus;
  OCR                       OCRRegister;
  CID                       CIDRegister;
  CSD                       CSDRegister; 
  EXT_CSD                   ExtCSDRegister;
  UINT8                     *RawBufferPointer;  
  UINT8                     *AlignedBuffer; 
  //
  //CE-ATA specific
  //
  TASK_FILE                 TaskFile;
  IDENTIFY_DEVICE_DATA      IndentifyDeviceData;
  //
  //SD specific
  //
  SCR                       SCRRegister;
  SD_STATUS_REG             SDSattus;
  SWITCH_STATUS             SwitchStatus; 
} ;

/**
  Starting the Media Device Driver.

  @param  SDHostIo                A pointer to EFI_BLOCK_IO_PROTOCOL.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Unsupport.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverStart (
  EFI_SD_HOST_IO_PROTOCOL   *SDHostIo
  );

/**
  The entry point for SD driver which installs the driver binding and component name
  protocol on its ImageHandle.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            if the driver binding and component name protocols
                                 are successfully
  @retval Others                 Failed to install the protocols.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param[out]  DriverName       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
MediaDeviceComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  CHAR8                           *Language,
  OUT CHAR16                          **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]  ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
MediaDeviceComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  EFI_HANDLE                      ControllerHandle,
  IN  EFI_HANDLE                      ChildHandle, OPTIONAL
  IN  CHAR8                           *Language,
  OUT CHAR16                          **ControllerName
  );

/**
  Tests to see if this driver supports a given controller.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.
                                   This parameter is ignored by device drivers,
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver
                                   specified by This.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the
                                   driver specified by This.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Start this driver on ControllerHandle.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.
                                   This parameter is ignored by device drivers,
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of
                                   resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
                                Not used.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.Not used.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

/**
  MMC/SD card init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
MmcSdCardInit (
  IN  CARD_DATA    *CardData
  );

/**
  MMC/SD card BlockIo init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
MmcSdBlockIoInit (
  IN  CARD_DATA    *CardData
  );

/**
  Send command by using Host IO protocol.

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
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
SendCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  );

/**
  Send the card FAST_IO command.

  @param  CardData                Pointer to CARD_DATA.
  @param  RegisterAddress         Register Address.
  @param  RegisterData            Pointer to register Data.
  @param  Write                   TRUE for write, FALSE for read.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
FastIO (
  IN      CARD_DATA   *CardData,
  IN      UINT8       RegisterAddress,
  IN  OUT UINT8       *RegisterData,
  IN      BOOLEAN     Write
  );

/**
  Judge whether it is CE-ATA device or not.

  @param  CardData                Pointer to CARD_DATA.

  @retval TRUE                    This is CE-ATA device.
  @retval FALSE                   This is not CE-ATA device.

**/
BOOLEAN
IsCEATADevice (
  IN  CARD_DATA    *CardData
  );

/**
  CEATA card BlockIo init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
CeAtaBlockIoInit (
  IN  CARD_DATA    *CardData
  );

/**
  IDENTIFY_DEVICE command.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
IndentifyDevice (
  IN  CARD_DATA    *CardData
  );

/**
  FLUSH_CACHE_EXT command.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
FlushCache (
  IN  CARD_DATA    *CardData
  );

/**
  STANDBY_IMMEDIATE command.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
StandByImmediate (
  IN  CARD_DATA    *CardData
  );

/**
  READ_DMA_EXT command.

  @param  CardData                Pointer to CARD_DATA.
  @param  LBA                     The starting logical block address to read from on the device.
  @param  Buffer                  A pointer to the destination buffer for the data. The caller
                                  is responsible for either having implicit or explicit ownership
                                  of the buffer.
  @param  SectorCount             Buffer size in 512 bytes unit.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
ReadDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  );

/**
  WRITE_DMA_EXT command.

  @param  CardData                Pointer to CARD_DATA.
  @param  LBA                     The starting logical block address to read from on the device.
  @param  Buffer                  A pointer to the destination buffer for the data. The caller
                                  is responsible for either having implicit or explicit ownership
                                  of the buffer.
  @param  SectorCount             Buffer size in 512 bytes unit.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
WriteDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  );

/**
  Send software reset.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
SoftwareReset (
  IN  CARD_DATA    *CardData
  );

/**
  Send the card APP_CMD command with the following command indicated
  by CommandIndex.

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
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
SendAppCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  );

EFI_STATUS
MediaDeviceDriverInstallBlockIo (
  IN  CARD_DATA    *CardData
  );

EFI_STATUS
MediaDeviceDriverUninstallBlockIo (
  IN  CARD_DATA    *CardData
  );

EFI_STATUS
MmcSelectPartitionNum (
  IN  CARD_DATA              *CardData,
  IN  UINT8                  Partition
  );

UINT32
MmcGetExtCsd8 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  );

UINT32
MmcGetExtCsd24 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  );

UINT32
MmcGetExtCsd32 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  );

EFI_STATUS
MmcSelectPartition (
  IN  MMC_PARTITION_DATA     *Partition
  );


#endif


