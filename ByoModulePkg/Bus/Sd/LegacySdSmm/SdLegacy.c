/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SdLegacy.c

Abstract: 
  SD card Model.

Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

  Bug 2026: Description of this bug.
  TIME: 2011-05-16
  $AUTHOR: Mac Peng
  $REVIEWERS: Donald Guan
  $SCOPE: SD card feature support.
  $TECHNICAL: .
  $END--------------------------------------------------------------------------

**/

#include "SdHostDriver.h"
#include "MediaDeviceDriver.h"
#include "SdLegacy.h"
#include <Protocol/PciIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SmmBase2.h>
#include <protocol/SwSmiValuePolicyData.h>

EFI_GUID                      gSdCardDataGuid = EFI_SD_CARD_DATA_GUID;
BOOLEAN                       UpdateBBS = FALSE;
BOOLEAN                       LegacySDStart = FALSE;
SD_MEMORY_CARD_INFO           SdData[5]={0};
UINT8                         *mOneSector;
EFI_HANDLE                    mLegacySDHandle = NULL;
volatile SD_SMMCALL_COMM      *mSmmCallComm;
SMMCALL_ENTRY                 *mSmmCallTablePtr;
SMMCALL_ENTRY                 SdSmmCallTable[__ID_LastSmmCall];
UINT8                         SdmemCardQty = 0;
//------------------------------------------------------------------------------
// DEBUG helper
//------------------------------------------------------------------------------
int _inp (unsigned short port);
int _outp (unsigned short port, int databyte );
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)


EFI_STATUS
SmmSingleSegmentPciAccess (
  IN     EFI_SMM_CPU_IO2_PROTOCOL    *CpuIo,
  IN     BOOLEAN                     IsWrite,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH   Width,
  IN     SMM_PCI_IO_ADDRESS          *Address,
  IN OUT VOID                        *Buffer
)
{
  EFI_STATUS            Status;
  PCI_CONFIG_ACCESS_CF8 PciCf8Data;
  UINT64                PciDataReg;
  UINT32                AddressPort;

  Status = CpuIo->Io.Read (CpuIo, EfiPciIoWidthUint32, 0xcf8, 1, &AddressPort);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciCf8Data.Bits.Reg      = Address->Register & 0xfc;
  PciCf8Data.Bits.Func     = Address->Function;
  PciCf8Data.Bits.Dev      = Address->Device;
  PciCf8Data.Bits.Bus      = Address->Bus;
  PciCf8Data.Bits.Reserved = 0;
  PciCf8Data.Bits.Enable   = 1;

  Status = CpuIo->Io.Write (CpuIo, EfiPciIoWidthUint32, 0xcf8, 1, &PciCf8Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciDataReg = 0xcfc + (Address->Register & 0x03);

  if (IsWrite) {
    //
    // This is a Pci write operation, write data into (0xcfc + offset)
    //
    Status = CpuIo->Io.Write (CpuIo, Width, PciDataReg, 1, Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // This is a Pci Read operation, read returned data from (0xcfc + offset)
    //
    Status = CpuIo->Io.Read (CpuIo, Width, PciDataReg, 1, Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = CpuIo->Io.Write (CpuIo, EfiPciIoWidthUint32, 0xcf8, 1, &AddressPort);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
SmmPciCfgRead (
  IN     EFI_SMM_SYSTEM_TABLE2     *Smst,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH Width,
  IN     SMM_PCI_IO_ADDRESS        *Address,
  IN OUT VOID                      *Buffer
)

{
  EFI_SMM_CPU_IO2_PROTOCOL  *SmmCpuIo;

  SmmCpuIo = &(Smst->SmmIo);

  return SmmSingleSegmentPciAccess (SmmCpuIo, FALSE, Width, Address, Buffer);
}


/**
  Start legacy SD driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of 
                                   resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
StartLegacySd (
  )
{
  EFI_STATUS    Status;
  UINT8         PciBusNum;
  UINT8         PciDeviceNum;
  UINT8         PciFunctionNum;
  UINT32        MemAddress;
  UINT64        PciAddress;
  UINT32        PciClassCode;
  UINT32        PciDidVid;

  if (LegacySDStart == FALSE) {
    LegacySDStart = TRUE;
    for (PciBusNum = 0; PciBusNum < MAX_PCI_BUSS; PciBusNum ++) {
      for (PciDeviceNum = 0; PciDeviceNum < MAX_PCI_DEVICES; PciDeviceNum ++) {
        for (PciFunctionNum = 0; PciFunctionNum < MAX_PCI_FUNCTIONS; PciFunctionNum ++) {
          //
          //  Add for check Pci Dev exist.
          //
          PciAddress = SMM_PCI_ADDRESS (PciBusNum, PciDeviceNum, PciFunctionNum, 0);
          Status = SmmPciCfgRead (
                     gSmst,
                     EfiPciIoWidthUint32,
                     (SMM_PCI_IO_ADDRESS *) &PciAddress,
                     &PciDidVid
                     );
          if (EFI_ERROR (Status) || (UINT16)PciDidVid == 0xFFFF) {
            if (PciFunctionNum == 0) {
              PciFunctionNum = MAX_PCI_FUNCTIONS;
            }
            continue;
          }
/*          
          if (!(PciDidVid == 0x0F148086 || 
                PciDidVid == 0x0F508086 || 
                PciDidVid == 0x0F168086 || 
                PciDidVid == 0x22948086 || 
                PciDidVid == 0x22968086 ||
                PciDidVid == 0x95D01106
                )){
            continue;
          }
*/          
          PciAddress = SMM_PCI_ADDRESS (PciBusNum, PciDeviceNum, PciFunctionNum, PCI_REVISION_ID_OFFSET);
          Status = SmmPciCfgRead (
                     gSmst,
                     EfiPciIoWidthUint32,
                     (SMM_PCI_IO_ADDRESS *) &PciAddress,
                     &PciClassCode
                    );
          if (EFI_ERROR(Status)) {
            continue;
          }
          PciClassCode &= 0x0FFFFFF00;
          if (PciClassCode == PCI_SDIO_CLASS1 || PciClassCode == PCI_SDIO_CLASS2) {
            PciAddress = SMM_PCI_ADDRESS (PciBusNum, PciDeviceNum, PciFunctionNum, PCI_BASE_ADDRESSREG_OFFSET);
            Status = SmmPciCfgRead (
                       gSmst,
                       EfiPciIoWidthUint32,
                       (SMM_PCI_IO_ADDRESS *) &PciAddress,
                       &MemAddress
                      );
            Status = SdHostDriverStart(PciBusNum, PciDeviceNum, PciFunctionNum, MemAddress, PciDidVid);
          }  
        }
      }
    }
  }
  return EFI_SUCCESS;
}

