%ifndef FUNC_FMEM_INC
%define FUNC_FMEM_INC

;==============================================================================
; Includes functions declared in the C fmem module
;==============================================================================

CFUNCEXTERN memoryEmemCardsReset

CFUNCEXTERN memoryEmemReadByteC
CFUNCEXTERN memoryEmemReadWordC
CFUNCEXTERN memoryEmemReadLongC
CFUNCEXTERN memoryEmemWriteByteC
CFUNCEXTERN memoryEmemWriteWordC
CFUNCEXTERN memoryEmemWriteLongC

CFUNCEXTERN memoryDmemWriteLongC
CFUNCEXTERN memoryLogUndefinedIOWrites
CFUNCEXTERN memoryLogUndefinedIOReads


;==============================================================================
; Includes functions declared in the asm fmem module
;==============================================================================


; defined in fmem.s to avoid including these functions in that file

%ifndef FMEM_ASM

extern fetb
extern fetw
extern fetl
extern wrib
extern wriw
extern wril
extern oddread
extern oddwrite

%endif

%endif
