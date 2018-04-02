/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  HswSpiFlashLib.c

Abstract:

Revision History:

**/
#include "HswSpiFlashLib.h"
#include "Library/PcdLib.h"
SPI_ID_DATA     mSpiIdTable;

SPI_INIT_TABLE  mInitTable =   {
    {
        SF_INST_WREN,       // Prefix Opcode 0: Write Enable
        SF_INST_EWSR        // Prefix Opcode 1: Enable Write Status Register
    },
    {
        {EnumSpiCycle50MHz,  EnumSpiOperationJedecId},             // Opcode 0: Read ID
        {EnumSpiCycle50MHz,  EnumSpiOperationReadData},            // Opcode 1: Read
        {EnumSpiCycle50MHz,  EnumSpiOperationReadStatus},          // Opcode 2: Read Status Register
        {EnumSpiCycle50MHz,  EnumSpiOperationWriteDisable},        // Opcode 3: Write Disable
        {EnumSpiCycle50MHz,  EnumSpiOperationErase_4K_Byte},       // Opcode 4: Sector Erase (4KB)
        {EnumSpiCycle50MHz,  EnumSpiOperationErase_64K_Byte},      // Opcode 5: Block Erase (64KB)
        {EnumSpiCycle50MHz,  EnumSpiOperationProgramData_1_Byte},  // Opcode 6: Byte Program
        {EnumSpiCycle50MHz,  EnumSpiOperationWriteStatus},         // Opcode 7: Write Status Register
    },
    //
    // The offset of the start of the BIOS image in flash. This value is platform specific
    // and depends on the system flash map. If BIOS size is bigger then flash return -1
    //
    // ((WINBOND_W25Q64B_SIZE >= BIOS_FLASH_SIZE) ? WINBOND_W25Q64B_SIZE - BIOS_FLASH_SIZE : (UINTN) (-1)),
    0x0,
    //
    // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map
    //
    // BIOS_FLASH_SIZE
    0x0
};

void InitSpiTable (
  IN OUT SPI_INIT_TABLE         **pInitTable,
  IN DEVICE_TABLE               *pDeviceTable,
  IN SPI_OPCODE_MENU_ENTRY      *pOpcodeMenuList,
  IN UINT8                      *PrefixOpcodeList
  )
{
    INTN i;
  SPI_CHIP_DATA      *SpiIdTableEntry = NULL;
  //
  // Reallocate memory for security info structure.
  //
  SpiIdTableEntry = AllocatePool (sizeof (SPI_CHIP_DATA));
  
  if (SpiIdTableEntry != NULL) {
    *pInitTable = &mInitTable;
    mSpiIdTable.VendorId               = pDeviceTable->VendorId;
	mSpiIdTable.TypeDataNum            = 1;
	mSpiIdTable.TypeData               = SpiIdTableEntry;
    (mSpiIdTable.TypeData)->DeviceId0     = pDeviceTable->DeviceId0;
    (mSpiIdTable.TypeData)->DeviceId1     = pDeviceTable->DeviceId1;
	(mSpiIdTable.TypeData)->FlashChipSize = (UINT32) pDeviceTable->Size;

    for (i=0; i < SPI_NUM_OPCODE; i++) {
//      mInitTable.SpiCmdConfig[i].Type = pOpcodeMenuList[i].Type;
//      mInitTable.SpiCmdConfig[i].Code = pOpcodeMenuList[i].Code;
      mInitTable.SpiCmdConfig[i].Frequency = pOpcodeMenuList[i].Frequency;
      mInitTable.SpiCmdConfig[i].Operation = pOpcodeMenuList[i].Operation;
    }

    for (i = 0; i < SPI_NUM_PREFIX_OPCODE; i++) {
      mInitTable.PrefixOpcode[i] = PrefixOpcodeList[i];
    }

    mInitTable.BiosStartOffset = pDeviceTable->Size - PcdGet32(PcdFlashAreaSize); // this will be set by Fixed PCD value.
    mInitTable.BiosSize = PcdGet32(PcdFlashAreaSize);                             // this will be set by Fixed PCD value.
  }
}

