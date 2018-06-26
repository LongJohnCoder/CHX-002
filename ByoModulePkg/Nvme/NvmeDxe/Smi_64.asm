
.code

PUBLIC  gSmmPort
gSmmPort dw 0x82F

;UINT32
;SMI_CALL(
;    UINT8    SmiValue,             ; RCX
;    UINT32   Address               ; RDX
;);

SMI_CALL PROC USES rbx

    mov     ebx,    edx
    mov     al,     cl  
    mov     dx,     gSmmPort
    out     dx,     al
    nop
    nop
    nop
       
    ret
    
SMI_CALL  ENDP

END
