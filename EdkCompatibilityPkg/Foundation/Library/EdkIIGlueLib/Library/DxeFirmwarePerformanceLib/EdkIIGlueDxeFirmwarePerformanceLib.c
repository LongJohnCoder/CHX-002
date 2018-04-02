/*++

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  EdkIIGlueDxeFirmwarePerformanceLib.c
  
Abstract: 

  DXE Library for FPDT performance logging. 

--*/

#include "Tiano.h"
#include "EdkIIGlueDxe.h"
#include "EfiImage.h"

#include EFI_PROTOCOL_DEFINITION (FirmwarePerformance)
#include EFI_GUID_DEFINITION (PeiPerformanceHob)
#include EFI_PROTOCOL_DEFINITION (LoadedImage)
#include EFI_GUID_DEFINITION (Hob)

static UINT32                 *mPerformancePointer;
static UINT32                 mPerformanceLength;
UINT16                        BdsAttemptNumber = 0;

FIRMWARE_PERFORMANCE_PROTOCOL *FirmwarePerformance;
EFI_GUID                      gNullGuid = EFI_NULL_GUID;

STATIC
VOID
GetShortPdbFileName (
  IN CHAR8  *PdbFileName,
  OUT CHAR8 *GaugeString
  )
/*++

Routine Description:

  Shotens PDB path name

Arguments:

  PdbFileName - PdbFileName to be shorten

Returns:
  
  Pointer to Shortened PdbFileName

--*/
{
  UINTN Index;
  UINTN Index1;
  UINTN StartIndex;
  UINTN EndIndex;

  if (PdbFileName == NULL) {
    AsciiStrCpy (GaugeString, " ");
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
  GaugeString - returns pointer to PDB Filename

Returns:

  None

--*/
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *Image;
  CHAR8                       *PdbFileName;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;

  AsciiStrCpy (GaugeString, " ");

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
                    (VOID **) &Image
                    );
  }

  PdbFileName = PeCoffLoaderGetPdbPointer (Image->ImageBase);

  if (PdbFileName != NULL) {
    GetShortPdbFileName (PdbFileName, GaugeString);
  }

  return ;
}

EFI_STATUS
LocatePerformanceProtocol (
  VOID
  )
/*++

Routine Description:

  locates Performance protocol interface

Arguments:

  None

Returns:

  EFI_SUCCESS - Located protocol successfully
  EFI_NOT_FOUND - Fail to locate protocol

--*/
{
  EFI_STATUS  Status;

  FirmwarePerformance = NULL;
  Status              = gBS->LocateProtocol (&gFirmwarePerformanceProtocolGuid, NULL, (VOID **) &FirmwarePerformance);

  return Status;
}

EFI_STATUS
EFIAPI
InsertMeasurement (
  IN  FIRMWARE_PERFORMANCE_PROTOCOL            *This,
  IN  EFI_HANDLE                               Handle,
  IN  UINT16                                   RecordType,
  IN  UINT64                                   Timestamp,
  IN  UINT16                                   Identifier OPTIONAL
  )
