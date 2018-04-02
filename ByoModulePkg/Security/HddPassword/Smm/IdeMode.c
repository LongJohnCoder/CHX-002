/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  IdeMode.c

Abstract: 
  part of Hdd password SMM driver.

Revision History:

Bug 3178 - add Hdd security Frozen support.
TIME: 2011-12-06
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Send ATA security frozen command to HDDs at ReadyToBoot and 
     _WAK().
$END--------------------------------------------------------------------

Bug 2749 - Fix S3 resume failed if HDD password enabled.
TIME: 2011-08-18
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Give more delay according to SATA spec.
$END--------------------------------------------------------------------

Bug 2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: coding error in IdeReadPortWMultiple()
$END--------------------------------------------------------------------

**/

//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  This driver is used for HDD Password Feature support at IDE mode.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation
(if any) is furnished under a license and may only be used or
copied in accordance with the terms of the license.  Except as
permitted by such license, no part of this software or
documentation may be reproduced, stored in a retrieval system, or
transmitted in any form or by any means without the express written
consent of Intel Corporation.            

**/
#include "HddPasswordSmm.h"

/**
  Write multiple words of data to the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo      Pointer to the EFI_PCI_IO instance
  @param  Port       IO port to read
  @param  Count      No. of UINT16's to read
  @param  Buffer     Pointer to the data buffer for read

**/
VOID
EFIAPI
IdeWritePortWMultiple (
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  UINT16                *Buffer
  )
{
  UINTN Index;

  for (Index = 0; Index < Count; Index++) {
    IoWrite16 (Port, Buffer[Index]);
  }
}

/**
  Reads multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo    Pointer to the EFI_PCI_IO instance
  @param  Port     IO port to read
  @param  Count    Number of UINT16's to read
  @param  Buffer   Pointer to the data buffer for read

**/
VOID
EFIAPI
IdeReadPortWMultiple (
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  UINT16                *Buffer
  )
{
  UINTN Index;

  for (Index = 0; Index < Count; Index++) {
    Buffer[Index] = IoRead16 (Port);
  }
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's value is also be parsed and print out.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

**/
VOID
EFIAPI
DumpAllIdeRegisters (
  IN     EFI_IDE_REGISTERS        *IdeRegisters,
  IN OUT EFI_ATA_STATUS_BLOCK     *AtaStatusBlock
  )
{
  EFI_ATA_STATUS_BLOCK StatusBlock;

  ASSERT (IdeRegisters != NULL);

  ZeroMem (&StatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

  StatusBlock.AtaStatus          = IoRead8 (IdeRegisters->CmdOrStatus);
  StatusBlock.AtaError           = IoRead8 (IdeRegisters->ErrOrFeature);
  StatusBlock.AtaSectorCount     = IoRead8 (IdeRegisters->SectorCount);
  StatusBlock.AtaSectorCountExp  = IoRead8 (IdeRegisters->SectorCount);
  StatusBlock.AtaSectorNumber    = IoRead8 (IdeRegisters->SectorNumber);
  StatusBlock.AtaSectorNumberExp = IoRead8 (IdeRegisters->SectorNumber);
  StatusBlock.AtaCylinderLow     = IoRead8 (IdeRegisters->CylinderLsb);
  StatusBlock.AtaCylinderLowExp  = IoRead8 (IdeRegisters->CylinderLsb);
  StatusBlock.AtaCylinderHigh    = IoRead8 (IdeRegisters->CylinderMsb);
  StatusBlock.AtaCylinderHighExp = IoRead8 (IdeRegisters->CylinderMsb);
  StatusBlock.AtaDeviceHead      = IoRead8 (IdeRegisters->Head);

  if (AtaStatusBlock != NULL) {
    //
    // Dump the content of all ATA registers.
    //
    CopyMem (AtaStatusBlock, &StatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  }

  DEBUG_CODE_BEGIN ();
  if ((StatusBlock.AtaStatus & ATA_STSREG_DWF) != 0) {
    DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Write Fault\n", StatusBlock.AtaStatus));
  }

  if ((StatusBlock.AtaStatus & ATA_STSREG_CORR) != 0) {
    DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Corrected Data\n", StatusBlock.AtaStatus));
  }

  if ((StatusBlock.AtaStatus & ATA_STSREG_ERR) != 0) {
    if ((StatusBlock.AtaError & ATA_ERRREG_BBK) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Bad Block Detected\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_UNC) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Uncorrectable Data\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_MC) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Media Change\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_ABRT) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Abort\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_TK0NF) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Track 0 Not Found\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_AMNF) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Address Mark Not Found\n", StatusBlock.AtaError));
    }
  }
  DEBUG_CODE_END ();
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's value is also be parsed and print out.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.

  @retval EFI_SUCCESS       No err information in the Status Register.
  @retval EFI_DEVICE_ERROR  Any err information in the Status Register.

