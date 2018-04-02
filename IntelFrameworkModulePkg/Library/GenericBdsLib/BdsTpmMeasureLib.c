/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.


Module Name:

  BdsTpmMeasureLib.c

Abstract:

  BDS TPM Lib functions implementation

Revision History

**/
#include "BdsTpmMeasureLib.h"
#include "String.h"

#include <Protocol/TcgService.h>
#include <Protocol/UsbIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

/**++

Routine Description:
  Function retrieves a protocol interface for a device.

Arguments:
  FilePath     - A pointer to a device path data structure.

  BlkIo        - Supplies and address where a pointer to the requested
                 Block IO Protocol interface is returned.
,
Returns:

  If a match is found, then the protocol interface of that device is
  returned in Interface.  Otherwise, Interface is set to NULL.

--**/
EFI_STATUS
BdsTpmLibDevicePathToBlockIo(
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  UINT16                    *Description,
  OUT EFI_BLOCK_IO_PROTOCOL     **BlkIo
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    FoundStatus;
  EFI_HANDLE                    Device;
  EFI_HANDLE                    *BlockIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL      *BlockIoDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *BlockIoHdDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  BBS_BBS_DEVICE_PATH           *BbsDev;
  UINTN                         BlockIoHandleCount;
  UINTN                         Index;
  INT32                         Primary;
  INT32                         Master;
  CHAR16                        TempChar;

  Primary = -1;
  Master  = -1;
  Status  = EFI_SUCCESS;

  if((DevicePathType(FilePath) == BBS_DEVICE_PATH) && (DevicePathSubType(FilePath) == BBS_BBS_DP)){
    BbsDev  = (BBS_BBS_DEVICE_PATH *)FilePath;

    Status = gBS->LocateHandleBuffer(
                    ByProtocol,
                    &gEfiBlockIoProtocolGuid,
                    NULL,
                    &BlockIoHandleCount,
                    &BlockIoBuffer
                    );
    if(EFI_ERROR(Status) || BlockIoHandleCount == 0){
      //
      // If there was an error or there are no device handles that support
      // the BLOCK_IO Protocol, then return.
      //
      return EFI_NOT_FOUND;
    }

    //
    // Ugly code to detect whether the device's channel, etc.
    //
    if((BbsDev->DeviceType == BBS_TYPE_HARDDRIVE) || (BbsDev->DeviceType == BBS_TYPE_CDROM)){
      TempChar = Description[13];
      Description[13] = 0;
      if(StrCmp(Description,L"Primary Slave") == 0){
        Primary = 1;
        Master  = 0;
      }
      Description[13] = TempChar;

      TempChar = Description[14];
      Description[14] = 0;
      if(StrCmp(Description,L"Primary Master") == 0){
        Primary = 1;
        Master  = 1;
      }
      Description[14] = TempChar;

      TempChar = Description[15];
      Description[15] = 0;
      if(StrCmp(Description,L"Secondary Slave") == 0 ){
        Primary = 0;
        Master  = 0;
      }
      Description[15] = TempChar;

      TempChar = Description[16];
      Description[16] = 0;
      if(StrCmp(Description,L"Secondary Master") == 0){
        Primary = 0;
        Master  = 1;
      }
      Description[16] = TempChar;
    }

    //
    // Loop through all the device handles that support the BLOCK_IO Protocol
    //
    for(Index = 0;Index < BlockIoHandleCount;Index++){

      Status = gBS->HandleProtocol(
                      BlockIoBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&BlockIoDevicePath
                      );
      if(EFI_ERROR(Status) || BlockIoDevicePath == NULL){
        continue;
      }

      FoundStatus = EFI_NOT_FOUND;
      switch(BbsDev->DeviceType){

        case BBS_TYPE_HARDDRIVE:
        case BBS_TYPE_CDROM:
          //
          // Find the hard driver blockIo
          //
          DevicePath          = BlockIoDevicePath;
          BlockIoHdDevicePath = NULL;

          //
          // find HardDriver device path node
          //
          while(!IsDevicePathEnd (DevicePath)){
            if((DevicePathType(DevicePath) == MESSAGING_DEVICE_PATH) && (DevicePathSubType(DevicePath) == MSG_ATAPI_DP)){
              BlockIoHdDevicePath = DevicePath;
              FoundStatus = EFI_SUCCESS;
              break;
            }
            if((DevicePathType(DevicePath) == MESSAGING_DEVICE_PATH) && (DevicePathSubType(DevicePath) == MSG_SATA_DP)){
              BlockIoHdDevicePath = DevicePath;
              FoundStatus = EFI_SUCCESS;
              break;
            }
            DevicePath = NextDevicePathNode(DevicePath);
          }
          break;

        case BBS_TYPE_USB:
          //
          // Find the USB blockIo. What to do if there are more than two USBIO protocols?
          //
          Status  = gBS->HandleProtocol(
                          BlockIoBuffer[Index],
                          &gEfiUsbIoProtocolGuid,
                          (VOID *)&UsbIo
                          );
          if((Status == EFI_SUCCESS) && (UsbIo != NULL)){
            FoundStatus = EFI_SUCCESS;
          }
          break;

        case BBS_TYPE_EMBEDDED_NETWORK:
          break;
        default:
          break;
      }
      if(FoundStatus == EFI_SUCCESS ){
        //
        // Found, get the blockIo protocol
        //
        Status  = gBS->HandleProtocol(
                        BlockIoBuffer[Index],
                        &gEfiBlockIoProtocolGuid,
                        (VOID *)BlkIo
                        );
        return Status;
      }
      else{
        return EFI_NOT_FOUND;
      }
    }
  }
  else{
    Status = gBS->LocateDevicePath(&gEfiBlockIoProtocolGuid,&FilePath,&Device);

    if(!EFI_ERROR(Status)){
      //
      // If we didn't get a direct match return not found
      //
      Status = EFI_NOT_FOUND;

      if(IsDevicePathEnd(FilePath)){
        //
        // It was a direct match, lookup the protocol interface
        //
        Status = gBS->HandleProtocol(Device,&gEfiBlockIoProtocolGuid,BlkIo);
        }
    }

    //
    // If there was an error, do not return an interface
    //
    if(EFI_ERROR(Status)){
      *BlkIo = NULL;
    }
  }
  return Status;
}


