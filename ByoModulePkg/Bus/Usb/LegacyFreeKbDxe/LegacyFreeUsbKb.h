/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  LegacyFreeUsbKb.h

Abstract:


Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_KB_H
#define _USB_KB_H

#include <Uefi.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LegacyBios.h> 
#include <Protocol/Ps2Policy.h>
#include <Protocol/IsaIo.h>

#include <Guid/HiiKeyBoardLayout.h>

#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiUsbLib.h>
#include <Library/HiiLib.h>
#include <IndustryStandard/Usb.h>
#include "ComponentName.h"

#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s
#define MAX_KEY_ALLOWED                 32
#define KEYBOARD_BUFFER_MAX_COUNT       32
//
// BISO Keyboard Defines
//
#define CHAR_SCANCODE                   0xe0
#define CHAR_ESC                        0x1b

#define HZ                  1000 * 1000 * 10
#define USBKBD_REPEAT_DELAY ((HZ) / 2)
#define USBKBD_REPEAT_RATE  ((HZ) / 50)

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

#define BOOT_PROTOCOL       0
#define REPORT_PROTOCOL     1

typedef struct {
  UINT8 Reserved : 4;
  UINT8 ScrollLock : 1;
  UINT8 NumLock : 1;
  UINT8 CapsLock : 1;
  UINT8 Insert : 1;
} PS2_BDA_LED_MAP;

#define  BDA_LED_PTR        0x417

//
// 0040h:0017h - KEYBOARD - STATUS FLAGS 1
//   7 INSert active
//   6 Caps Lock active
//   5 Num Lock active
//   4 Scroll Lock active
//   3 either Alt pressed
//   2 either Ctrl pressed
//   1 Left Shift pressed
//   0 Right Shift pressed
//
// 0040h:0018h - KEYBOARD - STATUS FLAGS 2
//   7: insert key is depressed
//   6: caps-lock key is depressed (does not work well)
//   5: num-lock key is depressed (does not work well)
//   4: scroll lock key is depressed (does not work well)
//   3: suspend key has been toggled (does not work well)
//   2: system key is pressed and held (does not work well)
//   1: left ALT key is pressed
//   0: left CTRL key is pressed
//
#define KB_INSERT_BIT             (0x1 << 7)
#define KB_CAPS_LOCK_BIT          (0x1 << 6)
#define KB_NUM_LOCK_BIT           (0x1 << 5)
#define KB_SCROLL_LOCK_BIT        (0x1 << 4)
#define KB_ALT_PRESSED            (0x1 << 3)
#define KB_CTRL_PRESSED           (0x1 << 2)
#define KB_LEFT_SHIFT_PRESSED     (0x1 << 1)
#define KB_RIGHT_SHIFT_PRESSED    (0x1 << 0)

#define KB_SUSPEND_PRESSED        (0x1 << 3)
#define KB_SYSREQ_PRESSED         (0x1 << 2)
#define KB_LEFT_ALT_PRESSED       (0x1 << 1)
#define KB_LEFT_CTRL_PRESSED      (0x1 << 0)

typedef struct {
  BOOLEAN  Down;
  UINT8 KeyCode;
} USB_KEY;

typedef struct {
  USB_KEY Buffer[MAX_KEY_ALLOWED + 1];
  UINT8   BufferHead;
  UINT8   BufferTail;
} USB_KB_BUFFER;

#define USB_KB_DEV_SIGNATURE  SIGNATURE_32 ('u', 'k', 'b', 'd')

#define USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('u', 'k', 'b', 'x')

typedef struct _KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_HANDLE                            NotifyHandle;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  LIST_ENTRY                            NotifyEntry;
} KEYBOARD_CONSOLE_IN_EX_NOTIFY;

#define USB_NS_KEY_SIGNATURE  SIGNATURE_32 ('u', 'n', 's', 'k')

typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;

  //
  // The number of EFI_NS_KEY_MODIFIER children definitions
  //
  UINTN                         KeyCount;

  //
  // NsKey[0] : Non-spacing key
  // NsKey[1] ~ NsKey[KeyCount] : Physical keys
  //
  EFI_KEY_DESCRIPTOR            *NsKey;
} USB_NS_KEY;

