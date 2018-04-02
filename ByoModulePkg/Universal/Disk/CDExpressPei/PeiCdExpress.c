/** @file
  Source file for CD recovery PEIM

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PeiCdExpress.h"


PEI_CD_EXPRESS_PRIVATE_DATA *mPrivateData = NULL;
PEI_CD_EXPRESS_VIRTURAL_BLOCK_IO_RECORD     PeiVirtualBlockIoRecord;

/**
  Installs the Device Recovery Module PPI, Initialize BlockIo Ppi
  installation notification

  @param  FileHandle            The file handle of the image.
  @param  PeiServices           General purpose services available to every PEIM.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_OUT_OF_RESOURCES  There is not enough system memory.

**/
EFI_STATUS
EFIAPI
CdExpressPeimEntry (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS                  Status;
  PEI_CD_EXPRESS_PRIVATE_DATA *PrivateData;

  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  PrivateData = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (*PrivateData)));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize Private Data (to zero, as is required by subsequent operations)
  //
  ZeroMem (PrivateData, sizeof (*PrivateData));
  PrivateData->Signature    = PEI_CD_EXPRESS_PRIVATE_DATA_SIGNATURE;

  PrivateData->BlockBuffer  = AllocatePages (EFI_SIZE_TO_PAGES (PEI_CD_BLOCK_SIZE));
  if (PrivateData->BlockBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->CapsuleCount = 0;
  PeiVirtualBlockIoRecord.Count = 0;

  //
  // Installs Ppi
  //
  PrivateData->DeviceRecoveryPpi.GetNumberRecoveryCapsules  = GetNumberRecoveryCapsules;
  PrivateData->DeviceRecoveryPpi.GetRecoveryCapsuleInfo     = GetRecoveryCapsuleInfo;
  PrivateData->DeviceRecoveryPpi.LoadRecoveryCapsule        = LoadRecoveryCapsule;

  PrivateData->PpiDescriptor.Flags                          = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  PrivateData->PpiDescriptor.Guid = &gEfiPeiDeviceRecoveryModulePpiGuid;
  PrivateData->PpiDescriptor.Ppi = &PrivateData->DeviceRecoveryPpi;

  Status = PeiServicesInstallPpi (&PrivateData->PpiDescriptor);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // PrivateData is allocated now, set it to the module variable
  //
  mPrivateData = PrivateData;

  //
  // Installs Block Io Ppi notification function
  //
  PrivateData->NotifyDescriptor.Flags =
    (
      EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |
      EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST
    );
  PrivateData->NotifyDescriptor.Guid    = &gEfiPeiVirtualBlockIoPpiGuid;
  PrivateData->NotifyDescriptor.Notify  = BlockIoNotifyEntry;
  return PeiServicesNotifyPpi (&PrivateData->NotifyDescriptor);

}

