/*++
=======================================================================
      NOTICE: Copyright (c) 2006 - 2012 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
=======================================================================
Module Name:

  PnpSmbios.c

Abstract:

Revision History:

--*/
#include <PnpSmbios.h>

SMBIOS_TABLE_ENTRY_POINT        *mSmbiosLegacyEntry = NULL;
SMBIOS_TABLE_ENTRY_POINT        *mSmbiosEfiEntry    = NULL;
UINT8                           *mSmbiosBufferPtr   = NULL;
UINT8                           *mSmbiosScratchPtr  = NULL;
LIST_ENTRY                      mPnp52HeadNode;
extern NV_MEDIA_ACCESS_PROTOCOL *pMediaAccessProtocol;

EFI_STATUS
InsertRecord(
  PNP_52_DATA_BUFFER *pDataBufferPtr
)
{
  EFI_STATUS          Status;
  PNP_52_RECORD       *Record;
  PNP_52_DATA_BUFFER  *pData;
  UINT16              Length;
  LIST_ENTRY          *Link;
  UINT16              StructDataSize;
  

  if (pDataBufferPtr == NULL)
    return EFI_INVALID_PARAMETER;

  if (pDataBufferPtr->Command == DeleteChanged) {
    Link = GetFirstNode (&mPnp52HeadNode);
    while(!IsNull (&mPnp52HeadNode, Link)) {
      Record = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
      if ((Record->pRecord->Command == AddChanged) &&
          (Record->pRecord->StructureHeader.Type == pDataBufferPtr->StructureHeader.Type) &&
          (Record->pRecord->StructureHeader.Handle == pDataBufferPtr->StructureHeader.Handle)) {
        RemoveEntryList (&Record->Link);
        gSmst->SmmFreePool(Record);
        return EFI_SUCCESS;
      }
      Link = GetNextNode(
               &mPnp52HeadNode,
               &Record->Link);
    }
  }

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (PNP_52_RECORD),
                    (VOID **)&Record
                    );
  if (EFI_ERROR (Status)) {
    return Status ;
  }

  ZeroMem (Record, sizeof (PNP_52_RECORD));

  InsertTailList(&mPnp52HeadNode, &Record->Link);

  Record->Signature = EFI_PNP_52_SIGNATURE;
  Record->header.Signature  = SMBIOS_REC_SIGNATURE;

  if(pDataBufferPtr->Command == ByteChanged ||
     pDataBufferPtr->Command == WordChanged ||
     pDataBufferPtr->Command == DoubleWordChanged){
    StructDataSize = 0;
  } else {
    StructDataSize = pDataBufferPtr->DataLength;
  }

  Length  = sizeof(PNP_52_DATA_BUFFER) - 1 + StructDataSize;
  Record->header.RecordLen = sizeof(SMBIOS_REC_HEADER) + Length;

  Status  = gSmst->SmmAllocatePool (
                     EfiRuntimeServicesData,
                     Length,
                     (VOID **)&pData
                     );
  if (EFI_ERROR (Status)) {
    return Status ;
  }

  CopyMem (pData, (UINT8 *) pDataBufferPtr, Length);

  Record->pRecord = pData;

  return Status;
}

EFI_STATUS
DeleteRecord(
  PNP_52_DATA_BUFFER *pDataBufferPtr
)
{
  LIST_ENTRY      *Link;
  PNP_52_RECORD   *Record = NULL;

  if (IsListEmpty (&mPnp52HeadNode)) {
    return EFI_UNSUPPORTED;
  } else {
    Link = GetFirstNode (&mPnp52HeadNode);
  }

  while(!IsNull (&mPnp52HeadNode, Link)) {
    Record = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
    if ((Record->pRecord->Command == pDataBufferPtr->Command) &&
        (Record->pRecord->FieldOffset == pDataBufferPtr->FieldOffset) &&
        (Record->pRecord->StructureHeader.Type == pDataBufferPtr->StructureHeader.Type) &&
        (Record->pRecord->StructureHeader.Handle == pDataBufferPtr->StructureHeader.Handle)) {
      RemoveEntryList (&Record->Link);
      gSmst->SmmFreePool(Record);
      break;
    }
    Link = GetNextNode(
             &mPnp52HeadNode,
             &Record->Link);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
Mem2RecordList (
  UINT8   *pBase,
  UINTN   size
)
{
  EFI_STATUS    Status;
  UINT8         *RecordPtr;
  UINT8         *RecordEndPtr;
  UINTN         Length;

  PNP_52_DATA_BUFFER *DataBufferInRec;

  RecordPtr = pBase;
  RecordEndPtr =  (UINT8 *)(pBase + size);

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
      break;

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr + sizeof (SMBIOS_REC_HEADER));
    Length  = sizeof (PNP_52_DATA_BUFFER) - 1;
    Length += DataBufferInRec->DataLength;

    Status  = InsertRecord (DataBufferInRec);
    if (EFI_ERROR (Status)) {
      return Status ;
    }
    RecordPtr = RecordPtr + Length + sizeof (SMBIOS_REC_HEADER);
  }

  return EFI_SUCCESS;
}

