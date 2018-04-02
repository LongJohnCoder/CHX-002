/** @file
  ByoFormSetManager.h
  
  Interface of Byo Formset Manager Protocol.
**/

#ifndef __BYO_FORMSET_MANAGER_H__
#define __BYO_FORMSET_MANAGER_H__

//
// Device Manager Setup Protocol GUID
//
#define EFI_BYO_FORMSET_MANAGER_PROTOCOL_GUID \
  { 0x65e4992f, 0xd77c, 0x494d, { 0x9a, 0xd1, 0x68, 0x77, 0x5b, 0xb9, 0x1a, 0xa1 } }

extern EFI_GUID gEfiByoFormsetManagerProtocolGuid;

typedef struct _EFI_BYO_FORMSET_MANAGER_PROTOCOL EFI_BYO_FORMSET_MANAGER_PROTOCOL;

typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                   Link;
  EFI_GUID                       Guid;
  EFI_HII_HANDLE             HiiHandle;  
  EFI_STRING_ID              FormSetTitle;
} BYO_BROWSER_FORMSET;

#define BYO_FORM_BROWSER_FORMSET_SIGNATURE  SIGNATURE_32 ('B', 'F', 'B', 'L')
#define BYO_FORM_BROWSER_FORMSET_FROM_LINK(a)  CR (a, BYO_BROWSER_FORMSET, Link, BYO_FORM_BROWSER_FORMSET_SIGNATURE)

typedef EFI_STATUS
(*INSERT_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
(*REMOVE_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
( *RUN_BYO_FORMSET) (
  IN EFI_BYO_FORMSET_MANAGER_PROTOCOL    *This,
  IN EFI_GUID    *FormsetGuid
);

typedef BOOLEAN
( *SETUP_CHECK_FORMSET) (
  IN EFI_GUID    *FormsetGuid
);

typedef EFI_STATUS
( *SETUP_CHECK_PASSWORD) (
  IN CHAR16 *Title,
  OUT CHAR16 *Password
);


/**
  Draw a confirm pop up windows based on the Title, number of lines and
  strings specified. TRUE will be return If YES be selected.

**/
typedef
BOOLEAN
(EFIAPI *CREATE_WARNING_DIALOG) (
  IN  CHAR16                    *Title,
  IN  UINTN                       NumberOfStrings,
  IN  CHAR16                    *String,
  ...
  );

typedef struct _EFI_BYO_FORMSET_MANAGER_PROTOCOL {
  LIST_ENTRY    ByoFormSetList;
  INSERT_BYO_FORMSET    Insert;
  REMOVE_BYO_FORMSET    Remove;
  RUN_BYO_FORMSET    Run;
  SETUP_CHECK_FORMSET    CheckFormset;  
  SETUP_CHECK_PASSWORD    CheckPassword;
  CREATE_WARNING_DIALOG    CreateWarningDialog;
} EFI_BYO_FORMSET_MANAGER_PROTOCOL;

#endif

