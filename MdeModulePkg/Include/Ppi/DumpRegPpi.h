/** @file
  Ppi for Ipmi of SMS.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DUMP_REG_PPI_H_
#define _DUMP_REG_PPI_H_

typedef struct _PEI_DUMP_REG_PPI PEI_DUMP_REG_PPI;


#define EFI_DUMP_REG_PPI_GUID \
  { \
	0x280516E8, 0x9188, 0x4571, 0x8F, 0xB3, 0xA, 0xA7, 0x4B, 0x49, 0x36, 0x49 \
  }

// for register dump in PEI phases

typedef
VOID
(EFIAPI *PEI_DUMP_NB_REG) (
  IN     PEI_DUMP_REG_PPI  *This,
  IN     BOOLEAN           DefaultValue,  
  IN BOOLEAN			  IsBit_Layout
  );

typedef
VOID
(EFIAPI *PEI_DUMP_SB_REG) (
  IN     PEI_DUMP_REG_PPI  *This,
  IN     BOOLEAN           DefaultValue,  
  IN BOOLEAN			  IsBit_Layout
  );


typedef
VOID
(EFIAPI *PEI_DUMP_S3_REG) (
  IN     PEI_DUMP_REG_PPI  *This,  
  IN BOOLEAN			  IsBit_Layout
  );

//
// DUMP REG PPI
//
struct _PEI_DUMP_REG_PPI {
  PEI_DUMP_NB_REG       DumpNBValue;
  PEI_DUMP_SB_REG       DumpSBValue;
  PEI_DUMP_S3_REG       DumpS3Reg;
};

extern EFI_GUID gEfiPeiDumpRegPpiGuid;

#endif
