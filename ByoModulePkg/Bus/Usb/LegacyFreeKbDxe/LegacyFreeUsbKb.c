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
  LegacyFreeUsbKb.c

Abstract:


Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/
#include "PiDxe.h"
#include <Protocol/UsbPolicy.h>

#include "LegacyFreeUsbKb.h"

EFI_GUID  mUsbKeyboardLayoutPackageGuid = USB_KEYBOARD_LAYOUT_PACKAGE_GUID;
EFI_GUID  mUsbKeyboardLayoutKeyGuid = USB_KEYBOARD_LAYOUT_KEY_GUID;

USB_KEYBOARD_LAYOUT_PACK_BIN  mUsbKeyboardLayoutBin = {
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN),   // Binary size

  //
  // EFI_HII_PACKAGE_HEADER
  //
  {
    sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_KEYBOARD_LAYOUT
  },
  1,  // LayoutCount
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32) - sizeof (EFI_HII_PACKAGE_HEADER) - sizeof (UINT16), // LayoutLength
  USB_KEYBOARD_LAYOUT_KEY_GUID,  // KeyGuid
  sizeof (UINT16) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (UINT8) + (USB_KEYBOARD_KEY_COUNT * sizeof (EFI_KEY_DESCRIPTOR)), // LayoutDescriptorStringOffset
  USB_KEYBOARD_KEY_COUNT, // DescriptorCount
  {
    //
    // EFI_KEY_DESCRIPTOR (total number is USB_KEYBOARD_KEY_COUNT)
    //
    {EfiKeyC1,         'a',      'A',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB5,         'b',      'B',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB3,         'c',      'C',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC3,         'd',      'D',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD3,         'e',      'E',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC4,         'f',      'F',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC5,         'g',      'G',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC6,         'h',      'H',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD8,         'i',      'I',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC7,         'j',      'J',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC8,         'k',      'K',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC9,         'l',      'L',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB7,         'm',      'M',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB6,         'n',      'N',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD9,         'o',      'O',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD10,        'p',      'P',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD1,         'q',      'Q',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD4,         'r',      'R',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC2,         's',      'S',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD5,         't',      'T',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD7,         'u',      'U',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB4,         'v',      'V',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD2,         'w',      'W',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB2,         'x',      'X',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD6,         'y',      'Y',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB1,         'z',      'Z',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyE1,         '1',      '!',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE2,         '2',      '@',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE3,         '3',      '#',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE4,         '4',      '$',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE5,         '5',      '%',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE6,         '6',      '^',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE7,         '7',      '&',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE8,         '8',      '*',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE9,         '9',      '(',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE10,        '0',      ')',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyEsc,        0x1b,     0x1b, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyBackSpace,  0x08,     0x08, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyTab,        0x09,     0x09, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeySpaceBar,   ' ',      ' ',  0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyE11,        '-',      '_',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE12,        '=',      '+',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD11,        '[',      '{',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD12,        ']',      '}',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD13,        '\\',     '|',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC10,        ';',      ':',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC11,        '\'',     '"',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE0,         '`',      '~',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB8,         ',',      '<',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB9,         '.',      '>',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB10,        '/',      '?',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyCapsLock,   0x00,     0x00, 0, 0,  EFI_CAPS_LOCK_MODIFIER,            0},
    {EfiKeyF1,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ONE_MODIFIER,     0},
    {EfiKeyF2,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWO_MODIFIER,     0},
    {EfiKeyF3,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_THREE_MODIFIER,   0},
    {EfiKeyF4,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FOUR_MODIFIER,    0},
    {EfiKeyF5,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FIVE_MODIFIER,    0},
    {EfiKeyF6,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SIX_MODIFIER,     0},
    {EfiKeyF7,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SEVEN_MODIFIER,   0},
    {EfiKeyF8,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_EIGHT_MODIFIER,   0},
    {EfiKeyF9,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_NINE_MODIFIER,    0},
    {EfiKeyF10,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TEN_MODIFIER,     0},
    {EfiKeyF11,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ELEVEN_MODIFIER,  0},
    {EfiKeyF12,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWELVE_MODIFIER,  0},
    {EfiKeyPrint,      0x00,     0x00, 0, 0,  EFI_PRINT_MODIFIER,                0},
    {EfiKeySLck,       0x00,     0x00, 0, 0,  EFI_SCROLL_LOCK_MODIFIER,          0},
    {EfiKeyPause,      0x00,     0x00, 0, 0,  EFI_PAUSE_MODIFIER,                0},
    {EfiKeyIns,        0x00,     0x00, 0, 0,  EFI_INSERT_MODIFIER,               0},
    {EfiKeyHome,       0x00,     0x00, 0, 0,  EFI_HOME_MODIFIER,                 0},
    {EfiKeyPgUp,       0x00,     0x00, 0, 0,  EFI_PAGE_UP_MODIFIER,              0},
    {EfiKeyDel,        0x00,     0x00, 0, 0,  EFI_DELETE_MODIFIER,               0},
    {EfiKeyEnd,        0x00,     0x00, 0, 0,  EFI_END_MODIFIER,                  0},
    {EfiKeyPgDn,       0x00,     0x00, 0, 0,  EFI_PAGE_DOWN_MODIFIER,            0},
    {EfiKeyRightArrow, 0x00,     0x00, 0, 0,  EFI_RIGHT_ARROW_MODIFIER,          0},
    {EfiKeyLeftArrow,  0x00,     0x00, 0, 0,  EFI_LEFT_ARROW_MODIFIER,           0},
    {EfiKeyDownArrow,  0x00,     0x00, 0, 0,  EFI_DOWN_ARROW_MODIFIER,           0},
    {EfiKeyUpArrow,    0x00,     0x00, 0, 0,  EFI_UP_ARROW_MODIFIER,             0},
    {EfiKeyNLck,       0x00,     0x00, 0, 0,  EFI_NUM_LOCK_MODIFIER,             0},
    {EfiKeySlash,      '/',      '/',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyAsterisk,   '*',      '*',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyMinus,      '-',      '-',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyPlus,       '+',      '+',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyOne,        '1',      '1',  0, 0,  EFI_END_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyTwo,        '2',      '2',  0, 0,  EFI_DOWN_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyThree,      '3',      '3',  0, 0,  EFI_PAGE_DOWN_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFour,       '4',      '4',  0, 0,  EFI_LEFT_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFive,       '5',      '5',  0, 0,  EFI_NULL_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySix,        '6',      '6',  0, 0,  EFI_RIGHT_ARROW_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySeven,      '7',      '7',  0, 0,  EFI_HOME_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyEight,      '8',      '8',  0, 0,  EFI_UP_ARROW_MODIFIER,    EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyNine,       '9',      '9',  0, 0,  EFI_PAGE_UP_MODIFIER,     EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyZero,       '0',      '0',  0, 0,  EFI_INSERT_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyPeriod,     '.',      '.',  0, 0,  EFI_DELETE_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyA4,         0x00,     0x00, 0, 0,  EFI_MENU_MODIFIER,            0},
    {EfiKeyLCtrl,      0,        0,    0, 0,  EFI_LEFT_CONTROL_MODIFIER,    0},
    {EfiKeyLShift,     0,        0,    0, 0,  EFI_LEFT_SHIFT_MODIFIER,      0},
    {EfiKeyLAlt,       0,        0,    0, 0,  EFI_LEFT_ALT_MODIFIER,        0},
    {EfiKeyA0,         0,        0,    0, 0,  EFI_LEFT_LOGO_MODIFIER,       0},
    {EfiKeyRCtrl,      0,        0,    0, 0,  EFI_RIGHT_CONTROL_MODIFIER,   0},
    {EfiKeyRShift,     0,        0,    0, 0,  EFI_RIGHT_SHIFT_MODIFIER,     0},
    {EfiKeyA2,         0,        0,    0, 0,  EFI_RIGHT_ALT_MODIFIER,       0},
    {EfiKeyA3,         0,        0,    0, 0,  EFI_RIGHT_LOGO_MODIFIER,      0},
  },
  1,                          // DescriptionCount
  {'e', 'n', '-', 'U', 'S'},  // RFC4646 language code
  ' ',                        // Space
  {'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', '\0'}, // DescriptionString[]
};