EFI_STATUS
EnableBlockProtection (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN  BOOLEAN ProtectionEnable
)
{
    EFI_STATUS          Status;
    UINT8               SpiData;
    UINT8               SpiStatus;
    NV_DEVICE_INSTANCE  *DeviceInstance;
    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    if (ProtectionEnable) {
        SpiData = SF_SR_WPE;
    } else {
        SpiData = 0;
    }

    //
    // Always disable block protection to workaround tool issue.
    // Feature may be re-enabled in a future bios.
    //
    SpiData = 0;
    Status = DeviceInstance->SpiProtocol->Execute (
                 DeviceInstance->SpiProtocol,
                 SPI_WRSR,
                 SPI_EWSR,
                 TRUE,
                 TRUE,
                 TRUE,
                 0,
                 1,
                 &SpiData,
                 EnumSpiRegionAll
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Status = DeviceInstance->SpiProtocol->Execute (
                 DeviceInstance->SpiProtocol,
                 SPI_RDSR,
                 SPI_WREN,
                 TRUE,
                 FALSE,
                 FALSE,
                 0,
                 1,
                 &SpiStatus,
                 EnumSpiRegionBios
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    if ((SpiStatus & SpiData) != SpiData) {
        Status = EFI_DEVICE_ERROR;
    }

    return Status;
}

EFI_STATUS
device_info (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN OUT MEDIA_BLOCK_MAP**           MapInfo
)
{
    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);
    *MapInfo = DeviceInstance->BlockMap;
    return EFI_SUCCESS;
}

EFI_STATUS
device_sense (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this
)
{
    EFI_STATUS        Status;
    NV_DEVICE_INSTANCE  *DeviceInstance;
//    UINT8             FlashPartId[3];

  SPI_INIT_DATA                 SpiInitData;
  SPI_TYPE_DATA                 *TypeData;
//  SPI_SPECIAL_OPCODE_ENTRY      *SpecialOpcodeEntry;
  UINTN                         Index;
  UINT8                           SfId[3];
  BOOLEAN                         SpiChipFound;
//  UINT8                           FlashIndex;
  UINT8                           IdIndex;
  UINT8                           Number;
  UINT32                          FlashChipSize;
  
  SpiChipFound = FALSE;
  //FlashIndex = 0;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    Status = gBS->AllocatePool (
               EfiRuntimeServicesData,
               sizeof (SPI_TYPE_DATA),
               &(TypeData)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - Allocate for SpiInitData.TypeData failed!\n"));
      goto DriverFailed;
    }
//    Status = gBS->AllocatePool (
//               EfiRuntimeServicesData,
//               (sizeof (SPI_SPECIAL_OPCODE_ENTRY)) * SPI_NUM_OPCODE,
//               &(SpecialOpcodeEntry)
//               );
//    if (EFI_ERROR (Status)) {
//      DEBUG ((EFI_D_ERROR, "ERROR - Allocate for SpiInitData.SpecialOpcodeEntry failed!\n"));
//      goto DriverFailed;
//    }
//    SpiInitData.VendorId           = mInitTable.VendorId;
    SpiInitData.BiosSize           = mInitTable.BiosSize;
	SpiInitData.SpecialOpcodeEntry = NULL;
//    TypeData->BiosStartOffset      = mSpiIdTable.TypeData.BiosStartOffset;
//    TypeData->DeviceId0            = mSpiIdTable.TypeData->DeviceId0;
//    TypeData->DeviceId1            = mSpiIdTable.TypeDatae->DeviceId1;
//    SpiInitData.TypeDataNum        = mSpiIdTable.TypeDataNum;
//    SpiInitData.TypeData           = TypeData;
    for (Index = 0; Index < SPI_NUM_OPCODE; Index++) {
      SpiInitData.SpiCmdConfig[Index].Frequency = mInitTable.SpiCmdConfig[Index].Frequency;
      SpiInitData.SpiCmdConfig[Index].Operation = mInitTable.SpiCmdConfig[Index].Operation;
    }
//    SpiInitData.SpecialOpcodeEntry = SpecialOpcodeEntry;

    gBS->CopyMem (
      &(SpiInitData.PrefixOpcode),
      &(mInitTable.PrefixOpcode),
      SPI_NUM_PREFIX_OPCODE
      );
  Status = DeviceInstance->SpiProtocol->ReadId (DeviceInstance->SpiProtocol, 0, SfId);
  if (!EFI_ERROR (Status)) {
//    for (; FlashIndex < sizeof (mInitTable) / sizeof (SPI_INIT_DATA), SpiChipFound == FALSE; FlashIndex++) {
      Number = mSpiIdTable.TypeDataNum;
      for (IdIndex = 0; IdIndex < Number; IdIndex++) {
        if ((SfId[0] == mSpiIdTable.VendorId) &&
            (SfId[1] == mSpiIdTable.TypeData[IdIndex].DeviceId0) &&
            (SfId[2] == mSpiIdTable.TypeData[IdIndex].DeviceId1)) {
          //
          // Found a matching SPI device, FlashIndex now contains flash device
          //
          DEBUG ((EFI_D_INFO, "Found SPI Flash Type in SPI Flash Driver\n"));
          DEBUG ((EFI_D_INFO, "  Device Vendor ID = 0x%02x!\n", SfId[0]));
          DEBUG ((EFI_D_INFO, "  Device Type ID 0 = 0x%02x!\n", SfId[1]));
          DEBUG ((EFI_D_INFO, "  Device Type ID 1 = 0x%02x!\n", SfId[2]));
          FlashChipSize = (UINT32) mSpiIdTable.TypeData[IdIndex].FlashChipSize;
          if (FlashChipSize < mInitTable.BiosSize) {
            DEBUG ((EFI_D_ERROR, "ERROR - The size of BIOS image is bigger than SPI Flash device!\n"));
            ASSERT (FALSE);
            return EFI_UNSUPPORTED;
          }
          mInitTable.BiosStartOffset = FlashChipSize - mInitTable.BiosSize;
          SpiInitData.BiosStartOffset = mInitTable.BiosStartOffset;
		  Status = DeviceInstance->SpiProtocol->Init (DeviceInstance->SpiProtocol, &(SpiInitData));
          SpiChipFound = TRUE;
          break;
        }
      }
    }
//  }
	/*
    for (Index = 0; Index < SPI_NUM_OPCODE; Index++) {
      SpiInitData.SpiCmdConfig[Index].Frequency = mInitTable.SpiCmdConfig[Index].Frequency;
      SpiInitData.SpiCmdConfig[Index].Operation = mInitTable.SpiCmdConfig[Index].Operation;
      (SpecialOpcodeEntry+Index)->OpcodeIndex   = (UINT8) Index;
      (SpecialOpcodeEntry+Index)->Code          = mInitTable.SpiCmdConfig[Index].Code;
      (SpecialOpcodeEntry+Index)->Type          = mInitTable.SpiCmdConfig[Index].Type;
    }
    Status = DeviceInstance->SpiProtocol->Init (DeviceInstance->SpiProtocol, &(SpiInitData));
	

    if (!EFI_ERROR (Status)) {
        //
        // Read Vendor/Device IDs to check if the driver supports the Serial Flash device
        //
        Status = DeviceInstance->SpiProtocol->Execute (
                     DeviceInstance->SpiProtocol,
                     SPI_READ_ID,
                     SPI_WREN,
                     TRUE,
                     FALSE,
                     FALSE,
                     0,
                     3,
                     FlashPartId,
                     EnumSpiRegionAll
                 );
        if (!EFI_ERROR (Status)) {
            if ((FlashPartId[0] == DeviceInstance->InitTable->VendorId)  &&
                    (FlashPartId[1] == DeviceInstance->InitTable->DeviceId0) &&
                    (FlashPartId[2] == DeviceInstance->InitTable->DeviceId1)) {

                //
                // Found a matching SPI device, FlashIndex now contains flash device
                //
                Status = EFI_SUCCESS;
            } else {
                Status = EFI_UNSUPPORTED;
            }
        }
    }
	*/

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    return Status;

DriverFailed:

   if (DeviceInstance->PlatformAccessProtocol)
      DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);
    return EFI_UNSUPPORTED;

}

