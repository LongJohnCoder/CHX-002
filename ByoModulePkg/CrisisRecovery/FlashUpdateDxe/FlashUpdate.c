/*++

Module Name:

  FlashUpdate.c

Abstract:

  This file contains flash update functions when system is under
  recovery mode or flash update mode.

--*/
#include "FlashUpdate.h"
#include <Protocol\NvMediaAccess.h>
#include <Pi\PiHob.h>
#include <Library\UefiBootServicesTableLib.h>
#include <Library\HobLib.h>
#include <Library\DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi\UefiSpec.h>
#include <Protocol\Spi.h>
#include <Protocol\FirmwareVolumeBlock.h>
#include <Library\BaseLib.h>
#include <Library\MemoryAllocationLib.h>
#include <Uefi\UefiInternalFormRepresentation.h>
#include <Library\PcdLib.h>
#include <Library\BaseMemoryLib.h>
#include <Protocol\SimpleTextIn.h>

extern FIRMWARE_READ_TOP_SWAP_PROTOCOL* mTopSwapper;
NV_MEDIA_ACCESS_PROTOCOL                *gMediaAccessProtocol;
MEDIA_BLOCK_MAP                         *gMapInfo;

typedef struct {
  CHAR16 FvString[27];
  UINT16 Mask;
  UINTN  BaseAddress;
  UINTN  SizeOfBlocks;
} UpdateFvStruct;

UpdateFvStruct mUpdateFvList[] = {
  {
    L"Top Swap Block............",
    BOOTBLOCK_UPDATE,
    0,  // PcdGet32 (PcdFlashFvRecoveryBase) - PcdGet32 (PcdFlashFvRecoverySize),
    0   // PcdGet32 (PcdFlashFvRecoverySize)
  },
  {
    L"Recovery2 Backup Block....",
    BOOTBLOCK_UPDATE,
    0,  // PcdGet32 (PcdFlashFvRecovery2BackupBase),
    0   // PcdGet32 (PcdFlashFvRecovery2BackupSize)
  },
  {
    L"MicroCode Backup..........",
    MICROCODE_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageMicrocodeBackupBase),
    0   // PcdGet32 (PcdFlashNvStorageMicrocodeSize)
  },
  {
    L"Recovery Block............",
    BOOTBLOCK_UPDATE,
    0,  // PcdGet32 (PcdFlashFvRecoveryBase),
    0   // PcdGet32 (PcdFlashFvRecoverySize)
  },
  {
    L"Recovery2 Block...........",
    BOOTBLOCK_UPDATE,
    0,  // PcdGet32 (PcdFlashFvRecovery2Base),
    0   // PcdGet32 (PcdFlashFvRecovery2Size)
  },
  {
    L"Test Menu.................",
    TEST_MENU_UPDATE,
    0,  // PcdGet32 (PcdFlashTestMenuBase),
    0   // PcdGet32 (PcdFlashTestMenuSize)
  },
  {
    L"Variable..................",
    NVSTORAGE_VARIABLE_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageVariableBase),
    0   // PcdGet32 (PcdFlashNvStorageVariableSize)
  },
  {
    L"MicroCode.................",
    MICROCODE_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageMicrocodeBase),
    0   // PcdGet32 (PcdFlashNvStorageMicrocodeSize)
  },
  {
    L"Event Log.................",
    FVMAIN_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageEventLogBase),
    0   // PcdGet32 (PcdFlashNvStorageEventLogSize)
  },
  {
    L"FtwWorking................",
    NVSTORAGE_FTW_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageFtwWorkingBase),
    0   // PcdGet32 (PcdFlashNvStorageFtwWorkingSize)
  },
  {
    L"FtwWorking Spare..........",
    NVSTORAGE_FTW_SPARE_UPDATE,
    0,  // PcdGet32 (PcdFlashNvStorageFtwSpareBase),
    0   // PcdGet32 (PcdFlashNvStorageFtwSpareSize)
  },
  {
    L"MrcNormal.................",
    FVMAIN_UPDATE,
    0,  // PcdGet32 (PcdFlashFvMrcNormalBase)
    0   // PcdGet32 (PcdFlashFvMrcNormalSize)
  },
  {
    L"Oem Logo..................",
    FVMAIN_UPDATE,
    0,  // PcdGet32 (PcdFvOemBase)
    0   // PcdGet32 (PcdFvOemSize)
  },
  {
    L"FvMain....................",
    FVMAIN_UPDATE,
    0,  // PcdGet32 (PcdFlashFvMainBase),
    0   // PcdGet32 (PcdFlashFvMainSize)
  }
};

