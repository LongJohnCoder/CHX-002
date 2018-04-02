//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Definitions for FAT recovery PEIM API functions

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation
(if any) is furnished under a license and may only be used or
copied in accordance with the terms of the license.  Except as
permitted by such license, no part of this software or
documentation may be reproduced, stored in a retrieval system, or
transmitted in any form or by any means without the express written
consent of Intel Corporation.

**/

#ifndef _FAT_API_H_
#define _FAT_API_H_

//
// API data structures
//
typedef VOID  *PEI_FILE_HANDLE;

typedef enum {
  Fat12,
  Fat16,
  Fat32,
  FatUnknown
} PEI_FAT_TYPE;

#endif
