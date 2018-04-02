/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.


Module Name:

  BdsTpmMeasureLib.h

Abstract:

  BDS Lib functions definition

Revision History

**/

#ifndef _BDS_TPM_LIB_H_
#define _BDS_TPM_LIB_H_

#include <PiDxe.h>
#include <Protocol/BlockIo.h>


#define EFI_CALLING_INT19         \
  "Calling INT 19H"


STATIC
EFI_STATUS
BdsTpmLibDevicePathToBlockIo(
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  CHAR16                    *Description,
  OUT EFI_BLOCK_IO_PROTOCOL     **BlkIo
  );

EFI_STATUS
BdsTpmLibMeasureInt19Event(
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  UINT16                    *Description
  );

#endif