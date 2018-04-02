/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  ReplaceSpi.h

Abstract: 

Revision History:

**/
/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/*++

Copyright (c) 2006 - 2010 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  Spi.h

Abstract:

  This file defines the EFI SPI Protocol which implements the
  Intel(R) ICH SPI Host Controller Compatibility Interface.

--*/
#ifndef _EFI_SPI_H_
#define _EFI_SPI_H_

//
// Define the SPI protocol GUID
//
// EDK and EDKII have different GUID formats
//
#if !defined(EDK_RELEASE_VERSION) || (EDK_RELEASE_VERSION < 0x00020000)
#define EFI_SPI_PROTOCOL_GUID \
  { \
    0xf8b84ae6, 0x8465, 0x4f95, 0x9f, 0xb, 0xea, 0xaa, 0x37, 0xc6, 0x15, 0x5a \
  }
#define EFI_SMM_SPI_PROTOCOL_GUID \
  { \
    0xbd75fe35, 0xfdce, 0x49d7, 0xa9, 0xdd, 0xb2, 0x6f, 0x1f, 0xc6, 0xb4, 0x37 \
  }
#else
#define EFI_SPI_PROTOCOL_GUID \
  { \
    0xf8b84ae6, 0x8465, 0x4f95, \
    { \
      0x9f, 0xb, 0xea, 0xaa, 0x37, 0xc6, 0x15, 0x5a \
    } \
  }
#define EFI_SMM_SPI_PROTOCOL_GUID \
  { \
    0xbd75fe35, 0xfdce, 0x49d7, \
    { \
      0xa9, 0xdd, 0xb2, 0x6f, 0x1f, 0xc6, 0xb4, 0x37 \
    } \
  }

#endif
//
// Extern the GUID for protocol users.
//
extern EFI_GUID                   gEfiSpiProtocolGuid;
extern EFI_GUID                   gEfiSmmSpiProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_SPI_PROTOCOL  EFI_SPI_PROTOCOL;

//
// SPI protocol data structures and definitions
//
//
// Number of Prefix Opcodes allowed on the SPI interface
//
#define SPI_NUM_PREFIX_OPCODE 2

//
// Number of Opcodes in the Opcode Menu
//
#define SPI_NUM_OPCODE  8

//
// Opcode Type
//    EnumSpiOpcodeCommand: Command without address
//    EnumSpiOpcodeRead: Read with address
//    EnumSpiOpcodeWrite: Write with address
//
typedef enum {
    EnumSpiOpcodeReadNoAddr,
    EnumSpiOpcodeWriteNoAddr,
    EnumSpiOpcodeRead,
    EnumSpiOpcodeWrite,
    EnumSpiOpcodeMax
} SPI_OPCODE_TYPE;

typedef enum {
    EnumSpiCycle20MHz,
    EnumSpiCycle33MHz,
    EnumSpiCycle66MHz,  // not supported by PCH
    EnumSpiCycle50MHz,
    EnumSpiCycleMax
} SPI_CYCLE_FREQUENCY;

typedef enum {
    EnumSpiRegionAll,
    EnumSpiRegionBios,
    EnumSpiRegionMe,
    EnumSpiRegionGbE,
    EnumSpiRegionDescriptor,
    EnumSpiRegionPlatformData,
    EnumSpiRegionMax
} SPI_REGION_TYPE;

//
// Hardware Sequencing required operations (as listed in CougarPoinr EDS Table 5-55: "Hardware
// Sequencing Commands and Opcode Requirements"
//
typedef enum {
    EnumSpiOperationWriteStatus,
    EnumSpiOperationProgramData_1_Byte,
    EnumSpiOperationProgramData_64_Byte,
    EnumSpiOperationReadData,
    EnumSpiOperationWriteDisable,
    EnumSpiOperationReadStatus,
    EnumSpiOperationWriteEnable,
    EnumSpiOperationFastRead,
    EnumSpiOperationEnableWriteStatus,
    EnumSpiOperationErase_256_Byte,
    EnumSpiOperationErase_4K_Byte,
    EnumSpiOperationErase_8K_Byte,
    EnumSpiOperationErase_64K_Byte,
    EnumSpiOperationFullChipErase,
    EnumSpiOperationJedecId,
    EnumSpiOperationDualOutputFastRead,
    EnumSpiOperationDiscoveryParameters,
    EnumSpiOperationOther,
    EnumSpiOperationMax
} SPI_OPERATION;

