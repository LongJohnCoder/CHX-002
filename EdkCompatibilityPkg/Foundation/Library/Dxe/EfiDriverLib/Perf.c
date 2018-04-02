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

  Support library for DXE Firmware Performance logging.  

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (FirmwarePerformance)
#include EFI_GUID_DEFINITION     (PeiPerformanceHob)
#include EFI_PROTOCOL_DEFINITION (LoadedImage)
#include EFI_PROTOCOL_DEFINITION (DriverBinding)
#include EFI_GUID_DEFINITION (Hob)

#include "EfiDriverLib.h"
#include "EfiHobLib.h"
#include "EfiImage.h"
#include "EfiCommonLib.h"
#include "CpuIA32.h"

EFI_GUID gNullGuid = EFI_NULL_GUID;
FIRMWARE_PERFORMANCE_PROTOCOL  *FirmwarePerformance;

static UINT32 *mPerformancePointer;
static UINT32 mPerformanceLength;
UINT16 BdsAttemptNumber = 0;

#define LOCAL_APIC_BASE                 0xfee00000
#define APIC_ID_REGISTER                0x20
#define MSR_EXT_XAPIC_LOGICAL_APIC_ID   0x802
#define MSR_XAPIC_BASE                  0x1B
#define MSR_XAPIC_BASE_MASK             0x0c00
#define MAX_NON_TURBO_RATIO_OFFSET      8
#define MAX_NON_TURBO_RATIO_MASK        0xff
#define PLATFORM_INFO_MSR               0xce

EFI_STATUS
GetTimerValue (
  OUT UINT64    *TimerValue
  );

EFI_STATUS
GetPeiFirmwarePerformanceHob (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
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
    GetTimerValue (&Tick);
  }

  pi = EfiReadMsr(PLATFORM_INFO_MSR);
  Ratio = (UINT8)( ((UINT32)(UINTN)RShiftU64(pi, MAX_NON_TURBO_RATIO_OFFSET)) & MAX_NON_TURBO_RATIO_MASK);

  return (UINT64)DivU64x32((UINT64)MultU64x32(Tick, 10), (UINTN)(Ratio), NULL);
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
  UINT32 ApicId;

  x2ApicEnabled = (BOOLEAN)(((EfiReadMsr (MSR_XAPIC_BASE)) & (MSR_XAPIC_BASE_MASK)) == MSR_XAPIC_BASE_MASK);
  if (x2ApicEnabled) {
    ApicId = (UINT32) EfiReadMsr (MSR_EXT_XAPIC_LOGICAL_APIC_ID);
  } else {
    ApicId = (UINT8) (*(volatile UINT32 *) (UINTN) (LOCAL_APIC_BASE + APIC_ID_REGISTER) >> 24);
  }

  return ApicId;
}

STATIC
VOID
GetShortPdbFileName (
  CHAR8 *PdbFileName,
  CHAR8 *GaugeString
  )
/*++

Routine Description:

  Shotens PDB path name

Arguments:

  PdbFileName - PdbFileName

Returns:
  GaugeString - GaugeString

--*/
{
  UINTN Index;
  UINTN Index1;
  UINTN StartIndex;
  UINTN EndIndex;

  if (PdbFileName == NULL) {
    EfiAsciiStrCpy (GaugeString, " ");
  } else {
    StartIndex = 0;
    for (EndIndex = 0; PdbFileName[EndIndex] != 0; EndIndex++)
      ;

    for (Index = 0; PdbFileName[Index] != 0; Index++) {
      if (PdbFileName[Index] == '\\') {
        StartIndex = Index + 1;
      }

      if (PdbFileName[Index] == '.') {
        EndIndex = Index;
      }
    }

    Index1 = 0;
    for (Index = StartIndex; Index < EndIndex; Index++) {
      GaugeString[Index1] = PdbFileName[Index];
      Index1++;
      if (Index1 == STRING_EVENT_RECORD_NAME_LENGTH - 1) {
        break;
      }
    }

    GaugeString[Index1] = 0;
  }

  return ;
}

STATIC
CHAR8 *
GetPdbPath (
  VOID *ImageBase
  )
