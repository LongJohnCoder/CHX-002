/** @file
  This module install ACPI Boot Graphics Resource Table (BGRT).

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/Acpi50.h>
#include <IndustryStandard/Bmp.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/BootLogo.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>


//
// Module globals.
//
EFI_EVENT  mBootGraphicsReadyToBootEvent;
UINTN      mBootGraphicsResourceTableKey = 0;

EFI_HANDLE                     mBootLogoHandle = NULL;
BOOLEAN                        mIsLogoValid = FALSE;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *mLogoBltBuffer = NULL;
UINTN                          mLogoDestX = 0;
UINTN                          mLogoDestY = 0;
UINTN                          mLogoWidth = 0;
UINTN                          mLogoHeight = 0;

BMP_IMAGE_HEADER  mBmpImageHeaderTemplate = {
  'B',    // CharB
  'M',    // CharM
  0,      // Size will be updated at runtime
  {0, 0}, // Reserved
  sizeof (BMP_IMAGE_HEADER), // ImageOffset
  sizeof (BMP_IMAGE_HEADER) - OFFSET_OF (BMP_IMAGE_HEADER, HeaderSize), // HeaderSize
  0,      // PixelWidth will be updated at runtime
  0,      // PixelHeight will be updated at runtime
  1,      // Planes
  24,     // BitPerPixel
  0,      // CompressionType
  0,      // ImageSize will be updated at runtime
  0,      // XPixelsPerMeter
  0,      // YPixelsPerMeter
  0,      // NumberOfColors
  0       // ImportantColors
};

BOOLEAN  mAcpiBgrtInstalled = FALSE;

EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE mBootGraphicsResourceTableTemplate = {
  {
    EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE),
    EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION,     // Revision
    0x00,  // Checksum will be updated at runtime
    //
    // It is expected that these values will be updated at EntryPoint.
    //    
    {0x00},     // OEM ID is a 6 bytes long field
    0x00,       // OEM Table ID(8 bytes long)
    0x00,       // OEM Revision
    0x00,       // Creator ID
    0x00,       // Creator Revision
  },
  EFI_ACPI_5_0_BGRT_VERSION,         // Version
  EFI_ACPI_5_0_BGRT_STATUS_VALID,    // Status
  EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP,  // Image Type
  0,                                 // Image Address
  0,                                 // Image Offset X
  0                                  // Image Offset Y
};

/**
  Update information of logo image drawn on screen.

  @param  This           The pointer to the Boot Logo protocol instance.
  @param  BltBuffer      The BLT buffer for logo drawn on screen. If BltBuffer
                         is set to NULL, it indicates that logo image is no
                         longer on the screen.
  @param  DestinationX   X coordinate of destination for the BltBuffer.
  @param  DestinationY   Y coordinate of destination for the BltBuffer.
  @param  Width          Width of rectangle in BltBuffer in pixels.
  @param  Height         Hight of rectangle in BltBuffer in pixels.

  @retval EFI_SUCCESS             The boot logo information was updated.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_OUT_OF_RESOURCES    The logo information was not updated due to
                                  insufficient memory resources.

**/
EFI_STATUS
EFIAPI
SetBootLogo (
  IN EFI_BOOT_LOGO_PROTOCOL            *This,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer       OPTIONAL,
  IN UINTN                             DestinationX,
  IN UINTN                             DestinationY,
  IN UINTN                             Width,
  IN UINTN                             Height
  );

EFI_BOOT_LOGO_PROTOCOL  mBootLogoProtocolTemplate = { SetBootLogo };

