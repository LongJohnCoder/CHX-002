#ifndef _BYO_DEVICE_MANAGER_SETUP_PROTOCOL_H_
#define _BYO_DEVICE_MANAGER_SETUP_PROTOCOL_H_

//
// Forward reference for pure ANSI compatability
//
//why EFI_FORWARD_DECLARATION (EFI_DEVICE_MANAGER_SETUP_PROTOCOL);


//
// Device Manager Setup Protocol GUID
//
#define EFI_BYO_DEVICE_MANAGER_SETUP_PROTOCOL_GUID \
{ 0x392744da, 0xdf68, 0x4c3b, 0x96, 0x6b, 0xf2, 0xf, 0xf, 0x47, 0xbc, 0x23 }

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiByoDeviceManagerSetupProtocolGuid;

//
// Direction Enumeration
//
typedef enum {
  NoChange,
  Left,
  Right,
  DownNextForm,
  StayAtCurrentPage,
} MOVE_TYPE;

//
// Internal menu structure
//
typedef struct {
  UINTN   Page;
  UINT16  MenuTitle;
  CHAR16* String;
} MENU_ITEM;

#define MAX_ITEMS 10

//
// Protocol Data Structures
//

typedef EFI_STATUS
( *SETUP_CHECK_PASSWORD) (
    IN CHAR16 *Title,
    IN CHAR16 *Password
);

typedef struct _EFI_BYO_DEVICE_MANAGER_SETUP_PROTOCOL {
  BOOLEAN   AtRoot;             // Is the DeviceManager at the root of the tree
  BOOLEAN   Finished;           // Has the user hit save and exit?
  BOOLEAN   Changed;            // Has any settings been changed?
  BOOLEAN   bFaForm;       // For FormId within 0xFA**.
  UINT16      FaFormId;        // For FormId within 0xFA**.
  UINTN     PreviousMenuEntry;  // Saved previous menu item
  BOOLEAN         UseMenus;           // Are the following structures used
  MOVE_TYPE       Direction;          // Direction of the menu change
  UINT8           CurRoot;            // Index of menu item currently selected
  UINT8           MenuItemCount;      // Number of valid menu items
  MENU_ITEM       MenuList[MAX_ITEMS];// Menu item list.
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ExitConfigAccess; // for f9\f10
  SETUP_CHECK_PASSWORD CheckPassword;
} EFI_BYO_DEVICE_MANAGER_SETUP_PROTOCOL;


#endif