/*++

Routine Description:

  Locate PDB path name in PE image

Arguments:

  ImageBase - base of PE to search

Returns:

  Pointer into image at offset of PDB file name if PDB file name is found,
  Otherwise a pointer to an empty string.

--*/
{
  CHAR8                           *PdbPath;
  UINT32                          DirCount;
  EFI_IMAGE_DOS_HEADER            *DosHdr;
  EFI_IMAGE_NT_HEADERS            *NtHdr;
  UINT16                          Magic;
  EFI_IMAGE_OPTIONAL_HEADER32     *OptionalHdr32;
  EFI_IMAGE_OPTIONAL_HEADER64     *OptionalHdr64;
  EFI_IMAGE_DATA_DIRECTORY        *DirectoryEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *DebugEntry;
  VOID                            *CodeViewEntryPointer;

  CodeViewEntryPointer  = NULL;
  PdbPath               = NULL;
  DosHdr                = ImageBase;
  if (DosHdr && DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    NtHdr           = (EFI_IMAGE_NT_HEADERS *) ((UINT8 *) DosHdr + DosHdr->e_lfanew);
    //
    // NOTE: We use Machine to identify PE32/PE32+, instead of Magic.
    //       It is for backward-compatibility consideration, because
    //       some system will generate PE32+ image with PE32 Magic.
    //
    if (NtHdr->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    } else if (NtHdr->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else if (NtHdr->FileHeader.Machine == EFI_IMAGE_MACHINE_X64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else {
      Magic = NtHdr->OptionalHeader.Magic;
    }
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      OptionalHdr32 = (VOID *) &NtHdr->OptionalHeader;
      DirectoryEntry  = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionalHdr32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    } else {
      OptionalHdr64 = (VOID *) &NtHdr->OptionalHeader;
      DirectoryEntry  = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionalHdr64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    }
    
    if (DirectoryEntry->VirtualAddress != 0) {
      for (DirCount = 0;
           (DirCount < DirectoryEntry->Size / sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)) && CodeViewEntryPointer == NULL;
           DirCount++
          ) {
        DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (DirectoryEntry->VirtualAddress + (UINTN) ImageBase + DirCount * sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY));
        if (DebugEntry->Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          CodeViewEntryPointer = (VOID *) ((UINTN) DebugEntry->RVA + (UINTN) ImageBase);
          switch (*(UINT32 *) CodeViewEntryPointer) {
          case CODEVIEW_SIGNATURE_NB10:
            PdbPath = (CHAR8 *) CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
            break;

          case CODEVIEW_SIGNATURE_RSDS:
            PdbPath = (CHAR8 *) CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
            break;

          default:
            break;
          }
        }
      }
    }
  }

  return PdbPath;
}

STATIC
VOID
GetNameFromHandle (
  IN  EFI_HANDLE Handle,
  OUT CHAR8      *GaugeString
  )
/*++

Routine Description:

  retrieves PDB path name from Handle

Arguments:

  Handle - Handle of image

Returns:

  Pointer to PDB Filename
--*/
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *Image;
  CHAR8                       *PdbFileName;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;

  EfiAsciiStrCpy (GaugeString, " ");

  //
  // Get handle name from image protocol
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**)&Image
                  );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Get handle name from image protocol
    //
    Status = gBS->HandleProtocol (
                    DriverBinding->ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID**)&Image
                    );
  }

  PdbFileName = GetPdbPath (Image->ImageBase);

  if (PdbFileName != NULL) {
    GetShortPdbFileName (PdbFileName, GaugeString);
  }

  return ;
}

EFI_GUID *
GetGuidFromHandle (
  EFI_HANDLE Handle
  )
/*++

Routine Description:

  retrieves GUID name from Handle

Arguments:

  Handle - Handle of image

Returns:

  Pointer to GUID name
--*/
{
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *pImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;

  if (Handle!=NULL){
    Status = gBS->HandleProtocol(Handle,
                        &gEfiLoadedImageProtocolGuid,
                        &pImage
                        );
    if (!EFI_ERROR(Status)){
      if (pImage->FilePath->Type==MEDIA_DEVICE_PATH  && pImage->FilePath->SubType==MEDIA_FV_FILEPATH_DP) {
        //
        // Determine GUID associated with module logging performance
        //
        FvFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH*)pImage->FilePath;

        return &FvFilePath->NameGuid;
      }
    }
  }

  return NULL;
}

EFI_STATUS 
LocatePerformanceProtocol(
  VOID
  )
/*++

Routine Description:

  locates Performance protocol interface

Arguments:

  None

Returns:
  EFI_STATUS

--*/
{
  EFI_STATUS Status;

  FirmwarePerformance = NULL;
  Status = gBS->LocateProtocol (&gFirmwarePerformanceProtocolGuid, NULL, (VOID **) &FirmwarePerformance);

  return Status;
}

