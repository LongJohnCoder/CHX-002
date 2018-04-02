/*++
=========================================================================================
      NOTICE: Copyright (c) 2006 - 2010 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
=========================================================================================
Module Name:
  LegacyUsbInf.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  R01   09/02/2008  Ivan    Add Legacy USB Protocol.
  ----------------------------------------------------------------------------------------
--*/

#ifndef _LEGACY_USB_INF_PROTOCOL_H_
#define _LEGACY_USB_INF_PROTOCOL_H_

typedef struct _EFI_LEGACY_USB_INF_PROTOCOL   EFI_LEGACY_USB_INF_PROTOCOL;

#define EFI_LEGACY_USB_INF_PROTOCOL_GUID \
  { \
    0xc28ab52, 0xd2c8, 0x4009, 0xbb, 0x9, 0x98, 0xd5, 0xa2, 0x77, 0x32, 0xbd \
  }

#define SMMCALL_SMI_VALUE  0x88
#define USB_KEYBOARD_EVENT 0x8B

#define EFI_TPL_NOTIFY  TPL_NOTIFY

typedef struct _USB_SMMCALL_COMM {
  UINTN       CallId;
  EFI_STATUS  *Return;
  UINTN       Argc;
  VOID        *Argv[30];
} USB_SMMCALL_COMM;

typedef enum {
  __ID_StartLegacyUsb = 0,
  __ID_EndLegacyUsb,

  __ID_GetOhcInfo,
  __ID_OhciReset,
  __ID_OhciGetState,
  __ID_OhciSetState,
  __ID_OhciControlTransfer,
  __ID_OhciBulkTransfer,
  __ID_OhciAsyncInterruptTransfer,
  __ID_OhciSyncInterruptTransfer,
  __ID_OhciIsochronousTransfer,
  __ID_OhciAsyncIsochronousTransfer,
  __ID_OhciGetRootHubPortNumber,
  __ID_OhciGetRootHubPortStatus,
  __ID_OhciSetRootHubPortFeature,
  __ID_OhciClearRootHubPortFeature,
  
  __ID_Ohci2GetCapability,
  __ID_Ohci2Reset,
  __ID_Ohci2GetState,
  __ID_Ohci2SetState,
  __ID_Ohci2ControlTransfer,
  __ID_Ohci2BulkTransfer,
  __ID_Ohci2AsyncInterruptTransfer,
  __ID_Ohci2SyncInterruptTransfer,
  __ID_Ohci2IsochronousTransfer,
  __ID_Ohci2AsyncIsochronousTransfer,
  __ID_Ohci2GetRootHubPortStatus,
  __ID_Ohci2SetRootHubPortFeature,
  __ID_Ohci2ClearRootHubPortFeature,

  __ID_GetUhcInfo,
  __ID_UhciReset,
  __ID_UhciGetState,
  __ID_UhciSetState,
  __ID_UhciControlTransfer,
  __ID_UhciBulkTransfer,
  __ID_UhciAsyncInterruptTransfer,
  __ID_UhciSyncInterruptTransfer,
  __ID_UhciIsochronousTransfer,
  __ID_UhciAsyncIsochronousTransfer,
  __ID_UhciGetRootHubPortNumber,
  __ID_UhciGetRootHubPortStatus,
  __ID_UhciSetRootHubPortFeature,
  __ID_UhciClearRootHubPortFeature,

  __ID_Uhci2GetCapability,
  __ID_Uhci2Reset,
  __ID_Uhci2GetState,
  __ID_Uhci2SetState,
  __ID_Uhci2ControlTransfer,
  __ID_Uhci2BulkTransfer,
  __ID_Uhci2AsyncInterruptTransfer,
  __ID_Uhci2SyncInterruptTransfer,
  __ID_Uhci2IsochronousTransfer,
  __ID_Uhci2AsyncIsochronousTransfer,
  __ID_Uhci2GetRootHubPortStatus,
  __ID_Uhci2SetRootHubPortFeature,
  __ID_Uhci2ClearRootHubPortFeature,

  __ID_GetEhcInfo,
  __ID_EhcGetCapability,
  __ID_EhcReset,
  __ID_EhcGetState,
  __ID_EhcSetState,
  __ID_EhcControlTransfer,
  __ID_EhcBulkTransfer,
  __ID_EhcAsyncInterruptTransfer,
  __ID_EhcSyncInterruptTransfer,
  __ID_EhcIsochronousTransfer,
  __ID_EhcAsyncIsochronousTransfer,
  __ID_EhcGetRootHubPortStatus,
  __ID_EhcSetRootHubPortFeature,
  __ID_EhcClearRootHubPortFeature,


  __ID_GetXhcInfo,
  __ID_XhcGetCapability,
  __ID_XhcReset,
  __ID_XhcGetState,
  __ID_XhcSetState,
  __ID_XhcControlTransfer,
  __ID_XhcBulkTransfer,
  __ID_XhcAsyncInterruptTransfer,
  __ID_XhcSyncInterruptTransfer,
  __ID_XhcIsochronousTransfer,
  __ID_XhcAsyncIsochronousTransfer,
  __ID_XhcGetRootHubPortStatus,
  __ID_XhcSetRootHubPortFeature,
  __ID_XhcClearRootHubPortFeature,
  __ID_ScanUsbBus,
  __ID_UsbIoControlTransfer,
  __ID_UsbIoBulkTransfer,
  __ID_UsbIoAsyncInterruptTransfer,
  __ID_UsbIoSyncInterruptTransfer,
  __ID_UsbIoIsochronousTransfer,
  __ID_UsbIoAsyncIsochronousTransfer,
  __ID_UsbIoGetDeviceDescriptor,
  __ID_UsbIoGetActiveConfigDescriptor,
  __ID_UsbIoGetInterfaceDescriptor,
  __ID_UsbIoGetEndpointDescriptor,
  __ID_UsbIoGetStringDescriptor,
  __ID_UsbIoGetSupportedLanguages,
  __ID_UsbIoPortReset,

  __ID_LastSmmCall
} USB_SMMCALL ;

