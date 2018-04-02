/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  Cmos.c

Abstract: 

Revision History:

**/
#include "Cmos.h"

EFI_HANDLE  mHandle = NULL;
MEDIA_BLOCK_MAP device_block_map[] = {
    {0x70, 128},
    {0x72, 128},
    {0,0}
};

NV_DEVICE_INSTANCE   *mNvDevice = NULL;

EFI_STATUS
device_info (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN OUT MEDIA_BLOCK_MAP**           MapInfo
)
{
    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);
    *MapInfo = DeviceInstance->BlockMap;
    return EFI_SUCCESS;
}

EFI_STATUS
device_sense (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
devcie_read (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN OUT UINT8* Buffer,
    IN OUT UINTN* Length
)
{
    UINT8  offset;
    UINT16 index, data;
    UINTN  count;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    if ((*Length == 0) || ((Address + *Length) >= (DeviceInstance->Bank0Size + DeviceInstance->Bank1Size)))
        return EFI_INVALID_PARAMETER;

    count = 0;
    offset = (UINT8)Address;

    if ((Address + *Length) < DeviceInstance->Bank0Size) {
        index = DeviceInstance->Bank0Port;
        data = DeviceInstance->Bank0Port + 1;
    } else {
        index = DeviceInstance->Bank1Port;
        data = DeviceInstance->Bank1Port + 1;
        offset = (UINT8)(Address - DeviceInstance->Bank0Size);
    }

    while (count<*Length) {
        _outp(index, (UINT8)(offset + count));
        Buffer[count] = (UINT8)_inp(data);
        count++;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
device_write (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINT8* Buffer,
    IN UINTN Length
)
{
    UINT8  offset;
    UINT16 index, data;
    UINTN  count;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    if ((Length == 0) || ((Address + Length) >= (DeviceInstance->Bank0Size + DeviceInstance->Bank1Size)))
        return EFI_INVALID_PARAMETER;

    count = 0;
    offset = (UINT8)Address;

    if ((Address + Length) < DeviceInstance->Bank0Size) {
        index = DeviceInstance->Bank0Port;
        data = DeviceInstance->Bank0Port + 1;
    } else {
        index = DeviceInstance->Bank1Port;
        data = DeviceInstance->Bank1Port + 1;
        offset = (UINT8)(Address - DeviceInstance->Bank0Size);
    }

    while (count < Length) {
        _outp(index, (UINT8)(offset + count));
        _outp(data, Buffer[count]);
        count++;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
device_erase (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
device_lock (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}

/**
  CMOS access Driver entry point.

  Register CMOS access method to NVMediaAccess Protocol.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.
  @param[in] SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES Fails to allocate memory for device.
  @retval other                Some error occurs when executing this entry point.

**/
EFI_STATUS
DriverEntry (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    NV_MEDIA_ACCESS_PROTOCOL *pMediaAccessProtocol;

    //
    // Allocate pool for SPI protocol instance
    //
    Status = gSmst->SmmAllocatePool (
                 EfiRuntimeServicesData, // MemoryType don't care
                 sizeof (NV_DEVICE_INSTANCE),
                 &mNvDevice
             );
    ASSERT_EFI_ERROR (Status);

    ZeroMem ((VOID *) mNvDevice, sizeof (NV_DEVICE_INSTANCE));

    mNvDevice->Signature = NV_DEVICE_DATA_SIGNATURE;
    mNvDevice->Handle = ImageHandle;

    mNvDevice->DeviceProtocol.Info  = device_info;
    mNvDevice->DeviceProtocol.Sense = device_sense;
    mNvDevice->DeviceProtocol.Read  = devcie_read;
    mNvDevice->DeviceProtocol.Write = device_write;
    mNvDevice->DeviceProtocol.Erase = device_erase;
    mNvDevice->DeviceProtocol.Lock  = device_lock;

    mNvDevice->BlockMap = &device_block_map[0];
    mNvDevice->Bank0Port = 0x70;
    mNvDevice->Bank0Size = 128;
    mNvDevice->Bank1Port = 0x72;
    mNvDevice->Bank1Size = 128;

    //
    // Locate the platform access protocol.
    //
    Status = gBS->LocateProtocol (&gEfiSmmNvMediaAccessProtocolGuid, NULL, &pMediaAccessProtocol);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->InstallProtocolInterface (
                 &mHandle,
                 &gEfiSmmNvMediaDeviceProtocolGuid,
                 EFI_NATIVE_INTERFACE,
                 &mNvDevice->DeviceProtocol);

    Status = pMediaAccessProtocol->Init(pMediaAccessProtocol, (void *)&mNvDevice->DeviceProtocol, CMOS_MEDIA_TYPE);
    /*
        // Test Example:
        {
          UINT8 buffer[10];
          UINTN length;
          length = 10;
          Status = pMediaAccessProtocol->Read(pMediaAccessProtocol, 0x0, (void *)buffer, &length, CMOS_MEDIA_TYPE);

          length = 10;
          Status = pMediaAccessProtocol->Write(pMediaAccessProtocol, 64, 	(void *)buffer, length, CMOS_MEDIA_TYPE);

          length = 10;
          Status = pMediaAccessProtocol->Read(pMediaAccessProtocol, 128, (void *)buffer, &length, CMOS_MEDIA_TYPE);

          length = 10;
          Status = pMediaAccessProtocol->Write(pMediaAccessProtocol, 128, 	(void *)buffer, length, CMOS_MEDIA_TYPE);
        }
    */
    return EFI_SUCCESS;
}
