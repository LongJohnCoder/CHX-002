/** @file
  HII Config Access protocol implementation of SecureBoot configuration module.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
//-#include <Library/PlatformSecureLib.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/FirmwareVolume2.h>
#include "SecureBootConfigNvData.h"

extern  UINT8  SecureBootConfigBin[];
extern  UINT8  SecureBootConfigDxeStrings[];
STATIC BOOLEAN IsAnyKeyPresent();
EFI_STATUS LibAuthVarSetPhysicalPresent(BOOLEAN Present);

#define ARRAY_ELEM_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define _FREE_NON_NULL(POINTER) \
  do{ \
    if((POINTER) != NULL) { \
      FreePool((POINTER)); \
      (POINTER) = NULL; \
    } \
  } while(FALSE)
  

typedef struct {
  VENDOR_DEVICE_PATH                VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINTN                             Signature;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;
} SECUREBOOT_CONFIG_PRIVATE_DATA;

#define SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE     SIGNATURE_32 ('S', 'E', 'C', 'B')
#define SECUREBOOT_CONFIG_PRIVATE_FROM_THIS(a)       CR(a, SECUREBOOT_CONFIG_PRIVATE_DATA, ConfigAccess, SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE)



CHAR16  mSecureBootStorageName[] = L"SECUREBOOT_CONFIGURATION";

HII_VENDOR_DEVICE_PATH  mSecureBootHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    SECUREBOOT_CONFIG_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};




#define BLT_BACKGROUND_WHITE    EFI_BACKGROUND_LIGHTGRAY

UINTN
UefiLibGetStringWidth (
  IN  CHAR16               *String,
  IN  BOOLEAN              LimitLen,
  IN  UINTN                MaxWidth,
  OUT UINTN                *Offset
  );

// Return Choice Index
//   left : 0
//   rigth: 1
//   ESC  : 0xFF
//
// Color:
//   0 - B:White, F:Blue		// default
//   1 - B:White, F:Red
UINTN
EFIAPI
DrawConfirmDialog (
  IN  UINTN          DialogColorIndex, 
  IN  CHAR16         **ChoiceStr,
  IN  CHAR16         *Title,
  ...
  )
{
  VA_LIST                          Args;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SavedConsoleMode;
  UINTN                            Columns;
  UINTN                            Rows;
  UINTN                            Column;
  UINTN                            Row;
  UINTN                            NumberOfLines;
  UINTN                            MaxLength;
  CHAR16                           *String;
  UINTN                            Length;
  CHAR16                           *Line;
  CHAR16                           *TmpString;
  CHAR16                           *CStr[2];
  UINTN                            Choice1X=0, Choice2X=0, ChoiceY=0;
  UINTN                            EventIndex;
  EFI_INPUT_KEY                    Key;  
  UINTN                            ChoiceIndex;
  UINTN                            AttribComm, AttribFocus;
  UINTN                            CStr0Len, CStr1Len;
  

//DEBUG((EFI_D_ERROR, "%a()\n", __FUNCTION__));
  
  CStr[0]  = ChoiceStr[0];
  CStr[1]  = ChoiceStr[1];
  
  CStr0Len = StrLen(CStr[0]);    
  CStr1Len = StrLen(CStr[1]);
  ASSERT(CStr0Len < 20 && CStr0Len);
  ASSERT(CStr1Len < 20);     
  
  AttribComm = EFI_LIGHTBLUE|BLT_BACKGROUND_WHITE;
  if(DialogColorIndex == 1){
    AttribComm = EFI_RED|BLT_BACKGROUND_WHITE;
  }
  AttribFocus = EFI_WHITE|EFI_BACKGROUND_BLACK;
  
  VA_START(Args, Title);
  MaxLength = 0;
  NumberOfLines = 0;
  while((String = VA_ARG (Args, CHAR16*)) != NULL) {
    MaxLength = MAX(MaxLength, UefiLibGetStringWidth(String, FALSE, 0, NULL) / 2);
    NumberOfLines++;
  }
  VA_END(Args);
  ASSERT(NumberOfLines != 0);
  ASSERT(MaxLength != 0);
  if(MaxLength < 40){MaxLength = 40;}

  ConOut = gST->ConOut;
  CopyMem(&SavedConsoleMode, ConOut->Mode, sizeof(SavedConsoleMode));
  ConOut->QueryMode(ConOut, SavedConsoleMode.Mode, &Columns, &Rows);
//DEBUG((EFI_D_ERROR, "(L%d) %d * %d\n", __LINE__, Columns, Rows));  
  ConOut->EnableCursor(ConOut, FALSE);
  ConOut->SetAttribute(ConOut, AttribComm);
  NumberOfLines = MIN(NumberOfLines, Rows - 7);
  MaxLength = MIN (MaxLength, Columns - 2);
  Row    = (Rows - (NumberOfLines + 7)) / 2;
  Column = (Columns - (MaxLength + 2)) / 2;
  Line = AllocateZeroPool((MaxLength + 3) * sizeof(CHAR16));
  ASSERT(Line != NULL);
//DEBUG((EFI_D_ERROR, "(L%d) %d %d %d %d\n", __LINE__, NumberOfLines, MaxLength, Row, Column));    

  SetMem16(Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_DOWN_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_DOWN_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);

  SetMem16(Line, (MaxLength + 2) * 2, L' ');
  Line[0]             = BOXDRAW_VERTICAL;
  Line[MaxLength + 1] = BOXDRAW_VERTICAL;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row);
  ConOut->OutputString(ConOut, Line);
  Length = UefiLibGetStringWidth(Title, FALSE, 0, NULL) / 2;  
  ConOut->SetCursorPosition(ConOut, Column + 1 + (MaxLength - Length) / 2, Row++);
  ConOut->OutputString(ConOut, Title);

  SetMem16(Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_VERTICAL_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_VERTICAL_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);  
  
  SetMem16(Line, (MaxLength + 2) * 2, L' ');
  Line[0]             = BOXDRAW_VERTICAL;
  Line[MaxLength + 1] = BOXDRAW_VERTICAL;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);  
  
  VA_START (Args, Title);
  while ((String = VA_ARG(Args, CHAR16 *)) != NULL && NumberOfLines > 0) {
    SetMem16(Line, (MaxLength + 2) * 2, L' ');
    Line[0]             = BOXDRAW_VERTICAL;
    Line[MaxLength + 1] = BOXDRAW_VERTICAL;
    Line[MaxLength + 2] = L'\0';
    ConOut->SetCursorPosition(ConOut, Column, Row);
    ConOut->OutputString(ConOut, Line);
    
    Length = UefiLibGetStringWidth(String, FALSE, 0, NULL) / 2;
    if (Length <= MaxLength) {
      ConOut->SetCursorPosition (ConOut, Column + 1 + (MaxLength - Length) / 2, Row++);
      ConOut->OutputString (ConOut, String);
    } else {
      UefiLibGetStringWidth (String, TRUE, MaxLength, &Length);
      TmpString = AllocateZeroPool ((Length + 1) * sizeof (CHAR16));
      ASSERT (TmpString != NULL);
      StrnCpy(TmpString, String, Length - 3);
      StrCat (TmpString, L"...");
      ConOut->SetCursorPosition (ConOut, Column + 1, Row++);
      ConOut->OutputString (ConOut, TmpString);
      FreePool(TmpString);
    }
    NumberOfLines--;
  }
  VA_END(Args);

  SetMem16(Line, (MaxLength + 2) * 2, L' ');
  Line[0]             = BOXDRAW_VERTICAL;
  Line[MaxLength + 1] = BOXDRAW_VERTICAL;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);  
  
  SetMem16(Line, (MaxLength + 2) * 2, L' ');
  Line[0]             = BOXDRAW_VERTICAL;
  Line[MaxLength + 1] = BOXDRAW_VERTICAL;
  Line[MaxLength + 2] = L'\0';
//DEBUG((EFI_D_INFO, "(L%d) (%d,%d)\n", __LINE__, Column, Row));  
  ConOut->SetCursorPosition(ConOut, Column, Row);
  ConOut->OutputString(ConOut, Line);
  
  if(CStr1Len){
    Choice1X = Column + 1 + (MaxLength - StrLen(CStr[0]) - StrLen(CStr[1]) - 6)/3;
    Choice2X = Column + MaxLength - (Choice1X - Column) - StrLen(CStr[1]) - 2;
    ChoiceY  = Row++;
//  DEBUG((EFI_D_INFO, "(L%d) (%d,%d),(%d,%d) MaxL:%d X:%d\n", __LINE__, Choice1X, ChoiceY, Choice2X, ChoiceY, MaxLength, Column));
    ConOut->SetCursorPosition(ConOut, Choice1X, ChoiceY);
    ConOut->OutputString(ConOut, L"[");
    ConOut->SetAttribute(ConOut, AttribFocus);
    ConOut->OutputString(ConOut, CStr[0]);
    ConOut->SetAttribute(ConOut, AttribComm);  
    ConOut->OutputString(ConOut, L"]");
    ConOut->SetCursorPosition(ConOut, Choice2X, ChoiceY);
    ConOut->OutputString(ConOut, L"[");
    ConOut->OutputString(ConOut, CStr[1]);
    ConOut->OutputString(ConOut, L"]");    
    Choice1X++;
    Choice2X++;
  } else {
    String = CStr[0];
    Length = UefiLibGetStringWidth(String, FALSE, 0, NULL) / 2 + 2;
    ConOut->SetCursorPosition(ConOut, Column + 1 + (MaxLength - Length) / 2, Row++);
    ConOut->OutputString(ConOut, L"[");
    ConOut->SetAttribute(ConOut, AttribFocus);
    ConOut->OutputString(ConOut, CStr[0]);
    ConOut->SetAttribute(ConOut, AttribComm); 
    ConOut->OutputString(ConOut, L"]");     
  }  
 
  SetMem16(Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_UP_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_UP_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition(ConOut, Column, Row++);
  ConOut->OutputString(ConOut, Line);
  FreePool(Line);
  
  ChoiceIndex = 0;
  while (TRUE) {
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    if(Key.ScanCode == SCAN_NULL && Key.UnicodeChar == CHAR_CARRIAGE_RETURN){
      break;
    } else if(CStr1Len &&
              (Key.ScanCode == SCAN_UP    || 
               Key.ScanCode == SCAN_DOWN  ||
               Key.ScanCode == SCAN_RIGHT || 
               Key.ScanCode == SCAN_LEFT)) {
      if(ChoiceIndex == 0){
        ChoiceIndex = 1;
      } else {
        ChoiceIndex = 0;
      }
      ConOut->SetCursorPosition(ConOut, Choice1X, ChoiceY);
      ConOut->SetAttribute(ConOut, ChoiceIndex==0?AttribFocus:AttribComm);
      ConOut->OutputString(ConOut, CStr[0]); 
      ConOut->SetCursorPosition(ConOut, Choice2X, ChoiceY);
      ConOut->SetAttribute(ConOut, ChoiceIndex==1?AttribFocus:AttribComm); 
      ConOut->OutputString(ConOut, CStr[1]);       
    } else if(Key.ScanCode == SCAN_ESC){
      ChoiceIndex = 0xFF;
      break;
    }    
  }  
  
  ConOut->EnableCursor(ConOut, SavedConsoleMode.CursorVisible);
  ConOut->SetCursorPosition(ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
  ConOut->SetAttribute(ConOut, SavedConsoleMode.Attribute);

  return ChoiceIndex;
}



/**
  Set Secure Boot option into variable space.

  @param[in] VarValue              The option of Secure Boot.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

**/
EFI_STATUS
SaveSecureBootVariable (
  IN UINT8                         VarValue
  )
{
  EFI_STATUS                       Status;

  LibAuthVarSetPhysicalPresent(TRUE);
  Status = gRT->SetVariable (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
             sizeof (UINT8),
             &VarValue
             );
  LibAuthVarSetPhysicalPresent(FALSE);        
  return Status;
}