**/
EFI_STATUS
EFIAPI
CheckStatusRegister (
  IN  EFI_IDE_REGISTERS        *IdeRegisters
  )
{
  EFI_STATUS      Status;
  UINT8           StatusRegister;

  ASSERT (IdeRegisters != NULL);

  StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

  if ((StatusRegister & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  This function is used to poll for the DRQ bit clear in the Status
  Register. DRQ is cleared when the device is finished transferring data.
  So this function is called after data transfer is finished.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.

  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.

  @note
  Read Status Register will clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear (
  IN  EFI_IDE_REGISTERS         *IdeRegisters,
  IN  UINT64                    Timeout
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;
  UINT8   ErrorRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((StatusRegister & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

    if ((StatusRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
      ErrorRegister = IoRead8 (IdeRegisters->ErrOrFeature);

      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    //  Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the DRQ bit clear in the Alternate
  Status Register. DRQ is cleared when the device is finished
  transferring data. So this function is called after data transfer
  is finished.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.

  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear2 (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    AltRegister = IoRead8 (IdeRegisters->AltOrDev);

    //
    //  wait for BSY == 0 and DRQ == 0
    //
    if ((AltRegister & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
      ErrorRegister = IoRead8 (IdeRegisters->ErrOrFeature);

      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


/**
  This function is used to poll for the DRQ bit set in the Alternate Status Register.
  DRQ is set when the device is ready to transfer data. So this function is called after 
  the command is sent to the device and before required data is transferred.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS           DRQ bit set within the time out.
  @retval EFI_TIMEOUT           DRQ bit not set within the time out.
  @retval EFI_ABORTED           DRQ bit not set caused by the command abort.
  @note  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQReady2 (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    //
    //  Read Alternate Status Register will not clear interrupt status
    //
    AltRegister = IoRead8 (IdeRegisters->AltOrDev);
    //
    // BSY == 0 , DRQ == 1
    //
    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
      ErrorRegister = IoRead8 (IdeRegisters->ErrOrFeature);

      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


/**
  This function is used to poll for the BSY bit clear in the Status Register. BSY
  is clear when the device is not busy. Every command must be sent after device is not busy.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS          BSY bit clear within the time out.
  @retval EFI_TIMEOUT          BSY bit not clear within the time out.

  @note Read Status Register will clear interrupt status.
**/
EFI_STATUS
EFIAPI
WaitForBSYClear (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

    if ((StatusRegister & ATA_STSREG_BSY) == 0x00) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


/**
  Get IDE i/o port registers' base addresses by mode. 

  In 'Compatibility' mode, use fixed addresses.
  In Native-PCI mode, get base addresses from BARs in the PCI IDE controller's
  Configuration Space.

  The steps to get IDE i/o port registers' base addresses for each channel
  as follows:

  1. Examine the Programming Interface byte of the Class Code fields in PCI IDE
  controller's Configuration Space to determine the operating mode.

  2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
   ___________________________________________
  |           | Command Block | Control Block |
  |  Channel  |   Registers   |   Registers   |
  |___________|_______________|_______________|
  |  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
  |___________|_______________|_______________|
  | Secondary |  170h - 177h  |  376h - 377h  |
  |___________|_______________|_______________|

  Table 1. Compatibility resource mappings
  
  b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
  in IDE controller's PCI Configuration Space, shown in the Table 2 below.
   ___________________________________________________
  |           |   Command Block   |   Control Block   |
  |  Channel  |     Registers     |     Registers     |
  |___________|___________________|___________________|
  |  Primary  | BAR at offset 0x10| BAR at offset 0x14|
  |___________|___________________|___________________|
  | Secondary | BAR at offset 0x18| BAR at offset 0x1C|
  |___________|___________________|___________________|

  Table 2. BARs for Register Mapping

  @param[in]      PciIo          Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param[in, out] IdeRegisters    Pointer to EFI_IDE_REGISTERS which is used to
                                 store the IDE i/o port registers' base addresses
           
  @retval EFI_UNSUPPORTED        Return this value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation data error

**/
EFI_STATUS
EFIAPI
GetIdeRegisterIoAddr (
  IN     UINTN                       Bus,
  IN     UINTN                       Device,
  IN     UINTN                       Function,
  IN OUT EFI_IDE_REGISTERS           *IdeRegisters
  )
{
  UINT16            CommandBlockBaseAddr;
  UINT16            ControlBlockBaseAddr;
  UINT8             ClassCode;
  UINT32            BaseAddress[4];

  if (IdeRegisters == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ClassCode = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x9));
  BaseAddress[0] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x10));
  BaseAddress[1] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x14));
  BaseAddress[2] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x18));
  BaseAddress[3] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x1C));

  if ((ClassCode & IDE_PRIMARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x1f0;
    ControlBlockBaseAddr = 0x3f6;
  } else {
    //
    // The BARs should be of IO type
    //
    if ((BaseAddress[0] & BIT0) == 0 ||
        (BaseAddress[1] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16) (BaseAddress[0] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16) ((BaseAddress[1] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE primary channel I/O register base address.
  //
  IdeRegisters[EfiIdePrimary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdePrimary].ErrOrFeature      = (UINT16) (CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdePrimary].SectorCount       = (UINT16) (CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdePrimary].SectorNumber      = (UINT16) (CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdePrimary].CylinderLsb       = (UINT16) (CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdePrimary].CylinderMsb       = (UINT16) (CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdePrimary].Head              = (UINT16) (CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdePrimary].CmdOrStatus       = (UINT16) (CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdePrimary].AltOrDev          = ControlBlockBaseAddr;

  if ((ClassCode & IDE_SECONDARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x170;
    ControlBlockBaseAddr = 0x376;
  } else {
    //
    // The BARs should be of IO type
    //
    if ((BaseAddress[2] & BIT0) == 0 ||
        (BaseAddress[3] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16) (BaseAddress[2] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16) ((BaseAddress[3] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE secondary channel I/O register base address.
  //
  IdeRegisters[EfiIdeSecondary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdeSecondary].ErrOrFeature      = (UINT16) (CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdeSecondary].SectorCount       = (UINT16) (CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdeSecondary].SectorNumber      = (UINT16) (CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdeSecondary].CylinderLsb       = (UINT16) (CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdeSecondary].CylinderMsb       = (UINT16) (CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdeSecondary].Head              = (UINT16) (CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdeSecondary].CmdOrStatus       = (UINT16) (CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdeSecondary].AltOrDev          = ControlBlockBaseAddr;

  return EFI_SUCCESS;
}

/**
  Send ATA Ext command into device with NON_DATA protocol.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param Timeout          The time to complete the command.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_DEVICE_ERROR Error executing commands on this device.

**/
EFI_STATUS
EFIAPI
AtaIssueCommand (
  IN  EFI_IDE_REGISTERS         *IdeRegisters,
  IN  EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN  UINT64                    Timeout
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceHead;
  UINT8       AtaCommand;

  ASSERT (IdeRegisters != NULL);
  ASSERT (AtaCommandBlock != NULL);

  DeviceHead = AtaCommandBlock->AtaDeviceHead;
  AtaCommand = AtaCommandBlock->AtaCommand;

  Status = WaitForBSYClear (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IoWrite8 (IdeRegisters->Head, (UINT8) (0xe0 | DeviceHead));

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  IoWrite8 (IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeaturesExp);
  IoWrite8 (IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeatures);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  IoWrite8 (IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCountExp);
  IoWrite8 (IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCount);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  IoWrite8 (IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumberExp);
  IoWrite8 (IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumber);

  IoWrite8 (IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLowExp);
  IoWrite8 (IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLow);

  IoWrite8 (IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHighExp);
  IoWrite8 (IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHigh);

  //
  // Send command via Command Register
  //
  IoWrite8 (IdeRegisters->CmdOrStatus, AtaCommand);

  //
  // Stall at least 400 microseconds.
  //
  MicroSecondDelay (400);

  return EFI_SUCCESS;
}

/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Buffer           A pointer to the source buffer for the data.
  @param ByteCount        The length of  the data.
  @param Read             Flag used to determine the data transfer direction.
                          Read equals 1, means data transferred from device to host;
                          Read equals 0, means data transferred from host to device.  
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param Timeout          The time to complete the command.
  
  @retval EFI_SUCCESS      send out the ATA command and device send required data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
EFIAPI
AtaPioDataInOut (  
  IN     EFI_IDE_REGISTERS         *IdeRegisters,
  IN OUT VOID                      *Buffer,
  IN     UINT64                    ByteCount,
  IN     BOOLEAN                   Read,
  IN     EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK      *AtaStatusBlock,
  IN     UINT64                    Timeout
  )
{
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  if ((IdeRegisters == NULL) || (Buffer == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Issue ATA command
  //
  Status = AtaIssueCommand (IdeRegisters, AtaCommandBlock, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO data in protocol, host can perform a series of reads to
  // the data register after each time device set DRQ ready;
  // The data size of "a series of read" is command specific.
  // For most ATA command, data size received from device will not exceed
  // 1 sector, hence the data size for "a series of read" can be the whole data
  // size of one command request.
  // For ATA command such as Read Sector command, the data size of one ATA
  // command request is often larger than 1 sector, according to the
  // Read Sector command, the data size of "a series of read" is exactly 1
  // sector.
  // Here for simplification reason, we specify the data size for
  // "a series of read" to 1 sector (256 words) if data size of one ATA command
  // request is larger than 256 words.
  //
  Increment = 256;

  //
  // used to record bytes of currently transfered data
  //
  WordCount = 0;

  while (WordCount < RShiftU64(ByteCount, 1)) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready
    //
    Status = DRQReady2 (IdeRegisters, Timeout);
    if (EFI_ERROR (Status)) {      
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > RShiftU64(ByteCount, 1)) {
      Increment = (UINTN)(RShiftU64(ByteCount, 1) - WordCount);
    }

    if (Read) {
      IdeReadPortWMultiple (
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    } else {
      IdeWritePortWMultiple (
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    }

    Status = CheckStatusRegister (IdeRegisters);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    WordCount += Increment;
    Buffer16  += Increment;
  }

  Status = DRQClear (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  //
  // Dump All Ide registers to ATA_STATUS_BLOCK
  //
  DumpAllIdeRegisters (IdeRegisters, AtaStatusBlock);

  return Status;
}





/**
  Send ATA command into device with NON_DATA protocol

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param Timeout          The time to complete the command.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_ABORTED Command failed
  @retval  EFI_DEVICE_ERROR Device status error.

**/
EFI_STATUS
EFIAPI
AtaNonDataCommandIn (  
  IN     EFI_IDE_REGISTERS         *IdeRegisters,
  IN     EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK      *AtaStatusBlock,
  IN     UINT64                    Timeout
  )
{
  EFI_STATUS  Status;

  if((IdeRegisters == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Issue ATA command
  //
  Status = AtaIssueCommand(IdeRegisters, AtaCommandBlock, Timeout);
  if(EFI_ERROR (Status)){
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  //
  // Wait for command completion
  //
  Status = WaitForBSYClear(IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }
  
  Status = CheckStatusRegister(IdeRegisters);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  //
  // Dump All Ide registers to ATA_STATUS_BLOCK
  //
  DumpAllIdeRegisters(IdeRegisters, AtaStatusBlock);

  return Status;
}






/**
  Sends out an ATA Identify Command to the specified device.

  This function sends out the ATA Identify Command to the
  specified device. Only ATA device responses to this command. If
  the command succeeds, it returns the Identify data structure which
  contains information about the device. This function extracts the
  information it needs to fill the IDE_BLK_IO_DEV data structure,
  including device type, media block size, media capacity, and etc.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.
**/
EFI_STATUS
EFIAPI
AtaIdentify (
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN OUT ATA_IDENTIFY_DATA             *Buffer,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;  

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  
  AtaCommandBlock.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  Status = AtaPioDataInOut (
             IdeRegisters,
             Buffer,
             sizeof (ATA_IDENTIFY_DATA),
             TRUE,
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_TIMEOUT
             );

  return Status;
}

/**
  Sends out an ATA unlock harddisk Command to the specified device.

  It sends out the ATA Identify Command to the specified device. 
  Only ATA device responses to this command. If the command succeeds,
  it returns the Identify data structure which contains information
  about the device. This function extracts the information it needs
  to fill the IDE_BLK_IO_DEV data structure, including device type,
  media block size, media capacity, and etc.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.
**/
EFI_STATUS
EFIAPI
AtaUnlockHddPassword (
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN     CHAR8                         Identifier,
  IN     CHAR8                         *Password,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;  

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (mBuffer, 512);
  AtaCommandBlock.AtaCommand    = 0xF2; //Security unlock cmd
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  ((CHAR16 *)mBuffer)[0] = Identifier & BIT0;
  CopyMem (&((CHAR16 *)mBuffer)[1], Password, 32);

  Status = AtaPioDataInOut (
             IdeRegisters,
             mBuffer,
             512,
             FALSE,
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_TIMEOUT
             );

  ZeroMem (mBuffer, 512);
  return Status;
}



/**
  Sends out an ATA Frozen harddisk security Command to the specified device.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Device           The device number of device.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.
**/
EFI_STATUS
EFIAPI
AtaFrozenHddSecurity (
  IN     EFI_IDE_REGISTERS     *IdeRegisters,
  IN     UINT8                 Device,
  IN OUT EFI_ATA_STATUS_BLOCK  *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;  

  ZeroMem(&AtaCommandBlock, sizeof(EFI_ATA_COMMAND_BLOCK));
  AtaCommandBlock.AtaCommand    = 0xF5;
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  Status = AtaNonDataCommandIn (
             IdeRegisters,
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_TIMEOUT
             );
             
  return Status;
}



EFI_STATUS
EFIAPI
WaitIDEDeviceReady (
  IN  EFI_IDE_REGISTERS *IdeRegisters
  )
{
  EFI_STATUS  Status;
  
  IoWrite8(IdeRegisters->CmdOrStatus, ATA_CMD_EXEC_DRIVE_DIAG);
  Status = WaitForBSYClear(IdeRegisters, 350000000);
  
  if(EFI_ERROR (Status)) {
    return EFI_TIMEOUT;
  }else{
    return EFI_SUCCESS;
  }
}

