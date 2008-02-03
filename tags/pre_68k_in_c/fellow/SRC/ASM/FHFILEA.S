;===============================================================================
; Fellow Amiga Emulator
; Hardfile device memory access wrappers
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; The hardfile unit is implemented in C, this file contains some
; routines to hook it up to the memory system.
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "generic/defs.inc"

%define FHFILE_ASM

FASMFILESTART
FDATASECTIONSTART

%include "data/fhfile.inc"

FDATASECTIONEND
FCODESECTIONSTART


;==============================================================================
; Hardfile device memory access stubs
;==============================================================================


		FALIGN32

global _fhfileReadByte_
_fhfileReadByte_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	dl, byte [fhfile_rom + ecx]		
		pop	ecx
		ret


		FALIGN32

global _fhfileReadWord_
_fhfileReadWord_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	dh, byte [fhfile_rom + ecx]		
		mov	dl, byte [fhfile_rom + 1 + ecx]		
		pop	ecx
		ret


		FALIGN32

global _fhfileReadLong_
_fhfileReadLong_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	edx, dword [fhfile_rom + ecx]
		pop	ecx
		bswap	edx
		ret



		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