/**
  This function extracts configuration from variable.
  
  @param[in, out]  ConfigData   Point to SecureBoot configuration private data.

**/
VOID
SecureBootExtractConfigFromVariable (
  IN OUT SECUREBOOT_CONFIGURATION  *ConfigData
  ) 
{
  UINT8       SecureBootEnable;
  UINTN       DataSize;
  EFI_STATUS  Status;
  UINT8       SetupMode;	

  DataSize = sizeof(SecureBootEnable);
  Status = gRT->GetVariable(
                  EFI_SECURE_BOOT_ENABLE_NAME,
                  &gEfiSecureBootEnableDisableGuid,
                  NULL,
                  &DataSize,
                  &SecureBootEnable
                  );  
  if(EFI_ERROR(Status)){
    ConfigData->SecureBootState = 0;
  }else{
    ConfigData->SecureBootState = SecureBootEnable;
  }

  DataSize = sizeof(SetupMode);
  Status = gRT->GetVariable(
                  EFI_SETUP_MODE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &SetupMode
                  );
  if(EFI_ERROR(Status)){
    SetupMode = SETUP_MODE;
  }
	
  ConfigData->SetupMode = SetupMode;
//DEBUG((EFI_D_INFO, "SecureBootState : %d\n", ConfigData->SecureBootState));
//DEBUG((EFI_D_INFO, "SetupMode       : %d\n", ConfigData->SetupMode)); 	
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]   Request           A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[out]  Progress          On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param[out]  Results           A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
SecureBootExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL        *This,
  IN CONST EFI_STRING                            Request,
       OUT EFI_STRING                            *Progress,
       OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS                        Status;
  UINTN                             BufferSize;
  UINTN                             Size;
  SECUREBOOT_CONFIGURATION          Configuration;
  EFI_STRING                        ConfigRequest;
  EFI_STRING                        ConfigRequestHdr;
  SECUREBOOT_CONFIG_PRIVATE_DATA    *PrivateData;
  BOOLEAN                           AllocatedRequest;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  AllocatedRequest = FALSE;
  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  Size             = 0;
  
  ZeroMem (&Configuration, sizeof (Configuration));
  PrivateData      = SECUREBOOT_CONFIG_PRIVATE_FROM_THIS (This);
  *Progress        = Request;
  
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gSecureBootConfigFormSetGuid, mSecureBootStorageName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get Configuration from Variable.
  //
  SecureBootExtractConfigFromVariable (&Configuration);
  
  BufferSize = sizeof (SECUREBOOT_CONFIGURATION);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request is set to NULL or OFFSET is NULL, construct full request string.
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gSecureBootConfigFormSetGuid, mSecureBootStorageName, PrivateData->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[out] Progress           A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
SecureBootRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN CONST EFI_STRING                          Configuration,
       OUT EFI_STRING                          *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gSecureBootConfigFormSetGuid, mSecureBootStorageName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}

