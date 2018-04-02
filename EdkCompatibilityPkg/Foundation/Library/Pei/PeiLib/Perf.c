/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Perf.c

Abstract:

  Support for FPDT performance structures. 

--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "PeiHob.h"
#include "CpuIA32.h"
#include "EfiCommonLib.h"

#include EFI_GUID_DEFINITION (PeiPerformanceHob)

#define MSR_PLATFORM_INFO             0xce
#define MAX_NON_TURBO_RATIO_OFFSET    8
#define MAX_NON_TURBO_RATIO_MASK      0xff
#define LOCAL_APIC_BASE               0xfee00000
#define APIC_ID_REGISTER              0x20
#define MSR_EXT_XAPIC_LOGICAL_APIC_ID 0x802
#define MSR_XAPIC_BASE                0x1b
#define MSR_XAPIC_BASE_MASK           0x0c00

#define MAX_FIRMWARE_PERFORMANCE_ENTRIES 80

//
// Prototype functions
//  
UINT64
IA32API
EfiReadMsr (
  IN UINT32     Index
  );

UINT64 GetTimeInNanoSec (
  UINT64 Ticker
  )
/*++

Routine Description:

  Internal routine to convert TSC value into nano second value

Arguments:

  Ticker - OPTIONAL. TSC value supplied by caller function

Returns:

  UINT64 - returns calculated timer value

--*/
{
  UINT64 Tick, pi;
  UINT8  Ratio;

  if(Ticker != 0){
    Tick = Ticker;
  } else {
    Tick = EfiReadTsc();
  }

  pi = EfiReadMsr(MSR_PLATFORM_INFO);
  Ratio = (UINT8)( ((UINT32)(UINTN)RShiftU64(pi, MAX_NON_TURBO_RATIO_OFFSET)) & MAX_NON_TURBO_RATIO_MASK);

  return (UINT64)DivU64x32(MultU64x32(Tick, 10), (UINTN)(Ratio), NULL);
}


UINT32 GetApicId (
  VOID
  )
/*++

Routine Description:

  Internal routine to retrieve current APIC Id

Arguments:

  None

Returns:

  UINT32 - returns Apic Id value

--*/
{
  BOOLEAN x2ApicEnabled;
  UINT32  ApicId;

  x2ApicEnabled = (BOOLEAN)(((EfiReadMsr (MSR_XAPIC_BASE)) & (MSR_XAPIC_BASE_MASK)) == MSR_XAPIC_BASE_MASK);
  if (x2ApicEnabled) {
    ApicId = (UINT32) EfiReadMsr (MSR_EXT_XAPIC_LOGICAL_APIC_ID);
  } else {
    ApicId = (UINT8) (*(volatile UINT32 *) (UINTN) (LOCAL_APIC_BASE + APIC_ID_REGISTER) >> 24);
  }

  return ApicId;
}

VOID
PeiPerfMeasureEx (
  EFI_PEI_SERVICES       **PeiServices,
  IN UINT16              *Token,
  IN EFI_FFS_FILE_HEADER *FileHeader,
  IN UINT16              Identifier,
  IN BOOLEAN             EntryExit,
  IN UINT64              Value
  )
/*++

Routine Description:

  Log an extended timestamp value.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  Identifier  - Identifier of the record

  EntryExit   - Indicates start or stop measurement

  Value       - The TSC value

Returns:

  None

--*/
{
  EFI_STATUS                   Status;
  EFI_PEI_PPI_DESCRIPTOR       *PerfHobDescriptor;
  PEI_FIRMWARE_PERFORMANCE_HOB *FirmwarePerformanceHob;
  PEI_GUID_EVENT_REC           *PeiGuidRec;
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
  if (!EFI_ERROR(Status)) {
    FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(((UINT8 *)(PerfHobDescriptor)) -
                                                                 ((sizeof(PEI_GUID_EVENT_REC) * (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1))
                                                                   + sizeof(PEI_FIRMWARE_PERFORMANCE_HOB)
                                                                 )
                                                             );

    //
    // return if performance buffer has filled up
    //
    if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
      return;
    }

    PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
    (*PeiServices)->SetMem (PeiGuidRec, sizeof(PEI_GUID_EVENT_REC), 0);

    //
    // Get the GUID name
    //
    if (FileHeader != NULL) {
      PeiGuidRec->Guid = FileHeader->Name;
    }

    //
    // Record the time stamp nanosec value.
    //
    PeiGuidRec->Timestamp = GetTimeInNanoSec(Value);

    //
    // Copy the progress ID
    //
    PeiGuidRec->ProgressID = Identifier;

    //
    // Record the APIC Id
    //
    PeiGuidRec->ApicID = GetApicId();

    //
    // Increment the number of valid log entries.
    //
    FirmwarePerformanceHob->NumberOfEntries++;
  }

  return;
}

