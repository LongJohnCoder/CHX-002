/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcmDxe.h

Abstract: 
  Pei part of TCM Module.

Revision History:

Bug 3223 - package ZTE SM3 hash source to .efi for ZTE's copyrights.
TIME: 2011-12-16
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. use ppi or protocol to let hash be independent.
$END------------------------------------------------------------

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



#ifndef  __TCM_DXE_H__
#define  __TCM_DXE_H__
//--------------------------------------------------------------
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/SmBios.h>
#include <Guid/SmBios.h>
#include <Guid/GlobalVariable.h>
#include <Guid/TcmEventHob.h>
#include <Guid/EventGroup.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Protocol/TcmService.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/TcmHashSm3.h>
#include "../DxeCommLib/TcmDxeLib.h"
#include "TcmSetupDataStruc.h"
#include <Protocol/SetupSaveNotify.h>



//--------------------------------------------------------------
extern unsigned char TcmSetupBin[];        // .vfr

#define TCM_DXE_CTX_SIGNATURE    SIGNATURE_32('T', 'C', 'M', 'D')

#define TCM_DXE_DATA_FROM_THIS_PROTOCOL(this)    CR(this, TCM_DXE_PRIVATE_DATA, TcmProtocol,     TCM_DXE_CTX_SIGNATURE)
#define TCM_DXE_DATA_FROM_THIS_HII_CONFIG(this)  CR(this, TCM_DXE_PRIVATE_DATA, ConfigAccess,    TCM_DXE_CTX_SIGNATURE)
#define TCM_DXE_DATA_FROM_THIS_SETUP_SAVE(this)  CR(this, TCM_DXE_PRIVATE_DATA, SetupSaveNotfiy, TCM_DXE_CTX_SIGNATURE)


#define EFI_TCM_LOG_AREA_SIZE   0x10000

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH           VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL     End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER       Header;
  UINT16                            PlatformClass;
  UINT32                            Laml;             // Log Area Minimum Length (LAML), 64KB
  EFI_PHYSICAL_ADDRESS              Lasa;             // Log Area Start Address (LASA)
} EFI_TCM_ACPI_TABLE;
#pragma pack()

typedef struct{
  UINTN                             Signature;
  EFI_TCM_PROTOCOL                  TcmProtocol;
  TCM_EFI_BOOT_SERVICE_CAPABILITY   BsCap;
  EFI_TCM_CLIENT_ACPI_TABLE         *TcmAcpiTable;
  UINTN                             CurEventLogSize;
  EFI_PHYSICAL_ADDRESS              EventLogLastEntry;
  EFI_TCM_HANDLE                    TcmHandle;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  SETUP_SAVE_NOTIFY_PROTOCOL        SetupSaveNotfiy;
  TCM_SETUP_CONFIG                  ConfigData;
} TCM_DXE_PRIVATE_DATA;












/**
  This service provides EFI protocol capability information, state information 
  about the TPM, and Event Log state information.

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

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  ProtocolCapability does not match TCG capability.
**/
EFI_STATUS
EFIAPI
TcmDxeStatusCheck (
  IN      EFI_TCM_PROTOCOL                *This,
  OUT     TCM_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
  OUT     UINT32                          *TcmFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS            *EventLogLastEntry
  );




