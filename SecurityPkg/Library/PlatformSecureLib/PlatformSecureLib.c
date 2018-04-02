/** @file
  Provides a platform-specific method to enable Secure Boot Custom Mode setup.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmVariable.h>


#define PP_VALID_FLAG  SIGNATURE_32('_', 'W', 'Z', '_')


STATIC CHAR16    gMyPPFlagName[] = {L"Name"};
STATIC EFI_GUID  gMyPPFlagGuid   = {0x4c5bc259, 0xe281, 0x4fe4, {0xb5, 0x8b, 0x2c, 0x4c, 0x8a, 0x9e, 0xf4, 0xcd}};
STATIC EFI_SMM_SYSTEM_TABLE2 *gMySmst;
  
EFI_STATUS LibAuthVarSetPhysicalPresent(BOOLEAN Present)
{
  UINT32       Data32;
  EFI_STATUS   Status;

  DEBUG((EFI_D_INFO, __FUNCTION__"(%d)\n", Present));
  
  if(gST->BootServices == NULL){    // RT
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  if(Present){
    Data32 = PP_VALID_FLAG;
  }else{
    Data32 = 0;
  }

  Status = gST->RuntimeServices->SetVariable(
                                   gMyPPFlagName, 
                                   &gMyPPFlagGuid, 
                                   EFI_VARIABLE_BOOTSERVICE_ACCESS, 
                                   sizeof(Data32), 
                                   &Data32
                                   );
  
ProcExit:
  DEBUG((EFI_D_INFO, __FUNCTION__"() Exit %r\n", Status));  
  return Status;
} 


/**

  This function provides a platform-specific method to detect whether the platform
  is operating by a physically present user. 

  Programmatic changing of platform security policy (such as disable Secure Boot,
  or switch between Standard/Custom Secure Boot mode) MUST NOT be possible during
  Boot Services or after exiting EFI Boot Services. Only a physically present user
  is allowed to perform these operations.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.
  
  @retval  TRUE       The platform is operated by a physically present user.
  @retval  FALSE      The platform is NOT operated by a physically present user.

**/
BOOLEAN
EFIAPI
UserPhysicalPresent (
  VOID
  )
{
  BOOLEAN      rc = FALSE;
  UINT32       Data32;
  UINTN        VarSize;
  EFI_STATUS   Status;  
  EFI_SMM_VARIABLE_PROTOCOL  *SmmVar;


  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
  
  if(gST->BootServices == NULL){    // RT
    goto ProcExit;
  }
  if(gMySmst == NULL){
    goto ProcExit;
  }  

  Status = gMySmst->SmmLocateProtocol(&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&SmmVar);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "SmmVariable not found\n"));    
    goto ProcExit;
  }

  VarSize = sizeof(Data32);  
  Status = SmmVar->SmmGetVariable (
                     gMyPPFlagName,
                     &gMyPPFlagGuid,
                     NULL,
                     &VarSize,
                     &Data32
                     ); 
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "GetVariable:%r\n", Status));    
    goto ProcExit;
  }  

  if(Data32 == PP_VALID_FLAG){
    rc = TRUE;
  }
  
ProcExit:
  DEBUG((EFI_D_INFO, "ChkPP:%d\n", rc));
  return rc;  
}


EFI_STATUS
EFIAPI
PlatformSecureLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_SMM_BASE2_PROTOCOL  *SmmBase2 = NULL;

  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiSmmBase2ProtocolGuid,
                                        NULL,
                                        (VOID **)&SmmBase2
                                        );
  ASSERT_EFI_ERROR (Status);
  ASSERT (SmmBase2 != NULL);

  gMySmst = NULL;
  SmmBase2->GetSmstLocation (SmmBase2, &gMySmst);

  return EFI_SUCCESS;
}



