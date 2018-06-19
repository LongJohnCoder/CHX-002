#ifndef _MP_CONFIG_PROTOCOL_H_
#define _MP_CONFIG_PROTOCOL_H_
#ifdef ZX_SECRET_CODE

///
/// Global ID for the EFI_MP_SERVICES_PROTOCOL.
///
#define EFI_MP_CONFIG_PROTOCOL_GUID \
  { \
    0x3fdda605, 0xa76e, 0x4f46, {0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0xd3, 0x06} \
  }
typedef enum {
  SEC0,
  SEC1,
  PEI0,
  PEI1,
  DXE,
  RDB
} MSR_PHASE;

typedef EFI_STATUS (EFIAPI * ZX_CPU_MSR_CONFIG) (
    IN MSR_PHASE Phase
);
typedef struct _EFI_MP_CONFIG_PROTOCOL {
   ZX_CPU_MSR_CONFIG ConfigMsr; 
} EFI_MP_CONFIG_PROTOCOL;

extern EFI_GUID gEfiCpuMpConfigProtocolGuid;
#endif
#endif