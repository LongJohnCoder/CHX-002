/*++

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              

Module Name:

  EdkIIGluePeiFirmwarePerformanceLib.c
  
Abstract:

  PEI Library for FPDT performance logging. 

--*/

#include "EdkIIGluePeim.h"
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

//
// MAX perfomance HOB Entries
//
#define MAX_FIRMWARE_PERFORMANCE_ENTRIES 80

EFI_STATUS
PeiPerfMeasureEx (
  IN VOID    *FileHeader,
  IN UINT16  *Token,
  IN BOOLEAN EntryExit,
  IN UINT64  TimeStamp,
  IN UINT16  Identifier
  )
/*++

Routine Description:

  Log an extended timestamp value into pre-allocated hob.

Arguments:

  FileHeader  - Pointer to the file header

  Token       - Pointer to Token Name
  
  EntryExit   - Indicates start or stop measurement

  Timestamp   - The TSC value

  Identifier  - Identifier of the record  

Returns:

  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records
  
  EFI_SUCCESS          - Successfully updated the record in hob

--*/
{
  EFI_STATUS                    Status;
  EFI_PEI_PPI_DESCRIPTOR        *PerfHobDescriptor;
  PEI_FIRMWARE_PERFORMANCE_HOB  *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC            *PeiGuidRec;
  EFI_PEI_SERVICES              **PeiServices;
  UINTN                         BufferSize;
  EFI_PEI_HOB_POINTERS          Hob;

  PeiServices = GetPeiServicesTablePointer ();
  //
  // Locate the Pei Performance Log Hob.
  //
  Status = (*PeiServices)->LocatePpi (
                            PeiServices,
                            &gPeiFirmwarePerformanceGuid,
                            0,
                            &PerfHobDescriptor,
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    BufferSize = (UINT16) (sizeof (PEI_FIRMWARE_PERFORMANCE_HOB) + 
                            ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * sizeof (PEI_GUID_EVENT_REC)) +
                            sizeof(EFI_PEI_PPI_DESCRIPTOR)
                           );

    Hob.Raw = BuildGuidHob (
                &gPeiFirmwarePerformanceGuid,
                BufferSize
                );

    if (Hob.Raw == NULL) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FirmwarePerformanceHob  = (VOID *) (Hob.Raw);
    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;

    PerfHobDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)((UINT8 *)(FirmwarePerformanceHob+1) +
                                                     (sizeof (PEI_GUID_EVENT_REC) *
                                                       (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1))
                                                  );
    PerfHobDescriptor->Flags  = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PerfHobDescriptor->Guid   = &gPeiFirmwarePerformanceGuid;
    PerfHobDescriptor->Ppi    = NULL;

    (*PeiServices)->InstallPpi (PeiServices, PerfHobDescriptor);
    ASSERT_EFI_ERROR (Status);
  }

  FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(((UINT8 *)(PerfHobDescriptor)) -
                                                               ((sizeof (PEI_GUID_EVENT_REC) *
                                                                  (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1))
                                                               + sizeof (PEI_FIRMWARE_PERFORMANCE_HOB)
                                                               )
                                                           );

  //
  // return if pre-allocated performance hob has filled up
  //
  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  ((*PeiServices)->SetMem) (PeiGuidRec, sizeof (PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = ((EFI_FFS_FILE_HEADER *)FileHeader)->Name;
  }
  //
  // Record the time stamp nanosec value.
  //
  PeiGuidRec->Timestamp = GetTimeInNanoSec (TimeStamp);

  //
  // Copy the progress ID
  //
  PeiGuidRec->ProgressID = Identifier;

  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId ();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return EFI_SUCCESS;
}

EFI_STATUS
PeiPerfMeasure (
  IN VOID     *FileHeader,
  IN UINT16   *Token,
  IN BOOLEAN  EntryExit,
  IN UINT64   TimeStamp
  )
