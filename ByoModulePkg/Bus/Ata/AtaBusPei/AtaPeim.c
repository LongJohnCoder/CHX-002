/** @file
PEIM to produce gEfiPeiVirtualBlockIoPpiGuid PPI for ATA controllers in the platform.
This PPI can be consumed by PEIM which produce
gEfiPeiDeviceRecoveryModulePpiGuid for Ata Hard Disk.

Copyright (c) 2006 - 2011, BYOSoft Corporation. All rights
reserved.<BR> This software and associated documentation (if
any) is furnished under a license and may only be used or copied
in accordance with the terms of the license. Except as permitted
by such license, no part of this software or documentation may
be reproduced, stored in a retrieval system, or transmitted in
any form or by any means without the express written consent of
BYOSoft Corporation.

**/

#include "AtaPeim.h"
#include "Library\PeiServicesLib.h"
#include "Library\DebugLib.h"
#include "Library\MemoryAllocationLib.h"
#include "Library\BaseMemoryLib.h"
#include "Library\PeiServicesTablePointerLib.h"
#include "Library\PcdLib.h"
#include "Library\TimerLib.h"
#include "Uefi\UefiBaseType.h"
#include "IndustryStandard\Atapi.h"
#include "Library\IoLib.h"

/**
  Wait specified time interval to poll for BSY bit clear in the Status Register.

  @param[in]  AtaBlkIoDev            A pointer to ata block IO device
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        BSY bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        BSY bit is not cleared in the specified time interval.

**/
EFI_STATUS
WaitForBSYClear (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    StatusValue = IoRead8 (StatusRegister);
    if ((StatusValue & ATA_STSREG_BSY) == 0x00) {
      break;
    }
    MicroSecondDelay (250);

    Delay--;

  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  Wait specified time interval to poll for DRDY bit set in the Status register.

  @param[in]  AtaBlkIoDev            A pointer to ata block IO
        device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRDY bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRDY bit is not set in the specified time interval.

**/
EFI_STATUS
DRDYReady (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    StatusValue = IoRead8 (StatusRegister);
    //
    //  BSY == 0 , DRDY == 1
    //
    if ((StatusValue & (ATA_STSREG_DRDY | ATA_STSREG_BSY)) == ATA_STSREG_DRDY) {
      break;
    }

  if ((StatusValue & (ATA_STSREG_ERR | ATA_STSREG_BSY)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;

  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit clear in the Status Register.

  @param[in]  AtaBlkIoDev           A pointer to ata block IO
        device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    StatusValue = IoRead8 (StatusRegister);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((StatusValue & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

  if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit clear in the Alternate Status Register.

  @param[in]  AtaBlkIoDev            A pointer to atapi block IO
        device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear2 (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  AltStatusRegister;
  UINT8   AltStatusValue;
  UINT8   ErrValue;

  AltStatusValue    = 0;

  AltStatusRegister = IdeIoRegisters->Alt.AltStatus;

  Delay             = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    AltStatusValue = IoRead8 (AltStatusRegister);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((AltStatusValue & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

  if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit set in the Status Register.

  @param[in]  AtaBlkIoDev            A pointer to ata block IO
        device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;
  ErrValue        = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    //
    //  read Status Register will clear interrupt
    //
    StatusValue = IoRead8 (StatusRegister);

    //
    //  BSY==0,DRQ==1
    //
    if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
      if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }
    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit set in the Alternate Status Register.

  @param[in]  AtaBlkIoDev            A pointer to ata block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady2 (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  AltStatusRegister;
  UINT8   AltStatusValue;
  UINT8   ErrValue;

  AltStatusValue    = 0;

  AltStatusRegister = IdeIoRegisters->Alt.AltStatus;

  Delay             = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    AltStatusValue = IoRead8 (AltStatusRegister);

    //
    //  BSY==0,DRQ==1
    //
    if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
      if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }
    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Check if there is an error in Status Register.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  StatusReg         The address to IDE IO registers.

  @retval EFI_SUCCESS        Operation success.
  @retval EFI_DEVICE_ERROR   Device error.

**/
EFI_STATUS
CheckErrorStatus (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  UINT16              StatusReg
  )
{
  UINT8 StatusValue;

  StatusValue = IoRead8 (StatusReg);

  if ((StatusValue & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {

    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;

}

/**
  Send out ATA commands conforms to the Packet Command with PIO
  Data In Protocol.

  @param[in]  AtaBlkIoDev           A pointer to ata block IO
        device.
  @param[in]  DevicePosition        An integer to signify device position.
  @param[in]  Packet                A pointer to ATAPI command packet.
  @param[in]  Buffer                Buffer to contain requested transfer data from device.
  @param[in]  ByteCount             Requested transfer data length.
  @param[in]  TimeoutInMilliSeconds Time out value, in unit of milliseconds.

  @retval EFI_SUCCESS        Command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed command successfully.

**/
EFI_STATUS
AtaIssueCommand (
  IN  ATA_BLK_IO_DEV        *AtaBlkIoDev,
  IN  UINTN                 DevicePosition,
  EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN  UINTN                 TimeoutInMilliSeconds
  )
{
  EFI_STATUS  Status;
  UINT8       Channel;
  UINT8       Device;
  UINT16      HeadReg;
  UINT16      CommandReg;
  UINT16      FeatureReg;
  UINT16      CylinderLsbReg;
  UINT16      CylinderMsbReg;
  UINT16      SectorNumberReg;
  UINT16      SectorCountReg;
  UINT8       DeviceHead;
  UINT8       AtaCommand;

  DeviceHead = AtaCommandBlock->AtaDeviceHead;
  AtaCommand = AtaCommandBlock->AtaCommand;

  Channel         = (UINT8) (DevicePosition / 2);
  Device          = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  SectorNumberReg   = AtaBlkIoDev->IdeIoPortReg[Channel].SectorNumber;
  SectorCountReg    = AtaBlkIoDev->IdeIoPortReg[Channel].SectorCount;
  HeadReg           = AtaBlkIoDev->IdeIoPortReg[Channel].Head;
  CommandReg        = AtaBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  FeatureReg        = AtaBlkIoDev->IdeIoPortReg[Channel].Reg1.Feature;
  CylinderLsbReg    = AtaBlkIoDev->IdeIoPortReg[Channel].CylinderLsb;
  CylinderMsbReg    = AtaBlkIoDev->IdeIoPortReg[Channel].CylinderMsb;

  Status = WaitForBSYClear (AtaBlkIoDev, &AtaBlkIoDev->IdeIoPortReg[Channel], TimeoutInMilliSeconds);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Select device via Device/Head Register.
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IoWrite8 (HeadReg, (UINT8) ((Device << 4) | 0xE0));
  //
  // Set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  if (DRQClear2 (
        AtaBlkIoDev,
        &(AtaBlkIoDev->IdeIoPortReg[Channel]),
        TimeoutInMilliSeconds
        ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice
  //
  IoWrite8 (FeatureReg, AtaCommandBlock->AtaFeaturesExp);
  IoWrite8 (FeatureReg, AtaCommandBlock->AtaFeatures);
  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  IoWrite8 (SectorCountReg, AtaCommandBlock->AtaSectorCountExp);
  IoWrite8 (SectorCountReg, AtaCommandBlock->AtaSectorCount);
  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  IoWrite8 (SectorNumberReg, AtaCommandBlock->AtaSectorNumberExp);
  IoWrite8 (SectorNumberReg, AtaCommandBlock->AtaSectorNumber);

  IoWrite8 (CylinderLsbReg, AtaCommandBlock->AtaCylinderLowExp);
  IoWrite8 (CylinderLsbReg, AtaCommandBlock->AtaCylinderLow);

  IoWrite8 (CylinderMsbReg, AtaCommandBlock->AtaCylinderHighExp);
  IoWrite8 (CylinderMsbReg, AtaCommandBlock->AtaCylinderHigh);
  //
  // Send command via Command Register
  //
  IoWrite8 (CommandReg, AtaCommand);
  //
  // Stall at least 400 microseconds.
  //
  MicroSecondDelay (400);

  return EFI_SUCCESS;
}

/**
  Check power mode of Ata devices.

  @param[in]  AtaBlkIoDev     A pointer to ata block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in]  AtaCommand      The Ata Command passed in.

  @retval EFI_SUCCESS         The Ata device support power mode.
  @retval EFI_NOT_FOUND       The Ata device not found.
  @retval EFI_TIMEOUT         Ata command transaction is time
          out.
  @retval EFI_ABORTED         Ata command abort.

**/
EFI_STATUS
AtaCheckPowerMode (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  UINTN               DevicePosition
  )
{
  UINT8       Channel;
  UINT8       Device;
  UINT16      StatusRegister;
  UINT16      HeadRegister;
  UINT16      CommandRegister;
  UINT16      ErrorRegister;
  UINT16      SectorCountRegister;
  EFI_STATUS  Status;
  UINT8       StatusValue;
  UINT8       ErrorValue;
  UINT8       SectorCountValue;

  EFI_ATA_COMMAND_BLOCK   AtaCommandBlock;

  Channel             = (UINT8) (DevicePosition / 2);
  Device              = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  StatusRegister      = AtaBlkIoDev->IdeIoPortReg[Channel].Reg.Status;
  HeadRegister        = AtaBlkIoDev->IdeIoPortReg[Channel].Head;
  CommandRegister     = AtaBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  ErrorRegister       = AtaBlkIoDev->IdeIoPortReg[Channel].Reg1.Error;
  SectorCountRegister = AtaBlkIoDev->IdeIoPortReg[Channel].SectorCount;

  ZeroMem (&AtaCommandBlock, sizeof(EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand = 0xE5;
  AtaCommandBlock.AtaDeviceHead = (UINT8) ((Device << 4) | 0xe0);

  Status = AtaIssueCommand (
              AtaBlkIoDev,
              DevicePosition,
              &AtaCommandBlock,
              ATATIMEOUT);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = WaitForBSYClear (AtaBlkIoDev, &(AtaBlkIoDev->IdeIoPortReg[Channel]), 3000);
  if (EFI_ERROR (Status)) {
    return EFI_TIMEOUT;
  }

  StatusValue = IoRead8 (StatusRegister);

  //
  // command returned status is DRDY, indicating device supports the command,
  // so device is present.
  //
  if ((StatusValue & ATA_STSREG_DRDY) == ATA_STSREG_DRDY) {
    return EFI_SUCCESS;
  }

  SectorCountValue = IoRead8 (SectorCountRegister);

  //
  // command returned status is ERR & ABRT_ERR, indicating device does not support
  // the command, so device is present.
  //
  if ((StatusValue & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
    ErrorValue = IoRead8 (ErrorRegister);
    if ((ErrorValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
      return EFI_ABORTED;
    } else {
      //
      // According to spec, no other error code is valid
      //
      return EFI_NOT_FOUND;
    }
  }

  if ((SectorCountValue == 0x00) || (SectorCountValue == 0x80) || (SectorCountValue == 0xff)) {
    //
    // Write SectorCount 0x55 but return valid state value. Maybe no device
    // exists or some slow kind of ATAPI device exists.
    //
    IoWrite8 (HeadRegister, (UINT8) ((Device << 4) | 0xe0));

    //
    // write 0x55 and 0xaa to SectorCounter register,
    // if the data could be written into the register,
    // indicating the device is present, otherwise the device is not present.
    //
    SectorCountValue = 0x55;
    IoWrite8 (SectorCountRegister, SectorCountValue);
    MicroSecondDelay (10000);

    SectorCountValue = IoRead8 (SectorCountRegister);
    if (SectorCountValue != 0x55) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Detect if an IDE controller exists in specified position.

  @param[in]  AtaBlkIoDev     A pointer to ATA block IO device.
  @param[in]  DevicePosition  An integer to signify device position.

  @retval TRUE         The Atapi device exists.
  @retval FALSE        The Atapi device does not present.

**/
BOOLEAN
DetectIDEController (
  IN  ATA_BLK_IO_DEV     *AtaBlkIoDev,
  IN  UINTN              DevicePosition
  )
{
  UINT8       Channel;
  EFI_STATUS  Status;

  Channel           = (UINT8) (DevicePosition / 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);
  //
  //  Wait 31 seconds for BSY clear
  //
  Status = WaitForBSYClear (AtaBlkIoDev, &(AtaBlkIoDev->IdeIoPortReg[Channel]), 31000);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Send 'check power' command for IDE device
  //
  Status      = AtaCheckPowerMode (AtaBlkIoDev, DevicePosition);
  if ((Status == EFI_ABORTED) || (Status == EFI_SUCCESS)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Reads multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  Port     IO port to read
  @param  Count    Number of UINT16's to read
  @param  Buffer   Pointer to the data buffer for read

**/
VOID
IdeReadPortWMultiple (
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
{
  UINTN     Index;
  UINT16    *Ptr;

  ASSERT (Buffer != NULL);

  Index = Count;
  Ptr = (UINT16 *)Buffer;
  for (Index = 0; Index < Count; Index++) {
    *(Ptr + Index) = IoRead16(Port);
  }
}

/**
  Write multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete write,
  not one word at a time

  @param  Port     IO port to write
  @param  Count    Number of UINT16's to write
  @param  Buffer   Pointer to the data buffer for write

**/
VOID
IdeWritePortWMultiple (
  IN      UINT16                Port,
  IN      UINTN                 Count,
  IN OUT  VOID                  *Buffer
  )
{
  UINTN     Index;
  UINT16    *Ptr;

  ASSERT (Buffer != NULL);

  Index = Count;
  Ptr = (UINT16 *)Buffer;
  for (Index = 0; Index < Count; Index++) {
    IoWrite16 (Port, *(Ptr + Index));
  }
}


/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

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
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
AtaPioDataInOut (
  IN  ATA_BLK_IO_DEV              *AtaBlkIoDev,
  IN  UINTN                       DevicePosition,
  IN OUT VOID                     *Buffer,
  IN     UINT64                   ByteCount,
  IN     BOOLEAN                  Read,
  IN     EFI_ATA_COMMAND_BLOCK    *AtaCommandBlock,
  IN     UINTN                    TimeoutMilliSeconds
  )
{
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  UINT8       Channel;
  UINT8       Device;
  UINT16      DataReg;
  UINT16      StatusReg;
  EFI_STATUS  Status;

  if ((AtaBlkIoDev == NULL) || (Buffer == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Channel         = (UINT8) (DevicePosition / 2);
  Device          = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  DataReg = AtaBlkIoDev->IdeIoPortReg[Channel].Data;
  StatusReg = AtaBlkIoDev->IdeIoPortReg[Channel].Reg.Status;
  //
  // Issue ATA command
  //
  Status = AtaIssueCommand (AtaBlkIoDev, DevicePosition, AtaCommandBlock, TimeoutMilliSeconds);
  if (EFI_ERROR (Status)) {
    return Status;
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
    Status = DRQReady2 (AtaBlkIoDev, &AtaBlkIoDev->IdeIoPortReg[Channel], TimeoutMilliSeconds);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > RShiftU64(ByteCount, 1)) {
      Increment = (UINTN)(RShiftU64(ByteCount, 1) - WordCount);
    }

    if (Read) {
      IdeReadPortWMultiple (
        DataReg,
        Increment,
        Buffer16
        );
    } else {
      IdeWritePortWMultiple (
        DataReg,
        Increment,
        Buffer16
        );
    }
    if ((IoRead8(StatusReg) & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {
    } else {
      return EFI_DEVICE_ERROR;
    }

    WordCount += Increment;
    Buffer16  += Increment;
  }

  Status = DRQClear (
              AtaBlkIoDev,
              &AtaBlkIoDev->IdeIoPortReg[Channel],
              TimeoutMilliSeconds);

  return Status;
}

/**
  Idendify Ata devices.

  @param[in]  AtaBlkIoDev     A pointer to ata block IO device.
  @param[in]  DevicePosition    An integer to signify device position.

  @retval EFI_SUCCESS        Identify successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be identified successfully.

**/
EFI_STATUS
ATAIdentify (
  IN      ATA_BLK_IO_DEV          *AtaBlkIoDev,
  IN      UINTN                   DevicePosition,
  IN OUT  ATA_IDENTIFY_DATA       *AtaIdentifyData
  )
{
  EFI_STATUS              Status;
  EFI_ATA_COMMAND_BLOCK   AtaCommandBlock;
  UINT8                   Channel;
  UINT8                   Device;

  Channel         = (UINT8) (DevicePosition / 2);
  Device          = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);


  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaDeviceHead = (UINT8)((Device << 0x4) | 0xE0);

  Status = AtaPioDataInOut (
             AtaBlkIoDev,
             DevicePosition,
             AtaIdentifyData,
             sizeof (ATA_IDENTIFY_DATA),
             TRUE,
             &AtaCommandBlock,
             3000
             );
  return Status;
}

/**
  Detect Ata devices.

  @param[in]  AtaBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[out] MediaInfo       The media information of the specified block media.

  @retval TRUE                Atapi device exists in specified position.
  @retval FALSE               Atapi device does not exist in specified position.

**/
BOOLEAN
DiscoverAtaDevice (
  IN  ATA_BLK_IO_DEV              *AtaBlkIoDev,
  IN  UINTN                         DevicePosition
  )
{
  ATA_IDENTIFY_DATA         AtaIdentifyData;
  EFI_STATUS  Status;

  if (!DetectIDEController (AtaBlkIoDev, DevicePosition)) {
    return FALSE;
  }

  //
  // test if it is an ATA device (only supported device)
  Status = ATAIdentify (AtaBlkIoDev, DevicePosition, &AtaIdentifyData);
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Used before read/write blocks from/to ATA device media. This
  function is used to get device inforation.

  @param[in]  AtaBlkIoDev       A pointer to atapi block IO
        device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in, out] MediaInfo         The media information of the specified block media.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate required resources.

**/
EFI_STATUS
DetectMedia (
  IN  ATA_BLK_IO_DEV                *AtaBlkIoDev,
  IN  UINTN                         DevicePosition,
  IN OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo
  )
{
  EFI_STATUS    Status;
  ATA_IDENTIFY_DATA  AtaIdentifyData;

  ZeroMem(&AtaIdentifyData, sizeof(ATA_IDENTIFY_DATA));

  Status = ATAIdentify(AtaBlkIoDev, DevicePosition, &AtaIdentifyData);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  //
  // it's hard disk, the media is not removable
  // Because PI1.2 spec. doesn't define HDD type, so we use MaxDeviceType
  //
  MediaInfo->DeviceType = MaxDeviceType;
  MediaInfo->MediaPresent = TRUE;

  //
  // word117-word118 = Logic block size
  //
  MediaInfo->BlockSize = 0x200;

  //
  // word83[10] = 48-bit address feature set support
  //
  if (AtaIdentifyData.command_set_supported_83 & 0x0400) {
    MediaInfo->LastBlock = 0;
    CopyMem (
      &MediaInfo->LastBlock,
      AtaIdentifyData.maximum_lba_for_48bit_addressing,
      2 * sizeof (UINT16));
    MediaInfo->LastBlock -= 1;
  } else {
    // only 28-bit address. word60-61 = the number of 28-bit address sector
    MediaInfo->LastBlock = 0;
    CopyMem (
      &MediaInfo->LastBlock,
      &AtaIdentifyData.user_addressable_sectors_lo,
      2 * sizeof (UINT16));
    MediaInfo->LastBlock -= 1;
  }

  return EFI_SUCCESS;
}

/**
  Enumerate Ata devices.

  This function is used to enumerate Ata device in Ide channel.

  @param[in]  AtaBlkIoDev  A pointer to atapi block IO device

**/
VOID
AtaEnumerateDevices (
  IN  ATA_BLK_IO_DEV  *AtaBlkIoDev
  )
{
  UINT8               Index1;
  UINT8               Index2;
  UINTN               DevicePosition;
  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_STATUS          Status;
  UINTN               DeviceCount;
  UINT16              CommandBlockBaseAddr;
  UINT16              ControlBlockBaseAddr;
  UINT32              IdeEnabledNumber;
  IDE_REGS_BASE_ADDR  IdeRegsBaseAddr[MAX_IDE_CHANNELS];

  DeviceCount = 0;
  DevicePosition = 0;

  //
  // Enable Sata and IDE controller.
  //
  AtaBlkIoDev->AtaControllerPpi->EnableAtaChannel (
                                  (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                                  AtaBlkIoDev->AtaControllerPpi,
                                  PEI_ICH_IDE_PRIMARY | PEI_ICH_IDE_SECONDARY
                                  );

  //
  // Get four channels (primary or secondary Pata, Sata Channel) Command and Control Regs Base address.
  //
  IdeEnabledNumber = AtaBlkIoDev->AtaControllerPpi->GetIdeRegsBaseAddr (
                                                      (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                                                      AtaBlkIoDev->AtaControllerPpi,
                                                      IdeRegsBaseAddr
                                                      );

  //
  // Using Command and Control Regs Base Address to fill other registers.
  //
  for (Index1 = 0; Index1 < IdeEnabledNumber; Index1 ++) {
    CommandBlockBaseAddr               = IdeRegsBaseAddr[Index1].CommandBlockBaseAddr;
    AtaBlkIoDev->IdeIoPortReg[Index1].Data         = CommandBlockBaseAddr;
    AtaBlkIoDev->IdeIoPortReg[Index1].Reg1.Feature = (UINT16) (CommandBlockBaseAddr + 0x1);
    AtaBlkIoDev->IdeIoPortReg[Index1].SectorCount  = (UINT16) (CommandBlockBaseAddr + 0x2);
    AtaBlkIoDev->IdeIoPortReg[Index1].SectorNumber = (UINT16) (CommandBlockBaseAddr + 0x3);
    AtaBlkIoDev->IdeIoPortReg[Index1].CylinderLsb  = (UINT16) (CommandBlockBaseAddr + 0x4);
    AtaBlkIoDev->IdeIoPortReg[Index1].CylinderMsb  = (UINT16) (CommandBlockBaseAddr + 0x5);
    AtaBlkIoDev->IdeIoPortReg[Index1].Head         = (UINT16) (CommandBlockBaseAddr + 0x6);
    AtaBlkIoDev->IdeIoPortReg[Index1].Reg.Command  = (UINT16) (CommandBlockBaseAddr + 0x7);

    ControlBlockBaseAddr                = IdeRegsBaseAddr[Index1].ControlBlockBaseAddr;
    AtaBlkIoDev->IdeIoPortReg[Index1].Alt.DeviceControl = ControlBlockBaseAddr;
    AtaBlkIoDev->IdeIoPortReg[Index1].DriveAddress      = (UINT16) (ControlBlockBaseAddr + 0x1);

    //
    // Scan IDE bus for ATAPI devices IDE or Sata device
    //
    for (Index2 = IdeMaster; Index2 < IdeMaxDevice; Index2++) {
      //
      // Pata & Sata, Primary & Secondary channel, Master & Slave device
      //
      DevicePosition = (UINTN) (Index1 * 2 + Index2);

      if (DiscoverAtaDevice (AtaBlkIoDev, DevicePosition)) {
        //
        // ATA Device at DevicePosition is found.
        //
        AtaBlkIoDev->DeviceInfo[DeviceCount].DevicePosition = DevicePosition;
        //
        // Retrieve Media Info
        //
        Status  = DetectMedia (AtaBlkIoDev, DevicePosition, &MediaInfo);
        CopyMem (&(AtaBlkIoDev->DeviceInfo[DeviceCount].MediaInfo), &MediaInfo, sizeof (MediaInfo));

        DEBUG ((EFI_D_INFO, "Ata Device Position is %d\n", DevicePosition));
        DEBUG ((EFI_D_INFO, "Ata DeviceType is   %d\n", MediaInfo.DeviceType));
        DEBUG ((EFI_D_INFO, "Ata MediaPresent is %d\n", MediaInfo.MediaPresent));
        DEBUG ((EFI_D_INFO, "Ata BlockSize is  0x%x\n", MediaInfo.BlockSize));

        if (EFI_ERROR (Status)) {
          AtaBlkIoDev->DeviceInfo[DeviceCount].MediaInfo.MediaPresent = FALSE;
          AtaBlkIoDev->DeviceInfo[DeviceCount].MediaInfo.LastBlock    = 0;
        }
        DeviceCount += 1;
      }
    }
  }

  AtaBlkIoDev->DeviceCount = DeviceCount;
}

/**
  Perform read from disk in block unit.

  @param[in]  AtaBlkIoDev     A pointer to ata block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in]  Buffer          Buffer to contain read data.
  @param[in]  StartLba        Starting LBA address.
  @param[in]  NumberOfBlocks  Number of blocks to read.
  @param[in]  BlockSize       Size of each block.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ReadSectors (
  IN  ATA_BLK_IO_DEV      *AtaBlkIoDev,
  IN  UINTN               DevicePosition,
  IN  VOID                *Buffer,
  IN  EFI_PEI_LBA         StartLba,
  IN  UINTN               NumberOfBlocks,
  IN  UINTN               BlockSize
  )
{
  EFI_STATUS              Status;
  UINT8                   Channel;
  UINT8                   Device;
  UINT64                  ByteCount;
  EFI_ATA_COMMAND_BLOCK   AtaCommandBlock;

  Channel         = (UINT8) (DevicePosition / 2);
  Device          = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);


// 1. block range has been checked at AtaReadBlocks()
// 2. DeviceInfo[Index], Index = PhysicalDevNo - 1.
//    PhysicalDevNo is start at 1, and is increased when new IDE devide found, not DevicePosition.
//
//  if ((StartLba + NumberOfBlocks - 1) > AtaBlkIoDev->DeviceInfo[Device].MediaInfo.LastBlock) {
//    return EFI_INVALID_PARAMETER;
//  }

  ZeroMem (&AtaCommandBlock, sizeof(EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand          = 0x24;
  AtaCommandBlock.AtaDeviceHead       = (UINT8)((Device << 0x4) | 0xE0);
  AtaCommandBlock.AtaSectorCount      = (UINT8)NumberOfBlocks;
  AtaCommandBlock.AtaSectorCountExp   = (UINT8)(NumberOfBlocks >> 8);
  AtaCommandBlock.AtaSectorNumber     = (UINT8)StartLba;
  AtaCommandBlock.AtaCylinderLow      = (UINT8)(RShiftU64(StartLba, 8));
  AtaCommandBlock.AtaCylinderHigh     = (UINT8)(RShiftU64(StartLba, 16));
  AtaCommandBlock.AtaSectorNumberExp  = (UINT8)(RShiftU64(StartLba, 24));
  AtaCommandBlock.AtaCylinderLowExp   = (UINT8)(RShiftU64(StartLba, 32));
  AtaCommandBlock.AtaCylinderHighExp  = (UINT8)(RShiftU64(StartLba, 40));

  ByteCount = NumberOfBlocks * BlockSize;

  Status = AtaPioDataInOut(
            AtaBlkIoDev,
            DevicePosition,
            Buffer,
            ByteCount,
            TRUE,
            &AtaCommandBlock,
            3000
            );

  return Status;
}

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATA driver, it
  returns the number of all the detected ATA devices it detects
  during the enumeration process. If no device is
  detected, then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
AtaGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  OUT  UINTN                             *NumberBlockDevices
  )
{
  ATA_BLK_IO_DEV     *AtaBlkIoDev;

  AtaBlkIoDev = NULL;

  AtaBlkIoDev       = PEI_RECOVERY_ATA_FROM_BLKIO_THIS (This);

  *NumberBlockDevices = AtaBlkIoDev->DeviceCount;

  return EFI_SUCCESS;
}

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @retval EFI_SUCCESS           Media information about the specified block device
                                was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware
                                error.
  @retval Others                Other failure occurs.

**/
EFI_STATUS
EFIAPI
AtaGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  )
{
  UINTN             DeviceCount;
  ATA_BLK_IO_DEV    *AtaBlkIoDev;
  EFI_STATUS        Status;
  UINTN             Index;

  AtaBlkIoDev = NULL;

  if (This == NULL || MediaInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtaBlkIoDev = PEI_RECOVERY_ATA_FROM_BLKIO_THIS (This);

  DeviceCount   = AtaBlkIoDev->DeviceCount;

  //
  // DeviceIndex is a value from 1 to NumberBlockDevices.
  //
  if ((DeviceIndex < 1) || (DeviceIndex > DeviceCount) || (DeviceIndex > MAX_IDE_DEVICES)) {
    return EFI_INVALID_PARAMETER;
  }

  Index = DeviceIndex - 1;

  //
  // probe media and retrieve latest media information
  //
  DEBUG ((EFI_D_INFO, "Ata GetInfo DevicePosition is %d\n", AtaBlkIoDev->DeviceInfo[Index].DevicePosition));
  DEBUG ((EFI_D_INFO, "Ata GetInfo DeviceType is   %d\n", AtaBlkIoDev->DeviceInfo[Index].MediaInfo.DeviceType));
  DEBUG ((EFI_D_INFO, "Ata GetInfo MediaPresent is %d\n", AtaBlkIoDev->DeviceInfo[Index].MediaInfo.MediaPresent));
  DEBUG ((EFI_D_INFO, "Ata GetInfo BlockSize is  0x%x\n", AtaBlkIoDev->DeviceInfo[Index].MediaInfo.BlockSize));
  DEBUG ((EFI_D_INFO, "Ata GetInfo LastBlock is  0x%x\n", AtaBlkIoDev->DeviceInfo[Index].MediaInfo.LastBlock));

  Status = DetectMedia (
             AtaBlkIoDev,
             AtaBlkIoDev->DeviceInfo[Index].DevicePosition,
             &AtaBlkIoDev->DeviceInfo[Index].MediaInfo
             );
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Get media info from AtaBlkIoDev
  //
//  CopyMem (MediaInfo, &AtaBlkIoDev->DeviceInfo[Index].MediaInfo, sizeof(EFI_PEI_BLOCK_IO_MEDIA));
  MediaInfo->DeviceType = AtaBlkIoDev->DeviceInfo[Index].MediaInfo.DeviceType;
  MediaInfo->MediaPresent = AtaBlkIoDev->DeviceInfo[Index].MediaInfo.MediaPresent;
  MediaInfo->LastBlock = AtaBlkIoDev->DeviceInfo[Index].MediaInfo.LastBlock;
  MediaInfo->BlockSize = AtaBlkIoDev->DeviceInfo[Index].MediaInfo.BlockSize;

  return EFI_SUCCESS;
}

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
AtaReadBlocks (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLBA,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  )
{

  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_STATUS              Status;
  UINTN                   NumberOfBlocks;
  UINTN                   BlockSize;
  ATA_BLK_IO_DEV          *AtaBlkIoDev;

  AtaBlkIoDev = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtaBlkIoDev = PEI_RECOVERY_ATA_FROM_BLKIO_THIS (This);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Status = AtaGetBlockDeviceMediaInfo (
            PeiServices,
            This,
            DeviceIndex,
            &MediaInfo
            );
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (!MediaInfo.MediaPresent) {
    return EFI_NO_MEDIA;
  }

  BlockSize = MediaInfo.BlockSize;

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if ((StartLBA + NumberOfBlocks - 1) > AtaBlkIoDev->DeviceInfo[DeviceIndex - 1].MediaInfo.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ReadSectors (
            AtaBlkIoDev,
            AtaBlkIoDev->DeviceInfo[DeviceIndex - 1].DevicePosition,
            Buffer,
            StartLBA,
            NumberOfBlocks,
            BlockSize
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Initializes the Ata Block Io PPI.

  @param[in]  FileHandle           Handle of the file being invoked.
  @param[in]  PeiServices          Describes the list of possible PEI Services.

  @retval     EFI_SUCCESS          Operation performed successfully.
  @retval     EFI_OUT_OF_RESOURCES Not enough memory to allocate.

**/
EFI_STATUS
EFIAPI
AtaPeimEntry (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PEI_ATA_CONTROLLER_PPI  *AtaControllerPpi;
  EFI_STATUS              Status;
  ATA_BLK_IO_DEV          *AtaBlkIoDev;


  Status = PeiServicesRegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiServicesLocatePpi (
              &gPeiAtaControllerPpiGuid,
              0,
              NULL,
              (VOID **) &AtaControllerPpi
              );
  ASSERT_EFI_ERROR (Status);

  AtaBlkIoDev = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (ATA_BLK_IO_DEV)));
  if (AtaBlkIoDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem(AtaBlkIoDev, EFI_PAGES_TO_SIZE(EFI_SIZE_TO_PAGES(sizeof(ATA_BLK_IO_DEV))));
  AtaBlkIoDev->Signature        = ATA_BLK_IO_DEV_SIGNATURE;
  AtaBlkIoDev->AtaControllerPpi = AtaControllerPpi;

  //
  // Ata device enumeration and build private data
  //
  AtaEnumerateDevices (AtaBlkIoDev);

  AtaBlkIoDev->AtaBlkIo.GetNumberOfBlockDevices = AtaGetNumberOfBlockDevices;
  AtaBlkIoDev->AtaBlkIo.GetBlockDeviceMediaInfo = AtaGetBlockDeviceMediaInfo;
  AtaBlkIoDev->AtaBlkIo.ReadBlocks              = AtaReadBlocks;

  AtaBlkIoDev->PpiDescriptor.Flags                = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  AtaBlkIoDev->PpiDescriptor.Guid                 = &gEfiPeiVirtualBlockIoPpiGuid;
  AtaBlkIoDev->PpiDescriptor.Ppi                  = &AtaBlkIoDev->AtaBlkIo;

  DEBUG ((EFI_D_INFO, "Ata Device Count is %d\n", AtaBlkIoDev->DeviceCount));
  if (AtaBlkIoDev->DeviceCount != 0) {
    Status = PeiServicesInstallPpi (&AtaBlkIoDev->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}