#pragma warning ( disable : 4306 )
#pragma warning ( disable : 4054 )

#define SMMCALL_0(func, ret) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 0; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 0)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](); \
    } \
  } while(0)

#define SMMCALL_1(func, ret, arg0) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 1; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 1)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0); \
    } \
  } while(0)

#define SMMCALL_2(func, ret, arg0, arg1) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 2; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 2)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1); \
    } \
  } while(0)

#define SMMCALL_3(func, ret, arg0, arg1, arg2) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 3; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 3)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2); \
    } \
  } while(0)

#define SMMCALL_4(func, ret, arg0, arg1, arg2, arg3) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 4; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 4)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)) \
      mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3); \
    } \
  } while(0)

#define SMMCALL_5(func, ret, arg0, arg1, arg2, arg3, arg4) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 5; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 5)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)) \
      mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4); \
    } \
  } while(0)

#define SMMCALL_6(func, ret, arg0, arg1, arg2, arg3, arg4, arg5) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 6; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 6)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5); \
    } \
  } while(0)

#define SMMCALL_7(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 7; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 7)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)) \
      {mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6); \
    } \
  } while(0)

#define SMMCALL_8(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 8; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      mSmmCallComm->Argv[7] = (VOID *)arg7; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 8)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)&& \
         (mSmmCallComm->Argv[7] == (VOID *)arg7)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); }\
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7); \
    } \
  } while(0)

#define SMMCALL_9(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 9; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      mSmmCallComm->Argv[7] = (VOID *)arg7; \
      mSmmCallComm->Argv[8] = (VOID *)arg8; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 9)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)&& \
         (mSmmCallComm->Argv[7] == (VOID *)arg7)&& \
         (mSmmCallComm->Argv[8] == (VOID *)arg8)) \
      {mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); \
    } \
  } while(0)

#define SMMCALL_10(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 10; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      mSmmCallComm->Argv[7] = (VOID *)arg7; \
      mSmmCallComm->Argv[8] = (VOID *)arg8; \
      mSmmCallComm->Argv[9] = (VOID *)arg9; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 10)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)&& \
         (mSmmCallComm->Argv[7] == (VOID *)arg7)&& \
         (mSmmCallComm->Argv[8] == (VOID *)arg8)&& \
         (mSmmCallComm->Argv[9] == (VOID *)arg9)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); \
    } \
  } while(0)