/**
  BlockIo installation notification function.

  This function finds out all the current Block IO PPIs in the system and add them
  into private data.

  @param  PeiServices            Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor       Address of the notification descriptor data structure.
  @param  Ppi                    Address of the PPI that was installed.

  @retval EFI_SUCCESS            The function completes successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  UpdateBlocksAndVolumes (mPrivateData);

  return EFI_SUCCESS;
}

/**
  Finds out all the current Block IO PPIs in the system and add them into private data.

  @param PrivateData                    The private data structure that contains recovery module information.

  @retval EFI_SUCCESS                   The blocks and volumes are updated successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA     *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_PPI_DESCRIPTOR          *TempPpiDescriptor;
  UINTN                           BlockIoPpiInstance;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  UINTN                           NumberBlockDevices;
  UINTN                           IndexBlockDevice;
  EFI_PEI_BLOCK_IO_MEDIA          Media;
  EFI_PEI_SERVICES                **PeiServices;
  UINTN                           Index;
  BOOLEAN                         Notifyed;
  BLOCK_DEVICE_INFO               DeviceInfo;

  //
  // Find out all Block Io Ppi instances within the system
  // Assuming all device Block Io Peims are dispatched already
  //
  for (BlockIoPpiInstance = 0; BlockIoPpiInstance < PEI_CD_EXPRESS_MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
    Status = PeiServicesLocatePpi (
                              &gEfiPeiVirtualBlockIoPpiGuid,
                              BlockIoPpiInstance,
                              &TempPpiDescriptor,
                              (VOID **) &BlockIoPpi
                              );
    if (EFI_ERROR (Status)) {
      //
      // Done with all Block Io Ppis
      //
      break;
    }

    //
    // if this Ppi is notifyed already, do next, or record it
    //
    Notifyed = FALSE;
    for (Index = 0; Index < PeiVirtualBlockIoRecord.Count; Index ++) {
      if (PeiVirtualBlockIoRecord.BlockIo[Index] == BlockIoPpi) {
        Notifyed = TRUE;
      }
    }

    if (Notifyed == TRUE) {
      continue;
    } else {
      PeiVirtualBlockIoRecord.BlockIo[PeiVirtualBlockIoRecord.Count] = BlockIoPpi;
      PeiVirtualBlockIoRecord.Count ++;
    }

    PeiServices = (EFI_PEI_SERVICES  **) GetPeiServicesTablePointer ();
    Status = BlockIoPpi->GetNumberOfBlockDevices (
                          PeiServices,
                          BlockIoPpi,
                          &NumberBlockDevices
                          );
    if (EFI_ERROR (Status) || (NumberBlockDevices == 0)) {
      continue;
    }
    //
    // Just retrieve the first block, should emulate all blocks.
    //
    DeviceInfo.BlockIo = NULL;
    DeviceInfo.DeviceIndex = 1;

    for (IndexBlockDevice = 1; IndexBlockDevice <= NumberBlockDevices && PrivateData->CapsuleCount < PEI_CD_EXPRESS_MAX_CAPSULE_NUMBER; IndexBlockDevice ++) {
      Status = BlockIoPpi->GetBlockDeviceMediaInfo (
                            PeiServices,
                            BlockIoPpi,
                            IndexBlockDevice,
                            &Media
                            );
      if (EFI_ERROR (Status) ||
          !Media.MediaPresent ||
           ((Media.DeviceType != IdeCDROM) && (Media.DeviceType != UsbMassStorage)) ||
          (Media.BlockSize != PEI_CD_BLOCK_SIZE)
          ) {
        continue;
      }

      DEBUG ((EFI_D_INFO, "PeiCdExpress DeviceType is   %d\n", Media.DeviceType));
      DEBUG ((EFI_D_INFO, "PeiCdExpress MediaPresent is %d\n", Media.MediaPresent));
      DEBUG ((EFI_D_INFO, "PeiCdExpress BlockSize is  0x%x\n", Media.BlockSize));
      DEBUG ((EFI_D_INFO, "PeiCdExpress Status is %r\n", Status));

      DEBUG ((EFI_D_INFO, "IndexBlockDevice is %d\n", IndexBlockDevice));
      DeviceInfo.DeviceIndex = IndexBlockDevice;
      DeviceInfo.BlockIo = BlockIoPpi;
      Status = FindRecoveryCapsules (PrivateData, DeviceInfo);
      DEBUG ((EFI_D_INFO, "Status is %r\n", Status));
    }
  }

  return EFI_SUCCESS;
}

/**
  Finds out the recovery capsule in the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.

**/
EFI_STATUS
EFIAPI
FindRecoveryCapsules (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA            *PrivateData,
  BLOCK_DEVICE_INFO                             DeviceInfo
  )
{
  EFI_STATUS                      Status;
  UINTN                           Lba;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  UINTN                           BufferSize;
  UINT8                           *Buffer;
  UINT8                           Type;
  UINT8                           *StandardID;
  UINT32                          RootDirLBA;
  PEI_CD_EXPRESS_DIR_FILE_RECORD  *RoorDirRecord;
  UINTN                           VolumeSpaceSize;
  BOOLEAN                         StartOfVolume;
  UINTN                           OriginalLBA;
  UINTN                           IndexBlockDevice;

  Buffer      = PrivateData->BlockBuffer;
  BufferSize  = PEI_CD_BLOCK_SIZE;

  Lba         = 16;
  //
  // The volume descriptor starts on Lba 16
  //
  IndexBlockDevice = DeviceInfo.DeviceIndex;
  BlockIoPpi = DeviceInfo.BlockIo;

  VolumeSpaceSize = 0;
  StartOfVolume   = TRUE;
  OriginalLBA     = 16;

  while (TRUE) {
    SetMem (Buffer, BufferSize, 0);
    Status = BlockIoPpi->ReadBlocks (
                          PrivateData->PeiServices,
                          BlockIoPpi,
                          IndexBlockDevice,
                          Lba,
                          BufferSize,
                          Buffer
                          );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    StandardID = (UINT8 *) (Buffer + PEI_CD_EXPRESS_STANDARD_ID_OFFSET);
    if (!StringCmp (StandardID, (UINT8 *) PEI_CD_STANDARD_ID, PEI_CD_EXPRESS_STANDARD_ID_SIZE, TRUE)) {
      break;
    }

    if (StartOfVolume) {
      OriginalLBA   = Lba;
      StartOfVolume = FALSE;
    }

    Type = *(UINT8 *) (Buffer + PEI_CD_EXPRESS_VOLUME_TYPE_OFFSET);
    if (Type == PEI_CD_EXPRESS_VOLUME_TYPE_TERMINATOR) {
      if (VolumeSpaceSize == 0) {
        break;
      } else {
        Lba             = (OriginalLBA + VolumeSpaceSize);
        VolumeSpaceSize = 0;
        StartOfVolume   = TRUE;
        continue;
      }
    }

    if (Type != PEI_CD_EXPRESS_VOLUME_TYPE_PRIMARY) {
      Lba++;
      continue;
    }

    VolumeSpaceSize = *(UINT32 *) (Buffer + PEI_CD_EXPRESS_VOLUME_SPACE_OFFSET);

    RoorDirRecord   = (PEI_CD_EXPRESS_DIR_FILE_RECORD *) (Buffer + PEI_CD_EXPRESS_ROOT_DIR_RECORD_OFFSET);
    RootDirLBA      = RoorDirRecord->LocationOfExtent[0];

    Status          = RetrieveCapsuleFileFromRoot (PrivateData, BlockIoPpi, IndexBlockDevice, RootDirLBA, DeviceInfo);
    if (!EFI_ERROR (Status)) {
      //
      // Just look for the first primary descriptor
      //
      return EFI_SUCCESS;
    }

    Lba++;
  }

  return EFI_NOT_FOUND;
}

