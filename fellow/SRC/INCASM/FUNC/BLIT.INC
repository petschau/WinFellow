%ifndef FUNC_BLIT_INC
%define FUNC_BLIT_INC

;==============================================================================
; Includes functions declared in the asm blit module
;==============================================================================

%ifndef BLITTER_ASM
extern _finish_blit_
extern _blitter_copy_
%endif

CFUNCEXTERN blitterOperationLog

%endif
