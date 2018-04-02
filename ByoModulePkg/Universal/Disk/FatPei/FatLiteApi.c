/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/** @file
  FAT recovery PEIM entry point, Ppi Functions and FAT Api functions.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation
(if any) is furnished under a license and may only be used or
copied in accordance with the terms of the license.  Except as
permitted by such license, no part of this software or
documentation may be reproduced, stored in a retrieval system, or
transmitted in any form or by any means without the express written
consent of Intel Corporation.

**/

#include "FatLitePeim.h"

PEI_FAT_PRIVATE_DATA  *mPrivateData = NULL;

PEI_FAT_VIRTURAL_BLOCK_IO_RECORD    FatVirtualBlockIoRecord;

/**
  BlockIo installation nofication function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );


/**
  Discover all the block I/O devices to find the FAT volume.

  @param  PrivateData             Global memory map for accessing global
                                  variables.

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes (
  IN OUT PEI_FAT_PRIVATE_DATA            *PrivateData
  )
{
  EFI_STATUS                    Status;
  EFI_PEI_PPI_DESCRIPTOR        *TempPpiDescriptor;
  UINTN                         BlockIoPpiInstance;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI *BlockIoPpi;
  UINTN                         NumberBlockDevices;
  UINTN                         Index;
  EFI_PEI_BLOCK_IO_MEDIA        Media;
  PEI_FAT_VOLUME                Volume;
  EFI_PEI_SERVICES              **PeiServices;
  PEI_FAT_FILE                  *FileBuffer;
  UINT8                         NoFile;
  UINT8                         FileCount;
  BOOLEAN                       Notifyed;
  UINTN                         OldBlockDeviceCount;
  UINTN                         OldVolumeCount;
  UINTN                         OldCapsuleCount;

  OldBlockDeviceCount = PrivateData->BlockDeviceCount;
  OldVolumeCount = PrivateData->VolumeCount;
  OldCapsuleCount = PrivateData->CapsuleCount;

  PeiServices = (EFI_PEI_SERVICES **) GetPeiServicesTablePointer ();

  //
  // Clean up caches
  //
  for (Index = 0; Index < PEI_FAT_CACHE_SIZE; Index++) {
    PrivateData->CacheBuffer[Index].Valid = FALSE;
  }

  //
  // Find out all Block Io Ppi instances within the system
  // Assuming all device Block Io Peims are dispatched already
  //
  for (BlockIoPpiInstance = 0; BlockIoPpiInstance < PEI_FAT_MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
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
    // and search capsule file
    //
    Notifyed = FALSE;
    for (Index = 0; Index < FatVirtualBlockIoRecord.Count; Index ++) {
      if (FatVirtualBlockIoRecord.BlockIo[Index] == BlockIoPpi) {
        Notifyed = TRUE;
      }
    }

    if (Notifyed == TRUE) {
      continue;
    } else {
      FatVirtualBlockIoRecord.BlockIo[FatVirtualBlockIoRecord.Count] = BlockIoPpi;
      FatVirtualBlockIoRecord.Count ++;
    }

    Status = BlockIoPpi->GetNumberOfBlockDevices (
                          PeiServices,
                          BlockIoPpi,
                          &NumberBlockDevices
                          );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 1; Index <= NumberBlockDevices && PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE; Index++) {

      Status = BlockIoPpi->GetBlockDeviceMediaInfo (
                            PeiServices,
                            BlockIoPpi,
                            Index,
                            &Media
                            );
      if (EFI_ERROR (Status) || !Media.MediaPresent || Media.DeviceType == IdeCDROM) {
        continue;
      }

      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockSize = (UINT32) Media.BlockSize;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].LastBlock = Media.LastBlock;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].IoAlign   = 0;
      //
      // Not used here
      //
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].Logical           = FALSE;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PartitionChecked  = FALSE;

      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockIo           = BlockIoPpi;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PhysicalDevNo     = (UINT8) Index;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].DevType           = Media.DeviceType;

      PrivateData->BlockDeviceCount++;
    }
  }

  if (PrivateData->BlockDeviceCount - OldBlockDeviceCount) {
    DEBUG ((EFI_D_INFO, "FAT find %d recovery block devices\n", PrivateData->BlockDeviceCount - OldBlockDeviceCount));
    //
    // Find out all logical devices
    //
    FatFindPartitions (PrivateData, OldBlockDeviceCount);

    //
    // Build up file system volume array
    //
    for (Index = OldBlockDeviceCount; Index < PrivateData->BlockDeviceCount; Index++) {
      Volume.BlockDeviceNo  = Index;
      Status                = FatGetBpbInfo (PrivateData, &Volume);
      if (Status == EFI_SUCCESS) {
        //
        // Add the detected volume to the volume array
        //
        CopyMem (
          (UINT8 *) &(PrivateData->Volume[PrivateData->VolumeCount]),
          (UINT8 *) &Volume,
          sizeof (PEI_FAT_VOLUME)
        );
        PrivateData->VolumeCount ++;
        if (PrivateData->VolumeCount >= PEI_FAT_MAX_VOLUME) {
          break;
        }
      }
    }
    DEBUG ((EFI_D_INFO, "FAT find %d volume(s)\n", PrivateData->VolumeCount - OldVolumeCount));

    //
    // build up capsule file array
    //
    if (PrivateData->VolumeCount - OldVolumeCount) {
      for (Index = OldVolumeCount; Index < PrivateData->VolumeCount; Index ++) {
        NoFile = 0;
        Status = FindRecoveryFile (PrivateData, Index, &NoFile, &FileBuffer);
        if (!EFI_ERROR(Status)) {
          for (FileCount = 0; FileCount < NoFile; FileCount ++) {
            CopyMem (
              (UINT8*)&PrivateData->CapsuleData[PrivateData->CapsuleCount],
              (UINT8*)&FileBuffer[FileCount],
              sizeof(PEI_FAT_FILE)
              );
            PrivateData->CapsuleCount ++;
          }
        }
      }
    }

    DEBUG ((EFI_D_INFO, "FAT find %d recovery capsule(s)\n", PrivateData->CapsuleCount - OldCapsuleCount));
  } else {
    DEBUG ((EFI_D_INFO, "FAT not find recovery block device\n"));
  }

  return EFI_SUCCESS;
}


/**
  BlockIo installation notification function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

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
  Installs the Device Recovery Module PPI, Initialize BlockIo Ppi
  installation notification

  @param  FileHandle              Handle of the file being invoked. Type
                                  EFI_PEI_FILE_HANDLE is defined in
                                  FfsFindNextFile().
  @param  PeiServices             Describes the list of possible PEI Services.

  @retval EFI_SUCCESS             The entry point was executed successfully.
  @retval EFI_OUT_OF_RESOURCES    There is no enough memory to complete the
                                  operations.

**/
EFI_STATUS
EFIAPI
FatPeimEntry (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  PEI_FAT_PRIVATE_DATA  *PrivateData;

  Status = PeiServicesRegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiServicesAllocatePages (
            EfiBootServicesCode,
            (sizeof (PEI_FAT_PRIVATE_DATA) - 1) / PEI_FAT_MEMMORY_PAGE_SIZE + 1,
            &Address
            );
  ASSERT (Status == EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData = (PEI_FAT_PRIVATE_DATA *) (UINTN) Address;

  //
  // Initialize Private Data (to zero, as is required by subsequent operations)
  //
  ZeroMem ((UINT8 *) PrivateData, sizeof (PEI_FAT_PRIVATE_DATA));

  PrivateData->Signature = PEI_FAT_PRIVATE_DATA_SIGNATURE;

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
  // Other initializations
  //
  PrivateData->BlockDeviceCount = 0;
  PrivateData->VolumeCount = 0;
  PrivateData->CapsuleCount = 0;
  FatVirtualBlockIoRecord.Count = 0;

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
  PEI_FAT_PRIVATE_DATA  *PrivateData;

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

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
  EFI_STATUS            Status;
  PEI_FAT_PRIVATE_DATA  *PrivateData;
  UINTN                 NumberRecoveryCapsules;
  UINTN                 BlockDeviceNo;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

  *Size = PrivateData->CapsuleData[CapsuleInstance - 1].FileSize;

  //
  // Find corresponding physical block device
  //
  BlockDeviceNo = PrivateData->CapsuleData[CapsuleInstance - 1].Volume->BlockDeviceNo;
  while (PrivateData->BlockDevice[BlockDeviceNo].Logical && BlockDeviceNo < PrivateData->BlockDeviceCount) {
    BlockDeviceNo = PrivateData->BlockDevice[BlockDeviceNo].ParentDevNo;
  }
  //
  // Fill in the Capsule Type GUID according to the block device type
  //
  if (BlockDeviceNo < PrivateData->BlockDeviceCount) {

    switch (PrivateData->BlockDevice[BlockDeviceNo].DevType) {
    case LegacyFloppy:
      CopyGuid (CapsuleType, &gRecoveryOnFatFloppyDiskGuid);
      break;

    case UsbMassStorage:
      CopyGuid (CapsuleType, &gRecoveryOnFatUsbDiskGuid);
      break;

    //
    // we want add ATA HDD support, but PI1.2 spec. doesn't define hdd
    // type. So for campatibility, we use MaxDeviceType as Hdd type
    //
    case MaxDeviceType:
      CopyGuid (CapsuleType, &gRecoveryOnFatIdeDiskGuid);
      break;

    default:
      break;
    }
  }

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
  EFI_STATUS            Status;
  PEI_FAT_PRIVATE_DATA  *PrivateData;
  UINTN                 NumberRecoveryCapsules;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

  Status = FatReadFile (
            PrivateData,
            &PrivateData->CapsuleData[CapsuleInstance - 1],
            (UINTN)PrivateData->CapsuleData[CapsuleInstance - 1].FileSize,
            Buffer
            );
  return Status;
}




CHAR16 CharToUpper(IN CHAR16 Char)
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}