/**
  Converts a union code character to upper case.
  This functions converts a unicode character to upper case.
  If the input Letter is not a lower-cased letter,
  the original value is returned.

  @param  Letter            The input unicode character.

  @return The upper cased letter.

**/
CHAR8
ToUpper (
  IN CHAR8                    Letter
  )
{
  if ('a' <= Letter && Letter <= 'z') {
    Letter = (CHAR8) (Letter - 0x20);
  }

  return Letter;
}


/**
  Check if a file name contain the special characters and has a .fd extension.

  @param  FileID                a pointer to file name string.
  @param  FileIDLength          Length of file name.
  @param  SpecialString         a pointer to a special string.
  @param  CaseSensitive         Flag to indicate whether the comparison is case sensitive.

  @return  TRUE                 The file is that we want to find
  @return  FALSE                Not file we want to find

**/
BOOLEAN
IsSpecialFile (
  IN UINT8          *FileID,
  IN UINT8          FileIDLength,
  IN UINT8          *SpecialString
  )
{
  UINT8       *FileIDExt;
  UINT8       Index;
  UINT8       BiosExt[4];
  UINT8       *FileExt;

  //
  // go to file name end
  //
  for (Index = 0; Index < FileIDLength; Index++) {
    if (FileID[Index] == ';') {
      break;
    }
  }

  //
  // find file name extension
  //
  for (; Index > 0; Index--) {
    if (FileID[Index] == '.') {
      break;
    }
  }

  //
  // no extension find, false
  //
  if (FileID[Index] != '.') {
    return FALSE;
  }

  FileIDExt = FileID + Index + 1;

  UnicodeStrToAsciiStr ((CHAR16 *)PcdGetPtr (PcdBiosFileExt), BiosExt);
  FileExt = BiosExt;
  while (*FileExt != '\0') {
    if (ToUpper(*FileExt) == ToUpper(*FileIDExt)) {
      FileExt ++;
      FileIDExt ++;
    } else {
      return FALSE;
    }
  }

  if (*FileIDExt != ';' && *FileIDExt != '\0') {
    return FALSE;
  }

// Do not check base name here as short name may be different to the first part of long name.
// We will check biosid in new fd later.
/*  
  while (*SpecialString != '\0' && *FileID != '~') {
    if (ToUpper(*FileID) == ToUpper(*SpecialString)) {
      FileID ++;
      SpecialString ++;
    } else {
      return FALSE;
    }
  }
*/
  return TRUE;
}