EFI_STATUS
devcie_read (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN OUT UINT8* Buffer,
    IN OUT UINTN* Length
)
{
    EFI_STATUS                Status;
    UINTN                     LbaAddress;
    UINTN                     SpiAddress;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;

    if ((Address < LbaAddress) ||
            ((Address + *Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    Status = EnableBlockProtection (this, FALSE);
    if (EFI_ERROR (Status)) {
        return EFI_ACCESS_DENIED;
    }

    //kirby: EfiCpuFlushCache (Address, *Length);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = Address - LbaAddress;

    Status = DeviceInstance->SpiProtocol->Execute(
                 DeviceInstance->SpiProtocol,
                 SPI_READ,
                 SPI_WREN,
                 TRUE,
                 FALSE,
                 FALSE,
                 (UINT32) SpiAddress,
                 (UINT32) *Length,
                 Buffer,
                 EnumSpiRegionAll
             );

    EnableBlockProtection (this, TRUE);

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    //
    // Flush the changed area to make the cache consistent
    //
    //kirby: EfiCpuFlushCache (Address, *Length);

    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.
    //
    //TBD kirby: EfiWbinvd ();
    WriteBackInvalidateDataCacheRange ((VOID *) Address, *Length);

    return Status;
}

EFI_STATUS
device_write (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINT8* Buffer,
    IN UINTN Length
)
{
    EFI_STATUS                Status;
    UINTN                     LbaAddress;
    UINTN                     SpiAddress;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;

    if ((Address < LbaAddress) ||
            ((Address + Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    Status = EnableBlockProtection (this, FALSE);
    if (EFI_ERROR (Status)) {
        return EFI_ACCESS_DENIED;
    }

    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = Address - LbaAddress;

    Status = DeviceInstance->SpiProtocol->Execute(
                 DeviceInstance->SpiProtocol,
                 SPI_PROG,
                 SPI_WREN,
                 TRUE,
                 TRUE,
                 TRUE,
                 (UINT32) SpiAddress,
                 (UINT32) Length,
                 Buffer,
                 EnumSpiRegionAll
             );

    EnableBlockProtection (this, TRUE);

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    //
    // Flush the changed area to make the cache consistent
    //
    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.
    //
    //TBD kirby: EfiWbinvd ();
    WriteBackInvalidateDataCacheRange ((VOID *) Address, Length);

    return Status;
}
EFI_STATUS
device_erase (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    EFI_STATUS Status;
    UINTN LbaAddress;
    UINTN BlockCount;
    UINTN SpiAddress;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;

    if ((Address < LbaAddress) ||
            ((Address + Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if ((Length % DeviceInstance->SectorSize) != 0) {
        return EFI_UNSUPPORTED;
    }

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    Status = EnableBlockProtection (this, FALSE);
    if (EFI_ERROR (Status)) {
        return EFI_ACCESS_DENIED;
    }

    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = (UINT32) (Address - LbaAddress);

    BlockCount = Length / DeviceInstance->SectorSize;
    while (BlockCount > 0) {
        Status = DeviceInstance->SpiProtocol->Execute(
                     DeviceInstance->SpiProtocol,
                     SPI_SERASE,
                     SPI_WREN,
                     FALSE,
                     TRUE,
                     FALSE,
                     (UINT32) SpiAddress,
                     0,
                     NULL,
                     EnumSpiRegionAll
                 );
        if (EFI_ERROR(Status)) {
            break;
        }
        SpiAddress += DeviceInstance->SectorSize;
        BlockCount--;
    }

    EnableBlockProtection (this, TRUE);

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    //
    // Flush the changed area to make the cache consistent
    //
    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.
    //
    //TBD kirby: EfiWbinvd ();
    WriteBackInvalidateDataCacheRange ((VOID *) Address, Length);

    return Status;
}

EFI_STATUS
device_lock (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}