//
// Opcode menu entries
//   Type            Operation Type (value to be programmed to the OPTYPE register)
//   Code            The opcode (value to be programmed to the OPMENU register)
//   Frequency       The expected frequency to be used (value to be programmed to the SSFC
//                   Register)
//   Operation       Which Hardware Sequencing required operation this opcode respoinds to.
//                   The required operations are listed in EDS Table 5-55: "Hardware
//                   Sequencing Commands and Opcode Requirements"
//                   If the opcode does not corresponds to any operation listed, use
//                   EnumSpiOperationOther
//
typedef struct _SPI_OPCODE_MENU_ENTRY {
    SPI_OPCODE_TYPE     Type;
    UINT8               Code;
    SPI_CYCLE_FREQUENCY Frequency;
    SPI_OPERATION       Operation;
} SPI_OPCODE_MENU_ENTRY;

typedef struct _SPI_ID_TABLE {
  UINT8       VendorId;
  UINT8       DeviceId0;
  UINT8       DeviceId1;
} SPI_ID_TABLE;

//
// Initialization data that can be used to identify SPI flash part
//    DeviceId0       Device ID0 of the SPI device
//    DeviceId1       Device ID1 of the SPI device
//    FlashChipSize   The size of flash chip
//
typedef struct _SPI_CHIP_DATA {
  UINT8  DeviceId0;
  UINT8  DeviceId1;
  UINT32 FlashChipSize;
} SPI_CHIP_DATA;

typedef struct _SPI_ID_DATA {
  UINT8                          VendorId;
  UINT8                          TypeDataNum;
  SPI_CHIP_DATA                  *TypeData;
} SPI_ID_DATA;

typedef struct _SPI_FLASH_DATA {
  UINT8 DeviceId0;
  UINT8 DeviceId1;
  UINTN BiosStartOffset;  // (Flash part Size - Bios Size)
} SPI_TYPE_DATA;

typedef struct _SPI_COMMAND_CONFIG {
  SPI_CYCLE_FREQUENCY Frequency;
  SPI_OPERATION       Operation;
} SPI_COMMAND_CONFIG;

typedef struct _SPI_SPECIAL_OPCODE_ENTRY {
  UINT8           OpcodeIndex;
  SPI_OPCODE_TYPE Type;
  UINT8           Code;
} SPI_SPECIAL_OPCODE_ENTRY;

///
/// Initialization data table loaded to the SPI host controller
///    PrefixOpcode    Prefix opcodes which are loaded into the SPI host controller
///    SpiCmdConfig    Determines Opcode Type, Menu and Frequency of the SPI commands
///    BiosStartOffset The offset of the start of the BIOS image relative to the flash device.
///                    Please note this is a Flash Linear Address, NOT a memory space address.
///                    This value is platform specific and depends on the system flash map.
///                    This value is only used on non Descriptor mode.
///    BiosSize        The the BIOS Image size in flash. This value is platform specific
///                    and depends on the system flash map. Please note BIOS Image size may
///                    be smaller than BIOS Region size (in Descriptor Mode) or the flash size
///                    (in Non Descriptor Mode), and in this case, BIOS Image is supposed to be
///                    placed at the top end of the BIOS Region (in Descriptor Mode) or the flash
///                    (in Non Descriptor Mode)
///
typedef struct _SPI_INIT_TABLE {
  UINT8               PrefixOpcode[SPI_NUM_PREFIX_OPCODE];
  SPI_COMMAND_CONFIG  SpiCmdConfig[SPI_NUM_OPCODE];
  UINTN               BiosStartOffset;
  UINTN               BiosSize;
} SPI_INIT_TABLE;