/**
  Retrieves the recovery capsule in root directory of the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.
  @param BlockIoPpi                     The Block IO PPI used to access the volume.
  @param IndexBlockDevice               The index of current block device.
  @param Lba                            The starting logic block address to retrieve capsule.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.
  @retval Others

**/
EFI_STATUS
EFIAPI
RetrieveCapsuleFileFromRoot (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA        *PrivateData,
  IN EFI_PEI_RECOVERY_BLOCK_IO_PPI          *BlockIoPpi,
  IN UINTN                                  IndexBlockDevice,
  IN UINT32                                 Lba,
  BLOCK_DEVICE_INFO                         DeviceInfo
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  UINT8                           *Buffer;
  PEI_CD_EXPRESS_DIR_FILE_RECORD  *FileRecord;
//-  UINTN                           Index;

  UINT8                           NoFile;

  Buffer      = PrivateData->BlockBuffer;
  BufferSize  = PEI_CD_BLOCK_SIZE;

  SetMem (Buffer, BufferSize, 0);

  Status = BlockIoPpi->ReadBlocks (
                        PrivateData->PeiServices,
                        BlockIoPpi,
                        IndexBlockDevice,
                        Lba,
                        BufferSize,
                        Buffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NoFile = 0;
  while (1) {
    FileRecord = (PEI_CD_EXPRESS_DIR_FILE_RECORD *) Buffer;

    if (FileRecord->Length == 0) {
      break;
    }
    //
    // Not intend to check other flag now
    //
    if ((FileRecord->Flag & PEI_CD_EXPRESS_DIR_FILE_REC_FLAG_ISDIR) != 0) {
      Buffer += FileRecord->Length;
      continue;
    }

// invalid code
//-    for (Index = 0; Index < FileRecord->FileIDLength; Index++) {
//-      if (FileRecord->FileID[Index] == ';') {
//-        break;
//-      }
//-    }

    if (IsSpecialFile (FileRecord->FileID, FileRecord->FileIDLength, NULL)) {
      if (NoFile < PEI_CD_EXPRESS_MAX_FILES_IN_DEVICE) {
        NoFile ++;
        PrivateData->CapsuleData[PrivateData->CapsuleCount].CapsuleStartLBA = (UINTN)FileRecord->LocationOfExtent[0];
        PrivateData->CapsuleData[PrivateData->CapsuleCount].CapsuleSize = (UINTN)FileRecord->DataLength[0];
        PrivateData->CapsuleData[PrivateData->CapsuleCount].IndexBlock = DeviceInfo.DeviceIndex;
        PrivateData->CapsuleData[PrivateData->CapsuleCount].BlockIo = DeviceInfo.BlockIo;
        PrivateData->CapsuleCount ++;
      } else {
        break;
      }
    }

    Buffer += FileRecord->Length;
  }

  if (NoFile == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Returns the number of DXE capsules residing on the device.

  This function searches for DXE capsules from the associated device and returns
  the number and maximum size in bytes of the capsules discovered. Entry 1 is
  assumed to be the highest load priority and entry N is assumed to be the lowest
  priority.

  @param[in]  PeiServices              General-purpose services that are available
                                       to every PEIM
  @param[in]  This                     Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                       instance.
  @param[out] NumberRecoveryCapsules   Pointer to a caller-allocated UINTN. On
                                       output, *NumberRecoveryCapsules contains
                                       the number of recovery capsule images
                                       available for retrieval from this PEIM
                                       instance.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetNumberRecoveryCapsules (
  IN EFI_PEI_SERVICES                               **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI             *This,
  OUT UINTN                                         *NumberRecoveryCapsules
  )
{
  PEI_CD_EXPRESS_PRIVATE_DATA *PrivateData;

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);
  *NumberRecoveryCapsules = PrivateData->CapsuleCount;

  if (*NumberRecoveryCapsules == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Returns the size and type of the requested recovery capsule.

  This function gets the size and type of the capsule specified by CapsuleInstance.

  @param[in]  PeiServices       General-purpose services that are available to every PEIM
  @param[in]  This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                instance.
  @param[in]  CapsuleInstance   Specifies for which capsule instance to retrieve
                                the information.  This parameter must be between
                                one and the value returned by GetNumberRecoveryCapsules()
                                in NumberRecoveryCapsules.
  @param[out] Size              A pointer to a caller-allocated UINTN in which
                                the size of the requested recovery module is
                                returned.
  @param[out] CapsuleType       A pointer to a caller-allocated EFI_GUID in which
                                the type of the requested recovery capsule is
                                returned.  The semantic meaning of the value
                                returned is defined by the implementation.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetRecoveryCapsuleInfo (
  IN  EFI_PEI_SERVICES                              **PeiServices,
  IN  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI            *This,
  IN  UINTN                                         CapsuleInstance,
  OUT UINTN                                         *Size,
  OUT EFI_GUID                                      *CapsuleType
  )
{
  PEI_CD_EXPRESS_PRIVATE_DATA *PrivateData;
  UINTN                       NumberRecoveryCapsules;
  EFI_STATUS                  Status;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);

  *Size = PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleSize;
  CopyMem (
    CapsuleType,
    &gRecoveryOnDataCdGuid,
    sizeof (EFI_GUID)
    );

  return EFI_SUCCESS;
}

/**
  Loads a DXE capsule from some media into memory.

  This function, by whatever mechanism, retrieves a DXE capsule from some device
  and loads it into memory. Note that the published interface is device neutral.

  @param[in]     PeiServices       General-purpose services that are available
                                   to every PEIM
  @param[in]     This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                   instance.
  @param[in]     CapsuleInstance   Specifies which capsule instance to retrieve.
  @param[out]    Buffer            Specifies a caller-allocated buffer in which
                                   the requested recovery capsule will be returned.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A requested recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadRecoveryCapsule (
  IN EFI_PEI_SERVICES                             **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI           *This,
  IN UINTN                                        CapsuleInstance,
  OUT VOID                                        *Buffer
  )
{
  EFI_STATUS                      Status;
  PEI_CD_EXPRESS_PRIVATE_DATA     *PrivateData;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  UINTN                           NumberRecoveryCapsules;
  UINTN                           BufferSize;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);
  BlockIoPpi  = PrivateData->CapsuleData[CapsuleInstance - 1].BlockIo;

  //
  // ajust read block size
  //
  BufferSize = ((PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleSize - 1) / PEI_CD_BLOCK_SIZE + 1) * PEI_CD_BLOCK_SIZE;

  Status = BlockIoPpi->ReadBlocks (
                        PrivateData->PeiServices,
                        BlockIoPpi,
                        PrivateData->CapsuleData[CapsuleInstance - 1].IndexBlock,
                        PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleStartLBA,
                        BufferSize,
                        Buffer
                        );
  return Status;
}

/**
  This function compares two ASCII strings in case sensitive/insensitive way.

  @param  Source1           The first string.
  @param  Source2           The second string.
  @param  Size              The maximum comparison length.
  @param  CaseSensitive     Flag to indicate whether the comparison is case sensitive.

  @retval TRUE              The two strings are the same.
  @retval FALSE             The two string are not the same.

**/
BOOLEAN
StringCmp (
  IN UINT8      *Source1,
  IN UINT8      *Source2,
  IN UINTN      Size,
  IN BOOLEAN    CaseSensitive
  )
{
  UINTN Index;
  UINT8 Dif;

  for (Index = 0; Index < Size; Index++) {
    if (Source1[Index] == Source2[Index]) {
      continue;
    }

    if (!CaseSensitive) {
      Dif = (UINT8) ((Source1[Index] > Source2[Index]) ? (Source1[Index] - Source2[Index]) : (Source2[Index] - Source1[Index]));
      if (Dif == ('a' - 'A')) {
        continue;
      }
    }

    return FALSE;
  }

  return TRUE;
}
