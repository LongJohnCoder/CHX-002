
#include "AcpiPlatform.h"

#define EFI_ACPI_SYSTEM_LOCALITIES_ENTRY_COUNT              4
#define EFI_ACPI_NUMBER_OF_SYSTEM_LOCALITIES                2


#pragma pack(1)

typedef struct {
  EFI_ACPI_5_0_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER  Hdr;
  UINT8                 Entry[EFI_ACPI_SYSTEM_LOCALITIES_ENTRY_COUNT];
} EFI_ACPI_TABLE_SLIT;

#pragma pack()


EFI_ACPI_TABLE_SLIT gSlit = {
  {
    {
      EFI_ACPI_5_0_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE,
      sizeof(EFI_ACPI_TABLE_SLIT),
      EFI_ACPI_5_0_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION,
      0,
      {0},
      0,
      0,
      0,
      0
    },
    EFI_ACPI_NUMBER_OF_SYSTEM_LOCALITIES,
  },
  {10, 21, 21, 10},
};

VOID
BuildAcpiSlitTable (
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
	)
{
  EFI_STATUS           Status;
  UINTN                TableKey;  
  
  
  PlatformUpdateTables((EFI_ACPI_COMMON_HEADER*)&gSlit);  
  AcpiTableUpdateChksum(&gSlit); 

  Status = AcpiTable->InstallAcpiTable(
                        AcpiTable,
                        &gSlit,
                        sizeof(gSlit),
                        &TableKey
                        );  
  ASSERT(!EFI_ERROR(Status));  
}

