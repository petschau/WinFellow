%ifndef FUNC_FLOPPY_INC
%define FUNC_FLOPPY_INC

;==============================================================================
; Includes functions declared in the C floppy module
;==============================================================================

CFUNCEXTERN floppyDMARead
CFUNCEXTERN floppyDMAWrite
CFUNCEXTERN rdskbytrC
CFUNCEXTERN wdskpthC
CFUNCEXTERN wdskptlC
CFUNCEXTERN wdsklenC
CFUNCEXTERN wdsksyncC

%endif