/**
  This service abstracts the capability to do a hash operation on a data buffer.
  
  @param  This                   Indicates the calling context
  @param  HashData               Pointer to the data buffer to be hashed
  @param  HashDataLen            Length of the data buffer to be hashed
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  HashedDataLen          Resultant length of the hashed data
  @param  HashedDataResult       Resultant buffer of the hashed data  
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  HashDataLen is NULL.
  @retval EFI_INVALID_PARAMETER  HashDataLenResult is NULL.
  @retval EFI_OUT_OF_RESOURCES   Cannot allocate buffer of size *HashedDataLen.
  @retval EFI_UNSUPPORTED        AlgorithmId not supported.
  @retval EFI_BUFFER_TOO_SMALL   *HashedDataLen < sizeof (TCG_DIGEST).
**/
EFI_STATUS
EFIAPI
TcmDxeHashAll (
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
  @param  TCGLogData             Pointer to the start of the data buffer containing 
                                 the TCM_PCR_EVENT data structure. All fields in 
                                 this structure are properly filled by the caller.
  @param  EventNumber            The event number of the event just logged
  @param  Flags                  Indicate additional flags. Only one flag has been 
                                 defined at this time, which is 0x01 and means the 
                                 extend operation should not be performed. All 
                                 other bits are reserved. 
 
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory in the event log to complete this action.
**/
EFI_STATUS
EFIAPI
TcmDxeLogEvent (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  );



/**
  This service is a proxy for commands to the TCM.

  @param  This                        Indicates the calling context
  @param  TpmInputParameterBlockSize  Size of the TPM input parameter block
  @param  TpmInputParameterBlock      Pointer to the TPM input parameter block
  @param  TpmOutputParameterBlockSize Size of the TPM output parameter block
  @param  TpmOutputParameterBlock     Pointer to the TPM output parameter block

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Invalid ordinal.
  @retval EFI_UNSUPPORTED        Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval EFI_TIMEOUT            The TCM timed-out.
**/
EFI_STATUS
EFIAPI
TcmDxePassThroughToTcm (
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
  @param  HashData               Physical address of the start of the data buffer 
                                 to be hashed, extended, and logged.
  @param  HashDataLen            The length, in bytes, of the buffer referenced by HashData
  @param  AlgorithmId            Identification of the Algorithm to use for the hashing operation
  @param  TCGLogData             The physical address of the start of the data 
                                 buffer containing the TCM_PCR_EVENT data structure.
  @param  EventNumber            The event number of the event just logged.
  @param  EventLogLastEntry      Physical address of the first byte of the entry 
                                 just placed in the Event Log. If the Event Log was 
                                 empty when this function was called then this physical 
                                 address will be the same as the physical address of 
                                 the start of the Event Log.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_UNSUPPORTED        AlgorithmId != TPM_ALG_SHA.
  @retval EFI_UNSUPPORTED        Current TPL >= EFI_TPL_CALLBACK.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
TcmDxeHashLogExtendEvent (
  IN      EFI_TCM_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TCM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCM_PCR_EVENT             *TCMLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  );








/**
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in] This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request    A null-terminated Unicode string in
                        <ConfigRequest> format. Note that this
                        includes the routing information as well as
                        the configurable name / value pairs. It is
                        invalid for this string to be in
                        <MultiConfigRequest> format.
  @param[out] Progress  On return, points to a character in the
                        Request string. Points to the string's null
                        terminator if request was successful. Points
                        to the most recent "&" before the first
                        failing name / value pair (or the beginning
                        of the string if the failure is in the first
                        name / value pair) if the request was not
                        successful.
  @param[out] Results   A null-terminated Unicode string in
                        <ConfigAltResp> format which has all values
                        filled in for the names in the Request string.
                        String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL. 
  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.Currently not implemented.
**/
EFI_STATUS
EFIAPI
TcmFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );









/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.   
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginn ing of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.  
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
TcmFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );





/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to 
                                 vary based on the opcode that enerated the callback.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]  ActionRequest     On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.Currently not implemented.
  @retval EFI_INVALID_PARAMETERS Passing in wrong parameter. 
  @retval Others                 Other errors as indicated. 
**/
EFI_STATUS
EFIAPI
TcmFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );




EFI_STATUS 
TcmNvSaveValue(  
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

EFI_STATUS 
TcmNvDiscardValue(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

EFI_STATUS 
TcmNvLoadDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

EFI_STATUS 
TcmNvSaveUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

EFI_STATUS 
TcmNvLoadUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  );

EFI_STATUS 
TcmIsNvDataChanged(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This,
  BOOLEAN                       *IsDataChanged
  );



//--------------------------------------------------------------
#endif

