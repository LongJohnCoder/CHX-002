/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MediaDeviceDriver.c

Abstract: 
  SD card Model.

Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-16
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE: SD card feature support.
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/
#include "MediaDeviceDriver.h"
#include "SdLegacy.h"
extern SD_MEMORY_CARD_INFO                 SdData[5];
extern UINT8                     SdmemCardQty;
//------------------------------------------------------------------------------
// DEBUG helper
//------------------------------------------------------------------------------
int            _inp (unsigned short port);
int            _outp (unsigned short port, int databyte );
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)

/**
  Starting the Media Device Driver.

  @param  SDHostIo                A pointer to EFI_BLOCK_IO_PROTOCOL.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Unsupport.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
MediaDeviceDriverStart (
  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo
  )
{
  EFI_STATUS                Status;
  CARD_DATA                 *CardData;
  UINTN                     Loop;

  CardData = NULL;
  Status = SdHostIo->DetectCardAndInitHost (SdHostIo);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //CardData = (CARD_DATA*)AllocateZeroPool(sizeof (CARD_DATA));
  Status = gBS->AllocatePool (
                  EfiReservedMemoryType,
                  sizeof (CARD_DATA),
                  (VOID **) &CardData
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((EFI_D_ERROR, "MediaDevice Fail to AllocateZeroPool(CARD_DATA) \n"));
    goto Exit;
  }
  ZeroMem (CardData, sizeof (CARD_DATA));

  ASSERT (SdHostIo->HostCapability.BoundarySize >= 4 * 1024);
  CardData->RawBufferPointer = (UINT8*)(UINTN)0xffffffff;
  //Allocated the buffer under 4G
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize),
                  (EFI_PHYSICAL_ADDRESS *)(&(CardData->RawBufferPointer))
                  );
  DEBUG ((EFI_D_ERROR, "CardData->RawBufferPointer = 0x%x \n",CardData->RawBufferPointer));
  if (!EFI_ERROR (Status)) {
    CardData->RawBufferPointer = ZeroMem (CardData->RawBufferPointer, 2 * SdHostIo->HostCapability.BoundarySize);
  } else {
    DEBUG ((EFI_D_ERROR, "MediaDeviceDriverBindingStart: Fail to AllocateZeroPool(2*x) \n"));
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CardData->AlignedBuffer = CardData->RawBufferPointer - ((UINTN)(CardData->RawBufferPointer) & (SdHostIo->HostCapability.BoundarySize - 1)) + SdHostIo->HostCapability.BoundarySize;

  CardData->Signature = CARD_DATA_SIGNATURE;
  CardData->SdHostIo  = SdHostIo;

  for (Loop = 0; Loop < MAX_NUMBER_OF_PARTITIONS; Loop++) {
    CardData->Partitions[Loop].Signature = CARD_PARTITION_SIGNATURE;
    CardData->Partitions[Loop].CardData = CardData;
  }

  Status = MmcSdCardInit (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDevice Fail to MMCSDCardInit \n"));
    goto Exit;
  }
  DEBUG ((EFI_D_INFO, "MediaDevice MMC SD card\n"));
  Status = MmcSdBlockIoInit (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDevice Card BlockIo init failed\n"));
    goto Exit;
  }

  SdData[SdmemCardQty].SDCardData = CardData;
  SdmemCardQty ++;
  
Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MediaDevice End with failure\n"));
    if (CardData != NULL) {
      if (CardData->RawBufferPointer != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)(CardData->RawBufferPointer), EFI_SIZE_TO_PAGES(2 * SdHostIo->HostCapability.BoundarySize));
      }
      gBS->FreePool (CardData);
    }
  }
  return Status;
}