#define USB_NS_KEY_FORM_FROM_LINK(a)  CR (a, USB_NS_KEY, Link, USB_NS_KEY_SIGNATURE)

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_EVENT                     DelayedRecoveryEvent;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL SimpleInput;       
  
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL SimpleInputEx;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   IntEndpointDescriptor;

  USB_KB_BUFFER                 KeyboardBuffer;
  BOOLEAN                       CtrlOn;
  BOOLEAN                       AltOn;
  BOOLEAN                       ShiftOn;
  BOOLEAN                       NumLockOn;
  BOOLEAN                       CapsOn;
  BOOLEAN                       ScrollOn;
  UINT8                         LastKeyCodeArray[8];
  UINT8                         CurKeyCode;
  UINT8                         RepeatKey;
  EFI_EVENT                     RepeatTimer;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;

  //
  // Buffer storing EFI_INPUT_KEY
  //
  BOOLEAN                       ExtendedKeyboard;
  EFI_INPUT_KEY                 KeyBuf[KEYBOARD_BUFFER_MAX_COUNT];
  UINT32                        KeyBufStartPos;
  UINT32                        KeyBufEndPos;
  UINT32                        KeyBufCount;
  //
  // The current available key stroke for SimpleTextIn.ReadKeyStroke
  //
  EFI_INPUT_KEY                 Key;

  BOOLEAN                       LeftCtrlOn;
  BOOLEAN                       LeftAltOn;
  BOOLEAN                       LeftShiftOn;
  BOOLEAN                       LeftLogoOn;
  BOOLEAN                       RightCtrlOn;
  BOOLEAN                       RightAltOn;
  BOOLEAN                       RightShiftOn;
  BOOLEAN                       RightLogoOn;  
  BOOLEAN                       MenuKeyOn;
  BOOLEAN                       SysReqOn;
  BOOLEAN                       AltGrOn;

  EFI_KEY_STATE                 KeyState;
  //
  // Notification function list
  //
  LIST_ENTRY                    NotifyList;

  //
  // Non-spacing key list
  //
  LIST_ENTRY                    NsKeyList;
  USB_NS_KEY                    *CurrentNsKey;
  EFI_KEY_DESCRIPTOR            *KeyConvertionTable;
  EFI_EVENT                     KeyboardLayoutEvent;
  EFI_EVENT                     TimerEvent;
} USB_KB_DEV;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gLegacyFreeDriverBinding;

extern EFI_COMPONENT_NAME2_PROTOCOL gLegacyFreeComponentName2;

extern EFI_COMPONENT_NAME_PROTOCOL  gLegacyFreeComponentName;

extern EFI_GUID                     gEfiLegacyFreeDriverGuid;


#define USB_KB_DEV_FROM_THIS(a) \
    CR(a, USB_KB_DEV, SimpleInput, USB_KB_DEV_SIGNATURE)
#define TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS(a) \
    CR(a, USB_KB_DEV, SimpleInputEx, USB_KB_DEV_SIGNATURE)


#define MOD_CONTROL_L           0x01
#define MOD_CONTROL_R           0x10
#define MOD_SHIFT_L             0x02
#define MOD_SHIFT_R             0x20
#define MOD_ALT_L               0x04
#define MOD_ALT_R               0x40
#define MOD_WIN_L               0x08
#define MOD_WIN_R               0x80

typedef struct {
  UINT8 Mask;
  UINT8 Key;
} KB_MODIFIER;

#define USB_KEYCODE_MAX_MAKE      0x7E

#define USBKBD_VALID_KEYCODE(Key) ((UINT8) (Key) > 3)

typedef struct {
  UINT8 NumLock : 1;
  UINT8 CapsLock : 1;
  UINT8 ScrollLock : 1;
  UINT8 Resrvd : 5;
} LED_MAP;

#define USB_KEYBOARD_LAYOUT_PACKAGE_GUID \
  { \
    0xc0f3b43, 0x44de, 0x4907, { 0xb4, 0x78, 0x22, 0x5f, 0x6f, 0x62, 0x89, 0xdc } \
  }

