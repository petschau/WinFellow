;===============================================================================
; Fellow Amiga Emulator
; SSE detection
;
; Adapted from example in Intel SSE manual
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
; Detect SSE                            
; Taken from the Intel manual for their SSE implementation 
; Returns TRUE (1) or FALSE (0), prepared for use from C.
;
; This procedure first checks if CPU supports the CPUID instruction by checking
; bit 21 in the EFLAGS register (32-bit wide). Bit 21 holds the Identification 
; (ID) flag. The ability of a program to set or clear this flag indicates 
; support for the CPUID instruction.
;
; If supported, we execute the CPUID instruction and this places the processor 
; features info into the EDX register. We check if bit 25 is set and this 
; bit will be set to 1 if SSE is supported.
;
;===============================================================================


		FALIGN32

global _detectSSE_
_detectSSE_:
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
		jz      .no_sse
		mov     eax, 1
		cpuid
		test    edx, 02000000h
		jz      .no_sse
		popad
		mov     eax, 1
		jmp     .outt
.no_sse:	
		popad
		xor     eax, eax
.outt:		
		ASMCALLCONV_OUT_EAX
		ret



		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