UINTN
RecordList2Mem (
  UINT8 *pBase
)
{
  LIST_ENTRY        *Link;
  PNP_52_RECORD     *Record;
  UINTN             Length = 0;

  if (IsListEmpty (&mPnp52HeadNode)) {
    return Length;
  } else {
    Link = GetFirstNode (&mPnp52HeadNode);
  }

  while(!IsNull (&mPnp52HeadNode, Link)) {
    Record  = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
    CopyMem (pBase, (UINT8 *)&Record->header, sizeof (SMBIOS_REC_HEADER));
    pBase  += sizeof(SMBIOS_REC_HEADER);
    CopyMem (pBase, Record->pRecord, (Record->header.RecordLen - sizeof (SMBIOS_REC_HEADER)));
    pBase  += (Record->header.RecordLen - sizeof (SMBIOS_REC_HEADER));
    Length += Record->header.RecordLen;
    Link    = GetNextNode(
                &mPnp52HeadNode,
                &Record->Link);
  }

  return Length;
}

EFI_STATUS
PnpGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
)
/*++

Routine Decription:

    Get table from configuration table by name

Arguments:

    TableGuid  - Table name to search

    Table        - Pointer to the table caller wants

Returns:

    EFI_NOT_FOUND - Not found the table

    EFI_SUCCESS - Found the table

--*/
{
  UINTN Index;

  *Table = NULL;
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid(TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}


VOID
PnpGetSmbiosEntry (
  VOID
)
/*++

Routine Description:

  Ensures that mSmbiosTableEntryPoint and mSmbiosTable are filled in.

Arguments:

Returns:

  none

--*/
{
  UINT8 *Ptr;

  if (mSmbiosLegacyEntry == NULL) {
    Ptr = (UINT8 *) (UINTN) 0xf0000;
    for ( ; Ptr < (UINT8 *) (UINTN) 0x100000; Ptr += 0x10) {
      if (*(UINT32 *) Ptr == SIGNATURE_32('_','S','M','_')) {
        mSmbiosLegacyEntry = (SMBIOS_TABLE_ENTRY_POINT *)Ptr;
      }
    }
  }

  //
  // Find the efi smbios table entry
  //
  if (mSmbiosEfiEntry == NULL) {
    PnpGetSystemConfigurationTable (
      &gEfiSmbiosTableGuid,
      (VOID **)&mSmbiosEfiEntry
    );
    ASSERT (mSmbiosEfiEntry != NULL);
  }

  return;
}

UINT16
PnpGetSmbiosInformation (
  IN VOID *ParameterBuffer
)
{
  PNP_PARAMS_50 *param = (PNP_PARAMS_50 *)ParameterBuffer;
  UINT32        DataAddress;
  VOID          *DataPtr = NULL;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry ();

  //
  // By default, we use the efi entry table
  //

  //
  // DmiBiosRevision
  //
  DataAddress = ((param->DmiBiosRevisionPtr >> 16) << 4) + (param->DmiBiosRevisionPtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  *(UINT8 *)DataPtr = mSmbiosEfiEntry->SmbiosBcdRevision;

  //
  // NumStructures
  //
  DataAddress = ((param->NumStructuresPtr >> 16) << 4) + (param->NumStructuresPtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  *(UINT16 *)DataPtr = mSmbiosEfiEntry->NumberOfSmbiosStructures;

  //
  // StructureSize
  //
  DataAddress = ((param->StructureSizePtr >> 16) << 4) + (param->StructureSizePtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  *(UINT16*)DataPtr = mSmbiosEfiEntry->MaxStructureSize;

  //
  // DmiStorageBase
  //
  DataAddress = ((param->DmiStorageBasePtr >> 16) << 4) + (param->DmiStorageBasePtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  *(UINT32 *)DataPtr = mSmbiosEfiEntry->TableAddress;

  //
  // DmiStorageSize
  //
  DataAddress = ((param->DmiStorageSizePtr >> 16) << 4) + (param->DmiStorageSizePtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  *(UINT16 *)DataPtr = mSmbiosEfiEntry->TableLength;

  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

UINTN
PnpGetSmbiosStructureSize (
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  UINT8 *SmbiosPtr;

  SmbiosPtr = StrucPtr->Raw + StrucPtr->Hdr->Length;

  //
  // Find two NULL string
  //
  for (; *(UINT16 *)SmbiosPtr != 0; SmbiosPtr ++);

  SmbiosPtr += 2;

  //
  // Return the correct size
  //
  return (UINTN)(SmbiosPtr - StrucPtr->Raw);
}

UINTN
PnpGetSmbiosStructuresSize (
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  UINTN                     TototalSize = 0;
  UINTN                     StrucSize   = 0;

  Smbios.Raw = StrucPtr->Raw;

  //
  // Scan the smbios table to the end and caculate the total size
  //
  while (Smbios.Hdr->Type != 127) {
    StrucSize = PnpGetSmbiosStructureSize(&Smbios);
    TototalSize += StrucSize;
    Smbios.Raw =  Smbios.Raw + StrucSize;
  }

  StrucSize = PnpGetSmbiosStructureSize(&Smbios);
  TototalSize += StrucSize;

  return TototalSize;
}


VOID
PnpGetSmbiosStructureByHanlde (
  IN UINT16 StrucHandle,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry ();

  //
  // By default, we use the efi entry table
  //
  Smbios.Raw = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress);
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosEfiEntry->TableLength;

  //
  // Scan the smbios table and find the matched smbios structure having the
  // Same handle number as given
  //
  while (Smbios.Raw < SmbiosEnd.Raw) {
    if (Smbios.Hdr->Handle == StrucHandle) {
      StrucPtr->Raw = Smbios.Raw;
      return;
    }
    Smbios.Raw = Smbios.Raw + PnpGetSmbiosStructureSize (&Smbios);
  }

  //
  // if no matched structure is found, return null
  //
  StrucPtr->Raw = NULL;
  return;
}


VOID
PnpGetSmbiosStructureByHandleWithUpdate (
  IN UINT16                   *StrucHandle,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  SMBIOS_STRUCTURE_POINTER  SmbiosNext;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry();

  //
  // By default, we use the efi entry table
  //
  Smbios.Raw = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress);
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosEfiEntry->TableLength;

  //
  // If the smbios handle is zero, return the fist smbios structue
  //
  if (*StrucHandle == 0) {
    SmbiosNext.Raw = Smbios.Raw + PnpGetSmbiosStructureSize (&Smbios);

    //
    // update the structure handle with the next sequential structure
    // handle or the end of list indicator 0xffff
    //
    if (SmbiosNext.Raw >= SmbiosEnd.Raw)
      *StrucHandle = END_HANDLE;
    else
      *StrucHandle = SmbiosNext.Hdr->Handle;
    StrucPtr->Raw = Smbios.Raw;
    return;
  }

  //
  // Scan the smbios table and find the matched smbios structure having
  // the same handle number as given
  //
  while (Smbios.Raw < SmbiosEnd.Raw) {
    SmbiosNext.Raw = Smbios.Raw + PnpGetSmbiosStructureSize (&Smbios);

    if (Smbios.Hdr->Handle == *StrucHandle) {
      //
      // update the structure handle with the next sequential structure
      // handle or the end of list indicator 0xffff
      //
      if (SmbiosNext.Raw >= SmbiosEnd.Raw)
        *StrucHandle = END_HANDLE;
      else
        *StrucHandle = SmbiosNext.Hdr->Handle;
      StrucPtr->Raw = Smbios.Raw;
      return;
    }

    Smbios.Raw = SmbiosNext.Raw;
  }

  //
  // if no matched structure is found, return null
  //
  *StrucHandle  = END_HANDLE;
  StrucPtr->Raw = NULL;
  return;
}

UINT16
PnpGetSmbiosStructure(
  IN VOID *ParameterBuffer
)
{
  PNP_PARAMS_51             *param = (PNP_PARAMS_51 *)ParameterBuffer;
  UINT32                    DataAddress;
  VOID                      *DataPtr = NULL;
  UINT16                    StrucHandle;
  SMBIOS_STRUCTURE_POINTER  StrucPtr;
  UINTN                     StrucSize;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry();

  //
  // Get the Smbios structure handle
  //
  DataAddress = ((param->StructurePtr >>16) <<4) + (param->StructurePtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;
  StrucHandle = *(UINT16 *)DataPtr;

  //
  // Find the SMBIOS structure pointer according to the given handle number
  //
  PnpGetSmbiosStructureByHandleWithUpdate (&StrucHandle, &StrucPtr);

  if (StrucPtr.Raw == NULL)
    return PNP_BIOS_DMI_RETURN_INVALID_HANDLE;

  //
  // Update the SMBIOS structure handle with new value
  //
  *(UINT16 *)DataPtr = StrucHandle;

  //
  // Get the pointer to SMBIOS structure data buffer
  //
  DataAddress = ((param->DmiStrucBufferPtr >> 16) << 4) + (param->DmiStrucBufferPtr & 0xFFFF);
  DataPtr = (VOID *)(UINTN) DataAddress;

  StrucSize = PnpGetSmbiosStructureSize (&StrucPtr);

  //
  // Copy the content into the structure buffer
  //
  CopyMem (DataPtr, StrucPtr.Raw, StrucSize);

  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

UINT16
PnpValidateParamsInDataBuffer (
  IN  PNP_52_DATA_BUFFER *DataBufferPtr
)
{
  UINT16 ErrorCode = PNP_BIOS_DMI_RETURN_SUCCESS;

  //
  // Commands > 6 are reserved for future, return bad parameter
  //
  if (DataBufferPtr->Command > BlockChanged)
    return PNP_BIOS_DMI_RETURN_BAD_PARAMETER;

  //
  // Scan all the allowed modified field, see if it is allowed
  //
  switch (DataBufferPtr->StructureHeader.Type) {
    case 0:
      //
      // Type 0, BIOS Information
      //
      switch (DataBufferPtr->FieldOffset) {
        case 0x04:
        case 0x05:
        case 0x08:
          if (DataBufferPtr->Command != StringChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        case 0x0a:
          if (DataBufferPtr->Command != DoubleWordChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
          if (DataBufferPtr->Command != ByteChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        default:
          ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        }
      break;

    case 1:
      //
      // Type 1, System Information
      //
      switch (DataBufferPtr->FieldOffset) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x19:
        case 0x1a:
          if (DataBufferPtr->Command != StringChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        case 0x08:
          if ((DataBufferPtr->Command != BlockChanged) ||(DataBufferPtr->DataLength != 0x10))
            if (DataBufferPtr->Command != DoubleWordChanged)
              ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        case 0x0c:
        case 0x10:
        case 0x14:
          if (DataBufferPtr->Command != DoubleWordChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        default:
          ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        }
      break;

    case 2:
      //
      // Type 2, Base Board Information
      //
      switch (DataBufferPtr->FieldOffset) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x0a:
          if (DataBufferPtr->Command != StringChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        default:
          ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        }
      break;

    case 3:
      //
      // Type 3, System enclosure
      //
      switch (DataBufferPtr->FieldOffset) {
        case 0x04:
        case 0x06:
        case 0x07:
        case 0x08:
          if (DataBufferPtr->Command != StringChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        case 0x05:
          if (DataBufferPtr->Command != ByteChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        default:
          ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        }
      break;

    case 4:
      //
      // Type 4, Processor Information
      //
      switch (DataBufferPtr->FieldOffset) {
        case 0x04:
        case 0x07:
          if (DataBufferPtr->Command != StringChanged)
            ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;

        default:
          ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
          break;
        }
      break;

    default:
      ErrorCode = PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
      break;
  }

  return ErrorCode;
}

UINT16
PnpUpdateFixedLengthDataInMem(
  IN PNP_52_DATA_BUFFER       *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  UINT8   Value8;
  UINT16  Value16;
  UINT32  Value32;

  if (DataBufferPtr->Command == ByteChanged) {
    Value8   = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
    Value8  &= DataBufferPtr->ChangeMask;
    Value8  |= DataBufferPtr->ChangeValue;
    *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset) = Value8;
  } else if (DataBufferPtr->Command == WordChanged) {
    Value16  = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
    Value16 &= DataBufferPtr->ChangeMask;
    Value16 |= DataBufferPtr->ChangeValue;
    *(UINT16 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset) = Value16;
  } else if (DataBufferPtr->Command == DoubleWordChanged) {
    Value32  = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
    Value32 &= DataBufferPtr->ChangeMask;
    Value32 |= DataBufferPtr->ChangeValue;
    *(UINT32 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset) = Value32;
  }

  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

UINTN
PnpStringDataLen (
  IN UINT8 *StringData
)
{
  UINT8 *StringPtr = StringData;

  for (; *StringPtr != 0; StringPtr++);

  return (UINTN)(StringPtr -StringData);
}

UINTN
PnpGetMaxSmbiosStructureSize (
  VOID
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINTN                     MaxSize   = 0;
  UINTN                     StrucSize = 0;

  Smbios.Raw    = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress);
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosEfiEntry->TableLength;

  //
  // Scan the smbios table to the end and caculate the total size
  //
  while (Smbios.Raw < SmbiosEnd.Raw) {
    StrucSize = PnpGetSmbiosStructureSize (&Smbios);
    if (StrucSize > MaxSize)
      MaxSize = StrucSize;
    Smbios.Raw = Smbios.Raw + StrucSize;
  }
  return MaxSize;
}

VOID
PnpUpdateSmbiosEntryInMem (
  VOID
)
{
  SMBIOS_STRUCTURE_POINTER  StrucTemp;
  UINT8                     CheckSum;
  UINTN                     Index;
  UINT8                     FsegValue8;
  UINT8                     EsegValue8;

  StrucTemp.Raw = (UINT8 *)(UINTN)mSmbiosEfiEntry->TableAddress;

  //
  // Update the EFI entry
  //

  //
  // Update some fields in entry
  //
  mSmbiosEfiEntry->TableLength = (UINT16)PnpGetSmbiosStructuresSize(&StrucTemp);
  mSmbiosEfiEntry->MaxStructureSize = (UINT16)PnpGetMaxSmbiosStructureSize();

  CheckSum = 0;
  mSmbiosEfiEntry->IntermediateChecksum = 0;
  for (Index = 0x10; Index < mSmbiosEfiEntry->EntryPointLength; Index++) {
    CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosEfiEntry))[Index]);
  }
  mSmbiosEfiEntry->IntermediateChecksum = (UINT8)(0 - CheckSum);

  CheckSum = 0;
  mSmbiosEfiEntry->EntryPointStructureChecksum = 0;
  for (Index = 0x0; Index < mSmbiosEfiEntry->EntryPointLength; Index++) {
    CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosEfiEntry))[Index]);
  }
  mSmbiosEfiEntry->EntryPointStructureChecksum = (UINT8)(0 - CheckSum);

  //
  // Enable E & F segment write
  //
  EnableMemAccess(&FsegValue8, &EsegValue8);

  //
  // Update the legacy entry by copy
  //
  if (mSmbiosLegacyEntry != NULL) {
    CopyMem (
      ((UINT8 *) (UINTN)mSmbiosLegacyEntry->TableAddress),
      ((UINT8 *) (UINTN)mSmbiosEfiEntry->TableAddress),
      mSmbiosEfiEntry->TableLength
    );
    mSmbiosLegacyEntry->TableLength              = mSmbiosEfiEntry->TableLength;
    mSmbiosLegacyEntry->MaxStructureSize         = mSmbiosEfiEntry->MaxStructureSize;
    mSmbiosLegacyEntry->NumberOfSmbiosStructures = mSmbiosEfiEntry->NumberOfSmbiosStructures;

    CheckSum = 0;
    mSmbiosLegacyEntry->IntermediateChecksum = 0;
    for (Index = 0x10; Index < mSmbiosLegacyEntry->EntryPointLength; Index++) {
      CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosLegacyEntry))[Index]);
    }
    mSmbiosLegacyEntry->IntermediateChecksum = (UINT8)(0 - CheckSum);

    CheckSum = 0;
    mSmbiosLegacyEntry->EntryPointStructureChecksum = 0;
    for (Index = 0x0; Index < mSmbiosLegacyEntry->EntryPointLength; Index++) {
      CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosLegacyEntry))[Index]);
    }
    mSmbiosLegacyEntry->EntryPointStructureChecksum = (UINT8)(0 - CheckSum);
  }
  //
  // Recover E & F segment write
  //
  DisableMemAccess (FsegValue8, EsegValue8);
}