//
// EFI_KEY to USB Keycode conversion table
// EFI_KEY is defined in UEFI spec.
// USB Keycode is defined in USB HID Firmware spec.
//
UINT8 EfiKeyToUsbKeyCodeConvertionTable[] = {
  0xe0,  //  EfiKeyLCtrl
  0xe3,  //  EfiKeyA0
  0xe2,  //  EfiKeyLAlt
  0x2c,  //  EfiKeySpaceBar
  0xe6,  //  EfiKeyA2
  0xe7,  //  EfiKeyA3
  0x65,  //  EfiKeyA4
  0xe4,  //  EfiKeyRCtrl
  0x50,  //  EfiKeyLeftArrow
  0x51,  //  EfiKeyDownArrow
  0x4F,  //  EfiKeyRightArrow
  0x62,  //  EfiKeyZero
  0x63,  //  EfiKeyPeriod
  0x28,  //  EfiKeyEnter
  0xe1,  //  EfiKeyLShift
  0x64,  //  EfiKeyB0
  0x1D,  //  EfiKeyB1
  0x1B,  //  EfiKeyB2
  0x06,  //  EfiKeyB3
  0x19,  //  EfiKeyB4
  0x05,  //  EfiKeyB5
  0x11,  //  EfiKeyB6
  0x10,  //  EfiKeyB7
  0x36,  //  EfiKeyB8
  0x37,  //  EfiKeyB9
  0x38,  //  EfiKeyB10
  0xe5,  //  EfiKeyRShift
  0x52,  //  EfiKeyUpArrow
  0x59,  //  EfiKeyOne
  0x5A,  //  EfiKeyTwo
  0x5B,  //  EfiKeyThree
  0x39,  //  EfiKeyCapsLock
  0x04,  //  EfiKeyC1
  0x16,  //  EfiKeyC2
  0x07,  //  EfiKeyC3
  0x09,  //  EfiKeyC4
  0x0A,  //  EfiKeyC5
  0x0B,  //  EfiKeyC6
  0x0D,  //  EfiKeyC7
  0x0E,  //  EfiKeyC8
  0x0F,  //  EfiKeyC9
  0x33,  //  EfiKeyC10
  0x34,  //  EfiKeyC11
  0x32,  //  EfiKeyC12
  0x5C,  //  EfiKeyFour
  0x5D,  //  EfiKeyFive
  0x5E,  //  EfiKeySix
  0x57,  //  EfiKeyPlus
  0x2B,  //  EfiKeyTab
  0x14,  //  EfiKeyD1
  0x1A,  //  EfiKeyD2
  0x08,  //  EfiKeyD3
  0x15,  //  EfiKeyD4
  0x17,  //  EfiKeyD5
  0x1C,  //  EfiKeyD6
  0x18,  //  EfiKeyD7
  0x0C,  //  EfiKeyD8
  0x12,  //  EfiKeyD9
  0x13,  //  EfiKeyD10
  0x2F,  //  EfiKeyD11
  0x30,  //  EfiKeyD12
  0x31,  //  EfiKeyD13
  0x4C,  //  EfiKeyDel
  0x4D,  //  EfiKeyEnd
  0x4E,  //  EfiKeyPgDn
  0x5F,  //  EfiKeySeven
  0x60,  //  EfiKeyEight
  0x61,  //  EfiKeyNine
  0x35,  //  EfiKeyE0
  0x1E,  //  EfiKeyE1
  0x1F,  //  EfiKeyE2
  0x20,  //  EfiKeyE3
  0x21,  //  EfiKeyE4
  0x22,  //  EfiKeyE5
  0x23,  //  EfiKeyE6
  0x24,  //  EfiKeyE7
  0x25,  //  EfiKeyE8
  0x26,  //  EfiKeyE9
  0x27,  //  EfiKeyE10
  0x2D,  //  EfiKeyE11
  0x2E,  //  EfiKeyE12
  0x2A,  //  EfiKeyBackSpace
  0x49,  //  EfiKeyIns
  0x4A,  //  EfiKeyHome
  0x4B,  //  EfiKeyPgUp
  0x53,  //  EfiKeyNLck
  0x54,  //  EfiKeySlash
  0x55,  //  EfiKeyAsterisk
  0x56,  //  EfiKeyMinus
  0x29,  //  EfiKeyEsc
  0x3A,  //  EfiKeyF1
  0x3B,  //  EfiKeyF2
  0x3C,  //  EfiKeyF3
  0x3D,  //  EfiKeyF4
  0x3E,  //  EfiKeyF5
  0x3F,  //  EfiKeyF6
  0x40,  //  EfiKeyF7
  0x41,  //  EfiKeyF8
  0x42,  //  EfiKeyF9
  0x43,  //  EfiKeyF10
  0x44,  //  EfiKeyF11
  0x45,  //  EfiKeyF12
  0x46,  //  EfiKeyPrint
  0x47,  //  EfiKeySLck
  0x48   //  EfiKeyPause
};


