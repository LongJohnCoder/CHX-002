#ifndef _CPU_MP_DXE_CONFIG_H_
#define _CPU_MP_DXE_CONFIG_H_
#include <Protocol/MpConfig.h>
#include <Protocol/MpService.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/Cpu.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/Timer.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/EventGroup.h>
#include <Guid/IdleLoopEvent.h>
#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/CpuLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/MtrrLib.h>
#include <Library/ZhaoXinCpuLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LocalApicLib.h>

#ifdef ZX_SECRET_CODE




#pragma pack (1)
typedef struct _MSR_Format{

  //Byte0-3:SR Address
  UINT32 Address;
  //Byte4-7:Core Mask;whichi Core need set;Bit0=cor0,bit1=core1,bit31=core31
  UINT32 CoreMask;
  //Byte8:bit 0 mean:set order
  //       1:order set(core0,cor1...);
  //       0:Reverse order(core max,..core 0)
  //      bit 1 mean:OperateType
  //              0:+/- OrMask
  //              1: | OrMask
  //      bit 2 mean: Add or Sub
  //              0: +
  //              1: -
  //other bit :reserved
  UINT8  SetOrder:1;
  UINT8  OperateType:1;
  UINT8  AddOrSub:1;
  UINT8  Byte8Reserved:5;
  //Byte9-16:temp Reserved
  UINT8  Reserved[7];
  //Byte16-23:Nand Mask
  UINT64 AndMask;
  //Byte24-31:Or Mask
  UINT64 OrMask;
}MSR_Format;
#pragma pack ()

#endif
#endif