/**++

Routine Description:
  Extend the boot event according to TCG PC client implementation spec.

--**/
EFI_STATUS
BdsTpmLibMeasureInt19Event(
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  UINT16                    *Description
  )
{
  EFI_STATUS              Status;
  EFI_BLOCK_IO_PROTOCOL   *BlkIo;
  EFI_TCG_PROTOCOL        *TcgProtocol;
  TCG_PCR_EVENT           *TcgEvent;
  UINT8                   *Buffer;
  UINT32                  BufferSize;
  UINT32                  EventSize;
  UINT32                  EventNumber;
  EFI_PHYSICAL_ADDRESS    EventLogLastEntry;

  Status = gBS->LocateProtocol(&gEfiTcgProtocolGuid,NULL,(VOID **)&TcgProtocol);
  if(EFI_ERROR(Status)){
    //
    // TPM not supported, return SUCCESS
    //
    return EFI_SUCCESS;
  }

  EventSize = sizeof(EFI_CALLING_INT19);
  TcgEvent = AllocateZeroPool(EventSize + sizeof(TCG_PCR_EVENT));
  if(TcgEvent == NULL){
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // We cannot really get the partition of the IPL. We can only know which hard
  // driver, or which removable device driver we are booting from
  // Get the first block of the IPL code
  //
  Status  = BdsTpmLibDevicePathToBlockIo(DevicePath,Description,&BlkIo);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR,"Failed to get Block IO protocol \r\n"));
  }
  else{
    BufferSize = BlkIo->Media->BlockSize;
    Buffer  = NULL;
    Buffer  = AllocatePool(BufferSize);
    Status  = BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId,0,BufferSize,Buffer);
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_ERROR,"Failed to read the first %d bytes of the boot device.\r\n",BufferSize));
    }
    else{
      //
      // Measure to PCR[4]
      //
      DEBUG((EFI_D_ERROR,"TCG hash extend the first %d bytes of IPL data.\r\n",BufferSize));

      TcgEvent->EventType = EV_EFI_BOOT_SERVICES_APPLICATION;
      TcgEvent->PCRIndex  = 4;
      TcgEvent->EventSize = EventSize;
      CopyMem(TcgEvent->Event,EFI_CALLING_INT19,sizeof(EFI_CALLING_INT19));
      Status  = TcgProtocol->HashLogExtendEvent(
                              TcgProtocol,
                              (EFI_PHYSICAL_ADDRESS)Buffer,
                              BufferSize,
                              TPM_ALG_SHA,
                              TcgEvent,
                              &EventNumber,
                              &EventLogLastEntry
                              );
      if(EFI_ERROR(Status)){
        DEBUG((EFI_D_ERROR,"Error: Failed to hash the first %d bytes of IPL data.\r\n",BufferSize));
      }
    }
  }
  return EFI_SUCCESS;
}
