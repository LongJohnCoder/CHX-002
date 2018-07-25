//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************

#ifndef __IO_TRAP_H__
#define __IO_TRAP_H__

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SmmServicesTableLib.h>

#include <Protocol/PciRootBridgeIo.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <Framework/SmmCis.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>



#define PM_BASE_ADDRESS 0x800

#define BIT(x)                (1ull<<(x))

#define DEAD_MESSAGE(x) {\
IoWrite8(0x80, x);\
while(1);\
}

typedef struct {
  UINT8   Register;
  UINT8   Function;
  UINT8   Device;
  UINT8   Bus;
  UINT32  ExtendedRegister;
} EFI_PCI_CONFIGURATION_ADDRESS;

#endif
