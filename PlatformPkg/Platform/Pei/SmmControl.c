
#include "PlatformPei.h"
#include <Ppi/SmmControl.h>
#include <PlatformDefinition2.h>


STATIC VOID SmmClear()
{
  IoWrite16(PMIO_REG(PMIO_GLOBAL_STA),  PMIO_SWSMIS);///PMIO_Rx28[6] Software SMI Status
  IoOr16(PMIO_REG(PMIO_GLOBAL_CTL), PMIO_INSMI);     ///PMIO_Rx2C[8] SMI Active Status
}

STATIC VOID SmmTrigger(UINT8 Data8)
{
  IoOr16(PMIO_REG(PMIO_GLOBAL_ENABLE), PMIO_SWSMIEN);///PMIO_Rx2A[6] SW SMI Enable
  IoOr16(PMIO_REG(PMIO_GLOBAL_CTL), PMIO_SMIEN);  ///PMIO_Rx2C[0] SMI Enable
  IoWrite8(PMIO_REG(PMIO_SW_SMI_CMD), Data8);         ///PMIO_Rx2F[7:0] Software SMI Command
}
  
EFI_STATUS
EFIAPI
PeiSmmTrigger (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PEI_SMM_CONTROL_PPI   *This,
  IN OUT INT8              *ArgumentBuffer     OPTIONAL,
  IN OUT UINTN             *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN               Periodic            OPTIONAL,
  IN UINTN                 ActivationInterval  OPTIONAL
)
{
  UINT8  Data8;


//DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  if (ArgumentBuffer == NULL) {
    Data8 = 0xFF;
  } else {
    if (ArgumentBufferSize == NULL || *ArgumentBufferSize != 1) {
      return EFI_INVALID_PARAMETER;
    }
    Data8 = *ArgumentBuffer;
  }

  SmmClear();
  SmmTrigger(Data8);
  
  return EFI_SUCCESS;
}  


EFI_STATUS
EFIAPI
PeiSmmClear (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PEI_SMM_CONTROL_PPI   *This,
  IN BOOLEAN               Periodic OPTIONAL
)
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  SmmClear();
  return EFI_SUCCESS;
}

STATIC PEI_SMM_CONTROL_PPI  gSmmControlPpi = {
  PeiSmmTrigger,
  PeiSmmClear
};


STATIC EFI_PEI_PPI_DESCRIPTOR  gPpiInstallList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPeiSmmControlPpiGuid,
    &gSmmControlPpi
  }
};


EFI_STATUS
SmmControlPpiInstall (
  IN CONST EFI_PEI_SERVICES  **PeiServices
)
{
  EFI_STATUS  Status;
  
  Status = PeiServicesInstallPpi(&gPpiInstallList[0]);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

VOID IssueS3PeiEndSwSmi(EFI_PEI_SERVICES **PeiServices)
{
  UINT8                SmiCommand;
  UINTN                Size;  
  PEI_SMM_CONTROL_PPI  *SmmControl;

  SmiCommand = EFI_ACPI_S3_PEI_END_SW_SMI;
  Size = sizeof(SmiCommand);
  SmmControl = &gSmmControlPpi;
  
  SmmControl->Trigger (
                PeiServices,
                SmmControl,
                (INT8*)&SmiCommand,
                &Size,
                FALSE,
                0
                );
}



