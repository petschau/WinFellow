%ifndef FUNC_CIA_INC
%define FUNC_CIA_INC

;==============================================================================
; Includes functions declared in the C cia module
;==============================================================================

CFUNCEXTERN ciaRaiseIRQC             ; Used in keyboard.mac
CFUNCEXTERN ciaHandleEventC          ; Used in busa.asm
CFUNCEXTERN ciaUpdateEventCounterC   ; Used in busa.asm
CFUNCEXTERN ciaUpdateTimersEOFC      ; Used in busa.asm
CFUNCEXTERN ciaReadByteC
CFUNCEXTERN ciaReadWordC
CFUNCEXTERN ciaReadLongC
CFUNCEXTERN ciaWriteByteC
CFUNCEXTERN ciaWriteWordC
CFUNCEXTERN ciaWriteLongC

;==============================================================================
; Includes functions declared in the asm Cia Module
;==============================================================================

%ifndef CIA_ASM


%endif


%endif