/*++

Routine Description:

  Logs performance data according to record type into pre-allocated buffer

Arguments:

  This       - Calling context
  Handle     - Handle of gauge data
  RecordType - Type of FPDT record
  Timestamp  - Timestamp of TSC timer when even happend.
  Identifier - Identifier of event records and other types of records

Returns:

  EFI_SUCCESS           - Successfully create and initialized a guage data node.
  EFI_OUT_OF_RESOURCES  - No enough resource to create a guage data node.
  EFI_ABORTED           - Type of record is not defined

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  HARDWARE_BOOT_REC         *HardwareBootRec;
  STRING_EVENT_REC          *StringEvent;
  GUID_EVENT_REC            *GuidEvent;
  BDS_ATTEMPT_REC           *BdsAttemptRec;
  EFI_GUID                  *GuidName;

  UINT8                     PdbFileName[STRING_EVENT_RECORD_NAME_LENGTH]  = { 0 };
  UINT8                     NullFileName[STRING_EVENT_RECORD_NAME_LENGTH] = { 0 };
  DevicePath = NULL;

  //
  // Check if pre-allocated buffer is full
  //
  if (mPerformanceLength + sizeof (STRING_EVENT_REC) > FIRMWARE_MAX_BUFFER) {
    return EFI_OUT_OF_RESOURCES;
  }

  switch (RecordType) {
  case HARDWARE_BOOT_TYPE:
    //
    // Hardware Boot Record Table
    //
    HardwareBootRec = (HARDWARE_BOOT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);

    HardwareBootRec->RecType      = RecordType;
    HardwareBootRec->RecLength    = sizeof (HARDWARE_BOOT_REC);
    HardwareBootRec->Revision     = RECORD_REVISION_1;
    HardwareBootRec->HardwareBoot = GetTimeInNanoSec (Timestamp);

    mPerformanceLength += sizeof (HARDWARE_BOOT_REC);
    break;

  case GUID_EVENT_REC_TYPE:
  case STRING_EVENT_REC_TYPE:
    //
    // Determine Pdb FileName associated with module logging performance
    //
    if (Handle != NULL) {
      GetNameFromHandle (Handle, PdbFileName);
    }

    GuidName = GetGuidFromHandle (Handle);
    if (CompareMem (PdbFileName,NullFileName, STRING_EVENT_RECORD_NAME_LENGTH)){
      //
      // String Event Record
      //
      StringEvent = (STRING_EVENT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
      StringEvent->RecType    = STRING_EVENT_REC_TYPE;
      StringEvent->RecLength  = sizeof (STRING_EVENT_REC);
      StringEvent->Revision   = RECORD_REVISION_1;
      StringEvent->ProgressID = Identifier;
      StringEvent->ApicID     = GetApicId ();
      StringEvent->Timestamp  = GetTimeInNanoSec (Timestamp);
      if (GuidName != NULL) {
        (gBS->CopyMem) (&(StringEvent->Guid), GuidName, sizeof (EFI_GUID));
      }

      (gBS->CopyMem) (StringEvent->NameString, PdbFileName, STRING_EVENT_RECORD_NAME_LENGTH);

      mPerformanceLength += sizeof (STRING_EVENT_REC);

    } else {
      //
      // GUID Event Record
      //
      GuidEvent = (GUID_EVENT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
      GuidEvent->RecType    = GUID_EVENT_REC_TYPE;
      GuidEvent->RecLength  = sizeof (GUID_EVENT_REC);
      GuidEvent->Revision   = RECORD_REVISION_1;
      GuidEvent->ProgressID = Identifier;
      GuidEvent->ApicID     = GetApicId ();
      GuidEvent->Timestamp  = GetTimeInNanoSec (Timestamp);
      if (GuidName != NULL) {
        (gBS->CopyMem) (&(GuidEvent->Guid), GuidName, sizeof (EFI_GUID));
      }

      mPerformanceLength += sizeof (GUID_EVENT_REC);
    }
    break;

  case BDS_ATTEMPT_EVENT_REC_TYPE:
    //
    // BDS Boot Attempt Record
    //
    DevicePath                      = DevicePathFromHandle (Handle);

    BdsAttemptRec = (BDS_ATTEMPT_REC*) ((UINT8*)mPerformancePointer + mPerformanceLength);
    BdsAttemptRec->RecType          = BDS_ATTEMPT_EVENT_REC_TYPE;
    BdsAttemptRec->RecLength        = sizeof (BDS_ATTEMPT_REC);
    BdsAttemptRec->Revision         = RECORD_REVISION_1;
    BdsAttemptRec->ApicID           = GetApicId ();
    BdsAttemptRec->BdsAttemptNo     = BdsAttemptNumber + 1;
    BdsAttemptRec->Timestamp        = GetTimeInNanoSec (Timestamp);
    BdsAttemptRec->UEFIBootVar      = UefiBootVariable + (UINT64) Identifier;
    BdsAttemptRec->DevicePathString = '0';
    mPerformanceLength += sizeof (BDS_ATTEMPT_REC);
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
/*++

Routine Description:

  Retrieves pointer to performance buffer

Arguments:

  This - Calling context
  PerformanceBuffer - returns pointer to pre-allocated memory buffer

Returns:

  EFI_SUCCESS - Located protocol successfully
  EFI_NOT_FOUND - Fail to locate protocol or invalide returned pointer

--*/
{
  EFI_STATUS  Status;

  *PerformanceBuffer  = 0;

  Status              = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (mPerformancePointer == NULL) {
    return EFI_NOT_FOUND;
  }

  *PerformanceBuffer = (UINT32) ((UINTN) mPerformancePointer);

  return Status;
}

