
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/VariableLock.h>
#include <Protocol/ExitPmAuth.h>
#include "Tcg2ConfigNvData.h"


VOID
EFIAPI
Tcg2ExitPmAuthCallBack (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EDKII_VARIABLE_LOCK_PROTOCOL  *VarLock;
  TCG2_DEVICE_DETECTION         DevId;
  UINTN                         DevIdSize;
  TCG2_DEVICE_DETECTION         NewDevId;  
  VOID                          *Interface;
  EFI_STATUS                    Status;  
  
  
  Status = gBS->LocateProtocol(&gExitPmAuthProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));


  NewDevId.TpmDeviceDetected = PcdGet8(PcdTpmInstanceId);

  DevId.TpmDeviceDetected = 0xFF;
  DevIdSize = sizeof(DevId);
  Status = gRT->GetVariable (
                  TCG2_DEVICE_DETECTION_NAME,
                  &gTcg2ConfigFormSetGuid,
                  NULL,
                  &DevIdSize,
                  &DevId
                  );
  DEBUG((EFI_D_INFO, "%X -> %X\n", DevId.TpmDeviceDetected, NewDevId.TpmDeviceDetected));
  if(EFI_ERROR(Status) || NewDevId.TpmDeviceDetected != DevId.TpmDeviceDetected){
    Status = gRT->SetVariable (
                    TCG2_DEVICE_DETECTION_NAME,
                    &gTcg2ConfigFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(NewDevId),
                    &NewDevId
                    );
    DEBUG((EFI_D_INFO, "Update:%r\n", Status));
  }
  
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VarLock);
  if (!EFI_ERROR (Status)) {
    Status = VarLock->RequestToLock (
                        VarLock,
                        TCG2_DEVICE_DETECTION_NAME,
                        &gTcg2ConfigFormSetGuid
                       );
    ASSERT_EFI_ERROR (Status);
  }  
}





EFI_STATUS
Tcg2Config2DriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  VOID          *Registration;   
    
  EfiCreateProtocolNotifyEvent (
    &gExitPmAuthProtocolGuid,
    TPL_CALLBACK,
    Tcg2ExitPmAuthCallBack,
    NULL,
    &Registration
    );   

  return EFI_SUCCESS;  
}  