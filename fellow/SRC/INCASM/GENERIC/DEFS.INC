;==============================================================================
; Fellow Amiga Emulator
; Compile options for conditional code selection
; Copyright (C) 1998 Petter Schau
;==============================================================================

%ifndef DEFS_INC
%define DEFS_INC

;================================
; Disable some or all OS drivers
;================================

%define WITH_OS_TIMER


;==============================================================================
; Enable UAE filesystem
;==============================================================================

%define UAE_FILESYS


;======================
; New sprite emulation
;======================

;%define USE_NEW_SPRITE


;=====================================================================
; Use graphics emulation in C, replaces USE_C some time in the future 
;=====================================================================

;%define GRAPHEM_C

;=======================================================
; Use graphics and drawing routines written in assembly
; C counterpart is USE_C, only one must be defined.
;=======================================================

%define USE_ASM

;==========================================
; Use M68k prefetch emulation
; For the moment not compatible with PC_PTR
;==========================================

;%define PREFETCH

;=========================
; Direct memory PC-pointer
;=========================

%define PC_PTR

;=====================
; Don't touch the rest
;=====================

%define TSC_PROFILING


%macro TSC_BEFORE 1
%ifdef TSC_PROFILING
                push    eax
                push    edx
                push    ecx
                mov     ecx,10h
                rdtsc
                pop     ecx
                mov     dword [%1], eax
                mov     dword [%1 + 4], edx
                pop     edx
                pop     eax
%endif
%endmacro


; TMPVARIABLE VARIABLE TIMES

%macro TSC_AFTER 3
%ifdef TSC_PROFILING
                push    eax
                push    edx
                push    ecx
                mov     ecx, 10h
                rdtsc
                pop     ecx
                sub     eax, dword [%1]
                sbb     edx, dword [%1 + 4]
                add     dword [%2], eax
                adc     dword [%2 + 4], edx
                inc     dword [%3]
                pop     edx
                pop     eax
%endif
%endmacro


%define CHIPMEM 0200000h
%define FASTMEM 0800000h
%define BOGOMEM 01c0000h
%define KICKMEM 0080000h


%define CYCLESPERLINE 228
%define CYCLESPERFRAME 71364
%define LINESPERFRAME 313

%endif