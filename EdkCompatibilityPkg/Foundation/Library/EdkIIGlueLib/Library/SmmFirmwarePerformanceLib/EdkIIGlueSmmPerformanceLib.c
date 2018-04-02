/*++

Copyright (c) 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  EdkIIGlueSmmPerformanceLib.c

Abstract: 

  SMM Library for FPDT performance logging. 

--*/

#include "EdkIIGlueSmmPerformanceLibInternal.h"

UINT64                        *mStartTicker;
UINT32                        mFunctionNumber;

RUNTIME_MODULE_PERF_RECORD    *mRuntimeModulePerfRecord     = NULL;
RUNTIME_FUNCTION_PERF_RECORD  *mRuntimeFunctionPerfRecord   = NULL;

EFI_GUID                      gFpdtPerformanceProtocolGuid  = FPDT_PERFORMANCE_PROTOCOL_GUID;
EFI_GUID                      gEfiEventReadyToBootGuid      = EFI_EVENT_GROUP_READY_TO_BOOT;

EFI_STATUS
StartMeasureEx (
  IN EFI_HANDLE Handle,
  IN UINT16     *Token,
  IN UINT16     *Host,
  IN UINT64     Timestamp,
  IN UINT16     Identifier
  )
/*++

Routine Description:

  Start measurement according to token field. Inserts data into pre-allocatede memory buffer
  Updates FPDT table using protocol API

Arguments:

  Handle     - Handle to measure
  Token      - Token to measure
  Host       - Host to measure
  Timestamp  - Ticker as start tick
  Identifier - Identifier for a record

Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_BUFFER_TOO_SMALL - Allocated memory is not enough to store new function
  EFI_NOT_FOUND - Failure in update

--*/
{
  EFI_STATUS                Status;
  UINT32                    Index;
  UINT32                    RmptTableLength;

  FPDT_PERFORMANCE_PROTOCOL *FpdtProtocol;
  VOID                      *TableAddress;
  EFI_GUID                  *GuidName;
  RUNTIME_PERF_TABLE_HEADER *RuntimePerfTableHeader;

  RmptTableLength = 0;

  if (!StrCmp(Token, SMM_MODULE_TOK)) {
    //
    // Create RMPT table. The input Identifier specifies the max number of functions in this module.
    //
    mFunctionNumber = Identifier;
    RmptTableLength = sizeof (RUNTIME_PERF_TABLE_HEADER) + 
                      sizeof (RUNTIME_MODULE_PERF_RECORD) + 
                      (sizeof(RUNTIME_FUNCTION_PERF_RECORD) * mFunctionNumber);
    
    //
    // Allocate reseved memory for RMPT table
    //
    Status = (gBS->AllocatePool) (EfiReservedMemoryType, RmptTableLength, &TableAddress);
    ZeroMem (TableAddress, RmptTableLength);

    Status = (gBS->AllocatePool) (EfiReservedMemoryType, sizeof (UINT64) * mFunctionNumber, &mStartTicker);
    ZeroMem (mStartTicker, (sizeof (UINT64) * mFunctionNumber));

    RuntimePerfTableHeader = (RUNTIME_PERF_TABLE_HEADER *) TableAddress;
    mRuntimeModulePerfRecord = (RUNTIME_MODULE_PERF_RECORD *) ((UINT8 *) TableAddress + sizeof (RUNTIME_PERF_TABLE_HEADER));
    mRuntimeFunctionPerfRecord = (RUNTIME_FUNCTION_PERF_RECORD *) ((UINT8 *) mRuntimeModulePerfRecord + sizeof (RUNTIME_MODULE_PERF_RECORD));

    //
    // Fill RMPT table header
    //
    RuntimePerfTableHeader->Signature = RMPT_SIG;
    RuntimePerfTableHeader->Length    = RmptTableLength;

    GuidName                          = GetGuidFromHandle (Handle);
    if (GuidName != NULL) {
      (gBS->CopyMem) (&(RuntimePerfTableHeader->Guid), GuidName, sizeof (EFI_GUID));
    }
    //
    // Fill mRuntimeModulePerfRecord.
    //
    mRuntimeModulePerfRecord->RuntimeRecType  = RUNTIME_MODULE_REC_TYPE;
    mRuntimeModulePerfRecord->Revision        = RECORD_REVISION_1;
    mRuntimeModulePerfRecord->Reclength       = sizeof (RUNTIME_MODULE_PERF_RECORD);

    //
    // Call FPDT performance protocol to add RMPT table pointer record into FPDT table.
    // 
    Status = (gBS->LocateProtocol) (
                  &gFpdtPerformanceProtocolGuid,
                  NULL,
                  &FpdtProtocol
                  );
    ASSERT_EFI_ERROR (Status);

    Status = FpdtProtocol->UpdateRecord (
                            FpdtProtocol,
                            Handle,
                            RUNTIME_MODULE_TABLE_PTR_TYPE,
                            (UINT64) TableAddress,
                            0
                            );
    ASSERT_EFI_ERROR (Status);
  }

  if (!StrCmp (Token, SMM_FUNCTION_TOK)) {
    //
    // Add function record into RMPT table.
    // The input Identifier is the function ID.
    //
    //
    // Add function record into Record buffer.
    //
    for (Index = 0; Index < mFunctionNumber; Index++) {
      if (mRuntimeFunctionPerfRecord[Index].RuntimeRecType == 0) {
        //
        // Fucntion ID is not found in Record buffer.
        //
        break;
      }

      if (mRuntimeFunctionPerfRecord[Index].FunctionId == Identifier) {
        //
        // This function ID is found in Record buffer.
        //
        break;
      }
    }

    if (Index == mFunctionNumber) {
      //
      // Function record buffer is not enough to store new function.
      //
      return EFI_BUFFER_TOO_SMALL;
    }

    if (mRuntimeFunctionPerfRecord[Index].RuntimeRecType == 0) {
      //
      // New function ID is found. Need to add it into FunctionRecord buffer.
      //
      mRuntimeFunctionPerfRecord[Index].RuntimeRecType  = RUNTIME_FUNCTION_REC_TYPE;
      mRuntimeFunctionPerfRecord[Index].Revision        = RECORD_REVISION_1;
      mRuntimeFunctionPerfRecord[Index].Reclength       = sizeof (RUNTIME_FUNCTION_PERF_RECORD);
      mRuntimeFunctionPerfRecord[Index].FunctionId      = Identifier;
    }
    //
    // Record start tick.
    //
    mStartTicker[Index] = GetTimeInNanoSec (Timestamp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EndMeasureEx (
  IN EFI_HANDLE Handle,
  IN UINT16     *Token,
  IN UINT16     *Host,
  IN UINT64     Timestamp,
  IN UINT16     Identifier
  )
/*++

Routine Description:

  End measurement according to token field. Inserts data into pre-allocatede memory buffer

Arguments:

  Handle     - Handle to measure
  Token      - Token to measure
  Host       - Host to measure
  Timestamp  - Ticker as start tick
  Identifier - Identifier for a record

Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_BUFFER_TOO_SMALL - Allocated memory is not enough to store new function
  EFI_NOT_FOUND - Fucntion ID is not found in Record buffer.

--*/
{
  UINT32  Index;

  if (!StrCmp (Token, SMM_FUNCTION_TOK)) {
    //
    // Update function record into RMPT table.
    // The input Identifier is the function ID.
    //
    //
    // Update function record into Record buffer.
    //
    for (Index = 0; Index < mFunctionNumber; Index++) {
      if (mRuntimeFunctionPerfRecord[Index].RuntimeRecType == 0) {
        //
        // Fucntion ID is not found in Record buffer.
        //
        return EFI_NOT_FOUND;
      }

      if (mRuntimeFunctionPerfRecord[Index].FunctionId == Identifier) {
        //
        // This function ID has been added.
        //
        break;
      }
    }

    if (Index == mFunctionNumber) {
      //
      // Fucntion ID is not found in Record buffer.
      //
      return EFI_NOT_FOUND;
    }

    mRuntimeFunctionPerfRecord[Index].FunctionCallCount++;
    mRuntimeFunctionPerfRecord[Index].FunctionResidency += (GetTimeInNanoSec (Timestamp)) - mStartTicker[Index];
    //
    // Update Module Record.
    //
    mRuntimeModulePerfRecord->ModuleCallCount++;
    mRuntimeModulePerfRecord->ModuleResidency  += (GetTimeInNanoSec (Timestamp)) - mStartTicker[Index];
  }

  return EFI_SUCCESS;
}