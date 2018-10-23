
;------------------------------------------------------------------------------
  INCLUDE Platform.inc
  INCLUDE Ia32.inc
  INCLUDE SecCore.inc


.686p
.xmm
.model small, c

EXTRN   SecStartup:NEAR

; ECP porting
EXTRN   PcdGet32 (PcdFlashNvStorageMicrocodeBase):DWORD
EXTRN   PcdGet32 (PcdFlashNvStorageMicrocodeSize):DWORD
EXTRN   PcdGet32 (PcdFlashFvRecoveryBase):DWORD
EXTRN   PcdGet32 (PcdFlashFvRecoverySize):DWORD
EXTRN   PcdGet32 (PcdFlashFvMainBase):DWORD
EXTRN   PcdGet32 (PcdFlashAreaBaseAddress):DWORD
EXTRN   PcdGet32 (PcdFlashAreaSize):DWORD
EXTRN   PcdGet32 (PcdTemporaryRamBase):DWORD
EXTRN   PcdGet32 (PcdTemporaryRamSize):DWORD
EXTRN   PcdGet64 (PcdPciExpressBaseAddress):DWORD
EXTRN   PcdGet16 (AcpiIoPortBaseAddress):WORD
#ifdef ZX_SECRET_CODE
EXTRN   PcdGet32 (PcdMSRSEC0Base):DWORD
EXTRN   PcdGet32 (PcdMSRSEC0Size):DWORD
EXTRN   PcdGet32 (PcdMSRSEC1Base):DWORD
EXTRN   PcdGet32 (PcdMSRSEC1Size):DWORD
EXTRN   PcdGet32 (PcdBootConfigBase):DWORD
EXTRN   PcdGet32 (PcdBootConfigSize):DWORD
#endif
#ifdef ZX_SECRET_CODE
MAX                           EQU 00h
MANUAL_PTx                    EQU 01h
BVIDStruc STRUC
  Signature             dd ?
  Reserved4_7           dd ?
  OpType                db ?
  Reserved9             db ?
  Reserved10_11         dw ?
  Reserved12_15         dd ?
BVIDStruc    ENDS
#endif



_TEXT_REALMODE SEGMENT PARA PUBLIC USE16 'CODE'
  ASSUME  CS:_TEXT_REALMODE, DS:_TEXT_REALMODE

align 4
_ModuleEntryPoint PROC NEAR C PUBLIC

  jmp     short @f
  db      '_', '3', '2', 'E'
  dd      offset ProtectedModeSECStart
