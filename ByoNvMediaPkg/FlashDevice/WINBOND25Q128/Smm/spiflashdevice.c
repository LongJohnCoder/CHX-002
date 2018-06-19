/** @file

Copyright (c) 2006 - 2013, Byosoft Corporation.<BR> 
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
#include "spiflashdevice.h"

EFI_HANDLE  mHandle = NULL;

MEDIA_BLOCK_MAP device_block_map[] = {
    RECORD_ENTRY_128M_OF_4K_BLOCKS,
    RECORD_MAP_END
};

DEVICE_TABLE mDeviceTable = {
    SF_VENDOR_ID_WINBOND,    // VendorId
    SF_DEVICE_ID0_W25Q128B,   // DeviceId 0
    SF_DEVICE_ID1_W25Q128B,   // DeviceId 1
    WINBOND_W25Q128B_SIZE     // size of the flash part
};

UINT8 mPrefixOpcode[] = {
  SF_INST_WREN,       // Prefix Opcode 0: Write Enable
  SF_INST_EWSR        // Prefix Opcode 1: Enable Write Status Register
};

STATIC SPI_OPCODE_MENU_ENTRY mOpcodeList[] = {
  {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz,  EnumSpiOperationJedecId},             // Opcode 0: Read ID
  {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz,  EnumSpiOperationReadData},            // Opcode 1: Read
  {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz,  EnumSpiOperationReadStatus},          // Opcode 2: Read Status Register
  {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz,  EnumSpiOperationWriteDisable},        // Opcode 3: Write Disable
  {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz,  EnumSpiOperationErase_4K_Byte},       // Opcode 4: Sector Erase (4KB)
  {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz,  EnumSpiOperationErase_64K_Byte},      // Opcode 5: Block Erase (64KB)
  {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz,  EnumSpiOperationProgramData_1_Byte},  // Opcode 6: Byte Program
  {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz,  EnumSpiOperationWriteStatus},         // Opcode 7: Write Status Register
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
		VOID					 *Interface;	
	
		Status = gBS->LocateProtocol(&gEfiSmmNvMediaDeviceProtocolGuid, NULL, &Interface);
		if(!EFI_ERROR(Status)){
		  DEBUG((EFI_D_INFO, "Spi Device Driver Already Run\n"));			
		  Status = EFI_ALREADY_STARTED;
		  goto ProcExit;
		}	 

    //
    // Allocate pool for SPI protocol instance
    //
    Status = gSmst->SmmAllocatePool (
                 EfiRuntimeServicesData, // MemoryType don't care
                 sizeof (NV_DEVICE_INSTANCE),
                 &mNvDevice
             );
    ASSERT_EFI_ERROR (Status);

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
    mNvDevice->FlashSize = WINBOND_W25Q128B_SIZE;
    mNvDevice->SectorSize = SF_SECTOR_SIZE;

    //
    // Locate the SPI protocol.
    //
    Status = gSmst->SmmLocateProtocol (&gEfiSmmSpiProtocolGuid, NULL, &(mNvDevice->SpiProtocol));
    ASSERT_EFI_ERROR (Status);

    //
    // Locate the platform access protocol.
    //
    Status = gBS->LocateProtocol (&gEfiSmmPlatformAccessProtocolGuid, NULL, &(mNvDevice->PlatformAccessProtocol));

    //
    // Locate the platform access protocol.
    //
    Status = gBS->LocateProtocol (&gEfiSmmNvMediaAccessProtocolGuid, NULL, &pMediaAccessProtocol);
    ASSERT_EFI_ERROR (Status);

    //
    // sense the device, and register this to NV_MEDIA_ACCESS(maybe a callback function)
    //

    Status = device_sense(&(mNvDevice->DeviceProtocol));
//    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR(Status)) {
        Status = gBS->InstallProtocolInterface (
                     &mHandle,
                     &gEfiSmmNvMediaDeviceProtocolGuid,
                     EFI_NATIVE_INTERFACE,
                     &mNvDevice->DeviceProtocol);
        ASSERT_EFI_ERROR (Status);

        Status = pMediaAccessProtocol->Init(pMediaAccessProtocol, (void *)&mNvDevice->DeviceProtocol, SPI_MEDIA_TYPE);
        /*
              // Test Example:
              {
                UINT8 * buffer;
                UINTN   length;

                Status =  gBS->AllocatePool (EfiRuntimeServicesData, sizeof (4096), &buffer);
                if (EFI_ERROR (Status)) {
                  return Status;
                }
                length = 4096;
        	    Status = pMediaAccessProtocol->Read(pMediaAccessProtocol, 0xfffe0000, (void *)buffer, &length, SPI_MEDIA_TYPE);

                length = 4096;
        	    Status = pMediaAccessProtocol->Erase(pMediaAccessProtocol, 0xfffe0000, length, SPI_MEDIA_TYPE);

                length = 4096;
        	    Status = pMediaAccessProtocol->Write(pMediaAccessProtocol, 0xfffe0000, 	(void *)buffer, length, SPI_MEDIA_TYPE);
              }
        */
    }else {
      DEBUG((EFI_D_ERROR, "NOT WINBOND25Q128\n"));
      gSmst->SmmFreePool(mNvDevice);
      mNvDevice = NULL;
    }			  
    
ProcExit:    			  
    return Status;
}
