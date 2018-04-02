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


#include <PiPei.h>
#include <Guid/TpmInstance.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/TpmInitialized.h>
#include <Protocol/Tcg2Protocol.h>
#include "Tcg2ConfigNvData.h"


CONST EFI_PEI_PPI_DESCRIPTOR gTpmSelectedPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTpmDeviceSelectedGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

UINT8
DetectTpmDevice (
  VOID
  );

/**
  The entry point for Tcg2 configuration driver.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCES             Convert variable to PCD successfully.
  @retval Others                 Fail to convert variable to PCD.
**/
EFI_STATUS
EFIAPI
Tcg2ConfigPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  EFI_STATUS                      Status2;
  UINT8                           TpmDevice;


  TpmDevice = DetectTpmDevice();
  PcdSet8(PcdTpmInstanceId, TpmDevice);
  DEBUG ((EFI_D_INFO, "TpmDevice:%d\n", TpmDevice));

  if (TpmDevice != TPM_DEVICE_NULL) {
    Status = PeiServicesInstallPpi (&gTpmSelectedPpi);
    ASSERT_EFI_ERROR (Status);
  }
  
  //
  // Even if no TPM is selected or detected, we still need intall TpmInitializationDonePpi.
  // Because TcgPei or Tcg2Pei will not run, but we still need a way to notify other driver.
  // Other driver can know TPM initialization state by TpmInitializedPpi.
  //
  if (TpmDevice == TPM_DEVICE_NULL) {
    Status2 = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
    ASSERT_EFI_ERROR (Status2);
  }

  return Status;
}


