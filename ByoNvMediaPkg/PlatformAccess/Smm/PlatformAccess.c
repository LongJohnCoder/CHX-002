/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PlatformAccess.c

Abstract:

Revision History:

**/
#include "PlatformAccess.h"

EFI_HANDLE  mHandle = NULL;

EFI_STATUS
Func_Enable (
    IN CONST PLATFORM_ACCESS_PROTOCOL*  this
)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    //DEBUG ((EFI_D_ERROR, "Platform Access: Func_Enable(...)\n"));

    return Status;
}

EFI_STATUS
Func_Disable (
    IN CONST PLATFORM_ACCESS_PROTOCOL*  this
)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    //DEBUG ((EFI_D_ERROR, "Platform Access: Func_Disable(...)\n"));

    return Status;
}

PLATFORM_ACCESS_PROTOCOL  PlatformAccessInstance = {
    Func_Enable,
    Func_Disable
};

/**
  Platform Access Driver entry point.

  Install Platform access protocol.

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
    EFI_STATUS  Status = EFI_SUCCESS;

    //DEBUG ((EFI_D_ERROR, "Platform Access: DriverEntry(...)\n"));

    Status = gBS->InstallProtocolInterface (
                 &mHandle,
                 &gEfiSmmPlatformAccessProtocolGuid,
                 EFI_NATIVE_INTERFACE,
                 &PlatformAccessInstance);

    return Status;
}