EFI_GUID  gEfiLegacyFreeDriverGuid = {
  0xa05f5f78, 0xfb3, 0x4d10, 0x90, 0x90, 0xac, 0x4, 0x6e, 0xeb, 0x7c, 0x3c
};
//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
LegacyFreeDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

EFI_STATUS
EFIAPI
LegacyFreeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
LegacyFreeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
LegacyFreeDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

BOOLEAN
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
  );

EFI_STATUS
EFIAPI
USBKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                          ExtendedVerification
  );
  


EFI_STATUS
EFIAPI
USBKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                 *Key
  );


VOID
EFIAPI
USBKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );


EFI_STATUS
KbdFreeNotifyList (
  IN OUT LIST_ENTRY       *ListHead
  );



BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );


VOID
EFIAPI
ReleaseKeyboardLayoutResources (
  IN OUT USB_KB_DEV              *UsbKeyboardDevice
  );

VOID
EFIAPI
BiosKeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

EFI_GUID gSimpleTextInExNotifyGuid = { \
  0x856f2def, 0x4e93, 0x4d6b, 0x94, 0xce, 0x1c, 0xfe, 0x47, 0x1, 0x3e, 0xa5 \
};

//
// USB Keyboard Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gLegacyFreeDriverBinding = {
  LegacyFreeDriverBindingSupported,
  LegacyFreeDriverBindingStart,
  LegacyFreeDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  Find Key Descriptor in Key Convertion Table given its USB keycode.

  @param  UsbKeyboardDevice   The USB_KB_DEV instance.
  @param  KeyCode             USB Keycode.

  @return The Key Descriptor in Key Convertion Table.
          NULL means not found.

**/
EFI_KEY_DESCRIPTOR *
EFIAPI
GetKeyDescriptor (
  IN USB_KB_DEV        *UsbKeyboardDevice,
  IN UINT8             KeyCode
  )
{
  UINT8  Index;

  //
  // Make sure KeyCode is in the range of [0x4, 0x65] or [0xe0, 0xe7]
  //
  if ((!USBKBD_VALID_KEYCODE (KeyCode)) || ((KeyCode > 0x65) && (KeyCode < 0xe0)) || (KeyCode > 0xe7)) {
    return NULL;
  }

  //
  // Calculate the index of Key Descriptor in Key Convertion Table
  //
  if (KeyCode <= 0x65) {
    Index = (UINT8) (KeyCode - 4);
  } else {
    Index = (UINT8) (KeyCode - 0xe0 + NUMBER_OF_VALID_NON_MODIFIER_USB_KEYCODE);
  }

  return &UsbKeyboardDevice->KeyConvertionTable[Index];
}


/**
  Initialize Key Convention Table by using default keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

  @retval EFI_SUCCESS          The default keyboard layout was installed successfully
  @retval Others               Failure to install default keyboard layout.
**/
EFI_STATUS
EFIAPI
InstallDefaultKeyboardLayout (
   IN OUT USB_KB_DEV           *UsbKeyboardDevice
  )
{
  EFI_STATUS                   Status;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Locate Hii database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Keyboard Layout package to HII database
  //
  HiiHandle = HiiAddPackages (
                &mUsbKeyboardLayoutPackageGuid,
                UsbKeyboardDevice->ControllerHandle,
                &mUsbKeyboardLayoutBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set current keyboard layout
  //
  Status = HiiDatabase->SetKeyboardLayout (HiiDatabase, &mUsbKeyboardLayoutKeyGuid);

  return Status;
}

/**
  Get current keyboard layout from HII database.

  @return Pointer to HII Keyboard Layout.
          NULL means failure occurred while trying to get keyboard layout.

**/
EFI_HII_KEYBOARD_LAYOUT *
EFIAPI
GetCurrentKeyboardLayout (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_HII_DATABASE_PROTOCOL *HiiDatabase;
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  UINT16                    Length;

  //
  // Locate HII Database Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get current keyboard layout from HII database
  //
  Length = 0;
  KeyboardLayout = NULL;
  Status = HiiDatabase->GetKeyboardLayout (
                          HiiDatabase,
                          NULL,
                          &Length,
                          KeyboardLayout
                          );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    KeyboardLayout = AllocatePool (Length);
    ASSERT (KeyboardLayout != NULL);

    Status = HiiDatabase->GetKeyboardLayout (
                            HiiDatabase,
                            NULL,
                            &Length,
                            KeyboardLayout
                            );
    if (EFI_ERROR (Status)) {
      FreePool (KeyboardLayout);
      KeyboardLayout = NULL;
    }
  }

  return KeyboardLayout;
}

/**
  The notification function for EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID.

  This function is registered to event of EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID
  group type, which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  It tries to get curent keyboard layout from HII database.

  @param  Event        Event being signaled.
  @param  Context      Points to USB_KB_DEV instance.

**/
VOID
EFIAPI
SetKeyboardLayoutEvent (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  USB_KB_DEV                *UsbKeyboardDevice;
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  EFI_KEY_DESCRIPTOR        TempKey;
  EFI_KEY_DESCRIPTOR        *KeyDescriptor;
  EFI_KEY_DESCRIPTOR        *TableEntry;
  EFI_KEY_DESCRIPTOR        *NsKey;
  USB_NS_KEY                *UsbNsKey;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     KeyCount;
  UINT8                     KeyCode;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;
  if (UsbKeyboardDevice->Signature != USB_KB_DEV_SIGNATURE) {
    return;
  }

  //
  // Try to get current keyboard layout from HII database
  //
  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout == NULL) {
    return;
  }

  //
  // Re-allocate resource for KeyConvertionTable
  //
  ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_USB_KEYCODE) * sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  //
  // Traverse the list of key descriptors following the header of EFI_HII_KEYBOARD_LAYOUT
  //
  KeyDescriptor = (EFI_KEY_DESCRIPTOR *) (((UINT8 *) KeyboardLayout) + sizeof (EFI_HII_KEYBOARD_LAYOUT));
  for (Index = 0; Index < KeyboardLayout->DescriptorCount; Index++) {
    //
    // Copy from HII keyboard layout package binary for alignment
    //
    CopyMem (&TempKey, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // Fill the key into KeyConvertionTable, whose index is calculated from USB keycode.
    //
    KeyCode = EfiKeyToUsbKeyCodeConvertionTable [(UINT8) (TempKey.Key)];
    TableEntry = GetKeyDescriptor (UsbKeyboardDevice, KeyCode);
    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // For non-spacing key, create the list with a non-spacing key followed by physical keys.
    //
    if (TempKey.Modifier == EFI_NS_KEY_MODIFIER) {
      UsbNsKey = AllocatePool (sizeof (USB_NS_KEY));
      ASSERT (UsbNsKey != NULL);

      //
      // Search for sequential children physical key definitions
      //
      KeyCount = 0;
      NsKey = KeyDescriptor + 1;
      for (Index2 = Index + 1; Index2 < KeyboardLayout->DescriptorCount; Index2++) {
        CopyMem (&TempKey, NsKey, sizeof (EFI_KEY_DESCRIPTOR));
        if (TempKey.Modifier == EFI_NS_KEY_DEPENDENCY_MODIFIER) {
          KeyCount++;
        } else {
          break;
        }
        NsKey++;
      }

      UsbNsKey->Signature = USB_NS_KEY_SIGNATURE;
      UsbNsKey->KeyCount = KeyCount;
      UsbNsKey->NsKey = AllocateCopyPool (
                          (KeyCount + 1) * sizeof (EFI_KEY_DESCRIPTOR),
                          KeyDescriptor
                          );
      InsertTailList (&UsbKeyboardDevice->NsKeyList, &UsbNsKey->Link);

      //
      // Skip over the child physical keys
      //
      Index += KeyCount;
      KeyDescriptor += KeyCount;
    }

    KeyDescriptor++;
  }

  //
  // There are two EfiKeyEnter, duplicate its key descriptor
  //
  TableEntry = GetKeyDescriptor (UsbKeyboardDevice, 0x58);
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, 0x28);
  CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

  FreePool (KeyboardLayout);
}

