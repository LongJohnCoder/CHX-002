/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  LegacySDInf.h

Abstract: 
  SD card Model.

Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-16
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE: SD card feature support.
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/

#ifndef _LEGACY_SD_INF_PROTOCOL_H_
#define _LEGACY_SD_INF_PROTOCOL_H_

//EFI_FORWARD_DECLARATION (EFI_LEGACY_SD_INF_PROTOCOL);

#define EFI_LEGACY_SD_INF_PROTOCOL_GUID \
  { \
    0xd7e2f4f8, 0x340f, 0x42e4, 0x8e, 0x60, 0xdb, 0xbb, 0xf1, 0x64, 0xa1, 0xc5 \
  }

#define EFI_TPL_NOTIFY      16
typedef struct _SD_SMMCALL_COMM {
  UINTN       CallId;
  EFI_STATUS  *Return;
  UINTN       Argc;
  VOID        *Argv[10];
} SD_SMMCALL_COMM;

typedef enum {
  __ID_DetectCardAndInitHost = 0,
  __ID_SetClockFrequency,
  __ID_SetBusWidth,
  __ID_SetHostVoltage,
  __ID_SetHostDdrMode,
  __ID_ResetSdHost,
  __ID_EnableAutoStopCmd,
  __ID_SetBlockLength,
  __ID_SendHCommand,
  __ID_StartLegacySd,
  __ID_SetupDevice,
  __ID_SetHostSpeedMode,

  __ID_LastSmmCall
} SD_SMMCALL ;

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
      {mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
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
      {mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
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
      {mSmmControl->Trigger (mSmmControl, &mArgBuffer, &mArgBufferSize, FALSE, 0); } \
      gBS->Stall(15); \
      gBS->RestoreTPL (OldTpl); \
    } \
    else { \
      *ret = mSmmCallTablePtr[__ID_##func](arg0, arg1); \
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

typedef UINTN (*SMMCALL_ENTRY)();

#define INIT_SMMCALL(func) \
  do { \
    SdSmmCallTable[__ID_##func] = (SMMCALL_ENTRY)func; \
  } while(0)

typedef struct _EFI_LEGACY_SD_INF_PROTOCOL {
  SD_SMMCALL_COMM                   *SmmCallCommBuf;
  SMMCALL_ENTRY                      *SmmCallTablePtr;
} EFI_LEGACY_SD_INF_PROTOCOL;

extern EFI_GUID gEfiLegacySDInfProtocolGuid;

#endif