#define SMMCALL_11(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 11; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      mSmmCallComm->Argv[7] = (VOID *)arg7; \
      mSmmCallComm->Argv[8] = (VOID *)arg8; \
      mSmmCallComm->Argv[9] = (VOID *)arg9; \
      mSmmCallComm->Argv[10] = (VOID *)arg10; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 11)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)&& \
         (mSmmCallComm->Argv[7] == (VOID *)arg7)&& \
         (mSmmCallComm->Argv[8] == (VOID *)arg8)&& \
         (mSmmCallComm->Argv[9] == (VOID *)arg9)&& \
         (mSmmCallComm->Argv[10] == (VOID *)arg10)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); }\
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); \
    } \
  } while(0)

#define SMMCALL_12(func, ret, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
  do { \
    EFI_TPL               OldTpl; \
    *ret = EFI_ABORTED; \
    if (!IfInSmm()) { \
      OldTpl  = gBS->RaiseTPL (EFI_TPL_NOTIFY); \
      mSmmCallComm->CallId = __ID_##func; \
      mSmmCallComm->Return = ret; \
      mSmmCallComm->Argc = 12; \
      mSmmCallComm->Argv[0] = (VOID *)arg0; \
      mSmmCallComm->Argv[1] = (VOID *)arg1; \
      mSmmCallComm->Argv[2] = (VOID *)arg2; \
      mSmmCallComm->Argv[3] = (VOID *)arg3; \
      mSmmCallComm->Argv[4] = (VOID *)arg4; \
      mSmmCallComm->Argv[5] = (VOID *)arg5; \
      mSmmCallComm->Argv[6] = (VOID *)arg6; \
      mSmmCallComm->Argv[7] = (VOID *)arg7; \
      mSmmCallComm->Argv[8] = (VOID *)arg8; \
      mSmmCallComm->Argv[9] = (VOID *)arg9; \
      mSmmCallComm->Argv[10] = (VOID *)arg10; \
      mSmmCallComm->Argv[11] = (VOID *)arg11; \
      if((mSmmCallComm->CallId == __ID_##func)&& \
         (mSmmCallComm->Return == ret)&& \
         (mSmmCallComm->Argc == 12)&& \
         (mSmmCallComm->Argv[0] == (VOID *)arg0)&& \
         (mSmmCallComm->Argv[1] == (VOID *)arg1)&& \
         (mSmmCallComm->Argv[2] == (VOID *)arg2)&& \
         (mSmmCallComm->Argv[3] == (VOID *)arg3)&& \
         (mSmmCallComm->Argv[4] == (VOID *)arg4)&& \
         (mSmmCallComm->Argv[5] == (VOID *)arg5)&& \
         (mSmmCallComm->Argv[6] == (VOID *)arg6)&& \
         (mSmmCallComm->Argv[7] == (VOID *)arg7)&& \
         (mSmmCallComm->Argv[8] == (VOID *)arg8)&& \
         (mSmmCallComm->Argv[9] == (VOID *)arg9)&& \
         (mSmmCallComm->Argv[10] == (VOID *)arg10)&& \
         (mSmmCallComm->Argv[11] == (VOID *)arg11)) \
      { mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); }\
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11); \
    } \
  } while(0)

typedef UINTN (*SMMCALL_ENTRY)();

#define INIT_SMMCALL(func) \
  do { \
    UsbSmmCallTable[__ID_##func] = (SMMCALL_ENTRY)func; \
  } while(0)

typedef struct _EFI_LEGACY_USB_INF_PROTOCOL {
  USB_SMMCALL_COMM                   *SmmCallCommBuf;
  SMMCALL_ENTRY                      *SmmCallTablePtr;
} ;

typedef struct _USB_MOUSE_COMMUNICATION_PROTOCOL {
    UINT8       CurrentMouseData0[3];
    UINT8       CurrentMouseData1[3];
    UINT8       CurrentMouseData2[3];
    UINT8       CurrentMouseButtonData;
    UINT8       MouseDataSize;
    UINT8       MouseDataStatus;
} USB_MOUSE_COMMUNICATION_PROTOCOL;

extern EFI_GUID gEfiLegacyUsbInfProtocolGuid;

extern EFI_GUID gEfiUsbMouseCommunicationProtocolGuid;

#define LOOP while(_l)

#endif