/**
  Dispatch function for a Software SMI handler.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
SdSmmCallEntry (
  IN  EFI_HANDLE                    DispatchHandle,
  IN CONST VOID                    *Context,
  IN OUT VOID                       *CommBuffer,
  IN OUT UINTN                      *CommBufferSize  
  )
{
  SMMCALL_ENTRY SmmCallFunc;
  EFI_SMM_SW_REGISTER_CONTEXT   *DispatchContext;
  DispatchContext = (EFI_SMM_SW_REGISTER_CONTEXT *)Context;
  if (DispatchContext->SwSmiInputValue == SD_SMI_VALUE) {
    SmmCallFunc = SdSmmCallTable[mSmmCallComm->CallId];

    switch (mSmmCallComm->Argc) {
    case 0:
      *(mSmmCallComm->Return) = SmmCallFunc();
      break;
	  
    case 1:
      *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0]);
      break;

    case 2:
      *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1]);
      break;

    case 9:
      *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
        mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
        mSmmCallComm->Argv[6], mSmmCallComm->Argv[7], mSmmCallComm->Argv[8]);
      break;

    default:
      *(mSmmCallComm->Return) = EFI_UNSUPPORTED;
    }
  }
    return EFI_SUCCESS;
}

VOID ConstructSmmCallTable(VOID)
{
  INIT_SMMCALL(DetectCardAndInitHost);
  INIT_SMMCALL(SetClockFrequency);
  INIT_SMMCALL(SetBusWidth);
  INIT_SMMCALL(SetHostVoltage);
  INIT_SMMCALL(SetHostDdrMode);
  INIT_SMMCALL(ResetSdHost);
  INIT_SMMCALL(EnableAutoStopCmd);
  INIT_SMMCALL(SetBlockLength);
  INIT_SMMCALL(SendHCommand);
  INIT_SMMCALL(StartLegacySd);
  INIT_SMMCALL(SetupDevice);
  INIT_SMMCALL(SetHostSpeedMode);
}

/**
  Add SD card into BBS table.

  @param[in]  pBbsTable                   A pointer to the BBS_TABLE.

  @retval SDHDDCount + SDFDDCount         Device count.

**/
UINT16
SdUpdateBbs(
  BBS_TABLE     *pBbsTable
  )
{
  EFI_STATUS                Status;
  BBS_TABLE                 *p_BbsTable;
  PARTITION_AREA            *partition;
  UINT8                     i,j,PartitionFound;
  UINT16                    head,sector,cylinder;
  UINT16                    pfa;
  UINT8                     SDHDDCount;
  UINT8                     SDFDDCount;
  UINT64                    TempCylinder;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;

  //_outp(0x80, 0xF1);
  //CpuDeadLoop();
  head = sector = cylinder = pfa = 0;
  SDFDDCount = SDHDDCount = 0;
  for (i = 0 ; i < (UINT8)SdmemCardQty; i++) {
    // default C.H.S.
    SdData[i].BytesPerSector = 512;
    SdData[i].SectorsPerHead = 63;
    SdData[i].TotalHeads     = 0x7F;

    BlockIo = &(SdData[i].SDCardData->Partitions[0].BlockIo);
    Status = BlockIo->ReadBlocks(BlockIo, 0, 0, 512, &mOneSector[0]);
    DEBUG ((EFI_D_ERROR, "Read MBR or DBR status:%r\n",Status));
    if (!EFI_ERROR (Status)) {
      if ((mOneSector[PARTITION_SIG1_OFFSET] == 0x55) && (mOneSector[PARTITION_SIG2_OFFSET] == 0xAA)) {
        //DEBUG ((EFI_D_ERROR, "SDUpdateBBS ReadBlocks MBR OR DBR EFI_SUCCESS\n"));
        //MBR or DBR is valid,check partition table
        partition = (PARTITION_AREA *)&(mOneSector[FIRST_PARTITION_OFFSET]);
        head      = 0;
        sector    = 0x3f;
        for (PartitionFound = 1,j = 0; j < 4; j++) {
          head     = (UINT16)(partition->EndingHead);
          cylinder = (UINT16)(((partition->EndCyl& 0x3) << 8) | (partition->EndCyl >> 2));
          sector   = (UINT16)(partition->EndSector);
          if ((partition->BootIndicator != 0x80) && (partition->BootIndicator != 0)) {
            PartitionFound = 0;
            break;
          }
        }
        if (PartitionFound) {
          SdData[i].BootAsFixedDrive = TRUE;
          if (head == 0) {
            // partition is invalid or no partition, using default C.H.S
          } else {
            // valid partition found
            head++;
            SdData[i].TotalHeads     = (UINT8)head;
            SdData[i].SectorsPerHead = (UINT8)sector;
            SdData[i].Cylinder       = cylinder;
            SdData[i].TotalLBA       = partition->PartitionTotalLBA;
          }
        } else {
          // this sd card as floppy,Check FAT16/FAT12/FAT32
          if ((mOneSector[0x26] == 0x29) || (mOneSector[0x42] == 0x29)) {
            TempCylinder = MultU64x32(head, sector);
            if (TempCylinder) {
              TempCylinder = DivU64x32(partition->PartitionTotalLBA, (UINT32)TempCylinder);
            }
            head                     = mOneSector[0x1A];
            sector                   = mOneSector[0x18];
            cylinder                 = (UINT16)TempCylinder;
            SdData[i].TotalHeads     = mOneSector[0x1A];
            SdData[i].SectorsPerHead = mOneSector[0x18];
            SdData[i].Cylinder       = cylinder;
            SdData[i].TotalLBA       = MultU64x32(MultU64x32(mOneSector[0x1A], mOneSector[0x18]), cylinder);
          }
          SdData[i].BootAsFixedDrive = FALSE;
        }
      } else {
        // default to hard disk;
        SdData[i].BootAsFixedDrive = TRUE;
      }
    } else {
      // default to hard disk type;
      SdData[i].BootAsFixedDrive = TRUE;
    }
    // add device into bbs table
    pfa                      = SdData[i].SDCardData->SdHostIo->PFA;
    p_BbsTable               = &(((BBS_TABLE *)(pBbsTable))[i + BBS_SDMEM_RESERVATION_START_INDEX]);
    p_BbsTable->BootPriority = BBS_UNPRIORITIZED_ENTRY;
    p_BbsTable->Bus          = (UINT32)(pfa >> 8);
    p_BbsTable->Device       = (UINT32)((pfa >> 3) & 0x1F);
    p_BbsTable->Function     = (UINT32)(pfa & 0x07);
    p_BbsTable->Class        = 0x08;
    p_BbsTable->SubClass     = 0x05;
    if (SdData[i].BootAsFixedDrive) {
      p_BbsTable->DeviceType = BBS_HARDDISK;
      SDHDDCount ++;
    } else {
      p_BbsTable->DeviceType = BBS_FLOPPY;
      SDFDDCount ++;
    }
    p_BbsTable->IBV1 = 1;//mean SD mem card
    p_BbsTable->IBV2 = i;
  }
  return (SDHDDCount * 0x100 + SDFDDCount);
}

