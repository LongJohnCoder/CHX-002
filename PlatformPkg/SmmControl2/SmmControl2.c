
#include <Guid/EventGroup.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Protocol/SmmControl2.h>
//#include <Protocol/SmmControl.h>
#include <PlatformDefinition.h>


STATIC VOID SmmClear()
{
  IoWrite16(PMIO_REG(PMIO_GBLSTS_REG),  PMIO_GBLSTS_SWSMI); ///PMIO_Rx28[6] Software SMI Status
  IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIACTIVE);      ///PMIO_Rx2C[8] SMI Active Status
}

STATIC VOID SmmTrigger(UINT8 Data8)
{
	
  IoOr16(PMIO_REG(PMIO_GBLEN_REG), PMIO_GBLEN_SWSMI);        ///PMIO_Rx2A[6] SW SMI Enable
  IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIEN);           ///PMIO_Rx2C[0] SMI Enable
  
  if(IoRead16(PMIO_REG(PMIO_GBLCTRL_REG))&PMIO_GC_SMIACTIVE){  ///PMIO_Rx2C[8] SMI Active Status
    DEBUG((EFI_D_ERROR, "[ERROR] LAST SMI NOT CLEAR!!!\n"));
    IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIACTIVE);           ///PMIO_Rx2C[8] SMI Active Status
    if(IoRead16(PMIO_REG(PMIO_GBLCTRL_REG))&PMIO_GC_SMIACTIVE){///PMIO_Rx2C[8] SMI Active Status
      DEBUG((EFI_D_ERROR, "[ERROR] LAST SMI CAN NOT CLEAR!!! Try To Clear Status...\n"));
      IoWrite16(PMIO_REG(PMIO_GBLSTS_REG), IoRead16(PMIO_REG(PMIO_GBLSTS_REG))); ///PMIO_Rx28[15:0] Global Status
      IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIACTIVE); ///PMIO_Rx2C[8] SMI Active Status
      ASSERT(!(IoRead16(PMIO_REG(PMIO_GBLCTRL_REG))&PMIO_GC_SMIACTIVE)); ///PMIO_Rx2C[8] SMI Active Status
    }
  }
  
  IoWrite8(PMIO_REG(PMIO_SWSMI_CMD_REG), Data8); ///PMIO_Rx2F[7:0] Software SMI Command
}

STATIC
EFI_STATUS
EFIAPI
SmmControl2Clear (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN BOOLEAN                          Periodic OPTIONAL
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  SmmClear();
  return EFI_SUCCESS;  
}


STATIC
EFI_STATUS
EFIAPI
SmmControl2Trigger (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN OUT UINT8                        *CommandPort,       OPTIONAL
  IN OUT UINT8                        *DataPort,          OPTIONAL
  IN BOOLEAN                          Periodic,           OPTIONAL
  IN UINTN                            ActivationInterval  OPTIONAL
  )
{
  UINT8       Data8;
  EFI_STATUS  Status;

  
  Status = EFI_SUCCESS;
  if (Periodic) {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  if(CommandPort == NULL){
    Data8 = 0;
  } else {
    Data8 = *CommandPort;
  }
  
  SmmClear();
  SmmTrigger(Data8);
  SmmClear();

ProcExit:  
  return Status;
}
  
STATIC EFI_SMM_CONTROL2_PROTOCOL gSmmControl2 = {
  SmmControl2Trigger,
  SmmControl2Clear,
  0
};


/*
STATIC
EFI_STATUS
EFIAPI
SmmControlTrigger (
  IN EFI_SMM_CONTROL_PROTOCOL  *This,
  IN OUT INT8                  *ArgumentBuffer     OPTIONAL,
  IN OUT UINTN                 *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                   Periodic            OPTIONAL,
  IN UINTN                     ActivationInterval  OPTIONAL
  )
{
  UINT8       Data8;

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

STATIC
EFI_STATUS
EFIAPI
SmmControlClear (
  IN EFI_SMM_CONTROL_PROTOCOL  *This,
  IN BOOLEAN                   Periodic OPTIONAL
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }
	
  SmmClear();
  return EFI_SUCCESS;  
}

STATIC
EFI_STATUS
EFIAPI
SmmControlGetRegisterInfo (
  IN EFI_SMM_CONTROL_PROTOCOL           *This,
  IN OUT EFI_SMM_CONTROL_REGISTER       *SmiRegister
  )
{
  if (SmiRegister == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SmiRegister->SmiTriggerRegister = SB_SWSMI_PORT_REG;
  SmiRegister->SmiDataRegister    = SB_SWSMI_DATA_REG;
  return EFI_SUCCESS;
}

STATIC EFI_SMM_CONTROL_PROTOCOL gSmmControl = {
  SmmControlTrigger,
  SmmControlClear,
  SmmControlGetRegisterInfo,
  0
};
*/


STATIC
VOID
DisablePendingSmis (
  VOID
  )
{
  IoWrite16(PMIO_REG(PMIO_STS_REG), IoRead16(PMIO_REG(PMIO_STS_REG)));///PMIO_Rx00[15:0] Power Management Status
  IoWrite16(PMIO_REG(PMIO_PM_EN),       0x0000);       ///PMIO_Rx02[15:0] Power Management Enable
  IoWrite16(PMIO_REG(PMIO_PM1_CNT_REG), 0x0000);///PMIO_Rx04[15:0] Power Management Control 
  IoWrite16(PMIO_REG(PMIO_GP_STS), IoRead16(PMIO_REG(PMIO_GP_STS)));///PMIO_Rx20[15:0] General Purpose Status
  IoWrite16(PMIO_REG(PMIO_GP_SMI_EN),   0x0000);  ///PMIO Rx24[15:0] General Purpose SMI / Resume Enable
  IoWrite16(PMIO_REG(PMIO_GBLSTS_REG), IoRead16(PMIO_REG(PMIO_GBLSTS_REG)));///PMIO_Rx28[15:0] Global Status
  IoWrite16(PMIO_REG(PMIO_GBLEN_REG),   0x0200); ///PMIO Rx2A[15:0] Global Enable    

}
  

  
VOID
EFIAPI
SmmControlVirtualddressChangeEvent (
  IN EFI_EVENT                  Event,
  IN VOID                       *Context
  )
{
  gRT->ConvertPointer (0, (VOID**)&(gSmmControl2.Trigger));
  gRT->ConvertPointer (0, (VOID**)&(gSmmControl2.Clear));
  
//gRT->ConvertPointer (0, (VOID**)&(gSmmControl.Trigger));
//gRT->ConvertPointer (0, (VOID**)&(gSmmControl.Clear));
//gRT->ConvertPointer (0, (VOID**)&(gSmmControl.GetRegisterInfo));    
}
  
  
  
EFI_STATUS
SmmControl2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  
  DisablePendingSmis();
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiSmmControl2ProtocolGuid, &gSmmControl2,
//-               &gEfiSmmControlProtocolGuid,  &gSmmControl,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SmmControlVirtualddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);  
  
  return Status;
}

