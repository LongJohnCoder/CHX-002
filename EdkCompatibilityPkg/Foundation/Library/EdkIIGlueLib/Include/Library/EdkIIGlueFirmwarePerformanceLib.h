/*++

Copyright (c) 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdkIIGluePerformanceLib.h

Abstract: 

  Public header file for Firmware Performance Lib

  
--*/

#ifndef __EDKII_GLUE_FIRMWARE_PERFORMANCE_LIB_H_
#define __EDKII_GLUE_FIRMWARE_PERFORMANCE_LIB_H_

EFI_STATUS
StartMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  Start measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to measure
  Token      - Token to measure
  Module     - Module to measure
  Timestamp  - Ticker as start tick

Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
;

EFI_STATUS
EndMeasure (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp
  )
/*++

Routine Description:

  End measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick

Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
;

EFI_STATUS
StartMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  Start extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record
Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
;

EFI_STATUS
EndMeasureEx (
  IN VOID   *Handle, 
  IN UINT16 *Token,  
  IN UINT16 *Module, 
  IN UINT64 TimeStamp,
  IN UINT16 Identifier
  )
/*++

Routine Description:

  End extended measurement according to token field and insert into pre-allocated buffer

Arguments:

  Handle     - Handle to stop
  Token      - Token to stop
  Module     - Module to stop
  Timestamp  - Ticker as end tick
  Identifier - Identifier for a given record

Returns:

  EFI_SUCCESS - Located protocol successfully, and buffer is updated with new record
  EFI_NOT_FOUND - Failure in update

--*/
;

#ifdef FIRMWARE_PERFORMANCE
#define PERF_START(Handle, Token, Module, TimeStamp)                 StartMeasure (Handle, Token, Module, TimeStamp)
#define PERF_END(Handle, Token, Module, TimeStamp)                   EndMeasure (Handle, Token, Module, TimeStamp)
#define PERF_START_EX(Handle, Token, Module, TimeStamp, Identifier)  StartMeasureEx (Handle, Token, Module, TimeStamp, Identifier)
#define PERF_END_EX(Handle, Token, Module, TimeStamp, Identifier)    EndMeasureEx (Handle, Token, Module, TimeStamp, Identifier)
#define PERF_CODE(code) code
#else
#define PERF_START(Handle, Token, Module, TimeStamp)
#define PERF_END(Handle, Token, Module, TimeStamp)
#define PERF_START_EX(Handle, Token, Module, TimeStamp, Identifier)
#define PERF_END_EX(Handle, Token, Module, TimeStamp, Identifier)
#define PERF_CODE(code)
#endif

#endif