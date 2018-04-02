/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  AhciMode.c

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

Bug 2400 - Unlock HDD is very slow under AHCI or Raid mode.
TIME: 2011-06-30
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. When PIO write, we should wait PxIS.DHRS instead of PxIS.PSS
$END--------------------------------------------------------------------

Bug 2274: improve hdd password feature.
TIME: 2011-06-09
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. give user max 3 times to input password at post.
  2. check device is hdd or not before unlock it when S3 resume.
  3. remove personal label such as wztest.
$END--------------------------------------------------------------------

Bug 2164: system cannot resume from S3 when hdd password has been set before.
TIME: 2011-05-26
$AUTHOR: Zhang Lin
$REVIEWERS: Chen Daolin
$SCOPE: SugarBay
$TECHNICAL: 
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
  This driver is used for HDD Password Feature support at AHCI mode.  
  
Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.         

**/

#include "HddPasswordSmm.h"


UINT32                 mAhciBar = 0;
EFI_AHCI_REGISTERS     mAhciRegisters;


/**
  Read AHCI Operation register.

  @param  Offset       The operation register offset.

  @return The register content read.

**/
UINT32
EFIAPI
AhciReadReg (
  IN  UINT32              Offset
  )
{
  UINT32   Data;
  
  Data = 0;

  Data = MmioRead32 (mAhciBar + Offset);

  return Data;
}

/**
  Write AHCI Operation register.

  @param  Offset       The operation register offset.
  @param  Data         The data used to write down.

**/
VOID
EFIAPI
AhciWriteReg (
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  MmioWrite32 (mAhciBar + Offset, Data);

  return ;
}

/**
  Do AND operation with the value of AHCI Operation register.

  @param  Offset       The operation register offset.
  @param  AndData      The data used to do AND operation.

**/
VOID
EFIAPI
AhciAndReg (
  IN UINT32               Offset,
  IN UINT32               AndData
  )
{
  UINT32 Data;
  
  Data  = AhciReadReg (Offset);

  Data &= AndData;

  AhciWriteReg (Offset, Data);
}

/**
  Do OR operation with the value of AHCI Operation register.

  @param  Offset       The operation register offset.
  @param  OrData       The data used to do OR operation.

**/
VOID
EFIAPI
AhciOrReg (
  IN UINT32               Offset,
  IN UINT32               OrData
  )
{
  UINT32 Data;

  Data  = AhciReadReg (Offset);

  Data |= OrData;

  AhciWriteReg (Offset, Data);
}

