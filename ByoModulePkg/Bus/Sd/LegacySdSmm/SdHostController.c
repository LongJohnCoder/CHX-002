/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SDHostController.c

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

  SDHostController.c

Abstract:

  SD Host I/O protocol implementation

**/

#include "SdHostDriver.h"
#include "SdCard.h"

#define SD_DEBUG(a)  //DEBUG(a)

//------------------------------------------------------------------------------
// DEBUG helper
//------------------------------------------------------------------------------
int            _inp (unsigned short port);
int            _outp (unsigned short port, int databyte );
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)

typedef union {
  UINT8  volatile  *buf;
  UINT8  volatile  *ui8;
  UINT16 volatile  *ui16;
  UINT32 volatile  *ui32;
  UINT64 volatile  *ui64;
  UINTN  volatile  ui;
} PTR;

/**
  15 us (+- 15us)- 983 ms (+- 15us)

  @param  Count                   Unit 15us.

  @retval
**/
VOID
Delay15us (
  UINT32 Count
  )
{
  int   Data1;
  int   Data2;

  Data1 = _inp(0x61);
  Data1 = Data1 & 0x10;

  for (; Count != 0; Count --) {
    do {
      Data2 = _inp(0x61);
      Data2 = Data2 & 0x10;
    } while (Data2 == Data1);
    Data1 = Data2;
  }
}

/**
  Private service to perform memory mapped I/O read/write.

  @param  Width                   Width of the memory mapped I/O operation.
  @param  Count                   Count of the number of accesses to perform.
  @param  Destination             Destination of the memory mapped I/O operation.
  @param  Source                  Source of the memory mapped I/O operation.

  @retval EFI_SUCCESS             Successful operation.
  @retval EFI_INVALID_PARAMETER   Width is invalid.

**/
EFI_STATUS
SdMemAccess (
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  PTR                        Source,
  IN  UINTN                      Count,
  OUT PTR                        Destination
  )
{
  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiPciIoWidthUint8:
    for (; Count > 0; Count--, Destination.buf += 1, Source.buf += 1) {
      *Destination.ui8 = *Source.ui8;
    }
    break;

  case EfiPciIoWidthUint16:
    for (; Count > 0; Count--, Destination.buf += 2, Source.buf += 2) {
      *Destination.ui16 = *Source.ui16;
    }
    break;

  case EfiPciIoWidthUint32:
    for (; Count > 0; Count--, Destination.buf += 4, Source.buf += 4) {
      *Destination.ui32 = *Source.ui32;
    }
    break;

  case EfiPciIoWidthUint64:
    for (; Count > 0; Count--, Destination.buf += 8, Source.buf += 8) {
      *Destination.ui64 = *Source.ui64;
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Private service to perform memory mapped I/O read/write.

  @param  Address                 Width of the memory mapped I/O operation.
  @param  ReadWriteOp             Read or write operation.
  @param  Destination             Destination of the memory mapped I/O operation.
  @param  WidthOperation          Width of the memory mapped I/O operation.
  @param  Count                   Count of the number of accesses to perform.
  @param  Buffer                  Source of the memory mapped I/O operation.

  @retval EFI_SUCCESS             Successful operation.
  @retval EFI_INVALID_PARAMETER   Width is invalid.

**/
EFI_STATUS
SdMemMapAccess (
  UINT32                    Address,
  BOOLEAN                   ReadWriteOp,
  UINTN                     Offset,
  EFI_PCI_IO_PROTOCOL_WIDTH WidthOperation,
  UINTN                     Count,
  VOID                      *Buffer
  )
{
  EFI_PHYSICAL_ADDRESS  OperationAddress;
  PTR                   Source;
  PTR                   Destination;

  OperationAddress = (EFI_PHYSICAL_ADDRESS)(Address + Offset);
  //DEBUG ((EFI_D_ERROR, "ReadWriteOp = %x, OperationAddress = %x\n", OperationAddress, ReadWriteOp));
  if (!ReadWriteOp) {
    //read
    Destination.buf = Buffer;
    Source.buf      = (VOID *) (UINTN) OperationAddress;
  } else {
    //write
    Source.buf      = Buffer;
    Destination.buf = (VOID *) (UINTN) OperationAddress;
  }
  return SdMemAccess (WidthOperation, Source, Count, Destination);
}

/**
  Output operation error reason.

  @param  CommandIndex            Operation command.
  @param  ErrorCode               Error code.

  @retval EFI_DEVICE_ERROR        Hardware Error.

**/
STATIC
EFI_STATUS
GetErrorReason (
  IN  UINT16    CommandIndex,
  IN  UINT16    ErrorCode
  )
{
  EFI_STATUS    Status;

  Status = EFI_DEVICE_ERROR;
  
  DEBUG((EFI_D_ERROR, "[%2d] -- ", CommandIndex));
  
  if (ErrorCode & BIT0) {
    Status = EFI_TIMEOUT;
    DEBUG((EFI_D_ERROR, "Command Timeout Error"));
  }

  if (ErrorCode & BIT1) {
    Status = EFI_CRC_ERROR;
    DEBUG((EFI_D_ERROR, "Command CRC Error"));
  }

  if (ErrorCode & BIT2) {
    DEBUG((EFI_D_ERROR, "Command End Bit Error"));
  }

  if (ErrorCode & BIT3) {
    DEBUG((EFI_D_ERROR, "Command Index Error"));
  }
  if (ErrorCode & BIT4) {
    Status = EFI_TIMEOUT;
    DEBUG((EFI_D_ERROR, "Data Timeout Error"));
  }

  if (ErrorCode & BIT5) {
    Status = EFI_CRC_ERROR;
    DEBUG((EFI_D_ERROR, "Data CRC Error"));
  }

  if (ErrorCode & BIT6) {
    DEBUG((EFI_D_ERROR, "Data End Bit Error"));
  }

  if (ErrorCode & BIT7) {
    DEBUG((EFI_D_ERROR, "Current Limit Error"));
  }

  if (ErrorCode & BIT8) {
    DEBUG((EFI_D_ERROR, "Auto CMD12 Error"));
  }

  if (ErrorCode & BIT9) {
    DEBUG((EFI_D_ERROR, "ADMA Error"));
  }

  DEBUG((EFI_D_ERROR, "\n"));

  return Status;
}


STATIC
EFI_STATUS
SdHostRead (
  IN     SDHOST_DATA                  *SdHost,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINTN                        Offset,
  IN OUT VOID                         *Buffer
  )
{
  UINT32 MemAddress;

  MemAddress = SdHost->SdHostIo.Slot0RegMap;

  return SdMemMapAccess(MemAddress, 0, Offset, Width, 1, Buffer);
}


UINT8
SdHostRead8 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset
  )
{
  UINT8 Data;

  SdHostRead (SdHost, EfiPciIoWidthUint8, Offset, &Data);

  return Data;
}


UINT16
SdHostRead16 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset
  )
{
  UINT16 Data;

  SdHostRead (SdHost, EfiPciIoWidthUint16, Offset, &Data);

  return Data;
}


