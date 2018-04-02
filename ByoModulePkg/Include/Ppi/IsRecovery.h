/*++

Module Name:

    IsRecovery.h

Abstract:

--*/
#ifndef __IS_RECOVERY_H__
#define __IS_RECOVERY_H__

//
// Global ID for recovery function in PEI
//
#define PEI_RECOVERY_SERVICE_PPI_GUID   \
    { \
        0xBCC610D9, 0x6C38, 0x4598, { 0x8E, 0x3E, 0x56, 0x56, 0x44, 0xE6, 0x47, 0x19 } \
    }

typedef
EFI_STATUS
(EFIAPI * PEI_RECOVERY_CHECK)(
    IN EFI_PEI_SERVICES          **PeiServices
    );

typedef struct {
    PEI_RECOVERY_CHECK          Check;
} PEI_RECOVERY_SERVICE;

extern EFI_GUID gPeiRecoveryJudgePpiGuid;
#endif
