#ifndef __PEI_MP_CONFIG_PPI_H__
#define __PEI_MP_CONFIG_PPI_H__

//#include <Protocol/MpService.h>
#ifdef ZX_SECRET_CODE

//cpu config ppi  {2bc92c61-2094-463e-8F75-38AD7F8AE986}
#define EFI_CPU_CONFIG_PPI_PROTOCOL_GUID \
{ \
0x2bc92c61, 0x2094, 0x463e, {0x8f, 0x75, 0x38, 0xad, 0x7f, 0x8a, 0xe9, 0x86} \
}

#define EFI_CONFIG_CPU_PPI EFI_CPU_CONFIG_PPI_PROTOCOL_GUID
typedef enum {
  SEC0,
  SEC1,
  PEI0,
  PEI1,
  DXE,
  RDB
} MSR_PHASE;

typedef EFI_STATUS (EFIAPI * ZX_CPU_MSR_CONFIG) (
	IN EFI_PEI_SERVICES  **PeiServices,
    IN MSR_PHASE phase
);
typedef struct _EFI_CPU_CONFIG_PPI_PROTOCOL {
   ZX_CPU_MSR_CONFIG ConfigMsr; 
} EFI_CPU_CONFIG_PPI_PROTOCOL;

extern EFI_GUID gCpuConfigPpiGuid; 
#endif

#endif

