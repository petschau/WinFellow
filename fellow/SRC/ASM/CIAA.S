;===============================================================================
; Fellow Amiga Emulator
; CIA memory access wrappers
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; The CIA chip implementation is C-code only. The code here is some stubs needed
; to hook the CIA registers into the memory system.
;
; * CIA memory access handlers. Forwards everything to C-code.
; 
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

%define CIA_ASM

FASMFILESTART
FDATASECTIONSTART

%include "data/fmem.inc"
%include "data/bus.inc"
%include "data/copper.inc"
%include "data/cia.inc"
%include "data/cpu.inc"

FDATASECTIONEND
FCODESECTIONSTART


%include "func/cia.inc"
%include "func/copper.inc"
%include "func/bus.inc"
%include "func/cpu.inc"

%include "mac/cia.mac"
%include "mac/bus.mac"

;==============================================================================
; CIA memory access wrappers, calls routines in cia.c
;==============================================================================


		FALIGN32

global _ciaWriteByte_
_ciaWriteByte_:
		CIAWRITEBYTE_CWRAP
		ret


		FALIGN32

global _ciaWriteWord_
_ciaWriteWord_:
		CIAWRITEWORD_CWRAP
		ret


		FALIGN32

global _ciaWriteLong_
_ciaWriteLong_:
		CIAWRITELONG_CWRAP
		ret


		FALIGN32

global _ciaReadByte_
_ciaReadByte_:
		CIAREADBYTE_CWRAP
		ret


		FALIGN32

global _ciaReadWord_
_ciaReadWord_:
		CIAREADWORD_CWRAP
		ret


		FALIGN32

global _ciaReadLong_
_ciaReadLong_:
		CIAREADLONG_CWRAP
		ret


FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
