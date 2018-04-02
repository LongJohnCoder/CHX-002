/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MMCSDBlockIo.c

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
/*++
  This file contains an 'Intel Peripheral Driver' and uniquely
  identified as Intel "Mobile Silicon Support Code" and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/** @file

Copyright (c) 2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


File Name:

  MMCSDBlockIo.c

Abstract:

   Block I/O protocol for MMC/SD device

**/
#include "MediaDeviceDriver.h"
#include <Library/BaseLib.h>

/**
  Implements EFI_BLOCK_IO_PROTOCOL.Reset() function.

  @param  This                    The EFI_BLOCK_IO_PROTOCOL instance.
  @param  ExtendedVerification    Indicates that the driver may perform a more exhaustive
                                  verification operation of the device during reset.
                                  (This parameter is ingored in this driver.)

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.

**/
EFI_STATUS
EFIAPI
MmcSdBlockReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  CARD_DATA                  *CardData;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  EFI_STATUS                 Status = EFI_SUCCESS;

  CardData  = CARD_DATA_FROM_THIS(This);
  SdHostIo = CardData->SdHostIo;

  DEBUG ((EFI_D_INFO, "MMC SD Block: Resetting host\n"));
  //
  // EFI 2.3.1 spec specifies this return status only for "EFI_DEVICE_ERROR" and "EFI_SUCCESS"
  // Others are illegal.
  //
  Status = SdHostIo->ResetSdHost (SdHostIo, Reset_DAT_CMD);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
 }