UINT16
PnpUpdateStringDataInMem (
  IN PNP_52_DATA_BUFFER       *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  StrucTemp;
  UINT8                     StringIndex;
  UINT8                     Index;
  UINT8                     *UpdateBufPtr   = NULL;
  UINT8                     *StringDataPtr  = NULL;
  UINT8                     *StrucEndPtr    = NULL;
  UINTN                     CopySize        = 0;

  //
  // Check whether the smbios update buffer can contain the new smbios table
  //
  if ((mSmbiosEfiEntry->TableLength + DataBufferPtr->DataLength > SMBIOS_BUFFER_SIZE) ||
      (DataBufferPtr->DataLength == 1)) {
    return PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
  }

  //
  // Clean out the smbios update buffer
  //
  ZeroMem (mSmbiosBufferPtr, SMBIOS_BUFFER_SIZE);

  //
  // Copy the structure into update buffer except all the string data belonged
  //
  UpdateBufPtr = mSmbiosBufferPtr;
  CopyMem (UpdateBufPtr, StrucPtr->Raw, StrucPtr->Hdr->Length);
  UpdateBufPtr += StrucPtr->Hdr->Length;

  //
  // Get the string number, 1 based
  //
  StringIndex = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);

  //
  // if no string belonged, return error
  //
  if (StringIndex == 0)
    return PNP_BIOS_DMI_RETURN_BAD_PARAMETER;

  StringDataPtr = StrucPtr->Raw + StrucPtr->Hdr->Length;
  StrucEndPtr   =  StrucPtr->Raw + PnpGetSmbiosStructureSize (StrucPtr) -1;
  Index         = 1;

  while (StringDataPtr < StrucEndPtr) {
    if (Index != StringIndex) {
      CopySize = PnpStringDataLen (StringDataPtr) + 1;
      CopyMem(UpdateBufPtr, StringDataPtr, CopySize);
      UpdateBufPtr += CopySize;
      StringDataPtr += CopySize;
    } else {
      CopySize = DataBufferPtr->DataLength;
      CopyMem(UpdateBufPtr, DataBufferPtr->StructureData, CopySize);
      UpdateBufPtr += CopySize;
      CopySize = PnpStringDataLen(StringDataPtr) + 1;
      StringDataPtr += CopySize;
    }
    Index++;
  }

  //
  // Add the NULL byte at the end of the string set
  //
  *UpdateBufPtr = 0;
  UpdateBufPtr++;
  StringDataPtr++;

  //
  // Now StringDataPtr point to the next sequential structure,
  // Copy the remained structures content into update buffer
  //
  StrucTemp.Raw = StringDataPtr;
  CopySize = PnpGetSmbiosStructuresSize(&StrucTemp);
  CopyMem (UpdateBufPtr, StringDataPtr, CopySize);

  //
  // Copy the update buffer content into smbios table
  //
  StrucTemp.Raw = mSmbiosBufferPtr;
  CopySize = PnpGetSmbiosStructuresSize(&StrucTemp);
  CopyMem (StrucPtr->Raw,  mSmbiosBufferPtr, CopySize);

  //
  // Update the smbios entry in mem because of the changed table length
  //
  PnpUpdateSmbiosEntryInMem();
  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