/**
  Destroy resources for keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
EFIAPI
ReleaseKeyboardLayoutResources (
  IN OUT USB_KB_DEV              *UsbKeyboardDevice
  )
{
  USB_NS_KEY      *UsbNsKey;
  LIST_ENTRY      *Link;

  if (UsbKeyboardDevice->KeyConvertionTable != NULL) {
    FreePool (UsbKeyboardDevice->KeyConvertionTable);
  }
  UsbKeyboardDevice->KeyConvertionTable = NULL;

  while (!IsListEmpty (&UsbKeyboardDevice->NsKeyList)) {
    Link = GetFirstNode (&UsbKeyboardDevice->NsKeyList);
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);
    RemoveEntryList (&UsbNsKey->Link);

    FreePool (UsbNsKey->NsKey);
    FreePool (UsbNsKey);
  }
}

/**
  Initialize USB keyboard layout.

  This function initializes Key Convertion Table for the USB keyboard device.
  It first tries to retrieve layout from HII database. If failed and default
  layout is enabled, then it just uses the default layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization succeeded.
  @retval EFI_NOT_READY          Keyboard layout cannot be retrieve from HII
                                 database, and default layout is disabled.
  @retval Other                  Fail to register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group.

**/
EFI_STATUS
EFIAPI
InitKeyboardLayout (
  OUT USB_KB_DEV   *UsbKeyboardDevice
  )
{
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  EFI_STATUS                Status;

  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_USB_KEYCODE) * sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  InitializeListHead (&UsbKeyboardDevice->NsKeyList);
  UsbKeyboardDevice->CurrentNsKey = NULL;
  UsbKeyboardDevice->KeyboardLayoutEvent = NULL;

  //
  // Register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group,
  // which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SetKeyboardLayoutEvent,
                  UsbKeyboardDevice,
                  &gEfiHiiKeyBoardLayoutGuid,
                  &UsbKeyboardDevice->KeyboardLayoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout != NULL) {
    //
    // If current keyboard layout is successfully retrieved from HII database,
    // force to initialize the keyboard layout.
    //
    gBS->SignalEvent (UsbKeyboardDevice->KeyboardLayoutEvent);
  } else {
    if (FeaturePcdGet (PcdDisableDefaultKeyboardLayoutInUsbKbDriver)) {
      //
      // If no keyboard layout can be retrieved from HII database, and default layout
      // is disabled, then return EFI_NOT_READY.
      //
      return EFI_NOT_READY;
    }
    //
    // If no keyboard layout can be retrieved from HII database, and default layout
    // is enabled, then load the default keyboard layout.
    //
    InstallDefaultKeyboardLayout (UsbKeyboardDevice);
  }
  
  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
LegacyFreeDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

  Routine Description:
    Driver Entry Point.

  Arguments:
    ImageHandle - EFI_HANDLE
    SystemTable - EFI_SYSTEM_TABLE
  Returns:
    EFI_STATUS

--*/
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gLegacyFreeDriverBinding,
             ImageHandle,
             &gLegacyFreeComponentName,
             &gLegacyFreeComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether USB keyboard driver supports this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
LegacyFreeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_USB_IO_PROTOCOL *UsbIo;
  EFI_STATUS          Status;

  //
  // Check if USB_IO protocol is attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                     (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  //
  // Use the USB I/O protocol interface to check whether the Controller is
  // the Keyboard controller that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUSBKeyboard (UsbIo)) {
     Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

EFI_STATUS
EFIAPI
UsbSetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  ASSERT (UsbIo != NULL);
  ASSERT (Report != NULL);

  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_SET_REQ_TYPE;
  Request.Request = EFI_USB_SET_REPORT_REQUEST;
  Request.Value   = (UINT16) ((ReportType << 8) | ReportId);
  Request.Index   = Interface;
  Request.Length  = ReportLen;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataOut,
                    3000,
                    Report,
                    ReportLen,
                    &Status
                    );

  return Result;
}

VOID
InitLedState (
  EFI_USB_IO_PROTOCOL           *UsbIo,
  USB_KB_DEV                    *UsbKeyboardDevice
)
{
  EFI_PS2_POLICY_PROTOCOL *Ps2Policy;
  LED_MAP                 Led;
  UINT8                   ReportId;
  PS2_BDA_LED_MAP         *BdaLed;

  //
  // Get Ps2 policy to set this
  //
  gBS->LocateProtocol (
        &gEfiPs2PolicyProtocolGuid,
        NULL,
        (VOID **) &Ps2Policy
        );
  if (Ps2Policy != NULL) {
    BdaLed = (PS2_BDA_LED_MAP *)((UINT8 *)(UINTN)BDA_LED_PTR);
    BdaLed->NumLock = 0;
    if ((Ps2Policy->KeyboardLight & EFI_KEYBOARD_NUMLOCK) == EFI_KEYBOARD_NUMLOCK) {
      BdaLed->NumLock = 1;
    }
    //
    // Set each field in Led map.
    //
    UsbKeyboardDevice->NumLockOn = BdaLed->NumLock;
    UsbKeyboardDevice->CapsOn    = BdaLed->CapsLock;
    UsbKeyboardDevice->ScrollOn  = BdaLed->ScrollLock;

    Led.NumLock    = (UINT8) UsbKeyboardDevice->NumLockOn;
    Led.CapsLock   = (UINT8) UsbKeyboardDevice->CapsOn;
    Led.ScrollLock = (UINT8) UsbKeyboardDevice->ScrollOn;
    Led.Resrvd     = 0;

    //
    // call Set Report Request to lighten the LED.
    //
    ReportId       = 0;

    UsbSetReportRequest (
        UsbKeyboardDevice->UsbIo,
        UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
        ReportId,
        HID_OUTPUT_REPORT,
        1,
        (CHAR8 *) &Led
    );
  }
}

