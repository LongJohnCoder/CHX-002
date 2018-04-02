/*++

Copyright (c) 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGluePcdPerformanceLib.h
  
Abstract: 

  PCD values for library customization

--*/

#ifndef __EDKII_GLUE_PCD_PERFORMANCE_LIB_H__
#define __EDKII_GLUE_PCD_PERFORMANCE_LIB_H__

//
// Following Pcd values are hard coded at compile time.
// Override these through compiler option "/D" in PlatformTools.env if needed
//

//
// Performance Lib Pcds
//
#ifndef __EDKII_GLUE_PCD_PlatformInfoMsr__
#define __EDKII_GLUE_PCD_PlatformInfoMsr__  EDKII_GLUE_PlatformInfoMsr
#endif

#ifndef __EDKII_GLUE_PCD_XAPICBaseMsr__
#define __EDKII_GLUE_PCD_XAPICBaseMsr__  EDKII_GLUE_XAPICBaseMsr
#endif

#ifndef __EDKII_GLUE_PCD_ExtXAPICLogicalAPICIdMsr__
#define __EDKII_GLUE_PCD_ExtXAPICLogicalAPICIdMsr__  EDKII_GLUE_ExtXAPICLogicalAPICIdMsr
#endif

#include "Pcd/EdkIIGluePcd.h"
#endif
