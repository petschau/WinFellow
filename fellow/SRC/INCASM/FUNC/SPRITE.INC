%ifndef FUNC_SPRITE_INC
%define FUNC_SPRITE_INC

;==============================================================================
; Includes functions declared in the C graph module
;==============================================================================

%ifndef SPRITE_ASM

extern _spritesMerge_

%endif

CFUNCEXTERN spriteEndOfFrame
CFUNCEXTERN spriteLogActiveSprites
CFUNCEXTERN spritesLog
CFUNCEXTERN spriteMergeHAM
CFUNCEXTERN spriteMergeHAM1x8
CFUNCEXTERN spriteMergeHAM2x8
CFUNCEXTERN spriteMergeHAM1x16
CFUNCEXTERN spriteMergeHAM2x16
CFUNCEXTERN spriteMergeHAM1x24
CFUNCEXTERN spriteMergeHAM2x24
CFUNCEXTERN spriteMergeHAM1x32
CFUNCEXTERN spriteMergeHAM2x32

%endif
