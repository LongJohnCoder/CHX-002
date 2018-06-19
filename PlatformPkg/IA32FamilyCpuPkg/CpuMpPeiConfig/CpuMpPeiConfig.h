#ifndef _CPU_MP_PEI_CONFIG_H_
#define _CPU_MP_PEI_CONFIG_H_
#include <PiPei.h>
#include <Ppi/CpuMpConfig.h>
#include <Ppi/MpServices.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>

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