VOID
PeiPerfMeasure (
  EFI_PEI_SERVICES       **PeiServices,
  IN UINT16              *Token,
  IN EFI_FFS_FILE_HEADER *FileHeader,
  IN BOOLEAN             EntryExit,
  IN UINT64              Value
  )
/*++

Routine Description:

  Log a timestamp count.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  EntryExit   - Indicates start or stop measurement

  Value       - The start time or the stop time

Returns:

--*/
{
  EFI_STATUS                   Status;
  EFI_HOB_GUID_TYPE            *Hob;
  PEI_FIRMWARE_PERFORMANCE_HOB *FirmwarePerformanceHob;
  EFI_PEI_PPI_DESCRIPTOR       *PerfHobDescriptor;
  PEI_GUID_EVENT_REC           *PeiGuidRec;
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

  //
  // If the Performance Hob was not found, build and install one.
  //
  if (EFI_ERROR(Status)) {
    Status = PeiBuildHobGuid (
               PeiServices,
               &gPeiFirmwarePerformanceGuid,
               (sizeof(PEI_FIRMWARE_PERFORMANCE_HOB) +
                 ((MAX_FIRMWARE_PERFORMANCE_ENTRIES-1) * 
                 sizeof(PEI_GUID_EVENT_REC)) +
                 sizeof(EFI_PEI_PPI_DESCRIPTOR)
               ),
               &Hob
               );
    ASSERT_PEI_ERROR(PeiServices, Status);

    FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(Hob+1);
    FirmwarePerformanceHob->NumberOfEntries = 0;
    FirmwarePerformanceHob->Reserved = 0;

    PerfHobDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)((UINT8 *)(FirmwarePerformanceHob+1) +
                                                     (sizeof(PEI_GUID_EVENT_REC) *
                                                       (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1)
                                                     )
                                                  );
    PerfHobDescriptor->Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PerfHobDescriptor->Guid = &gPeiFirmwarePerformanceGuid;
    PerfHobDescriptor->Ppi = NULL;

    (*PeiServices)->InstallPpi (PeiServices, PerfHobDescriptor);
    ASSERT_PEI_ERROR(PeiServices, Status);
  }

  FirmwarePerformanceHob = (PEI_FIRMWARE_PERFORMANCE_HOB *)(((UINT8 *)(PerfHobDescriptor)) -
                                                        ((sizeof(PEI_GUID_EVENT_REC) *
                                                           (MAX_FIRMWARE_PERFORMANCE_ENTRIES-1)
                                                         )
                                                         + sizeof(PEI_FIRMWARE_PERFORMANCE_HOB)
                                                      )
                                                     );

  if (FirmwarePerformanceHob->NumberOfEntries >= MAX_FIRMWARE_PERFORMANCE_ENTRIES) {
    return;
  }

  PeiGuidRec = &(FirmwarePerformanceHob->GuidEventRecord[FirmwarePerformanceHob->NumberOfEntries]);
  (*PeiServices)->SetMem (PeiGuidRec, sizeof(PEI_GUID_EVENT_REC), 0);

  //
  // If not NULL pointer, copy the file name
  //
  if (FileHeader != NULL) {
    PeiGuidRec->Guid = FileHeader->Name;
  }

  //
  // Record the time stamp nanosec value.
  //
  PeiGuidRec->Timestamp = GetTimeInNanoSec(Value);

  //
  // Record the Progress Id based upon token field
  //
  if (!EfiStrCmp (Token, L"PEIM")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = 1;
    } else {
      PeiGuidRec->ProgressID = 2;
    }   
  } else if (!EfiStrCmp (Token, L"PreMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = 0x12;
    } else {
      PeiGuidRec->ProgressID = 0x13;
    }   
  } else if (!EfiStrCmp (Token, L"DisMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = 0x14;
    } else {
      PeiGuidRec->ProgressID = 0x15;
    }   
  } else if (!EfiStrCmp (Token, L"PostMem")) {
    if(!EntryExit) {
      PeiGuidRec->ProgressID = 0x16;
    } else {
      PeiGuidRec->ProgressID = 0x17;
    }   
  } else {
    return ;
  }
  //
  // Record the APIC Id
  //
  PeiGuidRec->ApicID = GetApicId();

  //
  // Increment the number of valid log entries.
  //
  FirmwarePerformanceHob->NumberOfEntries++;

  return;
}
