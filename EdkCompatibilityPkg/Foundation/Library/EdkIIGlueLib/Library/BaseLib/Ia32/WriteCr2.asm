; Copyright (c) 2004, Intel Corporation                                                         
; All rights reserved. This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
;
; Module Name:
;
;   WriteCr2.Asm
;
; Abstract:
;
;   AsmWriteCr2 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .386p
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr2 (
;   UINTN  Cr2
;   );
;------------------------------------------------------------------------------
AsmWriteCr2 PROC
    mov     eax, [esp + 4]
    mov     cr2, eax
    ret
AsmWriteCr2 ENDP

    END
