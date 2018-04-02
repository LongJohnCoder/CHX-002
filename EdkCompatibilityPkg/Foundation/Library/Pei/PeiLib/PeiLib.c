/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiLib.c

Abstract:

  PEI Library Functions
 
--*/

#include "TianoCommon.h"
#include "PeiHob.h"
#include "Pei.h"
#include "PeiLib.h"
#include "EfiCommonLib.h"

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  );

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
/*++

Routine Description:

  Set Buffer to zero for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

Returns:

  None

--*/
{
  EfiCommonLibZeroMem (Buffer, Size);
}

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
  EfiCommonLibCopyMem (Destination, Source, Length);
}

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
 EfiCommonLibCopyMem (Destination, Source, Length);
}


BOOLEAN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
/*++

Routine Description:

  Compares two GUIDs

Arguments:

  Guid1 - guid to compare
  Guid2 - guid to compare

Returns:
  = TRUE  if Guid1 == Guid2
  = FALSE if Guid1 != Guid2 

--*/
{
  if ((((INT32 *) Guid1)[0] - ((INT32 *) Guid2)[0]) == 0) {
    if ((((INT32 *) Guid1)[1] - ((INT32 *) Guid2)[1]) == 0) {
      if ((((INT32 *) Guid1)[2] - ((INT32 *) Guid2)[2]) == 0) {
        if ((((INT32 *) Guid1)[3] - ((INT32 *) Guid2)[3]) == 0) {
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}


EFI_STATUS
EFIAPI 
PeiLibPciCfgModify (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_PCI_CFG_PPI          *PciCfg,
  IN PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                   Address,
  IN UINTN                    SetBits,
  IN UINTN                    ClearBits
  )
/*++

Routine Description:

  PCI read-modify-write operations.

  PIWG's PI specification replaces Inte's EFI Specification 1.10.
  EFI_PEI_PCI_CFG_PPI defined in Inte's EFI Specification 1.10 is replaced by
  EFI_PEI_PCI_CFG2_PPI in PI 1.0. "Modify" function  in these two PPI are not 
  compatibile with each other.
  

  For Framework code that make the following call:

      PciCfg->Modify (
                       PeiServices,
                       PciCfg,
                       Width,
                       Address,
                       SetBits,
                       ClearBits
                       );
   it will be updated to the following code which call this library API:
      PeiLibPciCfgModify (
          PeiServices,
          PciCfg,
          Width,
          Address,
          SetBits,
          ClearBits
          );

   The 

Arguments:
  
  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.
  PciCfg          A pointer to the this pointer of EFI_PEI_PCI_CFG_PPI. 
                          This parameter is unused as a place holder to make
                          the parameter list identical to PEI_PCI_CFG_PPI_RW.
  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  Address         The physical address of the access.

  SetBits         Points to value to bitwise-OR with the read configuration value.

                          The size of the value is determined by Width.

  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.


Returns:

  EFI_SUCCESS           The function completed successfully.

  EFI_DEVICE_ERROR      There was a problem with the transaction.

--*/
{
  EFI_STATUS            Status;
  EFI_PEI_PCI_CFG2_PPI  *PciCfg2;

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gPeiPciCfg2PpiGuid,
                             0,
                             NULL,
                             (VOID **) &PciCfg2
                             );
  ASSERT_PEI_ERROR (PeiServices, Status);

  Status = PciCfg2->Modify (
                      (CONST EFI_PEI_SERVICES **) PeiServices,
                      PciCfg2,
                      (EFI_PEI_PCI_CFG_PPI_WIDTH) Width,
                      Address,
                      &SetBits,
                      &ClearBits
                      );

  return Status;
}