EFI_STATUS
ReadKeyStroke (
  IN OUT EFI_INPUT_KEY      *Key
  );

/**
  Reset system according to a input attribute

  @param  Attribute             a value to determine what type reset
                                system will do

  @return  VOID

**/
VOID
ResetSystemByAttribute (
  UINT16        Attribute
  )
{
  EFI_STATUS        Status;
  UINT16            ResetType;
  EFI_INPUT_KEY     Key;
  UINTN             Index;

  ResetType = Attribute & POWER_MARK;
  switch (ResetType) {
  case SHUT_DOWN:
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);	
    Print (L"\rShutdown and Make Bios Configuration Jumper In Normal Mode.\n");
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);    
    Print (L"\rPress Any Key to Shutdown...\n");
    while (gST->ConIn != NULL) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);    	
      if (!EFI_ERROR (Status)) {      
        gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
      }
      gBS->Stall (1000 * 100);
    }    
    break;

  case RESET_COLD:
    for (Index = 10; Index > 0; Index --) {
      Status = ReadKeyStroke (&Key);
      if (!EFI_ERROR(Status)) {
        //
        // if ESC key, clean message and go to deadloop
        //
        if (Key.ScanCode == 0x0017) {
          Print (L"\r                                                  ");
          goto DeadLoop;
        }
        break;
      }
      Print (L"\rSystem will do reset in");
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);
      Print (L" %d ", Index);
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
      Print (L"seconds...");
      gBS->Stall (1000 * 1000);
    }
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    break;

  case RESET_WARM:
    for (Index = 10; Index > 0; Index --) {
      Status = ReadKeyStroke (&Key);
      if (!EFI_ERROR(Status)) {
        //
        // if ESC key, clean message and go to deadloop
        //
        if (Key.ScanCode == 0x0017) {
          Print (L"\r                                                  ");
          goto DeadLoop;
        }
        break;
      }
      Print (L"\rSystem will do reset in");
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);
      Print (L" %d ", Index);
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
      Print (L"seconds..");
      gBS->Stall (1000 * 1000);
    }
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    break;
  default:
    break;
  }

DeadLoop:
  CpuDeadLoop ();
}

EFI_STATUS
VerifyFv (
  IN UINTN                FlashAreaBaseAddress,
  IN UINTN                FdAreaBaseAddress,
  IN UINTN                Length,
  IN UINTN                BlockSize,
  IN EFI_PHYSICAL_ADDRESS Buffer
  )
{
  UINTN             Offset;
  UINTN             Index;
  EFI_STATUS        Status;

  for (Offset = 0; Offset < Length; Offset += BlockSize) {
    Print( L"\r   Verifying..............................%x", FlashAreaBaseAddress + Offset);
    Status = gMediaAccessProtocol->Read (
                                     gMediaAccessProtocol,
                                     FlashAreaBaseAddress,
                                     (UINT8 *)(UINTN)Buffer,
                                     &BlockSize,
                                     SPI_MEDIA_TYPE
                                     );
    ASSERT_EFI_ERROR(Status);
    for (Index = 0; Index < BlockSize; Index += sizeof(UINTN)) {
      if (*(UINTN *)(UINTN)(Buffer + Index) != *(UINTN *)(FdAreaBaseAddress + Index)) {
        Print( L"\r   Verifying..............................Error! (0x%x)\n", FlashAreaBaseAddress + Offset + Index );
        return EFI_DEVICE_ERROR;
      }
    }
  }
  return EFI_SUCCESS;
}

