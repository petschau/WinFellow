%ifndef FUNC_DRAW_INC
%define FUNC_DRAW_INC


;==============================================================================
; Includes functions declared in the C draw module
;==============================================================================

CFUNCEXTERN drawValidateBufferPointer
CFUNCEXTERN drawInvalidateBufferPointer
CFUNCEXTERN drawEndOfFramePostC


;==============================================================================
; Includes functions declared in the asm draw module
;==============================================================================

%ifndef DRAW_ASM

extern _drawEndOfFrame_

%endif

%endif