/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  SmmIoTrapDispatch.h

Abstract: 
  SMM IO Trap Dispatch Protocol.

Revision History:
Bug 1920: Create some SMM thunk drivers to support framework style silicon drivers.
TIME: 2011-05-13
$AUTHOR:  Cassie Liu
$REVIEWERS:  Hawker Chen
$SCOPE: Invoke framework implement on EDKII platform.
$TECHNICAL: Produce PI style protocol consuming framework style protocol.
$END----------------------------------------------------------------------------

**/
/*++
  This file contains a 'Sample Driver' and is licensed as such  
  under the terms of your license agreement with Intel or your  
  vendor.  This file may be modified by the user, subject to    
  the additional terms of the license agreement                 
--*/
/*++

Copyright (c) 2005 - 2008 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  SmmIoTrapDispatch.h

Abstract:

  PCH SMM IO Trap Dispatch Protocol

--*/
#ifndef _EFI_SMM_IO_TRAP_DISPATCH_H_
#define _EFI_SMM_IO_TRAP_DISPATCH_H_

//
// Share some common definitions with PI SMM
//
#include <Protocol/SmmIoTrapDispatch2.h>

//
// GUID for the SMM IO Trap Dispatch Protocol
//;
#define EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL_GUID \
  { \
    0x58dc368d, 0x7bfa, 0x4e77, 0xab, 0xbc, 0xe, 0x29, 0x41, 0x8d, 0xf9, 0x30 \
  };
//
// Extern the GUID for protocol users.
//
extern EFI_GUID                                   gEfiSmmIoTrapDispatchProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL;

//
// Related Definitions
//

//
// IO Trap context structure containing information about the IO trap event that should invoke the callback
//
typedef struct {
  UINT16                        Address;  // IO Trap range base address (NULL means allocate)
  UINT16                        Length;   // IO Trap range length
  EFI_SMM_IO_TRAP_DISPATCH_TYPE Type;     // Access types to trap on
  VOID                          *Context; // Callback function context
} EFI_SMM_IO_TRAP_DISPATCH_REGISTER_CONTEXT;

//
// IO Trap context structure containing information about the IO trap that occurred
//
typedef struct {
  UINT16                        Address;    // IO address trapped
  EFI_SMM_IO_TRAP_DISPATCH_TYPE Type;       // IO access type
  UINT32                        WriteData;  // Data written (contents undefined for read trap)
  VOID                          *Context;   // Callback function context
} EFI_SMM_IO_TRAP_DISPATCH_CALLBACK_CONTEXT;

//
// Member functions
//
typedef
VOID
(EFIAPI *EFI_SMM_IO_TRAP_DISPATCH_CALLBACK) (
  IN EFI_HANDLE                                 DispatchHandle,
  IN EFI_SMM_IO_TRAP_DISPATCH_CALLBACK_CONTEXT  * CallbackContext
  );

/*++

Routine Description:

  Dispatch function for an IO Trap specific SMI handler.

Arguments:

  DispatchHandle      - Handle of this dispatch function.
  CallbackContext     - Pointer to the dispatched function's context.
                        The CallbackContext fields are updated
                        by the dispatching driver prior to
                        invoking this callback function.

Returns:

  Nothing

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_IO_TRAP_DISPATCH_REGISTER) (
  IN     EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL            * This,
  IN     EFI_SMM_IO_TRAP_DISPATCH_CALLBACK            DispatchFunction,
  IN OUT EFI_SMM_IO_TRAP_DISPATCH_REGISTER_CONTEXT    * RegisterContext,
  OUT EFI_HANDLE                                      * DispatchHandle
  );

/*++

Routine Description:

  Register an IO trap SMI child handler for a specified SMI.
  
  This service will register a child for a given SMI source.  
  The caller will provide information about the IO trap characteristics via the context.  
  This includes base address, length, and type (read, write, read/write).
  The service will allocate the IO range if the base address is 0, and the RegisterContext 
  Address field will be updated and returned to the caller.
  The service will allocate system resources via GCD services for the requested IO trap range and type.  
  An error will be returned if insufficient resources are available to fulfill the request.  
  The service will not perform GCD allocation if the base address is non-zero.  In this case,
  the caller is responsible for the existence and allocation of the specific IO range.
  An error may be returned if some or all of the requested resources conflict with an existing IO trap child handler.
  It is not required that implementations will allow multiple children for a single IO trap SMI source.  
  Some implementations may support multiple children.
    
Arguments:

  This                  - Pointer to the EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL instance.
  DispatchFunction      - Pointer to the dispatch function to be invoked for this SMI source.
  RegisterContext       - Pointer to the dispatch function's context.  
                          The caller fills this context in before calling the register function to indicate to the 
                          register function the IO trap SMI source for which the dispatch function should be invoked.
                          This may not be NULL.
  DispatchHandle        - Handle of the dispatch function, for when interfacing with the parent SMM driver.
                          Type EFI_HANDLE is defined in InstallProtocolInterface() in the EFI 1.10 Specification.
                          This may not be NULL.
                            
Returns:

  EFI_SUCCESS           - The dispatch function has been successfully registered.
  EFI_DEVICE_ERROR      - The driver was unable to complete due to hardware error.
  EFI_OUT_OF_RESOURCES  - Insufficient resources are available to fulfill 
                          the IO trap range request.
  EFI_INVALID_PARAMETER - RegisterContext is invalid.  The input value is not within a valid range.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_IO_TRAP_DISPATCH_UNREGISTER) (
  IN EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL          * This,
  IN EFI_HANDLE                                 * DispatchHandle
  );

/*++

Routine Description:

  Unregister a child SMI source dispatch function with a parent SMM driver

  This service removes a previously installed child dispatch handler.  
  This does not guarantee that the system resources will be freed from the GCD.

Arguments:

  This                  - Pointer to the EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL instance.
  DispatchHandle        - Handle of the child service to remove.  
                          Type EFI_HANDLE is defined in InstallProtocolInterface() in the EFI 1.10 Specification.

Returns:

  EFI_SUCCESS           - The dispatch function has been successfully 
                          unregistered.
  EFI_INVALID_PARAMETER - Handle is invalid.

--*/

//
// Interface structure for the SMM IO trap specific SMI Dispatch Protocol
//
struct _EFI_SMM_IO_TRAP_DISPATCH_PROTOCOL {
  EFI_SMM_IO_TRAP_DISPATCH_REGISTER   Register;
  EFI_SMM_IO_TRAP_DISPATCH_UNREGISTER UnRegister;
};

#endif