/**
  Starts the keyboard device with this driver.

  This function produces Simple Text Input Protocol and Simple Text Input Ex Protocol,
  initializes the keyboard device, and submit Asynchronous Interrupt Transfer to manage
  this keyboard device.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @retval EFI_UNSUPPORTED        No interrupt endpoint can be found.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
LegacyFreeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_KB_DEV                    *UsbKeyboardDevice;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINT8                         EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  UINT8                         Index;
  BOOLEAN                       Found;
  EFI_TPL                       OldTpl;
  EFI_ISA_IO_PROTOCOL           *IsaIo;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;

  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiIsaIoProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NumberOfHandles; Index++) {
      Status = gBS->HandleProtocol (
                       HandleBuffer[Index],
                       &gEfiIsaIoProtocolGuid,
                       (VOID**) &IsaIo
                       );
      if (!EFI_ERROR (Status)) {
        if (IsaIo->ResourceList->Device.HID == EISA_PNP_ID (0x303)) {
          DEBUG((EFI_D_ERROR,"Has KBC, no need legacyFree support\n"));
          return EFI_SUCCESS;
        }
      }
    }
  }
  DEBUG((EFI_D_ERROR,"No KBC, need legacyFree support\n"));

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  (VOID **) &LegacyBios
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Open USB_IO Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit1;
  }

  UsbKeyboardDevice = AllocateZeroPool (sizeof (USB_KB_DEV));
  ASSERT (UsbKeyboardDevice != NULL);

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbKeyboardDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }


  //
  // Report that the USB keyboard is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE),
    UsbKeyboardDevice->DevicePath
    );

  //
  // This is pretty close to keyboard detection, so log progress
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Initialize UsbKeyboardDevice
  //
  UsbKeyboardDevice->UsbIo = UsbIo;

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbKeyboardDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbKeyboardDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find interrupt endpoint
  //
  Found = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {

    UsbIo->UsbGetEndpointDescriptor (
             UsbIo,
             Index,
             &EndpointDescriptor
             );

    if ((EndpointDescriptor.Attributes & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) {
      //
      // We only care interrupt endpoint here
      //
      CopyMem (&UsbKeyboardDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  UsbKeyboardDevice->Signature                  = USB_KB_DEV_SIGNATURE;
  UsbKeyboardDevice->SimpleInput.Reset          = USBKeyboardReset;
  UsbKeyboardDevice->SimpleInput.ReadKeyStroke  = USBKeyboardReadKeyStroke;
  UsbKeyboardDevice->LegacyBios                 = LegacyBios;
  UsbKeyboardDevice->ExtendedKeyboard           = TRUE;
  UsbKeyboardDevice->KeyBufStartPos             = 0;
  UsbKeyboardDevice->KeyBufEndPos               = 0;
  UsbKeyboardDevice->KeyBufCount                = 0;

  UsbKeyboardDevice->SimpleInputEx.Reset               = USBKeyboardResetEx;
  UsbKeyboardDevice->SimpleInputEx.ReadKeyStrokeEx     = USBKeyboardReadKeyStrokeEx;
  UsbKeyboardDevice->SimpleInputEx.SetState            = USBKeyboardSetState;
  UsbKeyboardDevice->SimpleInputEx.RegisterKeyNotify   = USBKeyboardRegisterKeyNotify;
  UsbKeyboardDevice->SimpleInputEx.UnregisterKeyNotify = USBKeyboardUnregisterKeyNotify;

  InitializeListHead (&UsbKeyboardDevice->NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx)
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInput.WaitForKey)
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Setup a periodic timer, used for reading keystrokes at a fixed interval
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  BiosKeyboardTimerHandler,
                  UsbKeyboardDevice,
                  &UsbKeyboardDevice->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status      = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  Status = gBS->SetTimer (
                  UsbKeyboardDevice->TimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    Status      = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // Install simple txt in protocol interface
  // for the usb keyboard device.
  // Usb keyboard is a hot plug device, and expected to work immediately
  // when plugging into system, so a HotPlugDeviceGuid is installed onto
  // the usb keyboard device handle, to distinguish it from other conventional
  // console devices.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &UsbKeyboardDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &UsbKeyboardDevice->SimpleInputEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbKeyboardDevice->ControllerHandle = Controller;
  Status = InitKeyboardLayout (UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
      Controller,
      &gEfiSimpleTextInProtocolGuid,
      &UsbKeyboardDevice->SimpleInput,
      &gEfiSimpleTextInputExProtocolGuid,
      &UsbKeyboardDevice->SimpleInputEx,
      NULL
      );
    goto ErrorExit;
  }


  //
  // Reset USB Keyboard Device
  //
  Status = UsbKeyboardDevice->SimpleInput.Reset (
                                            &UsbKeyboardDevice->SimpleInput,
                                            TRUE
                                            );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiSimpleTextInProtocolGuid,
           &UsbKeyboardDevice->SimpleInput,
           &gEfiSimpleTextInputExProtocolGuid,
           &UsbKeyboardDevice->SimpleInputEx,
           NULL
           );
    goto ErrorExit;
  }

  UsbKeyboardDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gLegacyFreeComponentName.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gLegacyFreeComponentName2.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    FALSE
    );
  
  gBS->RestoreTPL (OldTpl);

  InitLedState (UsbIo, UsbKeyboardDevice);

  return EFI_SUCCESS;

//
// Error handler
//
ErrorExit:
  if (UsbKeyboardDevice != NULL) {
    if (UsbKeyboardDevice->SimpleInput.WaitForKey != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    }
    if (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);
    }
    if (UsbKeyboardDevice->TimerEvent != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->TimerEvent);
    }		
    if (UsbKeyboardDevice->KeyboardLayoutEvent != NULL) {
      ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
      gBS->CloseEvent (UsbKeyboardDevice->KeyboardLayoutEvent);
    }
    FreePool (UsbKeyboardDevice);
    UsbKeyboardDevice = NULL;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

ErrorExit1:
  gBS->RestoreTPL (OldTpl);

  return Status;

}


/**
  Stop the USB keyboard device handled by this driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Text In Protocol or Simple Text In Ex Protocol
                                 is not installed on Controller.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
LegacyFreeDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleInput;
  USB_KB_DEV                  *UsbKeyboardDevice;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **) &SimpleInput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Get USB_KB_DEV instance.
  //
  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (SimpleInput);

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE),
    UsbKeyboardDevice->DevicePath
    );


  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &UsbKeyboardDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &UsbKeyboardDevice->SimpleInputEx,
                  NULL
                  );
  //
  // free all the resources.
  //
  if (UsbKeyboardDevice->RepeatTimer != NULL)
    gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL)
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
  if ((UsbKeyboardDevice->SimpleInput).WaitForKey != NULL) 
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
  if (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx != NULL)
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);
  if (UsbKeyboardDevice->TimerEvent != NULL)
    gBS->CloseEvent (UsbKeyboardDevice->TimerEvent);
	
  KbdFreeNotifyList (&UsbKeyboardDevice->NotifyList);

  ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
  if (UsbKeyboardDevice->KeyboardLayoutEvent != NULL)
    gBS->CloseEvent (UsbKeyboardDevice->KeyboardLayoutEvent);

  if (UsbKeyboardDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbKeyboardDevice->ControllerNameTable);
  }

  FreePool (UsbKeyboardDevice);

  return Status;
}

VOID
EFIAPI
BiosKeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
/*++

Routine Description:

  Timer event handler: read a series of key stroke from 8042
  and put them into memory key buffer.
  It is registered as running under EFI_TPL_NOTIFY

Arguments:

  Event - The timer event
  Context - A BIOS_KEYBOARD_DEV pointer

Returns:

--*/
{
  USB_KB_DEV              *UsbKeyboardDevice;
  UINT16                  ScanCode;
  UINT16                  KeyChar;
  EFI_IA32_REGISTER_SET   Regs;
  BOOLEAN                 CarryFlag;
  UINT8                   KbFlag1;  // 0040h:0017h - KEYBOARD - STATUS FLAGS 1
  UINT8                   KbFlag2;  // 0040h:0018h - KEYBOARD - STATUS FLAGS 2

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // if there is no key present, just return
  //
  if (UsbKeyboardDevice->ExtendedKeyboard) {
    Regs.H.AH = 0x11;
  } else {
    Regs.H.AH = 0x01;
  }

  CarryFlag = UsbKeyboardDevice->LegacyBios->Int86 (
                                               UsbKeyboardDevice->LegacyBios,
                                               0x16,
                                               &Regs
                                                );
  if (Regs.X.Flags.ZF) {
    return;
  }

  //
  // Read the key
  //
  if (UsbKeyboardDevice->ExtendedKeyboard) {
    Regs.H.AH = 0x10;
  } else {
    Regs.H.AH = 0x00;
  }

  UsbKeyboardDevice->LegacyBios->Int86 (
                                   UsbKeyboardDevice->LegacyBios,
                                   0x16,
                                   &Regs
                                   );

  ScanCode  = (CHAR16) Regs.H.AH;
  KeyChar   = (CHAR16) Regs.H.AL;

  UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
	
  //
  // Clear the CTRL and ALT BDA flag
  //
  KbFlag1 = *((UINT8 *) (UINTN) 0x417);  // read the STATUS FLAGS 1
  KbFlag2 = *((UINT8 *) (UINTN) 0x418); // read STATUS FLAGS 2

  //
  // Record toggle state
  //
  if ((KbFlag1 & KB_CAPS_LOCK_BIT) == KB_CAPS_LOCK_BIT) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  } else if ((KbFlag1 & KB_NUM_LOCK_BIT) == KB_NUM_LOCK_BIT) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  } else if ((KbFlag1 & KB_SCROLL_LOCK_BIT) == KB_SCROLL_LOCK_BIT) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }
  
  //
  // Record shift state
  // BUGBUG: Need add Menu key and Left/Right Logo key state in the future
  //
  if ((KbFlag1 & KB_ALT_PRESSED) == KB_ALT_PRESSED) {
    UsbKeyboardDevice->KeyState.KeyShiftState  |= ((KbFlag2 & KB_LEFT_ALT_PRESSED) == KB_LEFT_ALT_PRESSED) ? EFI_LEFT_ALT_PRESSED : EFI_RIGHT_ALT_PRESSED;
  } else if ((KbFlag1 & KB_CTRL_PRESSED) == KB_CTRL_PRESSED) {
    UsbKeyboardDevice->KeyState.KeyShiftState  |= ((KbFlag2 & KB_LEFT_CTRL_PRESSED) == KB_LEFT_CTRL_PRESSED) ? EFI_LEFT_CONTROL_PRESSED : EFI_RIGHT_CONTROL_PRESSED;
  } else  if ((KbFlag1 & KB_LEFT_SHIFT_PRESSED) == KB_LEFT_SHIFT_PRESSED) {
    UsbKeyboardDevice->KeyState.KeyShiftState  |= EFI_LEFT_SHIFT_PRESSED;
  } else  if ((KbFlag1 & KB_RIGHT_SHIFT_PRESSED) == KB_RIGHT_SHIFT_PRESSED) {
    UsbKeyboardDevice->KeyState.KeyShiftState  |= EFI_RIGHT_SHIFT_PRESSED;
  }

  //
  // Clear left alt and left ctrl BDA flag
  //
  KbFlag2 &= ~(KB_LEFT_ALT_PRESSED | KB_LEFT_CTRL_PRESSED);
  *((UINT8 *) (UINTN) 0x418) = KbFlag2;
  KbFlag1 &= ~0x0C;
  *((UINT8 *) (UINTN) 0x417) = KbFlag1;

  if (UsbKeyboardDevice->KeyBufCount < KEYBOARD_BUFFER_MAX_COUNT) {
    //
    // put the Key into the memory scancode buffer
    //
    UsbKeyboardDevice->KeyBuf[UsbKeyboardDevice->KeyBufEndPos].ScanCode    = ScanCode;
    UsbKeyboardDevice->KeyBuf[UsbKeyboardDevice->KeyBufEndPos].UnicodeChar = KeyChar;

    UsbKeyboardDevice->KeyBufCount++;
    UsbKeyboardDevice->KeyBufEndPos++;
    if (UsbKeyboardDevice->KeyBufEndPos >= KEYBOARD_BUFFER_MAX_COUNT) {
      UsbKeyboardDevice->KeyBufEndPos = 0;
    }
  }

  return ;
}

