/** @file
  This driver installs gEdkiiFaultTolerantWriteGuid PPI to inform
  the check for FTW last write data has been done.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#include <PiPei.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FaultTolerantWrite.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>


/**
  Get the last Write Header pointer.
  The last write header is the header whose 'complete' state hasn't been set.
  After all, this header may be a EMPTY header entry for next Allocate.


  @param FtwWorkSpaceHeader Pointer of the working block header
  @param FtwWorkSpaceSize   Size of the work space
  @param FtwWriteHeader     Pointer to retrieve the last write header

  @retval  EFI_SUCCESS      Get the last write record successfully
  @retval  EFI_ABORTED      The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER  *FtwWorkSpaceHeader,
  IN UINTN                                    FtwWorkSpaceSize,
  OUT EFI_FAULT_TOLERANT_WRITE_HEADER         **FtwWriteHeader
  )
{
  UINTN                           Offset;
  EFI_FAULT_TOLERANT_WRITE_HEADER *FtwHeader;

  *FtwWriteHeader = NULL;
  FtwHeader       = (EFI_FAULT_TOLERANT_WRITE_HEADER *) (FtwWorkSpaceHeader + 1);
  Offset          = sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER);

  while (FtwHeader->Complete == FTW_VALID_STATE) {
    Offset += FTW_WRITE_TOTAL_SIZE (FtwHeader->NumberOfWrites, FtwHeader->PrivateDataSize);
    //
    // If Offset exceed the FTW work space boudary, return error.
    //
    if (Offset >= FtwWorkSpaceSize) {
      *FtwWriteHeader = FtwHeader;
      return EFI_ABORTED;
    }

    FtwHeader = (EFI_FAULT_TOLERANT_WRITE_HEADER *) ((UINT8 *) FtwWorkSpaceHeader + Offset);
  }
  //
  // Last write header is found
  //
  *FtwWriteHeader = FtwHeader;

  return EFI_SUCCESS;
}

/**
  Get the last Write Record pointer. The last write Record is the Record
  whose DestinationCompleted state hasn't been set. After all, this Record
  may be a EMPTY record entry for next write.


  @param FtwWriteHeader  Pointer to the write record header
  @param FtwWriteRecord  Pointer to retrieve the last write record

  @retval EFI_SUCCESS        Get the last write record successfully
  @retval EFI_ABORTED        The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteRecord (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER          *FtwWriteHeader,
  OUT EFI_FAULT_TOLERANT_WRITE_RECORD         **FtwWriteRecord
  )
{
  UINTN                           Index;
  EFI_FAULT_TOLERANT_WRITE_RECORD *FtwRecord;

  *FtwWriteRecord = NULL;
  FtwRecord       = (EFI_FAULT_TOLERANT_WRITE_RECORD *) (FtwWriteHeader + 1);

  //
  // Try to find the last write record "that has not completed"
  //
  for (Index = 0; Index < FtwWriteHeader->NumberOfWrites; Index += 1) {
    if (FtwRecord->DestinationComplete != FTW_VALID_STATE) {
      //
      // The last write record is found
      //
      *FtwWriteRecord = FtwRecord;
      return EFI_SUCCESS;
    }

    FtwRecord++;

    if (FtwWriteHeader->PrivateDataSize != 0) {
      FtwRecord = (EFI_FAULT_TOLERANT_WRITE_RECORD *) ((UINTN) FtwRecord + (UINTN) FtwWriteHeader->PrivateDataSize);
    }
  }
  //
  //  if Index == NumberOfWrites, then
  //  the last record has been written successfully,
  //  but the Header->Complete Flag has not been set.
  //  also return the last record.
  //
  if (Index == FtwWriteHeader->NumberOfWrites) {
    *FtwWriteRecord = (EFI_FAULT_TOLERANT_WRITE_RECORD *) ((UINTN) FtwRecord - FTW_RECORD_SIZE (FtwWriteHeader->PrivateDataSize));
    return EFI_SUCCESS;
  }

  return EFI_ABORTED;
}

/**
  Check to see if it is a valid work space.


  @param WorkingHeader   Pointer of working block header
  @param WorkingLength   Working block length

  @retval TRUE          The work space is valid.
  @retval FALSE         The work space is invalid.

**/
BOOLEAN
IsValidWorkSpace (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER    *WorkingHeader,
  IN UINTN                                      WorkingLength
  )
{
  if (WorkingHeader == NULL) {
    return FALSE;
  }

  if ((WorkingHeader->WorkingBlockValid != FTW_VALID_STATE) || (WorkingHeader->WorkingBlockInvalid == FTW_VALID_STATE)) {
    DEBUG ((EFI_D_ERROR, "FtwPei: Work block header valid bit check error\n"));
    return FALSE;
  }

  if (WorkingHeader->WriteQueueSize != (WorkingLength - sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER))) {
    DEBUG ((EFI_D_ERROR, "FtwPei: Work block header WriteQueueSize check error\n"));
    return FALSE;
  }

  if (!CompareGuid (&gEfiSystemNvDataFvGuid, &WorkingHeader->Signature)) {
    DEBUG ((EFI_D_ERROR, "FtwPei: Signture Error\n"));
    return FALSE;
  }

  return TRUE;

}