#define USB_KEYBOARD_LAYOUT_KEY_GUID \
  { \
    0x3a4d7a7c, 0x18a, 0x4b42, { 0x81, 0xb3, 0xdc, 0x10, 0xe3, 0xb5, 0x91, 0xbd } \
  }

#define USB_KEYBOARD_KEY_COUNT            104

#define USB_KEYBOARD_LANGUAGE_STR_LEN     5         // RFC4646 Language Code: "en-US"
#define USB_KEYBOARD_DESCRIPTION_STR_LEN  (16 + 1)  // Description: "English Keyboard"

//
// According to Universal Serial Bus HID Usage Tables document ver 1.12,
// a Boot Keyboard should support the keycode range from 0x0 to 0x65 and 0xE0 to 0xE7.
// 0xE0 to 0xE7 are for modifier keys, and 0x0 to 0x3 are reserved for typical
// keyboard status or keyboard errors.
// So the number of valid non-modifier USB keycodes is 0x62, and the number of
// valid keycodes is 0x6A.
//
#define NUMBER_OF_VALID_NON_MODIFIER_USB_KEYCODE      0x62
#define NUMBER_OF_VALID_USB_KEYCODE                   0x6A



#pragma pack (1)
typedef struct {
  //
  // This 4-bytes total array length is required by PreparePackageList()
  //
  UINT32                 Length;

  //
  // Keyboard Layout package definition
  //
  EFI_HII_PACKAGE_HEADER PackageHeader;
  UINT16                 LayoutCount;

  //
  // EFI_HII_KEYBOARD_LAYOUT
  //
  UINT16                 LayoutLength;
  EFI_GUID               Guid;
  UINT32                 LayoutDescriptorStringOffset;
  UINT8                  DescriptorCount;
  EFI_KEY_DESCRIPTOR     KeyDescriptor[USB_KEYBOARD_KEY_COUNT];
  UINT16                 DescriptionCount;
  CHAR16                 Language[USB_KEYBOARD_LANGUAGE_STR_LEN];
  CHAR16                 Space;
  CHAR16                 DescriptionString[USB_KEYBOARD_DESCRIPTION_STR_LEN];
} USB_KEYBOARD_LAYOUT_PACK_BIN;
#pragma pack()


//
// Simple Text Input Ex protocol functions
//

//
// Simple Text Input Ex protocol functions
//
/**
  Resets the input device hardware.

  The Reset() function resets the input device hardware. As part
  of initialization process, the firmware/device will make a quick
  but reasonable attempt to verify that the device is functioning.
  If the ExtendedVerification flag is TRUE the firmware may take
  an extended amount of time to verify the device is operating on
  reset. Otherwise the reset operation is to occur as quickly as
  possible. The hardware verification process is not defined by
  this specification and is left up to the platform firmware or
  driver to implement.

  @param This                 A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param ExtendedVerification Indicates that the driver may perform a more exhaustive
                              verification operation of the device during reset.

  @retval EFI_SUCCESS         The device was reset.
  @retval EFI_DEVICE_ERROR    The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Reads the next keystroke from the input device.

  @param  This                   Protocol instance pointer.
  @param  KeyData                A pointer to a buffer that is filled in with the keystroke
                                 state data for the key that was pressed.

  @retval EFI_SUCCESS            The keystroke information was returned.
  @retval EFI_NOT_READY          There was no keystroke data available.
  @retval EFI_DEVICE_ERROR       The keystroke information was not returned due to
                                 hardware errors.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyToggleState          A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                  state for the input device.

  @retval EFI_SUCCESS             The device state was set appropriately.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly and could
                                  not have the setting adjusted.
  @retval EFI_UNSUPPORTED         The device does not support the ability to have its state set.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with the keystroke
                                      information data for the key that was pressed.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necesssary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle or KeyNotificationFunction is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                         *NotifyHandle
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid
  @retval EFI_NOT_FOUND            Cannot find the matching entry in database.

**/
EFI_STATUS
EFIAPI
USBKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  );

#endif
