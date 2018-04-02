
#include "SmiFlash.h"

SMBIOS_TABLE_ENTRY_POINT        *mSmbiosLegacyEntry = NULL;
SMBIOS_TABLE_ENTRY_POINT        *mSmbiosEfiEntry    = NULL;
UINT8                           *mSmbiosBufferPtr   = NULL;
UINT8                           *mSmbiosScratchPtr  = NULL;
LIST_ENTRY                      mPnp52HeadNode;

EFI_STATUS
InsertRecord(
  PNP_52_DATA_BUFFER *pDataBufferPtr
)
{
  EFI_STATUS          Status;
  UINT16              RecordLen;
  PNP_52_RECORD       *Record;
  PNP_52_DATA_BUFFER  *pData;
  UINTN               Length;
  LIST_ENTRY          *Link;

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

  RecordLen = sizeof (SMBIOS_REC_HEADER) + sizeof (PNP_52_DATA_BUFFER) - 1;
  RecordLen = RecordLen + pDataBufferPtr->DataLength;
  Record->header.RecordLen  = RecordLen;

  Length  = sizeof (PNP_52_DATA_BUFFER) - 1;
  Length += pDataBufferPtr->DataLength;

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

UINTN
PnpStringDataLen (
  IN UINT8 *StringData
)
{
  UINT8 *StringPtr = StringData;

  for (; *StringPtr != 0; StringPtr ++);

  return (UINTN)(StringPtr - StringData);
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

EFI_STATUS
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
    return EFI_INVALID_PARAMETER;
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
  DEBUG((EFI_D_ERROR,"StringIndex = %x\n", StringIndex));

  //
  // if no string belonged, return error
  //
  if (StringIndex == 0)
    return EFI_INVALID_PARAMETER;

  StringDataPtr = StrucPtr->Raw + StrucPtr->Hdr->Length;
  StrucEndPtr   = StrucPtr->Raw + PnpGetSmbiosStructureSize (StrucPtr) - 1;
  Index         = 1;

  while (StringDataPtr < StrucEndPtr) {
    if (Index != StringIndex) {
      CopySize = PnpStringDataLen (StringDataPtr) + 1;
      CopyMem (UpdateBufPtr, StringDataPtr, CopySize);
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
  UpdateBufPtr ++;
  StringDataPtr ++;

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

  return EFI_SUCCESS;
}


EFI_STATUS
UpdateToMem (
  IN PNP_52_DATA_BUFFER *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  EFI_STATUS  Status = EFI_UNSUPPORTED;
  UINT32      OffsetOfSmbios;
  UINT32      Value32;
  UINT8       FsegValue8;
  UINT8       EsegValue8;

  DEBUG((EFI_D_ERROR,"smbios Command = %x\n", DataBufferPtr->Command));
  if (DataBufferPtr->Command == DoubleWordChanged) {
    Value32  = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
    Value32 &= DataBufferPtr->ChangeMask;
    Value32 |= DataBufferPtr->ChangeValue;
    *(UINT32 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset) = Value32;
    //
    // update legacy smbios table
    //
    if (mSmbiosLegacyEntry && mSmbiosEfiEntry) {
      //
      // Enable E & F segment write
      //
      EnableMemAccess(&FsegValue8, &EsegValue8);
      OffsetOfSmbios = (UINT32)(UINTN)(StrucPtr->Raw + DataBufferPtr->FieldOffset) - mSmbiosEfiEntry->TableAddress;
      *(UINT32 *)(UINTN)(mSmbiosLegacyEntry->TableAddress + OffsetOfSmbios) = Value32;
      //
      // Restore E & F segment write
      //
      DisableMemAccess (FsegValue8, EsegValue8);
    }
    Status = EFI_SUCCESS;
  } else if (DataBufferPtr->Command == StringChanged) {
    Status =  PnpUpdateStringDataInMem (DataBufferPtr, StrucPtr);
    Status = EFI_SUCCESS;
  }
  DeleteRecord (DataBufferPtr);
  InsertRecord (DataBufferPtr);

  return Status;
}

VOID
GetSmbiosDataFromFlash ()
{
  EFI_STATUS      Status;
  UINTN           Size = EFI_PAGE_SIZE;
  UINTN           SmbiosRecBase = FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE;
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
  UINT8     *SourceAddress,
  UINTN     FlashAddress,
  UINTN     Size
)
{
  if (mMediaAccess != NULL && Size > 0) {
    mMediaAccess->Erase (
                    mMediaAccess,
                    FlashAddress,
                    SIZE_4KB,
                    SPI_MEDIA_TYPE
                    );
    mMediaAccess->Write (
                    mMediaAccess,
                    FlashAddress,
                    SourceAddress,
                    Size,
                    SPI_MEDIA_TYPE
                    );
  }
}


VOID
WriteSmbiosData2Flash ()
{
  UINTN       TotalLen;

  SetMem (mSmbiosScratchPtr, EFI_PAGE_SIZE, 0xff);
  TotalLen = RecordList2Mem (mSmbiosScratchPtr);

  Write2Flash (mSmbiosScratchPtr, FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE, TotalLen);

}

EFI_STATUS
GetValueFromAsccii (
  IN OUT UINT8  *BinBuffer,
  IN OUT UINT32 *BinLength,
  IN     CHAR8  *HexStr
  )
{
  UINTN   Index;
  UINTN   Length;
  UINT8   Digit;
  CHAR8   TemStr[2];

  ZeroMem (TemStr, sizeof (TemStr));

  //
  // Find out how many hex characters the string has.
  //
  if ((HexStr[0] == '0') && ((HexStr[1] == 'x') || (HexStr[1] == 'X'))) {
    HexStr += 2;
  }

  Length = AsciiStrLen (HexStr);

  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = HexStr[Index];
    Digit = (UINT8) AsciiStrHexToUint64 (TemStr);
    if (Digit == 0 && TemStr[0] != '0') {
      //
      // Invalid Lun Char
      //
      break;
    }
    if ((Index & 1) == 0) {
      BinBuffer [Index/2] = Digit;
    } else {
      BinBuffer [Index/2] = (UINT8) ((BinBuffer [Index/2] << 4) + Digit);
    }
  }

  *BinLength = (UINT32) ((Index + 1)/2);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
{
  UINTN Index;
  ASSERT (Table != NULL);

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


VOID
GetSmbiosEntry (
  VOID
)
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
    GetSystemConfigurationTable (
      &gEfiSmbiosTableGuid,
      (VOID **)&mSmbiosEfiEntry
    );
    ASSERT (mSmbiosEfiEntry != NULL);
  }

  return;
}

BOOLEAN
GetSmbiosStructureByType (
  IN UINT8                    StrucType,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    GetSmbiosEntry ();

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
    if (Smbios.Hdr->Type== StrucType) {
      StrucPtr->Raw = Smbios.Raw;
      return TRUE;
    }
    Smbios.Raw = Smbios.Raw + PnpGetSmbiosStructureSize (&Smbios);
  }

  //
  // if no matched structure is found, return null
  //
  StrucPtr->Raw = NULL;
  return FALSE;
}


EFI_STATUS AllocDataBuffer()
{
  EFI_STATUS  Status;

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

  InitializeListHead (&mPnp52HeadNode);

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    GetSmbiosEntry ();

  return Status;
}





EFI_STATUS HandleSmbiosDataRequest(UPDATE_SMBIOS_PARAMETER *SmbiosPtr)
{
  SMBIOS_STRUCTURE_POINTER StrucPtr;
  PNP_52_DATA_BUFFER       *Parameter;
  UINT8                    Index;
  UINT32                   DataLength = 0;
  UINT32                   Data32[4];
  BOOLEAN                  NeedUpdate  = FALSE;
  BOOLEAN                  DataIsValid = TRUE;
  EFI_STATUS               Status = EFI_SUCCESS;
  

  Parameter = &SmbiosPtr->Parameter;
  DEBUG ((DEBUG_INFO, "SubFun:%x\n",SmbiosPtr->SubFun));

  Parameter->DataLength = (UINT16)AsciiStrLen(Parameter->StructureData) + 1;
  DEBUG ((DEBUG_INFO, "Parameter->DataLength:%x\n",Parameter->DataLength));
  if (Parameter->DataLength == 1) {
    Status = RETURN_INVALID_PARAMETER;
    goto ProcExit;
  }

  if (SmbiosPtr->SubFun == UPDATE_UUID ||
      SmbiosPtr->SubFun == UPDATE_SERIAL_NUMBER ||
      SmbiosPtr->SubFun == UPDATE_MODEL_NUMBER ||
      SmbiosPtr->SubFun == UPDATE_BRAND_ID) {
    Parameter->StructureHeader.Type = 1;
  } else if (SmbiosPtr->SubFun == UPDATE_BASE_BOARD_SERIAL_NUMBER ||
             SmbiosPtr->SubFun == UPDATE_BASE_BOARD_ASSET_TAG) {
    Parameter->StructureHeader.Type = 2;
  } else if (SmbiosPtr->SubFun == UPDATE_CHASSIS_ASSET_TAG ||
             SmbiosPtr->SubFun == UPDATE_CHASSIS_SERIAL_NUMBER/* ||
             SmbiosPtr->SubFun == UPDATE_CHASSIS_ASSET_TAG*/) {
    Parameter->StructureHeader.Type = 3;
  }

  if (GetSmbiosStructureByType (Parameter->StructureHeader.Type, &StrucPtr)) {
    Parameter->StructureHeader.Length = StrucPtr.Hdr->Length;
    Parameter->StructureHeader.Handle = StrucPtr.Hdr->Handle;
    Parameter->Command                = StringChanged;  // default command
    NeedUpdate = TRUE;
  }

  GetSmbiosDataFromFlash ();

  switch(SmbiosPtr->SubFun) {
    
    case UPDATE_UUID:
      for (Index = 0; Index < 0x20; Index ++) {
        if (Parameter->StructureData[Index] > 'f') {
          DataIsValid = FALSE;
        } else if (Parameter->StructureData[Index] < 'a' && Parameter->StructureData[Index] > 'F') {
          DataIsValid = FALSE;
        } else if (Parameter->StructureData[Index] < 'A' && Parameter->StructureData[Index] > '9') {
          DataIsValid = FALSE;
        } else if (Parameter->StructureData[Index] < '0') {
          DataIsValid = FALSE;
        }
        if (DataIsValid == FALSE) {
          NeedUpdate = FALSE;
          Status     = RETURN_INVALID_PARAMETER;
          break;
        }
      }
      if (DataIsValid) {
        GetValueFromAsccii ((UINT8*)(UINTN)Data32, &DataLength, Parameter->StructureData);
      }
      
      if (DataIsValid) {
        Parameter->Command     = DoubleWordChanged;
        Parameter->ChangeMask  = 0;
        Parameter->DataLength  = 0;
        Parameter->FieldOffset = 0x08; // UUID offset of Type1
        for (Index = 0; Index < (DataLength / 4); Index ++) {
          Parameter->ChangeValue = Data32[Index];
          UpdateToMem (Parameter, &StrucPtr);
          Parameter->FieldOffset += 4;
        }
      }
      break;
      
    case UPDATE_SERIAL_NUMBER:
    case UPDATE_BASE_BOARD_SERIAL_NUMBER:
    case UPDATE_CHASSIS_SERIAL_NUMBER:
      Parameter->FieldOffset = 0x07;
      break;
      
    case UPDATE_MODEL_NUMBER:
      Parameter->FieldOffset = 0x05;
      break;
      
    case UPDATE_BRAND_ID:
      Parameter->FieldOffset = 0x06;
      break;
      
    case UPDATE_CHASSIS_ASSET_TAG:
    case UPDATE_BASE_BOARD_ASSET_TAG:
    //case UPDATE_CHASSIS_ASSET_TAG:
      Parameter->FieldOffset = 0x08;
      break;
      
    default:
      Status = RETURN_INVALID_PARAMETER;
      break;
  }
      
  if (NeedUpdate) {
    if (Parameter->Command == StringChanged) {
      if (Parameter->DataLength > MAX_STRING_LENGTH) {
        Parameter->StructureData[MAX_STRING_LENGTH] = '\0';
        Parameter->DataLength = MAX_STRING_LENGTH + 1;
      }
      UpdateToMem (Parameter, &StrucPtr);
    }
    WriteSmbiosData2Flash ();
  }
  
ProcExit:
  return Status;  
}