/**
  Check whether there is key pending in the keyboard buffer.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

  @retval EFI_SUCCESS          There is pending key to read.
  @retval EFI_NOT_READY        No pending key to read.

**/
EFI_STATUS
EFIAPI
BiosKeyboardCheckForKey (
  IN   USB_KB_DEV                             *UsbKeyboardDevice
  )
{
  //
  // If still has available key then just return.
  //
  if (UsbKeyboardDevice->Key.ScanCode == SCAN_NULL && 
      UsbKeyboardDevice->Key.UnicodeChar == 0x00) {

    //
    // Check whether the Key buffer is empty
    //
    if (UsbKeyboardDevice->KeyBufCount < 1) {
      return EFI_NOT_READY;
    }
    
    //
    // Retrieve and remove the values
    //
    UsbKeyboardDevice->Key.ScanCode    = UsbKeyboardDevice->KeyBuf[UsbKeyboardDevice->KeyBufStartPos].ScanCode;
    UsbKeyboardDevice->Key.UnicodeChar = UsbKeyboardDevice->KeyBuf[UsbKeyboardDevice->KeyBufStartPos].UnicodeChar;

    UsbKeyboardDevice->KeyBufStartPos++;
    if (UsbKeyboardDevice->KeyBufStartPos >= KEYBOARD_BUFFER_MAX_COUNT) {
      UsbKeyboardDevice->KeyBufStartPos = 0;
    }
    UsbKeyboardDevice->KeyBufCount--;
  }

  return EFI_SUCCESS;

}