UINT32
SdHostRead32 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset
  )
{
  UINT32 Data;

  SdHostRead (SdHost, EfiPciIoWidthUint32, Offset, &Data);

  return Data;
}
STATIC
EFI_STATUS
SdHostWrite (
  IN     SDHOST_DATA                  *SdHost,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINTN                        Offset,
  IN OUT VOID                         *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      MemAddress;

  MemAddress = SdHost->SdHostIo.Slot0RegMap;
  Status = SdMemMapAccess(MemAddress, 1, Offset, Width, 1, Buffer);

  return Status;
}


UINT8
SdHostWrite8 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset,
  IN     UINT8                        Data
  )
{
  SdHostWrite (SdHost, EfiPciIoWidthUint8, Offset, &Data);

  return Data;
}


UINT16
SdHostWrite16 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset,
  IN     UINT16                       Data
  )
{
  SdHostWrite (SdHost, EfiPciIoWidthUint16, Offset, &Data);

  return Data;
}


UINT32
SdHostWrite32 (
  IN     SDHOST_DATA                  *SdHost,
  IN     UINTN                        Offset,
  IN     UINT32                       Data
  )
{
  SdHostWrite (SdHost, EfiPciIoWidthUint32, Offset, &Data);

  return Data;
}


UINT32
CheckControllerVersion (
  IN SDHOST_DATA     *SdHost
  )
{
  UINT32 Data16;
  SdHostRead (SdHost, EfiPciIoWidthUint16, MMIO_CTRLRVER, &Data16);
  DEBUG ((EFI_D_INFO, "CheckControllerVersion: %x \n", Data16 & 0xFF));
  return (Data16 & 0xFF);
}

/**
  Power on/off the LED associated with the slot.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  BlockLength             card supportes block length.

  @retval EFI_SUCCESS             Success.

**/
STATIC
EFI_STATUS
HostLEDEnable (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  SDHOST_DATA            *SdHostData;
  UINT8                  Data;

  SdHostData = SDHOST_DATA_FROM_THIS (This);

  Data = SdHostRead8 (SdHostData, MMIO_HOSTCTL);

  if (Enable) {
    //
    //LED On
    //
    Data |= BIT0;
  } else {
    //
    //LED Off
    //
    Data &= ~BIT0;
  }

  SdHostWrite8 (SdHostData, MMIO_HOSTCTL, Data);

  return EFI_SUCCESS;
}

