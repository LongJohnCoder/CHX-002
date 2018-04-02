/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 EfiPerf.h

Abstract:
 EfiPerf.h provides performance primitive for the DXE and Shell phase

 
--*/

#ifndef _EFI_PERF_H_
#define _EFI_PERF_H_

#include EFI_PROTOCOL_DEFINITION (FirmwarePerformance)

EFI_STATUS
InitializePerformanceInfrastructure (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable,
  IN UINT64             Ticker
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description
  SystemTable - TODO: add argument description
  Ticker      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EndMeasure (
  IN EFI_HANDLE       Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Handle  - TODO: add argument description
  Token   - TODO: add argument description
  Host    - TODO: add argument description
  Ticker  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartMeasure (
  IN EFI_HANDLE       Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Handle  - TODO: add argument description
  Token   - TODO: add argument description
  Host    - TODO: add argument description
  Ticker  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartMeasureEx (
  IN EFI_HANDLE       Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker,
  IN UINT16           Identifier
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Handle  - TODO: add argument description
  Token   - TODO: add argument description
  Host    - TODO: add argument description
  Ticker  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EndMeasureEx (
  IN EFI_HANDLE       Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker,
  IN UINT16           Identifier
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Handle  - TODO: add argument description
  Token   - TODO: add argument description
  Host    - TODO: add argument description
  Ticker  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
UpdateMeasure (
  IN EFI_HANDLE      Handle,
  IN UINT16          *Token,
  IN UINT16          *Host,
  IN EFI_HANDLE      HandleNew,
  IN UINT16          *TokenNew,
  IN UINT16          *HostNew
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Handle    - TODO: add argument description
  Token     - TODO: add argument description
  Host      - TODO: add argument description
  HandleNew - TODO: add argument description
  TokenNew  - TODO: add argument description
  HostNew   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#ifdef FIRMWARE_PERFORMANCE
#define PERF_ENABLE(handle, table, ticker)      InitializePerformanceInfrastructure (handle, table, ticker)
#define PERF_START(handle, token, host, ticker) StartMeasure (handle, token, host, ticker)
#define PERF_END(handle, token, host, ticker) EndMeasure (handle, token, host, ticker)
#define PERF_START_EX(handle, token, host, ticker, identifier)   StartMeasureEx (handle, token, host, ticker, identifier)
#define PERF_END_EX(handle, token, host, ticker, identifier)   EndMeasureEx (handle, token, host, ticker, identifier)
#define PERF_UPDATE(handle, token, host, handlenew, tokennew, hostnew)
#define PERF_CODE(code) code
#else
#define PERF_ENABLE(handle, table, ticker)
#define PERF_START(handle, token, host, ticker)
#define PERF_END(handle, token, host, ticker)
#define PERF_START_EX(handle, token, host, ticker, identifier)
#define PERF_END_EX(handle, token, host, ticker, identifier)
#define PERF_UPDATE(handle, token, host, handlenew, tokennew, hostnew)
#define PERF_CODE(code)
#endif

#endif