///
/// Initialization data table loaded to the SPI host controller
///
/// Note:  Most of time, the SPI flash parts with the same vendor would have the same
///        Prefix Opcode, Opcode menu, so you can provide one table for the SPI flash parts with
///        the same vendor.
///
typedef struct _SPI_INIT_DATA {
  ///
  /// Prefix opcodes which are loaded into the SPI host controller
  ///
  UINT8 PrefixOpcode[SPI_NUM_PREFIX_OPCODE];
  ///
  /// Determines Opcode Type, Menu and Frequency of the SPI commands
  ///
  SPI_COMMAND_CONFIG SpiCmdConfig[SPI_NUM_OPCODE];
  ///
  /// Special Opcode entry for the special operations.
  ///
  SPI_SPECIAL_OPCODE_ENTRY  *SpecialOpcodeEntry;
  ///
  /// The offset of the start of the BIOS image relative to the flash device.
  /// Please note this is a Flash Linear Address, NOT a memory space address.
  /// This value is platform specific and depends on the system flash map.
  /// This value is only used on non Descriptor mode.
  ///
  UINTN BiosStartOffset;
  ///
  /// The the BIOS Image size in flash. This value is platform specific
  /// and depends on the system flash map. Please note BIOS Image size may
  /// be smaller than BIOS Region size (in Descriptor Mode) or the flash size
  /// (in Non Descriptor Mode), and in this case, BIOS Image is supposed to be
  /// placed at the top end of the BIOS Region (in Descriptor Mode) or the flash
  /// (in Non Descriptor Mode)
  ///
  UINTN BiosSize;
} SPI_INIT_DATA;

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_INIT) (
  IN EFI_SPI_PROTOCOL     * This,
  IN SPI_INIT_DATA        * InitData
  );

/*++

Routine Description:

  Initializes the host controller to execute SPI commands.

Arguments:

  This                    Pointer to the EFI_SPI_PROTOCOL instance.
  InitData                Pointer to caller-allocated buffer containing the SPI
                          interface initialization table.

Returns:

  EFI_SUCCESS             Opcode initialization on the SPI host controller completed.
  EFI_ACCESS_DENIED       The SPI configuration interface is locked.
  EFI_OUT_OF_RESOURCES    Not enough resource available to initialize the device.
  EFI_DEVICE_ERROR        Device error, operation failed.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_LOCK) (
    IN EFI_SPI_PROTOCOL     * This
);

/*++

Routine Description:

  Lock the SPI Static Configuration Interface.
  Once locked, the interface is no longer open for configuration changes.
  The lock state automatically clears on next system reset.

Arguments:

  This      Pointer to the EFI_SPI_PROTOCOL instance.

Returns:

  EFI_SUCCESS             Lock operation succeed.
  EFI_DEVICE_ERROR        Device error, operation failed.
  EFI_ACCESS_DENIED       The interface has already been locked.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_EXECUTE) (
    IN     EFI_SPI_PROTOCOL   * This,
    IN     UINT8              OpcodeIndex,
    IN     UINT8              PrefixOpcodeIndex,
    IN     BOOLEAN            DataCycle,
    IN     BOOLEAN            Atomic,
    IN     BOOLEAN            ShiftOut,
    IN     UINTN              Address,
    IN     UINT32             DataByteCount,
    IN OUT UINT8              *Buffer,
    IN     SPI_REGION_TYPE    SpiRegionType
);

//
// Protocol member functions
//

/**
  JEDEC Read IDs from SPI flash part, this function will return 1-byte Vendor ID and 2-byte Device ID

  @param[in] This                 Pointer to the EFI_SPI_PROTOCOL instance.
  @param[in] Address              This value is to determine the command is sent to SPI Component 1 or 2
  @param[in, out] Buffer          Pointer to caller-allocated buffer containing the data received or sent during the SPI cycle.

  @retval EFI_SUCCESS             Read Jedec Id completed.
  @retval EFI_DEVICE_ERROR        Device error, operation failed.
  @exception EFI_UNSUPPORTED      This function is unsupported after SpiProtocolInit is called
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_READ_ID) (
  IN EFI_SPI_PROTOCOL     * This,
  IN     UINTN            Address,
  IN OUT UINT8            * Buffer
  );

///
/// EFI SPI Protocol definition
///
/// These protocols/PPI allows a platform module to perform SPI operations through the
/// Intel PCH SPI Host Controller Interface.
///
struct _EFI_SPI_PROTOCOL {
  EFI_SPI_READ_ID ReadId;   ///< JEDEC Read IDs from SPI flash part, this function will return 1-byte Vendor ID and 2-byte Device ID.
  EFI_SPI_INIT    Init;     ///< Initialize the host controller to execute SPI commands.
  EFI_SPI_EXECUTE Execute;  ///< Execute SPI commands from the host controller.
};

#endif
