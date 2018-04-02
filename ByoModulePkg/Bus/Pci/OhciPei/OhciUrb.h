//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Provides some data struct used by OHCI controller driver.

Copyright (c) 2010, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/


#ifndef _OHCI_TRANSFERLIST_H
#define _OHCI_TRANSFERLIST_H

#include "Descriptor.h" 


//
// Func List
//


/**

  Create a TD

  @Param  Ohc                   UHC private data

  @retval                       TD structure pointer

**/
TD_DESCRIPTOR *
OhciCreateTD (
  IN USB_OHCI_HC_DEV      *Ohc
  ); 

/**

  Free a TD
	
  @Param  Ohc                   UHC private data  
  @Param  Td                    Pointer to a TD to free

  @retval  EFI_SUCCESS          TD freed

**/
EFI_STATUS
OhciFreeTD (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN TD_DESCRIPTOR        *Td
  ); 

/**

  Create a ED

  @Param   Ohc                  Device private data

  @retval  ED                   descriptor pointer

**/
ED_DESCRIPTOR *
OhciCreateED (
  USB_OHCI_HC_DEV          *Ohc
  ); 


/**

  Free a ED
  
  @Param  Ohc                   UHC private data
  @Param  Ed                    Pointer to a ED to free

  @retval  EFI_SUCCESS          ED freed

**/

EFI_STATUS
OhciFreeED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  );

/**

  Free  ED

  @Param  Ohc                    Device private data
  @Param  Ed                     Pointer to a ED to free

  @retval  EFI_SUCCESS           ED freed

**/
EFI_STATUS
OhciFreeAllTDFromED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  );

/**

  Attach an ED

  @Param  Ed                    Ed to be attached
  @Param  NewEd                 Ed to attach

  @retval EFI_SUCCESS           NewEd attached to Ed
  @retval EFI_INVALID_PARAMETER Ed is NULL

**/
EFI_STATUS
OhciAttachED (
  IN ED_DESCRIPTOR        *Ed,
  IN ED_DESCRIPTOR        *NewEd
  ); 
/**

  Attach an ED to an ED list

  @Param  OHC                   UHC private data
  @Param  ListType              Type of the ED list
  @Param  Ed                    ED to attach
  @Param  EdList                ED list to be attached

  @retval  EFI_SUCCESS          ED attached to ED list

**/
EFI_STATUS
OhciAttachEDToList (
  IN USB_OHCI_HC_DEV       *Ohc,
  IN DESCRIPTOR_LIST_TYPE  ListType,
  IN ED_DESCRIPTOR         *Ed,
  IN ED_DESCRIPTOR         *EdList
  );
EFI_STATUS
OhciLinkTD (
  IN TD_DESCRIPTOR        *Td1,
  IN TD_DESCRIPTOR        *Td2
  ); 


/**

  Attach TD list to ED

  @Param  Ed                    ED which TD list attach on
  @Param  HeadTd                Head of the TD list to attach

  @retval  EFI_SUCCESS          TD list attached on the ED

**/
EFI_STATUS
OhciAttachTDListToED (
  IN ED_DESCRIPTOR        *Ed,
  IN TD_DESCRIPTOR        *HeadTd
  ); 


/**

  Set value to ED specific field

  @Param  Ed                    ED to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field,
  IN UINT32               Value
  ); 


/**

  Get value from an ED's specific field

  @Param  Ed                    ED pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/
UINT32
OhciGetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field
  ); 


/**

  Set value to TD specific field

  @Param  Td                    TD to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetTDField (
  IN TD_DESCRIPTOR        *Td,
  IN UINT32               Field,
  IN UINT32               Value
  ); 


/**

  Get value from ED specific field

  @Param  Td                    TD pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/

UINT32
OhciGetTDField (
  IN TD_DESCRIPTOR      *Td,
  IN UINT32             Field
  ); 

#endif
