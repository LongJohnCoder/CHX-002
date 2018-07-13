#include "SmmHelpers.h"
#include "SbSmm.h"
#include "PlatformDefinition.h"

SB_SMM_SOURCE_DESC mTIMER_SOURCE_DESCS = {
  SB_SMM_NO_FLAGS,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GLOBAL_ENABLE
      },
      2,
      12
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GLOBAL_STA
      },
      2,
      12
    }
  }
};

VOID
MapPeriodicTimerToSrcDesc (
  IN  SB_SMM_CONTEXT             *DispatchContext,
  OUT SB_SMM_SOURCE_DESC         *SrcDesc
  )
{
  UINT8   TimerCount;
  UINT8   Data;

  TimerCount = (UINT8)DispatchContext->PeriodicTimer.SmiTickInterval;

  CopyMem (
    (VOID *) SrcDesc,
    (VOID *) (&mTIMER_SOURCE_DESCS),
    sizeof (SB_SMM_SOURCE_DESC)
    );

  //
  // Program the value of the interval into hardware
  //
  ///MTN-2016110201-Start
  Data = MmioRead8(LPC_PCI_REG (D17F0_PMU_GP2_GP3_TIMER_CTL));
  MmioWrite8 (LPC_PCI_REG (D17F0_PMU_GP2_GP3_TIMER_CTL), Data| BIT2 | BIT0); // enable GP2 timer tick to x1 ms
  MmioWrite8 (LPC_PCI_REG (D17F0_PMU_GP2_TIMER_CNTER), TimerCount);  // GP2 Timer Counter   
  MmioOr8 (LPC_PCI_REG (D17F0_PMU_GP2_GP3_TIMER_CTL), BIT3);           // GP2 Timer Start
  ///MTN-2016110201-End
}

BOOLEAN
PeriodicTimerCmpContext (
  IN SB_SMM_CONTEXT     *HwContext,
  IN SB_SMM_CONTEXT     *ChildContext
  )
{
  return TRUE;
}


