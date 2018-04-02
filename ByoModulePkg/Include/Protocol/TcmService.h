/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmService.h

Abstract: 
  Tcm protocol define.

Revision History:

Bug 3144 - Add Tcm Measurement Architecture.
TIME: 2011-11-24
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. PEI: Measure CRTM Version.
          Measure Main Bios.
  2. DXE: add 'TCPA' acpi table.
          add event log.
          Measure Handoff Tables.
          Measure All Boot Variables.
          Measure Action.
  Note: As software of SM3's hash has not been implemented, so hash
        function is invalid.
$END------------------------------------------------------------

Bug 3075 - Add TCM support.
TIME: 2011-11-14
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Tcm module init version.
     Only support setup function.
$END------------------------------------------------------------

**/


/** @file
  TCG Service Protocol as defined in TCG_EFI_Protocol_1_20_Final
  See http://trustedcomputinggroup.org for the latest specification

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCM_SERVICE_PROTOCOL_H__
#define __TCM_SERVICE_PROTOCOL_H__

#include <IndustryStandard/TcmPlatForm.h>


#define EFI_TCM_PROTOCOL_GUID  \
  {0xf51d6c88, 0x60d4, 0x44be, {0x84, 0xdf, 0x50, 0xbc, 0xe7, 0x9b, 0x5a, 0xa5}}

typedef struct _EFI_TCM_PROTOCOL EFI_TCM_PROTOCOL;

typedef struct {
  UINT8          Size;                /// Size of this structure.
  TCM_VERSION    StructureVersion;    
  TCM_VERSION    ProtocolSpecVersion;
  UINT8          HashAlgorithmBitmap; /// Hash algorithms . 
  BOOLEAN        TcmPresentFlag;      /// 00h = TPM not present.
  BOOLEAN        TcmDeactivatedFlag;  /// 01h = TPM currently deactivated.
} TCM_EFI_BOOT_SERVICE_CAPABILITY;



/**
  This service provides EFI protocol capability information, state information 
  about the TCM, and Event Log state information.

  @param  This                   Indicates the calling context
  @param  ProtocolCapability     The callee allocates memory for a TCG_BOOT_SERVICE_CAPABILITY 
                                 structure and fills in the fields with the EFI protocol 
                                 capability information and the current TPM state information.
  @param  TcmFeatureFlags        This is a pointer to the feature flags. No feature 
                                 flags are currently defined so this parameter 
                                 MUST be set to 0. However, in the future, 
                                 feature flags may be defined that, for example, 
                                 enable hash algorithm agility.
  @param  EventLogLocation       This is a pointer to the address of the event log in memory.
  @param  EventLogLastEntry      If the Event Log contains more than one entry, 
                                 this is a pointer to the address of the start of 
                                 the last entry in the event log in memory. 

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  ProtocolCapability does not match TCG capability.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCM_STATUS_CHECK)(
  IN      EFI_TCM_PROTOCOL                *This,
  OUT     TCM_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
  OUT     UINT32                          *TcmFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLastEntry
  );




/**
  This service abstracts the capability to do a hash operation on a data buffer.
  
  @param  This                   Indicates the calling context.
  @param  HashData               The pointer to the data buffer to be hashed.
  @param  HashDataLen            The length of the data buffer to be hashed.
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation.
  @param  HashedDataLen          Resultant length of the hashed data.
  @param  HashedDataResult       Resultant buffer of the hashed data.
  
  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  HashDataLen is NULL.
  @retval EFI_INVALID_PARAMETER  HashDataLenResult is NULL.
  @retval EFI_OUT_OF_RESOURCES   Cannot allocate buffer of size *HashedDataLen.
  @retval EFI_UNSUPPORTED        AlgorithmId not supported.
  @retval EFI_BUFFER_TOO_SMALL   *HashedDataLen < sizeof (TCG_DIGEST).
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCM_HASH_ALL)(
  IN      EFI_TCM_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  UINT64                    *HashedDataLen,
  IN OUT  UINT8                     **HashedDataResult
  );




/**
  This service abstracts the capability to add an entry to the Event Log.

  @param  This                   Indicates the calling context
  @param  TCGLogData             The pointer to the start of the data buffer containing 
                                 the TCG_PCR_EVENT data structure. All fields in 
                                 this structure are properly filled by the caller.
  @param  EventNumber            The event number of the event just logged.
  @param  Flags                  Indicates additional flags. Only one flag has been 
                                 defined at this time, which is 0x01 and means the 
                                 extend operation should not be performed. All 
                                 other bits are reserved. 
 
  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory in the event log to complete this action.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCM_LOG_EVENT)(
  IN      EFI_TCM_PROTOCOL          *This,
  IN      TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  );




/**
  This service is a proxy for commands to the TCM.

  @param  This                        Indicates the calling context.
  @param  TcmInputParameterBlockSize  Size of the TCM input parameter block.
  @param  TcmInputParameterBlock      The pointer to the TCM input parameter block.
  @param  TcmOutputParameterBlockSize Size of the TPM output parameter block.
  @param  TcmOutputParameterBlock     The pointer to the TCM output parameter block.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Invalid ordinal.
  @retval EFI_UNSUPPORTED        Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval EFI_TIMEOUT            The TIS timed-out.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCM_PASS_THROUGH_TO_TCM)(
  IN      EFI_TCM_PROTOCOL          *This,
  IN      UINT32                    TcmInputParameterBlockSize,
  IN      UINT8                     *TcmInputParameterBlock,
  IN      UINT32                    TcmOutputParameterBlockSize,
  IN      UINT8                     *TcmOutputParameterBlock
  );





/**
  This service abstracts the capability to do a hash operation on a data buffer, 
  extend a specific TCM PCR with the hash result, and add an entry to the Event Log

  @param  This                   Indicates the calling context
  @param  HashData               The physical address of the start of the data buffer 
                                 to be hashed, extended, and logged.
  @param  HashDataLen            The length, in bytes, of the buffer referenced by HashData
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  TCMLogData             The physical address of the start of the data 
                                 buffer containing the TCM_PCR_EVENT data structure.
  @param  EventNumber            The event number of the event just logged.
  @param  EventLogLastEntry      The physical address of the first byte of the entry 
                                 just placed in the Event Log. If the Event Log was 
                                 empty when this function was called then this physical 
                                 address will be the same as the physical address of 
                                 the start of the Event Log.
  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_UNSUPPORTED        AlgorithmId != TCM_ALG_SM3.
  @retval EFI_UNSUPPORTED        Current TPL >= EFI_TPL_CALLBACK.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCM_HASH_LOG_EXTEND_EVENT)(
  IN      EFI_TCM_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  );



///
/// The EFI_TCM Protocol abstracts TCM activity.
///
struct _EFI_TCM_PROTOCOL {
  EFI_TCM_STATUS_CHECK              StatusCheck;
  EFI_TCM_HASH_ALL                  HashAll;
  EFI_TCM_LOG_EVENT                 LogEvent;
  EFI_TCM_PASS_THROUGH_TO_TCM       PassThroughToTcm;
  EFI_TCM_HASH_LOG_EXTEND_EVENT     HashLogExtendEvent;
};

extern EFI_GUID gEfiTcmProtocolGuid;

#endif