INTN
StrCmpNoCase (
  IN      CONST CHAR16  *FirstString,
  IN      CONST CHAR16  *SecondString
  )
{
  CHAR16  a = 0, b = 0;

  while(1){
    a = CharToUpper(*FirstString);
    b = CharToUpper(*SecondString);

    if(a == 0 || a != b){
      break;
    }
    
    FirstString++;
    SecondString++;

  }  

  return a - b;

}




BOOLEAN
IsFdFile (
  IN PEI_FAT_FILE          *File
  )
{
  EFI_STATUS  Status;
  CHAR16      *BiosFileExt;
  CHAR16      FileNameExt[FAT_MAX_FILE_EXTENSION_LENGTH];
  CHAR16      *TargetFileName;  


  TargetFileName = (CHAR16*)PcdGetPtr(PcdBiosRecoveryFileName);
  if(TargetFileName[0] != 0){
    if(StrCmpNoCase(TargetFileName, File->FileName) == 0){
      return TRUE;
    } else {
      return FALSE;
    }
  }  

  //
  // check if file name has a ".fd" or ".bin" extend
  //
  ZeroMem (FileNameExt, sizeof(FileNameExt));
  Status = GetFileExt (File->FileName, FileNameExt);
  if (EFI_ERROR(Status)) {
    return FALSE;
  }
  BiosFileExt = (CHAR16 *)PcdGetPtr (PcdBiosFileExt);
  if (!EngStriColl(BiosFileExt, FileNameExt)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Finds the recovery file on a FAT volume.
  This function finds the the recovery file named FileName on a specified FAT volume and returns
  its FileHandle pointer.

  @param  PrivateData             Global memory map for accessing global
                                  variables.
  @param  VolumeIndex             The index of the volume.
  @param  Handle                  The output file handle.

  @retval EFI_DEVICE_ERROR        Some error occured when operating the FAT
                                  volume.
  @retval EFI_NOT_FOUND           The recovery file was not found.
  @retval EFI_SUCCESS             The recovery file was successfully found on the
                                  FAT volume.

**/
EFI_STATUS
FindRecoveryFile (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 VolumeIndex,
  IN OUT UINT8              *NoFile,
  OUT PEI_FAT_FILE          **FileBuffer
  )
{
  EFI_STATUS        Status;
  PEI_FAT_FILE      Parent;
  PEI_FAT_FILE      *File;
  STATIC PEI_FAT_FILE      Buffer[PEI_FAT_MAX_FILES_IN_VOLUME];

  File = &PrivateData->File;

  //
  // VolumeIndex must be less than PEI_FAT_MAX_VOLUME because PrivateData->VolumeCount
  // cannot be larger than PEI_FAT_MAX_VOLUME when detecting recovery volume.
  //
  ASSERT (VolumeIndex < PEI_FAT_MAX_VOLUME);

  //
  // Construct root directory file
  //
  Parent.IsFixedRootDir   = (BOOLEAN) ((PrivateData->Volume[VolumeIndex].FatType == Fat32) ? FALSE : TRUE);
  Parent.Attributes       = FAT_ATTR_DIRECTORY;
  Parent.CurrentPos       = 0;
  Parent.CurrentCluster   = Parent.IsFixedRootDir ? 0 : PrivateData->Volume[VolumeIndex].RootDirCluster;
  Parent.StartingCluster  = Parent.CurrentCluster;
  Parent.Volume           = &PrivateData->Volume[VolumeIndex];

  Status                  = FatSetFilePos (PrivateData, &Parent, 0);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Search for recovery capsule in root directory
  //
  Status = FatReadNextDirectoryEntry (PrivateData, &Parent, File);
  while (Status == EFI_SUCCESS) {
    if (IsFdFile (File)) {
      if (*NoFile < PEI_FAT_MAX_FILES_IN_VOLUME) {
        CopyMem ((UINT8 *)&Buffer[(*NoFile)], (UINT8 *)File, sizeof(PEI_FAT_FILE));
        (*NoFile) ++;
      } else {
        break;
      }
    }

    Status = FatReadNextDirectoryEntry (PrivateData, &Parent, File);
  }

  if (EFI_ERROR (Status) && *NoFile ==  0) {
    return EFI_NOT_FOUND;
  }

  *FileBuffer = Buffer;

  return EFI_SUCCESS;

}
