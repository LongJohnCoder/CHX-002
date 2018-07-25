//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************

#ifndef __CRB_SMI_H__
#define __CRB_SMI_H__
    
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmPowerButtonDispatch2.h>
#include <Protocol/SmmUsbDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/PciRootBridgeIo.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <Framework/SmmCis.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>





#define ECC_ERROR 1
#define PCIE_ERROR 2;
#define PM_BASE_ADDRESS 0x800


typedef struct {
  UINT8   Register;
  UINT8   Function;
  UINT8   Device;
  UINT8   Bus;
  UINT32  ExtendedRegister;
} EFI_PCI_CONFIGURATION_ADDRESS;














#endif