UINT16
PnpUpdateBlockDataInMem (
  IN PNP_52_DATA_BUFFER       *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  UINT8 *BlockDataPtr = NULL;

  BlockDataPtr = StrucPtr->Raw + DataBufferPtr->FieldOffset;

  //
  // Copy the content into the block area
  //
  CopyMem (BlockDataPtr, DataBufferPtr->StructureData, DataBufferPtr->DataLength);
  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

/**

  Get the full size of smbios structure including optional strings that follow the formatted structure.

  @param Head                   Pointer to the beginning of smbios structure.
  @param Size                   The returned size.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size retured in Size.
  @retval EFI_INVALID_PARAMETER Input smbios structure mal-formed or Size is NULL.

**/
EFI_STATUS
EFIAPI
PnpGetSmbiosRecordSize (
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINTN                            *Size
  )
{
  UINTN  FullSize;
  UINT8  StrLen;
  INT8*  CharInStr;

  if (Size == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FullSize = Head->Length;
  CharInStr = (INT8*)Head + Head->Length;
  *Size = FullSize;
  StrLen = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      *Size += 1;
      CharInStr++;
    }

    for (StrLen = 0 ; StrLen < SMBIOS_STRING_MAX_LENGTH; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    if (StrLen == SMBIOS_STRING_MAX_LENGTH) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // forward the pointer
    //
    CharInStr += StrLen;
    *Size += StrLen;
  }

  //
  // count ending two zeros.
  //
  *Size += 2;
  return EFI_SUCCESS;
}

UINT16
PnpUpdateRecordInMem (
  IN PNP_52_DATA_BUFFER       *DataBufferPtr
)
{
  EFI_SMBIOS_HANDLE               Handle;
  EFI_SMBIOS_TABLE_HEADER         *DataRecord;
  EFI_SMBIOS_TABLE_HEADER         *BufferRecord;
  UINTN                           RecordSize;
  UINTN                           BufferRecordSize = 0;
  EFI_SMBIOS_TABLE_END_STRUCTURE  EndStructure;
  UINT8                           *BufferPtr;
  UINT8                           *BufferEndPtr;
  UINT8                           *SourceBuffer;
  UINT8                           *DestinationBuffer;
  UINTN                           Index;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry ();

  DataRecord = (EFI_SMBIOS_TABLE_HEADER *)&DataBufferPtr->StructureHeader;
  Handle     = DataRecord->Handle;
  PnpGetSmbiosRecordSize (DataRecord, &RecordSize);
  BufferPtr    = (UINT8 *)(UINTN)mSmbiosEfiEntry->TableAddress;
  BufferEndPtr = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress +
                                  mSmbiosEfiEntry->TableLength -
                                  sizeof (EndStructure));
  if (DataBufferPtr->Command == AddChanged) {
    //
    // Find out the max Handle
    //
    while(BufferPtr < BufferEndPtr) {
      BufferRecord = (EFI_SMBIOS_TABLE_HEADER *)BufferPtr;
      if (Handle < BufferRecord->Handle) {
        Handle = BufferRecord->Handle;
      } else if (Handle == BufferRecord->Handle) {
        Handle ++;
      }
      PnpGetSmbiosRecordSize (BufferRecord, &BufferRecordSize);
      BufferPtr += BufferRecordSize;
    }
    //
    // Reset the new Handle which is not used
    //
    DataRecord->Handle = Handle;
    DEBUG((EFI_D_ERROR,"add smbios Handle= %x\n", Handle));
    //
    // Backup End-Of-Table structure
    //
    CopyMem (&EndStructure, BufferEndPtr, sizeof (EndStructure));
    //
    // Copy new structure to table
    //
    CopyMem (BufferEndPtr, DataRecord, RecordSize);
    //
    // Restore End-Of-Table structure
    //
    BufferEndPtr += RecordSize;
    CopyMem (BufferEndPtr, &EndStructure, sizeof (EndStructure));
    //
    // Record NumberOfSmbiosStructures, TableLength and MaxStructureSize
    //
    mSmbiosEfiEntry->NumberOfSmbiosStructures ++;
    mSmbiosEfiEntry->TableLength = (UINT16) (mSmbiosEfiEntry->TableLength + RecordSize);
    if (RecordSize > mSmbiosEfiEntry->MaxStructureSize) {
      mSmbiosEfiEntry->MaxStructureSize = (UINT16) RecordSize;
    }
  } else {
    DEBUG((EFI_D_ERROR,"delete smbios Handle = %x\n", Handle));
    //
    // Get the Record by given Handle
    //
    while(BufferPtr < BufferEndPtr) {
      BufferRecord = (EFI_SMBIOS_TABLE_HEADER *)BufferPtr;
      //
      // Get the Record size
      //
      PnpGetSmbiosRecordSize (BufferRecord, &BufferRecordSize);
      if (Handle == BufferRecord->Handle) {
        break;
      }
      BufferPtr += BufferRecordSize;
    }

    DestinationBuffer = BufferPtr;
    SourceBuffer      = BufferPtr + BufferRecordSize;

    //
    // Get the rest Records size
    //
    RecordSize  = (UINTN)(mSmbiosEfiEntry->TableAddress +
                          mSmbiosEfiEntry->TableLength -
                          (UINTN)SourceBuffer);
    for (Index = 0; Index < RecordSize; Index ++) {
      DestinationBuffer[Index] = SourceBuffer[Index];
    }
    //
    // Record NumberOfSmbiosStructures, TableLength
    //
    mSmbiosEfiEntry->NumberOfSmbiosStructures --;
    mSmbiosEfiEntry->TableLength = (UINT16) (mSmbiosEfiEntry->TableLength - RecordSize);
  }

  //
  // Update the smbios entry in mem because of the changed table length
  //
  PnpUpdateSmbiosEntryInMem();
  return PNP_BIOS_DMI_RETURN_SUCCESS;
}

