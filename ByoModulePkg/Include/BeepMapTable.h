/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  BeepMapTable.h

Abstract: 
  Beep status code definition.

Revision History:

Bug 2909:   Add some port 80 status codes into EDKII code.
TIME:       2011-09-23
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     All Platforms
$TECHNICAL: 
  1. Improve Port80 map table.
  2. Add Port80 status codes in the corresponding position to report status code.
  3. Change the seconed REPORT_STATUS_CODE_WITH_EXTENDED_DATA macro's parameter
     to EFI_SW_PC_INIT_END from EFI_SW_PC_INIT_BEGIN.
$END--------------------------------------------------------------------

Bug 2517:   Create the Module StatusCodeHandler to report status code to 
            all supported devide in ByoModule
TIME:       2011-7-22
$AUTHOR:    Liu Chunling
$REVIEWERS:  
$SCOPE:     All Platforms
$TECHNICAL:  
  1. Create the module StatusCodeHandler to support Serial Port, Memory, Port80,
     Beep and OEM devices to report status code.
  2. Create the Port80 map table and the Beep map table to convert status code 
     to code byte and beep times.
  3. Create new libraries to support status code when StatusCodePpi,
     StatusCodeRuntimeProtocol, SmmStatusCodeProtocol has not been installed yet.
$END--------------------------------------------------------------------

**/

#ifndef __BEEP_MAP_TABLE_H__
#define __BEEP_MAP_TABLE_H__

#include <ByoStatusCode.h>

STATUS_CODE_TO_DATA_MAP mProgressBeepMap[] = {
  //
  // PEI
  //
  //Recovery
  { PEI_RECOVERY_STARTED, 2 },

  //
  // DXE
  //

  {0,0}
};

STATUS_CODE_TO_DATA_MAP mErrorBeepMap[] = {
  //
  // PEI
  //
  // Regular boot
  { PEI_MEMORY_NOT_DETECTED, 3 },    // PeimMemoryInit() in MemoryInit.c
  // { PEI_MEMORY_INSTALLED_TWICE, 1 },
  // { PEI_DXEIPL_NOT_FOUND, 3 },
  { PEI_DXE_CORE_NOT_FOUND, 3 },     // DxeIplFindDxeCore() in DxeLoad.c
  // { PEI_RESET_NOT_AVAILABLE, 7 },
  
  // 
  // Recovery
  //
  // { PEI_RECOVERY_FAILED, 4 },

  //
  // S3 Resume
  // 
  // { PEI_S3_RESUME_FAILED, 4 },

  //
  // DXE
  //
  { DXE_ARCH_PROTOCOL_NOT_AVAILABLE, 4 },  // DxeMain() in DxeMain.c
  { DXE_BDS_LENOVO_COMMON_ERROR, 2 },
  { DXE_CON_OUT_CONNECT, 1 },
  { DXE_NO_CON_OUT, 5 },                   // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  { DXE_NO_CON_IN, 5 },                    // EfiBootManagerConnectAllDefaultConsoles() in BdsConnsole.c
  // { DXE_INVALID_PASSWORD, 1 },            
  // { DXE_FLASH_UPDATE_FAILED, 6 },    
  // { DXE_RESET_NOT_AVAILABLE, 7 },    

  {0,0}
};

#endif

