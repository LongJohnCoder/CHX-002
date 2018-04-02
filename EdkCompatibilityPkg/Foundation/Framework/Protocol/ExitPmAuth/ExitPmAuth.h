//
// This file contains 'Framework Code' and is licensed as such 
// under the terms of your license agreement with Intel or your
// vendor.  This file may not be modified, except as allowed by
// additional terms of your license agreement.                 
//
/*++

Copyright (c)  2009 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  ExitPmAuth.h

Abstract:


--*/

#ifndef _EXIT_PM_AUTH_PROTOCOL_H_
#define _EXIT_PM_AUTH_PROTOCOL_H_

#include "Tiano.h"

#define EXIT_PM_AUTH_PROTOCOL_GUID \
  { \
    0xd088a413, 0xa70, 0x4217, 0xba, 0x55, 0x9a, 0x3c, 0xb6, 0x5c, 0x41, 0xb3 \
  }

extern EFI_GUID gExitPmAuthProtocolGuid;

#endif
