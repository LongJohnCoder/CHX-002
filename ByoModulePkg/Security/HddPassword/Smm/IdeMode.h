/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  IdeMode.h

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

**/


//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Header file for IDE mode of ATA host controller.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation
(if any) is furnished under a license and may only be used or
copied in accordance with the terms of the license.  Except as
permitted by such license, no part of this software or
documentation may be reproduced, stored in a retrieval system, or
transmitted in any form or by any means without the express written
consent of Intel Corporation.            

**/
#ifndef __HD_PASSWORD_IDE_MODE_H__
#define __HD_PASSWORD_IDE_MODE_H__

typedef enum {
  EfiIdePrimary    = 0,
  EfiIdeSecondary  = 1,
  EfiIdeMaxChannel = 2
} EFI_IDE_CHANNEL;

//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  UINT16                          ErrOrFeature;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  UINT16                          CmdOrStatus;
  UINT16                          AltOrDev;
} EFI_IDE_REGISTERS;

//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            BIT0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    BIT1
#define IDE_SECONDARY_OPERATING_MODE          BIT2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  BIT3

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
  );

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
  );

/**
  Sends out an ATA Identify Command to the specified device.

  This function is called by DiscoverIdeDevice() during its device
  identification. It sends out the ATA Identify Command to the
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
AtaUnlockHddPassword (
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN     CHAR8                         Identifier,
  IN     CHAR8                         *Password,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  );




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
  );



EFI_STATUS
EFIAPI
WaitIDEDeviceReady (
  IN  EFI_IDE_REGISTERS *IdeRegisters
  );

#endif

