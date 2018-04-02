/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  spiflashdevice.c

Abstract: 

Revision History:

**/
//20161010_Hongxu_ADD
//ZX100-01-MTN-01+S
#include "spiflashdevice.h"

MEDIA_BLOCK_MAP device_block_map[] = {
    RECORD_ENTRY_64M_OF_4K_BLOCKS,
    RECORD_MAP_END
};

DEVICE_TABLE mDeviceTable = {
    SF_VENDOR_ID_SST,         // VendorId
    SF_DEVICE_ID0_26VF064B,   // DeviceId 0
    SF_DEVICE_ID1_26VF064B,   // DeviceId 1
    SST_26VF064B_SIZE         // size of the flash part
};

UINT8 mPrefixOpcode[] = {
  SF_INST_WREN,       // Prefix Opcode 0: Write Enable
  SF_INST_NOP        // Prefix Opcode 1:  no operation
};

STATIC SPI_OPCODE_MENU_ENTRY mOpcodeList[] = {
  {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle104MHz,  EnumSpiOperationJedecId},                // Opcode 0: Read ID
  {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle40MHz,   EnumSpiOperationReadData},               // Opcode 1: Read
  {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle104MHz,  EnumSpiOperationReadStatus},             // Opcode 2: Read Status Register 
  {EnumSpiOpcodeReadNoAddr,     SF_INST_RBPR,           EnumSpiCycle104MHz,  EnumSpiOperationReadBlockProtection},    // Opcode 3: Read block protection Register for replace WRDI
  {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle104MHz,  EnumSpiOperationErase_4K_Byte},          // Opcode 4: Sector Erase (4KB)
  {EnumSpiOpcodeWrite,          SF_INST_BERASE,         EnumSpiCycle104MHz,  EnumSpiOperationErase_64K_Byte_MAX},     // Opcode 5: Block Erase (8KB/32KB/64KB depending on address)
  {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle104MHz,  EnumSpiOperationProgramData_64_Byte},    // Opcode 6: Page Program 
  {EnumSpiOpcodeWriteNoAddr,    SF_INST_ULBPR,          EnumSpiCycle104MHz,  EnumSpiOperationGlobalBlockProtectionUnlock},            // Opcode 7: global block protection unlock for replace WRSR
};

//
// Global variables
//
NV_DEVICE_INSTANCE   *mNvDevice = NULL;

/**
  SPI Flash device Driver entry point.

  Register SPI flash device access method to NVMediaAccess Protocol.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.
  @param[in] SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES Fails to allocate memory for device.
  @retval other                Some error occurs when executing this entry point.

**/
EFI_STATUS
DriverEntry (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    NV_MEDIA_ACCESS_PROTOCOL *pMediaAccessProtocol;
    VOID                     *Interface;    

//  DEBUG((EFI_D_ERROR, "+++ SST26VF064B Dxe In.\n")); 			  
    Status = gBS->LocateProtocol(&gEfiNvMediaDeviceProtocolGuid, NULL, &Interface);
    if(!EFI_ERROR(Status)){
      DEBUG((EFI_D_INFO, "Spi Device Driver Already Run\n"));			
      Status = EFI_ALREADY_STARTED;
      goto ProcExit;
    }    
    //
    // Allocate runtime private data structure for the device driver.
    //
    Status = gBS->AllocatePool (
                 EfiRuntimeServicesData,        // MemoryType don't care
                 sizeof (NV_DEVICE_INSTANCE),
                 &mNvDevice
             );
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "ERROR - Allocatepool for mNvDevice failed!\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem ((VOID *) mNvDevice, sizeof (NV_DEVICE_INSTANCE));

    mNvDevice->Signature = NV_DEVICE_DATA_SIGNATURE;
    mNvDevice->Handle = ImageHandle;

    mNvDevice->DeviceProtocol.Info  = device_info;
    mNvDevice->DeviceProtocol.Sense = device_sense;
    mNvDevice->DeviceProtocol.Read  = devcie_read;
    mNvDevice->DeviceProtocol.Write = device_write;
    mNvDevice->DeviceProtocol.Erase = device_erase;
    mNvDevice->DeviceProtocol.Lock  = device_lock;

    mNvDevice->BlockMap = &device_block_map[0];

    InitSpiTable(&mNvDevice->InitTable, &mDeviceTable, mOpcodeList, mPrefixOpcode);

    mNvDevice->Number = FixedPcdGet32 (PcdNvMediaDeviceNumbers);
    mNvDevice->FlashSize = SST_26VF064B_SIZE;
    mNvDevice->SectorSize = SF_SECTOR_SIZE;

    //
    // Locate the SPI protocol.
    //
    Status = gBS->LocateProtocol (&gEfiSpiProtocolGuid, NULL, &(mNvDevice->SpiProtocol));
    ASSERT_EFI_ERROR (Status);

    //
    // Locate the platform access protocol.
    //
    Status = gBS->LocateProtocol (&gEfiPlatformAccessProtocolGuid, NULL, &(mNvDevice->PlatformAccessProtocol));

    //
    // Locate the NvNedia access protocol.
    //
    Status = gBS->LocateProtocol (&gEfiNvMediaAccessProtocolGuid, NULL, &pMediaAccessProtocol);
    ASSERT_EFI_ERROR (Status);

    //
    // sense the device, and register this to NV_MEDIA_ACCESS(maybe a callback function)
    //		  

    Status = device_sense(&(mNvDevice->DeviceProtocol));
    if (!EFI_ERROR(Status)) {
        Status = gBS->InstallProtocolInterface (
                     &ImageHandle,
                     &gEfiNvMediaDeviceProtocolGuid,
                     EFI_NATIVE_INTERFACE,
                     &mNvDevice->DeviceProtocol);
        ASSERT_EFI_ERROR (Status);
        Status = pMediaAccessProtocol->Init(pMediaAccessProtocol, (void *)&mNvDevice->DeviceProtocol, SPI_MEDIA_TYPE);
        ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG((EFI_D_ERROR, "NOT SST26VF064B\n"));
      gBS->FreePool(mNvDevice);
      mNvDevice = NULL;
    }			  
    
ProcExit:    			  
    return Status;
}
//ZX100-01-MTN-01+E
