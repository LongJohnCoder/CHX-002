/** @file
  This file contain functions to check main BIOS integrity and
  specifal GPIO jumper state, then determine if system should be
  in Recovery Mode.

**/

#ifndef __IS_RECOVERY_PEI_H__
#define __IS_RECOVERY_PEI_H__

#include "Ppi\IsRecovery.h" 

EFI_STATUS
PeiRecoveryValidCheck (
  IN EFI_PEI_SERVICES          **PeiServices
  );


EFI_STATUS
CheckGPIOJumper (
  IN EFI_PEI_SERVICES          **PeiServices
  );

EFI_STATUS
CheckMainBIOSCorrupted (
  IN EFI_PEI_SERVICES     **PeiServices
  );

BOOLEAN
VerifyFFSIntegrity (
  IN IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_FFS_FILE_HEADER      *FFSFileHeader
);

#endif