UINT32
GetPerfBufferLength (
  IN FIRMWARE_PERFORMANCE_PROTOCOL *This
  )
/*++

Routine Description:

  Retrieves length of performance buffer

Arguments:

  This - Calling context

Returns:

  mPerformanceLength - length of pre-allocated memory buffer

--*/
{
  EFI_STATUS  Status;

  Status = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return 0;
  }

  return mPerformanceLength;
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

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
{
  EFI_STATUS  Status;
  UINT16      RecordType;
  UINT16      Identifier;

  Status = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!StrCmp (Token, START_IMAGE_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_START_ID;
  } else if (!StrCmp (Token, LOAD_IMAGE_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_LOADIMAGE_START_ID;
  } else if (!StrCmp (Token, DRIVERBINDING_START_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_DRIVERBINDING_START_ID;
  } else {
    RecordType  = PERFORMANCE_RECORD_TYPE_MAX;
    Identifier  = 0;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, TimeStamp, Identifier);

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

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
{
  EFI_STATUS  Status;
  UINT16      RecordType;
  UINT16      Identifier;

  Status = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!StrCmp (Token, START_IMAGE_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_END_ID;
  } else if (!StrCmp (Token, LOAD_IMAGE_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_LOADIMAGE_END_ID;
  } else if (!StrCmp (Token, DRIVERBINDING_START_TOK)) {
    RecordType  = STRING_EVENT_REC_TYPE;
    Identifier  = MODULE_DRIVERBINDING_END_ID;
  } else {
    RecordType  = PERFORMANCE_RECORD_TYPE_MAX;
    Identifier  = 0;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, TimeStamp, Identifier);

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

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
{
  EFI_STATUS  Status;
  UINT16      RecordType;

  Status = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!StrCmp (Token, HARDWARE_BOOT_TOK)) {
    RecordType = HARDWARE_BOOT_TYPE;
  } else if (!StrCmp (Token, EVENT_REC_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
  } else if (!StrCmp (Token, BDS_ATTEMPT_TOK)) {
    RecordType = BDS_ATTEMPT_EVENT_REC_TYPE;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, TimeStamp, Identifier);

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

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
{
  EFI_STATUS  Status;
  UINT16      RecordType;

  Status = LocatePerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!StrCmp (Token, HARDWARE_BOOT_TOK)) {
    RecordType = HARDWARE_BOOT_TYPE;
  } else if (!StrCmp (Token, EVENT_REC_TOK)) {
    RecordType = STRING_EVENT_REC_TYPE;
  } else if (!StrCmp (Token, BDS_ATTEMPT_TOK)) {
    RecordType = BDS_ATTEMPT_EVENT_REC_TYPE;
  } else {
    RecordType = PERFORMANCE_RECORD_TYPE_MAX;
  }

  Status = FirmwarePerformance->InsertMeasurement (FirmwarePerformance, Handle, RecordType, TimeStamp, Identifier);

  return Status;
}