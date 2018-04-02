/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  CEATABlockIo.c

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

  CEATABlockIo.c

Abstract:

   Block I/O protocol for CE-ATA device

**/
#include "MediaDeviceDriver.h"

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
CeAtaBlockReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_STATUS                 Status;
  CARD_DATA                  *CardData;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;

  CardData  = CARD_DATA_FROM_THIS(This);
  SDHostIo = CardData->SDHostIo;

  if (!ExtendedVerification) {
    Status = SoftwareReset (CardData);
  } else {
    Status = SDHostIo->ResetSDHost (SDHostIo, Reset_DAT_CMD);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = MmcSdCardInit (CardData);
  }

  return Status;

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
CeAtaBlockReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  CARD_DATA                   *CardData;
  UINT32                      TransferSize;
  UINT8                       *pBuf;
  UINT32                      Index;
  UINT64                      Address;
  UINT64                      CEATALBA;
  UINT32                      BoundarySize;

  Status       = EFI_SUCCESS;
  CardData     = CARD_DATA_FROM_THIS(This);
  pBuf         = Buffer;
  Index        = 0;
  Address      = MultU64x32(LBA, CardData->BlockIoMedia.BlockSize);
  BoundarySize = CardData->SDHostIo->HostCapability.BoundarySize;

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (MediaId != CardData->BlockIoMedia.MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Exit;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if ((Address + BufferSize) > MultU64x32 (CardData->BlockIoMedia.LastBlock + 1, CardData->BlockIoMedia.BlockSize)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  do {
    if (BufferSize < BoundarySize) {
      TransferSize = (UINT32)BufferSize;
    } else {
      TransferSize = BoundarySize;
    }

    Address += Index * TransferSize;
    CEATALBA = DivU64x32 (Address, DATA_UNIT_SIZE);

    Status = ReadDMAExt (
               CardData,
               CEATALBA,
               pBuf,
               (UINT16)(TransferSize / DATA_UNIT_SIZE)
               );
    if (EFI_ERROR (Status)) {
     DEBUG((EFI_D_ERROR, "Read Failed at 0x%x, Index %d", Address, Index));
     DEBUG((EFI_D_ERROR, "0x%x\n", TransferSize));
     This->Reset (This, TRUE);
     goto Exit;
    }
    BufferSize -= TransferSize;
    pBuf       += TransferSize;
    Index ++;
  } while (BufferSize != 0);

Exit:
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
CeAtaBlockWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  CARD_DATA                   *CardData;
  UINT32                      TransferSize;
  UINT8                       *pBuf;
  UINT32                      Index;
  UINT64                      Address;
  UINT64                      CEATALBA;
  UINT32                      BoundarySize;

  Status       = EFI_SUCCESS;
  CardData     = CARD_DATA_FROM_THIS(This);
  pBuf         = Buffer;
  Index        = 0;
  Address      = MultU64x32(LBA, CardData->BlockIoMedia.BlockSize);
  BoundarySize = CardData->SDHostIo->HostCapability.BoundarySize;

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (MediaId != CardData->BlockIoMedia.MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Exit;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if (CardData->BlockIoMedia.ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Exit;
  }

  if ((Address + BufferSize) > MultU64x32 (CardData->BlockIoMedia.LastBlock + 1, CardData->BlockIoMedia.BlockSize)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  CardData->NeedFlush = TRUE;

  do {
    if (BufferSize < BoundarySize) {
      TransferSize = (UINT32)BufferSize;
    } else {
      TransferSize = BoundarySize;
    }

    Address += Index * TransferSize;
    CEATALBA = DivU64x32 (Address, DATA_UNIT_SIZE);

    Status = WriteDMAExt (
               CardData,
               CEATALBA,
               pBuf,
               (UINT16)(TransferSize / DATA_UNIT_SIZE)
               );
    if (EFI_ERROR (Status)) {
     DEBUG((EFI_D_ERROR, "Write Failed at 0x%x, Index %d", Address, Index));
     DEBUG((EFI_D_ERROR, "0x%x\n", TransferSize));
     This->Reset (This, TRUE);
     goto Exit;
    }
    BufferSize -= TransferSize;
    pBuf       += TransferSize;
    Index ++;
  } while (BufferSize != 0);

Exit:
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
CeAtaBlockFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{

  EFI_STATUS                  Status;
  CARD_DATA                   *CardData;

  CardData  = CARD_DATA_FROM_THIS(This);

  if (CardData->NeedFlush) {
    CardData->NeedFlush = FALSE;
    Status = FlushCache (CardData);
  }

  return EFI_SUCCESS;
}

/**
  CEATA card BlockIo init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
CeAtaBlockIoInit (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS   Status;
  UINT64       MaxSize;
  //
  //BlockIO protocol
  //
  CardData->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION;
  CardData->BlockIo.Media       = &(CardData->BlockIoMedia);
  CardData->BlockIo.Reset       = CeAtaBlockReset;
  CardData->BlockIo.ReadBlocks  = CeAtaBlockReadBlocks ;
  CardData->BlockIo.WriteBlocks = CeAtaBlockWriteBlocks;
  CardData->BlockIo.FlushBlocks = CeAtaBlockFlushBlocks;

  CardData->BlockIoMedia.MediaId          = 0;
  CardData->BlockIoMedia.RemovableMedia   = FALSE;
  CardData->BlockIoMedia.MediaPresent     = TRUE;
  CardData->BlockIoMedia.LogicalPartition = FALSE;

  if (CardData->CSDRegister.PERM_WRITE_PROTECT | CardData->CSDRegister.TMP_WRITE_PROTECT) {
    CardData->BlockIoMedia.ReadOnly       = TRUE;
  } else {
    CardData->BlockIoMedia.ReadOnly       = FALSE;
  }

  CardData->BlockIoMedia.WriteCaching     = FALSE;
  CardData->BlockIoMedia.IoAlign          = 1;

  Status = IndentifyDevice (CardData);
  if (EFI_ERROR (Status)) {
   goto Exit;
  }

  //
  //Some device does not support this feature
  //

  if (CardData->IndentifyDeviceData.MaxWritesPerAddress == 0) {
    CardData->BlockIoMedia.ReadOnly       = TRUE;
  }

  CardData->BlockIoMedia.BlockSize        = (1 << CardData->IndentifyDeviceData.Sectorsize);
  ASSERT(CardData->BlockIoMedia.BlockSize >= 12);

  MaxSize = *(UINT64*)(CardData->IndentifyDeviceData.MaximumLBA);
  MaxSize = MultU64x32 (MaxSize, 512);

  CardData->BlockNumber = DivU64x32 (MaxSize, CardData->BlockIoMedia.BlockSize);

  CardData->BlockIoMedia.LastBlock        = (EFI_LBA)(CardData->BlockNumber - 1);

Exit:
  return Status;

}