EFI_STATUS
EFIAPI
InsertMeasurement (
  IN FIRMWARE_PERFORMANCE_PROTOCOL *This,
  IN EFI_HANDLE                    Handle,
  IN UINT16                        RecordType,
  IN UINT64                        Ticker,
  IN UINT16                        Identifier OPTIONAL
  )
/*++

Routine Description:

  Logs performance data according to Record Type into pre-allocated buffer

Arguments:

  This       - Calling context
  Handle     - Handle of gauge data
  RecordType - Type of FPDT record
  Ticker     - Set event's Tick. If 0, Tick is current timer.
  Identifier - Identifier of event records and other types of records

Returns:

  EFI_SUCCESS     - Successfully create and initialized a guage data node.
  EFI_OUT_OF_RESOURCES  - No enough resource to create a guage data node.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  HARDWARE_BOOT_REC        *HardwareBootRec;
  STRING_EVENT_REC         *StringEvent;
  GUID_EVENT_REC           *GuidEvent;
  BDS_ATTEMPT_REC          *BdsAttemptRec;
  EFI_GUID                 *GuidName; 

  UINT8 PdbFileName[STRING_EVENT_RECORD_NAME_LENGTH] = {0};
  UINT8 NullFileName[STRING_EVENT_RECORD_NAME_LENGTH] = {0};
  DevicePath = NULL;

  // 
  // buffer overflow check
  //
  if (mPerformanceLength + sizeof(STRING_EVENT_REC) > FIRMWARE_MAX_BUFFER) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  switch (RecordType) {
    case HARDWARE_BOOT_TYPE:
      //
      // Hardware Boot Record Table
      //
      HardwareBootRec = (HARDWARE_BOOT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);

      HardwareBootRec->RecType      = RecordType;
      HardwareBootRec->RecLength    = sizeof(HARDWARE_BOOT_REC);
      HardwareBootRec->Revision     = RECORD_REVISION_1;
      HardwareBootRec->HardwareBoot = GetTimeInNanoSec(Ticker);

      mPerformanceLength            += sizeof(HARDWARE_BOOT_REC);
      break;

    case GUID_EVENT_REC_TYPE:
    case STRING_EVENT_REC_TYPE:
      //
      // Determine Pdb FileName associated with module logging performance
      //
      if (Handle != NULL){
        GetNameFromHandle (Handle, PdbFileName);
      }

      GuidName = GetGuidFromHandle(Handle);
      if (EfiCompareMem (PdbFileName,NullFileName, STRING_EVENT_RECORD_NAME_LENGTH)){
        //
        // String Event Record
        //
        StringEvent = (STRING_EVENT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
        StringEvent->RecType      = STRING_EVENT_REC_TYPE;
        StringEvent->RecLength    = sizeof(STRING_EVENT_REC);
        StringEvent->Revision     = RECORD_REVISION_1;
        StringEvent->ProgressID   = Identifier;
        StringEvent->ApicID       = GetApicId();
        StringEvent->Timestamp    = GetTimeInNanoSec(Ticker);
        if (GuidName != NULL) {
          gBS->CopyMem(&(StringEvent->Guid),GuidName,sizeof(EFI_GUID));
        }

        (gBS->CopyMem) (StringEvent->NameString, PdbFileName, STRING_EVENT_RECORD_NAME_LENGTH);

        mPerformanceLength         += sizeof(STRING_EVENT_REC);

      } else {
        //
        // GUID Event Record
        //
        GuidEvent = (GUID_EVENT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
        GuidEvent->RecType         = GUID_EVENT_REC_TYPE;
        GuidEvent->RecLength       = sizeof(GUID_EVENT_REC);
        GuidEvent->Revision        = RECORD_REVISION_1;
        GuidEvent->ProgressID      = Identifier;
        GuidEvent->ApicID          = GetApicId();
        GuidEvent->Timestamp       = GetTimeInNanoSec(Ticker);
        if (GuidName != NULL) {
          gBS->CopyMem(&(GuidEvent->Guid),GuidName,sizeof(EFI_GUID));
        }

        mPerformanceLength         += sizeof(GUID_EVENT_REC);
      }
      break;

    case BDS_ATTEMPT_EVENT_REC_TYPE:
      //
      // BDS Boot Attempt Record
      //
      DevicePath = EfiDevicePathFromHandle (Handle);

      BdsAttemptRec = (BDS_ATTEMPT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
      BdsAttemptRec->RecType          = BDS_ATTEMPT_EVENT_REC_TYPE;
      BdsAttemptRec->RecLength        = sizeof(BDS_ATTEMPT_REC);
      BdsAttemptRec->Revision         = RECORD_REVISION_1;
      BdsAttemptRec->ApicID           = GetApicId();
      BdsAttemptRec->BdsAttemptNo     = BdsAttemptNumber + 1;
      BdsAttemptRec->Timestamp        = GetTimeInNanoSec(Ticker);
      BdsAttemptRec->UEFIBootVar      = UefiBootVariable + (UINT64)Identifier;
      BdsAttemptRec->DevicePathString = '0';
      mPerformanceLength              += sizeof(BDS_ATTEMPT_REC);
      break;

    default:
      //
      // Record is undefined, return EFI_ABORTED
      //
      return EFI_ABORTED;
      break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetPerfBufferAddr (
  IN  FIRMWARE_PERFORMANCE_PROTOCOL *This,
  OUT UINT32                        *PerformanceBuffer
  )
{
  EFI_STATUS Status;

  *PerformanceBuffer = 0;

  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  if (mPerformancePointer == NULL) {
    return EFI_NOT_FOUND;
  }

  *PerformanceBuffer = (UINT32)((UINTN)mPerformancePointer);

  return Status;
}


UINT32
GetPerfBufferLength (
  IN FIRMWARE_PERFORMANCE_PROTOCOL   *This
  )
{
  EFI_STATUS Status;

  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return 0;
  }

  return mPerformanceLength;
}

FIRMWARE_PERFORMANCE_PROTOCOL FirmwarePerformanceProtocol = {
  InsertMeasurement,
  GetPerfBufferAddr,
  GetPerfBufferLength
};

//
// Driver entry point
//
EFI_STATUS
InitializePerformanceInfrastructure (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable,
  IN UINT64           Ticker
  )
/*++

Routine Description:

  Install gFirmwarePerformanceProtocolGuid protocol and transfer PEI performance to gauge data nodes.

Arguments:

  ImageHandle - Standard driver entry point parameter
  SystemTable - Standard driver entry point parameter
  Ticker      - End tick for PEI performance

Returns:

  EFI_OUT_OF_RESOURCES - No enough buffer to allocate
  EFI_SUCCESS - Protocol installed.

--*/
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;
  //
  //A buffer of MAX size
  //
  mPerformancePointer = EfiLibAllocateZeroPool (FIRMWARE_MAX_BUFFER);
  if (mPerformancePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPerformanceLength = 0;
  //
  // Install the protocol interfaces
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gFirmwarePerformanceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FirmwarePerformanceProtocol
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Get PEI performance hob and convert into FPDT structure
    //
    GetPeiFirmwarePerformanceHob (ImageHandle, SystemTable);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
StartMeasure (
  EFI_HANDLE Handle,
  IN UINT16  *Token,
  IN UINT16  *Host,
  IN UINT64  Ticker
  )
/*++

Routine Description:

  Start measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle  - Handle to measure
  Token   - Token to measure
  Host    - Host to measure
  Ticker  - Ticker as start tick

Returns:

  Status code.

--*/
{
  EFI_STATUS Status;
  UINT16     RecordType;
  UINT16     Identifier;

  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return Status;
  }

  if (!EfiStrCmp (Token, START_IMAGE_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_START_ID;
  } else if (!EfiStrCmp (Token, LOAD_IMAGE_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_LOADIMAGE_START_ID;
  } else if (!EfiStrCmp (Token, DRIVERBINDING_START_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_DRIVERBINDING_START_ID;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
    Identifier = 0;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, Ticker, Identifier);

  return Status;
}

EFI_STATUS
EndMeasure (
  EFI_HANDLE Handle,
  IN UINT16  *Token,
  IN UINT16  *Host,
  IN UINT64  Ticker
  )
/*++

Routine Description:

  End measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle  - Handle to stop
  Token   - Token to stop
  Host    - Host to stop
  Ticker  - Ticker as end tick

Returns:

  Status code.

--*/
{
  EFI_STATUS Status;
  UINT16     RecordType;
  UINT16     Identifier;

  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return Status;
  }

  if (!EfiStrCmp (Token, START_IMAGE_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_END_ID;
  } else if (!EfiStrCmp (Token, LOAD_IMAGE_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_LOADIMAGE_END_ID;
  } else if (!EfiStrCmp (Token, DRIVERBINDING_START_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
    Identifier = MODULE_DRIVERBINDING_END_ID;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
    Identifier = 0;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, Ticker, Identifier);

  return Status;
}

EFI_STATUS
StartMeasureEx (
  IN EFI_HANDLE Handle,
  IN UINT16     *Token,
  IN UINT16     *Host,
  IN UINT64     Ticker,
  IN UINT16     Identifier
  )
/*++

Routine Description:

  Start extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Host       - Host to stop
  Ticker     - Ticker as end tick
  Identifier - Identifier for a given record
Returns:

  Status code.

--*/
{
  EFI_STATUS Status;
  UINT16     RecordType; 
 
  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return Status;
  }

  if (!EfiStrCmp (Token, HARDWARE_BOOT_TOK)) {
    RecordType = HARDWARE_BOOT_TYPE;
  } else if (!EfiStrCmp (Token, EVENT_REC_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
  } else if (!EfiStrCmp (Token, BDS_ATTEMPT_TOK)) {
    RecordType = BDS_ATTEMPT_EVENT_REC_TYPE;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
  }
  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, Ticker, Identifier);

  return Status;
}

EFI_STATUS
EndMeasureEx (
  IN EFI_HANDLE Handle,
  IN UINT16     *Token,
  IN UINT16     *Host,
  IN UINT64     Ticker,
  IN UINT16     Identifier
  )
/*++

Routine Description:

  End extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Host       - Host to stop
  Ticker     - Ticker as end tick
  Identifier - Identifier for a given record
Returns:

  Status code.

--*/
{
  EFI_STATUS Status;
  UINT16     RecordType; 
 
  Status = LocatePerformanceProtocol();
  if(EFI_ERROR(Status)) {
    return Status;
  }

  if (!EfiStrCmp (Token, HARDWARE_BOOT_TOK)) {
    RecordType = HARDWARE_BOOT_TYPE;
  } else if (!EfiStrCmp (Token, EVENT_REC_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
  } else if (!EfiStrCmp (Token, BDS_ATTEMPT_TOK)) {
    RecordType = BDS_ATTEMPT_EVENT_REC_TYPE;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, Ticker, Identifier);

  return Status;
}

EFI_STATUS
GetPeiFirmwarePerformanceHob (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Transfer PEI performance data to pre-allocated memory into FPDT format.

Arguments:

  ImageHandle - Standard entry point parameter
  SystemTable - Standard entry point parameter

Returns:

  EFI_OUT_OF_RESOURCES - No enough resource to create data node.
  EFI_SUCCESS - Transfer done successfully.

--*/
{
  EFI_STATUS                   Status;
  UINT32                       Index;
  VOID                         *HobList;
  PEI_FIRMWARE_PERFORMANCE_HOB *PeiFirmwarePerformanceHob;
  PEI_GUID_EVENT_REC           *PeiGuidRec;
  GUID_EVENT_REC               *GuidEvent;

  //
  // Locate installed Performance HOB
  //
  PeiFirmwarePerformanceHob = NULL;

  EfiLibGetSystemConfigurationTable (&gEfiHobListGuid, &HobList);

  do {
    Status = GetNextGuidHob (&HobList, &gPeiFirmwarePerformanceGuid, (VOID **) &PeiFirmwarePerformanceHob, NULL);
    if (EFI_ERROR (Status) || (PeiFirmwarePerformanceHob == NULL)) {
      break;
    }

    for (Index = 0; Index < PeiFirmwarePerformanceHob->NumberOfEntries; Index++) {
      PeiGuidRec  = &(PeiFirmwarePerformanceHob->GuidEventRecord[Index]);
      //
      // GUID Event Records from PEI phase
      //
      GuidEvent = (GUID_EVENT_REC*)((UINT8*)mPerformancePointer + mPerformanceLength);
      GuidEvent->RecType      = GUID_EVENT_REC_TYPE;
      GuidEvent->RecLength    = sizeof(GUID_EVENT_REC);
      GuidEvent->Revision     = RECORD_REVISION_1;
      GuidEvent->ProgressID   = PeiGuidRec->ProgressID;
      GuidEvent->ApicID       = PeiGuidRec->ApicID;
      GuidEvent->Timestamp    = PeiGuidRec->Timestamp;
      GuidEvent->Guid         = PeiGuidRec->Guid;

      mPerformanceLength += sizeof(GUID_EVENT_REC);
    }
  } while (!EFI_ERROR (Status));

  return Status;
}