//- 3DD0DE67-02D7-4129-914A-9F377CC34B0D IDESecDev
// D5FD1546-22C5-4C2E-969F-273C00771080 HDD_PASSWORD_CONFIG
// D5FD1546-22C5-4C2E-969F-273C00771080 HPW_NV_CONF
// B0F901E4-C424-45DE-9081-95E20BDE6FB5 TCG_CONFIGURATION
// 5DAF50A5-EA81-4DE2-8F9B-CABDA9CF5C14 SECUREBOOT_CONFIGURATION
// 09D5B53F-F4B0-4F59-A0B1-7B57D35C0E05 EfiNicIp4ConfigVariable
// 02EEA107-98DB-400E-9830-460A1542D799 IP6_CONFIG_IFR_NVDATA
// D79DF6B0-EF44-43BD-9797-43E93BCF5FA8 VlanNvData
// 4B47D616-A8D6-4552-9D44-CCAD2E0F4CF9 ISCSI_CONFIG_IFR_NVDATA

#define HDD_PASSWORD_CONFIG_GUID \
  { \
    0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80 } \
  }
#define TCG_CONFIG_FORM_SET_GUID \
  { \
    0xb0f901e4, 0xc424, 0x45de, {0x90, 0x81, 0x95, 0xe2, 0xb, 0xde, 0x6f, 0xb5 } \
  }
#define SECUREBOOT_CONFIG_FORM_SET_GUID \
  { \
    0x5daf50a5, 0xea81, 0x4de2, {0x8f, 0x9b, 0xca, 0xbd, 0xa9, 0xcf, 0x5c, 0x14} \
  }  
#define EFI_NIC_IP4_CONFIG_NVDATA_GUID \
  { \
    0x9d5b53f, 0xf4b0, 0x4f59, { 0xa0, 0xb1, 0x7b, 0x57, 0xd3, 0x5c, 0xe, 0x5 } \
  }
#define IP6_CONFIG_NVDATA_GUID \
  { \
    0x2eea107, 0x98db, 0x400e, { 0x98, 0x30, 0x46, 0xa, 0x15, 0x42, 0xd7, 0x99 } \
  }
#define VLAN_CONFIG_FORM_SET_GUID \
  { \
    0xd79df6b0, 0xef44, 0x43bd, {0x97, 0x97, 0x43, 0xe9, 0x3b, 0xcf, 0x5f, 0xa8 } \
  }  
#define ISCSI_CONFIG_GUID \
  { \
    0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9 } \
  }
  
struct {
  EFI_GUID  VarGuid;
  CHAR16    *VarName;
} gVarListToBeDeleted[] = {
  {HDD_PASSWORD_CONFIG_GUID,        L"HDD_PASSWORD_CONFIG"},
  {HDD_PASSWORD_CONFIG_GUID,        L"HPW_NV_CONF"},
  {TCG_CONFIG_FORM_SET_GUID,        L"TCG_CONFIGURATION"},
  {SECUREBOOT_CONFIG_FORM_SET_GUID, L"SECUREBOOT_CONFIGURATION"},
  {EFI_NIC_IP4_CONFIG_NVDATA_GUID,  L"EfiNicIp4ConfigVariable"},
  {IP6_CONFIG_NVDATA_GUID,          L"IP6_CONFIG_IFR_NVDATA"},
  {VLAN_CONFIG_FORM_SET_GUID,       L"VlanNvData"},
  {ISCSI_CONFIG_GUID,               L"ISCSI_CONFIG_IFR_NVDATA"},
};

STATIC EFI_STATUS DeleteNoNeededNVData()
{
  UINTN         Index;
  CHAR16        *VarName;
  EFI_GUID      *VarGuid;
  UINT8         *Variable;
  UINTN         VarSize;
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT32        Attribute;
  
  for(Index  = 0; Index < ARRAY_ELEM_COUNT(gVarListToBeDeleted); Index++){
    VarGuid  = &gVarListToBeDeleted[Index].VarGuid;
    VarName  = gVarListToBeDeleted[Index].VarName;
    VarSize  = 0;
    Variable = NULL;
    Status = gRT->GetVariable(
                    VarName,
                    VarGuid,
                    &Attribute,
                    &VarSize,
                    Variable          
                    );
    if(Status != EFI_BUFFER_TOO_SMALL){
      continue;
    }
    
    Variable = AllocatePool(VarSize);
    if(Variable == NULL){
      Status = EFI_OUT_OF_RESOURCES;
      goto ProcExit;
    }
    Status = gRT->GetVariable(
                    VarName,
                    VarGuid,
                    &Attribute,   // now attribute is correct.
                    &VarSize,
                    Variable       
                    );
    gBS->FreePool(Variable);
    if(EFI_ERROR(Status)){
      continue;
    } 
    VarSize = 0;
    Variable = NULL;
    Status = gRT->SetVariable (
                    VarName,
                    VarGuid,
                    Attribute,
                    VarSize,
                    Variable
                    );    
    
  }  

ProcExit:
  return Status;  
}