/**
  Wait for memory set to the test value.
    
  @param  Offset            The memory address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set.

  @retval EFI_DEVICE_ERROR  The memory is not set.
  @retval EFI_TIMEOUT       The memory setting is time out.
  @retval EFI_SUCCESS       The memory is correct set.

**/
EFI_STATUS
EFIAPI
AhciWaitMemSet (
  IN  UINT32                    Offset,
  IN  UINT32                    MaskValue,
  IN  UINT32                    TestValue,
  IN  UINT64                    Timeout
  )
{
  UINT32     Value;  
  UINT32     Delay;

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg (Offset) & MaskValue;

    if (Value == TestValue) {
      return EFI_SUCCESS;
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

  return EFI_DEVICE_ERROR;
}

/**

  Clear the port interrupt and error status. It will also clear
  HBA interrupt status.
    
  @param      Port           The number of port.
     
**/ 
VOID
EFIAPI
AhciClearPortStatus (
  IN  UINT8                  Port
  )  
{
  UINT32 Offset;

  //
  // Clear any error status
  //  
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
  AhciWriteReg (Offset, AhciReadReg (Offset));

  //
  // Clear any port interrupt status
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  AhciWriteReg (Offset, AhciReadReg (Offset));

  //
  // Clear any HBA interrupt status
  //
  AhciWriteReg (EFI_AHCI_IS_OFFSET, AhciReadReg (EFI_AHCI_IS_OFFSET));
}

/**
  Enable the FIS running for giving port.
    
  @param      Port           The number of port.
  @param      Timeout        The timeout value of enabling FIS.

  @retval EFI_DEVICE_ERROR   The FIS enable setting fails.
  @retval EFI_TIMEOUT        The FIS enable setting is time out.
  @retval EFI_SUCCESS        The FIS enable successfully.

**/
EFI_STATUS
EFIAPI
AhciEnableFisReceive (
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )     
{ 
  UINT32 Offset;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (Offset, EFI_AHCI_PORT_CMD_FRE);

  return AhciWaitMemSet (   
           Offset,
           EFI_AHCI_PORT_CMD_FR,
           EFI_AHCI_PORT_CMD_FR,
           Timeout
           );
}

/**
  Disable the FIS running for giving port.

  @param      Port           The number of port.
  @param      Timeout        The timeout value of disabling FIS.

  @retval EFI_DEVICE_ERROR   The FIS disable setting fails.
  @retval EFI_TIMEOUT        The FIS disable setting is time out.
  @retval EFI_UNSUPPORTED    The port is in running state.
  @retval EFI_SUCCESS        The FIS disable successfully.

**/
EFI_STATUS
EFIAPI
AhciDisableFisReceive (
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )  
{
  UINT32 Offset;
  UINT32 Data;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data   = AhciReadReg (Offset);

  //
  // Before disabling Fis receive, the DMA engine of the port should NOT be in running status.
  //
  if ((Data & (EFI_AHCI_PORT_CMD_ST | EFI_AHCI_PORT_CMD_CR)) != 0) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Check if the Fis receive DMA engine for the port is running.
  //
  if ((Data & EFI_AHCI_PORT_CMD_FR) != EFI_AHCI_PORT_CMD_FR) {
    return EFI_SUCCESS;
  }

  AhciAndReg (Offset, (UINT32)~(EFI_AHCI_PORT_CMD_FRE));

  return AhciWaitMemSet (
           Offset,
           EFI_AHCI_PORT_CMD_FR,
           0,
           Timeout
           ); 
}

/**
  Build the command list, command table and prepare the fis receiver.
    
  @param    AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param    Port                  The number of port.
  @param    PortMultiplier        The timeout value of stop.
  @param    CommandFis            The control fis will be used for the transfer.
  @param    CommandList           The command list will be used for the transfer.
  @param    AtapiCommand          The atapi command will be used for the transfer.
  @param    AtapiCommandLength    The length of the atapi command.
  @param    CommandSlotNumber     The command slot will be used for the transfer.
  @param    DataPhysicalAddr      The pointer to the data buffer pci bus master address.
  @param    DataLength            The data count to be transferred.

**/  
VOID
EFIAPI
AhciBuildCommand (
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_COMMAND_FIS       *CommandFis,
  IN     EFI_AHCI_COMMAND_LIST      *CommandList,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,
  IN     UINT8                      CommandSlotNumber,
  IN OUT VOID                       *DataPhysicalAddr,
  IN     UINT64                     DataLength
  )   
{
  UINT64     BaseAddr; 
  UINT64     PrdtNumber;
  UINTN      RemainedData;
  UINTN      MemAddr;
  DATA_64    Data64;
  UINT32     Offset;

  //
  // Filling the PRDT
  //  
  PrdtNumber = (DataLength + EFI_AHCI_MAX_DATA_PER_PRDT - 1) / EFI_AHCI_MAX_DATA_PER_PRDT;

  //
  // According to AHCI 1.3 spec, a PRDT entry can point to a maximum 4MB data block.
  // It also limits that the maximum amount of the PRDT entry in the command table
  // is 65535.
  //
  ASSERT (PrdtNumber <= 1);

 // Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis) + sizeof (EFI_AHCI_RECEIVED_FIS) * Port;
  Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis);

  BaseAddr = Data64.Uint64;
  
  ZeroMem ((VOID *)((UINTN) BaseAddr), sizeof (EFI_AHCI_RECEIVED_FIS));  
    
  ZeroMem (AhciRegisters->AhciCommandTable, sizeof (EFI_AHCI_COMMAND_TABLE));

  CommandFis->AhciCFisPmNum = PortMultiplier;
  
  CopyMem (&AhciRegisters->AhciCommandTable->CommandFis, CommandFis, sizeof (EFI_AHCI_COMMAND_FIS));

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  if (AtapiCommand != NULL) {
    CopyMem (
      &AhciRegisters->AhciCommandTable->AtapiCmd,
      AtapiCommand,
      AtapiCommandLength
      );

    CommandList->AhciCmdA = 1;
    CommandList->AhciCmdP = 1;
    CommandList->AhciCmdC = (DataLength == 0) ? 1 : 0;

    AhciOrReg (Offset, (EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  } else {
    AhciAndReg (Offset, (UINT32)~(EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  }
  
  RemainedData = (UINTN) DataLength;
  MemAddr      = (UINTN) DataPhysicalAddr;
  CommandList->AhciCmdPrdtl = (UINT32)PrdtNumber;
  
  AhciRegisters->AhciCommandTable->PrdtTable.AhciPrdtDbc = (UINT32)RemainedData - 1;

  Data64.Uint64 = (UINT64)MemAddr;
  AhciRegisters->AhciCommandTable->PrdtTable.AhciPrdtDba  = Data64.Uint32.Lower32;
  AhciRegisters->AhciCommandTable->PrdtTable.AhciPrdtDbau = Data64.Uint32.Upper32;

  //
  // Set the last PRDT to Interrupt On Complete
  //
  AhciRegisters->AhciCommandTable->PrdtTable.AhciPrdtIoc = 1;

  CopyMem (
    (VOID *) ((UINTN) AhciRegisters->AhciCmdList + (UINTN) CommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST)),
    CommandList,
    sizeof (EFI_AHCI_COMMAND_LIST)
    );  

  Data64.Uint64 = (UINT64)(UINTN) AhciRegisters->AhciCommandTable;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtba  = Data64.Uint32.Lower32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtbau = Data64.Uint32.Upper32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdPmp   = PortMultiplier;

}

/**
  Buid a command FIS.
    
  @param  CmdFis            A pointer to the EFI_AHCI_COMMAND_FIS data structure.
  @param  AtaCommandBlock   A pointer to the AhciBuildCommandFis data structure.

**/
VOID
EFIAPI
AhciBuildCommandFis (
  IN OUT EFI_AHCI_COMMAND_FIS    *CmdFis,
  IN     EFI_ATA_COMMAND_BLOCK   *AtaCommandBlock
  )
{
  ZeroMem (CmdFis, sizeof (EFI_AHCI_COMMAND_FIS));

  CmdFis->AhciCFisType = EFI_AHCI_FIS_REGISTER_H2D;
  //
  // Indicator it's a command
  //
  CmdFis->AhciCFisCmdInd      = 0x1; 
  CmdFis->AhciCFisCmd         = AtaCommandBlock->AtaCommand;

  CmdFis->AhciCFisFeature     = AtaCommandBlock->AtaFeatures;
  CmdFis->AhciCFisFeatureExp  = AtaCommandBlock->AtaFeaturesExp;

  CmdFis->AhciCFisSecNum      = AtaCommandBlock->AtaSectorNumber;
  CmdFis->AhciCFisSecNumExp   = AtaCommandBlock->AtaSectorNumberExp;

  CmdFis->AhciCFisClyLow      = AtaCommandBlock->AtaCylinderLow;
  CmdFis->AhciCFisClyLowExp   = AtaCommandBlock->AtaCylinderLowExp;

  CmdFis->AhciCFisClyHigh     = AtaCommandBlock->AtaCylinderHigh;
  CmdFis->AhciCFisClyHighExp  = AtaCommandBlock->AtaCylinderHighExp;

  CmdFis->AhciCFisSecCount    = AtaCommandBlock->AtaSectorCount;
  CmdFis->AhciCFisSecCountExp = AtaCommandBlock->AtaSectorCountExp;

  CmdFis->AhciCFisDevHead     = (UINT8) (AtaCommandBlock->AtaDeviceHead | 0xE0);
}

/**
  Stop command running for giving port
    
  @param  Port               The number of port.
  @param  Timeout            The timeout value of stop.
   
  @retval EFI_DEVICE_ERROR   The command stop unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command stop successfully.

**/
EFI_STATUS
EFIAPI
AhciStopCommand (
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )
{
  UINT32 Offset;
  UINT32 Data;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data   = AhciReadReg (Offset);

  if ((Data & (EFI_AHCI_PORT_CMD_ST |  EFI_AHCI_PORT_CMD_CR)) == 0) {
    return EFI_SUCCESS;    
  }

  if ((Data & EFI_AHCI_PORT_CMD_ST) != 0) {
    AhciAndReg (Offset, (UINT32)~(EFI_AHCI_PORT_CMD_ST));
  }

  return AhciWaitMemSet (
           Offset,
           EFI_AHCI_PORT_CMD_CR,
           0,
           Timeout
           ); 
}

/**
  Start command for give slot on specific port.
    
  @param  Port               The number of port.
  @param  CommandSlot        The number of CommandSlot.
  @param  Timeout            The timeout value of start.
   
  @retval EFI_DEVICE_ERROR   The command start unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command start successfully.

**/
EFI_STATUS
EFIAPI
AhciStartCommand (
  IN  UINT8                     Port,
  IN  UINT8                     CommandSlot,
  IN  UINT64                    Timeout
  )
{
  UINT32     CmdSlotBit;
  EFI_STATUS Status;
  UINT32     PortStatus;
  UINT32     StartCmd;
  UINT32     PortTfd;
  UINT32     Offset;
  UINT32     Capability;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg(EFI_AHCI_CAPABILITY_OFFSET);

  CmdSlotBit = (UINT32) (1 << CommandSlot);

  AhciClearPortStatus (
    Port
    );

  Status = AhciEnableFisReceive (
             Port,
             Timeout
             );
  
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Setting the command
  //
  if(FeaturePcdGet(PcdSetActiveForAhciCommand)){  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SACT;
    AhciAndReg (Offset, 0);
    AhciOrReg (Offset, CmdSlotBit);
  }
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  AhciAndReg (Offset, 0);
  AhciOrReg (Offset, CmdSlotBit);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  PortStatus = AhciReadReg (Offset);
  
  StartCmd = 0;
  if ((PortStatus & EFI_AHCI_PORT_CMD_ALPE) != 0) {
    StartCmd = AhciReadReg (Offset);
    StartCmd &= ~EFI_AHCI_PORT_CMD_ICC_MASK;
    StartCmd |= EFI_AHCI_PORT_CMD_ACTIVE;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  PortTfd = AhciReadReg (Offset);

  if ((PortTfd & (EFI_AHCI_PORT_TFD_BSY | EFI_AHCI_PORT_TFD_DRQ)) != 0) {
    if ((Capability & BIT24) != 0) {
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
      AhciOrReg (Offset, EFI_AHCI_PORT_CMD_CLO);

      Status = AhciWaitMemSet (
        Offset,
        EFI_AHCI_PORT_CMD_CLO,
        0,
        Timeout
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (Offset, EFI_AHCI_PORT_CMD_ST | StartCmd);

  return EFI_SUCCESS;
}

/**
  Start a PIO data transfer on specific port.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  AtapiCommand        The atapi command will be used for the transfer.
  @param  AtapiCommandLength  The length of the atapi command.
  @param  Read                The transfer direction.
  @param  AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param  MemoryAddr          The pointer to the data buffer.
  @param  DataCount           The data count to be transferred.
  @param  Timeout             The timeout value of non data transfer.

  @retval EFI_DEVICE_ERROR    The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The PIO data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciPioTransfer (
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,  
  IN     BOOLEAN                    Read,  
  IN     EFI_ATA_COMMAND_BLOCK      *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK       *AtaStatusBlock,
  IN OUT VOID                       *MemoryAddr,
  IN     UINT32                     DataCount,
  IN     UINT64                     Timeout 
  )
{
  EFI_STATUS                    Status;
  UINTN                         FisBaseAddr;
  UINT32                        Offset;
  UINT32                        Value;
  UINT32                        Delay;
  EFI_AHCI_COMMAND_FIS          CFis;
  EFI_AHCI_COMMAND_LIST         CmdList;
  UINT32                        RegWaitSet;

  //
  // Package read needed
  //
  AhciBuildCommandFis(&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;
  CmdList.AhciCmdW   = Read ? 0 : 1;

  AhciBuildCommand (
    AhciRegisters,
    Port,
    PortMultiplier,
    &CFis,
    &CmdList,
    AtapiCommand,
    AtapiCommandLength,
    0,
    (VOID *)(UINTN)MemoryAddr,
    DataCount
    );    
  
  Status = AhciStartCommand (
             Port, 
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  //
  // Checking the status and wait the driver sending data
  //
  
//  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);
  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis;
	
  //
  // Wait device sends the PIO setup fis before data transfer
  //
  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    Value = *(volatile UINT32 *) (FisBaseAddr + EFI_AHCI_PIO_FIS_OFFSET);
    if ((Value & EFI_AHCI_FIS_TYPE_MASK) == EFI_AHCI_FIS_PIO_SETUP) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay--;    
  } while (Delay > 0);

  if (Delay == 0) {
    Status = EFI_TIMEOUT;
    goto Exit;
  }

  //
  // Wait for command compelte
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  Status = AhciWaitMemSet (
             Offset,
             0xFFFFFFFF,
             0,
             Timeout
             );

  if (EFI_ERROR (Status)) {
    goto Exit;   
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  RegWaitSet = Read?EFI_AHCI_PORT_IS_PSS:EFI_AHCI_PORT_IS_DHRS;  
  Status = AhciWaitMemSet (
             Offset,            
             RegWaitSet,
             RegWaitSet,
             Timeout
             );  
  if (EFI_ERROR (Status)) {
    goto Exit;  
  }

Exit: 
  AhciStopCommand (
    Port,
    Timeout
    );
  
  AhciDisableFisReceive (
    Port,
    Timeout
    );

  return Status;
}






/**
  Start a non data transfer on specific port.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  AtapiCommand        The atapi command will be used for the transfer.
  @param  AtapiCommandLength  The length of the atapi command.
  @param  AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param  Timeout             The timeout value of non data transfer.

  @retval EFI_DEVICE_ERROR    The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The non data transfer executes successfully.

**/ 
EFI_STATUS
EFIAPI
AhciNonDataTransfer (
  IN     EFI_AHCI_REGISTERS            *AhciRegisters,
  IN     UINT8                         Port,
  IN     UINT8                         PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND        *AtapiCommand OPTIONAL,
  IN     UINT8                         AtapiCommandLength,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN     UINT64                        Timeout
  ) 
{
  EFI_STATUS                   Status;  
  UINTN                        FisBaseAddr;
  UINT32                       Offset;
  UINT32                       Value;
  UINT32                       Delay;
  
  EFI_AHCI_COMMAND_FIS         CFis;
  EFI_AHCI_COMMAND_LIST        CmdList;

  //
  // Package read needed
  //
  AhciBuildCommandFis(&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;

  AhciBuildCommand (
    AhciRegisters,
    Port,
    PortMultiplier,
    &CFis,
    &CmdList,
    AtapiCommand,
    AtapiCommandLength,
    0,
    NULL,
    0
    ); 
  
  Status = AhciStartCommand (
             Port, 
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  //
  // Wait device sends the Response Fis
  //
//FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);
  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis;

  //
  // Wait device sends the PIO setup fis before data transfer
  //
  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    Value = *(volatile UINT32 *) (FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET);

    if ((Value & EFI_AHCI_FIS_TYPE_MASK) == EFI_AHCI_FIS_REGISTER_D2H) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay --;    
  } while (Delay > 0);

  if (Delay == 0) {
    Status = EFI_TIMEOUT;
    goto Exit;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;

  Status = AhciWaitMemSet (
             Offset,
             0xFFFFFFFF,
             0,
             Timeout
             );  
  
Exit:  
  AhciStopCommand (
    Port,
    Timeout
    );
  AhciDisableFisReceive (
    Port,
    Timeout
    );
  return Status;
}




/**
  Do AHCI HBA reset.
    
  @param  Timeout            The timeout value of reset.
 
  @retval EFI_DEVICE_ERROR   AHCI controller is failed to complete hardware reset.
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        AHCI controller is reset successfully.

**/
EFI_STATUS
EFIAPI
AhciReset (
  IN  UINT64                    Timeout
  )    
{
  EFI_STATUS             Status;
  UINT32                 Delay;
  UINT32                 Value;

  AhciOrReg (EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);

  AhciOrReg (EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_RESET);

  Status  = EFI_TIMEOUT;

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg(EFI_AHCI_GHC_OFFSET);
    if ((Value & EFI_AHCI_GHC_RESET) == 0) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}




/**
  Do AHCI port reset.

  @param  Port               The number of port.
  @param  Timeout            The timeout value of reset.
   
  @retval EFI_DEVICE_ERROR   The port reset unsuccessfully
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        The port reset successfully.

**/
/*
EFI_STATUS
EFIAPI
AhciPortReset (
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )
{
  EFI_STATUS      Status;
  UINT32          Offset;  
  
  AhciClearPortStatus(Port);

  Status = AhciStopCommand (Port, Timeout);
	if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AhciDisableFisReceive (Port, Timeout);
	if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AhciEnableFisReceive (Port, Timeout);
	if (EFI_ERROR (Status)) {
    return Status;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
  AhciOrReg (Offset, EFI_AHCI_PORT_SCTL_DET_INIT);
  MicroSecondDelay (5000);		// wait 5 milliseceond before de-assert DET  
  AhciAndReg(Offset, (UINT32)EFI_AHCI_PORT_SCTL_MASK);
  MicroSecondDelay (5000);		// wait 5 milliseceond before de-assert DET

  // Wait for communication to be re-established
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
  Status = AhciWaitMemSet (
             Offset,
             EFI_AHCI_PORT_SSTS_DET_MASK,
             EFI_AHCI_PORT_SSTS_DET_PCE,
             Timeout
           ); 
  if (EFI_ERROR (Status)){
    return Status;
  }

//  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
//  AhciOrReg (Offset, EFI_AHCI_PORT_ERR_CLEAR);
  AhciClearPortStatus(Port);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  Status = AhciWaitMemSet (
             Offset,
             EFI_AHCI_PORT_TFD_MASK,
             0,
             Timeout
             ); 
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
*/




/**
  Send Buffer cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentify (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN OUT ATA_IDENTIFY_DATA    *Buffer  
  )
{
  EFI_STATUS                   Status;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;

  if (AhciRegisters == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  
  AtaCommandBlock.AtaCommand     = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaSectorCount = 1;

  Status = AhciPioTransfer (
             AhciRegisters,
             Port,
             PortMultiplier,
             NULL,
             0,
             TRUE,
             &AtaCommandBlock,
             NULL,
             Buffer,
             sizeof (ATA_IDENTIFY_DATA),
             ATA_TIMEOUT
             );

  return Status;
}

/**
  Send unlock hdd password cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciUnlockHddPassword (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN CHAR8                    Identifier,
  IN CHAR8                    *Password
  )
{
  EFI_STATUS                   Status;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;

  if (AhciRegisters == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (mBuffer, 512);
  AtaCommandBlock.AtaCommand     = 0xF2; //Security Unlock Cmd
  AtaCommandBlock.AtaSectorCount = 1;

  ((CHAR16 *)mBuffer)[0] = Identifier & BIT0;
  CopyMem (&((CHAR16 *)mBuffer)[1], Password, 32);

  Status = AhciPioTransfer (
             AhciRegisters,
             Port,
             PortMultiplier,
             NULL,
             0,
             FALSE,
             &AtaCommandBlock,
             NULL,
             mBuffer,
             512,
             ATA_TIMEOUT
             );

  return Status;
}




/**
  Send Frozen hdd security cmd to specific device.
    
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciFrozenHddSecurity (
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier
  )
{
  EFI_STATUS                   Status;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;

  if (AhciRegisters == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof(EFI_ATA_COMMAND_BLOCK));
  AtaCommandBlock.AtaCommand = 0xF5;

  Status = AhciNonDataTransfer (
             AhciRegisters,
             Port,
             PortMultiplier,
             NULL,
             0,
             &AtaCommandBlock,
             NULL,
             ATA_TIMEOUT
             );

  return Status;
}





/**
  Get AHCI mode base address registers' value. 

  @param[in] Bus         The bus number of ata host controller.
  @param[in] Device      The device number of ata host controller.
  @param[in] Function    The function number of ata host controller.
           
  @retval EFI_UNSUPPORTED        Return this value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation data error

**/
EFI_STATUS
EFIAPI
GetAhciBaseAddress (
  IN     UINTN                       Bus,
  IN     UINTN                       Device,
  IN     UINTN                       Function
  )
{
  mAhciBar = PciRead32(PCI_LIB_ADDRESS(Bus, Device, Function, 0x24)) & 0xFFFFFFF0;

  return EFI_SUCCESS;
}

/**
  Allocate transfer-related data struct which is used at AHCI mode.
  
  @retval  EFI_OUT_OF_RESOURCE   The allocation is failure.
  @retval  EFI_SUCCESS           Successful to allocate memory.

**/
EFI_STATUS
EFIAPI
AhciAllocateResource (
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Base;

  //
  // Allocate runtime ACPI script table space. We need it to save some
  // settings done by CSM, which runs after normal script table closed
  //
  Base = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof (EFI_AHCI_RECEIVED_FIS)),
                  (EFI_PHYSICAL_ADDRESS *)&Base
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem ((VOID *)(UINTN)Base, sizeof (EFI_AHCI_RECEIVED_FIS));
  mAhciRegisters.AhciRFis = (VOID *)(UINTN)Base;

  Base = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof (EFI_AHCI_COMMAND_LIST)),
                  (EFI_PHYSICAL_ADDRESS *)&Base
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mAhciRegisters.AhciRFis);
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem ((VOID *)(UINTN)Base, sizeof (EFI_AHCI_COMMAND_LIST));
  mAhciRegisters.AhciCmdList = (VOID *)(UINTN)Base;

  Base = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof (EFI_AHCI_COMMAND_TABLE)),
                  (EFI_PHYSICAL_ADDRESS *)&Base
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mAhciRegisters.AhciRFis);
    FreePool (mAhciRegisters.AhciCmdList);
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem ((VOID *)(UINTN)Base, sizeof (EFI_AHCI_COMMAND_TABLE));

  mAhciRegisters.AhciCommandTable = (VOID *)(UINTN)Base;

  return EFI_SUCCESS;
}

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller. 
  
  @param[in]  Port          The port number to do initialization.

**/
EFI_STATUS
EFIAPI
AhciModeInitialize (
  UINT8              Port
  )
{
  EFI_STATUS         Status;
  UINT32             Capability;
  UINT32             PortImplementBitMap;
  UINT32             Offset;
  UINT32             PhyDetectDelay;
  UINT32             Data;
  
  Status = AhciReset(EFI_AHCI_BUS_RESET_TIMEOUT); 
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

// Enable AE before accessing any AHCI registers
  AhciOrReg (EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);
  
  Capability           = AhciReadReg(EFI_AHCI_CAPABILITY_OFFSET);
  PortImplementBitMap  = AhciReadReg(EFI_AHCI_PI_OFFSET);
  
  if((PortImplementBitMap & (BIT0 << Port)) == 0){
    return EFI_DEVICE_ERROR;
  }
  
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_FB;
  AhciWriteReg (Offset, (UINT32)(UINTN)mAhciRegisters.AhciRFis);
  AhciWriteReg (Offset+4, 0);
  
// Single task envrionment, we only use one command table for all port
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CLB;
  AhciWriteReg (Offset, (UINT32)(UINTN)mAhciRegisters.AhciCmdList);
  AhciWriteReg (Offset+4, 0);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data = AhciReadReg(Offset);
  if((Data & EFI_AHCI_PORT_CMD_CPD) != 0){
    AhciOrReg(Offset, EFI_AHCI_PORT_CMD_POD);
  }
  if((Capability & EFI_AHCI_CAP_SSS) != 0){
    AhciOrReg(Offset, EFI_AHCI_PORT_CMD_SUD);
  }

// Disable aggressive power management.
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
  AhciOrReg (Offset, EFI_AHCI_PORT_SCTL_IPM_INIT);  

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IE;
  AhciAndReg (Offset, 0);

// Enable FIS Receive DMA engine for the first D2H FIS.
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (Offset, EFI_AHCI_PORT_CMD_FRE);
  Status = AhciWaitMemSet (
             Offset,
             EFI_AHCI_PORT_CMD_FR,
             EFI_AHCI_PORT_CMD_FR,
             EFI_AHCI_PORT_CMD_FR_CLEAR_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

// Wait no longer than 10 ms to wait the Phy to detect the presence of a device.
// It's the requirment from SATA1.0a spec section 5.2.
  PhyDetectDelay = EFI_AHCI_BUS_PHY_DETECT_TIMEOUT;
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
  do {
    Data = AhciReadReg(Offset) & EFI_AHCI_PORT_SSTS_DET_MASK;
    if ((Data == EFI_AHCI_PORT_SSTS_DET_PCE) || (Data == EFI_AHCI_PORT_SSTS_DET)) {
      break;
    }
    MicroSecondDelay (1000);
    PhyDetectDelay--;
  } while(PhyDetectDelay > 0);

  if (PhyDetectDelay == 0) {        // No device detected at this port.
    return EFI_DEVICE_ERROR;
  }

// According to SATA1.0a spec section 5.2, we need to wait for PxTFD.BSY and PxTFD.DRQ
// and PxTFD.ERR to be zero. The maximum wait time is 16s which is defined at ATA spec.

  PhyDetectDelay = 16 * 1000;
  do {
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    if (AhciReadReg(Offset) != 0) {
      AhciWriteReg (Offset, AhciReadReg(Offset));
    }
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
    Data = AhciReadReg (Offset) & EFI_AHCI_PORT_TFD_MASK;
    if (Data == 0) {
      break;
    }
    MicroSecondDelay (1000);
    PhyDetectDelay--;
  } while (PhyDetectDelay > 0);
  if (PhyDetectDelay == 0) {
    return EFI_DEVICE_ERROR;
  }

// When the first D2H register FIS is received, the content of PxSIG register is updated.
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
  Status = AhciWaitMemSet (
             Offset,
             0x0000FFFF,
             0x00000101,
             EFI_TIMER_PERIOD_SECONDS(16)
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Data = AhciReadReg(Offset);
  if((Data & EFI_AHCI_ATAPI_SIG_MASK) != EFI_AHCI_ATA_DEVICE_SIG) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