UINT16
PnpUpdateStrucInMem (
  IN PNP_52_DATA_BUFFER *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  UINT16 ErrorCode = PNP_BIOS_DMI_RETURN_FUNCTION_NOT_SUPPORTED;
  DEBUG((EFI_D_ERROR,"pnp smbios Command = %x\n", DataBufferPtr->Command));
  if ((DataBufferPtr->Command == ByteChanged) || (DataBufferPtr->Command == WordChanged)||
    (DataBufferPtr->Command == DoubleWordChanged)) {
      ErrorCode = PnpUpdateFixedLengthDataInMem (DataBufferPtr, StrucPtr);
  } else if (DataBufferPtr->Command == StringChanged) {
    ErrorCode =  PnpUpdateStringDataInMem (DataBufferPtr, StrucPtr);
  } else if (DataBufferPtr->Command == BlockChanged) {
    ErrorCode =  PnpUpdateBlockDataInMem (DataBufferPtr, StrucPtr);
  } else if (DataBufferPtr->Command == AddChanged || DataBufferPtr->Command == DeleteChanged) {
    ErrorCode =  PnpUpdateRecordInMem (DataBufferPtr);
  }
  return ErrorCode;
}

UINTN
CountDataSize (
  IN UINT8 *pBase
)
{
  UINTN               DataSize = 0;
  UINT8               *RecordPtr;
  UINT8               *RecordEndPtr;
  UINTN               Length;
  PNP_52_DATA_BUFFER  *DataBufferInRec;

  RecordPtr    = pBase;
  RecordEndPtr =  (UINT8 *)(pBase + SMBIOS_BUFFER_SIZE);

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
      break;

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr +sizeof (SMBIOS_REC_HEADER));
    Length    = sizeof (PNP_52_DATA_BUFFER) - 1;
    Length   += DataBufferInRec->DataLength;
    DataSize += Length + sizeof(SMBIOS_REC_HEADER);
    RecordPtr = RecordPtr + Length + sizeof (SMBIOS_REC_HEADER);
  }
  return DataSize;
}