/*++

Routine Description:

  Log a timestamp value into pre-allocated buffer.
  Creates performance hob if not already installed

Arguments:

  FileHeader  - Pointer to the file header

  Token       - Pointer to Token Name

  EntryExit   - Indicates start or stop measurement

  TimeStamp   - The start time or the stop time

Returns:

  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

  EFI_UNSUPPORTED      - Unable to recognize used token
  
  EFI_SUCCESS          - Successfully updated the record in hob

--*/
{
  EFI_STATUS                    Status;
  PEI_FIRMWARE_PERFORMANCE_HOB  *FirmwarePerformanceHob;
  EFI_PEI_PPI_DESCRIPTOR        *PerfHobDescriptor;
  PEI_GUID_EVENT_REC            *PeiGuidRec;
  EFI_PEI_SERVICES              **PeiServices;
  UINTN                         BufferSize;
  EFI_PEI_HOB_POINTERS          Hob;

  PeiServices = GetPeiServicesTablePointer ();
  //
  // Locate the Pei Performance Log Hob.
  //
  Status = (*PeiServices)->LocatePpi (
                            PeiServices,
                            &gPeiFirmwarePerformanceGuid,
                            0,
                            &PerfHobDescriptor,
                            NULL
                            );

  if (EFI_ERROR (Status)) {

    BufferSize = (UINT16) (sizeof (PEI_FIRMWARE_PERFORMANCE_HOB) + 
                            ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * sizeof (PEI_GUID_EVENT_REC)) +
                            sizeof(EFI_PEI_PPI_DESCRIPTOR)
                           );

    Hob.Raw = BuildGuidHob (
                &gPeiFirmwarePerformanceGuid,
                BufferSize
                );

    if (Hob.Raw == NULL) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FirmwarePerformanceHob  = (VOID *) (Hob.Raw);
    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;

    PerfHobDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)((UINT8 *)(FirmwarePerformanceHob+1) +
                                                     (sizeof (PEI_GUID_EVENT_REC) *
                                                       (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1))
                                                  );
    PerfHobDescriptor->Flags  = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PerfHobDescriptor->Guid   = &gPeiFirmwarePerformanceGuid;
    PerfHobDescriptor->Ppi    = NULL;

    (*PeiServices)->InstallPpi (PeiServices, PerfHobDescriptor);
    ASSERT_EFI_ERROR (Status);
  }

  FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(((UINT8 *)(PerfHobDescriptor)) -
                                                             ((sizeof (PEI_GUID_EVENT_REC) *
                                                               (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1))
                                                             + sizeof (PEI_FIRMWARE_PERFORMANCE_HOB))
                                                           );

  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return EFI_BUFFER_TOO_SMALL;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  ((*PeiServices)->SetMem) (PeiGuidRec, sizeof (PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = ((EFI_FFS_FILE_HEADER *)FileHeader)->Name;
  }
  //
  // Record the time stamp nanosec value.
  //
  PeiGuidRec->Timestamp = GetTimeInNanoSec (TimeStamp);

  //
  // Record the Progress Id
  // Tokens are used by PEI core to log various phases of PEI
  //
  if (!StrCmp (Token, L"PEIM")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = 1;
    } else {
      PeiGuidRec->ProgressID = 2;
    }
  } else if (!StrCmp (Token, L"PreMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = 0x12;
    } else {
      PeiGuidRec->ProgressID = 013;
    }
  } else if (!StrCmp (Token, L"DisMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = 0x14;
    } else {
      PeiGuidRec->ProgressID = 015;
    }
  } else if (!StrCmp (Token, L"PostMem")) {
    if (!EntryExit) {
      PeiGuidRec->ProgressID = 0x16;
    } else {
      PeiGuidRec->ProgressID = 017;
    }
  } else {
    return EFI_UNSUPPORTED;
  }
  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId ();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return EFI_SUCCESS;
}

EFI_STATUS
StartMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  Start measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to measure
  Token      - Token to measure
  Module     - Module to measure
  Timestamp  - Ticker as start tick

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasure (Handle, Token, FALSE, TimeStamp);
  return Status;
}

EFI_STATUS
EndMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  End measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasure (Handle, Token, TRUE, TimeStamp);
  return Status;
}

EFI_STATUS
StartMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  Start extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasureEx (Handle, Token, FALSE, TimeStamp, Identifier);
  return Status;
}

EFI_STATUS
EndMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  End extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record

Returns:

  EFI_SUCCESS - Located hob successfully, and buffer is updated with new record
  EFI_UNSUPPORTED - Failure in update
  EFI_BUFFER_TOO_SMALL - Allocate buffer is not enough to hold new records

--*/
{
  EFI_STATUS Status;

  Status = PeiPerfMeasureEx (Handle, Token, TRUE, TimeStamp, Identifier);
  return Status;
}