@@:

  rdtsc
  movd    mm0, eax

  STATUS_CODE (02h)                     ; BSP_PROTECTED_MODE_START
  mov     esi,  OFFSET GdtDesc
  DB      66h
  lgdt    fword ptr cs:[si]
  mov     eax, cr0                      ; Get control register 0
  or      eax, 00000003h                ; Set PE bit (bit #0) & MP bit (bit #1)
  mov     cr0, eax                      ; Activate protected mode
  mov     eax, cr4                      ; Get control register 4
  or      eax, 00000600h                ; Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
  mov     cr4, eax

  ;
  ; Now we're in Protected16
  ; Set up the selectors for protected mode entry
  ;
  mov     ax, SYS_DATA_SEL
  mov     ds, ax
  mov     es, ax
  mov     gs, ax
  mov     fs, ax
  mov     ss, ax

  ;
  ; Go to Protected32
  ;
  mov     esi, offset NemInitLinearAddress
  jmp     fword ptr cs:[si]

TightLoop:
  cli
  hlt
  jmp     TightLoop

_ModuleEntryPoint ENDP
_TEXT_REALMODE      ENDS

; mm0 is used to save time-stamp counter value.
; mm1 is used to save microcode address.
; mm6 is used to save microcode size
; mm2 ~ mm5 for temp saver in building CAR.
; mm3 ~ mm4 for temp saver in building CAR.

_TEXT_PROTECTED_MODE SEGMENT PARA PUBLIC USE32 'CODE'
  ASSUME  CS:_TEXT_PROTECTED_MODE, DS:_TEXT_PROTECTED_MODE

align 4
ProtectedModeSECStart PROC NEAR PUBLIC
;Program SPI Clock  end
;bcfg(0,11,0,BC)[31:12] 
mov eax, 80000000h + (((0 shl 8) + (11h shl 3) + 0) shl 8) + 0BCh
mov dx, 0cf8h
out dx, eax
mov dx, 0cfch
mov eax, (0FEB32000h shr 8)
out dx, eax


; D17F0MMIO_SPI_BUS_0_CTL
mov edi, 0FEB32000h
mov eax, 0FEB30000h + 1h
mov [edi], eax


;Program SPI Clock to 48Mhz start
;MMIO Rx63[0]:Fast read enable
mov edi, 0FEB30000h + 063h
mov al, [edi]
or al, 01h
mov [edi], al

;Mmio Rx6C=00h CLKDIV
mov edi, 0FEB30000h + 06Ch
mov al, 00h
mov [edi], al

; SPI0MMIO_SPI_BUS_0_MISC_CTL_1
;Mmio Rx6D[7]=1 SPI_48EN
;Mmio Rx6D[5]=0 SPI_66EN
mov edi, 0FEB30000h + 06Dh
mov al, [edi]
or al, 80h
and al, 0DFh
mov [edi], al




;Program SPI Clock  end



;DLA_DEBUG_E
; msr(1B).BIT8 - BSP
  mov  ecx, 1Bh
  rdmsr
  test eax, BIT8
  jnz  short IamBSP

@@:
  cli
  hlt
  jmp  short @b

IamBSP:
#ifdef ZX_SECRET_CODE

    mov ecx, 1440h
    rdmsr
    or eax, eax
    jnz pstDone

    xor edx, edx

;;------------------------------------------------------------------------------
;; NOTE: To change CPU core frequency and voltage, give ax a Hex value xxyyh as such: 
;;  mov ax, xxyyh
; Here:
;;  xx:  FID, the x2 ratio to base freq(100MHz),  FID=Target_Freq_in_MHz*2/100
;;  yy:  VID, the VID value depends on target voltage and VRM Type.
;;     For SVID Board, VID=Target_Voltage * 200 - 49; 
;;     For PVID Board, VID=Target_Voltage * 80 + 7;
;; For example,  on a SVID board(HX002EA0_00),
;;  to get 3G/1.05V, set FID=3000/50=3C; VID=1.05*200-49=A1h, so xxyy=3CA1h.
;;  to get 2.2G/1.0V, set FID=2200/50=2C; VID=1.0*200-49=97h, so xxyy=2C97h
;;
;; Sample Table
;;
;;  Freq/Vol     SVID     PVID
;; 3.0G/1.0V  3C97     3C57
;; 3.0G/1.05V  3CA1     3C5B 
;; 3.0G/1.1V   3CAB   3C5F
;; 2.7G/1.0V   3697     3657
;; 2.7G/1.05V 36A1   365B     
;; 2.7G/1.1V   36AB   365F     
;; 2.2G/1.0V   2C97     3657
;; 2.2G/1.05V 2CA1     365B
;; 2.0G/0.95V 288D     
;; 2.0G/1.0V   2897     2857 ---- BIOS default setting
;; 2.0G/1.05V 28A1    285B
;; 1.6G/1.0V   2097     2057
;; 800M/0.8V  106F    1047
;;
;;------------------------------------------------------------------------------   
;;End-Of-Core-Freq-Vol {
#ifdef PCISIG_PLUGFEST_WORKAROUND
	mov ax, 36A1h    ;SVID Board, default to 2.7G/1.05V, see guide above to select the required freq/vol - for PCI_SIG compliance BIOS test  
#else
    mov ax, 2897h    ;SVID Board, default to 2.0G/1.0V, see guide above to select the required freq/vol  
#endif    
#if defined(HX002EB0_00)|| defined(HX002EB0_11)
    mov ax, 2857h       ;PVID Board
#endif
;;}End-Of-Core-Freq-Vol

    mov ecx, 1440h
    bts eax, 31
    wrmsr

    inc ecx
    bts eax, 27
    wrmsr
pstDone:    
    mov ecx, 1407h
    rdmsr
    bts eax, 18 
    wrmsr
#endif

  #ifdef ZX_SECRET_CODE
  mov  edi, PcdGet32(PcdMSRSEC0Base)
  mov  ebx, PcdGet32(PcdMSRSEC0Size)
  CALL_MMX  SecMsrConfig
 #endif
#if !defined(HX002EB0_00) && !defined(HX002EB0_11)
  CALL_MMX  CHX002A0PatchSVID
#endif
  STATUS_CODE (03h)
  CALL_MMX  VeryEarlyMicrocodeUpdate  
  #ifdef ZX_SECRET_CODE
  mov  edi, PcdGet32(PcdMSRSEC1Base)
  mov  ebx, PcdGet32(PcdMSRSEC1Size)
  CALL_MMX  SecMsrConfig
  #endif
  STATUS_CODE (04h)
  CALL_MMX  PlatformInitialization

  STATUS_CODE (05h)
  CALL_MMX  ForceCpuMAXRatio

  STATUS_CODE (06h)
  CALL_MMX  CacheAsRam

  STATUS_CODE (07h)
  jmp  CallPeiCoreEntryPoint

ProtectedModeSECStart ENDP
#ifdef ZX_SECRET_CODE
SecMsrConfig PROC NEAR PRIVATE
    mov esi, edi
    add edi, ebx

FillLoop:
    cmp  esi, edi
    jae  FillExit
    
    cmp DWORD PTR [esi], 0xffffffff
    jz  FillExit
    
    mov ecx, [esi]
    rdmsr
    mov     ebx,[esi + 8]
    ;Byte8: Bit1:
    ;            0:+/- OrMask
    ;            1: |  OrMask
    test    ebx,10b
    jz      AddOrSub
    mov     ebx,[esi + 16]
    NOT	    ebx
    and     eax, ebx
    
    mov     ebx,[esi + 20]
    NOT	    ebx
    and     edx, ebx
	
    or eax, [esi + 24]
    or edx, [esi + 28]
    wrmsr
    jmp NextItem
AddorSub:
    mov     ebx,[esi + 16]
    NOT	    ebx
    and     ebx, eax
    ;mm2 temp save
    movd    mm2,ebx
    mov     ebx,[esi + 20]
    NOT	    ebx
    and     ebx, edx
    ;mm3 temp eave
    movd    mm3,ebx
    mov     ebx,[esi + 16]
    and     eax,ebx
    mov     ebx,[esi + 20] 
    and     edx,ebx
    cmp     eax, 0h
    jnz     Operate
    cmp     edx ,0h
    jz      NextItem
Operate:
    mov     ebx,[esi + 8]
    ;Byte8: Bit2:
    ;            0:+ OrMask
    ;            1:- OrMask
    test    ebx,100b
    jz      AddValue
    sub  eax,[esi+24]
    sub  edx,[esi+28]
    jmp  SetValue
AddValue:
    ; if   ReadValue & AndMask==0 , not Set
    ; else SetValue = (ReadValue & AndMask + OrMask)&(~AndMask)
    add  eax,[esi+24]
    add  edx,[esi+28]
SetValue:
    mov  ebx,[esi + 16]
    and  eax,ebx
    mov  ebx,[esi + 20]
    and  edx,ebx
    ;if   ReadValue & AndMask==0 , not Set
    ;else SetValue = (ReadValue & AndMask - OrMask)&(~AndMask)
    movd ebx, mm2
    or   eax, ebx
    movd ebx, mm3
    or   edx, ebx
    wrmsr
NextItem:    
    add esi, 0x20
    jmp FillLoop
FillExit:
  RET_MMX
SecMsrConfig ENDP
#endif
CHX002A0PatchSVID    PROC    NEAR    PRIVATE
  mov edi,0E0088000h + 0BCh
  mov eax,0FEB320h
  mov [edi],eax
  ;Set VID
  mov ecx, 1444h
ReadPTx:
  rdmsr
  dec ecx
  cmp al,00h
  jz ReadPTx
  mov edi,0FEB3210Fh
  mov [edi],al

  mov edi,0FEB3210Bh
  mov al,01h
  mov [edi],al
CheckComplete:
  xor   eax,eax
  mov edi,0FEB32101h
  mov al ,[edi]
  cmp   al, 05h
  jnz   CheckComplete
  RET_MMX
CHX002A0PatchSVID ENDP

ForceCpuMAXRatio    PROC    NEAR    PRIVATE

; Enable EPS( Enhanced PowerSaver)  
; EPS flag cpuid[1].RegEcx[7]
  xor  eax, eax
  inc  eax
  cpuid
  test ecx, BIT7                        ; Enhanced PowerSaver.
  jz SetDone

  mov ebx, eax                          ; ebx = cpuid

  mov ecx, MSR_IA32_MISC_ENABLES        ; enable EPS MSR 0x1A0[16] 
  rdmsr
  bts     eax,16
  wrmsr       
#ifdef ZX_SECRET_CODE
  mov     esi, PcdGet32(PcdBootConfigBase)
  mov     edi, esi
  add     edi, PcdGet32(PcdBootConfigSize)
  cmp     (BVIDStruc PTR [esi]).Signature, 44495642h     ; "BVID"
  jnz      short SetMaxPSate
  cmp     (BVIDStruc PTR [esi]).OpType, MAX
  jz      short SetMaxPSate
  cmp     (BVIDStruc PTR [esi]).OpType, MANUAL_PTx 
  jae     SetMunal_PTx 
SetMunal_PTx:
  mov ecx ,01440h
  xor eax ,eax
  mov al,(BVIDStruc PTR [esi]).OpType
  add ecx ,eax
  sub ecx ,MANUAL_PTx
  cmp ecx, 1444h
  ja  SetMaxPSate
  rdmsr
  cmp eax ,0h
  je SetMaxPSate
  mov ebx, eax
  mov ecx, MSR_IA32_PERF_CTL
  rdmsr
  mov ax, bx
  wrmsr
  jmp   short SetTSCFreq
SetMaxPSate:
#endif
  mov ecx, MSR_IA32_PERF_STS
  rdmsr  
; [7:0]   - Current VID
; [15:8]  - Current FID
; [39:32] - Highest Supported Voltage
; [47:40] - Highest Supported Clock Ratio 
; [16]    - Clock Ratio Transition in progress  
; [17]    - Voltage Transition in progress 
  mov bx, dx                 ; Max PState
  mov ecx, MSR_IA32_PERF_CTL
  rdmsr
  mov ax, bx  
  wrmsr
SetTSCFreq:
;0x1203  CENT_HARDWARECTRL3 [32]
;  0 - TSC Always runs at the maximum frequency supported by the by the processor 
;  1 - TSC runs at the current operating frequency of the processor 
  mov ecx, 1203h
  rdmsr
;YKN-20161019 -S
;Apply CV's suggestion for non-fused chip
 ; and  dl, not BIT0
  or dl, 01h
;YKN-20161019 -E
  wrmsr  
  
SetDone:
  RET_MMX
ForceCpuMAXRatio    ENDP




VeryEarlyMicrocodeUpdate    PROC    NEAR    PRIVATE

    xor     eax, eax
    movd    mm3, eax 
    movd    mm4, eax
    movd    mm1, eax                    ; clean it to 0.
    movd    mm6, eax

    ;mov     ecx, IA32_BIOS_SIGN_ID
    ;rdmsr                               ; CPU PatchID -> EDX
    ;or      edx, edx                    ; If microcode has been updated
    ;jnz     luExit                      ; Skip if loaded

    inc     eax
    cpuid                               ; EAX = CPU signature.

    mov     esi, PcdGet32(PcdFlashNvStorageMicrocodeBase)
    mov     edi, esi
    add     edi, PcdGet32(PcdFlashNvStorageMicrocodeSize)
    
; EAX = Cpuid
; uCode Range: [ESI, EDI)
luCheckPatch:
    cmp     (UpdateHeaderStruc PTR [esi]).CpuSign, 53415252h     ; "RRAS"
    jnz      short luUnprogrammed
    cmp     (UpdateHeaderStruc PTR [esi]).MyCpuid, eax              ; Cpuid matched?
    jz      luFoundMatch

luCheckUnprogrammed:
    mov     ebx, (UpdateHeaderStruc PTR [esi]).DataSize						  ; ebx = current uC data size
    cmp     ebx, 0FFFFFFFFh                                         ; Current is not present?
    je      short luUnprogrammed
    cmp     (UpdateHeaderStruc PTR [esi]).LoaderRev, 1
    je      short luTryNextPatch 

luUnprogrammed:
    mov     ebx, 1024                         ; Unprogrammed space, 1KB checks
    jmp     short luPoinToNextBlock           ; for backword compatibility.

luTryNextPatch:
    mov     ebx, (UpdateHeaderStruc PTR [esi]).TotalSize
    or      ebx, ebx
    jnz     luPoinToNextBlock                 ; Variable size uCode format.
    mov     ebx, BLOCK_LENGTH_BYTES           ; Fixed size uCode format. 

luPoinToNextBlock:
    add     esi, ebx
    test    si,  1111b
    jz      short @f
    add     esi, 16
    and     si, 0FFF0h                        ; default 16 byte align
@@: cmp     esi, edi
    jb      luCheckPatch                      ; Check with all patches.

; All patch has been scanned.
    movd    eax, mm3
    movd    esi, mm4
    or      eax, eax
    jnz     luLoadPatch
    jmp     luExit                            ; No matching patch found.

luFoundMatch:

; MM3 = Patch date
; MM4 = Patch Pointer

; 4321 -> 2143
    mov   eax, (UpdateHeaderStruc PTR ds:[esi]).PatchDate
    mov   bx, ax
    shl   ebx, 16
    shr   eax, 16
    mov   bx, ax
    mov   eax, ebx                        ; Eax: PatchDate(YYYYMMDD)
    
    movd  ebx, mm3
    cmp   eax, ebx                        ; Current VS PreviousNewest
    jb    luTryNextPatch
    
    mov   ebx, eax
    movd  mm3, ebx                        ; save Patch Date
    movd  mm4, esi                        ; save Patch Address
    jmp   luTryNextPatch

luLoadPatch:
    mov   ecx, IA32_BIOS_UPDT_TRIG
    mov   eax, esi                        ; EAX - Abs addr of uCode patch.
    movd  mm1, eax                        ; Save microcode address for Mp.

    add   eax, SIZEOF(UpdateHeaderStruc)  ; EAX - Abs addr of uCode data.

    mov   edx, (UpdateHeaderStruc PTR [esi]).TotalSize
    movd  mm6, edx                        ; Save microcode size for Mp.
    xor   edx, edx                        ; EDX:EAX - Abs addr of uCode data.
    
    wrmsr                                 ; Trigger uCode load.

    xor  eax, eax
    inc  eax
    cpuid
    mov   ecx, 0x1205
    rdmsr
    cmp   al, 1
    jnz   path_err
    xor   eax, eax
    mov   ecx, 0x1a0
    rdmsr
    bts   eax, 16
    bts   eax, 20
    wrmsr
    jmp luExit
path_err:
;YKN-20161104 -S
    mov al, 0D3h    ;{ DXE_CPU_MICROCODE_UPDATE_FAILED, 0xD3 }
    out 80h, al
    jmp $
;YKN-20161104 -E
luExit:
    RET_MMX
VeryEarlyMicrocodeUpdate    ENDP






PlatformInitialization    PROC    NEAR    PRIVATE
; make sure PCIE base address and range is OK.
; (0,0,5,61)[7:0] = 0Eh, PCIE_BASE[35:28]
; (0,0,5,60)[1:0] = 11b

;En Acpi Io
;wcfg(0,11,0,88)[15:0] = 0x800
;bcfg(0,11,0,81)[7]    = 1
  mov   eax, 80000000h + (((0 shl 8) + (11h shl 3) + 0) shl 8) + 88h
  mov   dx,  0cf8h
  out   dx,  eax
  mov   dx,  0cfch
  mov   ax,  PcdGet16(AcpiIoPortBaseAddress)
  out   dx,  ax

  mov   eax, 80000000h + (((0 shl 8) + (11h shl 3) + 0) shl 8) + 80h
  mov   dx,  0cf8h
  out   dx,  eax
  mov   dx,  0cfch
  in    ax,  dx
  or    ax,  8000h
  out   dx,  ax
  ;MIKE_20161117 Modify Shell Or Set Up Crtl+Alt_Del_hang_s
  ; Check warm reset.
  ; if (0,0,6,9F)[0] = 1, issue a PCI reset. (skip if S3)
	mov   dx, PcdGet16(AcpiIoPortBaseAddress)
	in	  ax, dx
	test  ax, BIT15 				 ; WAK
	jz	  short CheckWarmReset
	add   dx, 4
	in	  ax, dx
	and   ax, 1C00h
	cmp   ax, 400h					 ; S3
	jz	  short SkipPciReset
  
  CheckWarmReset:
	mov   esi, PcdGet64(PcdPciExpressBaseAddress)
	add   esi, (((0 shl 8) + (0 shl 3) + 6) shl 12) + 9ch
	mov   eax, [esi]
	shr   eax, 24  
	test  al, BIT0
	jz	  short SkipPciReset
	
	mov   dx, 0cf9h
	mov   al, 06h
	out   dx, al
  
 SkipPciReset:

  ;SAD  Default Set
 ;(0,0,2,44) = 12h  (0,0,2,46) = 00h
  xor   eax,eax
  mov   eax, 80000000h + (((0 shl 8) + (0 shl 3) + 2) shl 8) + 44h
  mov   dx,  0cf8h
  out   dx,  eax
  xor   eax,eax
  mov   eax, 0012h
  mov   dx,  0cfch
  out   dx,  eax
 ;(0,0,2,4C) = C00fh;
  xor   eax,eax
  mov   eax, 80000000h + (((0 shl 8) + (0 shl 3) + 2) shl 8) + 4Ch
  mov   dx,  0cf8h
  out   dx,  eax
  xor   eax,eax
  mov   eax, 0c00fh
  mov   dx,  0cfch
  out   dx,  eax
  RET_MMX
PlatformInitialization    ENDP




CacheAsRam    PROC    NEAR    PRIVATE

; Ensure all APs are in the Wait for SIPI state.
   mov     edi, APIC_ICR_LO               ; 0FEE00300h - Send INIT IPI to all excluding self 
   mov     eax, ORAllButSelf + ORSelfINIT ; 000C4500h
   mov     [edi], eax
@@:mov     eax, [edi]
   bt      eax, 12                        ; Check if send is in progress
   jc      short @B                       ; Loop until idle


   mov   ecx, IA32_MTRR_CAP
   rdmsr
   movzx ebx, al
   shl   ebx, 2
   add   ebx, MtrrCountFixed * 2

; EBX = size of Fixed and Variable MTRRs.
; Clear all MTRRs.
   xor   eax, eax                       
   xor   edx, edx                       
InitMtrrLoop:
   add   ebx, -2
   movzx ecx, WORD PTR cs:MtrrInitTable[ebx]
   wrmsr
   jnz   short InitMtrrLoop
  

; Configure the default memory type to UC in the IA32_MTRR_DEF_TYPE
  mov     ecx, MTRR_DEF_TYPE
  rdmsr
  and     eax, NOT (00000CFFh)
  wrmsr
  
  mov     eax, 80000008h      ; Physical address space size [7:0]
  cpuid  
  sub     al, 32
  movzx   eax, al
  xor     esi, esi
  bts     esi, eax
  dec     esi									; mask above 32 bit

  mov     eax, PcdGet32(PcdTemporaryRamBase)
  or      eax, MTRR_MEMORY_TYPE_WB
  xor     edx, edx 
  mov     ecx, MTRR_PHYS_BASE_0 
  wrmsr 
  
; Compute MTRR mask value:  Mask = NOT (Size - 1)
  mov     eax, PcdGet32(PcdTemporaryRamSize)
  dec     eax
  not     eax
  or      eax, MTRR_PHYS_MASK_VALID
  mov     edx, esi
  mov     ecx, MTRR_PHYS_MASK_0
  wrmsr

  mov     eax, PcdGet32(PcdFlashAreaSize)
  mov     edi, PcdGet32(PcdFlashAreaBaseAddress)

  ; Round up to page size
  mov     ecx, eax                      ; Save
  and     ecx, 0FFFF0000h               ; Number of pages in 64K
  and     eax, 0FFFFh                   ; Number of "less-than-page" bytes
  jz      short Rounded
  mov     eax, 10000h                   ; Add the whole page size
Rounded:
  add     eax, ecx                      ; eax - rounded up code cache size

; Define "local" vars for this routine
  NEXT_MTRR_SIZE        TEXTEQU  <mm2>
  CODE_SIZE_TO_CACHE    TEXTEQU  <mm3>
  CODE_BASE_TO_CACHE    TEXTEQU  <mm4>
  NEXT_MTRR_INDEX       TEXTEQU  <mm5>

  xor     ecx, ecx
  movd    NEXT_MTRR_INDEX, ecx          ; Count from 0 but start from MTRR_PHYS_BASE_1

; eax: bios size
; edi: bios base address
  movd    CODE_SIZE_TO_CACHE, eax
  movd    CODE_BASE_TO_CACHE, edi

NextMtrr:
  movd    eax, CODE_SIZE_TO_CACHE       ; remaining size
  and     eax, eax
  jz      CodeRegionMtrrdone
  ;
  ; Determine next size to cache.
  ; We start from bottom up. Use the following algorythm:
  ; 1. Get our own alignment. Max size we can cache equals to our alignment
  ; 2. Determine what is bigger - alignment or remaining size to cache.
  ;    If aligment is bigger - cache it.
  ;      Adjust remaing size to cache and base address
  ;      Loop to 1.
  ;    If remaining size to cache is bigger
  ;      Determine the biggest 2^N part of it and cache it.
  ;      Adjust remaing size to cache and base address
  ;      Loop to 1.
  ; 3. End when there is no left size to cache or no left MTRRs
  ;
  movd    edi, CODE_BASE_TO_CACHE
  bsf     ecx, edi                      ; Get index of lowest bit set in base address
  ;
  ; Convert index into size to be cached by next MTRR
  ;
  mov     edx, 1h
  shl     edx, cl                       ; Alignment is in edx
  cmp     edx, eax                      ; What is bigger, alignment or remaining size?
  jbe     short gotSize                 ; JIf aligment is less
  ;
  ; Remaining size is bigger. Get the biggest part of it, 2^N in size
  ;
  bsr     ecx, eax                      ; Get index of highest set bit
  ;
  ; Convert index into size to be cached by next MTRR
  ;
  mov     edx, 1
  shl     edx, cl                       ; Size to cache

GotSize:
  mov     eax, edx
  movd    NEXT_MTRR_SIZE, eax           ; Save

  ;
  ; Compute MTRR mask value:  Mask = NOT (Size - 1)
  ;
  dec     eax                           ; eax - size to cache less one byte
  not     eax                           ; eax contains low 32 bits of mask
  or      eax, MTRR_PHYS_MASK_VALID     ; Set valid bit

  ;
  ; Program mask register
  ;
  mov     ecx, MTRR_PHYS_MASK_1         ; setup variable mtrr
  movd    ebx, NEXT_MTRR_INDEX
  add     ecx, ebx
  mov     edx, esi                      ; edx <- MTRR_PHYS_MASK_HIGH
  wrmsr
  ;
  ; Program base register
  ;
  sub     edx, edx
  mov     ecx, MTRR_PHYS_BASE_1         ; setup variable mtrr
  add     ecx, ebx                      ; ebx is still NEXT_MTRR_INDEX

  movd    eax, CODE_BASE_TO_CACHE
  or      eax, MTRR_MEMORY_TYPE_WP      ; set type to write protect
  wrmsr
  ;
  ; Advance and loop
  ; Reduce remaining size to cache
  ;
  movd    ebx, CODE_SIZE_TO_CACHE
  movd    eax, NEXT_MTRR_SIZE
  sub     ebx, eax
  movd    CODE_SIZE_TO_CACHE, ebx

  ;
  ; Increment MTRR index
  ;
  movd    ebx, NEXT_MTRR_INDEX
  add     ebx, 2
  movd    NEXT_MTRR_INDEX, ebx
  ;
  ; Increment base address to cache
  ;
  movd    ebx, CODE_BASE_TO_CACHE 
  movd    eax, NEXT_MTRR_SIZE
  add     ebx, eax
  movd    CODE_BASE_TO_CACHE, ebx 

  jmp     NextMtrr

CodeRegionMtrrdone:
; Enable the MTRRs by setting the IA32_MTRR_DEF_TYPE MSR E flag.
  mov     ecx, MTRR_DEF_TYPE            ; Load the MTRR default type index
  rdmsr
  or      eax, MTRR_DEF_TYPE_E          ; Enable variable range MTRRs
  wrmsr
  ;
  ;   Enable the logical processor's (BSP) cache: execute INVD and set 
  ;   CR0.CD = 0, CR0.NW = 0.
  ;
  mov     eax, cr0
  and     eax, NOT (CR0_CACHE_DISABLE + CR0_NO_WRITE)
  invd
  mov     cr0, eax
  ;
  ;   One location in each 64-byte cache line of the DataStack region
  ;   must be written to set all cache values to the modified state.
  ;
  mov     edi, PcdGet32(PcdTemporaryRamBase)
  mov     ecx, PcdGet32(PcdTemporaryRamSize)
  shr     ecx, 6
  mov     eax, CACHE_INIT_VALUE
@@:
  mov     [edi], eax
  sfence
  add     edi, 64
  loopd   short @b
  cld
  mov     edi, PcdGet32(PcdTemporaryRamBase)
  mov     ecx, PcdGet32(PcdTemporaryRamSize) 
  shr     ecx, 2
  mov     eax, CACHE_TEST_VALUE
TestDataStackArea:
  stosd
  cmp     eax, DWORD PTR [edi-4]
  jnz     short DataStackTestFail
  loop    TestDataStackArea 
  jmp     short DataStackTestPass

DataStackTestFail:
  STATUS_CODE (0D0h)
  jmp     $

ConfigurationTestFailed:
  STATUS_CODE (0D1h)
  jmp     $

DataStackTestPass:

  RET_MMX
CacheAsRam    ENDP





CallPeiCoreEntryPoint   PROC    NEAR    PRIVATE

  ; Switch to "C" code
  STATUS_CODE (0Ch)
  
  mov     esp, PcdGet32(PcdTemporaryRamBase)
  add     esp, PcdGet32(PcdTemporaryRamSize)  

  rdtsc
  push    eax         ; JmpSecCoreTsc
  
  movd    eax, mm0		; ResetTsc
  push    eax

  movd    eax, mm6    ;MicroCodeSize
  push    eax
  
  movd    eax, mm1    ; MicroCode
  push    eax

  mov     edi, PcdGet32(PcdFlashFvRecoveryBase)
  cmp     dword ptr [edi+40], 'HVF_'
  jz      short FvBBOK
  mov     edi, PcdGet32(PcdFlashFvMainBase)

FvBBOK:
  push    edi
  push    PcdGet32(PcdTemporaryRamBase)
  push    PcdGet32(PcdTemporaryRamSize)
  call    SecStartup
  
CallPeiCoreEntryPoint   ENDP



    


MtrrInitTable   LABEL BYTE
    DW  MTRR_DEF_TYPE
    DW  MTRR_FIX_64K_00000
    DW  MTRR_FIX_16K_80000
    DW  MTRR_FIX_16K_A0000
    DW  MTRR_FIX_4K_C0000
    DW  MTRR_FIX_4K_C8000
    DW  MTRR_FIX_4K_D0000
    DW  MTRR_FIX_4K_D8000
    DW  MTRR_FIX_4K_E0000
    DW  MTRR_FIX_4K_E8000
    DW  MTRR_FIX_4K_F0000
    DW  MTRR_FIX_4K_F8000

MtrrCountFixed EQU (($ - MtrrInitTable) / 2)

    DW  MTRR_PHYS_BASE_0
    DW  MTRR_PHYS_MASK_0
    DW  MTRR_PHYS_BASE_1
    DW  MTRR_PHYS_MASK_1
    DW  MTRR_PHYS_BASE_2
    DW  MTRR_PHYS_MASK_2
    DW  MTRR_PHYS_BASE_3
    DW  MTRR_PHYS_MASK_3
    DW  MTRR_PHYS_BASE_4
    DW  MTRR_PHYS_MASK_4
    DW  MTRR_PHYS_BASE_5
    DW  MTRR_PHYS_MASK_5
    DW  MTRR_PHYS_BASE_6
    DW  MTRR_PHYS_MASK_6
    DW  MTRR_PHYS_BASE_7
    DW  MTRR_PHYS_MASK_7
    DW  MTRR_PHYS_BASE_8      
    DW  MTRR_PHYS_MASK_8              
    DW  MTRR_PHYS_BASE_9             
    DW  MTRR_PHYS_MASK_9              
MtrrCount      EQU (($ - MtrrInitTable) / 2)







align 10h
PUBLIC  BootGDTtable

;
; GDT[0]: 0x00: Null entry, never used.
;
NULL_SEL        EQU $ - GDT_BASE        ; Selector [0]
GDT_BASE:
BootGDTtable        DD  0
                    DD  0
;
; Linear data segment descriptor
;
LINEAR_SEL      EQU $ - GDT_BASE        ; Selector [0x8]
    DW  0FFFFh                          ; limit 0xFFFFF
    DW  0                               ; base 0
    DB  0
    DB  092h                            ; present, ring 0, data, expand-up, writable
    DB  0CFh                            ; page-granular, 32-bit
    DB  0
;
; Linear code segment descriptor
;
LINEAR_CODE_SEL EQU $ - GDT_BASE        ; Selector [0x10]
    DW  0FFFFh                          ; limit 0xFFFFF
    DW  0                               ; base 0
    DB  0
    DB  09Bh                            ; present, ring 0, data, expand-up, not-writable
    DB  0CFh                            ; page-granular, 32-bit
    DB  0
;
; System data segment descriptor
;
SYS_DATA_SEL    EQU $ - GDT_BASE        ; Selector [0x18]
    DW  0FFFFh                          ; limit 0xFFFFF
    DW  0                               ; base 0
    DB  0
    DB  093h                            ; present, ring 0, data, expand-up, not-writable
    DB  0CFh                            ; page-granular, 32-bit
    DB  0

;
; System code segment descriptor
;
SYS_CODE_SEL    EQU $ - GDT_BASE        ; Selector [0x20]
    DW  0FFFFh                          ; limit 0xFFFFF
    DW  0                               ; base 0
    DB  0
    DB  09Ah                            ; present, ring 0, data, expand-up, writable
    DB  0CFh                            ; page-granular, 32-bit
    DB  0
;
; Spare segment descriptor
;
SYS16_CODE_SEL  EQU $ - GDT_BASE        ; Selector [0x28]
    DW  0FFFFh                          ; limit 0xFFFFF
    DW  0                               ; base 0
    DB  0Eh                             ; Changed from F000 to E000.
    DB  09Bh                            ; present, ring 0, code, expand-up, writable
    DB  00h                             ; byte-granular, 16-bit
    DB  0
;
; Spare segment descriptor
;
SYS16_DATA_SEL  EQU $ - GDT_BASE        ; Selector [0x30]
    DW  0FFFFh                          ; limit 0xFFFF
    DW  0                               ; base 0
    DB  0
    DB  093h                            ; present, ring 0, data, expand-up, not-writable
    DB  00h                             ; byte-granular, 16-bit
    DB  0

;
; Spare segment descriptor
;
SPARE5_SEL      EQU $ - GDT_BASE        ; Selector [0x38]
    DW  0                               ; limit 0
    DW  0                               ; base 0
    DB  0
    DB  0                               ; present, ring 0, data, expand-up, writable
    DB  0                               ; page-granular, 32-bit
    DB  0
GDT_SIZE        EQU $ - BootGDTtable    ; Size, in bytes

GdtDesc:                                ; GDT descriptor
OffsetGDTDesc   EQU $ - _ModuleEntryPoint
    DW  GDT_SIZE - 1                    ; GDT limit
    DD  OFFSET BootGDTtable             ; GDT base address

NemInitLinearAddress   LABEL   FWORD
NemInitLinearOffset    LABEL   DWORD
    DD  OFFSET ProtectedModeSECStart    ; Offset of our 32 bit code
    DW  LINEAR_CODE_SEL

_TEXT_PROTECTED_MODE    ENDS
END
