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
  UsbPolicy.h
  
Abstract:

--*/

#ifndef _USB_POLICY_H_
#define _USB_POLICY_H_

typedef struct _EFI_USB_POLICY_PROTOCOL    EFI_USB_POLICY_PROTOCOL;

#define USB_POLICY_GUID \
  {\
    0xf617b358, 0x12cf, 0x414a, 0xa0, 0x69, 0x60, 0x67, 0x7b, 0xda, 0x13, 0xb4\
  }
  
#define TIANO_CODE_BASE           0x00
#define ICBD_CODE_BASE            0x01

#define ATUO_TYPE                 0x00
#define FDD_TYPE                  0x01
#define HDD_TYPE                  0x02
#define ZIP_TYPE                  0x03
#define CDROM_TYPE                0x04
#define SIZE_TYPE                 0x05

#define HIGH_SPEED                0x00
#define FULL_SPEED                0x01
#define SUPER_SPEED               0x02

#define LEGACY_KB_EN              0x01
#define LEGACY_KB_DIS             0x00
#define LEGACY_MS_EN              0x01
#define LEGACY_MS_DIS             0x00
#define LEGACY_USB_EN             0x00
#define LEGACY_USB_DIS            0x01
#define LEGACY_FREE_SUPP          0x01
#define LEGACY_FREE_UN_SUPP       0x00
#define LEGACY_PERIOD_SUPP        0x01
#define LEGACY_PERIOD_UN_SUPP     0x00
#define LEGACY_XHC_SUPP           0x01
#define LEGACY_XHC_UN_SUPP        0x00
#define USB_SUPP                  0x01

#define LEGACY_USB_TIME_TUE_ENABLE       0x01
#define LEGACY_USB_TIME_TUE_DISABLE      0x00
#define USB_HAVE_HUB_INTERNEL            0x01
#define USB_NOT_HAVE_HUB_INTERNEL        0x00

#define USB_POLICY_PROTOCOL_REVISION_1 1
#define USB_POLICY_PROTOCOL_REVISION_2 2

#define GET_USB_CFG (UsbCfg);\
 do {\
   UINT16                EBdaSeg;\
   EBdaSeg = *(UINT16 *)(UINTN)0x40E;\
   UsbCfg  = (USB_CFG *)(UINTN)(((UINT32)EBdaSeg << 4) + 0x80);\
 } while(0);

#pragma    pack(1)
typedef struct {
  UINT8   HasUSBKeyboard:1;
  UINT8   HasUSBMouse:1;
  UINT8   LegacyFreeSupport:1;
  UINT8   UsbOperationMode:2;
  UINT8   LegacyKBEnable:1;
  UINT8   LegacyMSEnable:1;
  UINT8   USBPeriodSupport:1;
} USB_DEVICE_INFOR;

typedef struct {
  UINT8               Codebase;
  UINT8               USBHDDForceType;
  UINT8               Configurated;
  UINT8               LpcAcpiBase;
  UINT8               AcpiTimerReg;
  UINT8               UsbSupport;
  UINT8               LegacyUsbEnable;
  USB_DEVICE_INFOR    UsbDeviceInfor;
  UINT16              UsbEmulationSize;
  UINT8               XhcSupport;
  UINT8               Reserved2[0x05];
} USB_CFG;
#pragma pack()

typedef struct _EFI_USB_POLICY_PROTOCOL{
  UINT8   Version;
  UINT8   UsbMassStorageEmulationType;  // 1: FDD_Type; 2: HDD_Type; other:Auto_Type*
  UINT8   UsbOperationMode;             // 0: High_Speed; 1: Full_Speed;
  UINT8   LegacyKBEnable;               // 0: Disabled; 1: Enabled*
  UINT8   LegacyMSEnable;               // 0: Disabled; 1: Enabled*
  UINT8   USBPeriodSupport;             // 0;unsupport; 1:support
  UINT8   LegacyUsbEnable;              // 1: Disabled; 0: Enabled*
  UINT8   LegacyFreeSupport;            // 0:Unsupport ; 1:Support
  UINT8   CodeBase;
  UINT8   LpcAcpiBase;                  // 40h(default)
  UINT8   AcpiTimerReg;
  UINT8   UsbTimeTue;
  UINT8   InternelHubExist;             // 1: Host have internel hub on board; 0: No internel hub on board
  UINT8   EnumWaitPortStableStall;      // Value for wait port stable when enum a new dev.
  UINT16  UsbEmulationSize;             // Mbytes. 
  UINT8   UsbZipEmulationType;
  UINT8   XhcSupport;
  UINT8   UsbSupport;
  UINT8   Reserved[1];                  // Reserved fields for future expansion w/o protocol change
}EFI_USB_POLICY_PROTOCOL ;

extern EFI_GUID gUsbPolicyGuid;
#endif                 