/**
  Implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks() function.

  @param  This                    The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                 The media id that the read request is for.
  @param  LBA                     The starting logical block address to read from on the device.
  @param  BufferSize              The size of the Buffer in bytes. This must be a multiple of
                                  the intrinsic block size of the device.
  @param  Buffer                  A pointer to the destination buffer for the data. The caller
                                  is responsible for either having implicit or explicit ownership
                                  of the buffer.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
EFIAPI
MmcSdBlockReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  UINT32                      Address;
  CARD_DATA                   *CardData;
  EFI_SD_HOST_IO_PROTOCOL     *SdHostIo;
  UINT32                      RemainingLength;
  UINT32                      TransferLength;
  UINT8                       *BufferPointer;
  BOOLEAN                     SectorAddressing;
  UINT64                      CardSize;
  MMC_PARTITION_DATA          *Partition;
  UINTN                       TotalBlock;

  DEBUG((EFI_D_INFO, "MMCSDBlockReadBlocks: ReadBlocks ...\n"));

  Status   = EFI_SUCCESS;
  Partition = CARD_PARTITION_DATA_FROM_THIS (This);
  CardData  = Partition->CardData;
  SdHostIo = CardData->SdHostIo;
  //
  // Media ID has high priority that need to be verify first
  //
  if (MediaId != Partition->BlockIoMedia.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  Status = MmcSelectPartition (Partition);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // BufferSize must be a multiple of the intrinsic block size of the device.
  //
  if (ModU64x32 (BufferSize,Partition->BlockIoMedia.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CardSize = MultU64x32 (CardData->BlockNumber, CardData->BlockLen);

  if ((CardData->CardType == SDMemoryCard2High) || (CardSize >= SIZE_2GB)) {
    SectorAddressing = TRUE;
  } else {
    SectorAddressing = FALSE;
  }

  if (SectorAddressing) {
    //
    // Sector Address
    //
    Address = (UINT32)DivU64x32 (MultU64x32 (LBA, Partition->BlockIoMedia.BlockSize), 512);
  } else {
    //
    //Byte Address
    //
    Address  = (UINT32)MultU64x32 (LBA, Partition->BlockIoMedia.BlockSize);
  }


  TotalBlock = (UINTN) DivU64x32 (BufferSize, Partition->BlockIoMedia.BlockSize);
  //
  // Make sure the range to read is valid.
  //
  if (LBA + TotalBlock > Partition->BlockIoMedia.LastBlock + 1) {
    return EFI_INVALID_PARAMETER;
  }

  if (!(Partition->BlockIoMedia.MediaPresent)) {
    return EFI_NO_MEDIA;
  }

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((BufferSize % Partition->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  BufferPointer   = Buffer;
  RemainingLength = (UINT32) BufferSize;

  while (RemainingLength > 0) {
    if ((BufferSize > Partition->BlockIoMedia.BlockSize)) {
      if (RemainingLength > SdHostIo->HostCapability.BoundarySize) {
        TransferLength = SdHostIo->HostCapability.BoundarySize;
      } else {
        TransferLength = RemainingLength;
      }

      if (CardData->CardType == MMCCard) {
        if (!(CardData->ExtCSDRegister.CARD_TYPE & (BIT2 | BIT3))) {
          Status = SendCommand (
                     SdHostIo,
                     SET_BLOCKLEN,
                     Partition->BlockIoMedia.BlockSize,
                     NoData,
                     NULL,
                     0,
                     ResponseR1,
                     TIMEOUT_COMMAND,
                     (UINT32*)&(CardData->CardStatus)
                     );
          if (EFI_ERROR (Status)) {
            break;
          }
        }

        Status = SendCommand (
                   SdHostIo,
                   SET_BLOCK_COUNT,
                   TransferLength / Partition->BlockIoMedia.BlockSize,
                   NoData,
                   NULL,
                   0,
                   ResponseR1,
                   TIMEOUT_COMMAND,
                   (UINT32*)&(CardData->CardStatus)
                   );
        if (EFI_ERROR (Status)) {
          break;
        }
      }

      Status = SendCommand (
                 SdHostIo,
                 READ_MULTIPLE_BLOCK,
                 Address,
                 InData,
                 CardData->AlignedBuffer,
                 TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "MMCSDBlockReadBlocks: READ_MULTIPLE_BLOCK -> Fail\n"));
        break;
      }
    } else {
      if (RemainingLength > Partition->BlockIoMedia.BlockSize) {
        TransferLength = Partition->BlockIoMedia.BlockSize;
      } else {
        TransferLength = RemainingLength;
      }

      Status = SendCommand (
                 SdHostIo,
                 READ_SINGLE_BLOCK,
                 Address,
                 InData,
                 CardData->AlignedBuffer,
                 (UINT32)TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "MMCSDBlockReadBlocks: READ_SINGLE_BLOCK -> Fail\n"));
        break;
      }
    }

    CopyMem (BufferPointer, CardData->AlignedBuffer, TransferLength);

    if (SectorAddressing) {
      //
      // Sector Address
      //
      Address += TransferLength / 512;
    } else {
      //
      //Byte Address
      //
      Address += TransferLength;
    }
    BufferPointer   += TransferLength;
    RemainingLength -= TransferLength;
  }

  if (EFI_ERROR (Status)) {
    if ((CardData->CardType == SDMemoryCard) ||
        (CardData->CardType == SDMemoryCard2)||
        (CardData->CardType == SDMemoryCard2High)) {
         SendCommand (
           SdHostIo,
           STOP_TRANSMISSION,
           0,
           NoData,
           NULL,
           0,
           ResponseR1b,
           TIMEOUT_COMMAND,
           (UINT32*)&(CardData->CardStatus)
           );
    } else {
       SendCommand (
         SdHostIo,
         STOP_TRANSMISSION,
         0,
         NoData,
         NULL,
         0,
         ResponseR1,
         TIMEOUT_COMMAND,
         (UINT32*)&(CardData->CardStatus)
         );
    }
  }

Done:
  DEBUG((EFI_D_INFO, "MMCSDBlockReadBlocks: Status = %r\n", Status));
  return Status;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks() function.

  @param  This                    The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                 The media id that the read request is for.
  @param  LBA                     The starting logical block address to read from on the device.
  @param  BufferSize              The size of the Buffer in bytes. This must be a multiple of
                                  the intrinsic block size of the device.
  @param  Buffer                  A pointer to the destination buffer for the data. The caller
                                  is responsible for either having implicit or explicit ownership
                                  of the buffer.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_NO_MEDIA            No media.
  @retval EFI_MEDIA_CHANGED       Media Change.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.

**/
EFI_STATUS
EFIAPI
MmcSdBlockWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  UINT32                      Address;
  CARD_DATA                   *CardData;
  EFI_SD_HOST_IO_PROTOCOL     *SdHostIo;
  UINT32                      RemainingLength;
  UINT32                      TransferLength;
  UINT8                       *BufferPointer;
  BOOLEAN                     SectorAddressing;
  UINT64                      CardSize;
  MMC_PARTITION_DATA          *Partition;

  DEBUG((EFI_D_INFO, "MMCSDBlockWriteBlocks: WriteBlocks ...\n"));

  Status   = EFI_SUCCESS;
  Partition = CARD_PARTITION_DATA_FROM_THIS (This);
  CardData  = Partition->CardData;
  SdHostIo = CardData->SdHostIo;

  DEBUG((EFI_D_INFO,
    "MMCSDBlockWriteBlocks: Write (PART=%d, LBA=0x%08lx, Buffer=0x%08x, Size=0x%08x)\n",
    CARD_DATA_PARTITION_NUM (Partition), LBA, Buffer, BufferSize
    ));

  Status = MmcSelectPartition (Partition);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CardSize = MultU64x32 (CardData->BlockNumber, CardData->BlockLen);

  if ((CardData->CardType == SDMemoryCard2High) || (CardSize >= SIZE_2GB)) {
    SectorAddressing = TRUE;
  } else {
    SectorAddressing = FALSE;
  }

  if (SectorAddressing) {
    //
    // Sector Address
    //
    Address = (UINT32)DivU64x32 (MultU64x32 (LBA, Partition->BlockIoMedia.BlockSize), 512);
  } else {
    //
    //Byte Address
    //
    Address = (UINT32)MultU64x32 (LBA, Partition->BlockIoMedia.BlockSize);
  }

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Invalid parameter \n"));
    goto Done;
  }

  if ((BufferSize % Partition->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Bad buffer size \n"));
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  if (This->Media->ReadOnly == TRUE) {
    Status = EFI_WRITE_PROTECTED;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Write protected \n"));
    goto Done;
  }

  BufferPointer   = Buffer;
  RemainingLength = (UINT32) BufferSize;

  while (RemainingLength > 0) {
    if ((BufferSize > Partition->BlockIoMedia.BlockSize)) {
      if (RemainingLength > SdHostIo->HostCapability.BoundarySize) {
        TransferLength = SdHostIo->HostCapability.BoundarySize;
      } else {
        TransferLength = RemainingLength;
      }
      if (CardData->CardType == MMCCard) {
        if (!(CardData->ExtCSDRegister.CARD_TYPE & (BIT2 | BIT3)))  {
            Status = SendCommand (
                       SdHostIo,
                       SET_BLOCKLEN,
                       Partition->BlockIoMedia.BlockSize,
                       NoData,
                       NULL,
                       0,
                       ResponseR1,
                       TIMEOUT_COMMAND,
                       (UINT32*)&(CardData->CardStatus)
                       );
    
            if (EFI_ERROR (Status)) {
              break;
            }
        }
        Status = SendCommand (
                   SdHostIo,
                   SET_BLOCK_COUNT,
                   TransferLength / Partition->BlockIoMedia.BlockSize,
                   NoData,
                   NULL,
                   0,
                   ResponseR1,
                   TIMEOUT_COMMAND,
                   (UINT32*)&(CardData->CardStatus)
                   );
        if (EFI_ERROR (Status)) {
          break;
        }
      }

      CopyMem (CardData->AlignedBuffer, BufferPointer, TransferLength);

      Status = SendCommand (
                 SdHostIo,
                 WRITE_MULTIPLE_BLOCK,
                 Address,
                 OutData,
                 CardData->AlignedBuffer,
                 (UINT32)TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: WRITE_MULTIPLE_BLOCK -> Fail\n"));
        break;
      }
    } else {
      if (RemainingLength > Partition->BlockIoMedia.BlockSize) {
        TransferLength = Partition->BlockIoMedia.BlockSize;
      } else {
        TransferLength = RemainingLength;
      }

      CopyMem (CardData->AlignedBuffer, BufferPointer, TransferLength);

      Status = SendCommand (
                 SdHostIo,
                 WRITE_BLOCK,
                 Address,
                 OutData,
                 CardData->AlignedBuffer,
                 (UINT32)TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );
    }

    if (SectorAddressing) {
      //
      // Sector Address
      //
      Address += TransferLength / 512;
    } else {
      //
      //Byte Address
      //
      Address += TransferLength;
    }
    BufferPointer   += TransferLength;
    RemainingLength -= TransferLength;
  }

  if (EFI_ERROR (Status)) {
    SendCommand (
      SdHostIo,
      STOP_TRANSMISSION,
      0,
      NoData,
      NULL,
      0,
      ResponseR1b,
      TIMEOUT_COMMAND,
      (UINT32*)&(CardData->CardStatus)
      );
  }

Done:
//DEBUG((EFI_D_ERROR, "  Status = %r\n", Status));
  return Status;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.FlushBlocks() function.
  (In this driver, this function just returns EFI_SUCCESS.)

  @param  This                    The EFI_BLOCK_IO_PROTOCOL instance.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
MmcSdBlockFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  return EFI_SUCCESS;
}

/**
  MMC/SD card BlockIo init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
MmcSdBlockIoInit (
  IN  CARD_DATA    *CardData
  )
{
  UINTN               Loop;
  MMC_PARTITION_DATA  *Partition;
  EXT_CSD             *ExtCsd;
  UINT64              GP_CHUNK_SIZE;
  UINT32              GP_SIZE_MULT;
  UINT64              GppSize;
  UINTN               GppIndex=0;

  Partition = CardData->Partitions;

  ExtCsd = &CardData->ExtCSDRegister;

  //
  // Determine GP partitioning chunk size
  //
  GP_CHUNK_SIZE = 0;
  if (((ExtCsd->PARTITIONING_SUPPORT & BIT0) == BIT0) &&
      ((ExtCsd->PARTITION_SETTING_COMPLETED & BIT0) == BIT0)) {
    GP_CHUNK_SIZE = MultU64x32 (ExtCsd->HC_WP_GRP_SIZE * ExtCsd->HC_ERASE_GRP_SIZE, SIZE_512KB);
  }

  for (Loop = 0; Loop < MAX_NUMBER_OF_PARTITIONS; Partition++, Loop++) {
    //
    //BlockIO protocol
    //
    Partition->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION;
    Partition->BlockIo.Media       = &(Partition->BlockIoMedia);
    Partition->BlockIo.Reset       = MmcSdBlockReset;
    Partition->BlockIo.ReadBlocks  = MmcSdBlockReadBlocks ;
    Partition->BlockIo.WriteBlocks = MmcSdBlockWriteBlocks;
    Partition->BlockIo.FlushBlocks = MmcSdBlockFlushBlocks;

    Partition->BlockIoMedia.MediaId          = 0;
    Partition->BlockIoMedia.RemovableMedia   = FALSE;
    Partition->BlockIoMedia.MediaPresent     = TRUE;
    Partition->BlockIoMedia.LogicalPartition = FALSE;

    //
    // Force the User partition to be enabled
    //
    if (Loop == 0) {
      Partition->Present = TRUE;
    }

    if (CardData->CSDRegister.PERM_WRITE_PROTECT || CardData->CSDRegister.TMP_WRITE_PROTECT) {
      Partition->BlockIoMedia.ReadOnly         = TRUE;
    } else {
      Partition->BlockIoMedia.ReadOnly         = FALSE;
    }

    Partition->BlockIoMedia.WriteCaching     = FALSE;
    Partition->BlockIoMedia.BlockSize        = CardData->BlockLen;
    Partition->BlockIoMedia.IoAlign          = 1;
    Partition->BlockIoMedia.LastBlock        = (EFI_LBA)(CardData->BlockNumber - 1);

    //
    // Handle GPP partitions
    //
    GppSize = 0;
    if ((GP_CHUNK_SIZE != 0) && (Loop >= 4)) {
      Partition->BlockIoMedia.LastBlock = (EFI_LBA) 0;
      GppIndex = Loop - 4;
      GP_SIZE_MULT = MmcGetExtCsd24 (
                       CardData,
                       OFFSET_OF (EXT_CSD, GP_SIZE_MULT_1) + (3 * GppIndex)
                       );
      GppSize = MultU64x32 (GP_SIZE_MULT, (UINT32)GP_CHUNK_SIZE);
    }

    if (GppSize != 0) {
      Partition->BlockIoMedia.LastBlock =
        DivU64x32 (GppSize, Partition->BlockIoMedia.BlockSize) - 1;
      DEBUG ((EFI_D_INFO,
        "GPP%d last-block: 0x%lx\n",
        GppIndex + 1,
        Partition->BlockIoMedia.LastBlock
        ));
      Partition->Present = TRUE;
    }
  }

  DEBUG ((EFI_D_INFO, "MMC SD Block I/O: Initialized\n"));

  return EFI_SUCCESS;
}