#define TABLE_END 0x0

struct {
  UINT16  ScanCode;
  UINT16  EfiScanCode;
}
mConvertTable[] = {
  {
    0x47,
    SCAN_HOME
  },
  {
    0x48,
    SCAN_UP
  },
  {
    0x49,
    SCAN_PAGE_UP
  },
  {
    0x4b,
    SCAN_LEFT
  },
  {
    0x4d,
    SCAN_RIGHT
  },
  {
    0x4f,
    SCAN_END
  },
  {
    0x50,
    SCAN_DOWN
  },
  {
    0x51,
    SCAN_PAGE_DOWN
  },
  {
    0x52,
    SCAN_INSERT
  },
  {
    0x53,
    SCAN_DELETE
  },
  //
  // Function Keys are only valid if KeyChar == 0x00
  //  This function does not require KeyChar to be 0x00
  //
  {
    0x3b,
    SCAN_F1
  },
  {
    0x3c,
    SCAN_F2
  },
  {
    0x3d,
    SCAN_F3
  },
  {
    0x3e,
    SCAN_F4
  },
  {
    0x3f,
    SCAN_F5
  },
  {
    0x40,
    SCAN_F6
  },
  {
    0x41,
    SCAN_F7
  },
  {
    0x42,
    SCAN_F8
  },
  {
    0x43,
    SCAN_F9
  },
  {
    0x44,
    SCAN_F10
  },
  {
    0x85,
    SCAN_F11
  },
  {
    0x86,
    SCAN_F12
  },

  {
    TABLE_END,
    SCAN_NULL
  },
};

UINT16
ConvertToEFIScanCode (
  IN  CHAR16  KeyChar,
  IN  UINT16  ScanCode
  )
/*++

Routine Description:

  Convert unicode combined with scan code of key to the counterpart of EFIScancode of it.

Arguments:

  KeyChar             - Unicode of key.
  ScanCode            - Scan code of key.

Returns:

  SCAN_NULL           - No corresponding value in the EFI convert table is found for the key.
  other               - The value of EFI Scancode for the key.

--*/
{
  UINT16  EfiScanCode;
  UINT16  Index;

  if (KeyChar == CHAR_ESC) {
    EfiScanCode = SCAN_ESC;
  } else if (KeyChar == 0x00 || KeyChar == 0xe0) {
    //
    // Movement & Function Keys
    //
    for (Index = 0; mConvertTable[Index].ScanCode != TABLE_END; Index += 1) {
      if (ScanCode == mConvertTable[Index].ScanCode) {
        return mConvertTable[Index].EfiScanCode;
      }
    }
    //
    // Reach Table end, return default value
    //
    return SCAN_NULL;
  } else {
    return SCAN_NULL;
  }

  return EfiScanCode;
}

/**
  Uses USB I/O to check whether the device is a USB keyboard device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB keyboard device.
  @retval FALSE    Device is a not USB keyboard device.

**/
BOOLEAN
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the Default interface descriptor, currently we
  // assume it is interface 1
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (InterfaceDescriptor.InterfaceClass == CLASS_HID &&
      InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT &&
      InterfaceDescriptor.InterfaceProtocol == PROTOCOL_KEYBOARD
      ) {

    return TRUE;
  }

  return FALSE;
}

/**
  Internal function to read the next keystroke from the keyboard buffer.

  @param  UsbKeyboardDevice       USB keyboard's private structure.
  @param  KeyData                 A pointer to buffer to hold the keystroke
                                  data for the key that was pressed.

  @retval EFI_SUCCESS             The keystroke information was returned.
  @retval EFI_NOT_READY           There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR        The keystroke information was not returned due to
                                  hardware errors.
  @retval EFI_INVALID_PARAMETER   KeyData is NULL.
  @retval Others                  Fail to translate keycode into EFI_INPUT_KEY

**/
EFI_STATUS
USBKeyboardReadKeyStrokeWorker (
  IN  USB_KB_DEV                        *UsbKeyboardDevice,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  UINT16                                ScanCode;
  CHAR16                                UnicodeChar;
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;
  LIST_ENTRY                            *Link;
  LIST_ENTRY                            *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY         *CurrentNotify;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Use TimerEvent callback funciton to check whether there's any key pressed
  //

  //
  // Stall 1ms to give a chance to let other driver interrupt this routine for their timer event.
  // Csm will be used to check whether there is a key pending, but the csm will disable all
  // interrupt before switch to compatibility16, which mean all the efiCompatibility timer
  // event will stop work during the compatibility16. And If a caller recursivly invoke this function,
  // e.g. OS loader, other drivers which are driven by timer event will have a bad performance during this period,
  // e.g. usb keyboard driver.
  // Add a stall period can greatly increate other driver performance during the WaitForKey is recursivly invoked.
  // 1ms delay will make little impact to the thunk keyboard driver, and user can not feel the delay at all when input.
  //
  gBS->Stall (1000);
  BiosKeyboardTimerHandler (NULL, UsbKeyboardDevice);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  //
  // If there's no key, just return
  //
  Status = BiosKeyboardCheckForKey (UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_READY;
  }

  ScanCode    = UsbKeyboardDevice->Key.ScanCode;
  UnicodeChar = UsbKeyboardDevice->Key.UnicodeChar;

  UsbKeyboardDevice->Key.ScanCode    = SCAN_NULL;
  UsbKeyboardDevice->Key.UnicodeChar = 0x0000;

  //
  // Output EFI input key and shift/toggle state
  //
  if (UnicodeChar == CHAR_NULL || UnicodeChar == CHAR_SCANCODE || UnicodeChar == CHAR_ESC) {
    KeyData->Key.ScanCode     = ConvertToEFIScanCode (UnicodeChar, ScanCode);
    KeyData->Key.UnicodeChar  = CHAR_NULL;
  } else {
    KeyData->Key.ScanCode     = SCAN_NULL;
    KeyData->Key.UnicodeChar  = UnicodeChar;
  }


  //
  // Need not return associated shift state if a class of printable characters that
  // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
  //
  if (KeyData->Key.UnicodeChar >= 'A' && KeyData->Key.UnicodeChar <= 'Z') {
    UsbKeyboardDevice->KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
  }

  KeyData->KeyState.KeyShiftState  = UsbKeyboardDevice->KeyState.KeyShiftState;
  KeyData->KeyState.KeyToggleState = UsbKeyboardDevice->KeyState.KeyToggleState;
//UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
//UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;


  gBS->RestoreTPL (OldTpl);

  //
  // Invoke notification functions if the key is registered.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList);
       !IsNull (NotifyList, Link);
       Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      CurrentNotify->KeyNotificationFn (KeyData);
    }
  }
       
  return EFI_SUCCESS;
}


