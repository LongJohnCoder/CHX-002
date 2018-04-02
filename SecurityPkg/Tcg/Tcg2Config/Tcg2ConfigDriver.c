/** @file
  The module entry point for Tcg2 configuration module.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Tcg2ConfigImpl.h"




/**
  The entry point for Tcg2 configuration driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_ALREADY_STARTED    The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES   Fail to execute entry point due to lack of resources.
  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Fail to install protocols as indicated.

**/
EFI_STATUS
EFIAPI
Tcg2ConfigDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                    Status;
  TCG2_CONFIG_PRIVATE_DATA      *PrivateData;
  

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  ImageHandle,
                  ImageHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }
  
  //
  // Create a private data structure.
  //
  PrivateData = AllocateCopyPool (sizeof (TCG2_CONFIG_PRIVATE_DATA), &mTcg2ConfigPrivateDateTemplate);
  ASSERT (PrivateData != NULL);
  mTcg2ConfigPrivateDate = PrivateData;
  //
  // Install private GUID.
  //    
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &PrivateData->Tcg2Protocol);
  ASSERT_EFI_ERROR (Status);
  
  Status = InstallTcg2ConfigForm (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    UninstallTcg2ConfigForm (PrivateData);
  }  
  
  return Status;
}

/**
  Unload the Tcg2 configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The Tcg2 configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
Tcg2ConfigDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  TCG2_CONFIG_PRIVATE_DATA    *PrivateData;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );  
  if (EFI_ERROR (Status)) {
    return Status;  
  }
  
  ASSERT (PrivateData->Signature == TCG2_CONFIG_PRIVATE_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         &ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );
  
  UninstallTcg2ConfigForm (PrivateData);

  return EFI_SUCCESS;
}