/**
  Read flash from mapped memory and verify it with a input BIOS image

  @param  UpdateBehavior        Determine which flash area is updated
  @param  FDImageBaseAddress    A physical address where a BIOS image
                                stay

  @return  EFI_DEVICE_ERROR     Find some flash area not match with the
                                BIOS image
  @return  EFI_SUCCESS          Verify ok

**/
EFI_STATUS
ReadFlashAndVerify (
  INT16                   Behavior,
  EFI_PHYSICAL_ADDRESS    FDImageBaseAddress
  )
{
  EFI_STATUS            Status;
  UINTN                 Length;
  UINTN                 FlashAreaBaseAddress;
  UINTN                 FdAreaBaseAddress;
  UINTN                 BlockSize;
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Buffer;
  UINTN                 Index;

  BlockSize = (UINTN)1<<gMapInfo->Size;
  Pages = EFI_SIZE_TO_PAGES (BlockSize);

  Status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, Pages, &Buffer);
  ASSERT_EFI_ERROR (Status);

  for (Index = 1; Index < sizeof (mUpdateFvList) / sizeof (UpdateFvStruct); Index ++) {
    if (mUpdateFvList[Index].SizeOfBlocks == 0 || mUpdateFvList[Index].BaseAddress == 0)
      continue;
    if (Behavior & mUpdateFvList[Index].Mask) {
      FlashAreaBaseAddress = mUpdateFvList[Index].BaseAddress;
      Length               = mUpdateFvList[Index].SizeOfBlocks;
      FdAreaBaseAddress    = (UINTN)FDImageBaseAddress + mUpdateFvList[Index].BaseAddress - PcdGet32 (PcdFlashAreaBaseAddress);
      Status = VerifyFv (FlashAreaBaseAddress, FdAreaBaseAddress, Length, BlockSize, Buffer);
    }
  }
  if (!EFI_ERROR (Status)) {
    Print( L"\r   Verifying..............................Successed!\n");
  }
  gBS->FreePages(Buffer, Pages);

  return Status;
}

/**
  This is a internal function to show ByoSoft copyrights ans warning messages

  @param BootMode         Indicate which mode system is under

  @retval EFI_SUCCESS     Show copyrights successfully

**/
EFI_STATUS
ShowCopyRightsAndWarning (
  IN EFI_BOOT_MODE      BootMode
)
{
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
  //
  // Show Byosoft copyrights!
  //
  Print(L"              **************************************************************\n");
  Print(L"              *                   Byosoft Flash Update                     *\n");
  Print(L"              *         Copyright(C) 2006-2015, Byosoft Co.,Ltd.           *\n");
  Print(L"              *                   All rights reserved                      *\n");
  Print(L"              **************************************************************\n");
  //
  // Show Warnings!
  //
  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    Print(L"Warning: System is in Recovery Mode. Please don't shutdown system during erasing/programming flash!\n");
  } else if (BootMode == BOOT_ON_FLASH_UPDATE) {
    Print(L"Warning: System is in Flash Update Mode. Please don't shutdown system during erasing/programming flash!\n");
  }

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_WHITE | EFI_BRIGHT);

  return EFI_SUCCESS;
}

EFI_STATUS
ProcessFv (
  IN UINT8  ProgramFlag,
  IN UINTN  BaseAddress,
  IN UINTN  NumberOfBlocks,
  IN UINTN  BlockSize,
  IN UINTN  FVSouceBaseAddress,
  IN CHAR16 *FvString
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

/*
  DEBUG((EFI_D_INFO, "%a(%X,%X,%X,%X,%s)\n", \
                       __FUNCTION__,   \
                       BaseAddress,    \
                       NumberOfBlocks, \
                       BlockSize,      \
                       FVSouceBaseAddress, \
                       FvString
                       ));	
*/
  if (ProgramFlag & BIT0) {
    for (Index = 0; Index < NumberOfBlocks; Index ++) {
      Status = gMediaAccessProtocol->Erase (
                                       gMediaAccessProtocol,
                                       (BaseAddress + Index * BlockSize),
                                       BlockSize,
                                       SPI_MEDIA_TYPE);
      ASSERT(!EFI_ERROR(Status));
      Print( L"\r   Erasing %s.....%x", FvString, (BaseAddress + (Index + 1) * BlockSize));
    }
    Print( L"\r   Erasing %s.....Successed!", FvString);
  }

  if (ProgramFlag & BIT1) {
    for (Index = NumberOfBlocks; Index != 0 ; Index --) {
      Status = gMediaAccessProtocol->Write (
                                       gMediaAccessProtocol,
                                       (BaseAddress + (Index - 1) * BlockSize),
                                       (UINT8 *)(UINTN)(FVSouceBaseAddress + (Index - 1) * BlockSize),
                                       BlockSize,
                                       SPI_MEDIA_TYPE);
      ASSERT(!EFI_ERROR(Status));
      Print( L"\r   Programming %s.%x  ", FvString, (BaseAddress + Index * BlockSize));
    }
    Print( L"\r   Programming %s.Successed!\n", FvString);
  }
  return EFI_SUCCESS;
}