/**
  Update information of logo image drawn on screen.

  @param  This           The pointer to the Boot Logo protocol instance.
  @param  BltBuffer      The BLT buffer for logo drawn on screen. If BltBuffer
                         is set to NULL, it indicates that logo image is no
                         longer on the screen.
  @param  DestinationX   X coordinate of destination for the BltBuffer.
  @param  DestinationY   Y coordinate of destination for the BltBuffer.
  @param  Width          Width of rectangle in BltBuffer in pixels.
  @param  Height         Hight of rectangle in BltBuffer in pixels.

  @retval EFI_SUCCESS             The boot logo information was updated.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_OUT_OF_RESOURCES    The logo information was not updated due to
                                  insufficient memory resources.

**/
EFI_STATUS
EFIAPI
SetBootLogo (
  IN EFI_BOOT_LOGO_PROTOCOL            *This,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer       OPTIONAL,
  IN UINTN                             DestinationX,
  IN UINTN                             DestinationY,
  IN UINTN                             Width,
  IN UINTN                             Height
  )
{
  if (BltBuffer == NULL) {
    mIsLogoValid = FALSE;
    return EFI_SUCCESS;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (mLogoBltBuffer != NULL) {
    FreePool (mLogoBltBuffer);
  }

  mLogoBltBuffer = AllocateCopyPool (
                     Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
                     BltBuffer
                     );
  if (mLogoBltBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mLogoDestX = DestinationX;
  mLogoDestY = DestinationY;
  mLogoWidth = Width;
  mLogoHeight = Height;
  mIsLogoValid = TRUE;

  return EFI_SUCCESS;
}

/**
  This function calculates and updates an UINT8 checksum.

  @param[in]  Buffer          Pointer to buffer to checksum.
  @param[in]  Size            Number of bytes to checksum.

**/
VOID
BgrtAcpiTableChecksum (
  IN UINT8      *Buffer,
  IN UINTN      Size
  )
{
  UINTN ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first.
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value.
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/**
  Allocate EfiReservedMemoryType below 4G memory address.

  This function allocates EfiReservedMemoryType below 4G memory address.

  @param[in]  Size   Size of memory to allocate.

  @return Allocated address for output.

**/
VOID *
BgrtAllocateBsDataMemoryBelow4G (
  IN UINTN       Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Install Boot Graphics Resource Table to ACPI table.

  @return Status code.

**/
EFI_STATUS
InstallBootGraphicsResourceTable (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTableProtocol;
  UINT8                         *ImageBuffer;
  UINTN                         PaddingSize;
  UINTN                         BmpSize;
  UINT8                         *Image;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltPixel;
  UINTN                         Col;
  UINTN                         Row;

  //
  // Check whether Boot Graphics Resource Table is already installed.
  //
  if (mAcpiBgrtInstalled) {
    return EFI_SUCCESS;
  }

  //
  // Get ACPI Table protocol.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check whether Logo exist.
  //
  if (mLogoBltBuffer == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate memory for BMP file.
  //
  PaddingSize = mLogoWidth & 0x3;
  BmpSize = (mLogoWidth * 3 + PaddingSize) * mLogoHeight + sizeof (BMP_IMAGE_HEADER);
  ImageBuffer = BgrtAllocateBsDataMemoryBelow4G (BmpSize);
  if (ImageBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mBmpImageHeaderTemplate.Size = (UINT32) BmpSize;
  mBmpImageHeaderTemplate.ImageSize = (UINT32) BmpSize - sizeof (BMP_IMAGE_HEADER);
  mBmpImageHeaderTemplate.PixelWidth = (UINT32) mLogoWidth;
  mBmpImageHeaderTemplate.PixelHeight = (UINT32) mLogoHeight;
  CopyMem (ImageBuffer, &mBmpImageHeaderTemplate, sizeof (BMP_IMAGE_HEADER));

  //
  // Convert BLT buffer to BMP file.
  //
  Image = ImageBuffer + sizeof (BMP_IMAGE_HEADER);
  for (Row = 0; Row < mLogoHeight; Row++) {
    BltPixel = &mLogoBltBuffer[(mLogoHeight - Row - 1) * mLogoWidth];

    for (Col = 0; Col < mLogoWidth; Col++) {
      *Image++ = BltPixel->Blue;
      *Image++ = BltPixel->Green;
      *Image++ = BltPixel->Red;
      BltPixel++;
    }

    //
    // Padding for 4 byte alignment.
    //
    Image += PaddingSize;
  }
  FreePool (mLogoBltBuffer);
  mLogoBltBuffer = NULL;

  mBootGraphicsResourceTableTemplate.Status = (UINT8) (mIsLogoValid ? EFI_ACPI_5_0_BGRT_STATUS_VALID : EFI_ACPI_5_0_BGRT_STATUS_INVALID);
  mBootGraphicsResourceTableTemplate.ImageAddress = (UINT64) (UINTN) ImageBuffer;
  mBootGraphicsResourceTableTemplate.ImageOffsetX = (UINT32) mLogoDestX;
  mBootGraphicsResourceTableTemplate.ImageOffsetY = (UINT32) mLogoDestY;

  //
  // Update Checksum.
  //
  BgrtAcpiTableChecksum ((UINT8 *) &mBootGraphicsResourceTableTemplate, sizeof (EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE));

  //
  // Publish Boot Graphics Resource Table.
  //
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                &mBootGraphicsResourceTableTemplate,
                                sizeof (EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE),
                                &mBootGraphicsResourceTableKey
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mAcpiBgrtInstalled = TRUE;
  return Status;
}

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  install the Boot Graphics Resource Table.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
BgrtReadyToBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  InstallBootGraphicsResourceTable ();
}

/**
  The module Entry Point of the Boot Graphics Resource Table DXE driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BootGraphicsDxeEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      OemTableId;

  CopyMem (
    mBootGraphicsResourceTableTemplate.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mBootGraphicsResourceTableTemplate.Header.OemId)
    );
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mBootGraphicsResourceTableTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mBootGraphicsResourceTableTemplate.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mBootGraphicsResourceTableTemplate.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mBootGraphicsResourceTableTemplate.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);


  //
  // Install Boot Logo protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBootLogoHandle,
                  &gEfiBootLogoProtocolGuid,
                  &mBootLogoProtocolTemplate,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notify function to install BGRT on ReadyToBoot Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  BgrtReadyToBootEventNotify,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mBootGraphicsReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