/**
  Select access device.

  @param[in]  CurrentSdPrivateData        A pointer to the SD_MEMORY_CARD_INFO.
  @param[in]  Index                       The index of the device in the device table.
  @param[in]  Type                        Type < 0x80 is FDD device, or is HDD device.

  @retval i                               The index of the device in the same device table.

**/
UINT8
SelectDevice(
  SD_MEMORY_CARD_INFO     *CurrentSdPrivateData,
  UINT16                  Index,
  UINT8                   Type
)
{
  UINT8     i;
  BOOLEAN   BootAsFixedDrive;
  if (Type < 0x80) {
    BootAsFixedDrive = FALSE;
  } else {
    BootAsFixedDrive = TRUE;
  }
  for (i = 0; i < 5; i ++) {
    if (CurrentSdPrivateData->BootAsFixedDrive == BootAsFixedDrive) {
      if (Index == 0) {
        break;
      }
      Index -- ;
    }
  }
  return i;
}

/**
  Get int13 register stack.

  @retval CommBuf               A pointer to int13 register stack.

**/
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

/**
  Dispatch function for a Software SMI handler.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
SdDoCsm16SmmCall (
  IN  EFI_HANDLE                    DispatchHandle,
  IN CONST VOID                    *Context,
  IN OUT VOID                       *CommBuffer,
  IN OUT UINTN                      *CommBufferSize  
)
{
  EFI_STATUS  Status = EFI_SUCCESS;
  registerStackFrame    *reg;
  SD_MEMORY_CARD_INFO   *CurrentSdPrivateData;
  UINT32                MediaID;
  EFI_LBA               Lba;
  UINT32                ReadWriteSize;
  UINT64                BufferToReadWrite;
  BBS_TABLE             *pBbsTable;
  RESULT_BUF            *Result;
  INT13_DISK_PACKET     *Packet;
  UINT32                Sector;
  UINT32                Header;
  UINT32                Cylinder;
  UINT16                SectorCnt;
  UINT32                NumOfSector;
  UINT32                NumOfHead;
  UINT32                NumOfCylinder;
  UINT8                 *NumOfHDD;
  UINT8                 DevNum;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;

  reg = PointCommBuf();
  if (UpdateBBS == FALSE) {
    UpdateBBS = TRUE;
    pBbsTable = (BBS_TABLE *) (reg->X.edi);
    reg->H.ax = SdUpdateBbs(pBbsTable);
    return EFI_SUCCESS;
  }
  //_outp(0x80, 0xF1);
  //CpuDeadLoop();
  CurrentSdPrivateData  = &SdData[0];
  // Points to physical SD private data
  //CurrentSdPrivateData += reg->L.AccessOrder;
  DevNum = SelectDevice (CurrentSdPrivateData, reg->L.AccessOrder, reg->L.dl);
  if (DevNum == 5) {
    Status = EFI_NOT_FOUND;
    goto ExitSDBoot;
  }
  //DEBUG ((EFI_D_ERROR, "reg->L.AccessOrder = %x, reg->L.dl = %x, DevNum = %x \n", reg->L.AccessOrder, reg->L.dl, DevNum));
  CurrentSdPrivateData += DevNum;
  NumOfHead             = CurrentSdPrivateData->TotalHeads- 1;
  NumOfSector           = CurrentSdPrivateData->SectorsPerHead;
  NumOfCylinder         = CurrentSdPrivateData->Cylinder;
  Header                = (UINT32)(reg->L.dh);
  Sector                = (UINT32)(reg->L.cl & 0x3F);
  Cylinder              = ((UINT32)(reg->L.cl & 0xC0)) * 4 + (UINT32) (reg->L.ch);
  SectorCnt             = (UINT16)reg->L.al;
  //DEBUG ((EFI_D_ERROR, "reg->H.ax = %x \n", reg->H.ax));
  //DEBUG ((EFI_D_ERROR, "reg->L.dl = %x \n", reg->L.dl));
  switch(reg->L.ah) {
    case 0x02:
    case 0x03:
        //
        // Translate to LBA Mode?
        //
        BufferToReadWrite = reg->L.es * 16 + reg->H.bx;
        Lba  = (UINT64) (((Cylinder * (NumOfHead + 1)) + Header) * NumOfSector) + Sector - 1;
        goto SDReadWrite;

    case 0x08:
        if (reg->L.dl >= 0x80) {
          NumOfHDD   = (UINT8 *)(UINTN)(0x475);
        } else {
          reg->L.bl  = 0xFF;
          reg->L.bh  = 0x00;
          reg->H.es  = 0xF000;//Floppy_Parameter_Seg;
          reg->H.di  = 0xEFC7;//Floppy_Parameter_Offset;
          NumOfHDD   = (UINT8 *)(UINTN)(0x410);
          *NumOfHDD  = (*NumOfHDD & 0xC0) >> 6;
          *NumOfHDD += 1;
        }
        reg->H.ax    = 0;
        reg->L.cl    = ((UINT8)NumOfSector) & 0x3F | (UINT8)((NumOfCylinder & 0x0300) >> 2);
        reg->L.ch    = (UINT8)NumOfCylinder;
        reg->L.dl    = *NumOfHDD;
        reg->L.dh    = (UINT8)NumOfHead;
        break;

    case 0x42:
    case 0x43:
        Packet = (INT13_DISK_PACKET *) (reg->L.ds * 16 + reg->H.si);
        if ((Packet->bSize != 0x10) ||(Packet->bRsvd != 0)) {
          reg->L.ah       = 0x01;
          break;
        }
        SectorCnt = Packet->wNumOfTran;
        Lba = Packet->qwLba;
        BufferToReadWrite    = (((Packet->dwBuf >> 12) & 0xFFFF0) + (Packet->dwBuf & 0xFFFF));

    SDReadWrite:
        if (SectorCnt == 0) {
          reg->L.ah = 0;
          break;
        }
        ReadWriteSize = SectorCnt * CurrentSdPrivateData->BytesPerSector;
        //DEBUG ((EFI_D_ERROR, "SectorCnt = %X \n", SectorCnt));
        //DEBUG ((EFI_D_ERROR, "Buffer = %X \n", BufferToReadWrite));
        //DEBUG ((EFI_D_ERROR, "Lba = %X \n", Lba));
        BlockIo = &(CurrentSdPrivateData->SDCardData->Partitions[0].BlockIo);
        MediaID = CurrentSdPrivateData->SDCardData->Partitions[0].BlockIoMedia.MediaId;
        if ((reg->L.ah == 0x02) || (reg->L.ah == 0x42)) {
          Status = BlockIo->ReadBlocks(
                             BlockIo,
                             MediaID,
                             Lba,
                             ReadWriteSize,
                             (VOID *)(UINTN) BufferToReadWrite);
        } else if ((reg->L.ah == 0x03) || (reg->L.ah == 0x43)) {
          Status = BlockIo->WriteBlocks(
                             BlockIo,
                             MediaID,
                             Lba,
                             ReadWriteSize,
                             (VOID *)(UINTN) BufferToReadWrite);
        }
        reg->L.ah = (UINT8)(Status != EFI_SUCCESS);
        //DEBUG ((EFI_D_ERROR, "Status = %r \n", Status));
        break;

    case 0x48:
        Result = (RESULT_BUF *) (reg->L.ds * 16 + reg->H.si);
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
        Result->wInforFlag     = 0x03;
        Result->dwCylinder     = (UINT32) NumOfCylinder;
        Result->dwHeaders      = (UINT32) (NumOfHead + 1);
        Result->dwSector       = (UINT32) NumOfSector;
//-     Result->qwSectos       = CurrentSdPrivateData->TotalLBA;
        BlockIo = &(CurrentSdPrivateData->SDCardData->Partitions[0].BlockIo);
        Result->qwSectos       = BlockIo->Media->LastBlock + 1;
        Result->wBytesInSector = CurrentSdPrivateData->BytesPerSector;
        break;

    default:
        Status = EFI_UNSUPPORTED;
        break;
  }
  ExitSDBoot:
  reg->X.flags.CF    = (UINT16)(Status != EFI_SUCCESS);
  return EFI_SUCCESS;
}

/**
  The entry point for legacy SD driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            Success.
  @retval Others                 Failed to register software SMI for SD card boot.

**/
EFI_STATUS
EFIAPI
SdLegacyEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                        Status = EFI_SUCCESS;
  EFI_SMM_SW_DISPATCH2_PROTOCOL     *SwDispatch2;
  EFI_SMM_SW_REGISTER_CONTEXT       SwContext;
  EFI_HANDLE                        SwHandle;
  EFI_LEGACY_SD_INF_PROTOCOL        *LegacySDInf;
  EFI_PHYSICAL_ADDRESS              MemAddress;
  EFI_SWSMI_VALUE_POLICY_PROTOCOL   *SwSmiPolicy;
  EFI_HANDLE                        SwSmiValuePolicyHandle = NULL;
  
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_LEGACY_SD_INF_PROTOCOL),
                  &LegacySDInf
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  512,
                  &mOneSector
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiBootServicesData,
                  1,
                  &MemAddress
                  );    
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  LegacySDInf->SmmCallCommBuf = (SD_SMMCALL_COMM *)(UINTN)MemAddress;

  Status = gBS->InstallProtocolInterface (
                  &mLegacySDHandle,
                  &gEfiLegacySDInfProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *) LegacySDInf
                  );
	
  ConstructSmmCallTable();
	
  mSmmCallComm     = LegacySDInf->SmmCallCommBuf;
  mSmmCallTablePtr = LegacySDInf->SmmCallTablePtr = SdSmmCallTable;
  //
  //  Locate the ICH SMM SW dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch2 );
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Register SSMI 0xB2(SMI_PORT), 0x90(SMI_DATA) call back function
  //
  SwContext.SwSmiInputValue = SD_LEGACY_SMI_VALUE;
  Status = SwDispatch2->Register (
             SwDispatch2,
             SdDoCsm16SmmCall,
             &SwContext,
             &SwHandle
             );
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }
  Status = gBS->LocateProtocol (&gSwSmiValuePolicyGuid, NULL, &SwSmiPolicy);
  if (EFI_ERROR(Status)) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (EFI_SWSMI_VALUE_POLICY_PROTOCOL),
                    &SwSmiPolicy
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem (SwSmiPolicy, sizeof (EFI_SWSMI_VALUE_POLICY_PROTOCOL));
    SwSmiPolicy->NumSmmEntries = 1;      // first node
    SwSmiPolicy->SwSmiEntry[0].Type = 2; // for SD use
    SwSmiPolicy->SwSmiEntry[0].Value = (UINT8)SwContext.SwSmiInputValue;
    Status = gBS->InstallProtocolInterface (
                    &SwSmiValuePolicyHandle,
                    &gSwSmiValuePolicyGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID *) SwSmiPolicy
                    );
  } else {
    SwSmiPolicy->SwSmiEntry[SwSmiPolicy->NumSmmEntries].Type = 2;// for SD use
    SwSmiPolicy->SwSmiEntry[SwSmiPolicy->NumSmmEntries].Value = (UINT8)SwContext.SwSmiInputValue;
    SwSmiPolicy->NumSmmEntries ++;
  }
  SwContext.SwSmiInputValue = SD_SMI_VALUE;
  Status = SwDispatch2->Register (
                          SwDispatch2,
                          SdSmmCallEntry,
                          &SwContext,
                          &SwHandle
                          );
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }
  return Status;
}