/**
  This procedure is used to update flash part and shutdown
  system by behavior parameter

  @param  BootMode              Indicate the system boot mode
  @param  Behavior              Indicate which fv part will be updated and
                                do which system reset type
  @param  FDImageBaseAddress    New BIOS image base address
  @param  FDImageLength         New BIOS image length

  @return  EFI_SUCCESS          Flash is updated successfully

**/
EFI_STATUS
FlashUpdate (
  EFI_BOOT_MODE           BootMode,
  UINT16                  Behavior,
  EFI_PHYSICAL_ADDRESS    FDImageBaseAddress,
  UINT64                  FDImageLength
  )
{
  EFI_STATUS                       Status;
  UINTN                            BlockSize;
  UINTN                            NumberOfBlocks;
  UINTN                            BaseAddress;
  EFI_PHYSICAL_ADDRESS             FVSouceBaseAddress;
  BOOLEAN                          TopSwapState;
  UINTN                            Index;
  UINT8                            ProgramFlag;

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_WHITE | EFI_BRIGHT);
  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  ShowCopyRightsAndWarning (BootMode);

  //
  // NvMediaAccess protocol
  //
  Status = gBS->LocateProtocol (
             &gEfiNvMediaAccessProtocolGuid,
             NULL,
             &gMediaAccessProtocol);
  ASSERT(!EFI_ERROR(Status));

  //
  // Get flash device info
  //
  gMapInfo = NULL;
  Status = gMediaAccessProtocol->Info (
             gMediaAccessProtocol,
             &gMapInfo,
             SPI_MEDIA_TYPE);
  ASSERT(!EFI_ERROR(Status));
  BlockSize = (UINTN)1<<gMapInfo->Size;
  NumberOfBlocks = (UINTN)gMapInfo->Count;
  //DEBUG((EFI_D_INFO, "0x%X * %d\n", BlockSize, NumberOfBlocks));	
  //
  // init mUpdateFvList
  //
  if (PcdGet8 (PcdHasBackupBios) == 0) {
    mUpdateFvList[0].BaseAddress   = PcdGet32 (PcdFlashFvRecoveryBase) - PcdGet32 (PcdFlashFvRecoverySize);
    mUpdateFvList[0].SizeOfBlocks  = PcdGet32 (PcdFlashFvRecoverySize);
  }
  mUpdateFvList[1].BaseAddress   = PcdGet32 (PcdFlashFvRecovery2BackupBase);
  mUpdateFvList[1].SizeOfBlocks  = PcdGet32 (PcdFlashFvRecovery2BackupSize);
  mUpdateFvList[2].BaseAddress   = PcdGet32 (PcdFlashNvStorageMicrocodeBackupBase);
  mUpdateFvList[2].SizeOfBlocks  = PcdGet32 (PcdFlashNvStorageMicrocodeSize);
  mUpdateFvList[3].BaseAddress   = PcdGet32 (PcdFlashFvRecoveryBase);
  mUpdateFvList[3].SizeOfBlocks  = PcdGet32 (PcdFlashFvRecoverySize);
  mUpdateFvList[4].BaseAddress   = PcdGet32 (PcdFlashFvRecovery2Base);
  mUpdateFvList[4].SizeOfBlocks  = PcdGet32 (PcdFlashFvRecovery2Size);
  mUpdateFvList[5].BaseAddress   = PcdGet32 (PcdFlashTestMenuBase);
  mUpdateFvList[5].SizeOfBlocks  = PcdGet32 (PcdFlashTestMenuSize);
  mUpdateFvList[6].BaseAddress   = PcdGet32 (PcdFlashNvStorageVariableBase);
  mUpdateFvList[6].SizeOfBlocks  = PcdGet32 (PcdFlashNvStorageVariableSize);
  mUpdateFvList[7].BaseAddress   = PcdGet32 (PcdFlashNvStorageMicrocodeBase);
  mUpdateFvList[7].SizeOfBlocks  = PcdGet32 (PcdFlashNvStorageMicrocodeSize);
  mUpdateFvList[8].BaseAddress   = PcdGet32 (PcdFlashNvStorageEventLogBase);
  mUpdateFvList[8].SizeOfBlocks  = PcdGet32 (PcdFlashNvStorageEventLogSize);
  mUpdateFvList[9].BaseAddress   = PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  mUpdateFvList[9].SizeOfBlocks  = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  mUpdateFvList[10].BaseAddress  = PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  mUpdateFvList[10].SizeOfBlocks = PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  mUpdateFvList[11].BaseAddress  = PcdGet32 (PcdFlashFvMrcNormalBase);
  mUpdateFvList[11].SizeOfBlocks = PcdGet32 (PcdFlashFvMrcNormalSize);
  mUpdateFvList[12].BaseAddress  = PcdGet32 (PcdFvOemBase);
  mUpdateFvList[12].SizeOfBlocks = PcdGet32 (PcdFvOemSize);
  mUpdateFvList[13].BaseAddress  = PcdGet32 (PcdFlashFvMainBase);
  mUpdateFvList[13].SizeOfBlocks = PcdGet32 (PcdFlashFvMainSize);

  if (mTopSwapper != NULL) {
    //
    // Get A16 status
    //
    Status = mTopSwapper->GetSwap (mTopSwapper, &TopSwapState);
    ASSERT_EFI_ERROR (Status);
    if (!TopSwapState) {
      ProgramFlag = BIT0 | BIT1;
      for (Index = 0; Index < 3; Index ++) {
        if (mUpdateFvList[Index].SizeOfBlocks == 0 || mUpdateFvList[Index].BaseAddress == 0)
          continue;
        if (Behavior & mUpdateFvList[Index].Mask) {
          BaseAddress        = mUpdateFvList[Index].BaseAddress;
          NumberOfBlocks     = mUpdateFvList[Index].SizeOfBlocks / BlockSize;
          FVSouceBaseAddress = FDImageBaseAddress + mUpdateFvList[Index].BaseAddress - PcdGet32 (PcdFlashAreaBaseAddress);
          if (Index == 0)
            FVSouceBaseAddress += mUpdateFvList[Index].SizeOfBlocks;
          ProcessFv (ProgramFlag, BaseAddress, NumberOfBlocks, BlockSize, (UINTN)FVSouceBaseAddress, mUpdateFvList[Index].FvString);
        }
      }
      //
      // Invert A16
      //
      Status = mTopSwapper->SetSwap (mTopSwapper, TRUE);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }
  // Erase FvMain first
  Index = 13;
  if (Behavior & mUpdateFvList[Index].Mask) {
    BaseAddress        = mUpdateFvList[Index].BaseAddress;
    NumberOfBlocks     = mUpdateFvList[Index].SizeOfBlocks / BlockSize;
    FVSouceBaseAddress = FDImageBaseAddress + mUpdateFvList[Index].BaseAddress - PcdGet32 (PcdFlashAreaBaseAddress);
    ProgramFlag = BIT0;
    ProcessFv (ProgramFlag, BaseAddress, NumberOfBlocks, BlockSize, (UINTN)FVSouceBaseAddress, mUpdateFvList[Index].FvString);
  }

  ProgramFlag = BIT0 | BIT1;
  for (Index = 3; Index < sizeof (mUpdateFvList) / sizeof (UpdateFvStruct) - 1; Index ++) {  
    if (mUpdateFvList[Index].SizeOfBlocks == 0 || mUpdateFvList[Index].BaseAddress == 0)
      continue;
    if (Behavior & mUpdateFvList[Index].Mask) {
      BaseAddress        = mUpdateFvList[Index].BaseAddress;
      NumberOfBlocks     = mUpdateFvList[Index].SizeOfBlocks / BlockSize;
      FVSouceBaseAddress = FDImageBaseAddress + mUpdateFvList[Index].BaseAddress - PcdGet32 (PcdFlashAreaBaseAddress);
      ProcessFv (ProgramFlag, BaseAddress, NumberOfBlocks, BlockSize, (UINTN)FVSouceBaseAddress, mUpdateFvList[Index].FvString);
    }
  }

  // Promgram FvMain last
  Index = 13;
  if (Behavior & mUpdateFvList[Index].Mask) {
    BaseAddress        = mUpdateFvList[Index].BaseAddress;
    NumberOfBlocks     = mUpdateFvList[Index].SizeOfBlocks / BlockSize;
    FVSouceBaseAddress = FDImageBaseAddress + mUpdateFvList[Index].BaseAddress - PcdGet32 (PcdFlashAreaBaseAddress);
    ProgramFlag = BIT1;
    ProcessFv (ProgramFlag, BaseAddress, NumberOfBlocks, BlockSize, (UINTN)FVSouceBaseAddress, mUpdateFvList[Index].FvString);
  }

  return EFI_SUCCESS;
}