/**
  The main function used to send the command to the card inserted into the SD host slot.
  It will assemble the arguments to set the command register and wait for the command
  and transfer completed until timeout. Then it will read the response register to fill
  the ResponseData.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  CommandIndex            The command index to set the command index field of command register.
  @param  Argument                Command argument to set the argument field of command register.
  @param  DataType                TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                  Contains the data read from / write to the device.
  @param  BufferSize              The size of the buffer.
  @param  ResponseType            RESPONSE_TYPE.
  @param  TimeOut                 Time out value in 1 ms unit.
  @param  ResponseData            Depending on the ResponseType, such as CSD or card status.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SendHCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData OPTIONAL
  )
{
  EFI_STATUS            Status;
  SDHOST_DATA           *SdHostData;
  UINT32                ResponseDataCount;
  UINT16                Data16;
  UINT32                Data32;
  UINT64                Data64;
  UINT8                 Index;
  BOOLEAN               CommandCompleted;
  INT32                 Timeout = 1000;

  Status             = EFI_SUCCESS;
  ResponseDataCount  = 1;
  SdHostData         = SDHOST_DATA_FROM_THIS (This);

  if (Buffer != NULL && DataType == NoData) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "SendCommand: Invalid parameter -> \n"));
    goto Exit;
  }
 
  if (((UINTN)Buffer & (This->HostCapability.BoundarySize - 1)) != (UINTN)NULL) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "SendCommand: Invalid parameter -> \n"));
    goto Exit;
  }
  DEBUG ((EFI_D_INFO, "SendCommand: CMD%d \n", CommandIndex));

  //
  // Check CMD INHIBIT and DATA INHIBIT before send command
  //
  do {
    Data32 = SdHostRead32 ( SdHostData, MMIO_PSTATE);
    if (SdHostData->EnableVerboseDebug) {
      DEBUG((EFI_D_INFO, "Wait CMD INHIBIT %x\n",Data32 ));
    }
  } while ((Timeout -- > 0) && (Data32 & BIT0));
  
  Timeout = 1000;
  do {
    Data32 = SdHostRead32 ( SdHostData, MMIO_PSTATE);
    if (SdHostData->EnableVerboseDebug) {
      DEBUG((EFI_D_INFO, "Wait DATA INHIBIT %x\n",Data32 ));
    }
  } while ((Timeout -- > 0) && (Data32 & BIT1));

  //
  //Clear status bits
  //
  SD_DEBUG ((EFI_D_INFO, "NINTSTS(0x30) <- 0x%x\n", (UINTN) 0xffff));
  SdHostWrite16 (SdHostData, MMIO_NINTSTS, 0xffff);
  SD_DEBUG ((EFI_D_INFO, "SendCommand: NINTSTS(0x30): 0x%x\n", SdHostRead16 (SdHostData, MMIO_NINTSTS)));

  SD_DEBUG ((EFI_D_INFO, "ERINTSTS <- 0x%x\n", (UINTN) 0xffff));
  SdHostWrite16 (SdHostData, MMIO_ERINTSTS, 0xffff); 
  SD_DEBUG ((EFI_D_INFO, "SendCommand: ERINTSTS(0x32): 0x%x\n", SdHostRead16 (SdHostData, MMIO_ERINTSTS)));

  SD_DEBUG ((EFI_D_INFO, "SendCommand: DMAADR(0x00) <- 0x%x\n", (UINTN) Buffer));
  SdHostWrite32 (SdHostData, MMIO_DMAADR, (UINT32)(UINTN) Buffer);

  if (Buffer != NULL) {
    Data16 = SdHostRead16 (SdHostData, MMIO_BLKSZ);
    Data16 &= ~(0xFFF);
    if (BufferSize <= SdHostData->BlockLength) {
      Data16 |= BufferSize;
    } else {
      Data16 |= SdHostData->BlockLength;
    }
    Data16 |= 0x7000; // Set to 512KB for SDMA block size
  } else {
    Data16 = 0;
  }

  SD_DEBUG ((EFI_D_INFO, "SendCommand: BLKSZ(0x04) <- 0x%x\n", Data16));
  SdHostWrite16 (SdHostData, MMIO_BLKSZ, Data16);

  if (Buffer != NULL) {
    if (BufferSize <= SdHostData->BlockLength) {
      Data16 = 1;
    } else {
      Data16 = (UINT16) (BufferSize / SdHostData->BlockLength);
    }
  } else {
    Data16 = 0;
  }

  SD_DEBUG ((EFI_D_INFO, "SendCommand: BLKCNT(0x06) <- 0x%x\n", Data16));
  SdHostWrite16 (SdHostData, MMIO_BLKCNT, Data16);
  
  //
  // Argument
  //
  SD_DEBUG ((EFI_D_INFO, "SendCommand: CMDARG(0x08) <- 0x%x\n", Argument));
  SdHostWrite32 (SdHostData, MMIO_CMDARG, Argument);

  //
  // Transfer Mode
  //
  Data16 = SdHostRead16 (SdHostData, MMIO_XFRMODE); 
  SD_DEBUG ((EFI_D_INFO, "SendCommand: XFRMODE(0x0C) mode read 0x%x\n", Data16));
  
  //
  // BIT0 - DMA Enable
  // BIT2 - Auto Cmd12
  //
  if (DataType == InData) {
    Data16 |= BIT4 | BIT0;
  } else if (DataType == OutData){
    Data16 &= ~BIT4;
    Data16 |= BIT0;
  } else {
    Data16 &= ~(BIT4 | BIT0);
  }
 
  if (BufferSize <= SdHostData->BlockLength) {
    Data16 &= ~ (BIT5 | BIT1 | BIT2);
  } else {
     if (SdHostData->IsAutoStopCmd && !SdHostData->IsEmmc) {
      Data16 |= (BIT5 | BIT1 | BIT2);
     } else {
      Data16 |= (BIT5 | BIT1);
     }
  }
  if (CommandIndex == SEND_EXT_CSD) {   
    Data16 |= BIT1;   
    SD_DEBUG ((EFI_D_INFO, "SendCommand:Enable bit 1, XFRMODE <- 0x%x\n", Data16));   
  }

  SD_DEBUG ((EFI_D_INFO, "SendCommand: XFRMODE(0x0C) <- 0x%x\n", Data16));  
  SdHostWrite16 (SdHostData, MMIO_XFRMODE, Data16);

  //
  //Command
  //
  //ResponseTypeSelect    IndexCheck    CRCCheck    ResponseType
  //  00                     0            0           NoResponse
  //  01                     0            1           R2  
  //  10                     0            0           R3, R4
  //  10                     1            1           R1, R5, R6, R7
  //  11                     1            1           R1b, R5b
  // 
  switch (ResponseType) {
    case ResponseNo:
      Data16 = (CommandIndex << 8);
      ResponseDataCount = 0;
      break;

    case ResponseR1:
    case ResponseR5:
    case ResponseR6:
    case ResponseR7:
      Data16 = (CommandIndex << 8) | BIT1 | BIT4| BIT3;
      ResponseDataCount = 1;
      break;

    case ResponseR1b:
    case ResponseR5b:
      Data16 = (CommandIndex << 8) | BIT0 | BIT1 | BIT4| BIT3;
      ResponseDataCount = 1;
      break;

    case ResponseR2:
      Data16 = (CommandIndex << 8) | BIT0 | BIT3;
      ResponseDataCount = 4;
      break;
 
    case ResponseR3:
    case ResponseR4:
      Data16 = (CommandIndex << 8) | BIT1;
      ResponseDataCount = 1;
      break;

    default:
      ASSERT (0);
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  if (DataType != NoData) {
    Data16 |= BIT5;
  }

  HostLEDEnable (This, TRUE);

  SD_DEBUG ((EFI_D_INFO, "SendCommand: SDCMD(0x0E) <- 0x%x\n", Data16));
  CommandCompleted = FALSE;
  SdHostWrite16 (SdHostData, MMIO_SDCMD, Data16);

  //TimeOut *= 1000;  //ms to us conversion
  do {
    Data16 = SdHostRead16 (SdHostData, MMIO_ERINTSTS);
    if (SdHostData->EnableVerboseDebug && (Data16 != 0)) {
      SD_DEBUG ((EFI_D_INFO, "SendCommand: ERINTSTS(0x32): 0x%x\n", Data16));
      SD_DEBUG ((EFI_D_INFO, "SendCommand: ERINTEN(0x36): 0x%x\n", SdHostRead16 (SdHostData, MMIO_ERINTEN)));
      SD_DEBUG ((EFI_D_INFO, "SendCommand: NINTSTS(0x30): 0x%x\n", SdHostRead16 (SdHostData, MMIO_NINTSTS)));
      SD_DEBUG ((EFI_D_INFO, "SendCommand: NINTEN(0x34): 0x%x\n", SdHostRead16 (SdHostData, MMIO_NINTEN)));
    }

    if ((Data16 & 0x17FF) != 0) {
      Status = GetErrorReason (CommandIndex, Data16);
      SD_DEBUG ((EFI_D_INFO, "00 -10:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x00),SdHostRead32 (SdHostData, 0x04),SdHostRead32 (SdHostData, 0x08),SdHostRead32 (SdHostData, 0x0C)));
      SD_DEBUG ((EFI_D_INFO, "10 -20:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x10),SdHostRead32 (SdHostData, 0x14),SdHostRead32 (SdHostData, 0x18),SdHostRead32 (SdHostData, 0x1C)));
      SD_DEBUG ((EFI_D_INFO, "20 -30:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x20),SdHostRead32 (SdHostData, 0x24),SdHostRead32 (SdHostData, 0x28),SdHostRead32 (SdHostData, 0x2C)));
      SD_DEBUG ((EFI_D_INFO, "30 -40:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x30),SdHostRead32 (SdHostData, 0x34),SdHostRead32 (SdHostData, 0x38),SdHostRead32 (SdHostData, 0x3C)));
      SD_DEBUG ((EFI_D_INFO, "40 -50:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x40),SdHostRead32 (SdHostData, 0x44),SdHostRead32 (SdHostData, 0x48),SdHostRead32 (SdHostData, 0x4C)));
      SD_DEBUG ((EFI_D_INFO, "50 -60:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x50),SdHostRead32 (SdHostData, 0x54),SdHostRead32 (SdHostData, 0x58),SdHostRead32 (SdHostData, 0x5C)));
      goto Exit; 
    }
    
    Data16 = SdHostRead16 (SdHostData, MMIO_NINTSTS) & 0x1ff;
    if (SdHostData->EnableVerboseDebug && (Data16 > 1)) {
      SD_DEBUG ((EFI_D_INFO, "SendCommand: NINTSTS(0x30): 0x%x\n", Data16));
    }

    if ((Data16 & BIT0) == BIT0) {
      //
      // Command completed
      //
      CommandCompleted = TRUE;

      SdHostWrite16 (SdHostData, MMIO_NINTSTS, BIT0);

      if ((DataType == NoData) &(ResponseType != ResponseR1b)) {
        break;
      }
    }

    if (CommandCompleted) {
      //
      // DMA interrupted
      //
      if ((Data16 & BIT3) == BIT3) {
        SdHostWrite16 (SdHostData, MMIO_NINTSTS, BIT3);
        Data32 = SdHostRead32 (SdHostData, MMIO_DMAADR);
   
        SD_DEBUG ((EFI_D_INFO, "SendCommand: DMAADR(0x00) <- 0x%x\n", Data32));
        SdHostWrite32 (SdHostData, MMIO_DMAADR, Data32);
      }

      //
      // Transfer completed
      //
      if ((Data16 & BIT1) == BIT1) {
        SdHostWrite16 (SdHostData, MMIO_NINTSTS, BIT1);
        break;
      }
    }
    Delay15us(67);

    TimeOut --;
  } while (TimeOut > 0);

  if (TimeOut == 0) {
    Status = EFI_TIMEOUT;
    SD_DEBUG ((EFI_D_ERROR, "SendCommand: Time out \n"));
    goto Exit;
  }
  if (ResponseData != NULL) {
    UINT32 *ResDataPtr = NULL;

    ResDataPtr = ResponseData;
    for (Index = 0; Index < ResponseDataCount; Index++) {
        *ResDataPtr = SdHostRead32(SdHostData, MMIO_RESP + Index * 4);
         ResDataPtr++;
    }
    SD_DEBUG ((EFI_D_INFO, "Reponse Data 0: RESPONSE(0x10) <- 0x%x\n", *ResponseData));

    if (ResponseType == ResponseR2) {
      //
      // Adjustment for R2 response
      //
      Data32 = 1;
      for (Index = 0; Index < ResponseDataCount; Index++) {
        Data64 = LShiftU64(*ResponseData, 8);
        *ResponseData = (UINT32)((Data64 & 0xFFFFFFFF) | Data32);
        Data32 =  (UINT32)RShiftU64 (Data64, 32);
        ResponseData++;
      }
    }
  }

  if (SdHostData->EnableVerboseDebug) {
    SD_DEBUG ((EFI_D_INFO, "Before Exit Send Command\n"));
    SD_DEBUG ((EFI_D_INFO, "00 -10:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x00),SdHostRead32 (SdHostData, 0x04),SdHostRead32 (SdHostData, 0x08),SdHostRead32 (SdHostData, 0x0C)));
    SD_DEBUG ((EFI_D_INFO, "10 -20:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x10),SdHostRead32 (SdHostData, 0x14),SdHostRead32 (SdHostData, 0x18),SdHostRead32 (SdHostData, 0x1C)));
    SD_DEBUG ((EFI_D_INFO, "20 -30:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x20),SdHostRead32 (SdHostData, 0x24),SdHostRead32 (SdHostData, 0x28),SdHostRead32 (SdHostData, 0x2C)));
    SD_DEBUG ((EFI_D_INFO, "30 -40:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x30),SdHostRead32 (SdHostData, 0x34),SdHostRead32 (SdHostData, 0x38),SdHostRead32 (SdHostData, 0x3C)));
    SD_DEBUG ((EFI_D_INFO, "40 -50:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x40),SdHostRead32 (SdHostData, 0x44),SdHostRead32 (SdHostData, 0x48),SdHostRead32 (SdHostData, 0x4C)));
    SD_DEBUG ((EFI_D_INFO, "50 -60:  %08x	%08x  %08x	%08x \n", SdHostRead32 (SdHostData, 0x50),SdHostRead32 (SdHostData, 0x54),SdHostRead32 (SdHostData, 0x58),SdHostRead32 (SdHostData, 0x5C)));
  }

Exit:
  HostLEDEnable (This, FALSE);
  SD_DEBUG ((EFI_D_INFO, "SendCommand: Status -> %r\n", Status));
  
  return Status;
}

/**
  Set max clock frequency of the host, the actual frequency
  may not be the same as MaxFrequencyInKHz. It depends on
  the max frequency the host can support, divider, and host
  speed mode.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  MaxFrequency            Max frequency in HZ.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetClockFrequency (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     MaxFrequency
  )
{
  UINT16                 Data16;
  UINT32                 Frequency;
  UINT32                 Divider = 0;
  SDHOST_DATA            *SdHostData;
  UINT32                 TimeOutCount;

  SdHostData = SDHOST_DATA_FROM_THIS (This);


  SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: BaseClockInMHz = %d \n", SdHostData->BaseClockInMHz));

  Frequency = (SdHostData->BaseClockInMHz * 1000 * 1000) / MaxFrequency;
  SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: FrequencyInHz = %d \n", Frequency));

  if ((SdHostData->BaseClockInMHz * 1000 * 1000 % MaxFrequency) != 0) {
    Frequency += 1;
  }

  Divider = 1;
  while (Frequency > Divider) {
    Divider = Divider * 2;
  }
  if (Divider >= 0x400) {
    Divider = 0x200;
  }
  SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: before shift:Base Clock Divider = 0x%x \n", Divider));

  Divider = Divider >> 1;

  SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: before shit: Base Clock Divider = 0x%x \n", Divider));

  SdHostData->CurrentClockInKHz = (SdHostData->BaseClockInMHz * 1000);
  if (Divider != 0) {
    SdHostData->CurrentClockInKHz = SdHostData->CurrentClockInKHz / (Divider * 2);
  }

  //
  //Set frequency 
  //  Bit[15:8] SDCLK Frequency Select at offset 2Ch
  //    80h - base clock divided by 256
  //    40h - base clock divided by 128
  //    20h - base clock divided by 64
  //    10h - base clock divided by 32
  //    08h - base clock divided by 16
  //    04h - base clock divided by 8
  //    02h - base clock divided by 4
  //    01h - base clock divided by 2
  //    00h - Highest Frequency the target support (10MHz-63MHz)
  //
  //  Bit [07:06] are assigned to bit 09-08 of clock divider in SDCLK Frequency Select on SD controller 3.0
  //
  if (2 == CheckControllerVersion(SdHostData)) {
    Data16 =(UINT16)( (Divider & 0xFF) << 8 | (((Divider & 0xFF00) >>8)<<6));
  } else {
    Data16 = (UINT16) ( Divider << 8);
  }

  SD_DEBUG ((EFI_D_ERROR,
    "SetClockFrequency: base=%dMHz, clkctl=0x%04x, f=%dKHz\n",
    SdHostData->BaseClockInMHz,
    Data16,
    SdHostData->CurrentClockInKHz
    ));
  SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: set MMIO_CLKCTL value = 0x%x \n", Data16));
  SdHostWrite16 (SdHostData, MMIO_CLKCTL, Data16);
  Delay15us(67);
  Data16 |= BIT0;
  SdHostWrite16 (SdHostData, MMIO_CLKCTL, Data16);
  
  TimeOutCount = TIME_OUT_1S;
  do {
    Data16 = SdHostRead16 (SdHostData, MMIO_CLKCTL);
    Delay15us(67);
    TimeOutCount --;
    if (TimeOutCount == 0) {
      SD_DEBUG ((EFI_D_ERROR, "SetClockFrequency: Timeout\n"));
      return EFI_TIMEOUT;
    }
  } while ((Data16 & BIT1) != BIT1);

  Delay15us(2 * 67);
  Data16 |= BIT2;
  SdHostWrite16 (SdHostData, MMIO_CLKCTL, Data16);

  return EFI_SUCCESS;
}

/**
  Set bus width of the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  BusWidth                Bus width in 1, 4, 8 bits.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetBusWidth (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BusWidth
  )
{
  SDHOST_DATA            *SdHostData;
  UINT8                  Data;

  SdHostData = SDHOST_DATA_FROM_THIS (This);

  if ((BusWidth != 1) && (BusWidth != 4) && (BusWidth != 8)) {
    SD_DEBUG ((EFI_D_ERROR, "SetBusWidth: Invalid parameter \n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((SdHostData->SdHostIo.HostCapability.BusWidth8 == FALSE) && (BusWidth == 8)) {
     SD_DEBUG ((EFI_D_ERROR, "SetBusWidth: Invalid parameter \r\n"));
     return EFI_INVALID_PARAMETER;
  }

  Data = SdHostRead8 (SdHostData, MMIO_HOSTCTL);

  //
  // BIT3 8-bit MMC Support (MMC8): 
  // If set, SCH supports 8-bit MMC. When cleared, SCH does not support this feature
  //
  if (BusWidth == 8) {
    SD_DEBUG ((EFI_D_ERROR, "SetBusWidth: Bus Width is 8-bit ... \r\n"));
    Data |= BIT5;
  } else if (BusWidth == 4) {
    SD_DEBUG ((EFI_D_ERROR, "SetBusWidth: Bus Width is 4-bit ... \r\n"));
    Data &= ~BIT5;
    Data |= BIT1;
  } else {
    SD_DEBUG ((EFI_D_ERROR, "SetBusWidth: Bus Width is 1-bit ... \r\n"));
    Data &= ~BIT5;
    Data &= ~BIT1;
  }

  SdHostWrite8 (SdHostData, MMIO_HOSTCTL, Data);
  SD_DEBUG((EFI_D_ERROR, "SetBusWidth:MMIO_HOSTCTL value: 0x%x  \n", SdHostRead8 (SdHostData, MMIO_HOSTCTL))); 

  return EFI_SUCCESS;
}

/**
  Set voltage which could supported by the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  Voltage                 Units in 0.1 V.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetHostVoltage (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     Voltage
  )
{
  SDHOST_DATA            *SdHostData;
  UINT8                  Data;
  EFI_STATUS             Status;

  SdHostData = SDHOST_DATA_FROM_THIS (This);
  Status     = EFI_SUCCESS; 

  Data = SdHostRead8 (SdHostData, MMIO_PWRCTL);

  if (Voltage == 0) {
    //
    //Power Off the host 
    //
    Data &= ~BIT0;
  } else if (Voltage <= 18 && This->HostCapability.V18Support) {
     //
     //1.8V
     //
     Data |= (BIT1 | BIT3 | BIT0);
  } else if (Voltage > 18 &&  Voltage <= 30 && This->HostCapability.V30Support) {
     //
     //3.0V
     //
     Data |= (BIT2 | BIT3 | BIT0);
  } else if (Voltage > 30 && Voltage <= 33 && This->HostCapability.V33Support) {
     //
     //3.3V
     //
     Data |= (BIT1 | BIT2 | BIT3 | BIT0);
  } else {
     Status = EFI_UNSUPPORTED;
     goto Exit;
  }
   
  SdHostWrite8 (SdHostData, MMIO_PWRCTL, Data);
  Delay15us(67);

Exit:
  return Status;
}

EFI_STATUS
EFIAPI
SetHostSpeedMode(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     HighSpeed
  )
/*++

  Routine Description:
    Set Host High Speed
  Arguments:
    This      - Pointer to EFI_SD_HOST_IO_PROTOCOL
    HighSpeed   - True for High Speed Mode set, false for normal mode


  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER

--*/
{
  EFI_STATUS            Status;
  SDHOST_DATA           *SdHostData;
  UINT8                 Data8;

  Status             = EFI_SUCCESS;
  SdHostData         = SDHOST_DATA_FROM_THIS (This);

  if (SdHostData->IsEmmc) {
    Data8 = SdHostRead8 (SdHostData, MMIO_HOSTCTL);
    Data8 &= ~(BIT2);
    if (HighSpeed) {
      Data8 |= BIT2;
      SD_DEBUG((EFI_D_ERROR, "High Speed mode: Data8=0x%x \n", Data8));
    } else {
      SD_DEBUG((EFI_D_ERROR, "Normal Speed mode: Data8=0x%x \n", Data8));
    }
    SdHostWrite8 (SdHostData, MMIO_HOSTCTL, Data8);   	  
    Delay15us(67);
    SD_DEBUG((EFI_D_ERROR, "MMIO_HOSTCTL value: 0x%x  \n", SdHostRead8 (SdHostData, MMIO_HOSTCTL))); 
    return EFI_SUCCESS;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}


EFI_STATUS
EFIAPI
SetHostDdrMode(
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     DdrMode                       
  )
{
  EFI_STATUS            Status;
  SDHOST_DATA           *SdHostData;
  UINT16                ModeSet;

  Status             = EFI_SUCCESS;
  SdHostData         = SDHOST_DATA_FROM_THIS (This);

  if (SdHostData->IsEmmc) {
    ModeSet = SdHostRead16 (SdHostData, MMIO_HOST_CTL2);
    ModeSet &= ~(BIT0 | BIT1 | BIT2);
    if (DdrMode) {
      ModeSet |= 0x0004;
      SD_DEBUG((EFI_D_ERROR, "DDR mode: Data16=0x%x \n", ModeSet));
    } else {
      if (CheckControllerVersion(SdHostData) != 2) {
        ModeSet =  0x0;
      }  
    }
    SdHostWrite16 (SdHostData, MMIO_HOST_CTL2, ModeSet);   
    Delay15us (67);
    SD_DEBUG((EFI_D_ERROR, "MMIO_HOST_CTL2 value: 0x%x  \n", SdHostRead16 (SdHostData, MMIO_HOST_CTL2))); 
    return EFI_SUCCESS;   
  } else {
    return EFI_INVALID_PARAMETER;
  }
}


/**
  Reset the host.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  ResetAll                TRUE to reset all.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
ResetSdHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  RESET_TYPE                 ResetType
  )
{
  SDHOST_DATA            *SdHostData;
  UINT8                  Data8;
  UINT32                 Data;
  UINT16                 ErrStatus;
  UINT8                  Mask; 
  UINT32                 TimeOutCount;  
  UINT16                 SaveClkCtl;
  UINT16                 SavePwrCtl = 0;

  SdHostData = SDHOST_DATA_FROM_THIS (This);

  if ((SdHostData->PciDid == 0x0F14) || (SdHostData->PciDid == 0x0F50) || (SdHostData->PciDid == 0x2294)) {
    SdHostData->IsEmmc = TRUE;
  } else {
    SdHostData->IsEmmc = FALSE;  
  }

  Mask       = 0;
  ErrStatus  = 0;

  if (ResetType == Reset_Auto) {
    ErrStatus = SdHostRead16 (SdHostData, MMIO_ERINTSTS);
    if ((ErrStatus & 0xF) != 0) {
      //
      //Command Line
      //
      Mask |= BIT1;
    }
    if ((ErrStatus & 0x70) != 0) {
      //
      //Data Line
      //
      Mask |= BIT2;
    }
  }

  if (ResetType == Reset_DAT || ResetType == Reset_DAT_CMD) {
    Mask |= BIT2;
  }
  if (ResetType == Reset_CMD || ResetType == Reset_DAT_CMD) {
    Mask |= BIT1;
  }
  if (ResetType == Reset_All) {
    Mask = BIT0;
  }

  if (ResetType == Reset_HW) {
    SavePwrCtl = SdHostRead16 (SdHostData, MMIO_PWRCTL);
    SD_DEBUG ((EFI_D_ERROR, "Write SavePwrCtl: %x \n", (SavePwrCtl|BIT4)));

    SdHostWrite16 (SdHostData, MMIO_PWRCTL, SavePwrCtl|BIT4);
    Delay15us(67);
    SD_DEBUG ((EFI_D_ERROR, "Write SavePwrCtl: %x \n", (SavePwrCtl&(~BIT4))));
    SdHostWrite16 (SdHostData, MMIO_PWRCTL, SavePwrCtl&(~BIT4));
    Delay15us(67);
  }

  if (Mask == 0) {
    return EFI_SUCCESS;
  }

  //
  // To improve SD stability, we zero the MMIO_CLKCTL register and
  // stall for 50 microseconds before reseting the controller.  We
  // restore the register setting following the reset operation.
  //
  SaveClkCtl = SdHostRead16 (SdHostData, MMIO_CLKCTL);

  SdHostWrite16 (SdHostData, MMIO_CLKCTL, 0);
  Delay15us(67);
  //
  // Reset the SD host controller
  //
  SdHostWrite8 (SdHostData, MMIO_SWRST, Mask);

  Data          = 0;
  TimeOutCount  = TIME_OUT_1S;
  do {
    Delay15us(67);
    TimeOutCount --;

    Data8 = SdHostRead8 (SdHostData, MMIO_SWRST);
    if ((Data8 & Mask) == 0) {
      break;
    }
  } while (TimeOutCount > 0);

  //
  // We now restore the MMIO_CLKCTL register which we set to 0 above.
  //
  SdHostWrite16 (SdHostData, MMIO_CLKCTL, SaveClkCtl);

  if (TimeOutCount == 0) {
    SD_DEBUG ((EFI_D_ERROR, "ResetSDHost: Time out \n"));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Enable auto stop command.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  Enable                  TRUE to enable, FALSE to disable.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
EnableAutoStopCmd (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  SDHOST_DATA            *SdHostData;
 
  SdHostData = SDHOST_DATA_FROM_THIS (This);

  SdHostData->IsAutoStopCmd = Enable;

  return EFI_SUCCESS;
}

/**
  Set the Block length.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  BlockLength             card supportes block length.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/
EFI_STATUS
EFIAPI
SetBlockLength (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BlockLength
  )
{
  SDHOST_DATA            *SdHostData;
 
  SdHostData = SDHOST_DATA_FROM_THIS (This);

  SD_DEBUG ((EFI_D_ERROR, "SetBlockLength: Block length on the host controller: %d \n", BlockLength));
  SdHostData->BlockLength = BlockLength;

  return EFI_SUCCESS;
}

/**
  Find whether these is a card inserted into the slot. If so
  init the host. If not, return EFI_NOT_FOUND.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.

  @retval EFI_NOT_FOUND           Not find device.
  @retval EFI_SUCCESS             Success.

**/
EFI_STATUS
EFIAPI
DetectCardAndInitHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  )
{
  SDHOST_DATA            *SdHostData;
  UINT16                 Data16;
  UINT32                 Data;
  EFI_STATUS             Status;
  UINT8                  Voltages[] = { 33, 30, 18 };
  UINTN                  Loop;

  SdHostData = SDHOST_DATA_FROM_THIS (This);

  Data = 0;
  Data = SdHostRead32 (SdHostData, MMIO_PSTATE);

  if ((Data & (BIT16 | BIT17 | BIT18)) != (BIT16 | BIT17 | BIT18)) {
    //
    // Has no card inserted
    //
    SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: No card\n"));
    Status =  EFI_NOT_FOUND;
    goto Exit;
  }
  SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: Card present\n"));
    
  //
  //Enable normal status change
  //
  SdHostWrite16 (SdHostData, MMIO_NINTEN, BIT1 | BIT0);

  //
  // Enable error status change
  //
  Data16 = SdHostRead16 (SdHostData, MMIO_ERINTEN);
  Data16 |= 0xFFFF; //(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 | BIT8);
  SdHostWrite16 (SdHostData, MMIO_ERINTEN, Data16);

  //
  // Data transfer Timeout control
  //
  SdHostWrite8 (SdHostData, MMIO_TOCTL, 0x0E);

  //
  // Stall 1 milliseconds to increase SD stability.
  //
  Delay15us(67);

  Status = SetClockFrequency (This, FREQUENCY_OD);
  if (EFI_ERROR (Status)) {
    SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: SetClockFrequency failed\n"));
    goto Exit;
  }
  SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: SetClockFrequency done\n"));

  Status =  EFI_NOT_FOUND;
  for (Loop = 0; Loop < sizeof (Voltages); Loop++) {
    SD_DEBUG ((
      EFI_D_INFO,
      "DetectCardAndInitHost: SetHostVoltage %d.%dV\n",
      Voltages[Loop] / 10,
      Voltages[Loop] % 10
      ));
    Status = SetHostVoltage (This, Voltages[Loop]);
    if (EFI_ERROR (Status)) {
      SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: SetHostVoltage failed\n"));
    } else {
      SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: SetHostVoltage done\n"));
      break;
    }
  }
  if (EFI_ERROR (Status)) {
    SD_DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: Failed to SetHostVoltage\n"));
    goto Exit;
  }


Exit:
  return Status;
}