VOID
GetFromFlash ()
{
  EFI_STATUS  Status;
  UINTN       Size = EFI_PAGE_SIZE;
  UINTN       SmbiosRecBase = FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE;
  LIST_ENTRY      *Link;
  PNP_52_RECORD   *Record;

  if (!IsListEmpty (&mPnp52HeadNode)) {
    Link = GetFirstNode (&mPnp52HeadNode);
    while(!IsNull (&mPnp52HeadNode, Link)) {
      Record = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
      Link = RemoveEntryList (&Record->Link);
      gSmst->SmmFreePool(Record);
    }
  }
  InitializeListHead (&mPnp52HeadNode);
  Status = Mem2RecordList((UINT8*)SmbiosRecBase, Size);
  ASSERT_EFI_ERROR (Status);
}

VOID
Write2Flash (
  PNP_52_DATA_BUFFER *DataBufferPtr
)
{
  UINTN       TotalLen;

  DeleteRecord (DataBufferPtr);
  InsertRecord (DataBufferPtr);
  SetMem (mSmbiosScratchPtr, EFI_PAGE_SIZE, 0xff);
  TotalLen = RecordList2Mem (mSmbiosScratchPtr);
  ASSERT(TotalLen <= SMBIOS_BUFFER_SIZE);
  if(TotalLen > SMBIOS_BUFFER_SIZE){
    TotalLen = SMBIOS_BUFFER_SIZE;
  }
  
  if (pMediaAccessProtocol != NULL) {
    pMediaAccessProtocol->Erase (
                            pMediaAccessProtocol,
                            FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE,
                            SMBIOS_BUFFER_SIZE,
                            SPI_MEDIA_TYPE
                            );
    pMediaAccessProtocol->Write (
                            pMediaAccessProtocol,
                            FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE,
                            mSmbiosScratchPtr,
                            TotalLen,
                            SPI_MEDIA_TYPE
                            );
  }
}

