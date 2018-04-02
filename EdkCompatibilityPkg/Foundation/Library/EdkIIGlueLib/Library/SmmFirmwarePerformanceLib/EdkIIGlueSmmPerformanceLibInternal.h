/*++

Copyright (c) 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  EdkIIGlueSmmPerformanceLibInternal.h

Abstract: 

  Internal Header file of SMM Library for FPDT performance logging

--*/

#ifndef __SMM_FIRMWARE_PERFORMANCE_LIB_INTERNAL__H
#define __SMM_FIRMWARE_PERFORMANCE_LIB_INTERNAL__H

#include "EdkIIGlueDxe.h"
#include "EdkIIGlueUefiLib.h"
#include EFI_PROTOCOL_DEFINITION (Fpdt)

#define SMM_MODULE_TOK                L"SmmModule"
#define SMM_FUNCTION_TOK              L"SmmFunction"

#define RMPT_SIG                      0x54504D52
#define RUNTIME_MODULE_TABLE_PTR_TYPE 0x3
#define RUNTIME_MODULE_REC_TYPE       0x20
#define RUNTIME_FUNCTION_REC_TYPE     0x21

#define RECORD_REVISION_1             0x1
#define RECORD_REVISION_2             0x2

#pragma pack(push, 1)
typedef struct _RUNTIME_PERF_TABLE_HEADER {
  UINT32    Signature;
  UINT32    Length;
  EFI_GUID  Guid;
} RUNTIME_PERF_TABLE_HEADER;

typedef struct _RUNTIME_MODULE_PERF_RECORD {
  UINT16  RuntimeRecType;
  UINT8   Reclength;
  UINT8   Revision;
  UINT32  ModuleCallCount;
  UINT64  ModuleResidency;
} RUNTIME_MODULE_PERF_RECORD;

typedef struct _RUNTIME_FUNCTION_PERF_RECORD {
  UINT16  RuntimeRecType;
  UINT8   Reclength;
  UINT8   Revision;
  UINT32  Reserved;
  UINT32  FunctionId;
  UINT32  FunctionCallCount;
  UINT64  FunctionResidency;
} RUNTIME_FUNCTION_PERF_RECORD;
#pragma pack(pop)

#endif // __SMM_FIRMWARE_PERFORMANCE_LIB_INTERNAL__H