EFI_STATUS
EFIAPI
SetupDevice (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  )
{
  UINT32            tempD = 0;
  INT32             timeOut = 1000;
  UINT8             temp8 = 0;
  UINT16            temp16 = 0;
  SDHOST_DATA       *SdHostData;

  SdHostData = SDHOST_DATA_FROM_THIS (This);


  // Disable SDCLK
  SdHostWrite32(SdHostData, MMIO_CLKCTL, 0);
  Delay15us(5 * 67);
  // Disable Bus Power
  SdHostWrite8(SdHostData, MMIO_PWRCTL, 0);
  Delay15us(5 * 67);
  // Reset HC and wait for self-clear
  SdHostWrite8(SdHostData, MMIO_SWRST, 0x7);
  Delay15us(5 * 67);
  timeOut = 1000;
  do {
    temp8 = SdHostRead8(SdHostData, MMIO_SWRST);
    timeOut --;
  } while ((temp8 & (1<<0)) && (timeOut > 0));

  SD_DEBUG((EFI_D_ERROR, "Reset HC and wait for self-clear Done\n"));

  // Enable all interrupt status bits (NO CARD_INTERRUPT!)
  SdHostWrite16(SdHostData, MMIO_NINTEN, 0x3);
  // Clear all interrupt status bits
  SdHostWrite16(SdHostData, MMIO_NINTSTS, 0xFFFF);

  SdHostWrite16(SdHostData, MMIO_ERINTEN, 0xFFFF);

  if (2 == CheckControllerVersion(SdHostData))
    temp16 =(UINT16)( 0x1<<6 ); // SD Controller 3.0 uses bit 6,bit7 for Divder bit 8, bit 9. 200M base clock needs 0x100.
  else
    temp16 = (UINT16) ( 0x80 << 8); // SD controller 2.0, 100M base clock needs 0x80

  // Set to 400KB, enable internal clock and wait for stability
  SdHostWrite32(SdHostData, MMIO_CLKCTL, (1<<0) | temp16);

  Delay15us(5 * 67);
  timeOut = 1000;
  do {
    tempD = SdHostRead32(SdHostData, MMIO_CLKCTL);
    timeOut --;
  } while ((!(tempD & (1<<1))) && (timeOut > 0));
  Delay15us(10 * 67);
  // Enable SD clock
  tempD |= (1<<2);
  SdHostWrite32(SdHostData, MMIO_CLKCTL, tempD);
  Delay15us(5 * 67);

  temp8 = SdHostRead8(SdHostData, MMIO_PWRCTL);
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, offset=0x%x, PWRCTL = 0x%x================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8));

  // Apply 1.8V to the bus
  temp8 = (0x5 <<(1));
  SdHostWrite8(SdHostData, MMIO_PWRCTL, temp8);
  Delay15us(5 * 67);
  temp8 = SdHostRead8(SdHostData, MMIO_PWRCTL);
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, set (0x5<<1):offset=0x%x, PWRCTL = 0x%x================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8));

    // Set 1.8V sigaling Enabled
    temp16 = SdHostRead16 (SdHostData, MMIO_HOST_CTL2);
    temp16 |= BIT3;
    SdHostWrite16(SdHostData, MMIO_HOST_CTL2, temp16);
    SD_DEBUG ((EFI_D_ERROR, "Set 1.8 V signaling Enable:0x%x \r\n", SdHostRead16 (SdHostData, MMIO_HOST_CTL2)));


  // Apply power to SD
  temp8 = SdHostRead8(SdHostData, MMIO_PWRCTL);
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, read offset=0x%x, PWRCTL = 0x%x================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8));

  temp8 |= (1<<(0));
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, set 1<<0:offset=0x%x, PWRCTL = 0x%x================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8));

  SdHostWrite8(SdHostData, MMIO_PWRCTL, temp8);
  Delay15us(5 * 67);
  temp8 = SdHostRead8(SdHostData, MMIO_PWRCTL);
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, read offset=0x%x, PWRCTL = 0x%x================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8));

  do {
    temp8 = SdHostRead8(SdHostData, MMIO_PWRCTL);
    Delay15us(67);
    timeOut --;
  } while ((!(temp8 & (1<<0))) && (timeOut > 0));
  SD_DEBUG((EFI_D_ERROR, "==========%a, %d, read offset=0x%x, PWRCTL = 0x%x, timeOut =%d================\n", __FUNCTION__, __LINE__, MMIO_PWRCTL, temp8, timeOut));

  // MAX out the DATA_TIMEOUT
  SdHostWrite8(SdHostData, MMIO_TOCTL, 0xE);
  Delay15us(5 * 67);

  if (SdHostData->EnableVerboseDebug) {
    SD_DEBUG((EFI_D_ERROR, "==========%a, Start. RegMap================\n", __FUNCTION__));
    SD_DEBUG ((EFI_D_INFO, "00 -10:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x00),SdHostRead32 (SdHostData, 0x04),SdHostRead32 (SdHostData, 0x08),SdHostRead32 (SdHostData, 0x0C)));
    SD_DEBUG ((EFI_D_INFO, "10 -20:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x10),SdHostRead32 (SdHostData, 0x14),SdHostRead32 (SdHostData, 0x18),SdHostRead32 (SdHostData, 0x1C)));
    SD_DEBUG ((EFI_D_INFO, "20 -30:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x20),SdHostRead32 (SdHostData, 0x24),SdHostRead32 (SdHostData, 0x28),SdHostRead32 (SdHostData, 0x2C)));
    SD_DEBUG ((EFI_D_INFO, "30 -40:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x30),SdHostRead32 (SdHostData, 0x34),SdHostRead32 (SdHostData, 0x38),SdHostRead32 (SdHostData, 0x3C)));
    SD_DEBUG ((EFI_D_INFO, "40 -50:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x40),SdHostRead32 (SdHostData, 0x44),SdHostRead32 (SdHostData, 0x48),SdHostRead32 (SdHostData, 0x4C)));
    SD_DEBUG ((EFI_D_INFO, "50 -60:  %08x  %08x  %08x  %08x \n", SdHostRead32 (SdHostData, 0x50),SdHostRead32 (SdHostData, 0x54),SdHostRead32 (SdHostData, 0x58),SdHostRead32 (SdHostData, 0x5C)));
    SD_DEBUG((EFI_D_ERROR, "==========%a, END. RegMap================\n", __FUNCTION__));
  }
  return EFI_SUCCESS;
}


