/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  NvMediaAccess.c

Abstract: 

Revision History:

**/
#include "NvMediaAccess.h"

EFI_HANDLE  mHandle = NULL;
NV_MEDIA_INSTANCE* mNvMediaInstance;

EFI_STATUS
MediaInit (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN VOID *                           Device,
    IN NV_MEDIA_TYPE                    Type
)
{
    NV_MEDIA_INSTANCE  *pNvMediaInstance;
    EFI_STATUS         Status;
    EFI_HANDLE         Handle = NULL;


    DEBUG ((EFI_D_INFO, "NvMediaAccess: MediaInit(...)\n"));
    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL){
      pNvMediaInstance->NvDevice[Type] = Device;

      if(Type == SPI_MEDIA_TYPE){
        Status = gSmst->SmmInstallProtocolInterface (
                        &Handle,
                        &gEfiSmmNvMediaAccessSpiReadyGuid,
                        EFI_NATIVE_INTERFACE,
                        NULL
                        );
      }
      
    }
    return EFI_SUCCESS;
}

EFI_STATUS
MediaInfo (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN OUT MEDIA_BLOCK_MAP**            MapInfo,
    IN NV_MEDIA_TYPE                    Type
)
{
    EFI_STATUS  Status;
    NV_MEDIA_INSTANCE* pNvMediaInstance;

    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL)
        return EFI_UNSUPPORTED;

    Status = pNvMediaInstance->NvDevice[Type]->Info(pNvMediaInstance->NvDevice[Type], MapInfo);

    return Status;
}

EFI_STATUS
MediaRead (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN OUT UINT8*                       Buffer,
    IN OUT UINTN*                       Length,
    IN NV_MEDIA_TYPE                    Type
)
{
    EFI_STATUS  Status;
    NV_MEDIA_INSTANCE* pNvMediaInstance;

    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL)
        return EFI_UNSUPPORTED;

    Status = pNvMediaInstance->NvDevice[Type]->Read(pNvMediaInstance->NvDevice[Type], Address, Buffer, Length);

    return Status;
}

EFI_STATUS
MediaWrite (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINT8*                           Buffer,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
)
{
    EFI_STATUS  Status;
    NV_MEDIA_INSTANCE* pNvMediaInstance;

    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL)
        return EFI_UNSUPPORTED;

    Status = pNvMediaInstance->NvDevice[Type]->Write(pNvMediaInstance->NvDevice[Type], Address, Buffer, Length);

    return Status;
}

EFI_STATUS
MediaErase (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
)
{
    EFI_STATUS  Status;
    NV_MEDIA_INSTANCE* pNvMediaInstance;

    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL)
        return EFI_UNSUPPORTED;

    Status = pNvMediaInstance->NvDevice[Type]->Erase(pNvMediaInstance->NvDevice[Type], Address, Length);

    return Status;
}

EFI_STATUS
MediaLock (
    IN CONST NV_MEDIA_ACCESS_PROTOCOL*  this,
    IN UINTN                            Address,
    IN UINTN                            Length,
    IN NV_MEDIA_TYPE                    Type
)
{
    EFI_STATUS  Status;
    NV_MEDIA_INSTANCE* pNvMediaInstance;

    pNvMediaInstance = MEDIA_INSTANCE_FROM_DEVICEPROTOCOL(this);

    if (pNvMediaInstance->NvDevice[Type] == NULL)
        return EFI_UNSUPPORTED;

    Status = pNvMediaInstance->NvDevice[Type]->Lock(pNvMediaInstance->NvDevice[Type], Address, Length);

    return Status;
}

NV_MEDIA_ACCESS_PROTOCOL  NvMediaAccessInstance = {
    MediaInit,
    MediaInfo,
    MediaRead,
    MediaWrite,
    MediaErase,
    MediaLock
};

/**
  NV Media Access Driver entry point.

  Install NV Media access protocol.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.
  @param[in] SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES Fails to allocate memory for device.
  @retval other                Some error occurs when executing this entry point.

**/
EFI_STATUS
DriverEntry (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_STATUS  Status;
    EFI_HANDLE  SmmHandle = NULL;
    

    Status = gSmst->SmmAllocatePool (
                      EfiRuntimeServicesData,
                      sizeof (NV_MEDIA_INSTANCE),
                      &mNvMediaInstance
                      );
    ASSERT_EFI_ERROR (Status);

    ZeroMem ((VOID *) mNvMediaInstance, sizeof (NV_MEDIA_INSTANCE));

    mNvMediaInstance->Signature = NV_MEDIA_DATA_SIGNATURE;
    mNvMediaInstance->Handle = ImageHandle;
    mNvMediaInstance->MediaAccessProtocol.Init  = MediaInit;
    mNvMediaInstance->MediaAccessProtocol.Info  = MediaInfo;
    mNvMediaInstance->MediaAccessProtocol.Read  = MediaRead;
    mNvMediaInstance->MediaAccessProtocol.Write = MediaWrite;
    mNvMediaInstance->MediaAccessProtocol.Erase = MediaErase;
    mNvMediaInstance->MediaAccessProtocol.Lock  = MediaLock;

    Status = gBS->InstallProtocolInterface (
                    &mHandle,
                    &gEfiSmmNvMediaAccessProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mNvMediaInstance->MediaAccessProtocol
                    );

    Status = gSmst->SmmInstallProtocolInterface (
                      &SmmHandle,
                      &gEfiSmmNvMediaAccessProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &mNvMediaInstance->MediaAccessProtocol
                      );
    ASSERT_EFI_ERROR (Status);

    return Status;
}
