
#include "PlatformDxe.h"
#include <Protocol/IsaAcpi.h>
#include <Protocol/IsaIo.h>


#define MAX_ISA_ACPI_DEVICE_COUNT               16

extern EFI_GUID gIsaAcpiDeviceListGuid;

//
// COM 1 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom1DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x3f8, 0x3ff},
  {EfiIsaAcpiResourceInterrupt, 0, 4,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// COM 2 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom2DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x2f8, 0x2ff},
  {EfiIsaAcpiResourceInterrupt, 0, 3,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// COM 3 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom3DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x3e8, 0x2ef},
  {EfiIsaAcpiResourceInterrupt, 0, 3,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// COM 4 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom4DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x2e8, 0x2ef},
  {EfiIsaAcpiResourceInterrupt, 0, 3,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// PS/2 Keyboard Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE  mPcatIsaAcpiPs2KeyboardDeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x60, 0x64},
  {EfiIsaAcpiResourceInterrupt, 0, 1,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// PS/2 Mouse Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE  mPcatIsaAcpiPs2MouseDeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x60, 0x64},
  {EfiIsaAcpiResourceInterrupt, 0, 12,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// Floppy Disk Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiFloppyResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x3f0, 0x3f7},
  {EfiIsaAcpiResourceInterrupt, 0, 6,     0},
  {EfiIsaAcpiResourceDma,       EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE | EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8 | EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE, 2,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom1Device = {
  {EISA_PNP_ID(0x501), 0}, mPcatIsaAcpiCom1DeviceResources
}; // COM 1 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom2Device = {
  {EISA_PNP_ID(0x501), 1}, mPcatIsaAcpiCom2DeviceResources
}; // COM 2 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom3Device = {
  {EISA_PNP_ID(0x501), 2}, mPcatIsaAcpiCom3DeviceResources
}; // COM 1 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom4Device = {
  {EISA_PNP_ID(0x501), 3}, mPcatIsaAcpiCom4DeviceResources
}; // COM 2 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiPs2KeyboardDevice = {
  {EISA_PNP_ID(0x303), 0}, mPcatIsaAcpiPs2KeyboardDeviceResources
}; // PS/2 Keyboard Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiPs2MouseDevice = {
  {EISA_PNP_ID(0x303), 1}, mPcatIsaAcpiPs2MouseDeviceResources
}; // PS/2 Mouse Controller

EFI_ISA_ACPI_RESOURCE_LIST gIsaAcpiDevList[MAX_ISA_ACPI_DEVICE_COUNT+1]; // should big enough.
CONST UINT16 gComBaseList[] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};


EFI_STATUS IsaAcpiDevListDxe()
{
  UINTN                       Index = 0;
  EFI_ISA_ACPI_RESOURCE_LIST  *List;
  EFI_ISA_ACPI_RESOURCE_LIST  *TargetList;
  EFI_STATUS                  Status;

  
  ZeroMem(gIsaAcpiDevList, sizeof(gIsaAcpiDevList));
  TargetList = gIsaAcpiDevList;

  if(gSetupData->Uart0Enable){
    List = &mPcatIsaAcpiCom1Device;
    List->ResourceItem[0].StartRange = gComBaseList[gSetupData->Uart0IoBaseSelection];
    List->ResourceItem[0].EndRange   = List->ResourceItem[0].StartRange + 7;
    List->ResourceItem[1].StartRange = gSetupData->Uart0IRQSelection;
    CopyMem(&TargetList[Index++], List, sizeof(*List));
    
  }  
  if(gSetupData->Uart1Enable){
    List = &mPcatIsaAcpiCom2Device;
    List->ResourceItem[0].StartRange = gComBaseList[gSetupData->Uart1IoBaseSelection];
    List->ResourceItem[0].EndRange   = List->ResourceItem[0].StartRange + 7;
    List->ResourceItem[1].StartRange = gSetupData->Uart1IRQSelection;
    CopyMem(&TargetList[Index++], List, sizeof(*List));
  } 
  /*  
  if(gSetupData->Uart2Enable){
    List = &mPcatIsaAcpiCom3Device;
    List->ResourceItem[0].StartRange = gComBaseList[gSetupData->Uart2IoBaseSelection];
    List->ResourceItem[0].EndRange   = List->ResourceItem[0].StartRange + 7;
    List->ResourceItem[1].StartRange = gSetupData->Uart2IRQSelection;
    CopyMem(&TargetList[Index++], List, sizeof(*List));
  }      
  if(gSetupData->Uart3Enable){
    List = &mPcatIsaAcpiCom4Device;
    List->ResourceItem[0].StartRange = gComBaseList[gSetupData->Uart3IoBaseSelection];
    List->ResourceItem[0].EndRange   = List->ResourceItem[0].StartRange + 7;
    List->ResourceItem[1].StartRange = gSetupData->Uart3IRQSelection;
    CopyMem(&TargetList[Index++], List, sizeof(*List));
  }   
  */
  CopyMem(&TargetList[Index++], &mPcatIsaAcpiPs2KeyboardDevice, sizeof(EFI_ISA_ACPI_RESOURCE_LIST));

  ASSERT(Index <= MAX_ISA_ACPI_DEVICE_COUNT);

  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gIsaAcpiDeviceListGuid,
                  EFI_NATIVE_INTERFACE,
                  TargetList
                  );

  return Status;

}








