;===============================================================================
; Fellow Amiga Emulator
; MMX detection
;
; Adapted from example in AMD MMX manual
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


;===============================================================================
; Detect MMX                            
; Taken from the AMD K6 manual for their MMX implementation 
; Returns TRUE (1) or FALSE (0), prepared for use from C.
;
; This procedure first checks if CPU supports the CPUID instruction by checking
; bit 21 in the EFLAGS register (32-bit wide). Bit 21 holds the Identification 
; (ID) flag. The ability of a program to set or clear this flag indicates 
; support for the CPUID instruction.
;
; If supported, we execute the CPUID instruction and this places the processor 
; features info into the EDX register. We check if bit 23 is set and this 
; bit will be set to 1 if MMX is supported.
;
;===============================================================================


		FALIGN32

global _detectMMX_
_detectMMX_:
		ASMCALLCONV_IN_NONE
		pushad
		pushfd
		pop	eax
		mov     ebx, eax
		xor     eax, 00200000h
		push    eax
		popfd
		pushfd
		pop     eax
		cmp     eax, ebx
		jz      .no_mmx
		mov     eax, 1
		cpuid
		test    edx, 0800000h
		jz      .no_mmx
		popad
		mov     eax, 1
		jmp     .outt
.no_mmx:	popad
		xor     eax, eax
.outt:		
		ASMCALLCONV_OUT_EAX
		ret



		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
