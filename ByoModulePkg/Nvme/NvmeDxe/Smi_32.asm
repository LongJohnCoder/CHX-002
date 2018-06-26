
  .686
  .MODEL FLAT,C
  .CODE

PUBLIC  gSmmPort
gSmmPort dw 0x82F

;UINT32
;SMI_CALL(
;    UINT8    SmiValue,
;    UINT32   Address
;);

SMI_CALL PROC PUBLIC Func:BYTE, Address:DWORD
    push    ebx
    mov     ebx,    Address
    mov     al,     Func  
    mov     dx,     gSmmPort
    out     dx,     al
    pop     ebx
    nop
    nop
    nop
       
    ret
    
SMI_CALL  ENDP

END