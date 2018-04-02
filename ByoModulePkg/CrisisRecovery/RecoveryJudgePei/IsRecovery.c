/** @file
  This file contain functions to check main BIOS integrity and
  specifal GPIO jumper state, then determine if system should be
  in Recovery Mode.

Revision History:

BUG 2957    Add some port 80 status codes into EDKII code.
TIME: 2011-10-08
$AUTHOR:    Zhong Gangping
$REVIEWERS:
$SCOPE: All Platforms
$TECHNICAL: 1. Add 80 status codes when judging if recovery valid.

**/
#include "IsRecovery.h"
#include "FvExtChecksum.h"
#include "Library\PcdLib.h"
#include "Pi\PiFirmwareVolume.h"
#include "Library\BaseMemoryLib.h"
#include "Library\ReportStatusCodeLib.h"
#include "Library\BaseLib.h"
#include "Library\PerformanceLib.h"
#include "Ppi\FirmwareVolume.h"
#include "Library\PeiServicesLib.h"
#include "Guid\FirmwareFileSystem2.h"
#include "Library\DebugLib.h"
#include "Library\TimerLib.h"
#include "Library\DebugLib.h"

PEI_RECOVERY_SERVICE    mRecoveryPpi = {
  PeiRecoveryValidCheck
};

EFI_PEI_PPI_DESCRIPTOR  mRecoveryPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiRecoveryJudgePpiGuid,
  &mRecoveryPpi
};

/**
  Detect platform recovery mode

  @param  PeiServices   General purpose services available to
                        every PEIM

  @retval EFI_SUCCESS       System in Recovery Mode
  @retval EFI_NOT_FOUND     System is not in Recovery Mode

**/
EFI_STATUS
PeiRecoveryValidCheck (
  IN EFI_PEI_SERVICES          **PeiServices
)
{
  EFI_STATUS    Status;
  //
  // Check main BIOS integrity
  //
  PERF_START (NULL,"CHKsum", NULL, 0);
  Status = CheckMainBIOSCorrupted (PeiServices);
  PERF_END   (NULL,"CHKsum", NULL, 0);
  if(!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  This a internal function to check if a FFS file is FV extented
  checksum file

  @param  FileHeader        A pointer to FFS header

  @retval TRUE       It's a FV extented checksum file
  @retval FALSE      It's not a FV extented checksum file

**/
BOOLEAN
IsFvExtChecksumFile (
  EFI_FFS_FILE_HEADER     *FileHeader
)
{
  return CompareGuid(&FileHeader->Name, &gFvExtChecksumFileNameGuid);
}

/**
  This a internal function to check if main BIOS is corrupted

  @param  PeiServices       A pointer point to PeiServices

  @retval EFI_SUCCESS       Main BIOS is corrupted
  @retval EFI_UNSUPPORTED   Not support this function
  @retval EFI_NOT_FOUND     Main BIOS integrity is OK

**/
EFI_STATUS
CheckMainBIOSCorrupted (
  IN EFI_PEI_SERVICES     **PeiServices
)
{
  EFI_STATUS                          Status;
  EFI_PEI_FILE_HANDLE                 FileHandle;
  EFI_PEI_FV_HANDLE                   FvHandle;
  UINTN                               Offset;
  UINT32                              *Ptr;
  UINT32                              CheckSum;
  EFI_FIRMWARE_VOLUME_EXT_CHECKSUM    *ExtChecksum;
  EFI_FIRMWARE_VOLUME_HEADER          *FirmwareVolumeHeader;
  EFI_PEI_PPI_DESCRIPTOR              *PpiDescriptor;
  EFI_PEI_FIRMWARE_VOLUME_PPI         *Ffs2FvPpi;

  FileHandle = NULL;
  FvHandle = (EFI_PEI_FV_HANDLE)(UINTN)PcdGet32(PcdFlashFvMainBase);
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32(PcdFlashFvMainBase);

  Status = PeiServicesLocatePpi (
              &gEfiFirmwareFileSystem2Guid,
              0,
              &PpiDescriptor,
              &Ffs2FvPpi
              );
  ASSERT_EFI_ERROR (Status);

  Status = Ffs2FvPpi->FindFileByName (
                        Ffs2FvPpi,
                        &gFvExtChecksumFileNameGuid,
                        &FvHandle,
                        &FileHandle
                        );
  //
  // if not find the file we need, it means bios was corrupted.
  //
  if (EFI_ERROR(Status)) {
    return EFI_SUCCESS;
  }

  ExtChecksum = (EFI_FIRMWARE_VOLUME_EXT_CHECKSUM *)((UINTN)FileHandle + sizeof(EFI_FFS_FILE_HEADER));
  //
  // caculate Fv Ext checksum
  //
  CheckSum = 0;
  Ptr = (UINT32 *)FirmwareVolumeHeader;

  for (Offset = 0; Offset < FirmwareVolumeHeader->FvLength; Offset = Offset + FV_CHECK_ALIGNMENT) {
    //
    // Skip to next alignment point when the address overlap the Extend checsum file section data
    //
    if (((UINTN)Ptr + Offset) + sizeof(CheckSum) >= (UINTN)ExtChecksum && (UINTN)Ptr + Offset <= ((UINTN)ExtChecksum + sizeof(EFI_FIRMWARE_VOLUME_EXT_CHECKSUM))) {
      continue;
    }

    CheckSum = CheckSum + *(UINT32 *)((UINTN)Ptr + Offset);
  }
  CheckSum = ~CheckSum + 1;
  //
  // compare
  //
  if(ExtChecksum->Checksum != CheckSum)
    return  EFI_SUCCESS;

  return EFI_NOT_FOUND;
}

/**
  This is a PEIM entry, which is used to initailize Crisis
  Recovery function

  @param  FfsHeader     Pointer to the FFS file header
  @param  PeiServices   General purpose services available to
                        every PEIM.

  @return EFI_SUCCESS
**/
EFI_STATUS
RecoveryPeiEntry (
  IN EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES   **PeiServices
)
{
  EFI_STATUS  Status = EFI_UNSUPPORTED;

  Status = (**PeiServices).InstallPpi (PeiServices, &mRecoveryPpiList);
    return Status;
}




