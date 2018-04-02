

#include "AcpiPlatform.h"

#define SRAT_LAPIC_AFFINITY_COUNT       8
#define SRAT_MEM_AFFINITY_COUNT         2
#define SRAT_Lx2APIC_AFFINITY_COUNT     0

#pragma pack(1)

typedef struct {
  EFI_ACPI_5_0_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER          Hdr;
  
#if SRAT_LAPIC_AFFINITY_COUNT > 0  
  EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE  Apic[SRAT_LAPIC_AFFINITY_COUNT];
#endif

#if SRAT_MEM_AFFINITY_COUNT > 0  
  EFI_ACPI_5_0_MEMORY_AFFINITY_STRUCTURE                      Mem[SRAT_MEM_AFFINITY_COUNT];
#endif
  
#if SRAT_Lx2APIC_AFFINITY_COUNT > 0  
  EFI_ACPI_5_0_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE      x2Apic[SRAT_Lx2APIC_AFFINITY_COUNT];
#endif  

} EFI_ACPI_TABLE_SRAT;

#pragma pack()



EFI_ACPI_TABLE_SRAT  gAcpiTblSrat;


VOID
BuildAcpiSratTable (
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
	)
{
  EFI_ACPI_TABLE_SRAT  *Srat = &gAcpiTblSrat;
  EFI_STATUS           Status;
  UINTN                TableKey;
  CPU_APIC_ID_INFO     *CpuApicIdTable = NULL;
  UINTN                CpuCount;
  UINTN                Index;
  UINT8                Domain;
  UINT64               MemLength;
  UINT64               MemBase;
           

  ZeroMem(Srat, sizeof(EFI_ACPI_TABLE_SRAT));
  Srat->Hdr.Header.Signature = EFI_ACPI_5_0_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE;
  Srat->Hdr.Header.Length    = sizeof(EFI_ACPI_TABLE_SRAT);
  Srat->Hdr.Header.Revision  = EFI_ACPI_5_0_SYSTEM_RESOURCE_AFFINITY_TABLE_REVISION;
  Srat->Hdr.Reserved1        = 1;

  GetCpuLocalApicInfo(&CpuApicIdTable, &CpuCount);
  ASSERT(CpuCount <= SRAT_LAPIC_AFFINITY_COUNT);

// assmue one CPU socket has 4 cores.
  if(CpuCount <= 4){        
    goto ProcExit;
  }

#if SRAT_LAPIC_AFFINITY_COUNT > 0    
  Domain = 0;
  for(Index=0;Index<SRAT_LAPIC_AFFINITY_COUNT;Index++){
    Srat->Apic[Index].Type   = EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY;
    Srat->Apic[Index].Length = sizeof(EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE);
    if(Index<CpuCount){
      Srat->Apic[Index].ProximityDomain7To0 = Domain;
      Srat->Apic[Index].ApicId = CpuApicIdTable[Index].ApicId;
      Srat->Apic[Index].Flags  = EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_SAPIC_ENABLED;
      if(((Index+1)%4) == 0){
        Domain++;
      }
    } else {
      Srat->Apic[Index].ApicId = 0xFF;
    }
  }
#endif  

#if SRAT_MEM_AFFINITY_COUNT > 0
  ASSERT(SRAT_MEM_AFFINITY_COUNT >= CpuCount/4);
  MemLength = gMemInfo->PhyMemSize/(CpuCount/4);
  MemBase   = 0;
  for(Index=0;Index<SRAT_MEM_AFFINITY_COUNT;Index++){
    Srat->Mem[Index].Type = EFI_ACPI_5_0_MEMORY_AFFINITY;
    Srat->Mem[Index].Length = sizeof(EFI_ACPI_5_0_MEMORY_AFFINITY_STRUCTURE);
    if(Index<CpuCount/4){
      Srat->Mem[Index].ProximityDomain = (UINT32)Index;
      Srat->Mem[Index].AddressBaseLow  = (UINT32)MemBase;
      Srat->Mem[Index].AddressBaseHigh = (UINT32)RShiftU64(MemBase, 32);
      Srat->Mem[Index].LengthLow  = (UINT32)MemLength;
      Srat->Mem[Index].LengthHigh = (UINT32)RShiftU64(MemLength, 32);
      Srat->Mem[Index].Flags      = EFI_ACPI_5_0_MEMORY_ENABLED;
      MemBase += MemLength;
    }
  }
#endif

  PlatformUpdateTables((EFI_ACPI_COMMON_HEADER*)Srat);  
  AcpiTableUpdateChksum(Srat); 

  Status = AcpiTable->InstallAcpiTable(
                        AcpiTable,
                        Srat,
                        sizeof(EFI_ACPI_TABLE_SRAT),
                        &TableKey
                        );  
  ASSERT(!EFI_ERROR(Status));

ProcExit:
  if(CpuApicIdTable != NULL){
    FreePool(CpuApicIdTable);
  }
}  




