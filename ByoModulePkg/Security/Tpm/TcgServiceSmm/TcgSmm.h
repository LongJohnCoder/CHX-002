/** @file

Copyright (c) 2006 - 2012, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  TcgSmm.h

Abstract: 
  tpm int 1A smm handler.

Revision History:

Bug 3843 - Fix sometimes int19h loader IPL measurment will be failed.
TIME:       2012-06-26
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Core
$TECHNICAL: 
  1. Enum blockio to read IPL data by BBS devicepath may not match real
     boot device. Now we issue a int 1A hook before csm jump to 7c00.
$END--------------------------------------------------------------------
**/

/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/*++

Copyright (c)  2010 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  TcgSmm.h

Abstract:

  TcgSmm.h

--*/
#ifndef _TCG_SMM_H_
#define _TCG_SMM_H_

/*
Data structure copied from TCG PC Client Specific Implementation Specification For Conventional BIOS
*/
typedef enum {
  TCG_StatusCheck,
  TCG_HashLogExtendEvent,
  TCG_PassThroughToTPM,
  TCG_ShutdownPreBootInterface,
  TCG_HashLogEvent,
  TCG_HashAll,
  TCG_TSS,
  TCG_CompactHashLogExtendEvent,
  TCG_MemoryOverwriteRequest,
  TCG_DriverInit = 0x20,
  TCG_MeasureLoaderIpl = 0x80
};

#pragma pack(1)

typedef struct {
  UINT8 sha_digest[20];
} SHADigestStruc;

typedef struct {
  UINT32 PCRIndex;
  UINT32 eventType;
  SHADigestStruc digest;
  UINT32 eventDataSize;
  UINT8 event;
} TCG_PCClientPCREventStruc;

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 AlgorithmID;
} IPB_HashAll_Struc;

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 PCRIndex;
  UINT32 LogEventType;
  UINT32 LogDataPtr; // TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} IPB_HashLogEventStruc;

typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT32 EventNumber;
} OPB_HashLogEventStruc;

// Input parameter block definition for TCG Pass Thru To TPM function
typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT16 OPBLength;
  UINT16 Reserved2;
  UINT8 TPMOperandIn;
} IPB_TPMStruc;

//
// Input Parameter Block Definition for TCG_HashLogExtendEvent func
//
typedef struct {
  UINT32 Reserved2;
  UINT32 LogDataPtr; // TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} LongLogDataStruc;

typedef struct {
  UINT32 LogDataPtr;  //TCG_PCClientPCREventStruc pointer
  UINT32 LogDataLen;
} ShortLogDataStruc;

union LogDataUnion {
  LongLogDataStruc longLog;
  ShortLogDataStruc shortLog;
};

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved;
  UINT32 HashDataPtr;
  UINT32 HashDataLen;
  UINT32 PCRIndex;
  union LogDataUnion ulog;
} IPB_HashLogExtendEventStruc;

// Output parameter block definition for TCG Pass Thru To TPM function
typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT8 TPMOperandOut;
} OPB_TPMStruc;

//
// Output Parameter Block Definition for TCG_HashLogExtendEvent func
//
typedef struct {
  UINT16 OPBLength;
  UINT16 Reserved1;
  UINT32 EventNumber;
  UINT8 HashValue[20];
} OPB_HashLogExtendEventStruc;

typedef struct {
  UINT16 IPBLength;
  UINT16 Reserved1;
  UINT8 MORBitValue;
} IPB_MORStruc;

#pragma pack()

#endif // _TCG_SMM_H_