/**
  Main entry for Fault Tolerant Write PEIM.

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()

**/
EFI_STATUS
EFIAPI
PeimFaultTolerantWriteInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                                Status;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER   *FtwWorkingBlockHeader;
  EFI_FAULT_TOLERANT_WRITE_HEADER           *FtwLastWriteHeader;
  EFI_FAULT_TOLERANT_WRITE_RECORD           *FtwLastWriteRecord;
  EFI_PHYSICAL_ADDRESS                      WorkSpaceAddress;
  UINTN                                     WorkSpaceLength;
  EFI_PHYSICAL_ADDRESS                      SpareAreaAddress;
  UINTN                                     SpareAreaLength;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA      FtwLastWrite;

  FtwWorkingBlockHeader = NULL;
  FtwLastWriteHeader = NULL;
  FtwLastWriteRecord = NULL;

  WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageFtwWorkingBase64);
  if (WorkSpaceAddress == 0) {
    WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  }
  WorkSpaceLength = (UINTN) PcdGet32 (PcdFlashNvStorageFtwWorkingSize);

  SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageFtwSpareBase64);
  if (SpareAreaAddress == 0) {
    SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  }
  SpareAreaLength = (UINTN) PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  ASSERT ((WorkSpaceAddress != 0) && (SpareAreaAddress != 0));

//DEBUG((EFI_D_INFO, "FtwWorking(%X,%X), FtwSpare(%X,%X)\n", (UINT32)WorkSpaceAddress, WorkSpaceLength, (UINT32)SpareAreaAddress, SpareAreaLength));
  
  FtwWorkingBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) (UINTN) WorkSpaceAddress;
  if (IsValidWorkSpace (FtwWorkingBlockHeader, WorkSpaceLength)) {
    Status = FtwGetLastWriteHeader (
               FtwWorkingBlockHeader,
               WorkSpaceLength,
               &FtwLastWriteHeader
               );
    if (!EFI_ERROR (Status)) {
      Status = FtwGetLastWriteRecord (
                 FtwLastWriteHeader,
                 &FtwLastWriteRecord
                 );
      if(EFI_ERROR(Status)){				
        DEBUG((EFI_D_ERROR, "FtwGetLastWriteRecord:%r\n", Status));
      }		
    } else {
      DEBUG((EFI_D_ERROR, "FtwGetLastWriteHeader:%r\n", Status));
    }

    if (!EFI_ERROR (Status)) {
      ASSERT (FtwLastWriteRecord != NULL);
//-   DEBUG((EFI_D_INFO, "SpareC:%d, DestC:%d\n", !FtwLastWriteRecord->SpareComplete, !FtwLastWriteRecord->DestinationComplete));       
      if ((FtwLastWriteRecord->SpareComplete == FTW_VALID_STATE) && (FtwLastWriteRecord->DestinationComplete != FTW_VALID_STATE)) {
        //
        // If FTW last write was still in progress with SpareComplete set and DestinationComplete not set.
        // It means the target buffer has been backed up in spare block, then target block has been erased,
        // but the target buffer has not been writen in target block from spare block, we need to build
        // FAULT_TOLERANT_WRITE_LAST_WRITE_DATA GUID hob to hold the FTW last write data.
        //
        FtwLastWrite.TargetAddress = FtwLastWriteRecord->FvBaseAddress;
        FtwLastWrite.SpareAddress  = SpareAreaAddress;
        FtwLastWrite.Length        = SpareAreaLength;
        DEBUG ((
          EFI_D_INFO,
          "FtwPei last write data: TargetAddress - 0x%x SpareAddress - 0x%x Length - 0x%x\n",
          (UINTN) FtwLastWrite.TargetAddress,
          (UINTN) FtwLastWrite.SpareAddress,
          (UINTN) FtwLastWrite.Length));
        BuildGuidDataHob (&gEdkiiFaultTolerantWriteGuid, (VOID *) &FtwLastWrite, sizeof (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA));
      }
    }
  } else {
    DEBUG((EFI_D_ERROR, "InvalidWorkSpace\n"));
  }

  return EFI_SUCCESS;
}