typedef struct{
  EFI_GUID *VarGuid;
  CHAR16   *VarName;
} AUTHVAR_KEY_NAME;

STATIC AUTHVAR_KEY_NAME gSecureKeyVariableList[] = {
  {&gEfiGlobalVariableGuid, EFI_PLATFORM_KEY_NAME},                 // PK   0
  {&gEfiGlobalVariableGuid, EFI_KEY_EXCHANGE_KEY_NAME},             // KEK  1
  {&gEfiImageSecurityDatabaseGuid, EFI_IMAGE_SECURITY_DATABASE},    // db   2
  {&gEfiImageSecurityDatabaseGuid, EFI_IMAGE_SECURITY_DATABASE1},   // dbx  3
};

// {77fa9abd-0359-4d32-bd60-28f4e78f784b}.
STATIC EFI_GUID gMicrosoftSignatureOwnerGuid = \
{0x77fa9abd, 0x0359, 0x4d32, {0xBD, 0x60, 0x28, 0xF4, 0xE7, 0x8F, 0x78, 0x4b}};

// {7AE32DDF-1623-4904-AC2B-206AD65790D4}
STATIC EFI_GUID gMySignatureOwnerGuid = \
{0x7ae32ddf, 0x1623, 0x4904, {0xac, 0x2b, 0x20, 0x6a, 0xd6, 0x57, 0x90, 0xd4}};



STATIC
struct{
  EFI_GUID          *File;
  CHAR16            *UiName;
  AUTHVAR_KEY_NAME  *KeyName;
  UINT8             *Data;
  UINTN             DataSize;
  EFI_GUID          *SignatureOwner;
}gSecureKeyTable[] = {
  {(EFI_GUID*)PcdGetPtr(PcdSecureKeyPKFile),    L"PK",    &gSecureKeyVariableList[0], NULL, 0, &gMySignatureOwnerGuid},
//{(EFI_GUID*)PcdGetPtr(PcdSecureKeyKEKFile),   L"KEK",   &gSecureKeyVariableList[1], NULL, 0, &gMySignatureOwnerGuid},
  {(EFI_GUID*)PcdGetPtr(PcdSecureKeyDBFile),    L"DB",    &gSecureKeyVariableList[2], NULL, 0, &gMySignatureOwnerGuid},
  {(EFI_GUID*)PcdGetPtr(PcdSecureKeyMSKEKFile), L"MSKEK", &gSecureKeyVariableList[1], NULL, 0, &gMicrosoftSignatureOwnerGuid},
  {(EFI_GUID*)PcdGetPtr(PcdSecureKeyMSProFile), L"MSPro", &gSecureKeyVariableList[2], NULL, 0, &gMicrosoftSignatureOwnerGuid},
  {(EFI_GUID*)PcdGetPtr(PcdSecureKeyMSUEFFile), L"MSUEF", &gSecureKeyVariableList[2], NULL, 0, &gMicrosoftSignatureOwnerGuid},
};

STATIC BOOLEAN IsAnyKeyPresent()
{
  UINTN         Index;
  CHAR16        *VarName;
  EFI_GUID      *VarGuid;
  EFI_STATUS    Status;
  UINTN         VarSize;
  UINT8         *Variable;
  BOOLEAN       rc;
  
  Status = EFI_SUCCESS;
  rc     = FALSE;
  
  for(Index  = 0; Index < ARRAY_ELEM_COUNT(gSecureKeyVariableList); Index++){
    VarName  = gSecureKeyVariableList[Index].VarName;
    VarGuid  = gSecureKeyVariableList[Index].VarGuid;
    VarSize  = 0;
    Variable = NULL;
    Status = gRT->GetVariable(
                    VarName,
                    VarGuid,
                    NULL,
                    &VarSize,
                    Variable          
                    );
    if(Status == EFI_BUFFER_TOO_SMALL){
      rc = TRUE;
      break;
    }    
  }

  return rc;  
}

STATIC VOID FreeLoadedKeys()
{
  UINTN   Index; 
  
  for(Index = 0; Index < ARRAY_ELEM_COUNT(gSecureKeyTable); Index++){
    _FREE_NON_NULL(gSecureKeyTable[Index].Data);
    gSecureKeyTable[Index].DataSize = 0;
  }
}

/**
  Function to Load Secure Keys from firmware volume 
  given the image GUID 

  @param[in]          ImageGuid                Image guid of the file.
  @param[in,out]      DefaultsBuffer           Buffer to return file
  @param[in,out]      DefaultsBufferSize       File size

  @retval EFI_SUCCESS              Found Key.
  @retval Others                   Key search failed.

**/
STATIC
EFI_STATUS
GetX509Cert (
  IN   EFI_GUID      *ImageGuid,
  OUT  VOID          **DefaultsBuffer,
  OUT  UINTN         *DefaultsBufferSize
  )
{
  EFI_STATUS                      Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv;
  UINTN                           FvProtocolCount;
  EFI_HANDLE                      *FvHandles;
  UINTN                           Index1;
  UINT32                          AuthenticationStatus;

  *DefaultsBuffer      = NULL;
  *DefaultsBufferSize = 0;

  FvHandles = NULL;
  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiFirmwareVolume2ProtocolGuid,
                NULL,
                &FvProtocolCount,
                &FvHandles
                );

  if (!EFI_ERROR (Status)) {
    for (Index1 = 0; Index1 < FvProtocolCount; Index1++) {
      Status = gBS->HandleProtocol (
                      FvHandles[Index1],
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **) &Fv
                      );
      *DefaultsBufferSize= 0;

      Status = Fv->ReadSection (
                    Fv,
                    ImageGuid,
                    EFI_SECTION_RAW,
                    0,
                    DefaultsBuffer,
                    DefaultsBufferSize,
                    &AuthenticationStatus
                    );

      if (!EFI_ERROR (Status)) {
        Status = EFI_SUCCESS;
        break;
      }
    }
  }
  return Status;
}

