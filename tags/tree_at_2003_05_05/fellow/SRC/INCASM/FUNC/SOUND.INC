%ifndef FUNC_SOUND_INC
%define FUNC_SOUND_INC

;==============================================================================
; Includes functions declared in the C sound module
;==============================================================================

CFUNCEXTERN soundEmulationStart
CFUNCEXTERN soundEmulationStop
CFUNCEXTERN soundEndOfLine


;==============================================================================
; Includes functions declared in the asm sound module
;==============================================================================

extern _soundChannelKill_
extern _soundState0_

%endif