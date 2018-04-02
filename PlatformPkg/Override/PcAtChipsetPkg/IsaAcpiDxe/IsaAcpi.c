/** @file
  ISA ACPI Protocol Implementation

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#include "PcatIsaAcpi.h"
#include <Library/DebugLib.h>


extern EFI_GUID gIsaAcpiDeviceListGuid;

EFI_ISA_ACPI_RESOURCE_LIST *gPcatIsaAcpiDeviceList = NULL;

VOID
InitializePcatIsaAcpiDeviceList (
  VOID
  )
{
  EFI_STATUS                  Status;  
  EFI_HANDLE                  *HandleBuffer = NULL;
  UINTN                       HandleCount;
  UINTN                       Index;
  UINTN                       Index2;
  UINTN                       DevCount = 0;
  UINTN                       CurIdx;
  EFI_ISA_ACPI_RESOURCE_LIST  *List;
  EFI_ISA_ACPI_RESOURCE_LIST  *TmpList;


  if(gPcatIsaAcpiDeviceList != NULL){
    gBS->FreePool(gPcatIsaAcpiDeviceList);
    gPcatIsaAcpiDeviceList = NULL;
  }
  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gIsaAcpiDeviceListGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if(EFI_ERROR(Status) || HandleCount == 0){
    goto ProcExit;
  }

  for(Index=0; Index<HandleCount; Index++){
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gIsaAcpiDeviceListGuid,
                    (VOID**)&List
                    );
    for(Index2 = 0; List[Index2].ResourceItem != NULL; Index2++) {
      DevCount++;
    }    
  }

  Status = gBS->AllocatePool(
                  EfiBootServicesData, 
                  (DevCount+1) * sizeof(EFI_ISA_ACPI_RESOURCE_LIST), 
                  (VOID**)&TmpList
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  CurIdx = 0;
  for(Index=0; Index<HandleCount; Index++){
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gIsaAcpiDeviceListGuid,
                    (VOID**)&List
                    );
    for(Index2 = 0; List[Index2].ResourceItem != NULL; Index2++) {
      CopyMem(&TmpList[CurIdx++], &List[Index2], sizeof(EFI_ISA_ACPI_RESOURCE_LIST));
      DEBUG((EFI_D_INFO, "HID:%X\n", List[Index2].Device.HID));
    }    
  }  

  TmpList[CurIdx].ResourceItem = NULL;

  gPcatIsaAcpiDeviceList = TmpList;

ProcExit:
  if(HandleBuffer != NULL){
    gBS->FreePool(HandleBuffer);
  }
}


//
// ISA ACPI Protocol Functions
//
/**
  Enumerate the ISA devices on the ISA bus.

  @param Device             Point to device ID instance 
  @param IsaAcpiDevice      On return, point to resource data for Isa device
  @param NextIsaAcpiDevice  On return, point to resource data for next Isa device
**/
VOID
IsaDeviceLookup (
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **IsaAcpiDevice,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **NextIsaAcpiDevice
  )
{
  UINTN  Index;

  *IsaAcpiDevice = NULL;
  if (NextIsaAcpiDevice != NULL) {
    *NextIsaAcpiDevice = NULL;
  }

  if(gPcatIsaAcpiDeviceList == NULL){
    return;
  }
  
  if (Device == NULL) {
    Index = 0;
  } else {
    for(Index = 0; gPcatIsaAcpiDeviceList[Index].ResourceItem != NULL; Index++) {
      if (Device->HID == gPcatIsaAcpiDeviceList[Index].Device.HID && 
          Device->UID == gPcatIsaAcpiDeviceList[Index].Device.UID    ) {
        break;
      }
    }
    if (gPcatIsaAcpiDeviceList[Index].ResourceItem == NULL) {
      return;
    }
    *IsaAcpiDevice = &(gPcatIsaAcpiDeviceList[Index]);
    Index++;
  }
  if (gPcatIsaAcpiDeviceList[Index].ResourceItem != NULL && NextIsaAcpiDevice != NULL) {
    *NextIsaAcpiDevice = &(gPcatIsaAcpiDeviceList[Index]);
  }
}

/**
  Enumerate the ISA devices on the ISA bus


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 

  @retval EFI_NOT_FOUND Can not found the next Isa device.
  @retval EFI_SUCESS    Success retrieve the next Isa device for enumration.

**/
EFI_STATUS
EFIAPI
IsaDeviceEnumerate (
  IN  EFI_ISA_ACPI_PROTOCOL   *This,
  OUT EFI_ISA_ACPI_DEVICE_ID  **Device
  )
{
  EFI_ISA_ACPI_RESOURCE_LIST  *IsaAcpiDevice;
  EFI_ISA_ACPI_RESOURCE_LIST  *NextIsaAcpiDevice;

  IsaDeviceLookup (*Device, &IsaAcpiDevice, &NextIsaAcpiDevice);
  if (NextIsaAcpiDevice == NULL) {
    return EFI_NOT_FOUND;
  }
  *Device = &(NextIsaAcpiDevice->Device);
  return EFI_SUCCESS;
}

/**
  Set ISA device power


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param OnOff           TRUE for setting isa device power on,
                         FALSE for setting isa device power off

  @return EFI_SUCCESS    Sucess to change power status for isa device.
**/
EFI_STATUS
EFIAPI
IsaDeviceSetPower (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 OnOff
  )
{
  return EFI_SUCCESS;
} 

/**
  Get current resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_NOT_FOUND Can not found the resource instance for given isa device
  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetCurrentResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,  
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  )
{
  IsaDeviceLookup (Device, ResourceList, NULL);
  if (*ResourceList == NULL) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  Get possible resource for the specific ISA device.
  
  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetPossibleResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,  
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  )
{
  return EFI_SUCCESS;
}

/**
  Set resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    Point to resources instances for given isa device

  @return EFI_SUCESS  Success to set resource.

**/
EFI_STATUS
EFIAPI
IsaSetResource (
  IN EFI_ISA_ACPI_PROTOCOL       *This,
  IN EFI_ISA_ACPI_DEVICE_ID      *Device,  
  IN EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  )
{
  return EFI_SUCCESS;
}
        
/**
  Enable/Disable the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param Enable          Enable/Disable

  @return EFI_SUCESS  Success to enable/disable.

**/
EFI_STATUS
EFIAPI
IsaEnableDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 Enable
  )
{
  return EFI_SUCCESS;  
}

/**
  Initialize the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 

  @return EFI_SUCESS  Success to initialize.

**/
EFI_STATUS
EFIAPI
IsaInitDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device
  )
{
  return EFI_SUCCESS;
}


/**
  Initialize the ISA interface.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL

  @return EFI_SUCESS  Success to initialize ISA interface.

**/
EFI_STATUS
EFIAPI
IsaInterfaceInit (
  IN EFI_ISA_ACPI_PROTOCOL  *This
)  
{
  return EFI_SUCCESS;
}  
