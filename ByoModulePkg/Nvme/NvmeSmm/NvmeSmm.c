/** @file
  Opal password smm driver which is used to support Opal security feature at s3 path.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "../NvmeDxe/NvmExpress.h"
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>

#define   MAX_NVME_CARD_SUPPORT 5

typedef struct _NVME_CONTROLLER_CARD_INFO {
  NVME_CONTROLLER_PRIVATE_DATA  *PrivateData;
  UINT16                        OpRomSeg;
} NVME_CONTROLLER_CARD_INFO;
NVME_CONTROLLER_CARD_INFO      gNVMEInfo[MAX_NVME_CARD_SUPPORT] = {0};

EFI_SMM_CPU_PROTOCOL           *gSmmCpu;
NVME_CONTROLLER_PRIVATE_DATA   *gPrivate = NULL;

typedef struct {
    UINT16    CF : 1;
    UINT16    Reserved1 : 1;
    UINT16    PF : 1;
    UINT16    Reserved2 : 1;
    UINT16    AF : 1;
    UINT16    Reserved3 : 1;
    UINT16    ZF : 1;
    UINT16    SF : 1;
    UINT16    TF : 1;
    UINT16    IF : 1;
    UINT16    DF : 1;
    UINT16    OF : 1;
    UINT16    IOPL : 2;
    UINT16    NT : 1;
    UINT16    Reserved4 : 1;
} FlagRegister;

typedef struct {
    UINT32    edi;
    UINT32    esi;
    UINT32    ebp;
    UINT32    esp;
    UINT8     bl, bh;
    UINT16    hWordEbx;
    UINT8     dl, dh;
    UINT16    hWordEdx;
    UINT8     cl, ch;
    UINT16    hWordEcx;
    UINT8     al, ah;
    UINT16    hWordEax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
} byteRegisterStackFrame;

typedef struct {
    UINT16    di;
    UINT16    hWordEdi;
    UINT16    si;
    UINT16    hWordEsi;
    UINT32    ebp;
    UINT32    esp;
    UINT16    bx;
    UINT16    hWordEbx;
    UINT16    dx;
    UINT16    hWordEdx;
    UINT16    cx;
    UINT16    hWordEcx;
    UINT16    ax;
    UINT16    hWordEax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
} wordRegisterStackFrame;

typedef struct {
    UINT32    edi;
    UINT32    esi;
    UINT32    ebp;
    UINT32    esp;
    UINT32    ebx;
    UINT32    edx;
    UINT32    ecx;
    UINT32    eax;
    UINT16    ds;
    UINT16    es;
    UINT16    AccessOrder;
    UINT16    Reserved;
    FlagRegister    flags;
}dwordRegisterStackFrame;

typedef union{
    byteRegisterStackFrame    L;
    wordRegisterStackFrame    H;
    dwordRegisterStackFrame   X;
}registerStackFrame;

typedef struct {
    UINT16    wSize;
    UINT16    wInforFlag;
    UINT32    dwCylinder;
    UINT32    dwHeaders;
    UINT32    dwSector;
    UINT64    qwSectos;
    UINT16    wBytesInSector;
    VOID      *lpDpte;
} RESULT_BUF;

typedef struct {
    UINT8    bSize;
    UINT8    bRsvd;
    UINT16   wNumOfTran;
    UINT32   dwBuf;
    UINT64   qwLba;
} INT13_DISK_PACKET;



VOID*
NvmeCreatePrpListSmm (
  IN     NVME_CONTROLLER_PRIVATE_DATA   *Private,
  IN     EFI_PHYSICAL_ADDRESS           PhysicalAddr,
  IN     UINTN                          Pages
  )
{
  UINTN                       PrpEntryNo;
  UINT64                      PrpListBase;
  UINTN                       PrpListIndex;
  UINTN                       PrpEntryIndex;
  UINT64                      Remainder;
  UINTN                       Bytes;
  UINTN                       PrpListNo;
  UINT64                      *PrpListHost;

  // DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
  if (gPrivate->PrpListHost2 == NULL) {
    goto ProcExit;
  }
  //
  // The number of Prp Entry in a memory page.
  //
  PrpEntryNo = EFI_PAGE_SIZE / sizeof (UINT64);

  //
  // Calculate total PrpList number.
  //
  PrpListNo = (UINTN)DivU64x64Remainder ((UINT64)Pages, (UINT64)PrpEntryNo - 1, &Remainder);
  if (PrpListNo == 0) {
    PrpListNo = 1;
  } else if ((Remainder != 0) && (Remainder != 1)) {
    PrpListNo += 1;
  } else if (Remainder == 1) {
    Remainder = PrpEntryNo;
  } else if (Remainder == 0) {
    Remainder = PrpEntryNo - 1;
  }

  if(PrpListNo > 1 && Remainder != 0){
    goto ProcExit;
  }

  PrpListHost = (UINT64*)Private->PrpListHost2;
  Bytes = EFI_PAGES_TO_SIZE (PrpListNo);

  ZeroMem (PrpListHost, Bytes);
  for (PrpListIndex = 0; PrpListIndex < PrpListNo - 1; ++PrpListIndex) {
    PrpListBase = (UINT64)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;

    for (PrpEntryIndex = 0; PrpEntryIndex < PrpEntryNo; ++PrpEntryIndex) {
      if (PrpEntryIndex != PrpEntryNo - 1) {
        //
        // Fill all PRP entries except of last one.
        //
        *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = PhysicalAddr;
        PhysicalAddr += EFI_PAGE_SIZE;
      } else {
        //
        // Fill last PRP entries with next PRP List pointer.
        //
        *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = *PrpListHost + (PrpListIndex + 1) * EFI_PAGE_SIZE;
      }
    }
  }
  //
  // Fill last PRP list.
  //
  PrpListBase = (UINT64)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;
  for (PrpEntryIndex = 0; PrpEntryIndex < Remainder; ++PrpEntryIndex) {
    *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = PhysicalAddr;
    PhysicalAddr += EFI_PAGE_SIZE;
  }

  return (VOID*)(UINTN)PrpListHost;

ProcExit:
  return NULL;
}


STATIC VOID DumpMem32(VOID *Base, UINTN Size)
{
  UINTN  Index;
  UINTN  Addr;

  Addr = (UINTN)Base;
  Addr &= ~0x3;

  DEBUG((EFI_D_ERROR, "%a(%X,%X)\n", __FUNCTION__, Base, Size));
  for (Index=0; Index < Size; Index+=4) {
    if(Index%16==0){
      DEBUG((EFI_D_ERROR, "%08X: ", Addr+Index));
    }
    DEBUG((EFI_D_ERROR, "%08X ", MmioRead32(Addr+Index)));
    if((Index+4)%16==0){
      DEBUG((EFI_D_ERROR, "\n"));
    }
  }

  DEBUG((EFI_D_ERROR, "\n"));  
}


EFI_STATUS
EFIAPI
NvmExpressPassThruSmm (
  IN     NVME_CONTROLLER_PRIVATE_DATA                *Private,
  IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    *Packet
  )
{

  EFI_STATUS                    Status;
  NVME_SQ                       *Sq;
  NVME_CQ                       *Cq;
  UINT8                         QueueType;
  UINT32                        Bytes;
  UINT16                        Offset;
  EFI_EVENT                     TimerEvent;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  UINT64                        *Prp;
  UINT32                        Data;
  UINTN                         TimeOut;


  // DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  if ((Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((Packet->NvmeCmd == NULL) || (Packet->NvmeCompletion == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Packet->QueueType != NVME_ADMIN_QUEUE && Packet->QueueType != NVME_IO_QUEUE) {
    return EFI_INVALID_PARAMETER;
  }

  Prp        = NULL;
  TimerEvent = NULL;
  Status     = EFI_SUCCESS;

  QueueType = Packet->QueueType;
  Sq  = Private->SqBuffer[QueueType] + Private->SqTdbl[QueueType].Sqt;
  Cq  = Private->CqBuffer[QueueType] + Private->CqHdbl[QueueType].Cqh;

  ZeroMem (Sq, sizeof(NVME_SQ));
  Sq->Opc  = (UINT8)Packet->NvmeCmd->Cdw0.Opcode;
  Sq->Fuse = (UINT8)Packet->NvmeCmd->Cdw0.FusedOperation;
  Sq->Cid  = Private->Cid[QueueType]++;
  Sq->Nsid = Packet->NvmeCmd->Nsid;

//ASSERT (Sq->Psdt == 0);      // should have asserted in dxe driver.
  if (Sq->Psdt != 0) {
    DEBUG ((EFI_D_ERROR, "NvmExpressPassThru: doesn't support SGL mechanism\n"));
    return EFI_UNSUPPORTED;
  }

  Sq->Prp[0] = (UINT64)(UINTN)Packet->TransferBuffer;
  //
  // If the NVMe cmd has data in or out, then mapping the user buffer to the PCI controller specific addresses.
  // Note here we don't handle data buffer for CreateIOSubmitionQueue and CreateIOCompletionQueue cmds because
  // these two cmds are special which requires their data buffer must support simultaneous access by both the
  // processor and a PCI Bus Master. It's caller's responsbility to ensure this.
  //
  if (((Sq->Opc & (BIT0 | BIT1)) != 0) && (Sq->Opc != NVME_ADMIN_CRIOCQ_CMD) && (Sq->Opc != NVME_ADMIN_CRIOSQ_CMD)) {

    Sq->Prp[0] = (UINTN)Packet->TransferBuffer;
    Sq->Prp[1] = 0;

    if(Packet->MetadataBuffer != NULL) {
      Sq->Mptr = (UINTN)Packet->MetadataBuffer;
    }
  }
  
  //
  // If the buffer size spans more than two memory pages (page size as defined in CC.Mps),
  // then build a PRP list in the second PRP submission queue entry.
  //
  Offset = ((UINT16)Sq->Prp[0]) & (EFI_PAGE_SIZE - 1);
  Bytes  = Packet->TransferLength;

  if ((Offset + Bytes) > (EFI_PAGE_SIZE * 2)) {
    //
    // Create PrpList for remaining data buffer.
    //
    PhyAddr = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
    Prp = NvmeCreatePrpListSmm (Private, PhyAddr, EFI_SIZE_TO_PAGES(Offset + Bytes) - 1);
    if (Prp == NULL) {
      goto ProcExit;
    }

    Sq->Prp[1] = (UINT64)(UINTN)Prp;
  } else if ((Offset + Bytes) > EFI_PAGE_SIZE) {
    Sq->Prp[1] = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
  }

  if(Packet->NvmeCmd->Flags & CDW2_VALID) {
    Sq->Rsvd2 = (UINT64)Packet->NvmeCmd->Cdw2;
  }
  if(Packet->NvmeCmd->Flags & CDW3_VALID) {
    Sq->Rsvd2 |= LShiftU64 ((UINT64)Packet->NvmeCmd->Cdw3, 32);
  }
  if(Packet->NvmeCmd->Flags & CDW10_VALID) {
    Sq->Payload.Raw.Cdw10 = Packet->NvmeCmd->Cdw10;
  }
  if(Packet->NvmeCmd->Flags & CDW11_VALID) {
    Sq->Payload.Raw.Cdw11 = Packet->NvmeCmd->Cdw11;
  }
  if(Packet->NvmeCmd->Flags & CDW12_VALID) {
    Sq->Payload.Raw.Cdw12 = Packet->NvmeCmd->Cdw12;
  }
  if(Packet->NvmeCmd->Flags & CDW13_VALID) {
    Sq->Payload.Raw.Cdw13 = Packet->NvmeCmd->Cdw13;
  }
  if(Packet->NvmeCmd->Flags & CDW14_VALID) {
    Sq->Payload.Raw.Cdw14 = Packet->NvmeCmd->Cdw14;
  }
  if(Packet->NvmeCmd->Flags & CDW15_VALID) {
    Sq->Payload.Raw.Cdw15 = Packet->NvmeCmd->Cdw15;
  }

  // DumpMem32(Sq, sizeof(NVME_SQ));

  //
  // Ring the submission queue doorbell.
  //
  Private->SqTdbl[QueueType].Sqt ^= 1;
  Data = ReadUnaligned32 ((UINT32*)&Private->SqTdbl[QueueType]);
  MmioWrite32(Private->NvmeBar + NVME_SQTDBL_OFFSET(QueueType, Private->Cap.Dstrd), Data);

  //DEBUG((EFI_D_INFO, "(L%d)\n", __LINE__));

  TimeOut = 3000;
  while (TimeOut--) {
    if (Cq->Pt != Private->Pt[QueueType]) {
      Status = EFI_SUCCESS;
      break;
    }
    MicroSecondDelay(1000);
  }
  if(TimeOut == 0){
    Status = EFI_TIMEOUT;
  }

  //DEBUG((EFI_D_INFO, "(L%d)\n", __LINE__));

  if (Status != EFI_TIMEOUT) {
    if ((Cq->Sct == 0) && (Cq->Sc == 0)) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_DEVICE_ERROR;
      CopyMem(Packet->NvmeCompletion, Cq, sizeof(EFI_NVM_EXPRESS_COMPLETION));
    }
  }
  if ((Private->CqHdbl[QueueType].Cqh ^= 1) == 0) {
    Private->Pt[QueueType] ^= 1;
  }

  Data = ReadUnaligned32((UINT32*)&Private->CqHdbl[QueueType]);
  MmioWrite32(Private->NvmeBar + NVME_CQHDBL_OFFSET(QueueType, Private->Cap.Dstrd), Data);

  //DEBUG((EFI_D_INFO, "(L%d)\n", __LINE__));

ProcExit:
  return Status;
}


registerStackFrame *
PointCommBuf (
  IN    VOID
  )
{
  registerStackFrame    *CommBuf;
  UINT16                *pSegOfEbda;
  UINT32                mToEbda;

  pSegOfEbda = (UINT16 *)(UINTN)0x40E;
  mToEbda    = (UINT32)(((UINTN)(*pSegOfEbda) << 4) + 0x90);
  CommBuf    = (registerStackFrame *)(UINTN)mToEbda;

  return CommBuf;
}

UINT16
GetOpRomSeg ()
{
  UINT16                *pSegOfEbda;
  UINT32                mToEbda;
  UINT16                OpRomSeg;
  pSegOfEbda = (UINT16 *)(UINTN)0x40E;
  mToEbda    = (UINT32)(((UINTN)(*pSegOfEbda) << 4) + 0x142);
  OpRomSeg   = *(UINT16 *)(UINTN)mToEbda;

  return OpRomSeg;
}

EFI_STATUS
NvmeReadWriteSectors (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT64                        Buffer,
  IN UINT64                        Lba,
  IN UINT32                        Blocks,
  IN BOOLEAN                       ReadFlag
  )
{
  UINT32                                   Bytes;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                  Command;
  EFI_NVM_EXPRESS_COMPLETION               Completion;
  EFI_STATUS                               Status;


  //DEBUG((EFI_D_INFO, "Read(%lX,%lX,%X)\n", Buffer, Lba, Blocks));

  if(Private->BlockInfoCount == 0){
    return EFI_NOT_READY;
  }

  Bytes = Blocks * Private->BlockInfo[0].BlockSize;

  ZeroMem (&CommandPacket, sizeof(EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof(EFI_NVM_EXPRESS_COMPLETION));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  CommandPacket.NvmeCmd->Cdw0.Opcode = ReadFlag ? NVME_IO_READ_OPC : NVME_IO_WRITE_OPC;
  CommandPacket.NvmeCmd->Nsid  = Private->BlockInfo[0].NamespaceId;
  CommandPacket.TransferBuffer = (VOID *)(UINTN)Buffer;

  CommandPacket.TransferLength = Bytes;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_IO_QUEUE;

  CommandPacket.NvmeCmd->Cdw10 = (UINT32)Lba;
  CommandPacket.NvmeCmd->Cdw11 = (UINT32)RShiftU64(Lba, 32);
  //
  // Set Force Unit Access bit (bit 30) to use write-through behaviour
  //
  CommandPacket.NvmeCmd->Cdw12 = ((Blocks - 1) & 0xFFFF) | BIT30;

  CommandPacket.MetadataBuffer = NULL;
  CommandPacket.MetadataLength = 0;

  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID | CDW12_VALID;

  Status = NvmExpressPassThruSmm(Private, &CommandPacket);

  return Status;
}




EFI_STATUS
NvmeRwSmm (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  EFI_STATUS            Status;
  registerStackFrame    *reg;
  EFI_LBA               Lba;
  UINT64                BufferToReadWrite;
  RESULT_BUF            *Result;
  INT13_DISK_PACKET     *Packet;
  UINT32                Sector;
  UINT32                Header;
  UINT32                Cylinder;
  UINT16                SectorCnt;
  UINT32                NumOfSector;
  UINT32                NumOfHead;
  UINT32                NumOfCylinder;
  UINTN                 StrLen;
  UINT32                BlockSize;
  UINT16                MaxTransferBlocks;
  BOOLEAN               ReadFlag = TRUE;
  UINT16                OpRomSeg;
  UINT8                 Index;
  static UINT8          NvmeCount = 0;

  // DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
  reg = PointCommBuf();
  // ax = 0xFFFF;  Get Sn & Mn
  // bx = OpromSeg;
  // cx = OffsetSn;
  // dx = OffsetMn;
  if (reg->H.ax == 0xffff) {
    if (NvmeCount >= MAX_NVME_CARD_SUPPORT) {
      return EFI_ABORTED;
    }
    gPrivate = gNVMEInfo[NvmeCount].PrivateData;
    if(gPrivate == NULL || gPrivate->Signature != NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE){
      return EFI_NOT_READY;
    }
    BufferToReadWrite = reg->H.bx * 16 + reg->H.cx;
    AsciiStrCpy((CHAR8*)(UINTN)BufferToReadWrite, gPrivate->Sn);
    StrLen = AsciiStrSize (gPrivate->Mn);
    if (StrLen > 32) {
      StrLen = 31;
    }
    BufferToReadWrite = reg->H.bx * 16 + reg->H.dx;
    AsciiStrnCpy ((CHAR8*)(UINTN)BufferToReadWrite, gPrivate->Mn, StrLen);
    gNVMEInfo[NvmeCount ++].OpRomSeg = reg->H.bx;
    return  EFI_SUCCESS;
  }
  OpRomSeg = GetOpRomSeg ();
  gPrivate = NULL;
  for (Index = 0; Index < NvmeCount; Index ++) {
    if (gNVMEInfo[Index].OpRomSeg == OpRomSeg) {
      gPrivate = gNVMEInfo[Index].PrivateData;
      break;
    }
  }
  if(gPrivate == NULL || gPrivate->Signature != NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE){
    return EFI_NOT_READY;
  }
  if(gPrivate->BlockInfoCount == 0){
    return EFI_NOT_READY;
  }
  //DEBUG ((EFI_D_ERROR, "reg->H.ax = %x \n", reg->H.ax));
  //DEBUG ((EFI_D_ERROR, "reg->H.bx = %x \n", reg->H.bx));
  //DEBUG ((EFI_D_ERROR, "reg->H.cx = %x \n", reg->H.cx));
  //DEBUG ((EFI_D_ERROR, "reg->H.dx = %x \n", reg->H.dx));
  //DEBUG ((EFI_D_ERROR, "reg->L.es = %x \n", reg->L.es));
	//DEBUG ((EFI_D_ERROR, "OpRomSeg = %x \n", OpRomSeg));

  Status        = EFI_SUCCESS;
  NumOfCylinder = 0x3ff;
  NumOfHead     = 0xfe;
  NumOfSector   = 0x3f;

  Header    = (UINT32)(reg->L.dh);
  Sector    = (UINT32)(reg->L.cl & 0x3F);
  Cylinder  = ((UINT32)(reg->L.cl & 0xC0)) * 4 + (UINT32) (reg->L.ch);
  SectorCnt = (UINT16)reg->L.al;

  switch(reg->L.ah) {
    case 0x02:
    case 0x03:
      BufferToReadWrite = reg->L.es * 16 + reg->H.bx;
      Lba  = (UINT64) (((Cylinder * (NumOfHead + 1)) + Header) * NumOfSector) + Sector - 1;
      goto ReadWrite;

    case 0x08:
      reg->H.ax    = 0;
      reg->H.cx    = 0xFEFF;
      reg->L.dl    = *(UINT8 *)(UINTN)(0x475);
      reg->L.dh    = (UINT8)NumOfHead;
      break;

    case 0x15:
      reg->L.ah = 0x03;
      reg->H.cx = 0xffff;
      reg->H.dx = 0xffff;
      break;

    case 0x42:
    case 0x43:
      Packet = (INT13_DISK_PACKET *)(UINTN)(reg->L.ds * 16 + reg->H.si);
      if ((Packet->bSize != 0x10) ||(Packet->bRsvd != 0)) {
        reg->L.ah       = 0x01;
        break;
      }
      SectorCnt         = Packet->wNumOfTran;
      Lba               = Packet->qwLba;
      BufferToReadWrite = (((Packet->dwBuf >> 12) & 0xFFFF0) + (Packet->dwBuf & 0xFFFF));

    ReadWrite:
      if (SectorCnt == 0) {
        reg->L.ah = 0;
        break;
      }
      
      //DEBUG ((DEBUG_INFO, "SectorCnt = %X \n", SectorCnt));
      //DEBUG ((DEBUG_INFO, "Buffer = %X \n", BufferToReadWrite));
      //DEBUG ((DEBUG_INFO, "Lba = %X \n", Lba));
      if ((reg->L.ah == 0x03) || (reg->L.ah == 0x43)) {
        ReadFlag = FALSE;
      }
      BlockSize = gPrivate->BlockInfo[0].BlockSize;
      MaxTransferBlocks = gPrivate->MaxTransferBlocks;
      while (SectorCnt > 0) {
        if (SectorCnt > MaxTransferBlocks) {
          Status     = NvmeReadWriteSectors(gPrivate, BufferToReadWrite, Lba, MaxTransferBlocks, ReadFlag);
          SectorCnt -= MaxTransferBlocks;
          Lba       += MaxTransferBlocks;
          BufferToReadWrite += MaxTransferBlocks * BlockSize;
        } else {
          Status = NvmeReadWriteSectors(gPrivate, BufferToReadWrite, Lba, SectorCnt, ReadFlag);
          SectorCnt = 0;
        }
        if (EFI_ERROR(Status)) {
          break;
        }
      }
      reg->L.ah = (UINT8)(Status != EFI_SUCCESS);
      // DEBUG((DEBUG_INFO, "(L%d) %r\n", __LINE__, Status));
      break;

    case 0x48:
      Result = (RESULT_BUF *)(UINTN)(reg->L.ds * 16 + reg->H.si);
      if (Result->wSize < 26) {
        reg->L.ah = 0x01;
        break;
      }
      reg->L.ah = 0;
      if (Result->wSize < 30) {
        Result->wSize  = 26;
      } else {
        Result->wSize  = 30;
      }
      Result->wInforFlag     = 0x1;
      Result->dwCylinder     = (UINT32) NumOfCylinder;
      Result->dwHeaders      = (UINT32) NumOfHead;
      Result->dwSector       = (UINT32) NumOfSector;
      Result->qwSectos       = gPrivate->BlockInfo[0].LastBlock + 1;
      Result->wBytesInSector = (UINT16)gPrivate->BlockInfo[0].BlockSize;
      break;

    default:
      reg->L.ah = 0x01;
      Status = EFI_UNSUPPORTED;
      break;
  }
  
  reg->X.flags.CF = (UINT16)(Status != EFI_SUCCESS);
  
  return EFI_SUCCESS;  
}



EFI_STATUS
NvmeSmmInit (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  UINTN                        CpuIndex;
  EFI_SMM_SAVE_STATE_IO_INFO   IoState;
  UINT32                       RegEbx;
  static UINT8                 NvmeCount = 0;
  NVME_CONTROLLER_PRIVATE_DATA *NvmePrivate = NULL;

  if (NvmeCount >= MAX_NVME_CARD_SUPPORT) {
    return EFI_ABORTED;
  }
  CpuIndex = 0;
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Status = gSmmCpu->ReadSaveState (
                        gSmmCpu,
                        sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                        EFI_SMM_SAVE_STATE_REGISTER_IO,
                        Index,
                        &IoState
                        );
    if (!EFI_ERROR (Status) && (IoState.IoData == 0x81)) {
      CpuIndex = Index;
      break;
    }
  }
  if (Index >= gSmst->NumberOfCpus) {
    CpuDeadLoop ();
  }

  Status = gSmmCpu->ReadSaveState (
                      gSmmCpu,
                      sizeof(UINT32),
                      EFI_SMM_SAVE_STATE_REGISTER_RBX,
                      CpuIndex,
                      &RegEbx
                      );
  ASSERT_EFI_ERROR (Status);

  NvmePrivate = (NVME_CONTROLLER_PRIVATE_DATA*)(UINTN)RegEbx;
  if (((RegEbx & 0xFFF) == 0) && (NvmePrivate->Signature == NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE)) {
    gNVMEInfo[NvmeCount ++].PrivateData = NvmePrivate;
  }

  return EFI_SUCCESS;
}



/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
NvmeSmmEntry (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                            Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL         *SwDispatch;
  EFI_HANDLE                            SwHandle = NULL;
  EFI_SMM_SW_REGISTER_CONTEXT           Context;


  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&gSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "(L%d) %r\n", __LINE__, Status));
    return Status;
  }

  Context.SwSmiInputValue = NVME_LEGACY_BOOT_SMI;
  Status = SwDispatch->Register (
                         SwDispatch,
                         NvmeRwSmm,
                         &Context,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "(L%d) %r\n", __LINE__, Status));
    goto ProcExit;
  }

  Context.SwSmiInputValue = NVME_LEGACY_INIT_SMI;
  Status = SwDispatch->Register (
                         SwDispatch,
                         NvmeSmmInit,
                         &Context,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "(L%d) %r\n", __LINE__, Status));
    goto ProcExit;
  }

ProcExit:
  return Status;
}