/**
  Reset the input device and optionally run diagnostics

  There are 2 types of reset for USB keyboard.
  For non-exhaustive reset, only keyboard buffer is cleared.
  For exhaustive reset, in addition to clearance of keyboard buffer, the hardware status
  is also re-initialized.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                       ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

/**
  Reads the next keystroke from the input device.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          The keystroke information was returned.
  @retval EFI_NOT_READY        There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR     The keystroke information was not returned due to
                               hardware errors.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                    *Key
  )
{
  USB_KB_DEV   *UsbKeyboardDevice;
  EFI_STATUS   Status;
  EFI_KEY_DATA KeyData;

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (This);

  Status = USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));

  return EFI_SUCCESS;

}


/**
  Event notification function registered for EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx
  and EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey.

  @param  Event        Event to be signaled when a key is pressed.
  @param  Context      Points to USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;
  EFI_TPL                 OldTpl;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  gBS->Stall (1000);
  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (!EFI_ERROR (BiosKeyboardCheckForKey (Context))) {
    gBS->SignalEvent (Event);
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

}

/**
  Free keyboard notify list.

  @param  NotifyList              The keyboard notify list to free.

  @retval EFI_SUCCESS             Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER   NotifyList is NULL.

**/
EFI_STATUS
EFIAPI
KbdFreeNotifyList (
  IN OUT LIST_ENTRY           *NotifyList
  )
{
  KEYBOARD_CONSOLE_IN_EX_NOTIFY *NotifyNode;
  LIST_ENTRY                    *Link;

  if (NotifyList == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (NotifyList)) {
    Link = GetFirstNode (NotifyList);
    NotifyNode = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    RemoveEntryList (Link);
    FreePool (NotifyNode);
  }
  
  return EFI_SUCCESS;
}

/**
  Check whether the pressed key matches a registered key or not.

  @param  RegsiteredData    A pointer to keystroke data for the key that was registered.
  @param  InputData         A pointer to keystroke data for the key that was pressed.

  @retval TRUE              Key pressed matches a registered key.
  @retval FLASE             Key pressed does not matches a registered key.

**/
BOOLEAN
EFIAPI
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )
{
  ASSERT (RegsiteredData != NULL && InputData != NULL);

  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar)) {
    return FALSE;
  }

  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means these state could be ignored.
  //
  if (RegsiteredData->KeyState.KeyShiftState != 0 &&
      RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState) {
    return FALSE;
  }
  if (RegsiteredData->KeyState.KeyToggleState != 0 &&
      RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState) {
    return FALSE;
  }

  return TRUE;
}

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
  )
{
  EFI_STATUS                Status;
  USB_KB_DEV                *UsbKeyboardDevice;
  EFI_TPL                   OldTpl;
  

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  Status = UsbKeyboardDevice->SimpleInput.Reset (&UsbKeyboardDevice->SimpleInput, ExtendedVerification);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

}

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
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  return USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, KeyData);
  
}

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
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  if (((UsbKeyboardDevice->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) ||
      ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Update the status light
  //
  UsbKeyboardDevice->ScrollOn   = FALSE;
  UsbKeyboardDevice->NumLockOn  = FALSE;
  UsbKeyboardDevice->CapsOn     = FALSE;

  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    UsbKeyboardDevice->ScrollOn = TRUE;
  }
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    UsbKeyboardDevice->NumLockOn = TRUE;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    UsbKeyboardDevice->CapsOn = TRUE;
  }

  UsbKeyboardDevice->KeyState.KeyToggleState = *KeyToggleState;

  return EFI_SUCCESS;
}

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with the keystroke
                                      information data for the key that was pressed.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necessary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle or KeyNotificationFunction is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                        *NotifyHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *NewNotify;
  LIST_ENTRY                        *Link;
  LIST_ENTRY                        *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;

  if (KeyData == NULL || NotifyHandle == NULL || KeyNotificationFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  
  for (Link = GetFirstNode (NotifyList);
       !IsNull (NotifyList, Link);
       Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify->NotifyHandle;
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Allocate resource to save the notification function
  //
  NewNotify = (KEYBOARD_CONSOLE_IN_EX_NOTIFY *) AllocateZeroPool (sizeof (KEYBOARD_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  NewNotify->NotifyHandle      = (EFI_HANDLE) NewNotify;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&UsbKeyboardDevice->NotifyList, &NewNotify->NotifyEntry);


  *NotifyHandle = NewNotify->NotifyHandle;

  return EFI_SUCCESS;
  
}

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid

**/
EFI_STATUS
EFIAPI
USBKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;
  LIST_ENTRY                        *Link;
  LIST_ENTRY                        *NotifyList;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }  

  if (((KEYBOARD_CONSOLE_IN_EX_NOTIFY *) NotificationHandle)->Signature != USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  } 
  
  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);
  
  //
  // Traverse notify list of USB keyboard and remove the entry of NotificationHandle.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList);
       !IsNull (NotifyList, Link);
       Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );       
    if (CurrentNotify->NotifyHandle == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);      

      FreePool (CurrentNotify);            
      return EFI_SUCCESS;
    }
  }

  //
  // Cannot find the matching entry in database.
  //
  return EFI_INVALID_PARAMETER;  
}

