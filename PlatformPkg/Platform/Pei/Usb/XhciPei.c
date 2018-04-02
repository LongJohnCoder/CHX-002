
#include <PlatformDefinition.h>
#include <UsbController.h>
#include <IndustryStandard/Pci.h>
#include <Library/TimerLib.h>
#include <Library/PlatformCommLib.h>


STATIC
EFI_STATUS
  // ARL-20171023-02
// GetXhciFwFromFv (
//   OUT VOID   **pHubFile,
//   OUT UINT32  *pHubSize,
//   OUT VOID   **pMcuFile,
//   OUT UINT32  *pMcuSize
//   )
GetXhciFwFromFv (
  OUT VOID   **pMcuFile,
  OUT UINT32  *pMcuSize
  )
{
  EFI_STATUS                 Status;
  UINTN                      Instance;
  EFI_PEI_FV_HANDLE          VolumeHandle;
  EFI_PEI_FILE_HANDLE        FileHandle;
  VOID                       *McuFile;
  // VOID                       *HubFile;   // ARL-20171023-02
  BOOLEAN                    HasFound;
  UINT32                     McuSize;
  // UINT32                     HubSize;   // ARL-20171023-02
  EFI_PHYSICAL_ADDRESS       Address;

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Instance = 0;
  HasFound = FALSE;
  McuFile  = NULL;
  // HubFile  = NULL;   // ARL-20171023-02

  // ASSERT(pHubFile!=NULL && pMcuFile!=NULL);   // ARL-20171023-02
  ASSERT(pMcuFile!=NULL);

  while (1) {
    Status = PeiServicesFfsFindNextVolume(Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    if((UINT32)(UINTN)VolumeHandle >= PcdGet32(PcdFlashFvMainBase)){
      goto NextCycle;
    }

    Status = PeiServicesFfsFindFileByName((EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile), VolumeHandle, &FileHandle);
    if(EFI_ERROR(Status)){
      goto NextCycle;
    }

    Status = PeiServicesFfsFindSectionData(EFI_SECTION_RAW, FileHandle, &McuFile);
    ASSERT_EFI_ERROR(Status);
    // ARL-20171023-02
    // Status = PeiServicesFfsFindFileByName((EFI_GUID*)PcdGetPtr(PcdXhciHubFwFile), VolumeHandle, &FileHandle);
    // ASSERT_EFI_ERROR(Status);
    // Status = PeiServicesFfsFindSectionData(EFI_SECTION_RAW, FileHandle, &HubFile);
    // ASSERT_EFI_ERROR(Status);
    // ARL-20171023-02
    HasFound = TRUE;
    break;

NextCycle:
    Instance++;
  }

  if(!HasFound){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  McuSize  = *(UINT32*)(&((EFI_COMMON_SECTION_HEADER*)((UINT8*)McuFile - sizeof(EFI_COMMON_SECTION_HEADER)))->Size);
  McuSize &= 0xFFFFFF;
  McuSize -= sizeof(EFI_COMMON_SECTION_HEADER);
  // ARL-20171023-02
  // HubSize  = *(UINT32*)(&((EFI_COMMON_SECTION_HEADER*)((UINT8*)HubFile - sizeof(EFI_COMMON_SECTION_HEADER)))->Size);
  // HubSize &= 0xFFFFFF;
  // HubSize -= sizeof(EFI_COMMON_SECTION_HEADER);
  // ARL-20171023-02

  // DEBUG((EFI_D_INFO, "Mcu(%X,%X), Hub(%X,%X)\n", McuFile, McuSize, HubFile, HubSize));   // ARL-20171023-02
  DEBUG((EFI_D_INFO, "Mcu(%X,%X)\n", McuFile, McuSize));

  Status = PeiServicesAllocatePages(EfiBootServicesData, EFI_SIZE_TO_PAGES(McuSize+0x10000), &Address);
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_INFO, "(L%d)Address:%lX\n", __LINE__, Address));
  Address = ALIGN_VALUE(Address, 0x10000);
  CopyMem((VOID*)(UINTN)Address, McuFile, McuSize);
  McuFile = (VOID*)(UINTN)Address;
  // ARL-20171023-02
  // Status = PeiServicesAllocatePages(EfiBootServicesData, EFI_SIZE_TO_PAGES(HubSize+0x10000), &Address);
  // ASSERT_EFI_ERROR(Status);
  // DEBUG((EFI_D_INFO, "(L%d)Address:%lX\n", __LINE__, Address));
  // Address = ALIGN_VALUE(Address, 0x10000);
  // CopyMem((VOID*)(UINTN)Address, HubFile, HubSize);
  // HubFile = (VOID*)(UINTN)Address;
  // ARL-20171023-02

  // *pHubFile = HubFile;   // ARL-20171023-02
  // *pHubSize = HubSize;   // ARL-20171023-02
  *pMcuFile = McuFile;
  *pMcuSize = McuSize;

ProcExit:
  DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, Status));
  return Status;
}


EFI_STATUS
HandleXhciFwForRecovery ()
{
  EFI_STATUS          Status;
  VOID                *McuFile;
  // VOID                *HubFile;    // ARL-20171023-02
  UINT32              McuSize;
  // UINT32              HubSize;    // ARL-20171023-02

  if(MmioRead16(XHCI_PCI_REG(PCI_VENDOR_ID_OFFSET)) == 0xFFFF){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  // Status = GetXhciFwFromFv(&HubFile, &HubSize, &McuFile, &McuSize);    // ARL-20171023-02
  Status = GetXhciFwFromFv(&McuFile, &McuSize);
  ASSERT_EFI_ERROR(Status);
  // Status = LoadXhciFw(HubFile, McuFile);    // ARL-20171023-02
  Status = LoadXhciFw(McuFile);
  ASSERT_EFI_ERROR(Status);

ProcExit:
  return Status;
}