UINT16
PnpSetSmbiosStructure (
  IN VOID *ParameterBuffer
)
{
  PNP_PARAMS_52             *param = (PNP_PARAMS_52 *) ParameterBuffer;
  UINT32                    DataAddress;
  PNP_52_DATA_BUFFER        *DataBufferPtr;
  SMBIOS_STRUCTURE_POINTER  StrucPtr;
  UINT16                    ErrorCode = 0;

  //
  // Get the data buffer structure pointer
  //
  DataAddress   = ((param->DmiDataBufferPtr >> 16) << 4) + (param->DmiDataBufferPtr & 0xFFFF);
  DataBufferPtr = (PNP_52_DATA_BUFFER *)(UINTN) DataAddress;

  if (DataBufferPtr->Command != AddChanged && DataBufferPtr->Command != DeleteChanged) {
    //
    // Check the params in data buffer
    //
    ErrorCode = PnpValidateParamsInDataBuffer (DataBufferPtr);
    if (ErrorCode != 0)
      return ErrorCode;


    //
    // Find the structure with the handle number given by data buffer
    //
    PnpGetSmbiosStructureByHanlde (DataBufferPtr->StructureHeader.Handle, &StrucPtr);

    //
    // If not found, return invalid handle
    //
    if (StrucPtr.Raw == NULL)
      return PNP_BIOS_DMI_RETURN_INVALID_HANDLE;

    //
    // Check the structure header in data buffer has the same value
    // as the current structure in smbios table
    //
    if ((StrucPtr.Hdr->Type != DataBufferPtr->StructureHeader.Type)
      ||(StrucPtr.Hdr->Length !=  DataBufferPtr->StructureHeader.Length))
      return PNP_BIOS_DMI_RETURN_BAD_PARAMETER;
  }
  //
  // If control flag shows it only needs to validate params, just return success
  //
  if (!(param->Control & 1))
    return PNP_BIOS_DMI_RETURN_SUCCESS;

  //
  // Update contents in memory
  //
  ErrorCode = PnpUpdateStrucInMem (DataBufferPtr, &StrucPtr);

  if (ErrorCode != PNP_BIOS_DMI_RETURN_SUCCESS)
    return ErrorCode;
  //
  // Update contents in flash
  //
  Write2Flash (DataBufferPtr);

  return ErrorCode;
}

EFI_STATUS
AllocDataBuffer ()
{
  EFI_STATUS  Status;

  InitializeListHead (&mPnp52HeadNode);

  //
  // Allocate buffer for updating smbios table in memory
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    EFI_PAGE_SIZE,
                    (VOID **)&mSmbiosBufferPtr
                    );
  if (EFI_ERROR(Status))
    return Status;

  //
  // Allocate buffer for updating smbios table in flash
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    EFI_PAGE_SIZE,
                    (VOID **)&mSmbiosScratchPtr
                    );

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    PnpGetSmbiosEntry ();

  return Status;
}