STATIC
EFI_STATUS
AppendX509FromFV(
  IN EFI_GUID                         *CertificateGuid,
  IN  CHAR16                          *VariableName,
  IN EFI_GUID                         *VendorGuid,
  IN EFI_GUID                         *SignatureOwner
  )
{
  EFI_STATUS                        Status;
  VOID                              *Data;
  UINTN                             DataSize;
  UINTN                             SigDBSize;
  UINT32                            Attr;
  UINTN                             X509DataSize;
  VOID                              *X509Data;

  X509DataSize  = 0;
  X509Data      = NULL;
  SigDBSize     = 0;
  DataSize      = 0;
  Data          = NULL;

  Status = GetX509Cert( CertificateGuid, &X509Data,&X509DataSize);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  SigDBSize = X509DataSize;

  Data = AllocateZeroPool (SigDBSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  CopyMem ((UINT8* )Data, X509Data, X509DataSize);

  Attr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS 
          | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  //
  // Check if signature database entry has been already existed. 
  // If true, use EFI_VARIABLE_APPEND_WRITE attribute to append the 
  // new signature data to original variable
  //    

  Status = gRT->GetVariable(
                  VariableName,
                  VendorGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Attr |= EFI_VARIABLE_APPEND_WRITE;
  } else if (Status != EFI_NOT_FOUND) {
    goto ON_EXIT;
  }  

  Status = gRT->SetVariable(
                  VariableName,
                  VendorGuid,
                  Attr,
                  SigDBSize,
                  Data
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

ON_EXIT:
  
  if (Data != NULL) {
    FreePool (Data);
  }

  if (X509Data != NULL) {
    FreePool (X509Data);
  }

  return Status;
}

STATIC EFI_STATUS LoadKeysInFv()
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINTN                         Index;  
  EFI_FV_FILETYPE               FileType;
  UINT32                        AuthStatus;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         FileSize;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *ptFV2;
  UINT8                         *FileBuffer;

  
  HandleBuffer = NULL;
  ptFV2        = NULL;
  
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  };
  
  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     &ptFV2
                     );
    ASSERT(!EFI_ERROR(Status));
    ASSERT(ARRAY_ELEM_COUNT(gSecureKeyTable)>0);
    Status = ptFV2->ReadFile (
                      ptFV2,
                      gSecureKeyTable[0].File,
                      NULL,        // get info only
                      &FileSize,
                      &FileType,
                      &Attributes,
                      &AuthStatus
                      );
    if(!EFI_ERROR(Status)){
      break;
    }
  }
  if(Index >= NumberOfHandles){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  _FREE_NON_NULL(HandleBuffer);
  
  for(Index = 0; Index < ARRAY_ELEM_COUNT(gSecureKeyTable); Index++){
    FileBuffer = NULL;
    FileSize   = 0;
    Status = ptFV2->ReadSection(
                      ptFV2,
                      gSecureKeyTable[Index].File,
                      EFI_SECTION_RAW,
                      0,              // SectionInstance
                      &FileBuffer,
                      &FileSize,
                      &AuthStatus
                      );
    if(EFI_ERROR(Status)){
      goto ProcExit;
    };                
    
    gSecureKeyTable[Index].Data     = FileBuffer;
    gSecureKeyTable[Index].DataSize = FileSize;
  }
  
ProcExit:
  _FREE_NON_NULL(HandleBuffer);
  if(EFI_ERROR(Status)){
    FreeLoadedKeys();
  }
  return Status;   
}


STATIC EFI_STATUS GetTimeStamp(EFI_TIME *Time)
{
  EFI_STATUS  Status;
  
  ASSERT(Time != NULL);
  
	Status = gRT->GetTime(Time, NULL);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  };
	
// Pad1, Nanosecond, TimeZone, Daylight and Pad2 components 
// of the TimeStamp value must be set to zero.	
	Time->Pad1       = 0;
	Time->Nanosecond = 0;
	Time->TimeZone   = 0;
	Time->Daylight   = 0;
	Time->Pad2       = 0;
	
ProcExit:
  return Status;
}

STATIC
EFI_STATUS
CreateDummyTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data
  )
{
  EFI_STATUS                       Status;
  UINT8                            *NewData;
  UINT8                            *Payload;
  UINTN                            PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2    *DescriptorData;
  UINTN                            DescriptorSize;
  EFI_TIME                         Time;
  
  
  ASSERT(Data != NULL && DataSize != NULL);
  Status  = EFI_SUCCESS;
  NewData = NULL;  
  
  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the 
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;


// EFI_VARIABLE_AUTHENTICATION_2
//   TimeStamp(EFI_TIME)
//   AuthInfo(WIN_CERTIFICATE_UEFI_GUID) 
//     Hdr(WIN_CERTIFICATE) 
//       dwLength
//       wRevision
//       wCertificateType
//     CertType(EFI_GUID)
//     CertData(UINT8[1])
  DescriptorSize = OFFSET_OF(EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) +   // TimeStamp 
                   OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData);        // Hdr + CertType
  NewData = AllocateZeroPool(DescriptorSize + PayloadSize);
  if(NewData == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  if((Payload != NULL) && (PayloadSize != 0)){
    CopyMem(NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2*)NewData;
  Status = GetTimeStamp(&Time);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  };
  CopyMem(&DescriptorData->TimeStamp, &Time, sizeof(EFI_TIME));
 
  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);
  
  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  
ProcExit:
  if(EFI_ERROR(Status) && NewData!=NULL){
    gBS->FreePool(NewData);
  }
  return Status;
}


STATIC
EFI_STATUS
DeleteSecureBootVariableNoAuth (
  IN  CHAR16     *VariableName,
  IN  EFI_GUID   *VendorGuid
  )
{
  EFI_STATUS              Status;
  VOID*                   Variable;
  UINTN                   VarSize;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINT32                  Attribute;

  
  Status   = EFI_SUCCESS;
  Variable = NULL;
  VarSize  = 0;
  
  Status = gRT->GetVariable(
                  VariableName,
                  VendorGuid,
                  &Attribute,   // bug:if DataSize is not big enough, Attribute will not be updated.
                  &VarSize,
                  Variable          
                  );
  if(Status != EFI_BUFFER_TOO_SMALL){   // not found
//- Status = EFI_NOT_FOUND;
    Status = EFI_SUCCESS;
    goto ProcExit;  
  }
  Variable = AllocatePool(VarSize);
  if(Variable == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  Status = gRT->GetVariable(
                  VariableName,
                  VendorGuid,
                  &Attribute,   // now attribute is correct.
                  &VarSize,
                  Variable       
                  );
  gBS->FreePool(Variable);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  
  Data     = NULL;
  DataSize = 0;
  Status = CreateDummyTimeBasedPayload(&DataSize, &Data);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "(L%d) %r 0x%X 0x%X\n", Status, Data, DataSize));
    goto ProcExit;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attribute,
                  DataSize,
                  Data
                  );
  _FREE_NON_NULL(Data);

