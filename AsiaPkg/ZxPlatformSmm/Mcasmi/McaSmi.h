#ifndef __MCA_SMI_H__
#define __MCA_SMI_H__

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SmmServicesTableLib.h>
//#include <Protocol/SmmSxDispatch2.h>
//#include <Protocol/SmmSwDispatch2.h>
//#include <Protocol/SmmPowerButtonDispatch2.h>
//#include <Protocol/SmmUsbDispatch2.h>
//#include <Protocol/SmmPeriodicTimerDispatch2.h>
//#include <Protocol/SmmGeneralDispatch2.h>
#include <Protocol/PciRootBridgeIo.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <Framework/SmmCis.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmCpu.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>



#define PM_BASE_ADDRESS 0x800


//===========================================================
//
//    MCA in SMM init
//
//
//===========================================================

#define BIT(x)                (1ull<<(x))

#define MSR_IA32_MCG_CAP        0x00000179
#define MSR_IA32_MCG_STATUS     0x0000017a
#define MSR_IA32_MCG_CTL        0x0000017b

// MC0: Core
#define  MC0_CORE_CTL          0x400
#define  MC0_CORE_STATUS       0x401
#define  MC0_CORE_ADDR         0x402
#define  MC0_CORE_MISC         0x403
#define  MC0_CORE_CTL2         0x280

// MCG_STATUS register defines
#define MCG_STATUS_RIPV_BIT     (1ULL << 0)   // restart ip valid
#define MCG_STATUS_EIPV_BIT     (1ULL << 1)   // ip points to correct instruction
#define MCG_STATUS_MCIP_BIT     (1ULL << 2)   // machine check in progress
#define MCG_STATUS_MASK  \
    (MCG_STATUS_RIPV_BIT | MCG_STATUS_EIPV_BIT | MCG_STATUS_MCIP_BIT )

#define MSR_IA32_MCx_CTL(x)         (MC0_CORE_CTL + 4*(x))
#define MSR_IA32_MCx_STATUS(x)      (MC0_CORE_STATUS + 4*(x))
#define MSR_IA32_MCx_ADDR(x)        (MC0_CORE_ADDR + 4*(x))
#define MSR_IA32_MCx_MISC(x)        (MC0_CORE_MISC + 4*(x))
#define MSR_IA32_MCx_CTL2(x)        (MC0_CORE_CTL2 + (x))

#define MCI_CTL2_CSMI_EN_BIT            (1ULL << 32)
#define MCI_CTL2_MSMI_EN_BIT            (1ULL << 34)

#define MCI_STS_UC_BIT                  (1ULL << 61)
#define MCI_STS_VAL_BIT                 (1ULL << 63)

#define DEAD_MESSAGE(x) {\
IoWrite8(0x80, x);\
while(1);\
}

#define BJ_MCA_ZX_CTRL2_MSR                 0x1617

typedef struct {
  UINT8   Register;
  UINT8   Function;
  UINT8   Device;
  UINT8   Bus;
  UINT32  ExtendedRegister;
} EFI_PCI_CONFIGURATION_ADDRESS;

#endif
