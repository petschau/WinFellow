;==============================================================================
; Fellow Amiga Emulator
; UAE support memory access wrappers
; Copyright (C) 1999 Petter Schau
;==============================================================================

%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

%define UAESUPP_ASM

FASMFILESTART
FDATASECTIONSTART

%include "data/fmem.inc"

FDATASECTIONEND
FCODESECTIONSTART

%include "func/uaesupp.inc"


;==============================================================================
; Write data to rtarea
;
; Assembly calling convention:
; In:  edx - Data, ecx - Address
; Out: Nothing
;==============================================================================

%macro RTAREA_BPUT_CWRAP 0
		mov	ecx, dword [memory_wriorgadr]
		CCALLCONV_IN_EDXECX
		call	rtarea_bputC
		CCALLCONV_OUT_NONE	2
		mov	ecx, dword [memory_wriorgadr]
%endmacro

%macro RTAREA_LPUT_CWRAP 0
		mov	ecx, dword [memory_wriorgadr]
		CCALLCONV_IN_EDXECX
		call	rtarea_lputC
		CCALLCONV_OUT_NONE	2
		mov	ecx, dword [memory_wriorgadr]
%endmacro


;==============================================================================
; Read data from rtarea
;
; Assembly calling convention:
; In:  ecx - Address
; Out: edx - data
;==============================================================================

%macro RTAREA_BGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	rtarea_bgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro

%macro RTAREA_WGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	rtarea_wgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro

%macro RTAREA_LGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	rtarea_lgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro


;==============================================================================
; Write data to filesys
;
; Assembly calling convention:
; In:  edx - Data, ecx - Address
; Out: Nothing
;==============================================================================

%macro FILESYS_BPUT_CWRAP 0
		mov	ecx, dword [memory_wriorgadr]
		CCALLCONV_IN_EDXECX
		call	filesys_bputC
		CCALLCONV_OUT_NONE	2
		mov	ecx, dword [memory_wriorgadr]
%endmacro

%macro FILESYS_WPUT_CWRAP 0
		mov	ecx, dword [memory_wriorgadr]
		CCALLCONV_IN_EDXECX
		call	filesys_wputC
		CCALLCONV_OUT_NONE	2
		mov	ecx, dword [memory_wriorgadr]
%endmacro

%macro FILESYS_LPUT_CWRAP 0
		mov	ecx, dword [memory_wriorgadr]
		CCALLCONV_IN_EDXECX
		call	filesys_lputC
		CCALLCONV_OUT_NONE	2
		mov	ecx, dword [memory_wriorgadr]
%endmacro


;==============================================================================
; Read data from filesys
;
; Assembly calling convention:
; In:  ecx - Address
; Out: edx - data
;==============================================================================

%macro FILESYS_BGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	filesys_bgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro

%macro FILESYS_WGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	filesys_wgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro

%macro FILESYS_LGET_CWRAP 0
		push	ecx
		CCALLCONV_IN_ECX
		call	filesys_lgetC
		CCALLCONV_OUT_EDX	1
		pop	ecx
%endmacro


;==============================================================================
; rtarea memory access wrappers, calls routines in autoconf.c
;==============================================================================


		FALIGN32

global _rtarea_bgetASM_
_rtarea_bgetASM_:
		RTAREA_BGET_CWRAP
		ret


		FALIGN32

global _rtarea_wgetASM_
_rtarea_wgetASM_:
		RTAREA_WGET_CWRAP
		ret


		FALIGN32

global _rtarea_lgetASM_
_rtarea_lgetASM_:
		RTAREA_LGET_CWRAP
		ret


		FALIGN32

global _rtarea_bputASM_
_rtarea_bputASM_:
		RTAREA_BPUT_CWRAP
		ret


		FALIGN32

global _rtarea_lputASM_
_rtarea_lputASM_:
		RTAREA_LPUT_CWRAP
		ret


;==============================================================================
; filesys memory access wrappers, calls routines in expansio.c
;==============================================================================


		FALIGN32

global _filesys_bgetASM_
_filesys_bgetASM_:
		FILESYS_BGET_CWRAP
		ret


		FALIGN32

global _filesys_wgetASM_
_filesys_wgetASM_:
		FILESYS_WGET_CWRAP
		ret


		FALIGN32

global _filesys_lgetASM_
_filesys_lgetASM_:
		FILESYS_LGET_CWRAP
		ret


		FALIGN32

global _filesys_bputASM_
_filesys_bputASM_:
		FILESYS_BPUT_CWRAP
		ret


		FALIGN32

global _filesys_wputASM_
_filesys_wputASM_:
		FILESYS_WPUT_CWRAP
		ret


		FALIGN32

global _filesys_lputASM_
_filesys_lputASM_:
		FILESYS_LPUT_CWRAP
		ret


		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