ProcExit:  
  return Status;
}

STATIC EFI_STATUS SetCustomMode(UINT8 Mode)
{
  EFI_STATUS  Status;
  UINT8       CustomMode;
  UINTN       DataSize;
  UINT32      Attribute;

  DEBUG((EFI_D_INFO, "%a(%d)\n", __FUNCTION__, Mode)); 
  
  Status = EFI_SUCCESS;  
  DataSize = sizeof(CustomMode);
  Status = gRT->GetVariable(
                  EFI_CUSTOM_MODE_NAME,
                  &gEfiCustomModeEnableGuid,
                  &Attribute,
                  &DataSize,
                  &CustomMode
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  
  if(CustomMode != Mode){
    CustomMode = Mode;
    Status = gRT->SetVariable (                          
                    EFI_CUSTOM_MODE_NAME,
                    &gEfiCustomModeEnableGuid,
                    Attribute,
                    sizeof(CustomMode),
                    &CustomMode
                    );
  }

ProcExit:  
  return Status;
}



STATIC 
EFI_STATUS
AddDbxInitKey (
  VOID
  )
{
  EFI_STATUS Status;

  Status = AppendX509FromFV(
             (EFI_GUID*)PcdGetPtr(PcdSecureKeyMSDBXFile), 
             gSecureKeyVariableList[3].VarName, 
             gSecureKeyVariableList[3].VarGuid, 
             &gMicrosoftSignatureOwnerGuid
             );
  DEBUG((EFI_D_INFO, "%a() %r\n", __FUNCTION__, Status));
  return Status;
}




// WHCK request platform ships with an initialized, possibly empty, forbidden signature database.
// Add a SHA256 with ALL ZERO.
STATIC 
EFI_STATUS
AddDummyDBxInitKey (
  VOID
  )
{
  CHAR16                         *VarName; 
  EFI_GUID                       *VarGuid;
  EFI_STATUS                     Status;
  EFI_SIGNATURE_LIST             *SignList;
  UINTN                          SignListSize;
  UINTN                          CertDataSize;
  UINT8                          *VarData;
  EFI_SIGNATURE_DATA             *SignData; 
  UINTN                          VarDataSize;  
  UINT32                         Attribute;  
  
  SignList     = NULL;  
  VarData      = NULL;
  CertDataSize = 32;          // SHA256
  VarName      = gSecureKeyVariableList[3].VarName;
  VarGuid      = gSecureKeyVariableList[3].VarGuid;

  SignListSize = sizeof(EFI_SIGNATURE_LIST) + OFFSET_OF(EFI_SIGNATURE_DATA, SignatureData) + CertDataSize;
  SignList     = (EFI_SIGNATURE_LIST*)AllocatePool(SignListSize);
  if(SignList == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  
  CopyMem(&SignList->SignatureType, &gEfiCertSha256Guid, sizeof(EFI_GUID));
  SignList->SignatureListSize   = (UINT32)SignListSize;
  SignList->SignatureHeaderSize = 0;
  SignList->SignatureSize       = (UINT32)(OFFSET_OF(EFI_SIGNATURE_DATA,SignatureData) + CertDataSize);
  SignData = (EFI_SIGNATURE_DATA*)((UINT8*)SignList + sizeof(EFI_SIGNATURE_LIST));
  CopyMem(&SignData->SignatureOwner, &gMySignatureOwnerGuid, sizeof(EFI_GUID));
  ZeroMem(&SignData->SignatureData[0], CertDataSize);

  VarData     = (UINT8*)SignList;
  VarDataSize = SignListSize;
  Status = CreateDummyTimeBasedPayload(&VarDataSize, &VarData);
  if(EFI_ERROR(Status)){
    if((UINTN)VarData == (UINTN)SignList){
      VarData = NULL;
    }
    goto ProcExit;
  }

  Attribute = EFI_VARIABLE_NON_VOLATILE | 
              EFI_VARIABLE_BOOTSERVICE_ACCESS | 
              EFI_VARIABLE_RUNTIME_ACCESS |
              EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;  

  Status = gRT->SetVariable (
                  VarName,
                  VarGuid,
                  Attribute,
                  VarDataSize,
                  VarData
                  );

ProcExit:
  _FREE_NON_NULL(SignList);
  _FREE_NON_NULL(VarData);
  return Status;              
}



STATIC 
EFI_STATUS 
AddSecureBootVarNoAuth(
        CHAR16        *VarName, 
        EFI_GUID      *VarGuid, 
  CONST UINT8         *CertData, 
        UINTN         CertDataSize,
        EFI_GUID      *SignatureOwner
  )
{
  EFI_STATUS                     Status;
  EFI_SIGNATURE_LIST             *SignList;
  UINTN                          SignListSize;
  UINT8                          *VarData;
  UINTN                          VarDataSize;
  UINT32                         Attribute;
  UINTN                          DataSize;
  EFI_SIGNATURE_DATA             *SignData;

  
  SignList = NULL;
  VarData  = NULL;
  
  SignListSize = sizeof(EFI_SIGNATURE_LIST) + OFFSET_OF(EFI_SIGNATURE_DATA, SignatureData) + CertDataSize;
  SignList     = (EFI_SIGNATURE_LIST*)AllocatePool(SignListSize);
  if(SignList == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
 
  CopyMem(&SignList->SignatureType, &gEfiCertX509Guid, sizeof(EFI_GUID));
  SignList->SignatureListSize   = (UINT32)SignListSize;
  SignList->SignatureHeaderSize = 0;
  SignList->SignatureSize       = (UINT32)(OFFSET_OF(EFI_SIGNATURE_DATA,SignatureData) + CertDataSize);
  SignData = (EFI_SIGNATURE_DATA*)((UINT8*)SignList + sizeof(EFI_SIGNATURE_LIST));
  CopyMem(&SignData->SignatureOwner, SignatureOwner, sizeof(EFI_GUID));
  CopyMem(&SignData->SignatureData[0], CertData, CertDataSize);

  VarData     = (UINT8*)SignList;
  VarDataSize = SignListSize;
  Status = CreateDummyTimeBasedPayload(&VarDataSize, &VarData);
  if(EFI_ERROR(Status)){
    if((UINTN)VarData == (UINTN)SignList){
      VarData = NULL;
    }  
    goto ProcExit;
  }

  Attribute = EFI_VARIABLE_NON_VOLATILE | 
              EFI_VARIABLE_BOOTSERVICE_ACCESS | 
              EFI_VARIABLE_RUNTIME_ACCESS |
              EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  
  if(StrCmp(VarName, EFI_PLATFORM_KEY_NAME)!=0){
    DataSize = 0;
    Status = gRT->GetVariable(
                   VarName,
                   VarGuid,
                   NULL,
                   &DataSize,
                   NULL
                   );         
    if(Status == EFI_BUFFER_TOO_SMALL){
      Attribute |= EFI_VARIABLE_APPEND_WRITE;
    }
  }  
  
  Status = gRT->SetVariable (
                  VarName,
                  VarGuid,
                  Attribute,
                  VarDataSize,
                  VarData
                  );

ProcExit:
  _FREE_NON_NULL(SignList);
  _FREE_NON_NULL(VarData);
  return Status;
}

STATIC EFI_STATUS AddMfgKeys()
{
  UINTN         Index;
  CHAR16        *VarName;
  EFI_GUID      *VarGuid;
  EFI_STATUS    Status;
  UINT8         *CertData;
  UINTN         CertDataSize;
  EFI_GUID      *SignatureOwner;
  
  Status = EFI_SUCCESS;
  LibAuthVarSetPhysicalPresent(TRUE);  
  SetCustomMode(CUSTOM_SECURE_BOOT_MODE);
//AddDummyDBxInitKey();                   // WHCK request.
  AddDbxInitKey();
  for(Index = 0; Index < ARRAY_ELEM_COUNT(gSecureKeyTable); Index++){
    VarName        = gSecureKeyTable[Index].KeyName->VarName;
    VarGuid        = gSecureKeyTable[Index].KeyName->VarGuid;
    CertData       = gSecureKeyTable[Index].Data;
    CertDataSize   = gSecureKeyTable[Index].DataSize;
    SignatureOwner = gSecureKeyTable[Index].SignatureOwner;
    Status = AddSecureBootVarNoAuth(VarName, VarGuid, CertData, CertDataSize, SignatureOwner);
  }
  SetCustomMode(STANDARD_SECURE_BOOT_MODE);
  LibAuthVarSetPhysicalPresent(FALSE);
  
  return Status;
}

STATIC EFI_STATUS RemoveAllCertKey(VOID)
{
  UINTN           Index;
  CHAR16          *VarName;
  EFI_GUID        *VarGuid;
  EFI_STATUS      Status;
  
  Status = EFI_SUCCESS;
  LibAuthVarSetPhysicalPresent(TRUE);
  SetCustomMode(CUSTOM_SECURE_BOOT_MODE);
  for(Index = 0; Index < ARRAY_ELEM_COUNT(gSecureKeyVariableList); Index++){
    VarName = gSecureKeyVariableList[Index].VarName;
    VarGuid = gSecureKeyVariableList[Index].VarGuid;
    Status = DeleteSecureBootVariableNoAuth(VarName, VarGuid);
  }
  SetCustomMode(STANDARD_SECURE_BOOT_MODE);
  LibAuthVarSetPhysicalPresent(FALSE);
  
  return Status;  
}

STATIC EFI_STATUS AuthVarMfgReset(VOID)
{
  EFI_STATUS  Status;

  Status = LoadKeysInFv();
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  RemoveAllCertKey();
  AddMfgKeys();
  FreeLoadedKeys();  
  
  DeleteNoNeededNVData();
  
ProcExit:
  return Status;  
}



CHAR16 *gYesNoStrList[2];
CHAR16 *gRestoreFactoryKeysStr;
CHAR16 *gInstallDefaultKeyStr;
CHAR16 *gResetToSetupModeStr;	
CHAR16 *gWishResetToSetupModeStr;


VOID InitString(EFI_HII_HANDLE  HiiHandle)
{
  EFI_STRING  Str;

  Str = HiiGetString(HiiHandle, STRING_TOKEN(STR_YES), NULL);
  if(gYesNoStrList[0] != NULL && StrCmp(gYesNoStrList[0], Str) == 0){
    goto ProcExit;
  }	

  DEBUG((EFI_D_INFO, "Update String\n"));

  if(gYesNoStrList[0] != NULL){
    FreePool(gYesNoStrList[0]);
    FreePool(gYesNoStrList[1]);
    FreePool(gRestoreFactoryKeysStr);
    FreePool(gInstallDefaultKeyStr);
    FreePool(gResetToSetupModeStr);
    FreePool(gWishResetToSetupModeStr);		
  }
  gYesNoStrList[0]         = HiiGetString(HiiHandle, STRING_TOKEN(STR_YES), NULL);
  gYesNoStrList[1]         = HiiGetString(HiiHandle, STRING_TOKEN(STR_NO), NULL);	  
  gRestoreFactoryKeysStr   = HiiGetString(HiiHandle, STRING_TOKEN(STR_RESTORE_FACTORY_KEY_TITLE), NULL);
  gInstallDefaultKeyStr    = HiiGetString(HiiHandle, STRING_TOKEN(STR_RESTORE_FACTORY_KEY_CONFIRM), NULL);
  gResetToSetupModeStr     = HiiGetString(HiiHandle, STRING_TOKEN(STR_RESET_TO_SETUP_MODE_TITLE), NULL);
  gWishResetToSetupModeStr = HiiGetString(HiiHandle, STRING_TOKEN(STR_RESET_TO_SETUP_MODE_CONFIRM), NULL);
  
ProcExit:
  if(Str != NULL){FreePool(Str);}
  return;	
}




/**
  This function is called to provide results data to the driver.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
SecureBootCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN     EFI_BROWSER_ACTION                    Action,
  IN     EFI_QUESTION_ID                       QuestionId,
  IN     UINT8                                 Type,
  IN     EFI_IFR_TYPE_VALUE                    *Value,
     OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  EFI_INPUT_KEY                   InputKey;
  EFI_STATUS                      Status;  
  SECUREBOOT_CONFIG_PRIVATE_DATA  *Private;
  UINTN                           BufferSize;
  SECUREBOOT_CONFIGURATION        IfrNvData;
  UINT8                           *SecureBootEnable;
  BOOLEAN                         rc;
	STATIC BOOLEAN                  ResetFlagSet = FALSE;
  UINTN                           Choice;
  UINTN                           DataSize;
  UINT8                           SetupMode;
	

  if (This == NULL || Value == NULL || ActionRequest == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
  
  Private    = SECUREBOOT_CONFIG_PRIVATE_FROM_THIS(This);
  Status     = EFI_SUCCESS;
  BufferSize = sizeof(IfrNvData);
  rc = HiiGetBrowserData (
         &gSecureBootConfigFormSetGuid, 
         mSecureBootStorageName, 
         BufferSize, 
         (UINT8*)&IfrNvData
         );
  if(!rc){
    DEBUG((EFI_D_ERROR, "(L%d) HiiGetBrowserData error\n", __LINE__));
    Status = EFI_ABORTED;
    goto ProcExit;
  }

  InitString(Private->HiiHandle);	
  
  if (Action == EFI_BROWSER_ACTION_CHANGING) {

    DataSize = sizeof(SetupMode);
    Status = gRT->GetVariable(
                    EFI_SETUP_MODE_NAME,
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    &SetupMode
                    );
    if(EFI_ERROR(Status)) {
      SetupMode = SETUP_MODE;
    }

    Status = EFI_ABORTED;
		
    switch (QuestionId) {
    
      case KEY_SECURE_BOOT_ENABLE:
        if(SetupMode == USER_MODE) {
          SecureBootEnable = NULL;
          GetVariable2 (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, (VOID**)&SecureBootEnable, NULL);
          if (SecureBootEnable != NULL) {
            Status = SaveSecureBootVariable (Value->u8);
            if (EFI_ERROR(Status)) {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &InputKey,
                L"Only Physical Presence User could disable secure boot!",
                NULL
                );
              Status = EFI_UNSUPPORTED;
            }
            gBS->FreePool(SecureBootEnable);
          }
        }  
        break;
        
      case KEY_SECURE_BOOT_RESET:
        Choice = DrawConfirmDialog(0, gYesNoStrList, gResetToSetupModeStr, gWishResetToSetupModeStr, NULL);
        if(Choice == 0){          // yes
          Status = RemoveAllCertKey();
        }      
        break;      
      
      case KEY_SECURE_BOOT_RESTORE:
        Choice = DrawConfirmDialog(0, gYesNoStrList, gRestoreFactoryKeysStr, gInstallDefaultKeyStr, NULL);
        if(Choice == 0){          // yes
          Status = AuthVarMfgReset();
        }      
        break;
        
      default:
        break;
    }

    if(!EFI_ERROR(Status) && !ResetFlagSet){
      PcdSetBool(PcdSetupExitNeedReset, TRUE);
      ResetFlagSet = TRUE;			
    }			
  } 
  
  if (!EFI_ERROR (Status)) {
    SecureBootExtractConfigFromVariable(&IfrNvData);
    BufferSize = sizeof(SECUREBOOT_CONFIGURATION);
    rc = HiiSetBrowserData(
           &gSecureBootConfigFormSetGuid, 
           mSecureBootStorageName, 
           BufferSize, 
           (UINT8*)&IfrNvData, 
           NULL
           );
    if(!rc){
      DEBUG((EFI_D_ERROR, "(L%d) HiiSetBrowserData error\n", __LINE__));
      Status = EFI_ABORTED;
    }
  }
  
ProcExit:  
//DEBUG((EFI_D_INFO, "%a() A:%d Q:%d %r\n", __FUNCTION__, Action, QuestionId, Status));
  return Status;
}

/**
  This function publish the SecureBoot configuration Form.

  @param[in, out]  PrivateData   Points to SecureBoot configuration private data.

  @retval EFI_SUCCESS            HII Form is installed successfully.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallSecureBootConfigForm (
  IN OUT SECUREBOOT_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  DriverHandle = NULL;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSecureBootHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = DriverHandle;

  //
  // Publish the HII package list
  //
  HiiHandle = HiiAddPackages (
                &gSecureBootConfigFormSetGuid,
                DriverHandle,
                SecureBootConfigDxeStrings,
                SecureBootConfigBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mSecureBootHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle = HiiHandle;
  
  return EFI_SUCCESS;
}

/**
  This function removes SecureBoot configuration Form.

  @param[in, out]  PrivateData   Points to SecureBoot configuration private data.

**/
VOID
UninstallSecureBootConfigForm (
  IN OUT SECUREBOOT_CONFIG_PRIVATE_DATA    *PrivateData
  )
{
  //
  // Uninstall HII package list
  //
  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }

  //
  // Uninstall HII Config Access Protocol
  //
  if (PrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           PrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mSecureBootHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &PrivateData->ConfigAccess,
           NULL
           );
    PrivateData->DriverHandle = NULL;
  }

  FreePool (PrivateData);
}


SECUREBOOT_CONFIG_PRIVATE_DATA  mSecureBootConfigPrivateDateTemplate = {
  SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE,  
  {
    SecureBootExtractConfig,
    SecureBootRouteConfig,
    SecureBootCallback
  }
};

/**
  The entry point for SecureBoot configuration driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_ALREADY_STARTED    The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES   Fail to execute entry point due to lack of resources.
  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Fail to get the SecureBootEnable variable.

**/
EFI_STATUS
EFIAPI
SecureBootConfigDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                       Status;
  SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData;
  
  //
  // If already started, return.
  //
    Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  ImageHandle,
                  ImageHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }
  
  //
  // Create a private data structure.
  //
  PrivateData = AllocateCopyPool (sizeof(SECUREBOOT_CONFIG_PRIVATE_DATA), &mSecureBootConfigPrivateDateTemplate);
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
    
  //
  // Install SecureBoot configuration form
  //
  Status = InstallSecureBootConfigForm(PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install private GUID.
  //    
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );

  if (EFI_ERROR(Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    UninstallSecureBootConfigForm (PrivateData);
  }  
  
  return Status;
}

/**
  Unload the SecureBoot configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The SecureBoot configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
SecureBootConfigDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );  
  if (EFI_ERROR (Status)) {
    return Status;  
  }
  
  ASSERT (PrivateData->Signature == SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         &ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );
  
  UninstallSecureBootConfigForm (PrivateData);

  return EFI_SUCCESS;
}


