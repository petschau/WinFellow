;===============================================================================
; Fellow Amiga Emulator      
; 680X0 emulation core		   
;
; Authors: Petter Schau (peschau@online.no)
;          Roman Dolejsi
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; * Stack frame generation for 680X0
; * Status register set function (to catch side effects).
; * IRQ and exception setup and execution code.
; * Effective address calculation and fetch / write code.
; * Branch evaluation functions.
; * Implementation of all 000/010 instructions and all relevant 020/030
; 
; State of this code:
; * Works, not so sure about all the 0X0 stuff (for X > 0). It does not
;   break immediately when you throw 0X0 code at it, but users have not
;   been very responsive towards this extension to the code either.
; * There are many optimizations that could be implemented to improve
;   it. The drawback is that it is handwritten, huge and likely to
;   break if you start juggling code around.
;   Maybe better to replace it with a new design, if you're up to it.
; * A replacement could use a source-code generator. I have experimented with
;   that approach and found assembly code generated to run 33-70% faster
;   if you lay it out carefully, and avoid the duplicated EA evaluation
;   that the code in this file does.
;   And that is before enabling some additional simplifications for the 
;   status register. Unfortunately it was only a few instructions to sample
;   the concept. I present it here as an idea.
;===============================================================================


%define ASM_CPU_ASM

%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"
	
FASMFILESTART
	
FDATASECTIONSTART

%include "data/fmem.inc"

%include "data/cpu.inc"
%include "data/bus.inc"
%include "data/copper.inc"

			
upsrtraceset: db 0
cpu_cycles: dd 0

rte_stackinc:	dd	0, 0, 4, 4, 8, 0, 0, 52, 50, 10, 24, 84, 16, 18, 0, 0

FDATASECTIONEND
FCODESECTIONSTART
FALIGNHELPER

; Note:	 irq's do not clear trace bits.	 Should they?

global _m68000start_
_m68000start_:
	
%include "func/fmem.inc"
%include "func/bus.inc"
%include "func/copper.inc"
%include "func/cpu.inc"
;%include "func/fellow.inc"

CFUNCEXTERN fellowSoftReset;

%include "mac/fmem.mac"
%include "mac/bus.mac"
%include "mac/cpu.mac"

CFUNCEXTERN busScanEventsLevel5

%ifdef PREFETCH

; Load prefetch word from PC
; Called at reset, irq, exception and jumps

; edx - PC to prefetch



		FALIGN32

global prefetch_fillASM
prefetch_fillASM:
%ifdef PC_PTR
		push	ecx
		mov	ch, byte [edx]
		mov	cl, byte [edx + 1]
		mov	word [prefetch_word], cx
		pop	ecx
%else
		push	ecx
		push	edx
		mov	ecx, edx
		fetchw
		mov	word [prefetch_word], dx
		pop	edx
		pop	ecx
%endif
		ret
%endif

;========================================================================
; Group 1 Frame format
;
; 000:	All, except bus and address error
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
;========================================================================
	
%macro GROUP1 0
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew		
%endmacro


		FALIGN32

global _cpuGroup1_
_cpuGroup1_:
		GROUP1
		ret


;========================================================================
; Group 2 Frame format
;
; 000:	Bus and address error
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
; memory_fault_address contains the violating address
; memory_fault_read is TRUE if the access was a read
;========================================================================
	
%macro GROUP2 0
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
		mov	ecx, dword [da_regs + 60]	; Fault addr, skip ireg
		sub	ecx, 6			 
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [memory_fault_address]
		writel
		mov	edx, dword [memory_fault_read]
		shl	edx, 4
		mov	ecx, dword [da_regs + 60]
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		writel
%endmacro


		FALIGN32

global _cpuGroup2_
_cpuGroup2_:
		GROUP2
		ret


;========================================================================
; Frame format $0, four word frame
;
; Stack words:
; ------------
; SR
; PCHI
; PCLO
; 0000 Vector no. (4 upper bits are frame no., rest is vector no.)
;
; 010:	All, except bus and address errors
; 020:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, copr preinst
; 030:	Same as for 020
; 040:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, FPU preinst
; 060:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, FPU preinst, Unimpl. Integer, Unimpl. EA
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
;========================================================================
	
%macro FRAME0 1		; VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector word 
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame0_
_cpuFrame0_:	FRAME0	ebx
		ret


;========================================================================
; Frame format $1, 4 word throwaway frame
;
; Stack words:
; ------------
; SR
; PCHI
; PCLO
; 0000 Vector no. (4 upper bits are frame no., rest is vector no.)
;
; 020:	Irq, second frame created
; 030:	Same as for 020
; 040:	Same as for 020
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
;========================================================================
	
%macro FRAME1 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector word 
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 01000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame1_
_cpuFrame1_:
		FRAME1	ebx
		ret


;========================================================================
; Frame format $2
;
; 020:	chk, chk2, cpTrapcc, trapcc, trapv, trace, zero divide, MMU config,
;	copr postinst 
; 030:	Same as for 020
; 040:	chk, chk2, FTrapcc, trapcc, trapv, trace, zero divide, address error,
;	Unimplemented FPU inst. 
; 060:	Same as for 040
;
; Assumes esi is loaded with correct pc ptr wherever it should point,
; that SR has been updated to whatever it should contain
; and that _pc contains the start of the current instruction
;========================================================================
	
%macro FRAME2 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save inst address 
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [pc]
		writel
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 02000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame2_
_cpuFrame2_:
		FRAME2	ebx
		ret


;========================================================================
; Frame format $3
;
; 040:	FPU post inst.
; 060:	Same as for 040
;
; Fellow never generates this frame
;========================================================================
	
%macro FRAME3 1		; VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 6
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 03000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		writel
		PTR_TO_PC	edx
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame3_
_cpuFrame3_:
		FRAME3	ebx
		ret

	
;========================================================================
; Frame format $4
;
; 040:	Unimplemented FPU inst. on EC040 and LC040
; 060:	FPU Disabled, Bus error
;
;========================================================================
	
%macro FRAME4 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 10
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 04000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame4_
_cpuFrame4_:
		FRAME4	ebx
		ret


;========================================================================
; Frame format $7
;
; 040:	Address error for non-inst word accesses
;
;========================================================================
	
%macro FRAME7 1		; VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 54
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 07000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame7_
_cpuFrame7_:
		FRAME7	ebx
		ret


;========================================================================
; Frame format $8
;
; 010:	Bus and address error
;
;========================================================================
	
%macro FRAME8 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 52
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 08000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro

				
		FALIGN32

global _cpuFrame8_
_cpuFrame8_:
		FRAME8	ebx
		ret


;============================================================================
; Frame format $9
;
; 020:	copr midinst, main detected protocol violation, irq during copr inst
; 030:	Same as for 020
;
; Not generated in Fellow
;========================================================================
	
%macro FRAME9 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save inst address 
		sub	ecx, 12
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [pc]
		writel
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 09000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrame9_
_cpuFrame9_:
		FRAME9	ebx
		ret


;========================================================================
; Frame format $A
;
; 020:	Address or bus-error on instruction boundrary
; 030:	Same as for 020
;
; Assumes that SR has been updated to whatever it should contain
; and that _pc contains the start of the current instruction
;
; Will not set any values beyond the format/offset word
; Fellow will always generate this frame for bus/address errors
;========================================================================
	
%macro FRAMEA 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 26
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 0a000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrameA_
_cpuFrameA_:
		FRAMEA	ebx
		ret


;========================================================================
; Frame format $B
;
; 020:	Address or bus-error mid-instruction
; 030:	Same as for 020
;
; Fellow will not generate this frame for bus/address errors
;========================================================================
	
%macro FRAMEB 1	;  VECTOR_OFFSET
		mov	ecx, dword [da_regs + 60]	; Save Vector offset 
		sub	ecx, 84
		mov	dword [da_regs + 60], ecx
		mov	edx, %1
		or	edx, 0b000h
		writew
		mov	ecx, dword [da_regs + 60]	; Save PC
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		PTR_TO_PC	edx
		writel
		mov	ecx, dword [da_regs + 60]	; Save SR
		sub	ecx, 2
		mov	dword [da_regs + 60], ecx
		mov	edx, dword [sr]
		writew
%endmacro


		FALIGN32

global _cpuFrameB_
_cpuFrameB_:
		FRAMEB	ebx
		ret


;========================================================================
;
; Generate stack frame
;
; Input:	ebx - vector offset
;========================================================================


%macro GENERATE_STACK_FRAME 0
		and	ebx, 0fch
		call	dword [cpu_stack_frame_gen + ebx]
%endmacro


;=============================================================
; Move ISP (SSP) or MSP to A7, return it in ecx
;=============================================================

%macro ACTIVATE_SSP 0
		mov	ecx, dword [da_regs + 60]		; ecx - Current SP 
		test	byte [sr + 1], 20h		; Check supervisor 
		jnz	%%l1
		mov	dword [usp], ecx		; User mode, save USP 
		mov	ecx, dword [ssp]
		test	byte [sr + 1], 10h		; Get ISP or MSP 
		jz	%%l2
		mov	ecx, dword [msp]
%%l2:		mov	dword [da_regs + 60], ecx		; Set new A7 (SP) 
%%l1:		; Here, a7 is either ISP (SSP) or MSP
%endmacro
	

;=========================================================================
; Takes new SR in edx and maintains the integrity of the super/user state
;
; Hack:
; If trace mode is on, the return address on the stack is changed
; to bus_trace_mode
; This means that _cpuUpdateSR_ must be called with nothing else
; on the stack.
;=========================================================================

		FALIGN32

global _cpuUpdateSR_
_cpuUpdateSR_:	and	edx, 0ffffh
		mov	byte [upsrtraceset], 0
		test	byte [sr + 1], 0c0h
		jz	upsrnotraceset
		inc	byte [upsrtraceset]
upsrnotraceset:	mov	ecx, dword [da_regs + 60]	; Get current stack pointer
		test	byte [sr + 1], 20h	; Are we in super mode?
		jnz	upsup
		mov	dword [usp], ecx	; No, save stack pointer to usp
		jmp	upd2
upsup:		test	byte [sr + 1], 010h	; In super mode, save ISP/MSP
		jnz	upsup2
		mov	dword [ssp], ecx	; Master = 0
		jmp	upd2
upsup2:		mov	dword [msp], ecx	; Master = 1 
upd2:		test	dh, 20h			; Check new super/user mode
		jnz	upsup3
		mov	ecx, dword [usp]	; New mode is user, get usp
		jmp	upd3
upsup3:		mov	ecx, dword [ssp]	; New mode is super, get ssp
		test	dh, 10h			; or msp
		jz	upd3
		mov	ecx, dword [msp]
upd3:		mov	dword [da_regs + 60], ecx	; Set new stack pointer
		mov	dword [sr], edx	; Set new SR
upd4:		test	byte [sr + 1], 0c0h
		jz	updsrnotrace
		test	byte [upsrtraceset], 1
		jnz	updsrnotrace
		add	esp, 8
		push	dword _bus_trace_mode_
		sub	esp, 4
updsrnotrace:	jmp	_cpuRaiseInterrupt_	; Evaluate interrupts


;=====================================================
; No parameters, just checking for waiting interrupts
;
; First, check which irq that should be generated
; Then check if it already has been scheduled.
; If not, then schedule it
;=====================================================

	
		FALIGN32

global _cpuRaiseInterrupt_
_cpuRaiseInterrupt_:
		push	eax
		push	ebx
		push	ecx
		push	edx
	
		mov	edx, 0ffh			; IRQ level to raise 
		mov	ebx, dword [intena]
		mov	eax, dword [intreq]
		test	bh, 40h				; IRQ disabled?
		jz	rIRQ_schedule
		and	ebx, 3fffh
		and	eax, ebx			; eax - ena AND req
		mov	ebx, dword [sr]		; Get current IRQlevel
		and	ebx, 0700h
		shl	ebx, 6
		or	eax, ebx
		mov	dl, byte [irq_table + eax]
rIRQ_schedule:	test	dl, dl				; Irq waiting?
		jns	rIRQ_int

		; No irq up for schedule

		test	byte [interruptflag], 1
		jz	near rIRQ_exit
		mov	dword [interruptflag], 0
		mov	dword [irq_next], -1
		jmp	rIRQ_sched_exit
	
		; Irq waiting here, test if it has been scheduled already
		
rIRQ_int:	test	byte [interruptflag], 1	; Already scheduled? 
		jz	rIRQ_noprevious
		cmp	dl, byte [interruptlevel]	; Same irq? 
		jne	rIRQ_noprevious
		mov	eax, dword [curcycle]	; Just for safety, 
		cmp	eax, dword [irq_next]	; check that the time
		jb	near rIRQ_exit			; has not expired.
		add	eax, 1				; Set new time
		mov	dword [irq_next], eax
		jmp	rIRQ_sched_exit

		; Schedule new irq
		
rIRQ_noprevious:mov	dword [interruptlevel], edx	; Save runlevel 
		mov	ebx, dword [vbr]
		lea	ecx, [60h + ebx + edx*4]	; Save vector address
		fetchl
		mov	dword [interruptaddress], edx
		mov	eax, dword [curcycle]	; Set time 
		; Note: Lotus has code that expects a low delay in servicing requested irqs. This used to be 38
		add	eax, 10
		mov	dword [irq_next], eax
		mov	dword [interruptflag], 1

rIRQ_sched_exit:
		
		pushad
		call busScanEventsLevel5
		popad
		;SCAN_EVENTS_LVL5

rIRQ_exit:	pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		ret


;============================================================
; Transfers control to an interrupt routine
;
; 1. Put SSP/MSP into A7
; 2. Generate stack frame with normal format
; 3. T1 = T0 = 0, S = 1, set IRQ level (A7 already correct)
; 4. If 020++ and not 060: Get ISP, generate throwaway frame
;    with SR from step 3
; 5. If 020++ and not 060: Clear M bit (A7 already correct)
; 6. Set new PC and fill prefetch if enabled
; 7. Restart CPU if stopped
; 8. Reschedule events
;============================================================


		FALIGN32

global _cpuSetUpInterrupt_
_cpuSetUpInterrupt_:
		pushad
		ACTIVATE_SSP				; step 1
		mov	ebx, dword [interruptlevel]	; step 2
		mov	esi, dword [pc]
		GENERATE_STACK_FRAME
		and	byte [sr + 1], 38h		; step 3 
		mov	cl, byte [interruptlevel]
		or	cl, 20h
		or	byte [sr + 1], cl
		cmp	byte [cpu_type], 2	; step 4
		jb	near si_step6
		cmp	byte [cpu_type], 6
		jae	near si_step6
		mov	ecx, dword [da_regs + 60]
		test	byte [sr + 1], 010h
		jz	si_alreadyisp
		mov	dword dword [msp], ecx
		mov	ecx, dword [ssp]
		mov	dword [da_regs + 60], ecx
si_alreadyisp:	mov	ebx, dword [interruptlevel]
		FRAME1	ebx
		and	byte [sr + 1], 0efh		; step 5
si_step6:	mov	edx, dword [interruptaddress]; step 6
		PC_TO_PTR edx
		mov	dword [pc], edx
%ifdef PREFETCH
		call	prefetch_fillASM
%endif
		test	byte [cpu_stop], 1		; step 7
		jz	si_nostop
		mov	edx, dword [curcycle]
		add	edx, 4
		mov	dword [cpu_next], edx
		mov	dword [cpu_stop], 0
si_nostop:	mov	dword [interruptflag], 0
		mov	dword [irq_next], -1		; step 8
		
		pushad
		call busScanEventsLevel5
		popad
		;SCAN_EVENTS_LVL5

		popad
		ret


;===============================================
; Sets up an exception - ebx - offset of vector
;===============================================


		FALIGN32

global _cpuPrepareException_
_cpuPrepareException_:
		push	eax
		ACTIVATE_SSP
		GENERATE_STACK_FRAME
		mov	ecx, dword [vbr]
		add	ecx, ebx 
		fetchl
		PC_TO_PTR edx
		mov	dword [pc], edx		; Set PC
		mov	esi, edx

		or	byte [sr + 1], 20h	; Set supermodus
		and	byte [sr + 1], 03fh

		test	byte [cpu_stop], 1	; If CPU stopped, restart it
		jz	prpenostop
		mov	dword [cpu_stop], 0
		mov	eax, dword [curcycle]
		add	eax, 4
		mov	dword [cpu_next], eax

prpenostop:	pop	eax
		mov	esp, dword [exceptionstack]
%ifdef PREFETCH
		mov	edx, dword [pc]
		call	prefetch_fillASM
%endif
		jmp	dword [exceptionbackdooraddress]


;=============================================================================
; Adressmode routines to read byte, word, longword
; With PC increase
; Parameters:  In:  edx - pointer to register	esi - current PC position     
;	      Out:  edx - result		esi - current PC position
;=============================================================================

	
; Dx	    read
; --------------
; Ax	    read
; --------------


		FALIGN32

global _arb00_
global _arw00_
global _arl00_
global _arw01_
global _arl01_

_arb00_:
_arw00_:
_arl00_:
_arw01_:
_arl01_:	mov	edx, dword [edx]	       
		ret					
						

; (Ax)	    read
; --------------


		FALIGN32

global _arl02_
_arl02_:	mov	ecx, dword [edx]
		fetchlret


		FALIGN32

global _arw02_
_arw02_:	mov	ecx, dword [edx]
		fetchwret


		FALIGN32

global _arb02_
_arb02_:	mov	ecx, dword [edx]
		fetchbret


; (Ax)+	    read
; --------------


		FALIGN32

global _arl03_
_arl03_:	mov	ecx, dword [edx]
		add	dword [edx], 4
		fetchlret


		FALIGN32

global _arw03_
_arw03_:	mov	ecx, dword [edx]
		add	dword [edx], 2
		fetchwret


		FALIGN32

global _arb03_
_arb03_:	mov	ecx, dword [edx]
		inc	dword [edx]
		cmp	edx, dword da_regs + 60
		jne	arb03_l1
		inc	dword [edx]
arb03_l1:	fetchbret



; -(Ax)	    read
; --------------


		FALIGN32

global _arl04_
_arl04_:	sub	dword [edx], 4
		mov	ecx, dword [edx]
		fetchlret


		FALIGN32

global _arw04_
_arw04_:	sub	dword [edx], 2
		mov	ecx, dword [edx]
		fetchwret


		FALIGN32

global _arb04_
_arb04_:	dec	dword [edx]
		cmp	edx, dword da_regs + 60
		jne	arb04_l1
		dec	dword [edx]
arb04_l1:	mov	ecx, dword [edx]
		fetchbret



; disp(Ax)  read
; --------------


		FALIGN32

global _arl05_
_arl05_:	push	edx		  
		mov	ecx, esi		  
		fetchwopcode_inc_pos
		movsx	ecx, dx
		pop	edx			
		add	esi, 2			
		add	ecx, dword [edx]     
		fetchlret


		FALIGN32

global _arw05_
_arw05_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dx
		pop	edx
		add	esi, 2
		add	ecx, dword [edx]
		fetchwret


		FALIGN32

global _arb05_
_arb05_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dx
		pop	edx
		add	esi, 2
		add	ecx, dword [edx]
		fetchbret



; disp(Ax,Ri) read
; ----------------


		FALIGN32

global _arl06_
_arl06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]
		test	dh, 8
		jnz	arl06_l1
		movsx	ebx, bx
arl06_l1:	add	ecx, ebx
		pop	ebx
		fetchlret


		FALIGN32

global _arw06_
_arw06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	arw06_l1
		movsx	ebx, bx
arw06_l1:	add	ecx, ebx
		pop	ebx
		fetchwret


		FALIGN32

global _arb06_
_arb06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi			; Get word for adressmode
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl				; Get displacement
		add	ecx, dword [ebx]		; Add adress register
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	arb06_l1
		movsx	ebx, bx
arb06_l1:	add	ecx, ebx				; Add dataregister
		pop	ebx
		fetchb
		ret



; xxx.W	    read
; --------------


		FALIGN32

global _arl70_
_arl70_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		fetchlret


		FALIGN32

global _arw70_
_arw70_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		fetchwret


		FALIGN32

global _arb70_
_arb70_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		fetchbret



; xxx.L	    read
; --------------


		FALIGN32

global _arl71_
_arl71_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		fetchlret


		FALIGN32

global _arw71_
_arw71_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		fetchwret


		FALIGN32

global _arb71_
_arb71_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		fetchbret



; disp(pc)  read
; --------------


		FALIGN32

global _arl72_
_arl72_:	mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		add	esi, 2
		fetchlret


		FALIGN32

global _arw72_
_arw72_:	mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		add	esi, 2
		fetchwret


		FALIGN32

global _arb72_
_arb72_:	mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		add	esi, 2
		fetchbret



; disp(pc,Ri) read
; ----------------


		FALIGN32

global _arl73_
_arl73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dl
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	esi, 2
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	arl73_l1
		movsx	ebx, bx
arl73_l1:	add	ecx, ebx
		pop	ebx
		fetchlret


		FALIGN32

global _arw73_
_arw73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dl
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	esi, 2
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	arw73_l1
		movsx	ebx, bx
arw73_l1:	add	ecx, ebx
		pop	ebx
		fetchwret


		FALIGN32

global _arb73_
_arb73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ecx, dl
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	esi, 2
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	arb73_l1
		movsx	ebx, bx
arb73_l1:	add	ecx, ebx
		pop	ebx
		fetchb
		ret



; #xxxxxxxx read
; --------------


		FALIGN32

global _arl74_
_arl74_:	mov	ecx, esi
		add	esi, 4
		fetchlopcode_inc_pos
		ret

		FALIGN32

global _arw74_
global _arb74_
_arw74_:
_arb74_:	mov	ecx, esi
		add	esi, 2
		fetchwopcode_inc_pos
		ret


;=============================================================================
; Adressmoderoutines for byte,word,longword read
; without PC increase and  An inc/dec
; Constantes on start:	edx - reg.no*4	 edx - return  esi - PC
;=============================================================================


; (Ax)+	    read
; -------------


		FALIGN32

global _parl03_
_parl03_:	mov	ecx, dword [edx]
		fetchlret


		FALIGN32

global _parw03_
_parw03_:	mov	ecx, dword [edx]
		fetchwret


		FALIGN32

global _parb03_
_parb03_:	mov	ecx, dword [edx]
		fetchbret



; -(Ax)	    read
; -------------


		FALIGN32

global _parl04_
_parl04_:	mov	ecx, dword [edx]
		sub	ecx, 4
		fetchlret


		FALIGN32

global _parw04_
_parw04_:	mov	ecx, dword [edx]
		sub	ecx, 2
		fetchwret


		FALIGN32

global _parb04_
_parb04_:	mov	ecx, dword [edx]
		dec	ecx
		cmp	edx, dword da_regs + 60
		jne	parb04_l1
		dec	ecx
parb04_l1:	fetchbret



; disp(Ax)  read
; -------------


		FALIGN32

global _parl05_
_parl05_:	push	edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		pop	edx
		add	ecx, dword [edx]
		fetchlret


		FALIGN32

global	 _parw05_
_parw05_:	push	edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		pop	edx
		add	ecx, dword [edx]
		fetchwret


		FALIGN32

global _parb05_
_parb05_:	push	edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		pop	edx
		add	ecx, dword [edx]
		fetchbret



; disp(Ax,Ri) read
; ---------------


		FALIGN32

global _parl06_
_parl06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parl06_l1
		movsx	ebx, bx
parl06_l1:	add	ecx, ebx
		pop	ebx
		fetchlret


		FALIGN32

global _parw06_
_parw06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parw06_l1
		movsx	ebx, bx
parw06_l1:	add	ecx, ebx
		pop	ebx
		fetchwret


		FALIGN32

global _parb06_
_parb06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parb06_l1
		movsx	ebx, bx
parb06_l1:	add	ecx, ebx
		pop	ebx
		fetchbret



; xxx.W	    read
; -------------


		FALIGN32

global _parl70_
_parl70_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		fetchlret


		FALIGN32

global _parw70_
_parw70_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		fetchwret


		FALIGN32

global _parb70_
_parb70_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		fetchbret



; xxx.L	    read
; -------------


		FALIGN32

global _parl71_
_parl71_:	mov	ecx, esi
		fetchlopcode_keep_pos
		mov	ecx, edx
		fetchlret


		FALIGN32

global _parw71_
_parw71_:	mov	ecx, esi
		fetchlopcode_keep_pos
		mov	ecx, edx
		fetchwret


		FALIGN32

global _parb71_
_parb71_:	mov	ecx, esi
		fetchlopcode_keep_pos
		mov	ecx, edx
		fetchbret



; disp(pc)  read
; -------------


		FALIGN32

global _parl72_
_parl72_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		fetchlret


		FALIGN32

global _parw72_
_parw72_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		fetchwret


		FALIGN32

global _parb72_
_parb72_:	mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dx
		add	ecx, esi
		PTR_TO_PC ecx
		fetchbret



; disp(pc,Ri) read
; ---------------


		FALIGN32

global _parl73_
_parl73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parl73_l1
		movsx	ebx, bx
parl73_l1:	add	ecx, ebx
		pop	ebx
		fetchlret


		FALIGN32

global _parw73_
_parw73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parw73_l1
		movsx	ebx, bx
parw73_l1:	add	ecx, ebx
		pop	ebx
		fetchwret


		FALIGN32

global _parb73_
_parb73_:	push	ebx
		mov	ecx, esi
		fetchwopcode_keep_pos
		movsx	ecx, dl
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		add	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	parb73_l1
		movsx	ebx, bx
parb73_l1:	add	ecx, ebx
		pop	ebx
		fetchbret



; #xxxxxxxx read
; -------------


		FALIGN32

global _parl74_
_parl74_:	mov	ecx, esi
		fetchlopcode_keep_pos
		ret

		FALIGN32

global _parw74_
global _parb74_
_parw74_:
_parb74_:	mov	ecx, esi
		fetchwopcode_keep_pos
		ret


;=============================================================================
; Addressmode write
; with PC and -(aX)+ modify
; In: EBX - reg.nr EDX - data to write
;=============================================================================


; Dx	 write
; -------------


		FALIGN32

global _awb00_
_awb00_:	mov	byte [ebx], dl
		ret


		FALIGN32

global _aww00_
_aww00_:	mov	word [ebx], dx
		ret


		FALIGN32

global _awl00_
_awl00_:	mov	dword [ebx], edx
		ret


; Ax	  write
; -------------


		FALIGN32

global _aww01_
_aww01_:	movsx	edx, dx
		mov	dword [ebx], edx
		ret


		FALIGN32

global _awl01_
_awl01_:	mov	dword [ebx], edx
		ret


; (Ax)	  write
; -------------


		FALIGN32

global _awl02_
_awl02_:	mov	ecx, dword [ebx]
		writelret


		FALIGN32

global _aww02_
_aww02_:	mov	ecx, dword [ebx]
		writewret


		FALIGN32

global _awb02_
_awb02_:	mov	ecx, dword [ebx]
		writebret


; (Ax)+	    write
; ---------------

		FALIGN32

global _awl03_
_awl03_:	mov	ecx, dword [ebx]
		add	dword [ebx], 4
		writelret


		FALIGN32

global _aww03_
_aww03_:	mov	ecx, dword [ebx]
		add	dword [ebx], 2
		writewret


		FALIGN32

global _awb03_
_awb03_:	mov	ecx, dword [ebx]
		inc	dword [ebx]
		cmp	ebx, dword da_regs + 60
		jne	awb03_l1
		inc	dword [ebx]
awb03_l1:	writebret


; -(Ax)	    write
; ---------------


		FALIGN32

global _awl04_
_awl04_:	sub	dword [ebx], 4
		mov	ecx, dword [ebx]
		writelret


		FALIGN32

global _aww04_
_aww04_:	sub	dword [ebx], 2
		mov	ecx, dword [ebx]
		writewret


		FALIGN32

global _awb04_
_awb04_:	dec	dword [ebx]
		cmp	ebx, dword da_regs + 60
		jne	awb04_l1
		dec	dword [ebx]
awb04_l1:	mov	ecx, dword [ebx]
		writebret


; disp(Ax)  write
; -------------


		FALIGN32

global _awl05_
_awl05_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		add	ecx, dword [ebx]
		pop	edx
		writelret


		FALIGN32

global _aww05_
_aww05_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		add	ecx, dword [ebx]
		pop	edx
		writewret


		FALIGN32

global _awb05_
_awb05_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		add	ecx, dword [ebx]
		pop	edx
		writebret


; disp(Ax,Ri) write
; -----------------


		FALIGN32

global _awl06_
_awl06_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	awl06_l1
		movsx	ebx, bx
awl06_l1:	add	ecx, ebx
		pop	edx
		writelret


		FALIGN32

global _aww06_
_aww06_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	aww06_l1
		movsx	ebx, bx
aww06_l1:	add	ecx, ebx
		pop	edx
		writewret


		FALIGN32

global _awb06_
_awb06_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	awb06_l1
		movsx	ebx, bx
awb06_l1:	add	ecx, ebx
		pop	edx
		writebret


; xxx.W	    write
; ---------------


		FALIGN32

global _awl70_
_awl70_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		pop	edx
		writelret


		FALIGN32

global _aww70_
_aww70_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		pop	edx
		writewret


		FALIGN32

global _awb70_
_awb70_:	push	edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		pop	edx
		writebret


; xxx.L	    write
; ---------------


		FALIGN32

global _awl71_
_awl71_:	push	edx
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		pop	edx
		writelret


		FALIGN32

global _aww71_
_aww71_:	push	edx
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		pop	edx
		writewret


		FALIGN32

global _awb71_
_awb71_:	push	edx
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		pop	edx
		writebret


;=============================================================================
; Effective address evaluation
; Increases PC
; In:  edx - reg.nr*4+d	  esi - PC
; Out: edx - effective address
;=============================================================================


; (Ax)	   read
; -------------


		FALIGN32

global _eac02_
_eac02_:	mov	edx, dword [edx] 
		ret			    


; disp(Ax) read
; -------------


		FALIGN32

global _eac05_
_eac05_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	edx, dx
		add	edx, dword [ebx]
		pop	ebx
		ret


; disp(Ax,Ri) read
; ---------------


		FALIGN32

global _eac06_
_eac06_:	push	ebx
		mov	ebx, edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, dword [ebx]
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	eac06_l1
		movsx	ebx, bx
eac06_l1:	add	ecx, ebx
		mov	edx, ecx
		pop	ebx
		ret


; xxx.W	    read
; -------------


		FALIGN32

global _eac70_
_eac70_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	edx, dx
		ret


; xxx.L	    read
; -------------


		FALIGN32

global _eac71_
_eac71_:	mov	ecx, esi
		add	esi, 4
		fetchlopcode_inc_pos
		ret


; disp(pc)  read
; -------------


		FALIGN32

global _eac72_
_eac72_:	push	ebx
		mov	ebx, esi
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	edx, dx
		add	edx, ebx
		PTR_TO_PC edx
		pop	ebx
		ret


; disp(pc,Ri) read
; ---------------


		FALIGN32

global _eac73_
_eac73_:	push	ebx
		mov	ebx, esi
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dl
		add	ecx, ebx
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]	; Exploit position
		test	dh, 8				; Size
		jnz	eac73_l1
		movsx	ebx, bx
eac73_l1:	add	ecx, ebx
		mov	edx, ecx
		PTR_TO_PC edx
		pop	ebx
		ret


;==============================================================================
; Evaluation of branch conditions
; Out:	dl = 1 means branch / 0 not
;==============================================================================


; RA (TRUE)
; ---------


		FALIGN32

global _cc0_
_cc0_:		mov	edx, 1
		ret


; SR (TRUE) PC to stack
; ---------------------


		FALIGN32

global _cc1_
_cc1_:		mov	ebx, ecx			; Save ecx
		mov	ecx, dword [da_regs + 60]
		sub	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	edx, esi
		add	edx, dword [edi + 12]
		PTR_TO_PC edx
		writel					; Push PC to stack
		mov	ecx, ebx
		mov	edx, 1
		ret


; F (FALSE)
; ---------


		FALIGN32

global _cc1false_
_cc1false_:	xor	edx, edx
		ret


; HI (-C & -Z)
; ------------


		FALIGN32

global _cc2_
_cc2_:		xor	edx, edx
		test	byte [sr], 5
		jnz	cc2_l1
		inc	edx
cc2_l1:		ret


; LS (C OR Z)
; -----------


		FALIGN32

global _cc3_
_cc3_:		mov	edx, dword [sr]
		and	edx, 5
		ret


; CC (-C)
; -------


		FALIGN32

global _cc4_
_cc4_:		mov	edx, dword [sr]
		and	edx, 1
		xor	edx, 1
		ret


; CS (C)
; ------


		FALIGN32

global _cc5_
_cc5_:		mov	edx, dword [sr]
		and	edx, 1
		ret


; NE (-Z)
; -------


		FALIGN32

global _cc6_
_cc6_:		mov	edx, dword [sr]
		and	edx, 4
		xor	edx, 4
		ret


; EQ (Z)
; ------


		FALIGN32

global _cc7_
_cc7_:		mov	edx, dword [sr]
		and	edx, 4
		ret


; VC (-V)
; -------


		FALIGN32

global _cc8_
_cc8_:		mov	edx, dword [sr]
		and	edx, 2
		xor	edx, 2
		ret


; VS (V)
; ------


		FALIGN32

global _cc9_
_cc9_:		mov	edx, dword [sr]
		and	edx, 2
		ret


; PL (-N)
; -------


		FALIGN32

global _cca_
_cca_:		mov	edx, dword [sr]
		and	edx, 8
		xor	edx, 8
		ret


; MI (N)
; ------


		FALIGN32

global _ccb_
_ccb_:		mov	edx, dword [sr]
		and	edx, 8
		ret


; GE (N AND V OR -N AND -V)
; -------------------------


		FALIGN32

global _ccc_
_ccc_:		mov	edx, dword [sr]
		and	edx, 0fh
		movzx	edx, byte [ccctt + edx]
		ret


; LT (N AND -V OR -N AND V)
; -------------------------


		FALIGN32

global _ccd_
_ccd_:		mov	edx, dword [sr]
		and	edx, 0fh
		movzx	edx, byte [ccdtt + edx]
		ret


; GT (-Z & -N & V) OR (-Z & N & -V) 
; --------------------------------


		FALIGN32

global _cce_
_cce_:		mov	edx, dword [sr]
		and	edx, 0fh
		movzx	edx, byte [ccett + edx]
		ret

; LE ( Z OR (N & -V) OR (-N & V) )
; --------------------------------

		FALIGN32

global _ccf_
_ccf_:		mov	edx, dword [sr]
		and	edx, 0fh
		movzx	edx, byte [ccftt + edx]
		ret


; End of branch conditions
;==============================================================================

;==============================================================================
; Exceptions
;==============================================================================


		FALIGN32

global _cpuPrivilegieViolation_
_cpuPrivilegieViolation_:
		sub	esi, 2
		mov	ebx, 020h
		mov	eax, 38
		jmp	_cpuPrepareException_


;==============================================================================
; Illegal instruction
;==============================================================================

%ifdef UAE_FILESYS

CFUNCEXTERN call_calltrap

%macro CALL_CALLTRAP_CWRAP 0
		CCALLCONV_IN_EAX
		call	call_calltrap
		CCALLCONV_OUT_NONE	1
%endmacro


%endif

			FALIGN32

global _i00_
_i00_:		mov	esi, dword [pc]
		mov	ecx, esi
		PTR_TO_PC ecx
		mov	ebx, ecx
		fetchw
		mov	ecx, edx
		and	dh, 0f0h
		cmp	dh, 0f0h
		je	i00fline
		cmp	dh, 0a0h
		je	i00aline
		mov	ebx, 010h
		mov	eax, 38
		jmp	_cpuPrepareException_

i00fline:	mov	ebx, 02ch
		mov	eax, 38
		jmp	_cpuPrepareException_

i00aline:
		and	ebx, 0ff0000h
		cmp	ebx, 0f00000h
		je	i00fsysline
		mov	ebx, 028h
		mov	eax, 38
		jmp	_cpuPrepareException_

i00fsysline:	add	dword [pc], 2
		and	ecx, 0fffh
		mov	eax, ecx
%ifdef UAE_FILESYS
		CALL_CALLTRAP_CWRAP
%endif
		mov	eax, 512
		mov	esi, dword [pc]
		sub	dword [pc], 2
		ret
			

;==============================================================================
; Instruction 1	  ABCD.B Dy,Dx	  ABCD.B -(Ay),-(Ax)
; Tableusage  2 - rx*4(dest)  3 - ry*4
; Opcode*32 i edi
; FLAGS: XC
;==============================================================================

		FALIGN32

global _i01_1_
_i01_1_:	mov	ebx, dword [8 + edi]	; ebx - Source register
		mov	ecx, dword [4 + edi]	; ecx - Destination register
		mov	edx, dword [sr]		; edx - Flags
		movzx	eax, byte [ebx]		; eax - Get Source data
		movzx	ebx, byte [bcd2hex + eax]; ebx - Binary of source
		movzx	eax, byte [ecx]		; eax - Get Destination data
		movzx	eax, byte [bcd2hex + eax]; eax - Binary of source
		test	dl, 10h			; Add X flag to bl
		jz	i01_1a
		inc	ebx
i01_1a:		add	eax, ebx
		and	dl, 0eh
		cmp	al, 100			; Carry?
		jb	i01_1b
		or	edx, 11h		; Carry!
		sub	al, 100
i01_1b:		test	al, al			; Zero?
		jz	i01_1c
		and	dl, 01bh		; Non-Zero!
i01_1c:		mov	byte [sr], dl		; Flags end
		and	eax, 0ffh
		mov	al, byte [hex2bcd + eax]
		mov	byte [ecx], al
		mov	eax, 6
		ret					


		FALIGN32

global _i01_2_
_i01_2_:	mov	ebx, dword [8 + edi]		; Source
		mov	ecx, dword [ebx]
		dec	ecx
		cmp	ebx, dword da_regs + 60
		jne	i01_2i
		dec	ecx
i01_2i:		mov	dword [ebx], ecx
		xor	edx, edx
		fetchb
		xor	ebx, ebx
		mov	bl, byte [bcd2hex + edx]	; bl is now sourcedata
		mov	eax, dword [4 + edi]		; Dest
		mov	ecx, dword [eax]
		dec	ecx
		cmp	eax, dword da_regs + 60
		jne	i01_2j
		dec	ecx
i01_2j:		mov	dword [eax], ecx
		xor	edx, edx
		mov	ebp, ecx			; Save ecx
		fetchb
		xor	eax, eax
		mov	al, byte [bcd2hex + edx]
		mov	dl, byte [sr]			; Flags
		test	dl, 10h
		jz	i01_2a
		inc	eax
i01_2a:		add	eax, ebx
		and	dl, 0eh
		cmp	al, 100				; Carry?
		jb	i01_2b
		or	dl, 11h				; Carry!
		sub	al, 100
i01_2b:		test	al, al				; Zero?
		jz	i01_2c
		and	dl, 01bh			; Non-Zero!
i01_2c:		mov	byte [sr], dl			; Flags end
		and	eax, 0ffh
		mov	dl, byte [hex2bcd + eax]
		mov	ecx, ebp			; restore saved <ea>
		writeb
		mov	eax, 18				; return cyclecount
		ret


%define IMSK_XNZVC 0x8c1
%define MCLR_XNZVC 0
%define MSET_XNZVC 0x1f

%macro SETFLAGS 3  ;  INTELMASK, M68CLEAR, M68SET
		pushfd
		pop	eax
		and	eax, %1 
		mov	eax, dword [statustab + 4*eax]
		%if	%3 = 0x1f
			mov	byte [sr], al
		%else
			mov	ah, byte [sr]
			and	ah, %2
			and	al, %3
			or	ah, al
			mov	byte [sr], ah
		%endif
%endmacro
		
%macro GETARG 3 ;  ROUTINE,REGISTER,DESTREG
		mov	edx, dword [%2]
		call	dword [%1]
%endmacro

%macro SETARG 3 ;  ROUTINE,REGISTER,SOURREG 
		mov	ebx, dword [%2]
		call	dword [%1]
%endmacro	

%macro GETREGVAL 3 ;  DESTREG , REGISTER , REGREG
		mov	%3, dword [%2]
		mov	%1, dword [%3]
%endmacro

%macro SETREGVAL 2 ;  SOURREG , REGISTER
		mov	eax, dword [%2]
		mov	dword [eax], %1
%endmacro

%macro SETREGVALHREG 3 ;   SOURREG , REGISTER , REGREG
		mov	dword [%3], %1
%endmacro
	
			
;================================================
; ADD.X <ea>,Dn (case 1) / ADD.X Dn,<ea> (case 2)
; Flags: All
;================================================

		FALIGN32

global _i02_B_1_
_i02_B_1_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, ebx
		add		cl, dl
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		mov		byte [ebx], cl
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i02_W_1_
_i02_W_1_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, ebx
		add		cx, dx
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		mov		word [ebx], cx
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i02_L_1_
_i02_L_1_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, ebx
		add		ecx, edx
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		SETREGVALHREG	ecx, edi + 16, ebx
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i02_B_2_
_i02_B_2_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, eax
		add		dl, cl
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		SETARG		edi + 24, edi + 8, edx
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i02_W_2_
_i02_W_2_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, eax
		add		dx, cx
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		SETARG		edi + 24, edi + 8, edx
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i02_L_2_
_i02_L_2_:
		GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, eax
		add		edx, ecx
		SETFLAGS	IMSK_XNZVC, MCLR_XNZVC, MSET_XNZVC
		SETARG		edi + 24, edi + 8, edx
		mov		eax, dword [edi + 20]
		ret


;==============================================================================
; ADDA.W/L <ea>,An
; Flags:  NONE
;==============================================================================

		FALIGN32

global _i03_W_
_i03_W_:	GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, ebx
		movsx		edx, dx
		add		ecx, edx
		SETREGVALHREG	ecx, edi + 16, ebx
		mov		eax, dword [edi + 20]
		ret

		FALIGN32

global _i03_L_
_i03_L_:	GETARG		edi + 12, edi + 8, edx
		GETREGVAL	ecx, edi + 16, ebx
		add		ecx, edx
		SETREGVALHREG	ecx, edi + 16, ebx
		mov		eax, dword [edi + 20]
		ret

;==============================================================================
; ADDI.X #,<ea>
; Flags: ALL,verified OK
;==============================================================================

		FALIGN32

global _i04_B_
_i04_B_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		add	dl, bl
		pushfd
		pop	eax
		and	eax, 0fffh
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i04_W_
_i04_W_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		add	dx, bx
		pushfd
		pop	eax
		and	eax, 0fffh
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret

		FALIGN32

global _i04_L_
_i04_L_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		add	edx, ebx
		pushfd
		pop	eax
		and	eax, 0fffh
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


;==============================================================================
; ADDQ.X #,<ea>
;  0-rutinepeker 1-disasm rutine  2-eareg 3-ea read rutine
;		 4-cycle count	  5-ea write rutine 6-add immediate
; Flags: ALL
;==============================================================================

		FALIGN32

global _i05_B_
_i05_B_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 24]
		add	dl, bl
		pushfd
		pop	eax
		and	eax, 0fffh
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al	   
		mov	ebx, dword [edi + 8]
		call	ecx		       
		mov	eax, dword [edi + 16] 
		ret			       


		FALIGN32

global _i05_W_
_i05_W_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8] 
		call	eax		      
		mov	ebx, dword [edi + 24]
		add	dx, bx			
		pushfd				
		pop	eax			
		and	eax, 0fffh		
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al	    
		mov	ebx, dword [edi + 8]
		call	ecx		      
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i05_L_
_i05_L_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8] 
		call	eax
		mov	ebx, dword [edi + 24]
		add	edx, ebx
		pushfd
		pop	eax
		and	eax, 0fffh
		mov	eax, dword [statustab + 4*eax]
		mov	ecx, dword [edi + 20]
		mov	byte [sr], al
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i05_ADR_
_i05_ADR_:	mov	eax, dword [edi + 8] 
		mov	edx, dword [eax]
		mov	ebx, dword [edi + 24]
		add	edx, ebx		      
		mov	dword [eax], edx
		mov	eax, dword [edi + 16]
		ret


;==============================================================================
; Instruction 6	  ADDX.X Dy,Dx	  ADDX.X -(Ay),-(Ax)
; Tableusage  2 - rx*4(dest)  3 - ry*4
; Opcode*32 i edi
; Flags: ALL, Z unchanged if result is non-zero
;==============================================================================


		FALIGN32

global _i06_B_1_
_i06_B_1_:	mov	ebx, dword [12 + edi]
		mov	ecx, dword [8 + edi]
		mov	eax, dword [ebx]
		mov	ebx, dword [ecx]	       
		mov	edx, dword [sr]
		test	dl, 10h
		jz	i06b1nc
		stc	
i06b1nc:	adc	bl, al
		pushfd
		pop	eax				
		and	eax, 0fbfh
		mov	byte [ecx], bl
		mov	eax, dword [statustab + 4*eax]
		mov	ah, byte [sr]
		and	ah, 4
		test	bl, bl
		jz	i06b1nz
		xor	ah, ah
i06b1nz:	or	al, ah
		mov	byte [sr], al
		mov	eax, 4
		ret


		FALIGN32

global _i06_W_1_
_i06_W_1_:	mov	ebx, dword [12 + edi]
		mov	ecx, dword [8 + edi]
		mov	eax, dword [ebx]
		mov	ebx, dword [ecx]
		mov	edx, dword [sr]
		test	dl,10h
		jz	i06w1nc
		stc	
i06w1nc:	adc	bx, ax
		pushfd				      
		pop	eax			      
		and	eax, 0fbfh		      
		mov	word [ecx], bx	     
		mov	eax, dword [statustab + 4*eax]
		mov	ah, byte [sr]
		and	ah, 4
		test	bx, bx
		jz	i06w1nz
		xor	ah, ah
i06w1nz:	or	al, ah
		mov	byte [sr], al	       
		mov	eax, 4
		ret


		FALIGN32

global _i06_L_1_
_i06_L_1_:	mov	ebx, dword [12 + edi]
		mov	ecx, dword [8 + edi]
		mov	eax, dword [ebx]
		mov	ebx, dword [ecx]
		mov	edx, dword [sr]
		test	dl, 10h
		jz	i06l1nc
		stc
i06l1nc:	adc	ebx, eax
		pushfd
		pop	eax
		and	eax, 0fbfh
		mov	dword [ecx], ebx
		mov	eax, dword [statustab + 4*eax]
		mov	ah, byte [sr]
		and	ah, 4
		test	ebx, ebx
		jz	i06l1nz
		xor	ah, ah
i06l1nz:	or	al, ah
		mov	byte [sr], al
		mov	eax, 8
		ret


		FALIGN32

global _i06_B_2_
_i06_B_2_:	mov	ebx, dword [12 + edi]	; Source 
		mov	ecx, dword [ebx]
		dec	ecx
		cmp	ebx, dword da_regs + 60
		jne	i06_B_2j
		dec	ecx
i06_B_2j:	mov	dword [ebx], ecx
		fetchb
		mov	ebx, edx
		mov	eax, dword [8 + edi]	; Dest
		mov	ecx, dword [eax]
		dec	ecx
		cmp	eax, dword da_regs + 60
		jne	i06_B_2i
		dec	ecx
i06_B_2i:	mov	dword [eax], ecx
		mov	ebp, ecx		; Save <ea> for later use
		fetchb
		mov	eax, dword [sr]
		test	al, 10h
		jz	i06b2nc
		stc	
i06b2nc:	adc	dl, bl
		pushfd
		pop	eax
		and	eax, 0fbfh
		mov	eax, dword [statustab+4*eax]
		mov	ah, byte [sr]
		and	ah, 4				; Preserve Z
		test	dl, dl				; Clear if nonzero
		jz	i06b2nz
		xor	ah, ah
i06b2nz:	or	al, ah
		mov	byte [sr], al		 
		mov	ecx, ebp			; restore saved <ea>
		writeb
		mov	eax, 18
		ret


		FALIGN32

global _i06_W_2_
_i06_W_2_:	mov	ebx, dword [12 + edi]	; Source 
		mov	ecx, dword [ebx]
		sub	ecx, 2
		mov	dword [ebx],ecx
		fetchw
		mov	ebx, edx
		mov	eax, dword [8 + edi]		; Dest
		mov	ecx, dword [eax]
		sub	ecx, 2
		mov	dword [eax], ecx
		mov	ebp, ecx			; Save <ea> for later use
		fetchw
		mov	eax, dword [sr]
		test	al, 10h
		jz	i06w2nc
		stc	
i06w2nc:	adc	dx, bx
		pushfd					
		pop	eax				
		and	eax, 0fbfh			
		mov	eax, dword [statustab + 4*eax]
		mov	ah, byte [sr]
		and	ah, 4				; Preserve Z
		test	dx, dx				; Clear if nonzero
		jz	i06w2nz
		xor	ah, ah
i06w2nz:	or	al, ah
		mov	byte [sr], al	     
		mov	ecx, ebp			; restore saved <ea>
		writew
		mov	eax, 18
		ret


		FALIGN32

global _i06_L_2_
_i06_L_2_:	mov	ebx, dword [12 + edi]	; Source 
		mov	ecx, dword [ebx]
		sub	ecx, 4
		mov	dword [ebx], ecx
		fetchl
		mov	ebx, edx
		mov	eax, dword [8 + edi]		; Dest
		mov	ecx, dword [eax]
		sub	ecx, 4
		mov	dword [eax], ecx
		mov	ebp, ecx			; Save <ea> for later use
		fetchl
		mov	al, byte [sr]
		test	al, 10h
		jz	i06l2nc
		stc	
i06l2nc:	adc	edx, ebx
		pushfd
		pop	eax
		and	eax, 0fbfh 
		mov	eax, dword [statustab + 4*eax]
		mov	ah, byte [sr]
		and	ah, 4				; Preserve Z
		test	edx, edx			; Clear if nonzero
		jz	i06l2nz
		xor	ah, ah
i06l2nz:	or	al, ah
		mov	byte [sr], al		 
		mov	ecx, ebp			; restore saved <ea>
		writel
		mov	eax, 30
		ret


;==============================================================================
; AND.X <ea>,Dn / AND.X Dn,<ea>
; Flags: X unaffected, N Z, V = C = 0, verfied OK
;==============================================================================


		FALIGN32

global _i07_B_1_
_i07_B_1_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	ecx, dword [ebx]
		and	cl, dl
		pushfd
		pop	eax
		and	eax, 07feh
		mov	byte [ebx], cl
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	dword [sr], eax
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i07_W_1_
_i07_W_1_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	ecx, dword [ebx]
		and	cx, dx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	word [ebx], cx
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	dword [sr], eax
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i07_L_1_
_i07_L_1_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	ecx, dword [ebx]
		and	ecx, edx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	dword [ebx], ecx
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	dword [sr], eax
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i07_B_2_
_i07_B_2_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	ecx, dword [ebx]
		and	dl, cl
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 24]
		mov	dword [sr], eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i07_W_2_
_i07_W_2_:	mov	eax,dword [edi + 12]
		mov	edx,dword [edi + 8]
		call	eax
		mov	ebx,dword [edi + 16]
		mov	ecx,dword [ebx]
		and	dx,cx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 24]
		mov	dword [sr],eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i07_L_2_
_i07_L_2_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	ecx, dword [ebx]
		and	edx, ecx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 24]
		mov	dword [sr], eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 20]
		ret

;==============================================================================
; ANDI.X #,<ea>
; Flagg: X unaffected, N Z, V = C = 0, verfied OK
;==============================================================================


		FALIGN32

global _i08_B_
_i08_B_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		and	dl, bl
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 20]
		mov	dword [sr], eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i08_W_
_i08_W_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		and	dx, bx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 20]
		mov	dword [sr], eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i08_L_
_i08_L_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ebx, edx
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		and	edx, ebx
		pushfd
		pop	eax
		and	eax, 07feh
		mov	ebx, dword [statustab + 4*eax]
		mov	eax, dword [sr]
		and	eax, 0fffffff0h
		or	eax, ebx
		mov	ecx, dword [edi + 20]
		mov	dword [sr], eax
		mov	ebx, dword [edi + 8]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i08_B_CCR_
_i08_B_CCR_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	eax, dword [sr]
		and	al, dl
		mov	byte [sr], al
		mov	eax, 20
		ret


		FALIGN32

global _i08_W_SR_
_i08_W_SR_:	test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	eax, dword [sr]
		and	eax, edx
		mov	edx, eax
		call	_cpuUpdateSR_
		mov	eax, 20
		ret

;==============================================================================
; ASX.X
; Tableusage:(imm) 0-rut 1-disrut 2-shcount 3-dreg 4-cyclecount
; Tableusage:(reg) 0-rut 1-disrut 2-shcountreg 3-dreg 4-cyclecount
; Tableusage:(mem) 0-rut 1-disrut 2-eareg 3-earead 4-eawrite 5-cyclecount
; Flags: ALL, XC unaffected if shift count is zero, only happens with shiftcount
;	  in Dx, V set if sign bit changes ANYTIME during shift, only happes during ASL 
;==============================================================================


		FALIGN32

global _i09_RI_I_B_
_i09_RI_I_B_:	mov	ecx, dword [edi + 8]		; get shcount
		mov	ebx, dword [edi + 12]	; get dreg ptr
		mov	eax, dword [ebx]		; get dreg content
		sar	al, cl
		pushfd
		pop	ecx
		and	ecx, 07ffh					; Intel, OF undefined or 0
		mov	byte [ebx], al
		mov	ecx, dword [statustab + 4*ecx]
		mov	byte [sr], cl
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_RI_I_W_
_i09_RI_I_W_:	mov	ecx, dword [edi + 8]		; get shcount
		mov	ebx, dword [edi + 12]	; get dreg ptr
		mov	eax, dword [ebx]		; get dreg content
		sar	ax, cl
		pushfd
		pop	ecx
		and	ecx, 07ffh
		mov	word [ebx], ax
		mov	ecx, dword [statustab + 4*ecx]
		mov	byte [sr], cl
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_RI_I_L_
_i09_RI_I_L_:	mov	ecx, dword [edi + 8]		; get shcount
		mov	ebx, dword [edi + 12]	; get dreg ptr
		mov	eax, dword [ebx]		; get dreg content
		sar	eax, cl
		pushfd
		pop	ecx
		and	ecx, 07ffh
		mov	dword [ebx], eax
		mov	ecx, dword [statustab + 4*ecx]
		mov	byte [sr], cl
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_RI_R_B_
_i09_RI_R_B_:	mov	ecx, dword [edi + 8]		; get shcount reg ptr
		mov	ebx, dword [edi + 12]	; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	i09rirbz
		sar	al, cl
		pushfd
		mov	byte [ebx], al
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	eax, dword [statustab + 4*ecx]
		mov	byte [sr], al
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret
i09rirbz:	test	al, al
		pushfd
		pop	ebx
		and	ebx, 0c0h			; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h			; preserve X
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_RI_R_W_
_i09_RI_R_W_:	mov	ecx, dword [edi+8]		; get shcount reg ptr
		mov	ebx, dword [edi+12]		; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	i09rirwz
		sar	ax, cl
		pushfd
		mov	word [ebx], ax
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	eax, dword [statustab + 4*ecx]
		mov	byte [sr], al
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret
i09rirwz:	test	ax, ax
		pushfd
		pop	ebx
		and	ebx, 0c0h			; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h			; preserve XC
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_RI_R_L_
_i09_RI_R_L_:	mov	ecx, dword [edi + 8]		; get shcount reg ptr
		mov	ebx, dword [edi + 12]	; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	i09rirlz
		cmp	ecx, 32
		jb	i09rirlok
		je	i09rirlc32
		sar	eax, 31
		sar	eax, 2
		jmp	i09rirlnok
i09rirlc32:	sar	eax, 31
		sar	eax, 1
		jmp	i09rirlnok
i09rirlok:	sar	eax, cl
i09rirlnok:	pushfd
		mov	dword [ebx], eax
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	eax, dword [statustab + 4*ecx]
		mov	byte [sr], al
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret
i09rirlz:	test	eax, eax
		pushfd
		pop	ebx
		and	ebx, 0c0h			; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h			; preserve XC
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_LE_I_B_
_i09_LE_I_B_:	mov	ebx, dword [edi + 12]	       ; get dreg ptr
		mov	ecx, dword [edi + 8]	       ; get shcount
		mov	eax, dword [ebx]	       ; get dreg content
		movsx	eax, al
		mov	edx, eax
		sal	dl, cl
		pushfd
		sal	eax, cl
		pop	ecx
		and	ecx, 07ffh
		mov	byte [ebx], al
		mov	bl, byte [statustab + 4*ecx]
		; eax has original sign since we can only shift 31 positions
		; so if sign was not changed, eax>>7 is now either 0 or -1
		; O is already cleared, so we only need to set it
		sar	eax, 7
		cmp	eax, -1
		je	.no_o
		test	eax, eax
		jz	.no_o
		or	bl, 2
.no_o:		mov	byte [sr], bl
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i09_LE_I_W_
_i09_LE_I_W_:	mov	ebx, dword [edi + 12]	       ; get dreg ptr
		mov	ecx, dword [edi + 8]	       ; get shcount
		mov	ax, word [ebx]		       ; get dreg content
		movsx	eax, ax
		mov	edx, eax
		sal	dx, cl
		pushfd
		sal	eax, cl
		pop	ecx
		and	ecx, 07ffh
		mov	word [ebx], ax
		mov	ebx, dword [statustab + 4*ecx]
		; eax has original sign since we can only shift 31 positions
		; so if sign was not changed, eax>>15 is now either 0 or -1
		; O is already cleared, so we only need to set it
		sar	eax, 15
		cmp	eax, -1
		je	.no_o
		test	eax, eax
		jz	.no_o
		or	bl, 2
.no_o:		mov	eax,dword [edi + 16]
		mov	byte [sr], bl
		ret

; Overflow detection here is as follows
; Mask out the bits that gets shifted out + 1 (31 ... (31 - shiftcount)), we can only shift 8 bits
; Check for all zeros (and mask), or all ones (xor mask) for no overflow

		FALIGN32

global _i09_LE_I_L_
_i09_LE_I_L_:	mov	ebx, dword [edi + 12]		; get dreg ptr
		mov	eax, dword [ebx]		; get dreg content

		; Overflow detection
		mov	edx, -1				; Create mask
		mov	ecx, 31				; edx is zero if sign change 
		sub	ecx, dword [edi + 8]
		shl	edx, cl
		mov	ebp, eax
		and	ebp, edx
		jz	.no_o
		xor	ebp, edx
		jz	.no_o
		mov	edx, 0
.no_o:		mov	ecx, dword [edi+8]		; get shcount
		sal	eax, cl
		pushfd
		pop	ecx
		and	ecx, 07ffh
		mov	dword [ebx], eax
		mov	ebx, dword [statustab + 4*ecx]
		test	edx, edx
		jnz	.no_o2
		or	bl,2h
.no_o2:		mov	eax, dword [edi + 16]
		mov	byte [sr], bl
		ret


		FALIGN32

global _i09_LE_R_B_
_i09_LE_R_B_:	mov	ecx, dword [edi + 8]		; get shcount reg ptr
		mov	ebx, dword [edi + 12]		; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	.zero_shift
		cmp	ecx, 8
		jae	.too_big_shift
		movsx	eax, al
		mov	edx, eax
		sal	dl, cl
		pushfd
		sal	eax, cl
		mov	byte [ebx], al
		mov	ebp, ecx
		add	ebp, ebp			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	ebx, dword [statustab + 4*ecx]
		; eax has original sign since we can only shift 31 positions
		; so if sign was not changed, eax>>7 is now either 0 or -1
		; O is already cleared, so we only need to set it
		sar	eax, 7
		cmp	eax, -1
		je	.no_o
		test	eax, eax
		jz	.no_o
		or	ebx, 2
.no_o:		mov	byte [sr], bl
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret

; Need to clear CV and set NZ
.zero_shift:	test	al, al
		pushfd
		pop	ebx
		and	ebx, 0c0h		; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h		; preserve X
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret

; Set zero result, set Z flag, if count == size then
; set xc to sign of original and set V flag if original was not zero
.too_big_shift:
		mov	byte [ebx], 0
		mov	bl, 4h
		test	al, al
		jz	.no_o2
		or	bl, 2
.no_o2:		cmp	cl, 8
		jne	.no_xc
		test	al, 80h
		jz	.no_xc
		or	bl, 11h
.no_xc:		mov	byte [sr], bl
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret


		FALIGN32

global _i09_LE_R_W_
_i09_LE_R_W_:	mov	ecx, dword [edi + 8]		; get shcount reg ptr
		mov	ebx, dword [edi + 12]		; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	.zero_shift
		cmp	ecx, 16
		jae	.too_big_shift
		movsx	eax, ax
		mov	edx, eax
		sal	dx, cl
		pushfd
		sal	eax, cl
		mov	word [ebx], ax
		mov	ebp, ecx
		add	ebp, ebp			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	ebx, dword [statustab + 4*ecx]
		; eax has original sign since we can only shift 31 positions
		; so if sign was not changed, eax>>15 is now either 0 or -1
		; O is already cleared, so we only need to set it
		sar	eax, 7
		cmp	eax, -1
		je	.no_o
		test	eax, eax
		jz	.no_o
		or	ebx, 2
.no_o:		mov	byte [sr], bl
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret
.zero_shift:
		test	ax, ax
		pushfd
		pop	ebx
		and	ebx, 0c0h		; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h		; preserve X
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret

; Set zero as result, set Z flag, if count == size then
; set xc to sign bit and set V flag if original isnt zero 
.too_big_shift:
		mov	word [ebx], 0
		mov	bl, 4h
		test	ax, ax
		jz	.no_o2
		or	bl, 2
.no_o2:		cmp	cl, 16
		jne	.no_xc
		test	eax, 00008000h
		jz	.no_xc
		or	bl, 11h
.no_xc:		mov	byte [sr], bl
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret


		FALIGN32

global _i09_LE_R_L_
_i09_LE_R_L_:	mov	ecx, dword [edi + 8]		; get shcount reg ptr
		mov	ebx, dword [edi + 12]		; get dreg ptr
		mov	ecx, dword [ecx]		; get shcount reg content
		mov	eax, dword [ebx]		; get dreg content
		and	ecx, 3fh			; modulo 64
		jz	.zero_shift
		cmp	ecx, 32
		jae	.too_big_shift
		push	ecx
		mov	ebp, ecx
		mov	edx, -1				; Create mask
		mov	ecx, 31				; edx is zero if sign change 
		sub	ecx, ebp
		shl	edx, cl
		mov	ebp, eax
		and	ebp, edx
		jz	.no_o
		xor	ebp, edx
		jz	.no_o
		mov	edx, 0
.no_o:
		pop	ecx
		sal	eax, cl
		pushfd
		mov	dword [ebx], eax
		mov	ebp, ecx
		add	ebp, ebp			; Calculate cyclecount
		pop	ecx
		and	ecx, 07ffh
		mov	ebx, dword [statustab + 4*ecx]
		test	edx, edx
		jnz	.no_o2
		or	bl, 2h
.no_o2:		mov	byte [sr], bl
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret
.zero_shift:
		test	eax, eax
		pushfd
		pop	ebx
		and	ebx, 0c0h			  ; We want sign and zero
		shr	ebx, 4
		mov	bh, byte [sr]
		and	bh, 010h			  ; preserve X
		or	bh, bl
		mov	byte [sr], bh
		mov	eax, dword [edi + 16]
		ret

; here, set zero as result, Set Z flag, if count == size then
; set xc to sign bit and set V flag if original isnt zero 
.too_big_shift:	mov	dword [ebx], 0
		mov	bl, 4h
		test	eax, eax
		jz	.no_o3
		or	bl, 2
.no_o3:		cmp	cl, 32
		jne	.no_xc
		test	eax, eax
		jns	.no_xc
		or	bl, 11h
.no_xc:		mov	byte [sr], bl
		mov	edx, ecx
		add	edx, edx			; Calculate cyclecount
		mov	eax, dword [edi + 16]
		add	eax, edx
		ret

; Here, shift count always 1.  Things to care about:
; 1. Sign change during shift, done in hardware when count is 1


		FALIGN32

global _i09_RI_M_W_
_i09_RI_M_W_:	mov	ebx, dword [edi + 8]		; get ea reg ptr
		mov	edx, ebx
		mov	eax, dword [edi + 12]		; get ea read rut
		call	eax
		sar	dx, 1
		pushfd
		pop	ecx
		and	ecx, 0fffh			; Overflow defined here!
		mov	eax, dword [edi + 16]		; get dest rut ptr.
		mov	ecx, dword [statustab + 4*ecx]
		mov	byte [sr], cl
		call	eax	
		mov	eax, dword [edi + 20]
		ret


		FALIGN32

global _i09_LE_M_W_
_i09_LE_M_W_:	mov	ebx, dword [edi + 8]		; get ea reg ptr
		mov	edx, ebx
		mov	eax, dword [edi + 12]		; get ea read rut
		call	eax
		sal	dx, 1
		pushfd
		pop	ecx
		and	ecx, 0fffh			; OF defined here
		mov	eax, dword [edi + 16]		; get dest rut ptr.
		mov	ecx, dword [statustab + 4*ecx]
		mov	byte [sr], cl
		call	eax	
		mov	eax, dword [edi + 20]
		ret


;==============================================================================
; Bcc
; Tableusage 0- routine 1 - displacement(32bit)	 2-condition evaluate routine
;==============================================================================


		FALIGN32

global	 _i11_B_
_i11_B_:	call	dword [edi + 8] 				; edx = 1 take branch
		mov	ebx, dword [edi + 4]				; get disp
		test	dl, dl
		jz	i11bnob
		test	ebx, 1
		jnz	i11ex
		add	esi, ebx			; New PC
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		mov	eax, 8
		ret
i11bnob:	mov	eax, 10
		ret
i11ex:		mov	ebx, 0ch			; Jump to odd address
		mov	eax, 44				; Raise exception
		jmp	_cpuPrepareException_


		FALIGN32

global _i11_W_
_i11_W_:	call	dword [edi + 8]			; edx != 0 take branch
		test	dl, dl
		jz	i11wnob
		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	ebx, dx
		test	dl, 1
		jnz	near i11ex
		mov	eax, 12
		add	esi, ebx
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		ret
i11wnob:
%ifdef PREFETCH
		mov	ecx,esi
		fetchwopcode_inc_pos
%endif
		add	esi, 2
		mov	eax, 10
		ret
	

		FALIGN32

global _i11_L_
_i11_L_:	call	dword [edi + 8]			; edx != 0 take branch
		test	dl, dl
		jz	i11lnob
		mov	ecx, esi
		fetchlopcode_inc_pos
		test	dl, 1
		jnz	near i11ex
		mov	eax, 12
		add	esi, edx
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		ret
i11lnob:
%ifdef PREFETCH
		mov	ecx, esi
		fetchlopcode_inc_pos
%endif
		add	esi, 4
		mov	eax, 10
		ret
	

;==============================================================================
; BCHG.B Dn,<ea>
; Tableusage dynamic:
; 2 - eareg 3-earead 4-reg 5-cycle count 6-eawrite
; Tableusage static:
; 2 - eareg 3-earead 4-cycle count 5-eawrite
; Flag: Z - state of the bit before the operation
;==============================================================================


		FALIGN32

global _i12_D_L_
_i12_D_L_:	mov	eax,dword [edi + 8]
		mov	ecx,dword [edi + 16]
		mov	ebx,dword [eax]
		mov	edx,dword [ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,1fh
		btc	ebx,edx
		jc	i12dl1
		or	ecx,4
i12dl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx
		mov	eax,dword [edi + 20]
		ret


		FALIGN32

global _i12_D_B_
_i12_D_B_:	mov	eax,dword [edi + 12]
		mov	edx,dword [edi + 8]
		call	eax
		mov	eax,dword [edi + 16]
		mov	eax,dword [eax]
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	eax,7
		btc	edx,eax
		jc	i12db1
		or	ecx,4
i12db1:		mov	dword [sr],ecx
		mov	ebx,dword [edi + 8]
		mov	eax,dword [edi + 24]
		call	eax
		mov	eax,dword [edi + 20]
		ret


		FALIGN32

global _i12_S_L_
_i12_S_L_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [edi + 8]
		mov	ebx,dword [eax]
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,1fh
		btc	ebx,edx
		jc	i12sl1
		or	ecx,4
i12sl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

global _i12_S_B_
_i12_S_B_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi + 12]
		mov	edx,dword [edi + 8]
		call	eax
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	ebx,7
		btc	edx,ebx
		jc	i12sb1
		or	ecx,4
i12sb1:		mov	dword [sr],ecx
		mov	ebx,dword [edi + 8]
		mov	eax,dword [edi + 20]
		call	eax
		mov	eax,dword [edi + 16]
		ret


;==============================================================================
; BCLR.B Dn,<ea>
; Tableusage dynamic:
; 2 - eareg 3-earead 4-reg 5-cycle count 6-eawrite
; Tableusage static:
; 2 - eareg 3-earead 4-cycle count 5-eawrite
; Flag:	Z - state of the bit before the operation
;==============================================================================


		FALIGN32

global _i13_D_L_
_i13_D_L_:	mov	eax,dword [edi + 8]
		mov	ecx,dword [edi + 16]
		mov	ebx,dword [eax]
		mov	edx,dword [ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,1fh
		btr	ebx,edx
		jc	i13dl1
		or	ecx,4
i13dl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx
		mov	eax,dword [edi + 20]
		ret


		FALIGN32

global _i13_D_B_
_i13_D_B_:	mov	eax,dword [edi + 12]
		mov	edx,dword [edi + 8]
		call	eax
		mov	eax,dword [edi + 16]
		mov	eax,dword [eax]
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	eax,7
		btr	edx,eax
		jc	i13db1
		or	ecx,4
i13db1:		mov	dword [sr],ecx
		mov	ebx,dword [edi + 8]
		mov	eax,dword [edi + 24]
		call	eax
		mov	eax,dword [edi + 20]
		ret


		FALIGN32

global _i13_S_L_
_i13_S_L_:	mov	ecx,esi			 ; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [edi + 8]	; get register pointer
		mov	ebx,dword [eax]	; get register content
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,1fh			 ; modulo 32
		btr	ebx,edx
		jc	i13sl1
		or	ecx,4
i13sl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

global _i13_S_B_
_i13_S_B_:	mov	ecx,esi			  ; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi + 12]	 ; get ea routine pointer
		mov	edx,dword [edi + 8]	 ; get earegister pointer
		call	eax
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	ebx,7			  ; modulo 8
		btr	edx,ebx
		jc	i13sb1
		or	ecx,4
i13sb1:		mov	dword [sr],ecx
		mov	ebx,dword [edi + 8]
		mov	eax,dword [edi + 20]
		call	eax
		mov	eax,dword [edi + 16]
		ret


;==============================================================================
; BSET.B Dn,<ea>
; Tableusage dynamic:
; 2 - eareg 3-earead 4-reg 5-cycle count 6-eawrite
; Tableusage static:
; 2 - eareg 3-earead 4-cycle count 5-eawrite
;==============================================================================


		FALIGN32

global _i15_D_L_
_i15_D_L_:	mov	eax,dword [edi+8]   ; get ea register pointer
		mov	ecx,dword [edi+16]  ; get register with bit number
		mov	ebx,dword [eax]	    ; get ea register content
		mov	edx,dword [ecx]	    ; get bit number
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,01fh		 ; modulo 32
		bts	ebx,edx
		jc	i15dl1
		or	ecx,4
i15dl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx	    ; write ea register content
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i15_D_B_
_i15_D_B_:	mov	eax,dword [edi+12]  ; get ea routine pointer
		mov	edx,dword [edi+8]   ; get earegister pointer
		call	eax			; get ea-content
		mov	eax,dword [edi+16]  ; get bit number register pointer
		mov	eax,dword [eax]	    ; get bit number
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	eax,07h			; modulo 8
		bts	edx,eax
		jc	i15db1
		or	ecx,4
i15db1:		mov	dword [sr],ecx
		mov	ebx,dword [edi+8]
		mov	eax,dword [edi+24]
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i15_S_L_
_i15_S_L_:	mov	ecx,esi			; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [edi+8]   ; get register pointer
		mov	ebx,dword [eax]	    ; get register content
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,01fh		 ; modulo 32
		bts	ebx,edx
		jc	i15sl1
		or	ecx,4
i15sl1:		mov	dword [sr],ecx
		mov	dword [eax],ebx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i15_S_B_
_i15_S_B_:	mov	ecx,esi			; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]  ; get ea routine pointer
		mov	edx,dword [edi+8]   ; get earegister pointer
		call	eax
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	ebx,07h			; modulo 8
		bts	edx,ebx
		jc	i15sb1
		or	ecx,4
i15sb1:		mov	dword [sr],ecx
		mov	ebx,dword [edi+8]
		mov	eax,dword [edi+20]
		call	eax
		mov	eax,dword [edi+16]
		ret


;==============================================================================
; BTST.B Dn,<ea>
; Tableusage dynamic:
; 2 - eareg 3-earead 4-reg 5-cycle count 
; Tableusage static:
; 2 - eareg 3-earead 4-cycle count 
;==============================================================================


		FALIGN32

global _i17_D_L_
_i17_D_L_:	mov	eax,dword [edi+8]   ; get ea register pointer
		mov	ecx,dword [edi+16]  ; get register with bit number
		mov	ebx,dword [eax]	    ; get ea register content
		mov	edx,dword [ecx]	    ; get bit number
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,01fh		 ; modulo 32
		bt	ebx,edx
		jc	i17dl1
		or	ecx,4
i17dl1:		mov	dword [sr],ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i17_D_B_
_i17_D_B_:	mov	eax,dword [edi+12]  ; get ea routine pointer
		mov	edx,dword [edi+8]   ; get earegister pointer
		call	eax			; get ea-content
		mov	eax,dword [edi+16]  ; get bit number register pointer
		mov	eax,dword [eax]	    ; get bit number
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	eax,07h			; modulo 8
		bt	edx,eax
		jc	i17db1
		or	ecx,4
i17db1:		mov	dword [sr],ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i17_S_L_
_i17_S_L_:	mov	ecx,esi			; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [edi+8]   ; get register pointer
		mov	ebx,dword [eax]	    ; get register content
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	edx,01fh		 ; modulo 32
		bt	ebx,edx
		jc	i17sl1
		or	ecx,4
i17sl1:		mov	dword [sr],ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i17_S_B_
_i17_S_B_:	mov	ecx,esi			; get bit number
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]  ; get ea routine pointer
		mov	edx,dword [edi+8]   ; get earegister pointer
		call	eax
		mov	ecx,dword [sr]
		and	ecx,0fffffffbh
		and	ebx,07h			; modulo 8
		bt	edx,ebx
		jc	i17sb1
		or	ecx,4
i17sb1:		mov	dword [sr],ecx
		mov	eax,dword [edi+16]
		ret

       
;==============================================================================
; CHK.W <ea>,Dn
; 2 - eareg 3-earead 4-reg 5-cycle count
; NOTE:	Long version in 68030.asm
; Word:	012346
; Long:	XX2346
;==============================================================================


		FALIGN32

global _i18_
_i18_:		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		mov	ebx, dword [edi + 16]
		mov	eax, dword [ebx]
		and	dword [sr], 0fffffffbh
		test	ax, ax
		js	i18trapset
		cmp	ax, dx
		ja	i18trap
		mov	eax, dword [edi + 20]
		ret
i18trapset:	or	dword [sr],4
i18trap:	mov	ebx, 18h		; vector at $18 (nr. 6)
		mov	eax, 34
		add	eax, dword [edi + 20]
		jmp	_cpuPrepareException_


;==============================================================================
; CLR.? <ea>
; 2-eareg 3-eawrite 4-cyclecount
;==============================================================================


		FALIGN32

global _i19_
_i19_:		xor	edx,edx
		mov	eax,dword [edi+12]
		mov	ebx,dword [edi+8]
		call	eax
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,4
		mov	dword [sr],eax
		mov	eax,dword [edi + 16]
		ret

;==============================================================================
; CMP.X <ea>,Dn 
; Flag: All, X unaffected
;==============================================================================


		FALIGN32

global _i20_B_
_i20_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+16]
		mov	eax,dword [edi+20]
		mov	ecx,dword [ebx]	  
		cmp	cl,dl		      
		pushfd			      
		pop	ecx		      
		and	ecx,0fffh	      
		mov	ebx,dword [statustab+4*ecx]
		mov	ecx,dword [sr]	 
		and	ecx,0fffffff0h		
		and	ebx,0fh			
		or	ecx,ebx			
		mov	dword [sr],ecx	 
		ret				


		FALIGN32

global _i20_W_
_i20_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	eax,dword [edi+20]
		mov	ecx,dword [ebx]
		cmp	cx,dx
		pushfd
		pop	ecx
		and	ecx,0fffh
		mov	ebx,dword [statustab+4*ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		and	ebx,0fh
		or	ecx,ebx
		mov	dword [sr],ecx
		ret


		FALIGN32

global _i20_L_
_i20_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	eax,dword [edi+20]
		mov	ecx,dword [ebx]
		cmp	ecx,edx
		pushfd
		pop	ecx
		and	ecx,0fffh
		mov	ebx,dword [statustab+4*ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		and	ebx,0fh
		or	ecx,ebx
		mov	dword [sr],ecx
		ret


;==============================================================================
; CMPA.W/L <ea>,An 
; Flag: All, X unaffected
;==============================================================================


		FALIGN32

global _i21_W_
_i21_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	eax,dword [edi+20]
		movsx	edx,dx
		mov	ecx,dword [ebx]
		cmp	ecx,edx
		pushfd
		pop	ecx
		and	ecx,0fffh
		mov	ebx,dword [statustab+4*ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		and	ebx,0fh
		or	ecx,ebx
		mov	dword [sr],ecx
		ret
	
	
		FALIGN32

global _i21_L_	
_i21_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	eax,dword [edi+20]
		mov	ecx,dword [ebx]
		cmp	ecx,edx
		pushfd
		pop	ecx
		and	ecx,0fffh
		mov	ebx,dword [statustab+4*ecx]
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		and	ebx,0fh
		or	ecx,ebx
		mov	dword [sr],ecx
		ret


;==============================================================================
; CMPI.X #,<ea>
; Flag: NZCV, X unaffected
;==============================================================================


		FALIGN32

global _i22_B_
_i22_B_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		cmp	dl, bl
		pushfd
		pop	ecx
		mov	eax, dword [edi + 12]
		and	ecx, 0fffh
		mov	ebx, dword [sr]
		mov	ecx, dword [statustab + 4*ecx]
		and	ebx, 0fffffff0h
		and	ecx, 0fh
		or	ebx, ecx
		mov	dword [sr], ebx
		ret


		FALIGN32

global _i22_W_
_i22_W_:	mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		cmp	dx, bx
		pushfd
		pop	ecx
		mov	eax, dword [edi + 12]
		and	ecx, 0fffh
		mov	ebx, dword [sr]
		mov	ecx, dword [statustab + 4*ecx]
		and	ebx, 0fffffff0h
		and	ecx, 0fh
		or	ebx, ecx
		mov	dword [sr], ebx
		ret


		FALIGN32

global _i22_L_
_i22_L_:	mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ebx, edx
		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		cmp	edx, ebx
		pushfd
		pop	ecx
		mov	eax, dword [edi + 12]
		and	ecx, 0fffh
		mov	ebx, dword [sr]
		mov	ecx, dword [statustab + 4*ecx]
		and	ebx, 0fffffff0h
		and	ecx, 0fh
		or	ebx, ecx
		mov	dword [sr], ebx
		ret


;==============================================================================
; CMPM
; Tableusage 0- rut 1- disrut 2-soureg 3-destreg
; Flag: All, X unaffected
;==============================================================================


		FALIGN32

global _i23_B_
_i23_B_:	mov	eax,dword [edi+8]
		mov	ecx,dword [eax]
		fetchb
		inc	ecx
		cmp	eax, dword da_regs + 60
		jne	i23_b_j
		inc	ecx
i23_b_j:	mov	dword [eax],ecx
		mov	ebx,edx				; source in ebx
		mov	eax,dword [edi+12]		 
		mov	ecx,dword [eax]
		fetchb
		inc	ecx
		cmp	eax, dword da_regs + 60
		jne	i23_b_i
		inc	ecx
i23_b_i:	mov	dword [eax],ecx	       ; Dest in edx
		cmp	dl,bl
		pushfd
		pop	eax
		and	eax,0fffh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,12
		ret


		FALIGN32

global _i23_W_
_i23_W_:	mov	eax,dword [edi+8]
		mov	ecx,dword [eax]
		fetchw
		add	ecx,2
		mov	dword [eax],ecx
		mov	ebx,edx				; source in ebx
		mov	eax,dword [edi+12]		 
		mov	ecx,dword [eax]
		fetchw
		add	ecx,2
		mov	dword [eax],ecx	       ; Dest in edx
		cmp	dx,bx
		pushfd
		pop	eax
		and	eax,0fffh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,12
		ret


		FALIGN32

global _i23_L_
_i23_L_:	mov	eax,dword [edi+8]
		mov	ecx,dword [eax]
		fetchl
		add	ecx,4
		mov	dword [eax],ecx
		mov	ebx,edx				; source in ebx
		mov	eax,dword [edi+12]		 
		mov	ecx,dword [eax]
		fetchl
		add	ecx,4
		mov	dword [eax],ecx	       ; Dest in edx
		cmp	edx,ebx
		pushfd
		pop	eax
		and	eax,0fffh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,20
		ret


;==============================================================================
; DBcc
; Tableusage 0- rout 1- disrout 2-register*4+d 3-handle routine
; Flags: None
;==============================================================================


		FALIGN32

global _i24_
_i24_:		mov	eax,dword [edi+12]
		call	eax
		test	dl,dl
		jnz	near i24ferdig
		mov	ebx,dword [edi+8]
		mov	edx,dword [ebx]
		dec	dx
		mov	dword [ebx],edx
		cmp	dx,-1
		je	i24out
		mov	ecx,esi
		fetchwopcode_inc_pos
		movsx	edx,dx
		add	esi,edx
%ifdef PREFETCH
		mov	edx,esi
		call	prefetch_fillASM
%endif
		mov	eax,10
		ret
i24out:		mov	eax,14
%ifdef PREFETCH
		mov	ecx,esi
		fetchwopcode_inc_pos
%endif
		add	esi,2
		ret
i24ferdig:
		mov	eax,12
%ifdef PREFETCH
		mov	ecx,esi
		fetchwopcode_inc_pos
%endif
		add	esi,2
		ret


;==============================================================================
; DIVS.W <ea>,Dx
; 2-reg 3-ea read routine 4-dreg*4+d
; Flags: X unaffected, C = 0, NZV
; Cycle time varies, set to 70
;==============================================================================


		FALIGN32

global _i25_
_i25_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		movsx	ecx,dx
		or	ecx,ecx
		jz	i25divzero
		mov	ebx,dword [edi+16]
		mov	eax,dword [ebx]
		xor	edx,edx
		or	eax,eax
		jns	i25ns
		dec	edx
i25ns:		idiv	ecx
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		cmp	eax,32768			; Sjekk resultat overflow
		jle	i25noov1
		or	ecx,1				; Overflow
		jmp	i25fin
i25noov1:	cmp	eax,-32767			;overflow
		jge	i25noov2
		or	ecx,1
		jmp	i25fin
i25noov2:	or	eax,eax
		jnz	i25noz
		or	ecx,4
		jmp	i25finish
i25noz:		jns	i25finish
		or	ecx,8
i25finish:	and	eax,0000ffffh
		shl	edx,16
		and	edx,0ffff0000h
		or	eax,edx
		mov	dword [ebx],eax
i25fin:		mov	dword [sr],ecx
		mov	eax,70
		ret
i25divzero:	mov	ebx,14h
		mov	eax,200
		jmp	_cpuPrepareException_

;==============================================================================
; DIVU.W <ea>,Dx
; 2-reg 3-ea read routine 4-dreg*4+d
; Flags: X unaffected, C = 0, NZV
;==============================================================================


		FALIGN32

global _i26_
_i26_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		movzx	ecx,dx
		or	ecx,ecx
		jz	i26divzero
		mov	ebx,dword [edi+16]
		mov	eax,dword [ebx]
		xor	edx,edx
		idiv	ecx
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		cmp	eax,65536			; Sjekk resultat overflow
		jb	i26noov1
		or	ecx,1				; Overflow
		jmp	i26fin
i26noov1:	or	eax,eax
		jnz	i26noz
		or	ecx,4
		jmp	i26finish
i26noz:		jns	i26finish
		or	ecx,8
i26finish:	and	eax,0000ffffh
		shl	edx,16
		and	edx,0ffff0000h
		or	eax,edx
		mov	dword [ebx],eax
i26fin:		mov	dword [sr],ecx
		mov	eax,70 
		ret
i26divzero:	mov	ebx,14h
		mov	eax,182
		jmp	_cpuPrepareException_


;==============================================================================
; EOR.X Dn,<ea>
; Flag: X unaffected, V = C = 0, NZ
;==============================================================================


		FALIGN32

global _i27_B_
_i27_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ebx,dword [ebx]
		xor	dl,bl
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i27_W_
_i27_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ebx,dword [ebx]
		xor	dx,bx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i27_L_
_i27_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ebx,dword [ebx]
		xor	edx,ebx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret

;==============================================================================
; EORI.X #,<ea>
; Flag: X unaffected, V = C = 0, NZ
;==============================================================================


		FALIGN32

global _i28_B_
_i28_B_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		xor	dl,bl
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i28_B_CCR_
_i28_B_CCR_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [sr]
		xor	al,dl
		mov	dword [sr],eax
		mov	eax,20
		ret


		FALIGN32

global _i28_W_SR_
_i28_W_SR_:	test	byte [sr+1],020h
		jz	near _cpuPrivilegieViolation_
		mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [sr]
		xor	eax,edx
		mov	edx,eax
		call	_cpuUpdateSR_
		mov	eax,20
		ret


		FALIGN32

global _i28_W_
_i28_W_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		xor	dx,bx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i28_L_
_i28_L_:	mov	ecx,esi
		fetchlopcode_inc_pos
		add	esi,4
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		xor	edx,ebx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		and	ebx,0fh
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


;==============================================================================
; Instruction EXG.L 
; Tableusage 0 - routine  1 - disrout  2-reg1 3-reg2 
; Flag: NONE
;==============================================================================


		FALIGN32

global _i29_
_i29_:		mov	eax,dword [edi+8]
		mov	ebx,dword [edi+12]
		mov	ecx,dword [eax]
		mov	edx,dword [ebx]
		mov	dword [eax],edx
		mov	dword [ebx],ecx
		mov	eax,6
		ret

;==============================================================================
; Instruction EXT.L 
; Tableusage 0 - routine  1 - reg1  
; Flag: X unaffected, V = C = 0, NZ
;==============================================================================


		FALIGN32

global _i30_W_
_i30_W_:	mov	ecx, dword [edi + 4]
		mov	eax, 4
		mov	edx, dword [sr]
		and	edx, 0fffffff0h
		mov	ebx, dword [ecx]
		movsx	bx,bl
		mov	dword [ecx], ebx
		test	bx, bx
		jz	i30wz
		jns	i30wns
		or	edx, 8
		jmp	i30wns
i30wz:		or	edx, 4
i30wns:		mov	dword [sr], edx
		ret


		FALIGN32

global _i30_L_
_i30_L_:	mov	ecx, dword [edi + 4]
		mov	eax, 4
		mov	edx, dword [sr]
		and	edx, 0fffffff0h
		mov	ebx, dword [ecx]
		movsx	ebx, bx
		mov	dword [ecx], ebx
		test	ebx, ebx
		jz	i30lz
		jns	i30lns
		or	edx, 8
		jmp	i30lns
i30lz:		or	edx, 4
i30lns:		mov	dword [sr], edx
		ret


;==============================================================================
; Instruction JMP <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- eacalcrout 4- cycle time
; Flag: NONE
;==============================================================================


		FALIGN32

global	 _i31_
_i31_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	esi,edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		mov	eax,dword [edi+16]
		ret


;==============================================================================
; Instruction JSR <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- eacalcrout 4- cycle time
; Flag: NONE
;==============================================================================


		FALIGN32

global _i32_
_i32_:		mov	eax,dword [edi+12]	       ; get new pc
		mov	edx,dword [edi+8]
		call	eax
		xchg	esi,edx

		; The order is crucial, PTR_TO_PC sets a new
		; base-pointer.

		PTR_TO_PC edx
		PC_TO_PTR esi
		mov	ecx,dword [da_regs + 60]
		sub	ecx,4
		mov	dword [da_regs + 60],ecx
		writel
%ifdef PREFETCH
		mov	edx,esi
		call	prefetch_fillASM
%endif
		mov	eax,dword [edi+16]
		ret


;==============================================================================
; Instruction LEA <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- eacalcrout 4-areg 5-cycletime
; Flag: NONE
;==============================================================================


		FALIGN32

global _i33_
_i33_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ecx,dword [edi+16]
		mov	dword [ecx],edx	       
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; Instruction LINK An,#
; Tableusage 0 - routine  1 - disrout  2 - Register
; Flag: NONE
;==============================================================================


		FALIGN32
global _i34_
_i34_:		mov	ebx,dword [edi+8]
		mov	edx,dword [ebx]
		mov	ecx,dword [da_regs + 60]
		sub	ecx,4
		mov	dword [ebx],ecx	  ; Load An with updated SP
		writel
		mov	ebx,ecx
		mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		movsx	edx,dx
		add	ebx,edx
		mov	dword [da_regs + 60],ebx	       ; add disp to SP
		mov	eax,16
		ret


;==============================================================================
; LSX.X
; Tableusage:(imm) 0-rout 1-disrout 2-shcount 3-dreg 4-cyclecount
; Tableusage:(reg) 0-rout 1-disrout 2-shcountreg 3-dreg 4-cyclecount
; Tableusage:(mem) 0-rout 1-disrout 2-eareg 3-earead 4-eawrite 5-cyclecount
; Flag: ALL, V = 0
;==============================================================================


		FALIGN32

i35_zerocountb: test	al,al
		pushfd
		pop	ecx			; Only NZ, X is unaffected
		and	ecx,0c0h
		shr	ecx,4
		mov	ch,byte [sr]
		and	ch,10h
		or	ch,cl
		mov	byte [sr],ch
		mov	eax,4
		ret


		FALIGN32

i35_zerocountw: test	ax,ax
		pushfd
		pop	ecx			; Only NZ, X is unaffected
		and	ecx,0c0h
		shr	ecx,4
		mov	ch,byte [sr]
		and	ch,10h
		or	ch,cl
		mov	byte [sr],ch
		mov	eax,4
		ret


		FALIGN32

i35_zerocountl: test	eax,eax
		pushfd
		pop	ecx			; Only NZ, X is unaffected
		and	ecx,0c0h
		shr	ecx,4
		mov	ch,byte [sr]
		and	ch,10h
		or	ch,cl
		mov	byte [sr],ch
		mov	eax,4
		ret


		FALIGN32

global _i35_RI_I_B_
_i35_RI_I_B_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		shr	al,cl
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	ecx,dword [statustab+4*ecx]
		mov	byte [sr],cl
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i35_RI_I_W_
_i35_RI_I_W_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		shr	ax,cl
		pushfd	
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	ecx,dword [statustab+4*ecx]
		mov	byte [sr],cl
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i35_RI_I_L_
_i35_RI_I_L_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		shr	eax,cl
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	ecx,dword [statustab+4*ecx]
		mov	byte [sr],cl
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i35_RI_R_B_
_i35_RI_R_B_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountb
		cmp	ecx,32
		jb	i35rirbsok
		mov	ecx,31
i35rirbsok:	shr	al,cl
		pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_RI_R_W_
_i35_RI_R_W_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountw
		cmp	ecx,32
		jb	i35rirwsok
		mov	ecx,31
i35rirwsok:	shr	ax,cl
		pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_RI_R_L_
_i35_RI_R_L_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountl
		cmp	ecx,32
		jb	i35rirlok
		je	i35rircount32
		xor	eax,eax
		jmp	i35rirlnok
i35rircount32:	mov	edx,eax
		test	edx,edx
		js	i35rircount32c
		xor	eax,eax
		jmp	i35rirlnok
i35rircount32c: xor	eax,eax
		stc
		jmp	i35rirlnok
i35rirlok:	shr	eax,cl
i35rirlnok:	pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_LE_I_B_
_i35_LE_I_B_:	mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [edi+8]	       ; get shcount
		mov	eax,dword [ebx]	       ; get dreg content
		shl	al,cl
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		mov	ebx,dword [statustab+4*ecx]
		mov	byte [sr],bl
		ret


		FALIGN32

global _i35_LE_I_W_
_i35_LE_I_W_:	mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [edi+8]	       ; get shcount
		mov	eax,dword [ebx]	       ; get dreg content
		shl	ax,cl
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		mov	ebx,dword [statustab+4*ecx]
		mov	byte [sr],bl
		ret


		FALIGN32

global _i35_LE_I_L_
_i35_LE_I_L_:	mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [edi+8]	       ; get shcount
		mov	eax,dword [ebx]	       ; get dreg content
		shl	eax,cl
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		mov	ebx,dword [statustab+4*ecx]
		mov	byte [sr],bl
		ret


		FALIGN32

global _i35_LE_R_B_
_i35_LE_R_B_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountb
		cmp	ecx,32
		jb	i35lerbsok
		mov	ecx,31
i35lerbsok:	shl	al,cl
		pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_LE_R_W_
_i35_LE_R_W_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountw
		cmp	ecx,32
		jb	i35lerwsok
		mov	ecx,31
i35lerwsok:	shl	ax,cl
		pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_LE_R_L_
_i35_LE_R_L_:	mov	ecx,dword [edi+8]	       ; get shcount reg ptr
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	ecx,dword [ecx]	       ; get shcount reg content
		mov	eax,dword [ebx]	       ; get dreg content
		and	ecx,3fh				; modulo 64
		jz	near i35_zerocountl

		cmp	ecx,32
		jb	i35lerlok
		je	i35lercount32
		xor	eax,eax
		jmp	i35lerlnok
i35lercount32:	mov	edx,eax
		test	dl,1
		jnz	i35lercount32c
		xor	eax,eax
		jmp	i35lerlnok
i35lercount32c: xor	eax,eax
		stc
		jmp	i35lerlnok
i35lerlok:	shl	eax,cl
i35lerlnok:	pushfd
		mov	dword [ebx],eax
		mov	edx,ecx
		add	edx,edx				; Calculate cyclecount
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [statustab+4*ecx]
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		add	eax,edx
		ret


		FALIGN32

global _i35_RI_M_W_
_i35_RI_M_W_:	mov	ebx,dword [edi+8]	       ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]	       ; get ea read rout
		call	eax
		shr	dx,1
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [edi+16]	       ; get dest rout ptr.
		mov	ecx,dword [statustab+4*ecx]
		mov	byte [sr],cl
		call	eax	
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i35_LE_M_W_
_i35_LE_M_W_:	mov	ebx,dword [edi+8]	       ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]	       ; get ea read rout
		call	eax
		shl	dx,1
		pushfd
		pop	ecx
		and	ecx,07ffh			; Overflow always 0
		mov	eax,dword [edi+16]	       ; get dest rout ptr.
		mov	ecx,dword [statustab+4*ecx]
		mov	byte [sr],cl
		call	eax	
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; Instruction 37   MOVE.X <ea>,<ea>
; Table usage: 2 - get src ea  3 - src reg  4 - set dst ea  5 - cycles
;	       6 - dst reg
; Flags: NZ, V=C=0, X unaffected
;==============================================================================


		FALIGN32

global _i37_B_
_i37_B_:	mov	ecx, dword [edi + 8]   
		mov	ebx, dword [sr]	  
		mov	edx, dword [edi + 12]	 
		and	ebx, 0fffffff0h		 
		call	ecx			 
		mov	ebp, dword [edi + 16]	
		or	dl, dl			 
		pushfd				 
		mov	eax, dword [edi + 20]	
		pop	ecx			 
		and	ecx, 07feh		 
		or	ebx, dword [statustab + 4*ecx]
		mov	dword [sr], ebx	   
		mov	ebx, dword [edi + 24]
		jmp	ebp	 


		FALIGN32

global _i37_W_
_i37_W_:	mov	ecx,dword [edi+8]	
		mov	ebx,dword [sr]	   
		mov	edx,dword [edi+12]	  
		and	ebx,0fffffff0h		  
		call	ecx			  
		mov	ebp,dword [edi+16]	 
		and	edx,0ffffh
		or	dx,dx			  
		pushfd				  
		pop	ecx			  
		and	ecx,07feh		  
		mov	eax,dword [edi+20]	 
		or	ebx,dword [statustab+4*ecx]
		mov	dword [sr],ebx	  
		mov	ebx,dword [edi+24]	
		jmp	ebp			 


		FALIGN32

global _i37_L_
_i37_L_:	mov	ecx,dword [edi+8]
		mov	ebx,dword [sr]	   
		mov	edx,dword [edi+12] 
		and	ebx,0fffffff0h	      
		call	ecx		      
		mov	ebp,dword [edi+16]
		or	edx,edx		      
		pushfd			      
		pop	ecx		      
		and	ecx,07feh	      
		mov	eax,dword [edi+20]
		or	ebx,dword [statustab+4*ecx]
		mov	dword [sr],ebx	
		mov	ebx,dword [edi+24] 
		jmp	ebp		       


;==============================================================================
; MOVE to CCR
; 2-_sreg 3-read 4-cycle time
;==============================================================================


		FALIGN32

global _i38_
_i38_:		mov	eax,dword [edi + 12]
		mov	edx,dword [edi + 8]
		call	eax
		mov	byte [sr], dl
		mov	eax, dword [edi + 16]
		ret


;==============================================================================
; MOVE to SR
; 2-_sreg 3-read 4-cycle time
; Privileged instruction, cannot be executed by user state programs
;==============================================================================


		FALIGN32

global _i39_
_i39_:		test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
		mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		call	_cpuUpdateSR_
		mov	eax, dword [edi + 16]
		ret


;==============================================================================
; MOVE from SR
; 2-_dreg 3-destwrite 4-cycle time
;==============================================================================


		FALIGN32

global _i40_
_i40_:		cmp	dword [cpu_type], 0
		jz	i40ok
		test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
i40ok:		mov	eax, dword [edi + 12]
		mov	ebx, dword [edi + 8]
		mov	edx, dword [sr]
		call	eax
		mov	eax, dword [edi + 16]
		ret


;==============================================================================
; MOVE USP
; 2-_Areg
; Privileged instruction, cannot be executed by user state programs
;==============================================================================

; to Ax


		FALIGN32

global _i41_1_
_i41_1_:	test	byte [sr + 1], 020h	    ; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	eax, 4
		mov	edx, dword [usp]	 
		mov	ecx, dword [edi + 8]
		mov	dword [ecx], edx
		ret

; from Ax


		FALIGN32

global _i41_2_
_i41_2_:	test	byte [sr + 1], 020h	   ; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ebx, dword [edi + 8]
		mov	eax, 4
		mov	edx, dword [ebx]     
		mov	dword [usp], edx
		ret


;==============================================================================
; MOVEA
; 2-_sreg 3-Srcread  4-Areg 5-cycle time
; Flag: NONE
;==============================================================================


		FALIGN32

global _i42_W_
_i42_W_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8]
		call	eax
		movsx	edx, dx
		mov	ebx, dword [edi + 16]
		mov	eax, dword [edi + 20]
		mov	dword [ebx], edx
		ret


		FALIGN32

global _i42_L_
_i42_L_:	mov	eax, dword [edi + 12]
		mov	edx, dword [edi + 8] 
		call	eax
		mov	ebx, dword [edi + 16]
		mov	eax, dword [edi + 20]
		mov	dword [ebx], edx
		ret
	

;==============================================================================
; MOVEM
; 2-reg 3-eacalc  4-cycle time
; Flag: NONE
;==============================================================================


; MOVEM.W regs, -(Ax)
; Predecrement, to memory
; Order: d0-d7,a0-a7   a7 first

		FALIGN32

global _MOVEM_W_PREM_
_MOVEM_W_PREM_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs + 64	; pointer to A7
.l1:		sub	eax, 4			; Point to 1 register less
		shr	ebx, 1
		jnc	.l2			; Skip this register
		mov	edx, dword [eax]	; get register content
		mov	ecx, dword [edi + 8]	; register to predecrement
		sub	dword [ecx], 2		; Decrement register
		mov	ecx, dword [ecx]	; get ea
		add	ebp, 4			; 4 cycles each register
		writew				; store word
.l2:		cmp	eax, dword da_regs
		jne	.l1
		lea	eax, [8 + ebp]
		ret

; MOVEM.L regs, -(Ax)
; Predecrement, to memory
; Order: d0-d7,a0-a7   a7 first


		FALIGN32

global _MOVEM_L_PREM_
_MOVEM_L_PREM_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs + 64	; pointer to A7
.l1:		sub	eax, 4
		shr	ebx, 1
		jnc	.l2			; Skip this register
		mov	edx, dword [eax]	; get register content
		mov	ecx, dword [edi + 8]	; register to predecrement
		sub	dword [ecx], 4	        ; Decrement register
		mov	ecx, dword [ecx]	; get ea
		add	ebp, 8			; 8 cycles for each register
		writel				; store word
.l2:		cmp	eax, dword da_regs
		jne	.l1
		lea	eax, [8 + ebp]
		ret

; MOVEM.W (Ax)+, regs
; Postincrement, to registers
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_W_POSTR_
_MOVEM_W_POSTR_:mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to D0
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		mov	edx, dword [edi + 8]	; register to postincrement
		add	ebp, 4			; 4 cycles for each register
		mov	ecx, dword [edx]	; get ea
		add	dword [edx], 2		; increment register
		fetchw				; get memory content
		movsx	edx, dx
		mov	dword [eax], edx	; store to register
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		lea	eax, [8 + ebp]
		ret

; MOVEM.L (Ax)+, regs
; Postincrement, to registers
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_L_POSTR_
_MOVEM_L_POSTR_:mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to D0
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		mov	edx, dword [edi + 8]	; register to postincrement
		add	ebp, 8			; 8 cycles for each register
		mov	ecx, dword [edx]	; get ea
		add	dword [edx], 4	        ; increment register
		fetchl				; get memory content
		mov	dword [eax], edx	; store to register
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		lea	eax, [8 + ebp]
		ret

; MOVEM.W <control ea>, regs
; Control, to registers
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_W_CONR_
_MOVEM_W_CONR_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		mov	edx, dword [edi + 8]
		mov	ecx, dword [edi + 12]	; calculate first ea
		call	ecx
		mov	ecx, edx		; Source address in ecx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to D0
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		add	ebp, 4			; 4 cycles for each register
		fetchw				; get memory content
		add	ecx, 2			; increment ea 
		movsx	edx, dx
		mov	dword [eax], edx	; store to register
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		mov	eax,dword [edi + 16]
		add	eax, ebp
		ret

; MOVEM.L <control ea>, regs
; Control, to registers
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_L_CONR_
_MOVEM_L_CONR_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		mov	edx, dword [edi + 8]
		mov	ecx, dword [edi + 12]	; calculate first ea
		call	ecx
		mov	ecx, edx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to D0
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		fetchl				; get memory content
		add	ebp, 8			; 8 cycles for each register
		add	ecx, 4			; increment ea 
		mov	dword [eax], edx	; store to register
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret

; MOVEM.W regs, <control ea>
; Control, to memory
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_W_CONM_
_MOVEM_W_CONM_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		push	edx
		mov	edx, dword [edi + 8]
		mov	ecx, dword [edi + 12]	; calculate first ea
		call	ecx
		mov	ecx, edx		; ecx is ea
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to d0
		pop	ebx
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		add	ebp, 4			; 4 cycles for each register
		mov	edx, dword [eax]
		writew				; get memory content
		add	ecx, 2			; increment ea 
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret

; Control, to memory
; Order: a7-a0,d7-d0   d0 first


		FALIGN32

global _MOVEM_L_CONM_
_MOVEM_L_CONM_:	mov	ecx, esi		; get register mask word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Register mask in ebx
		mov	edx, dword [edi + 8]
		mov	ecx, dword [edi + 12]	; calculate first ea
		call	ecx
		mov	ecx, edx
		xor	ebp, ebp		; count cycles in ebp
		mov	eax, dword da_regs	; pointer to d0
.l1:		shr	ebx, 1
		jnc	.l2			; Skip this register
		add	ebp, 8			; 8 cycles for each register
		mov	edx, dword [eax]
		writel				; store memory content
		add	ecx, 4			; increment ea 
.l2:		add	eax, 4			; Point to 1 register more
		cmp	eax, dword da_regs + 64
		jne	.l1
		mov	eax, dword [edi + 16]
		add	eax, ebp
		ret

;==============================================================================
; MOVEP
; Table:  2- areg 3-dreg
;==============================================================================

; word, mem to reg


		FALIGN32

global _i44_W_1_
_i44_W_1_:	mov	eax,dword [edi+8]   ; Get areg
		mov	eax,dword [eax]
		mov	ecx,esi
		fetchwopcode_inc_pos			; Get disp
		movsx	edx,dx			; Sign extend
		add	eax,edx			; Add
		mov	ecx,eax
		fetchb			  ; Get first byte
		mov	bh,dl
		add	eax,2
		mov	ecx,eax
		fetchb			  ; get second byte
		mov	bl,dl
		mov	eax,dword [edi+12]  ; Get data reg
		mov	word [eax],bx	    ; Store word
		add	esi,2
		mov	eax,16
		ret

; Long, mem to reg


		FALIGN32

global _i44_L_1_
_i44_L_1_:	mov	eax,dword [edi+8]   ; Get areg
		mov	eax,dword [eax]
		mov	ecx,esi
		fetchwopcode_inc_pos			; Get disp
		movsx	edx,dx			; Sign extend
		add	eax,edx			; Add
		mov	ecx,eax
		fetchb			  ; Get first byte
		mov	bh,dl
		add	eax,2
		mov	ecx,eax
		fetchb			  ; get second byte
		mov	bl,dl
		shl	ebx,16
		add	eax,2
		mov	ecx,eax
		fetchb			  ; get third byte
		mov	bh,dl
		add	eax,2
		mov	ecx,eax
		fetchb			  ; get fourth byte
		mov	bl,dl
		mov	eax,dword [edi+12]  ; Get data reg
		mov	dword [eax],ebx	; Store word
		add	esi,2
		mov	eax,20
		ret

; Word, reg to mem


		FALIGN32

global _i44_W_2_
_i44_W_2_:	mov	eax,dword [edi+8]   ; Get areg
		mov	eax,dword [eax]
		mov	ecx,esi
		fetchwopcode_inc_pos			; Get disp
		movsx	edx,dx			; Sign extend
		add	eax,edx			; Add
		mov	ebx,dword [edi+12]  ; Get data reg
		mov	ebx,dword [ebx]	    ; get reg contents
		rol	bx,8
		mov	dl,bl
		mov	ecx,eax
		writeb			  ; Write first byte
		rol	bx,8
		mov	dl,bl
		add	eax,2
		mov	ecx,eax
		writeb			  ; Write second byte
		add	esi,2
		mov	eax,16
		ret

; Long, reg to mem

		FALIGN32

global _i44_L_2_
_i44_L_2_:	mov	eax,dword [edi+8]   ; Get areg
		mov	eax,dword [eax]
		mov	ecx,esi
		fetchwopcode_inc_pos			; Get disp
		movsx	edx,dx			; Sign extend
		add	eax,edx			; Add
		mov	ebx,dword [edi+12]  ; Get data reg
		mov	ebx,dword [ebx]	    ; get reg contents
		rol	ebx,8
		mov	dl,bl
		mov	ecx,eax
		writeb			  ; Write first byte
		rol	ebx,8
		mov	dl,bl
		add	eax,2
		mov	ecx,eax
		writeb			  ; Write second byte
		rol	ebx,8
		mov	dl,bl
		add	eax,2
		mov	ecx,eax
		writeb			  ; Write third byte
		rol	ebx,8
		mov	dl,bl
		add	eax,2
		mov	ecx,eax
		writeb			  ; Write fourth byte
		add	esi,2
		mov	eax,20
		ret


;==============================================================================
; MOVEQ.L #,Dx
; 2-Dreg 3-32 bit immediate  4-flag byte
; Flag: NZ, X unaffected, V=C=0
;==============================================================================


		FALIGN32

global _i45_
_i45_:		mov	ebx,dword [edi+8]
		mov	ecx,dword [sr]
		mov	eax,dword [edi+12]
		mov	dword [ebx],eax
		mov	ebx,dword [edi+16]
		and	ecx,0fffffff0h
		or	ecx,ebx
		mov	eax,4
		mov	dword [sr],ecx
		ret


		FALIGN32

global _i45ai_
_i45ai_:	mov	edx,edi
		and	edx,0f80000h
		cmp	edx,0f80000h
		jne	near _i00_
		mov	ebx,dword [edi+8]
		mov	ecx,dword [sr]
		mov	eax,dword [edi+12]
		mov	dword [ebx],eax
		mov	ebx,dword [edi+16]
		and	ecx,0fffffff0h
		or	ecx,ebx
		mov	eax,4
		mov	dword [sr],ecx
		ret


;==============================================================================
; MULS.W <ea>,Dx
; 2-reg 3-ea read rout 4-dreg*4+d
; Flag: NZ, X unaffected, V=C=0
;==============================================================================


		FALIGN32

global _i46_
_i46_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		movsx	ecx,dx
		mov	ebp,edx
		mov	ebx,dword [edi+16]
		movsx	eax,word [ebx]
		imul	eax,ecx
		mov	dword [ebx],eax
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		or	eax,eax
		jnz	i46noz
		or	ecx,4
		jmp	i46finish
i46noz:		jns	i46finish
		or	ecx,8
i46finish:	mov	dword [sr],ecx
		mov	ebx,ebp
		mov	cl,bh
		and	ebx,0ffh
		and	ecx,0ffh
		mov	eax,40
		add	al,byte [mulstab + ebx]
		add	al,byte [mulstab + ecx]
		ret


;==============================================================================
; MULU.W <ea>,Dx
; 2-reg 3-ea read rout 4-dreg*4+d
; Flag: NZ, X unaffected, V=C=0
;==============================================================================


		FALIGN32

global _i47_
_i47_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		movzx	ecx,dx
		mov	ebp,edx
		mov	ebx,dword [edi+16]
		mov	eax,dword [ebx]
		movzx	eax,ax
		xor	edx,edx
i47ns:		imul	ecx
		mov	ecx,dword [sr]
		and	ecx,0fffffff0h
		or	eax,eax
		jnz	i47noz
		or	ecx,4
		jmp	i47finish
i47noz:		jns	i47finish
		or	ecx,8
i47finish:	mov	dword [ebx],eax
		mov	dword [sr],ecx
		mov	ebx,ebp
		mov	cl,bh
		and	ebx,0ffh
		and	ecx,0ffh
		mov	eax,40
		add	al,byte [mulutab + ebx]
		add	al,byte [mulutab + ecx]
		ret


;==============================================================================
; Instruction NBCD <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- earead 4- eawrite 5-cycletime
;==============================================================================


		FALIGN32

global _i48_B_
_i48_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]	       ; get operand
		mov	ebp,edx
		call	eax
		movzx	edx,dl
		xor	eax,eax
		mov	ah,byte [bcd2hex+edx]	   ; convert 2 hex
		mov	al,100
		sub	al,ah
		mov	ah,byte [sr]
		test	ah,010h
		jz	i48_bnx
		dec	al
i48_bnx:	and	ah,0eeh	 ; mask	 1110 1110
		or	al,al
		jz	i48_bz
		and	ah,0eah
i48_bz:		cmp	al,100
		je	i48_bnc
		or	ah,11h
		jmp	i48_bc
i48_bnc:	xor	al,al
i48_bc:		mov	byte [sr],ah
		and	eax,0ffh
		mov	dl,byte [hex2bcd+eax]
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; Instruction NEG <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- earead 4- eawrite 5-cycletime
; Flag: All, X=C
;==============================================================================


		FALIGN32

global _i49_B_
_i49_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		neg	dl
		pushfd
		pop	eax
		and	eax,0fffh		    
		mov	ebx,dword [statustab+4*eax]
		mov	byte [sr],bl
		mov	eax,dword [edi+16]
		mov	ebx,dword [edi+8]
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i49_W_
_i49_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		neg	dx
		pushfd
		pop	eax
		and	eax,0fffh
		mov	ebx,dword [statustab+4*eax]
		mov	byte [sr],bl	     
		mov	eax,dword [edi+16]
		mov	ebx,dword [edi+8]
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i49_L_
_i49_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		neg	edx
		pushfd
		pop	eax
		and	eax,0fffh		    
		mov	ebx,dword [statustab+4*eax]
		mov	byte [sr],bl	     
		mov	eax,dword [edi+16]
		mov	ebx,dword [edi+8]
		call	eax
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; Instruction NEGX <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- earead 4- eawrite 5-cycletime
; Flag: All, Z unchanged if result is non-zero
;==============================================================================


		FALIGN32

global _i50_B_
_i50_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		mov	eax,dword [sr]
		test	al,10h
		jz	i50_b1
		inc	dl
i50_b1:		neg	dl
		pushfd
		pop	eax
		and	eax,0fbfh		    
		mov	eax,dword [statustab+4*eax]
		mov	ah,byte [sr]
		and	ah,4				; Preserve Z
		test	dl,dl				; Clear if nonzero
		jz	i50bnz
		xor	ah,ah
i50bnz:		or	al,ah
		mov	byte [sr],al	      
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32
		
global _i50_W_
_i50_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		mov	eax,dword [sr]
		test	al,10h
		jz	i50_w1
		inc	dx
i50_w1:		neg	dx
		pushfd
		pop	eax
		and	eax,0fbfh		    
		mov	eax,dword [statustab+4*eax]
		mov	ah,byte [sr]
		and	ah,4				; Preserve Z
		test	dx,dx				; Clear if nonzero
		jz	i50wnz
		xor	ah,ah
i50wnz:		or	al,ah
		mov	byte [sr],al
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i50_L_
_i50_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		mov	eax,dword [sr]
		test	al,10h
		jz	i50_l1
		inc	edx
i50_l1:		neg	edx
		pushfd
		pop	eax
		and	eax,0fbfh		    
		mov	eax,dword [statustab+4*eax]
		mov	ah,byte [sr]
		and	ah,4				; Preserve Z
		test	edx,edx				; Clear if nonzero
		jz	i50lnz
		xor	ah,ah
i50lnz:		or	al,ah
		mov	byte [sr],al	   
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; NOP
; Flag: NONE
;==============================================================================


		FALIGN32

global _i51_
_i51_:		mov	eax, 4
		ret


;==============================================================================
; Instruction NOT <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- earead 4- eawrite 5-cycletime
; Flag: NZ, V=C=0, X unaffected
;==============================================================================


		FALIGN32

global _i52_B_
_i52_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		not	dl
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	dl,dl
		js	i52_bn
		jnz	i52_bnz
		or	eax,4
		jmp	i52_bnz
i52_bn:		or	eax,8
i52_bnz:	mov	dword [sr],eax 
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i52_W_
_i52_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		not	dx
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	dx,dx
		js	i52_wn
		jnz	i52_wnz
		or	eax,4
		jmp	i52_wnz
i52_wn:		or	eax,8
i52_wnz:	mov	dword [sr],eax
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i52_L_
_i52_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		mov	ebp,edx
		call	eax
		not	edx
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	edx,edx
		js	near i52_ln
		jnz	near i52_lnz
		or	eax,4
		jmp	i52_lnz
i52_ln:		or	eax,8
i52_lnz:	mov	dword [sr],eax
		mov	eax,dword [edi+16]
		mov	ebx,ebp
		call	eax
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; OR.X <ea>,Dn / OR.X Dn,<ea>
; Flagg: X unaffected, N Z, V = C = 0
;==============================================================================


		FALIGN32

global _i53_B_1_
_i53_B_1_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		or	cl,dl
		pushfd
		pop	eax
		and	eax,07feh
		mov	dword [ebx],ecx
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i53_W_1_
_i53_W_1_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		or	cx,dx
		pushfd
		pop	eax
		and	eax,07feh
		mov	dword [ebx],ecx
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i53_L_1_
_i53_L_1_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		or	ecx,edx
		pushfd
		pop	eax
		and	eax,07feh
		mov	dword [ebx],ecx
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	dword [sr],eax
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i53_B_2_
_i53_B_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	dh,byte [ebx]
		or	dl,dh
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i53_W_2_
_i53_W_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		or	dx,cx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret


		FALIGN32

global _i53_L_2_
_i53_L_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		or	edx,ecx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+24]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+20]
		ret


;==============================================================================
; ORI.X #,<ea>
; Flagg: X unaffected, N Z, V = C = 0
;==============================================================================


		FALIGN32

global _i54_B_
_i54_B_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		or	dl,bl
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i54_W_
_i54_W_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		or	dx,bx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i54_L_
_i54_L_:	mov	ecx,esi
		fetchlopcode_inc_pos
		add	esi,4
		mov	ebx,edx
		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		or	edx,ebx
		pushfd
		pop	eax
		and	eax,07feh
		mov	ebx,dword [statustab+4*eax]
		mov	eax,dword [sr]
		and	eax,0fffffff0h
		or	eax,ebx
		mov	ecx,dword [edi+20]
		mov	dword [sr],eax
		mov	ebx,dword [edi+8]
		call	ecx
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i54_B_CCR_
_i54_B_CCR_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [sr]
		or	al,dl
		mov	dword [sr],eax
		mov	eax,20
		ret


		FALIGN32

global _i54_W_SR_
_i54_W_SR_:	test	byte [sr + 1],020h
		jz	near _cpuPrivilegieViolation_
		mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	eax,dword [sr]
		or	eax,edx
		mov	edx,eax
		call	_cpuUpdateSR_
		mov	eax,20
		ret

;==============================================================================
; Instruction PEA <ea>
; Tableusage 0 - routine  1 - disrout  2 - eareg 3- eacalcrout 4- cycle time
;==============================================================================


		FALIGN32

global _i55_
_i55_:		mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ecx,dword [da_regs + 60]
		sub	ecx,4
		mov	dword [da_regs + 60],ecx
		writel
		mov	eax,dword [edi+16]
		ret


;==============================================================================
; Instruction RESET
;==============================================================================


		FALIGN32

global _i56_
_i56_:		pushad
		call	fellowSoftReset
		popad
		mov	eax,132
		ret


;==============================================================================
; ROX.X
; Tableusage:(imm) 0-rout 1-disrout 2-shcount 3-dreg 4-cyclecount
; Tableusage:(reg) 0-rout 1-disrout 2-shcountreg 3-dreg 4-cyclecount
; Tableusage:(mem) 0-rout 1-disrout 2-eareg 3-earead 4-eawrite 5-cyclecount
;==============================================================================

; Immediate rotates do not have the problems with count > 31, or zero
; rotates.


		FALIGN32

global _i57_RI_I_B_
_i57_RI_I_B_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		and	ch,10h
		ror	al,cl
		jnc	i57ribnocs
		or	ch,1
i57ribnocs:	test	al,al
		jz	i57rib1
		jns	i57rib2
		or	ch,8				; set N flag
		jmp	i57rib2
i57rib1:	or	ch,4				; set Z flag
i57rib2:	mov	byte [sr],ch	       ; Store flags
		mov	byte [ebx],al
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i57_RI_I_W_
_i57_RI_I_W_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch, byte [sr]
		and	ch,10h
		ror	ax,cl
		jnc	i57riwnocs
		or	ch,1
i57riwnocs:	test	ax,ax
		jz	i57riw1
		jns	i57riw2
		or	ch,8				; set N flag
		jmp	i57riw2
i57riw1:	or	ch,4				; set Z flag
i57riw2:	mov	byte [sr],ch	       ; Store flags
		mov	word [ebx],ax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i57_RI_I_L_
_i57_RI_I_L_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		and	ch,10h
		ror	eax,cl
		jnc	i57rilnocs
		or	ch,1
i57rilnocs:	test	eax,eax
		jz	i57ril1
		jns	i57ril2
		or	ch,8				; set N flag
		jmp	i57ril2
i57ril1:	or	ch,4				; set Z flag
i57ril2:	mov	byte [sr],ch	       ; Store flags
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i57_LE_I_B_
_i57_LE_I_B_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		and	ch,10h
		rol	al,cl
		jnc	i57libnocs
		or	ch,1
i57libnocs:	test	al,al
		jz	i57lib1
		jns	i57lib2
		or	ch,8				; set N flag
		jmp	i57lib2
i57lib1:	or	ch,4				; set Z flag
i57lib2:	mov	byte [sr],ch	       ; Store flags
		mov	byte [ebx],al
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i57_LE_I_W_
_i57_LE_I_W_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		and	ch,10h
		rol	ax,cl
		jnc	i57liwnocs
		or	ch,1
i57liwnocs:	test	ax,ax
		jz	i57liw1
		jns	i57liw2
		or	ch,8				; set N flag
		jmp	i57liw2
i57liw1:	or	ch,4				; set Z flag
i57liw2:	mov	byte [sr],ch	       ; Store flags
		mov	word [ebx],ax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i57_LE_I_L_
_i57_LE_I_L_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		and	ch,10h
		rol	eax,cl
		jnc	i57lilnocs
		or	ch,1
i57lilnocs:	test	eax,eax
		jz	i57lil1
		jns	i57lil2
		or	ch,8				; set N flag
		jmp	i57lil2
i57lil1:	or	ch,4				; set Z flag
i57lil2:	mov	byte [sr],ch	       ; Store flags
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		ret


		FALIGN32


global _i57_RI_R_B_
_i57_RI_R_B_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57rrbzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,010h
i57rrb7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57rrbok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		ror	al,cl
		mov	cl,dl
		jmp	i57rrb7
i57rrbok:	ror	al,cl
		jnc	i57rrbnocs
		or	ch,1
i57rrbnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	al,al
		jz	i57rrb1
		jns	i57rrb2
		or	ch,8			; set N flag
		jmp	i57rrb2
i57rrb1:	or	ch,4			; set Z flag
i57rrb2:	mov	byte [sr],ch	   ; Store flags
		mov	byte [ebx],al
		mov	eax,ebp
		ret

i57rrbzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; al is data
		test	al,al
		jz	i57rrbzero2
		jns	i57rrbzero3
		or	cl,8			; set N flag
		jmp	i57rrbzero3
i57rrbzero2:	or	cl,4			; set Z flag
i57rrbzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32


global _i57_RI_R_W_
_i57_RI_R_W_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57rrwzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,10h
i57rrw7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57rrwok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		ror	ax,cl
		mov	cl,dl
		jmp	i57rrw7
i57rrwok:	ror	ax,cl
		jnc	i57rrwnocs
		or	ch,1
i57rrwnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	ax,ax
		jz	i57rrw1
		jns	i57rrw2
		or	ch,8			; set N flag
		jmp	i57rrw2
i57rrw1:	or	ch,4			; set Z flag
i57rrw2:	mov	byte [sr],ch	   ; Store flags
		mov	word [ebx],ax
		mov	eax,ebp
		ret

i57rrwzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; ax is data
		test	ax,ax
		jz	i57rrwzero2
		jns	i57rrwzero3
		or	cl,8			; set N flag
		jmp	i57rrwzero3
i57rrwzero2:	or	cl,4			; set Z flag
i57rrwzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32


global _i57_RI_R_L_
_i57_RI_R_L_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57rrlzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,10h
i57rrl7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57rrlok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		ror	eax,cl
		mov	cl,dl			; Update rotate count
		jmp	i57rrl7
i57rrlok:	ror	eax,cl
		jnc	i57rrlnocs
		or	ch,1
i57rrlnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	eax,eax
		jz	i57rrl1
		jns	i57rrl2
		or	ch,8			; set N flag
		jmp	i57rrl2
i57rrl1:	or	ch,4			; set Z flag
i57rrl2:	mov	byte [sr],ch	   ; Store flags
		mov	dword [ebx],eax
		mov	eax,ebp
		ret

i57rrlzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; eax is data
		test	eax,eax
		jz	i57rrlzero2
		jns	i57rrlzero3
		or	cl,8			; set N flag
		jmp	i57rrlzero3
i57rrlzero2:	or	cl,4			; set Z flag
i57rrlzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32


global _i57_LE_R_B_
_i57_LE_R_B_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57lrbzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,10h
i57lrb7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57lrbok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		rol	al,cl
		mov	cl,dl			; Update rotate count
		jmp	i57lrb7
i57lrbok:	rol	al,cl
		jnc	i57lrbnocs
		or	ch,1
i57lrbnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	al,al
		jz	i57lrb1
		jns	i57lrb2
		or	ch,8			; set N flag
		jmp	i57lrb2
i57lrb1:	or	ch,4			; set Z flag
i57lrb2:	mov	byte [sr],ch	   ; Store flags
		mov	byte [ebx],al
		mov	eax,ebp
		ret

i57lrbzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; al is data
		test	al,al
		jz	i57lrbzero2
		jns	i57lrbzero3
		or	cl,8			; set N flag
		jmp	i57lrbzero3
i57lrbzero2:	or	cl,4			; set Z flag
i57lrbzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i57_LE_R_W_
_i57_LE_R_W_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57lrwzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,10h
i57lrw7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57lrwok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		rol	ax,cl
		mov	cl,dl			; Update rotate count
		jmp	i57lrw7
i57lrwok:	rol	ax,cl
		jnc	i57lrwnocs
		or	ch,1
i57lrwnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	ax,ax
		jz	i57lrw1
		jns	i57lrw2
		or	ch,8			; set N flag
		jmp	i57lrw2
i57lrw1:	or	ch,4			; set Z flag
i57lrw2:	mov	byte [sr],ch	   ; Store flags
		mov	word [ebx],ax
		mov	eax,ebp
		ret

i57lrwzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; ax is data
		test	ax,ax
		jz	i57lrwzero2
		jns	i57lrwzero3
		or	cl,8			; set N flag
		jmp	i57lrwzero3
i57lrwzero2:	or	cl,4			; set Z flag
i57lrwzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i57_LE_R_L_
_i57_LE_R_L_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i57lrlzero
		push	ecx
		mov	ch,byte [sr]
		and	ch,10h
i57lrl7:	cmp	cl,20h			; Run several passes of 31
		jb	near i57lrlok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		rol	eax,cl
		mov	cl,dl			; Update rotate count
		jmp	i57lrl7
i57lrlok:	rol	eax,cl
		jnc	i57lrlnocs
		or	ch,1
i57lrlnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	eax,eax
		jz	i57lrl1
		jns	i57lrl2
		or	ch,8			; set N flag
		jmp	i57lrl2
i57lrl1:	or	ch,4			; set Z flag
i57lrl2:	mov	byte [sr],ch	   ; Store flags
		mov	dword [ebx],eax
		mov	eax,ebp
		ret

i57lrlzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; eax is data
		test	eax,eax
		jz	i57lrlzero2
		jns	i57lrlzero3
		or	cl,8			; set N flag
		jmp	i57lrlzero3
i57lrlzero2:	or	cl,4			; set Z flag
i57lrlzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend memory word 1 to the right

global _i57_RI_M_W_
_i57_RI_M_W_:	mov	ebx,dword [edi+8]	       ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]	       ; get ea read rout
		call	eax
		mov	al,byte [sr]
		and	al,010h
		ror	dx,1
		jnc	i57rmwnocs
		or	al,1
i57rmwnocs:	test	dx,dx
		jz	i57rmw1
		jns	i57rmw2
		or	al,8				; set N flag
		jmp	i57rmw2
i57rmw1:	or	al,4				; set Z flag
i57rmw2:	mov	byte [sr],al	       ; Store flags
		mov	eax,dword [edi+16]
		call	eax	
		mov	eax,dword [edi+20]
		ret


		FALIGN32

; Rotate extend memory word 1 to the left.

global _i57_LE_M_W_
_i57_LE_M_W_:	mov	ebx,dword [edi+8]		  ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]		  ; get ea read rout
		call	eax
		mov	al,byte [sr]
		and	al,10h
		rol	dx,1
		jnc	i57lmwnocs
		or	al,1				   ; Clear C and X
i57lmwnocs:	test	dx,dx
		jz	i57lmw1
		jns	i57lmw2
		or	al,8				   ; set N flag
		jmp	i57lmw2
i57lmw1:	or	al,4				   ; set Z flag
i57lmw2:	mov	byte [sr],al		  ; Store flags
		mov	eax,dword [edi+16]
		call	eax	
		mov	eax,dword [edi+20]
		ret


;=============================================================================
; ROXX.X
; Tableusage:(imm) 0-rout 1-disrout 2-shcount 3-dreg 4-cyclecount
; Tableusage:(reg) 0-rout 1-disrout 2-shcountreg 3-dreg 4-cyclecount
; Tableusage:(mem) 0-rout 1-disrout 2-eareg 3-earead 4-eawrite 5-cyclecount
; Immediate rotates do not have the problems with count > 31, or zero
; rotates.
;=============================================================================


		FALIGN32

global _i59_RI_I_B_
_i59_RI_I_B_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59rib3
		stc
i59rib3:	rcr	al,cl
		jc	i59ribcs
		xor	ch,ch
		jmp	i59ribnocs
i59ribcs:	mov	ch,11h
i59ribnocs:	test	al,al
		jz	i59rib1
		jns	i59rib2
		or	ch,8				; set N flag
		jmp	i59rib2
i59rib1:	or	ch,4				; set Z flag
i59rib2:	mov	byte [sr],ch	       ; Store flags
		mov	byte [ebx],al
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i59_RI_I_W_
_i59_RI_I_W_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59riw3
		stc
i59riw3:	rcr	ax,cl
		jc	i59riwcs
		xor	ch,ch
		jmp	i59riwnocs
i59riwcs:	mov	ch,11h
i59riwnocs:	test	ax,ax
		jz	i59riw1
		jns	i59riw2
		or	ch,8				; set N flag
		jmp	i59riw2
i59riw1:	or	ch,4				; set Z flag
i59riw2:	mov	byte [sr],ch	       ; Store flags
		mov	word [ebx],ax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i59_RI_I_L_
_i59_RI_I_L_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59ril3
		stc
i59ril3:	rcr	eax,cl
		jc	i59rilcs
		xor	ch,ch
		jmp	i59rilnocs
i59rilcs:	mov	ch,11h
i59rilnocs:	test	eax,eax
		jz	i59ril1
		jns	i59ril2
		or	ch,8				; set N flag
		jmp	i59ril2
i59ril1:	or	ch,4				; set Z flag
i59ril2:	mov	byte [sr],ch	       ; Store flags
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i59_LE_I_B_
_i59_LE_I_B_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59lib3
		stc
i59lib3:	rcl	al,cl
		jc	i59libcs
		xor	ch,ch
		jmp	i59libnocs
i59libcs:	mov	ch,11h
i59libnocs:	test	al,al
		jz	i59lib1
		jns	i59lib2
		or	ch,8				; set N flag
		jmp	i59lib2
i59lib1:	or	ch,4				; set Z flag
i59lib2:	mov	byte [sr],ch	       ; Store flags
		mov	byte [ebx],al
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i59_LE_I_W_
_i59_LE_I_W_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59liw3
		stc
i59liw3:	rcl	ax,cl
		jc	i59liwcs
		xor	ch,ch
		jmp	i59liwnocs
i59liwcs:	mov	ch,11h
i59liwnocs:	test	ax,ax
		jz	i59liw1
		jns	i59liw2
		or	ch,8				; set N flag
		jmp	i59liw2
i59liw1:	or	ch,4				; set Z flag
i59liw2:	mov	byte [sr],ch	       ; Store flags
		mov	word [ebx],ax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

global _i59_LE_I_L_
_i59_LE_I_L_:	mov	ecx,dword [edi+8]	       ; get shcount
		mov	ebx,dword [edi+12]	       ; get dreg ptr
		mov	eax,dword [ebx]	       ; get dreg content
		mov	ch,byte [sr]
		test	ch,10h
		jz	i59lil3
		stc
i59lil3:	rcl	eax,cl
		jc	i59lilcs
		xor	ch,ch
		jmp	i59lilnocs
i59lilcs:	mov	ch,11h
i59lilnocs:	test	eax,eax
		jz	i59lil1
		jns	i59lil2
		or	ch,8				; set N flag
		jmp	i59lil2
i59lil1:	or	ch,4				; set Z flag
i59lil2:	mov	byte [sr],ch	       ; Store flags
		mov	dword [ebx],eax
		mov	eax,dword [edi+16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_RI_R_B_
_i59_RI_R_B_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59rrbzero
		push	ecx
		mov	ch,byte [sr]
i59rrb7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59rrbok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59rrbext
		stc
i59rrbext:	rcr	al,cl
		jc	i59rrb6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrb7
i59rrb6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrb7
i59rrbok:	test	ch,10h			; The final rotate pass
		jz	i59rrb3
		stc
i59rrb3:	rcr	al,cl
		jc	i59rrbcs
		xor	ch,ch
		jmp	i59rrbnocs
i59rrbcs:	mov	ch,11h
i59rrbnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	al,al
		jz	i59rrb1
		jns	i59rrb2
		or	ch,8			; set N flag
		jmp	i59rrb2
i59rrb1:	or	ch,4			; set Z flag
i59rrb2:	mov	byte [sr],ch	   ; Store flags
		mov	byte [ebx],al
		mov	eax,ebp
		ret

i59rrbzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; al is data
		jz	i59rrbzero1
		or	cl,1			; Set C to X
i59rrbzero1:	test	al,al
		jz	i59rrbzero2
		jns	i59rrbzero3
		or	cl,8			; set N flag
		jmp	i59rrbzero3
i59rrbzero2:	or	cl,4			; set Z flag
i59rrbzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_RI_R_W_
_i59_RI_R_W_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59rrwzero
		push	ecx
		mov	ch,byte [sr]
i59rrw7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59rrwok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59rrwext
		stc
i59rrwext:	rcr	ax,cl
		jc	i59rrw6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrw7
i59rrw6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrw7
i59rrwok:	test	ch,10h			; The final rotate pass
		jz	i59rrw3
		stc
i59rrw3:	rcr	ax,cl
		jc	i59rrwcs
		xor	ch,ch
		jmp	i59rrwnocs
i59rrwcs:	mov	ch,11h
i59rrwnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	ax,ax
		jz	i59rrw1
		jns	i59rrw2
		or	ch,8			; set N flag
		jmp	i59rrw2
i59rrw1:	or	ch,4			; set Z flag
i59rrw2:	mov	byte [sr],ch	   ; Store flags
		mov	word [ebx],ax
		mov	eax,ebp
		ret

i59rrwzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; ax is data
		jz	i59rrwzero1
		or	cl,1			; Set C to X
i59rrwzero1:	test	ax,ax
		jz	i59rrwzero2
		jns	i59rrwzero3
		or	cl,8			; set N flag
		jmp	i59rrwzero3
i59rrwzero2:	or	cl,4			; set Z flag
i59rrwzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_RI_R_L_
_i59_RI_R_L_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59rrlzero
		push	ecx
		mov	ch,byte [sr]
i59rrl7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59rrlok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59rrlext
		stc
i59rrlext:	rcr	eax,cl
		jc	i59rrl6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrl7
i59rrl6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59rrl7
i59rrlok:	test	ch,10h			; The final rotate pass
		jz	i59rrl3
		stc
i59rrl3:	rcr	eax,cl
		jc	i59rrlcs
		xor	ch,ch
		jmp	i59rrlnocs
i59rrlcs:	mov	ch,11h
i59rrlnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	eax,eax
		jz	i59rrl1
		jns	i59rrl2
		or	ch,8			; set N flag
		jmp	i59rrl2
i59rrl1:	or	ch,4			; set Z flag
i59rrl2:	mov	byte [sr],ch	   ; Store flags
		mov	dword [ebx],eax
		mov	eax,ebp
		ret

i59rrlzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; eax is data
		jz	i59rrlzero1
		or	cl,1			; Set C to X
i59rrlzero1:	test	eax,eax
		jz	i59rrlzero2
		jns	i59rrlzero3
		or	cl,8			; set N flag
		jmp	i59rrlzero3
i59rrlzero2:	or	cl,4			; set Z flag
i59rrlzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_LE_R_B_
_i59_LE_R_B_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59lrbzero
		push	ecx
		mov	ch,byte [sr]
i59lrb7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59lrbok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59lrbext
		stc
i59lrbext:	rcl	al,cl
		jc	i59lrb6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrb7
i59lrb6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrb7
i59lrbok:	test	ch,10h			; The final rotate pass
		jz	i59lrb3
		stc
i59lrb3:	rcl	al,cl
		jc	i59lrbcs
		xor	ch,ch
		jmp	i59lrbnocs
i59lrbcs:	mov	ch,11h
i59lrbnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	al,al
		jz	i59lrb1
		jns	i59lrb2
		or	ch,8			; set N flag
		jmp	i59lrb2
i59lrb1:	or	ch,4			; set Z flag
i59lrb2:	mov	byte [sr],ch	   ; Store flags
		mov	byte [ebx],al
		mov	eax,ebp
		ret

i59lrbzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; al is data
		jz	i59lrbzero1
		or	cl,1			; Set C to X
i59lrbzero1:	test	al,al
		jz	i59lrbzero2
		jns	i59lrbzero3
		or	cl,8			; set N flag
		jmp	i59lrbzero3
i59lrbzero2:	or	cl,4			; set Z flag
i59lrbzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_LE_R_W_
_i59_LE_R_W_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59lrwzero
		push	ecx
		mov	ch,byte [sr]
i59lrw7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59lrwok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59lrwext
		stc
i59lrwext:	rcl	ax,cl
		jc	i59lrw6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrw7
i59lrw6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrw7
i59lrwok:	test	ch,10h			; The final rotate pass
		jz	i59lrw3
		stc
i59lrw3:	rcl	ax,cl
		jc	i59lrwcs
		xor	ch,ch
		jmp	i59lrwnocs
i59lrwcs:	mov	ch,11h
i59lrwnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	ax,ax
		jz	i59lrw1
		jns	i59lrw2
		or	ch,8			; set N flag
		jmp	i59lrw2
i59lrw1:	or	ch,4			; set Z flag
i59lrw2:	mov	byte [sr],ch	   ; Store flags
		mov	word [ebx],ax
		mov	eax,ebp
		ret

i59lrwzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; ax is data
		jz	i59lrwzero1
		or	cl,1			; Set C to X
i59lrwzero1:	test	ax,ax
		jz	i59lrwzero2
		jns	i59lrwzero3
		or	cl,8			; set N flag
		jmp	i59lrwzero3
i59lrwzero2:	or	cl,4			; set Z flag
i59lrwzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend Dx to the left with shiftcount in Dy-register

global _i59_LE_R_L_
_i59_LE_R_L_:	mov	ecx,dword [edi+8]   ; get shcount reg ptr
		mov	ebx,dword [edi+12]  ; get dreg ptr
		mov	ecx,dword [ecx]	    ; get shcount reg content
		mov	eax,dword [ebx]	    ; get dreg content
		and	ecx,3fh			; modulo 64
		jz	near i59lrlzero
		push	ecx
		mov	ch,byte [sr]
i59lrl7:	cmp	cl,20h			; Run several passes of 31		    
		jb	near i59lrlok		; until count < 32
		mov	dl,cl			
		mov	cl,31			; rotate 31 
		sub	dl,cl
		test	ch,10h
		jz	i59lrlext
		stc
i59lrlext:	rcl	eax,cl
		jc	i59lrl6			
		xor	ch,ch			; Clear XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrl7
i59lrl6:	mov	ch,11h			; Set XC
		mov	cl,dl			; Update rotate count
		jmp	i59lrl7
i59lrlok:	test	ch,10h			; The final rotate pass
		jz	i59lrl3
		stc
i59lrl3:	rcl	eax,cl
		jc	i59lrlcs
		xor	ch,ch
		jmp	i59lrlnocs
i59lrlcs:	mov	ch,11h
i59lrlnocs:	pop	ebp
		add	ebp,ebp
		add	ebp,dword [edi+16]
		test	eax,eax
		jz	i59lrl1
		jns	i59lrl2
		or	ch,8			; set N flag
		jmp	i59lrl2
i59lrl1:	or	ch,4			; set Z flag
i59lrl2:	mov	byte [sr],ch	   ; Store flags
		mov	dword [ebx],eax
		mov	eax,ebp
		ret

i59lrlzero:	mov	cl,byte [sr]	   ; rotate zero positions
		and	cl,010h			; eax is data
		jz	i59lrlzero1
		or	cl,1			; Set C to X
i59lrlzero1:	test	eax,eax
		jz	i59lrlzero2
		jns	i59lrlzero3
		or	cl,8			; set N flag
		jmp	i59lrlzero3
i59lrlzero2:	or	cl,4			; set Z flag
i59lrlzero3:	mov	byte [sr],cl	   ; Store flags
		mov	eax,dword [edi + 16]
		ret


		FALIGN32

; Rotate extend memory word 1 to the right

global _i59_RI_M_W_
_i59_RI_M_W_:	mov	ebx,dword [edi+8]	       ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]	       ; get ea read rout
		call	eax
		mov	al,byte [sr]
		test	al,10h				; test clears C
		jz	i59rmw3
		stc
i59rmw3:	rcr	dx,1
		jc	i59rmwcs
		xor	al,al
		jmp	i59rmwnocs
i59rmwcs:	mov	al,11h
i59rmwnocs:	test	dx,dx
		jz	i59rmw1
		jns	i59rmw2
		or	al,8				; set N flag
		jmp	i59rmw2
i59rmw1:	or	al,4				; set Z flag
i59rmw2:	mov	byte [sr],al	       ; Store flags
		mov	eax,dword [edi+16]
		call	eax	
		mov	eax,dword [edi+20]
		ret


		FALIGN32

; Rotate extend memory word 1 to the left.

global _i59_LE_M_W_
_i59_LE_M_W_:	mov	ebx,dword [edi+8]		  ; get ea reg ptr
		mov	edx,ebx
		mov	eax,dword [edi+12]		  ; get ea read rout
		call	eax
		mov	al,byte [sr]
		test	al,10h				   ; test clears C
		jz	i59lmw3
		stc
i59lmw3:	rcl	dx,1
		jc	i59lmwcs
		xor	al,al				   ; Clear C and X
		jmp	i59lmwnocs
i59lmwcs:	mov	al,11h				   ; Set C and X
i59lmwnocs:	test	dx,dx
		jz	i59lmw1
		jns	i59lmw2
		or	al,8				   ; set N flag
		jmp	i59lmw2
i59lmw1:	or	al,4				   ; set Z flag
i59lmw2:	mov	byte [sr],al		  ; Store flags
		mov	eax,dword [edi+16]
		call	eax	
		mov	eax,dword [edi+20]
		ret


;=============================================================================
; Instruction 61 RTE
; Opcode*32+t i edi
; Flag: From stack
;
; step 1:	get SR from frame
; step 2:	get PC from frame
; step 3:	010++, get frame nr.
; step 4:	update stack ptr according to frame nr.
; step 5:	020++, not 060, if throwaway frame, commit SR, redo RTE
; step 6:	commit SR
;
;=============================================================================


		FALIGN32

global _i61_
_i61_:		test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
i61_redo:	mov	ecx, dword [da_regs + 60]	; Get SR, step 1
		fetchw
		mov	ebp, edx
		mov	ecx, dword [da_regs + 60]
		add	ecx, 2
		mov	dword [da_regs + 60], ecx	; Get PC, step 2
		fetchl
		mov	ecx, dword [da_regs + 60]
		add	ecx, 4
		mov	dword [da_regs + 60], ecx
		mov	esi, edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif

		cmp	dword [cpu_type], 0	; step 3 
		je	near i61_000
		mov	ecx, dword [da_regs + 60]		; Get Frame type
		fetchw
		add	ecx, 2
		shr	edx, 12
		and	edx, 0fh
		add	ecx, dword [rte_stackinc + 4*edx]; step 4
		mov	dword [da_regs + 60], ecx
		cmp	edx, 1				; step 5
		jne	near i61_000
		cmp	dword [cpu_type], 2
		jb	near i61_000
		cmp	dword [cpu_type], 6
		jae	near i61_000
		mov	edx, ebp
		call	_cpuUpdateSR_
		jmp	i61_redo
	
i61_000:	mov	edx, ebp			; step 6 
		call	_cpuUpdateSR_
		mov	eax, 20
		ret

;=============================================================================
; Instruction 62 RTR
; Opcode*32+t i edi
; Flag: From stack
;=============================================================================


		FALIGN32

global _i62_
_i62_:		mov	ecx,dword [da_regs + 60]
		fetchw
		and	dl,01fh
		mov	byte [sr],dl
		add	ecx,2
		mov	dword [da_regs + 60],ecx
		fetchl
		mov	ecx,dword [da_regs + 60]
		mov	esi,edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		add	ecx,4
		mov	dword [da_regs + 60],ecx
		mov	eax,20
		ret


;=============================================================================
; Instruction 63 RTS
; Opcode*32+t i edi
; Flag: NONE
;=============================================================================


		FALIGN32

global _i63_
_i63_:		mov	ecx,dword [da_regs + 60]
		fetchl
		mov	esi,edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		add	ecx,4
		mov	dword [da_regs + 60],ecx
		mov	eax,16
		ret


;=============================================================================
; Instruction 64   SBCD.B Dy,Dx	   SBCD.B -(Ay),-(Ax)
; Tableusage  2 - rx*4(dest)  3 - ry*4
; Opcode*32 i edi
;=============================================================================


		FALIGN32

global _i64_1_
_i64_1_:	mov	ebx,dword [8+edi]
		xor	eax,eax		      
		mov	ecx,dword [4+edi] 
		mov	dl,byte [sr]	   
		mov	al,byte [ebx]	  
		xor	ebx,ebx
		mov	bl,byte [bcd2hex+eax]
		mov	al,byte [ecx]	 
		test	dl,10h			  
		jz	i64_1a			  
		inc	ebx			  
i64_1a:		mov	al,byte [bcd2hex+eax]
		sub	eax,ebx			  
		and	dl,0eh			  
		test	eax,eax			  
		jns	i64_1b			  
		or	dl,11h			  
		add	eax,100			  
i64_1b:		test	al,al			  
		jz	i64_1c			  
		and	dl,01bh			  
i64_1c:		mov	byte [sr],dl	   
		and	eax,0ffh
		mov	al,byte [hex2bcd+eax]
		mov	byte [ecx],al	 
		mov	eax,6 
		ret	      


		FALIGN32

global _i64_2_
_i64_2_:	mov	ebx,dword [8+edi]
		mov	ecx,dword [ebx]
		dec	ecx
		cmp	ebx, dword da_regs + 60
		jne	i64_2_j
		dec	ecx
i64_2_j:	mov	dword [ebx],ecx
		xor	edx,edx
		fetchb
		xor	ebx,ebx
		mov	bl,byte [bcd2hex+edx]
		mov	eax,dword [4+edi]
		mov	ecx,dword [eax]
		dec	ecx
		cmp	eax, dword da_regs + 60
		jne	i64_2_i
		dec	ecx
i64_2_i:	mov	dword [eax],ecx
		xor	edx,edx
		fetchb
		xor	eax,eax
		mov	al,byte [bcd2hex+edx]
		mov	dl,byte [sr]	
		test	dl,10h
		jz	i64_2a
		inc	ebx
i64_2a:		sub	eax,ebx
		and	dl,0eh
		test	eax,eax	       
		jns	i64_2b
		or	dl,11h	       
		add	eax,100
i64_2b:		test	eax,eax	       
		jz	i64_2c
		and	dl,01bh	       
i64_2c:		mov	byte [sr],dl
		and	eax,0ffh
		mov	dl,byte [hex2bcd+eax]
		writeb
		mov	eax,18
		ret


;=============================================================================
; Scc.B <ea>
;
; Tableusage
;    0 - Emulation routine
;    1 - register*4 + d
;    2 - ea write routine
;    3 - condition handler
;    4 - cycle count/false
;    5 - cycle count/true
;
; Flags: NONE
;=============================================================================


		FALIGN32

global _i65_
_i65_:		mov	eax, dword [edi + 12]
		call	eax
		mov	eax, dword [edi + 8]
		mov	ebx, dword [edi + 4]
		test	dl, dl
		jz	.i65false
		mov	dl, -1
		call	eax
		mov	eax, dword [edi + 20]
		ret
.i65false:	call	eax
		mov	eax, dword [edi + 16]
		ret



;=============================================================================
; STOP #xxx.W
; Flag:	 Set to immediate operand
;=============================================================================

		FALIGN32

global _i66_
_i66_:		test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	dword [cpu_stop], 1
		mov	dword [cpu_next], -1
		mov	ebx, edx
		call	_cpuUpdateSR_
		test	dword [cpu_stop], 1	   ; Awful hack
		jz	.i66ah
		mov	esp, dword [exceptionstack]
		jmp	_return_from_stop_  ; Stop is sort of a hack...
					    ; This stub will clean up,
					    ; and enter the bus-loop afterwrds.
.i66ah:		mov	eax, 16
		ret


;=============================================================================
; SUB.X <ea>,Dn / SUB.X Dn,<ea>
; Flag: ALL, X=C
;=============================================================================


		FALIGN32

global _i67_B_1_
_i67_B_1_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+16]
		mov	dh,byte [ebx]	  
		sub	dh,dl		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	byte [ebx],dh	  
		mov	eax,dword [statustab+4*eax]
		mov	byte [sr],al	  
		mov	eax,dword [edi+20]	
		ret				 


		FALIGN32

global _i67_W_1_
_i67_W_1_:	mov	eax,dword [edi+12] 
		mov	edx,dword [edi+8]  
		call	eax		       
		mov	ebx,dword [edi+16] 
		mov	cx,word [ebx]	   
		sub	cx,dx		       
		pushfd			       
		pop	eax		       
		and	eax,0fffh	       
		mov	word [ebx],cx	   
		mov	eax,dword [statustab+4*eax]
		mov	byte [sr],al	   
		mov	eax,dword [edi+20]
		ret

		FALIGN32

global _i67_L_1_
_i67_L_1_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]	  
		sub	ecx,edx		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	dword [ebx],ecx	  
		mov	eax,dword [statustab+4*eax]
		mov	byte [sr],al	   
		mov	eax,dword [edi+20]
		ret			      


		FALIGN32

global _i67_B_2_
_i67_B_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+16]
		mov	dh,byte [ebx]	  
		sub	dl,dh		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+24]	
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx			 
		mov	eax,dword [edi+20]	
		ret				 


		FALIGN32

global _i67_W_2_
_i67_W_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+16]
		mov	cx,word [ebx]	  
		sub	dx,cx		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+24]	
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx			 
		mov	eax,dword [edi+20]	
		ret				 
					 

		FALIGN32

global _i67_L_2_
_i67_L_2_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8]
		call	eax
		mov	ebx,dword [edi+16]
		mov	ecx,dword [ebx]
		sub	edx,ecx
		pushfd 
		pop	eax 
		and	eax,0fffh
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+24]
		mov	byte [sr],al	   
		mov	ebx,dword [edi+8]
		call	ecx		       
		mov	eax,dword [edi+20] 
		ret			       


;=============================================================================
; SUBA.W/L <ea>,An
; No flags.
;=============================================================================


		FALIGN32

global _i68_W_
_i68_W_:	mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		mov	ebx, dword [edi + 12]
		mov	ecx, dword [ebx]
		movsx	edx, dx
		sub	ecx, edx
		mov	dword [ebx], ecx
		mov	eax, dword [edi + 16]
		ret


		FALIGN32

global _i68_L_
_i68_L_:	mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		mov	ebx, dword [edi + 12]
		mov	ecx, dword [ebx]
		sub	ecx, edx
		mov	dword [ebx], ecx
		mov	eax, dword [edi + 16]
		ret


;=============================================================================
; SUBI.X #,<ea>
;  0-routineptr 1-disasm routine  2-eareg 3-ea read routine
;		 4-cycle count	  5-ea write routine		   
; Flag: ALL, X=C
;=============================================================================


		FALIGN32

global _i69_B_
_i69_B_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]	
		mov	edx,dword [edi+8]	
		call	eax			 
		sub	dl,bl			 
		pushfd				 
		pop	eax			 
		and	eax,0fffh		 
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+20]  
		mov	byte [sr],al	 
		mov	ebx,dword [edi+8]
		call	ecx			
		mov	eax,dword [edi+16]  
		ret				


		FALIGN32

global _i69_W_
_i69_W_:	mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	ebx,edx
		mov	eax,dword [edi+12]  
		mov	edx,dword [edi+8]   
		call	eax			
		sub	dx,bx			
		pushfd				
		pop	eax			
		and	eax,0fffh		
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+20]
		mov	byte [sr],al	   
		mov	ebx,dword [edi+8]
		call	ecx		      
		mov	eax,dword [edi+16]
		ret			      


		FALIGN32

global _i69_L_
_i69_L_:	mov	ecx,esi
		fetchlopcode_inc_pos
		add	esi,4
		mov	ebx,edx
		mov	eax,dword [edi+12]	
		mov	edx,dword [edi+8]	
		call	eax			 
		sub	edx,ebx			 
		pushfd				 
		pop	eax			 
		and	eax,0fffh		 
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+20]
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx		      
		mov	eax,dword [edi+16]
		ret


;=============================================================================
; SUBQ.X #,<ea>
; Flag: ALL, X=C
;=============================================================================


		FALIGN32

global _i70_B_
_i70_B_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+24]
		sub	dl,bl		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	ecx,dword [edi+20]
		mov	eax,dword [statustab+4*eax]
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx		      
		mov	eax,dword [edi+16]
		ret			      


		FALIGN32

global _i70_W_
_i70_W_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+24]
		sub	dx,bx		       
		pushfd			       
		pop	eax		       
		and	eax,0fffh	       
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+20]
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx		      
		mov	eax,dword [edi+16]
		ret			      


		FALIGN32

global _i70_L_
_i70_L_:	mov	eax,dword [edi+12]
		mov	edx,dword [edi+8] 
		call	eax		      
		mov	ebx,dword [edi+24]
		sub	edx,ebx		      
		pushfd			      
		pop	eax		      
		and	eax,0fffh	      
		mov	eax,dword [statustab+4*eax]
		mov	ecx,dword [edi+20]
		mov	byte [sr],al	  
		mov	ebx,dword [edi+8]
		call	ecx		      
		mov	eax,dword [edi+16]
		ret			      


		FALIGN32

global _i70_ADR_
_i70_ADR_:	mov	eax,dword [edi+8]
		mov	edx,dword [eax]
		mov	ebx,dword [edi+24]
		sub	edx,ebx		   
		mov	dword [eax],edx
		mov	eax,dword [edi+16]
		ret			      


;=============================================================================
; Instruction  SUBX.B/W/L Dy, Dx    SUBX.B/W/L -(Ay), -(Ax)
;
; Tableusage:
;   0 - Emulation routine
;   1 - Rx pointer (Destination)
;   2 - Ry pointer (Source)
;
; Flags: ALL, Z unchanged if result non-zero
;=============================================================================


		FALIGN32

global _cpuSubXB1_
_cpuSubXB1_:	mov	ebx, dword [8 + edi]	      
		mov	ecx, dword [4 + edi]	      
		mov	al, byte [ebx]	  
		mov	bl, byte [ecx]	  
		mov	edx, dword [sr]
		test	dl, 10h
		jz	.SubXB1NoC
		stc	
.SubXB1NoC:	sbb	bl, al
		pushfd			   
		pop	eax
		and	eax, 0fbfh		   
		mov	byte [ecx], bl	  
		mov	eax, dword [statustab + 4*eax]
		and	edx, 0ffffff04h			; Preserve Z
		test	bl, bl				; Clear if nonzero
		jz	.SubXB1Zero
		and	edx, 0ffffff00h
.SubXB1Zero:	or	edx, eax
		mov	dword [sr], edx
		mov	eax, 4
		ret


		FALIGN32

global _cpuSubXW1_
_cpuSubXW1_:	mov	ebx, dword [8 + edi]	      
		mov	ecx, dword [4 + edi]	      
		mov	eax, dword [ebx]	       
		mov	ebx, dword [ecx]	       
		mov	edx, dword [sr]
		test	dl, 10h
		jz	.SubXW1NoC
		stc
.SubXW1NoC:	sbb	bx, ax
		pushfd   
		pop	eax    
		and	eax, 0fbfh
		mov	word [ecx], bx
		mov	eax, dword [statustab + 4*eax]
		and	edx, 0ffffff04h			; Preserve Z
		test	bx, bx				; Clear if nonzero
		jz	.SubXW1Zero
		and	edx, 0ffffff00h
.SubXW1Zero:	or	edx, eax
		mov	dword [sr], edx
		mov	eax, 4
		ret


		FALIGN32

global _cpuSubXL1_
_cpuSubXL1_:	mov	ebx, dword [8 + edi]
		mov	ecx, dword [4 + edi]
		mov	eax, dword [ebx]
		mov	ebx, dword [ecx]
		mov	edx, dword [sr]
		test	dl, 10h
		jz	.SubXL1NoC
		stc	
.SubXL1NoC:	sbb	ebx, eax
		pushfd			       
		pop	eax		       
		and	eax, 0fbfh	       
		mov	dword [ecx], ebx	   
		mov	eax, dword [statustab + 4*eax]
		and	edx, 0ffffff04h			; Preserve Z
		test	ebx, ebx			; Clear if nonzero
		jz	.SubXL1Zero
		and	edx, 0ffffff00h
.SubXL1Zero:	or	edx, eax
		mov	dword [sr], edx
		mov	eax, 8
		ret


		FALIGN32

global _cpuSubXB2_
_cpuSubXB2_:	mov	ebx, dword [8 + edi]	     ; Source
		mov	ecx, dword [ebx]
		dec	ecx
		cmp	ebx, dword da_regs + 60
		jne	i71_B_2j
		dec	ecx
i71_B_2j:	mov	dword [ebx], ecx
		fetchb
		mov	eax, dword [4 + edi]	     ; Dest
		mov	ecx, dword [eax]
		dec	ecx
		cmp	eax, dword da_regs + 60
		jne	i71_B_2i
		dec	ecx
i71_B_2i:	push	ecx
		mov	dword [eax], ecx
		fetchb
		mov	ebx, edx
		mov	eax, dword [sr]
		test	al, 10h
		jz	i71b2nc
		stc	
i71b2nc:	sbb	dl, bl
		pushfd				   
		pop	ecx			   
		and	ecx, 0fbfh		   
		mov	ecx, dword [statustab + 4*ecx]
		and	al, 4				; Preserve Z
		test	dl, dl				; Clear if nonzero
		jz	i71b2nz
		xor	al, al
i71b2nz:	or	al, cl
		mov	byte [sr], al	      
		pop	ecx
		writeb
		mov	eax, 18
		ret


		FALIGN32

global _cpuSubXW2_
_cpuSubXW2_:	mov	ebx, dword [8 + edi]	     ; Source
		mov	ecx, dword [ebx]
		sub	ecx, 2
		mov	dword [ebx], ecx
		fetchw
		mov	ebx, edx
		mov	eax, dword [4 + edi]	     ; Dest
		mov	ecx, dword [eax]
		sub	ecx, 2
		mov	dword [eax], ecx
		push	ecx
		fetchw
		mov	eax, dword [sr]
		test	al, 10h
		jz	i71w2nc
		stc	
i71w2nc:	sbb	dx, bx
		pushfd		   
		pop	ecx	   
		and	ecx, 0fbfh  
		mov	ecx, dword [statustab + 4*ecx]
		and	al, 4				; Preserve Z
		test	dx, dx				; Clear if nonzero
		jz	i71w2nz
		xor	al, al
i71w2nz:	or	al, cl
		mov	byte [sr], al	   
		pop	ecx
		writew
		mov	eax, 18
		ret


		FALIGN32

global _cpuSubXL2_
_cpuSubXL2_:	mov	ebx, dword [8 + edi]		; Source 
		mov	ecx, dword [ebx]
		sub	ecx, 4
		mov	dword [ebx], ecx
		fetchl
		mov	ebx, edx
		mov	eax, dword [4 + edi]		; Dest
		mov	ecx, dword [eax]
		sub	ecx, 4
		mov	dword [eax], ecx
		push	ecx
		fetchl
		mov	eax, dword [sr]
		test	al, 10h
		jz	i71l2nc
		stc	
i71l2nc:	sbb	edx, ebx
		pushfd				
		pop	ecx			
		and	ecx, 0fbfh		 
		mov	ecx, dword [statustab + 4*ecx]
		and	al, 4				; Preserve Z
		test	edx, edx			; Clear if nonzero
		jz	i71l2nz
		xor	al, al
i71l2nz:	or	al, cl
		mov	byte [sr], al
		pop	ecx
		writel
		mov	eax, 30
		ret


;=============================================================================
; Instruction SWAP.W Dn
;
; Tableusage:
;   0 - Emulation routine
;   1 - Register pointer
; 
; Flags: NZ, V=C=0, X unaffected
;=============================================================================


		FALIGN32

global _cpuSwapW_
_cpuSwapW_:	mov	edx, dword [edi + 4]
		mov	ebp, dword [sr]
		mov	eax, 4
		and	ebp, 0ffffff10h
		mov	ebx, dword [edx]
		ror	ebx, 16
		test	ebx, ebx
		pushfd
		pop	ecx
		mov	dword [edx], ebx
		and	ecx, 0c0h
		shr	ecx, 4
		or	ebp, ecx
		mov	dword [sr], ebp
		ret


;=============================================================================
; Instruction TAS.B <ea>
;
; Tableusage:
;   0 - Emulation routine
;   1 - EA register pointer
;   2 - EA read routine
;   3 - EA write routine
;   4 - Cycle count
;
; Flags: NZ, V=C=0, X unaffected
;=============================================================================


		FALIGN32

global _cpuTasB_
_cpuTasB_:	mov	edx, dword [edi + 4]
		mov	ecx, dword [edi + 8]
		call	ecx
		test	dl, dl
		pushfd
		pop	ecx
		mov	eax, dword [sr]
		and	ecx, 0c0h
		and	eax, 0ffffff10h
		shr	ecx, 4
		or	dl, 80h
		or	eax, ecx
		mov	dword [sr], eax
		mov	ecx, dword [edi + 12]
		mov	ebx, dword [edi + 4]
		call	ecx
		mov	eax, dword [edi + 16]
		ret


;=============================================================================
; Instruction TRAP #n
;
; Jumps to routine specified at VBR + $80 + n*4
;
; Tableusage:
;   0 - Emulation routine
;   1 - n*4 + $80
;
; Flags: None
;=============================================================================


		FALIGN32

global _cpuTrap_
_cpuTrap_:	mov	ebx, dword [edi + 4]
		mov	eax, 38
		jmp	_cpuPrepareException_


;=============================================================================
; Instruction TRAPV
;
; Jumps to routine specified at VBR + $1c when V is set
;
; Tableusage:
;   0 - Emulation routine
;
; Flags: None
;=============================================================================


		FALIGN32

global _cpuTrapV_
_cpuTrapV_:	test	dword [sr], 2
		jz	near .noTrapV
		mov	ebx, 01ch
		mov	eax, 34
		jmp	_cpuPrepareException_
.noTrapV:	mov	eax, 4
		ret


;=============================================================================
; Instruction TST.B/W/L <ea>
;
; Tableusage:
;   0 - Emulation routine
;   1 - EA register pointer
;   2 - EA read routine
;   3 - Cycle count
;
; Flags: NZ, V=C=0, X unaffected
;=============================================================================


		FALIGN32

global _cpuTstB_
_cpuTstB_:	mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		test	dl, dl
		pushfd
		pop	eax
		mov	ecx, dword [sr]
		and	eax, 0c0h
		and	ecx, 0ffffff10h
		shr	eax, 4
		or	ecx, eax
		mov	dword [sr], ecx
		mov	eax, dword [edi + 12]
		ret

		
		FALIGN32

global _cpuTstW_
_cpuTstW_:	mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		test	dx, dx
		pushfd
		pop	eax
		mov	ecx, dword [sr]
		and	eax, 0c0h
		and	ecx, 0ffffff10h
		shr	eax, 4
		or	ecx, eax
		mov	dword [sr], ecx
		mov	eax, dword [edi + 12]
		ret


		FALIGN32

global _cpuTstL_
_cpuTstL_:	mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		test	edx, edx
		pushfd
		pop	eax
		mov	ecx, dword [sr]
		and	eax, 0c0h
		and	ecx, 0ffffff10h
		shr	eax, 4
		or	ecx, eax
		mov	dword [sr], ecx
		mov	eax, dword [edi + 12]
		ret


;==============================================================================
; Instruction UNLK An
;
; Tableusage:
;   0 - Emulation routine
;   1 - Address register pointer
;
; Flags: None
;==============================================================================


		FALIGN32

global _cpuUnlk_
_cpuUnlk_:	mov	ebx, dword [edi + 4]
		mov	ecx, dword [ebx]
		mov	edx, ecx
		add	edx, 4
		mov	dword [da_regs + 60], edx
		fetchl
		mov	dword [ebx], edx
		mov	eax, 12
		ret


;==============================================================================
;		      68030 specific address mode routines
;==============================================================================

;==============================================================================
; Adressmode routines for reading byte,word,longword
; With PC increase
; Parameters:  In:  edx - pointer to register	esi - current PC position
;	      Out:  edx - result		esi - current PC position
;==============================================================================

; disp(Ax,Ri) read (68030 compliant)
; ----------------------------------


		FALIGN32

global _arl063_
_arl063_:	call	_eac063_
		mov	ecx, edx
		fetchlret


		FALIGN32

global _arw063_
_arw063_:	call	_eac063_
		mov	ecx, edx
		fetchwret


		FALIGN32

global _arb063_
_arb063_:	call	_eac063_
		mov	ecx, edx
		fetchbret


		FALIGN32

global _arl0733_
_arl0733_:	call	_eac0733_
		mov	ecx, edx
		fetchlret


		FALIGN32

global _arw0733_
_arw0733_:	call	_eac0733_
		mov	ecx, edx
		fetchwret


		FALIGN32

global _arb0733_
_arb0733_:	call	_eac0733_
		mov	ecx, edx
		fetchbret


;==============================================================================
; Adressmode routines for reading byte,word,longword
; (NOTE!) _Without_ PC increase, without An inc/dec, does not change prefetch
; Parameters:  In:  edx - pointer to register	esi - current PC position
;	      Out:  edx - result		esi - current PC position
;==============================================================================

; disp(Ax,Ri) read (68030 compliant)
; ----------------------------------

		FALIGN32

global _parl063_
_parl063_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac063_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchlret


		FALIGN32

global _parw063_
_parw063_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac063_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchwret


		FALIGN32

global _parb063_
_parb063_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac063_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchbret


		FALIGN32

global _parl0733_
_parl0733_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac0733_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchlret


		FALIGN32

global _parw0733_
_parw0733_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac0733_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchwret


		FALIGN32

global _parb0733_
_parb0733_:	push	esi
%ifdef PREFETCH
		push	dword [prefetch_word]
%endif
		call	_eac0733_
%ifdef PREFETCH
		pop	dword [prefetch_word]
%endif
		pop	esi
		mov	ecx, edx
		fetchbret


;==============================================================================
; Adressmode routines for writing byte,word,longword
; With PC increase
; Parameters:  In:  ebx - pointer to register	esi - current PC position
;		    edx - data
;==============================================================================

; disp(Ax,Ri) write (68030 compliant)
; -----------------------------------



		FALIGN32

global _awl063_
_awl063_:	push	edx
		mov	edx, ebx
		call	_eac063_
		mov	ecx, edx
		pop	edx
		writelret


		FALIGN32

global _aww063_
_aww063_:	push	edx
		mov	edx, ebx
		call	_eac063_
		mov	ecx, edx
		pop	edx
		writewret


		FALIGN32

global _awb063_
_awb063_:	push	edx
		mov	edx, ebx
		call	_eac063_
		mov	ecx, edx
		pop	edx
		writebret


;==============================================================================
; Adressmode routines for getting effective address
; With PC increase
; Parameters:  In:  edx - pointer to register	esi - current PC position
;	      Out:  edx - result		esi - current PC position
;==============================================================================

; disp(Ax,Ri) get <ea> (68030 compliant)
; --------------------------------------

		FALIGN32

global _eac063_
_eac063_:	push	ebx
		mov	ebx, dword [edx]	; Get An value
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		test	dh, 1			; extended params (68030)
		jnz	eac063ext
		call	eac063normal
		mov	edx, ecx
		pop	ebx
		ret

eac063ext:	call	eac063extended
		mov	edx, ecx
		pop	ebx
		ret


; disp(pc,Ri) get <ea> (68030 compliant)
; --------------------------------------

		FALIGN32

global _eac0733_
_eac0733_:	push	ebx
		mov	ebx, esi	; Get PC value
		PTR_TO_PC	ebx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		test	dh, 1			; extended params (68030)
		jnz	eac0733ext
		call	eac063normal
		mov	edx, ecx
		pop	ebx
		ret

eac0733ext:	call	eac063extended
		mov	edx, ecx
		pop	ebx
		ret


;======================================================
; Brief format (68030) ea calculation
;
; In:	esi - current PC (points after extension word)
;	ebx - Ax or PC value
;	edx - Extension word
; Out:	ecx - Effective Address
;=============================================


		FALIGN32

eac063normal:	movsx	ecx, dl
		add	ecx, ebx
		mov	ebx, edx
		shr	ebx, 10
		and	ebx, 03ch
		mov	ebx, dword [da_regs + ebx]
		test	dh, 8
		jnz	eac063nosext
		movsx	ebx, bx
eac063nosext:	xchg	edx, ecx
		shr	ecx, 9
		and	ecx, 3
		shl	ebx, cl			; Ri index multiply (68030)
		xchg	edx, ecx
		add	ecx, ebx
		ret


;======================================================
; Extended format (68030) ea calculation
;
; In:	esi - current PC (points after extension word)
;	ebx - Ax or PC value (base register value)
;	edx - Extension word
; Out:	ecx - Effective Address
;======================================================


		FALIGN32

eac063extended:
		; Calculate base register + base displacement

		push	eax
		test	dl, 80h		; Base register included?
		jz	eac063a
		xor	ebx, ebx	; Base register not included!
eac063a:	mov	eax, edx	; Find out about base displacement
		and	eax, 030h
		jz	near eac063illegal	; 0 - Illegal
		cmp	eax, 010h	; 1 - Null base displacement
		je	near eac063c
		cmp	eax, 020h	; 2 - Word base displacement
		jne	eac063b
		mov	ecx, esi
		push	edx
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	edx, dx
		add	ebx, edx
		pop	edx
		jmp	eac063c
eac063b:	mov	ecx, esi	; 3 - Long base displacement
		push	edx
		fetchlopcode_inc_pos
		add	esi, 4
		add	ebx, edx
		pop	edx
		
		; Here ebx = (An) + bd (base register + base displacement)
		; Calculate index next

eac063c:	xor	eax, eax
		test	edx, 040h	; Find out about index
		jnz	eac063e
		mov	al, dh		; Index is included
		shr	al, 2
		and	eax, 3ch
		mov	eax, dword [da_regs + eax] ; Get index reg value
		test	dh, 8		; Index size check
		jnz	eac063d
		movsx	eax, ax
eac063d:	mov	cl, dh
		shr	cl, 1
		and	cl, 3
		shl	eax, cl		; Multiply index value

		; Here still ebx = (An) + bd (base register + base displacement)
		; Here eax = (Ri)*scale (index)
		; Calulate based on I/IS mode next

eac063e:	mov	ebp, edx
		and	ebp, 07h	; Find I/IS mode
		cmp	ebp, 3
		jbe	eac063f
		test	edx, 040h	; Test for reserved I/IS modes
		jnz	eac063illegal
eac063f:	call	dword [eac063istab + 4*ebp] ; Decode I/IS field
		pop	eax
		ret

eac063illegal:	mov	ebx, 010h	; Reserved, ie. INVALID OPCODE
		mov	eax, 38
		jmp	_cpuPrepareException_


;========================
; IS decode routines
; In:	eax - (Ri)*scale
;	ebx - (An) + bd
;	esi - Current PC
; Out:	ecx - EA
;========================


		FALIGN32

global _eac063is0_
_eac063is0_:	mov	ecx, ebx	; No memory indirect action
		add	ecx, eax	; ie. (An, Ri*scale, bd) 
		ret


		FALIGN32

global _eac063is1_
_eac063is1_:	mov	ecx, ebx	; Preindexed with null outer disp 
		add	ecx, eax	; ie. ([An, Ri*scale, bd])
		fetchl
		mov	ecx, edx
		ret


		FALIGN32

global _eac063is2_
_eac063is2_:	mov	ecx, ebx	; Preindexed with word outer disp
		add	ecx, eax	; ie. ([An, Ri*scale, bd], od)
		fetchl			; Fetch intermediary address
		mov	ebx, edx
		mov	ecx, esi	; Fetch outer displacement
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		add	ecx, ebx
		ret


		FALIGN32

global _eac063is3_
_eac063is3_:	mov	ecx, ebx	; Preindexed with long outer disp
		add	ecx, eax	; ie. ([An, Ri*scale, bd], od)
		fetchl			; Fetch intermediary address
		mov	ebx, edx
		mov	ecx, esi	; Fetch outer displacement
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		add	ecx, ebx
		ret


		FALIGN32

global _eac063is4_
_eac063is4_:	jmp	eac063illegal	; Reserved, ie. ILLEGAL OPCODE


		FALIGN32

global _eac063is5_
_eac063is5_:	mov	ecx, ebx	; Postindexed with null outer disp
		fetchl			; Fetch intermediary address
		mov	ecx, edx	; ie. ([An, bd], Ri*scale) 
		add	ecx, eax
		ret


		FALIGN32

global _eac063is6_
_eac063is6_:	mov	ecx, ebx	; Postindexed with word outer disp
		fetchl			; Fetch intermediary address
		mov	ebx, edx	; ie. ([An, bd], Ri*scale, od)
		add	ebx, eax
		mov	ecx, esi	; Fetch outer displacement
		fetchwopcode_inc_pos
		add	esi, 2
		movsx	ecx, dx
		add	ecx, ebx
		ret


		FALIGN32

global _eac063is7_
_eac063is7_:	mov	ecx, ebx	; Postindexed with long outer disp
		fetchl			; Fetch intermediary address
		mov	ebx, edx	; ie. ([An, bd], Ri*scale, od)
		add	ebx, eax
		mov	ecx, esi	; Fetch outer displacement
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ecx, edx
		add	ecx, ebx
		ret


;==============================================================================
;			 68030 specific instructions
;==============================================================================

; Common code for bitfields: (Saves 700 lines of sourcecode.)


; Fetches the extension word and stores it in the specified register
; Destroys ecx and edx

%macro BF_GET_EXTENSION 1 ; EXTREG
		mov	ecx,esi
		fetchwopcode_inc_pos
		add	esi,2
		mov	%1, edx
%endmacro

; Extracts bit field size from the extension word and stores it in the
; specified register

%macro BF_GET_SIZE 2 ;	EXTREG, SIZEREG
		mov	%2, %1
		test	%2, 020h
		jz	%%l1
		and	%2, 7	; size is in Dn 
		mov	%2, dword [da_regs + 4*%2]
%%l1:		dec	%2
		and	%2, 1fh
%endmacro

%macro BF_GET_MASK 2 ;	SIZEREG, MASKREG
		mov	ecx, %1
		mov	%2, 07fffffffh
		shr	%2, cl
		not	%2
%endmacro

%macro BF_GET_SIZEMASK 2 ;  EXTREG, MASKREG
		BF_GET_SIZE %1, %2
		BF_GET_MASK %2, %2
%endmacro

%macro BF_GET_OFFSET 2 ;  EXTREG, OFFREG
		mov	%2, %1
		shr	%2, 6
		and	%2, 01fh
		test	%1, 0800h
		jz	%%l1
		and	%2, 7	; offset is in Dn
		mov	%2, dword [da_regs + 4*%2]
%%l1:
%endmacro

; Calculates ea and fetches two longwords to DEST1 and DEST2

%macro BF_FETCH_MEM 3 ;	 OFFREG, DEST1, DEST2
		mov	%3, %1
		sar	%3, 3
		and	%3, 0fffffffch
		push	%3
		mov	edx, dword [4 + edi]
		call	dword [8 + edi]
		pop	ecx
		add	ecx, edx
		call	fetl
		mov	%2, edx
		add	ecx, 4
		call	fetl
		mov	%3, edx
%endmacro

%macro BF_FETCH_MEM_EA_SAVE 3 ; OFFREG, DEST1, DEST2
		BF_FETCH_MEM %1, %2, %3
		push	ecx
%endmacro

%macro BF_FETCH_REG 2 ; OFFREG, DEST2
		mov	%2, dword [4 + edi]
		mov	%2, dword [%2]
		and	%1, 01fh	; ?
%endmacro

%macro BF_FETCH_REG_EA_SAVE 2 ; OFFREG, DEST2
		mov	%2, dword [4 + edi]
		push	%2
		mov	%2, dword [%2]
		and	%1, 01fh	; ?
%endmacro

; Offset goes to ecx, and OFFREG is used as the left end of the workspace
%macro BF_SHIFTL_96 3 ; OFFREG, DEST1, DEST2
		mov	ecx, %1			; length of the shift
		and	ecx, 1fh
		shld	%1, %2, cl		; move 96-bit work area
		shld	%2, %3, cl
		shl	%3, cl
%endmacro

%macro BF_SHIFTR_96 3 ; DEST0, DEST1, DEST2
		shrd	%3, %2, cl
		shrd	%2, %1, cl
%endmacro

; Offset goes to ecx, and OFFREG is used as the left end of the workspace
%macro BF_SHIFTL_64 2 ; OFFREG, DEST1
		mov	ecx, %1			; length of the shift
		and	ecx, 1fh
		shld	%1, %2, cl		; move 64-bit work area
		shl	%2, cl
%endmacro

%macro BF_SHIFTR_64 2 ; DEST0, DEST1
		shrd	%2, %1, cl
%endmacro

%macro BF_DESTREG_PUSH 1 ; EXTREG
		mov	ecx, %1
		shr	ecx, 10
		and	ecx, 01ch
		push	ecx
%endmacro
		
%macro BF_SET_DESTREG_CYCLES 1 ; FIELDREG
		pop	ecx
		mov	dword [da_regs + ecx], %1
		mov	eax, dword [12 + edi]
%endmacro

%macro BF_FLAGS 2 ; FREG, MASKREG
		and	dword [sr], 0fffffff0h
		test	%1, 80000000h
		jz	%%l1
		or	byte [sr], 8 ; set N according to highest one
%%l1:		test	%1, %2
		jnz	%%l2
		or	byte [sr], 4 ; set Z according to whole field
%%l2:
%endmacro

%macro BF_STORE_MEM_CYCLES 1 ; DEST1
		pop	ecx
		call	wril
		mov	edx, %1
		sub	ecx, 4
		call	wril
		mov	eax, dword [12 + edi]
%endmacro

%macro BF_STORE_REG_CYCLES 1 ; DEST1
		pop	ecx
		mov	dword [ecx], %1
		mov	eax, dword [12 + edi]
%endmacro

%macro BF_EXT 2 ; FREG, SHOP
		pop	edx
		mov	cl, 31
		sub	cl, dl
		%2	%1, cl
%endmacro

%macro BF_INS 2 ; FREG, MASKREG
		not	%2
		and	ebp, %2		; Original field cleared 
		not	%2
		push	%2
		mov	%2, dword [8 + esp]
		mov	ecx, 31
		sub	ecx, %2
		mov	%2, dword [12 + esp]
		mov	%2, dword [da_regs + %2]
		shl	%2, cl
		or	%1, %2
		pop	%2
%endmacro

%macro BF_FFO 3 ; FREG, MASKREG, DESTREG
		and	%1, %2
		xor	%2, %2
		bsr	%3, %1
		jz	%%l1
		mov	%2, 31
		sub	%2, %3
%%l1:		pop	%3
		add	%3, %2
%endmacro



; I have verified that BFEXTU, BFEXTS and BFSET in memory works
; The rest have probably never been executed by any program


;==============================================================================
; Instruction BFCHG <ea>{offset:size}
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i301_M_
_i301_M_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM_EA_SAVE ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		xor	ebp, eax
		BF_SHIFTR_96 ebx, ebp, edx
		BF_STORE_MEM_CYCLES ebp
		ret


		FALIGN32

global _i301_R_
_i301_R_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG_EA_SAVE ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		xor	ebp, eax
		BF_SHIFTR_64 ebx, ebp
		BF_STORE_REG_CYCLES ebp
		ret


;==============================================================================
; Instruction BFCLR <ea>{offset:size}
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i302_M_
_i302_M_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM_EA_SAVE ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		not	eax
		and	ebp, eax
		BF_SHIFTR_96 ebx, ebp, edx
		BF_STORE_MEM_CYCLES ebp
		ret


		FALIGN32

global _i302_R_
_i302_R_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG_EA_SAVE ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		not	eax
		and	ebp, eax
		BF_SHIFTR_64 ebx, ebp
		BF_STORE_REG_CYCLES ebp
		ret


;==============================================================================
; Instruction BFEXTS <ea>{offset:size},Dn
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i303_M_
_i303_M_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		BF_EXT ebp, sar
		BF_SET_DESTREG_CYCLES ebp
		ret
	

		FALIGN32

global _i303_R_
_i303_R_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		BF_EXT ebp, sar
		BF_SET_DESTREG_CYCLES ebp
		ret


;==============================================================================
; Instruction BFEXTU <ea>{offset:size},Dn
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i304_M_
_i304_M_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		BF_EXT ebp, shr
		BF_SET_DESTREG_CYCLES ebp
		ret
	


		FALIGN32

global _i304_R_
_i304_R_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		BF_EXT ebp, shr
		BF_SET_DESTREG_CYCLES ebp
		ret


;==============================================================================
; Instruction BFFFO <ea>{offset:size},Dn
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i305_M_
_i305_M_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		push	ebx	; save offset
		BF_FETCH_MEM ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		BF_FFO ebp, eax, ebx
		BF_SET_DESTREG_CYCLES ebx
		ret


		FALIGN32

global _i305_R_
_i305_R_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		push	ebx	; save offset
		BF_FETCH_REG ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		BF_FFO ebp, eax, ebx
		BF_SET_DESTREG_CYCLES ebx
		ret


;==============================================================================
; Instruction BFINS Dn,<ea>{offset:size}
; Table use: 0 - routine, 1 - dstreg, 2 - dstproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i306_M_
_i306_M_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM_EA_SAVE ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_INS ebp, eax
		BF_FLAGS ebp, eax
		BF_SHIFTR_96 ebx, ebp, edx
		BF_STORE_MEM_CYCLES ebp
		pop	ebx	; Garbage on stack
		pop	ecx
		ret
	

		FALIGN32

global _i306_R_
_i306_R_:	BF_GET_EXTENSION ebp
		BF_DESTREG_PUSH ebp
		BF_GET_SIZE ebp, eax
		push	eax
		BF_GET_MASK eax, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG_EA_SAVE ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_INS ebp, eax
		BF_FLAGS ebp, eax
		BF_SHIFTR_64 ebx, ebp
		BF_STORE_REG_CYCLES ebp
		pop	ebx	; Garbage on stack
		pop	ecx
		ret


;==============================================================================
; Instruction BFSET <ea>{offset:size}
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i307_M_
_i307_M_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM_EA_SAVE ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		or	ebp, eax
		BF_SHIFTR_96 ebx, ebp, edx
		BF_STORE_MEM_CYCLES ebp
		ret


		FALIGN32

global _i307_R_
_i307_R_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG_EA_SAVE ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		or	ebp, eax
		BF_SHIFTR_64 ebx, ebp
		BF_STORE_REG_CYCLES ebp
		ret

	
;==============================================================================
; Instruction BFTST <ea>{offset:size}
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX2346
;==============================================================================


		FALIGN32

global _i308_M_
_i308_M_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_MEM ebx, ebp, edx
		BF_SHIFTL_96 ebx, ebp, edx
		BF_FLAGS ebp, eax
		mov	eax, dword [12 + edi]
		ret


		FALIGN32

global _i308_R_
_i308_R_:	BF_GET_EXTENSION ebp
		BF_GET_SIZEMASK ebp, eax
		BF_GET_OFFSET ebp, ebx
		BF_FETCH_REG ebx, ebp
		BF_SHIFTL_64 ebx, ebp
		BF_FLAGS ebp, eax
		mov	eax, dword [12 + edi]
		ret

	
;==============================================================================
; Instruction CAS Dc,Du,<ea>				     (All, not X)
; Table use: 0 - routine, 1 - srcreg, 2 - eacproc, 3 - timing
; XX234X
;==============================================================================

%macro CAS 5 ;	FETCHFUNC, WRITEFUNC, EBXSIZED, EDXSIZED, SIZESPEC
		mov	edx, [edi + 4]
		call	[edi + 8]		; edx = <ea>
		mov	eax, edx		; eax is <ea> address 
		mov	ecx, esi
		fetchwopcode_inc_pos		; get extra word
		add	esi, 2
		push	edx			; save extension word
		mov	bl, dl			; Start to get Dc
		shl	bl, 2
		and	ebx, 1ch
		mov	ebx, [da_regs + ebx]		; ebx holds Dc operand
		mov	ecx, eax		; Start to get value at <ea>
		call	%1
		cmp	%3, %4			; Compare Dc with <ea> operand
		pushfd				; Set flags
		pop	ecx
		and	ecx, 0fffh
		mov	ebx, dword [statustab + 4*ecx]
		mov	ecx, dword [sr]
		and	ecx, 0fffffff0h
		and	ebx, 0fh
		or	ecx, ebx
		mov	dword [sr], ecx
		cmp	%3, %4
		jnz	%%l1
		pop	ebx			; write Du to <ea>
		shr	bx, 4
		and	ebx, 1ch
		mov	edx, [da_regs + ebx]
		mov	ecx, eax
		call	%2
		mov	eax, [edi + 12]
		ret
%%l1:		pop	ebx			; write <ea> value to Dc
		shl	bl, 2
		and	ebx, 1ch
		mov	%5 [da_regs + ebx], %4
		mov	eax, [edi + 12]
		ret
%endmacro


		FALIGN32

global _i309_B_
_i309_B_:	CAS	fetb, wrib, bl, dl, byte


		FALIGN32

global _i309_W_
_i309_W_:	CAS	fetw, wriw, bx, dx, word


		FALIGN32

global _i309_L_
_i309_L_:	CAS	fetl, wril, ebx, edx, dword


;==============================================================================
; Instruction CAS2 Dc1:Dc2,Du1:Du2,(Rx1):(Rx2)		    (All, not X)
; Table use: 0 - routine, 1 - timing
; XX234X
;==============================================================================

%macro CAS2 4 FETCHFUNC, WRITEFUNC, EDXSIZED, SIZESPEC
		mov	ecx, esi
		fetchlopcode_inc_pos		; get params
		add	esi, 4
		mov	eax, edx
		ror	edx, 16			; first we need Dc1/Du1/Rx1
		mov	bx, dx
		rol	bx, 6
		and	ebx, 3ch
		mov	ecx, [da_regs + ebx]
		call	%1
		push	edx			; Save data from (Rx1)
		push	ecx			; Save eac from Rx1
		mov	ebx, eax
		shr	ebx, 14
		and	ebx, 1ch		; Dc1 register number*4
		cmp	%3, %4 [da_regs + ebx]	; Dc1 = Rx1 ?
		jne	near %%l1
		mov	bx, ax			; First compare equal
		rol	bx, 6
		and	ebx, 3ch
		mov	ecx, [da_regs + ebx]		; ecx - rx2 ea
		call	%1			; edx - rx2 value
		mov	bx, ax
		shl	bx, 2
		and	ebx, 1ch		; ebx - dc2 regnr*4
		cmp	%3, %4 [da_regs + ebx]	; Dc2 = Rx2 ?
		push	ecx
		pushfd				; Set flags for Dc2 Rx2 cmp
		pop	ecx
		and	ecx, 0fffh
		mov	ebx, dword [statustab + 4*ecx]
		mov	ecx, dword [sr]
		and	ecx, 0fffffff0h
		and	ebx, 0fh
		or	ecx, ebx
		mov	dword [sr], ecx
		pop	ecx
		cmp	%3, %4 [da_regs + ebx] ; Dc2 = Rx2 ?
		jne	%%l2
		mov	bx, ax			; Perform Rx1 = Du1, Rx2 = Du2
		shr	bx, 4
		and	bx, 1ch
		mov	edx, [da_regs + ebx]		; Get Du2 value
		call	%2			; (Rx2) <- Du2
		pop	ecx			; Reload Rx1 ea
		ror	eax, 16
		mov	bx, ax
		shr	bx, 4
		and	bx, 1ch
		mov	edx, [da_regs + ebx]		; Get Du1 value
		call	%2			; (Rx1) <- Du1
		pop	edx
		mov	eax, [edi + 4]
		ret
%%l1:		pushfd				; First cmp failed, set flags
		pop	ecx			; Then do Dc1 = Rx1, Dc2 = Rx2
		and	ecx, 0fffh
		mov	ebx, dword [statustab+4*ecx]
		mov	ecx, dword [sr]
		and	ecx, 0fffffff0h
		and	ebx, 0fh
		or	ecx, ebx
		mov	dword [sr], ecx
%%l2:		pop	ecx			; eac Rx1, dont need it
		pop	edx			; (Rx1) value
		mov	ebx, eax
		ror	ebx, 20
		and	ebx, 1ch
		mov	%4 [da_regs + ebx], %3	; Dc1 <- (Rx1)
		mov	bx, ax
		rol	bx, 6
		and	ebx, 3ch
		mov	ecx, [da_regs + ebx]		; eac Rx2
		call	%1			; may not have Rx2 val
		mov	bx, ax
		rol	bx, 2
		and	ebx, 1ch
		mov	%4 [da_regs + ebx], %3	; Dc2 <- (Rx2)
		mov	eax, [edi + 4]
		ret
%endmacro


		FALIGN32

global _i310_W_
_i310_W_:	CAS2	fetw, wriw, dx, word


		FALIGN32

global _i310_L_
_i310_L_:	CAS2	fetl, wril, edx, dword


;==============================================================================
; Instruction CHK.L <ea>,Dn
; Table use: 0 - routine, 1 - srcreg
;	     2 - srcproc, 3 - dstreg
; NOTE:	Word version in 68000.asm
; Word:	012346
; Long:	XX2346
;==============================================================================


		FALIGN32

global _i311_
_i311_:		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		call	eax
		mov	ebx, dword [edi + 12]
		mov	eax, dword [ebx]
		and	dword [sr], 0fffffffbh
		or	eax, eax
		js	i311trapset
		cmp	eax, edx
		ja	i311trap
		mov	eax, 4
		ret
i311trapset:	or	dword [sr], 8
i311trap:	mov	ebx, 18h		; vector at $18 (no. 6)
		mov	eax, 38
		jmp	_cpuPrepareException_


;==============================================================================
; Instruction CHK2.X <ea>,Rn / CMP2.X <ea>,Rn
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc, 3 - timing
; XX234X
;==============================================================================

; Sign extend register is usually EDXSIZED, but since sign extend of
; long to long is not allowed, this is just a dummy for the long version

%macro CHKCMP2 4 EAXSIZED, EDXSIZED, SEXTREG, BINC
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		rol	bx, 6
		push	ebx
		and	ebx, 3ch
		mov	eax, [da_regs + ebx]			; eax - Rn value 
		push	eax				; Save Rn value
		mov	eax, dword [edi + 8]		; Fetch lower bound 
		mov	edx, dword [edi + 4]
		call	eax
		pop	eax
		and	dword [sr], 0fffffffah
		test	bl, 20h
		jnz	%%Areg
		cmp	%1, %2				; Cmp lower bound
		jz	%%ZFSet
		js	%%CFSet
		push	eax				; Lower OK
		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		add	edx, %4
		call	eax				; Fetch higher bound
		pop	eax
		cmp	%1, %2				; Cmp higher bound
		ja	%%CFSet
		jnz	%%End1
%%ZFSet:	or	dword [sr], 4
%%End1:		pop	ebx
%%End2:		mov	eax, dword [edi + 12]
		ret
%%CFSet:	or	dword [sr], 1
		pop	ebx
		test	bl, 2			; are we doing CHK2 or CMP2 ?
		jz	%%End2
		mov	ebx, 18h		; vector at $18 (nr. 6)
		mov	eax, 34
		add	eax, dword [edi + 12]
		jmp	_cpuPrepareException_
%%Areg:		movsx	edx, %3			; Sign extend
		cmp	eax, edx
		jz	%%ZFSet
		js	%%CFSet
		push	eax
		mov	eax, dword [edi + 8]
		mov	edx, dword [edi + 4]
		add	edx, %4
		call	eax
		pop	eax
		movsx	edx, %3
		cmp	eax, edx
		ja	%%CFSet
		jnz	%%End1
		or	dword [sr], 4
		pop	ebx
		mov	eax, dword [edi + 12]
		ret
%endmacro


		FALIGN32

global _i312_B_
_i312_B_:	CHKCMP2 al, dl, dl, 2


		FALIGN32

global _i312_W_
_i312_W_:	CHKCMP2	ax, dx, dl, 2


		FALIGN32

global _i312_L_
_i312_L_:	CHKCMP2	eax, edx, bl, 4


;==============================================================================
; Instruction cpBcc <label>
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - condition
;==============================================================================

	
cpillegal:	jmp	_i00_



		FALIGN32

global _i313_W_
_i313_W_:	jmp	cpillegal
	
	


		FALIGN32

global _i313_L_
_i313_L_:	jmp	cpillegal



;==============================================================================
; Instruction cpDBcc <label>
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - *Dn
;==============================================================================


		FALIGN32

global _i314_
_i314_:		jmp	cpillegal


;==============================================================================
; Instruction cpGEN
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - reg, 4 - srcproc
;==============================================================================


		FALIGN32

global _i315_
_i315_:		jmp	cpillegal

	  mov	  ecx, esi
	fetchw
	add	esi, 2
	ret


;==============================================================================
; Instruction cpRESTORE
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - reg, 4 -srcproc
;==============================================================================


		FALIGN32

global _i316_
_i316_:
	; Note supervisor
		jmp	cpillegal

;==============================================================================
; Instruction cpSAVE
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - reg, 4 -dstreg
;==============================================================================


		FALIGN32

global _i317_
_i317_:
					; NOTE supervisor
		jmp	cpillegal


;==============================================================================
; Instruction cpScc
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - reg, 4 - dstproc
;==============================================================================


		FALIGN32

global _i318_
_i318_:
		jmp	cpillegal


;==============================================================================
; Instruction cpTRAPcc
; Table use: 0 - routine, 1 - disasm, 2 - coprocessor, 3 - optype (00/1W/2L)
;==============================================================================


		FALIGN32

global _i319_
_i319_:
		jmp	cpillegal



;==============================================================================
; Instruction DIVS.L <ea>,Dq / <ea>,Dr:Dq / DIVSL <ea>,Dr:Dq	   (NZV,C=0)
; Table use: 0 - routine, 1 - ea register, 2 - ea read routine
; 32-bit: XX2346
; 64-bit: XX234X
;==============================================================================

; Convert all types of divisions to 64/32 bits.
; For 68k 64 bit divisions, an overflow can occur.
; Problem: Intel generates an exception on divide overflows.
;	   To predict overflow in the result, do the following (64bit) check:
;	   if (MAXINT32*(divisor+1) <= quotient) then overflow
; In the resulting conversion to signed numbers, an overflow can occur,
; does M68K treat this as a proper overflow and leave the operands untouched?


; Divisons are simply too messy to bother with it in asm. This one reads the
; extension word, the ea value and leaves the rest to a C routine.
	

		FALIGN32

global _i320_
_i320_:		mov	ecx, esi			; step 1 Ext. word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	eax, edx
		mov	edx, dword [4 + edi]	; step 4 Get ea
		call	dword [8 + edi]
		push	esi
		I320_CWRAP
		pop	esi
		test	eax, eax
		jz	i320_div0
		js	i320_unimp_060
		mov	eax, 4
		ret

i320_div0:	mov	ebx, 14h
		mov	eax, 200
		jmp	_cpuPrepareException_


i320_unimp_060:			; Raise exception
		ret
	
;==============================================================================
; Instruction EXTB Dn
; Table use: 0 - Routine pointer, 1 - Register pointer
; Flag: X unaffected, V = C = 0, NZ
; Byte:	XX2346
;==============================================================================


		FALIGN32

global _i321_
_i321_:		mov	ecx, dword [edi + 4]
		mov	eax, 4
		mov	edx, dword [sr]
		and	edx, 0fffffff0h
		mov	ebx, dword [ecx]
		movsx	ebx, bl
		mov	dword [ecx], ebx
		test	ebx, ebx
		jz	i321wz
		jns	i321wns
		or	edx, 8
		jmp	i321wns
i321wz:		or	edx, 4
i321wns:	mov	dword [sr], edx
		ret


;==============================================================================
; Instruction LINK.L An,#					   ()
; Table use: 0 - routine, 1 - register pointer
; Word:	012346 found in 68000.asm
; Long:	XX2346
;==============================================================================


		FALIGN32

global _i322_
_i322_:		mov	ebx, [edi + 4]
		mov	edx, [ebx]		   ; get An
		mov	ecx, [da_regs + 60]
		sub	ecx, 4
		mov	[ebx], ecx		   ; Store new A7 into An
		writel				   ; (A7) <- An
		mov	ebx, ecx
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		add	ebx, edx
		mov	[da_regs + 60], ebx		   ; A7 += #
		mov	eax, 16
		ret


;==============================================================================
; Instruction MOVE CCR,<ea>					   ()
; Table use: 0 - routine, 1 - dstreg, 2 - dstproc, 3 - timing
; X12346
;==============================================================================


		FALIGN32

global _i323_
_i323_:		mov	dl, byte [sr]
		mov	dh, 0
		mov	ebx, [edi + 4]
		call	[edi + 8]
		mov	eax, dword [edi + 12]
		ret


;==============================================================================
; Instruction MOVEC Rc,Rn / Rn,Rc				   ()
; Table use: 0 - routine
; X12346
;==============================================================================

; Not speed-critical, cleanness favored over speed.

; Move to cacr, collect and report dummy values


		FALIGN32

cacrTo020_:	and	eax, 3
		mov	dword [cacr], eax
		ret


		FALIGN32

cacrTo030_:	and	eax, 0303h
		mov	dword [cacr], eax
		ret


		FALIGN32

cacrTo040_:	and	eax, 080008000h
		mov	dword [cacr], eax
		ret


		FALIGN32

cacrTo060_:	and	eax, 0f880e000h
		mov	dword [cacr], eax
		ret


		FALIGN32

global _i324_TO_
_i324_TO_:	test	byte [sr + 1], 20h	; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, edx
		and	ebx, 0fffh
		shr	eax, 10
		and	eax, 03ch
		mov	eax, dword [da_regs + eax]

		cmp	ebx, 000h		; SFC  X12346
		jne	i324w_1
		and	eax, 7 
		mov	dword [sfc], eax
		jmp	i324w_end

i324w_1:	cmp	ebx, 001h		; DFC  X12346
		jne	i324w_2
		and	eax, 7
		mov	dword [dfc], eax
		jmp	i324w_end

i324w_2:	cmp	ebx, 0800h		; USP  X12346
		jne	i324w_3
		mov	dword [usp], eax
		jmp	i324w_end

i324w_3:	cmp	ebx, 0801h		; VBR  X12346
		jne	i324w_4
		mov	dword [vbr], eax
		jmp	i324w_end

i324w_4:	cmp	ebx, 002h		; CACR XX2346
		jne	i324w_5

		cmp	dword [cpu_type], 2
		jb	near i324w_illegal
		jne	i324w_11
		call	cacrTo020_
		jmp	i324w_end
i324w_11:	cmp	dword [cpu_type], 3
		jne	i324w_12
		call	cacrTo030_
		jmp	i324w_end
i324w_12:	cmp	dword [cpu_type], 4
		jne	i324w_13
		call	cacrTo040_
		jmp	i324w_end
i324w_13:	call	cacrTo060_
		jmp	i324w_end

i324w_5:	cmp	ebx, 0802h		; CAAR XX23XX
		jne	i324w_6

		cmp	dword [cpu_type], 2
		jb	near i324w_illegal
		cmp	dword [cpu_type], 4
		jae	near i324w_illegal
		and	eax, 0fch
		mov	dword [caar], eax
		jmp	i324w_end

i324w_6:	cmp	ebx, 0803h		; MSP  XX234X
		jne	i324w_7

		cmp	dword [cpu_type], 2
		jb	near i324w_illegal
		cmp	dword [cpu_type], 6
		jae	near i324w_illegal
		mov	dword [msp], eax
		jmp	i324w_end

i324w_7:	cmp	ebx, 0804h		; ISP(SSP here)	 XX2346
		jne	i324w_8

		cmp	dword [cpu_type], 2
		jb	i324w_illegal
		mov	dword [ssp], eax
		jmp	i324w_end

i324w_8:	cmp	ebx, 004h		; IACR0 (EC040)
		jne	i324w_14
		cmp	dword [cpu_type], 4
		jne	i324w_illegal
	
		jmp	i324w_end
i324w_14:	cmp	ebx, 005h		; IACR1 (EC040)
		jne	i324w_15
		cmp	dword [cpu_type], 4
		jne	i324w_illegal

		jmp	i324w_end
i324w_15:	cmp	ebx, 006h		; DACR0 (EC040)
		jne	i324w_16
		cmp	dword [cpu_type], 4
		jne	i324w_illegal

		jmp	i324w_end
i324w_16:	cmp	ebx, 007h		; DACR1 (EC040)
		jne	i324w_17
		cmp	dword [cpu_type], 4
		jne	i324w_illegal

		jmp	i324w_end
i324w_17:					; ILLEGAL OPCODE when here
i324w_illegal:	mov	ebx,010h
		mov	eax,38
		jmp	_cpuPrepareException_

i324w_end:	mov	eax, 4
		ret

; From control register


		FALIGN32

global _i324_FROM_
_i324_FROM_:	test	byte	 [sr + 1], 20h	; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	eax, edx
		and	ebx, 0fffh
		shr	eax, 10
		and	eax, 03ch

		cmp	ebx, 000h		; SFC  X12346
		jne	i324m_1
		mov	ecx, dword [sfc]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_1:	cmp	ebx, 001h		; DFC  X12346
		jne	i324m_2
		mov	dword [dfc], eax
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_2:	cmp	ebx, 0800h		; USP  X12346
		jne	i324m_3
		mov	eax, dword [usp]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_3:	cmp	ebx, 0801h		; VBR  X12346
		jne	i324m_4
		mov	ecx, dword [vbr]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_4:	cmp	ebx, 002h		; CACR XX2346
		jne	i324m_5

		cmp	dword [cpu_type], 2
		jb	near i324m_illegal
		mov	ecx, dword [cacr]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_5:	cmp	ebx, 0802h		; CAAR XX23XX
		jne	i324m_6

		cmp	dword [cpu_type], 2
		jb	near i324m_illegal
		cmp	dword [cpu_type], 4
		jae	near i324m_illegal
		mov	ecx, dword [caar]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_6:	cmp	ebx, 0803h		; MSP  XX234X
		jne	i324m_7
		cmp	dword [cpu_type], 2
		jb	near i324m_illegal
		cmp	dword [cpu_type], 6
		jae	near i324m_illegal
		mov	ecx, dword [msp]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_7:	cmp	ebx, 0804h		; ISP  XX2346
		jne	i324m_8
		cmp	dword [cpu_type], 2
		jb	near i324m_illegal
		mov	ecx, dword [ssp]
		mov	dword [da_regs + eax], ecx
		jmp	i324m_end

i324m_8:	cmp	ebx, 004h		; IACR0 (EC040)
		jne	i324m_14
	
		cmp	dword [cpu_type], 4
		jne	i324m_illegal
		jmp	i324m_end
i324m_14:	cmp	ebx, 005h		; IACR1 (EC040)
		jne	i324m_15
		cmp	dword [cpu_type], 4
		jne	i324m_illegal

		jmp	i324m_end
i324m_15:	cmp	ebx, 006h		; DACR0 (EC040)
		jne	i324m_16
		cmp	dword [cpu_type], 4
		jne	i324m_illegal

		jmp	i324m_end
i324m_16:	cmp	ebx, 007h		; DACR1 (EC040)
		jne	i324m_17
		cmp	dword [cpu_type], 4
		jne	i324m_illegal

		jmp	i324m_end
i324m_17:					; ILLEGAL OPCODE when here
						; ILLEGAL OPCODE when here
i324m_illegal:	
		mov	ebx,010h
		mov	eax,38
		jmp	_cpuPrepareException_

i324m_end:	mov	eax, 4
		ret


;==============================================================================
; Instruction MOVES Rn,<ea> / <ea>,Rn				   ()
; Table use: 0 - routine, 1 - reg
;	     2 - arproc,  3 - awproc 
; It should use SFC / DFC registers to access memory... Currently, it doesn't
;==============================================================================


		FALIGN32

global _i325_B_
_i325_B_:	test	byte [sr + 1], 20h	; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	bx, dx
		rol	bx, 6
		and	ebx, 3ch
		test	dx, 800h
		jz	i325_Br
		mov	edx, [da_regs + ebx]		 ; write to memory
		mov	ebx, [edi + 4]
		call	[edi + 12]
		mov	eax, 4
		ret
i325_Br:	mov	edx, [edi + 4]
		call	[edi + 8]
		test	bl, 8			 ; is this An register ?
		jz	i325_B1
		movsx	edx, dl
		mov	[da_regs + ebx], edx
		mov	eax, 4
		ret
i325_B1:	mov	byte [da_regs + ebx], dl
		mov	eax, 4
		ret


		FALIGN32

global _i325_W_
_i325_W_:	test	byte [sr + 1], 20h	; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	bx, dx
		rol	bx, 6
		and	ebx, 3ch
		test	dx, 800h
		jz	i325_Wr
		mov	edx, [da_regs + ebx]		 ; write to memory
		mov	ebx, [edi + 4]
		call	[edi + 12]
		mov	eax, 4
		ret
i325_Wr:	mov	edx, [edi + 4]
		call	[edi + 8]
		test	bl, 8			 ; is this An register ?
		jz	i325_W1
		movsx	edx, dx
		mov	[da_regs + ebx], edx
		mov	eax, 4
		ret
i325_W1:	mov	word [da_regs + ebx], dx
		mov	eax, 4
		ret


		FALIGN32

global _i325_L_
_i325_L_:	test	byte [sr + 1], 20h	; Are we in supervisormode??
		jz	near _cpuPrivilegieViolation_
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		mov	bx, dx
		rol	bx, 6
		and	ebx, 3ch
		test	dx, 800h
		jz	i325_Lr
		mov	edx, [da_regs + ebx]		 ; write to memory
		mov	ebx, [edi + 4]
		call	[edi + 12]
		mov	eax, 4
		ret
i325_Lr:	mov	edx, [edi + 4]
		call	[edi + 8]
		mov	[da_regs + ebx], edx
		mov	eax, 4
		ret


;==============================================================================
; Instruction MULS.L <ea>,Dn / <ea>,Dh:Dl			   (NZV,C=0)
; Table use: 0 - routine, 1 - earegister, 2 - scrproc
;==============================================================================


		FALIGN32

global _i326_
_i326_:		mov	ecx, esi		; Extension word
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx		; Get dl nr
		shr	ebx, 10
		and	ebx, 1ch
		mov	eax, dword[da_regs + ebx]; Get dl value
		push	ebx			; Save dl nr
		mov	ebx, edx
		mov	edx, dword [edi + 4]
		call	dword [edi + 8]	; Get value at <ea>
		test	bh, 8
		jz	i326um
		mov	ebp, 1			; Flag signed operations
		imul	edx			; Signed mul
		jmp	short i326cm
i326um:		xor	ebp, ebp		; Flag unsigned operations
		mul	edx			; Unsigned mul
i326cm:		pop	ecx
		mov	dword [da_regs + ecx], eax; Low result into dl
		and	byte [sr], 0f0h
		mov	cl, 0
		test	bh, 4
		jz	i326_3
		and	ebx, 7			; 64 bit result
		mov	dword [da_regs + 4*ebx], edx
		test	eax, eax
		jnz	i326_1
		test	edx, edx
		jnz	i326_1
		or	cl, 4			; Zero 64 bit result 
i326_1:		test	edx, edx
		jns	i326_2
		or	cl, 8			; Negative 64 bit result
i326_2:		or	byte [sr], cl
		mov	eax, 4
		ret

i326_3:		test	edx, edx		; 32 bit result
		jz	i326_4			; zero is always OK
		test	ebp, ebp
		jz	i326_7			; Always overflow if unsig
		cmp	edx, 0ffffffffh
		je	i326_4
i326_7:		or	cl, 2			; 32 bit overflowed result
i326_4:		test	eax, eax
		jnz	i326_5
		or	cl, 4			; 32 bit zero result
i326_5:		test	eax, eax
		jns	i326_6			; 32 bit negative result
		or	cl, 8
i326_6:		or	byte [sr], cl
		mov	eax, 4
		ret


;==============================================================================
; Instruction PACK -(Ax),-(Ay),# / Dx,Dy,#			   ()
; Table use: 0 - routine, 1 - srcear, 2 - scrproc(W)
;	     3 - dstear, 4 - dstproc(B)
;==============================================================================


		FALIGN32

global _i327_
_i327_:		mov	edx, [edi + 4]
		call	[edi + 8]
		mov	eax, edx
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		add	edx, eax
		shl	dh, 4
		and	dl, 0fh
		or	dl, dh
		mov	ebx, [edi + 12]
		call	[edi + 16]
		mov	eax, 4
		ret


;==============================================================================
; Instruction PFLUSH A / <fc>,# / <fc>,#,<ea>			   ()
; Instruction PLOADR <fc>,<ea> / PLOADW <fc>,<ea>		   ()
; Instruction PMOVE reg,<ea> / <ea>,reg / PMOVEFD <ea>,reg	   ()
; Instruction PTESTR/W <fc>,<ea>,# / <fc>,<ea>,#,An		   ()
; Table use: 0 - routine, 1 - srcreg, 2 - srcproc
;==============================================================================
; PFLUSH is not and probably will NEVER be really supported (ATC command)
; PLOAD	 is not and probably will NEVER be really supported (ATC command)
; PMOVE	 is not yet supported (MMU command)
; PTEST	 is not and probably will NEVER be really supported (ATC command)
; Note:	Supervisor


		FALIGN32

global _MMUIllegal030_
_MMUIllegal030_:test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_	; Priv in user mode 
		mov	ebx, 02ch		; F-line in super mode 
		mov	eax, 4
		jmp	_cpuPrepareException_

	
		FALIGN32

global _MMU030_
_MMU030_:	test	byte [sr + 1], 020h
		jz	near _cpuPrivilegieViolation_
	
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2		; skip param word
		mov	eax, 4
		ret


		FALIGN32

global _PFLUSHEC040_
_PFLUSHEC040_:	mov	eax, 4			; NOP on EC040
		ret


		FALIGN32

global _PTESTEC040_
_PTESTEC040_:	mov	eax, 4			; NOP on EC040
		ret


;==============================================================================
; Instruction RTD #						   ()
; Table use: 0 - routine
;==============================================================================


		FALIGN32

global _i329_
_i329_:		mov	ecx, esi
		fetchwopcode_inc_pos
		movsx	eax, dx
		mov	ecx, dword [da_regs + 60]
		fetchl
		mov	esi, edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		add	ecx, 4
		add	ecx, eax
		mov	dword [da_regs + 60], ecx
		mov	eax, 4
		ret


;==============================================================================
; Instruction TRAPcc / .W # / .L #				   ()
; Table use: 0 - routine, 1 - cond handle proc, 2 - mode
;==============================================================================


		FALIGN32

global _i330_
_i330_:		call	[edi + 4]		; edx = 1 succeeded
		test	dl, dl
		jz	near i330_n
		cmp	dword [edi + 8], 0
		je	near i330_0
		mov	ecx, esi
		cmp	dword [edi + 8], 1
		je	i330_w
		fetchlopcode_inc_pos
		add	esi, 4
		jmp	short i330_0
i330_w:		
		fetchwopcode_inc_pos
		add	esi, 2
i330_0:		mov	ebx, 1ch		; make exc 7
		mov	eax, 100
		jmp	_cpuPrepareException_

i330_n:		cmp	dword [edi + 8], 0
		je	i330_e
		add	esi, 2
		cmp	dword [edi + 8], 1
		je	i330_e
		add	esi, 2
i330_e:		mov	eax, 4
		ret


;==============================================================================
; Instruction UNPK -(Ax),-(Ay),# / Dx,Dy,#			   ()
; Table use: 0 - routine, 1 - srcear, 2 - scrproc(B)
;	     3 - dstear, 4 - dstproc(W)
;==============================================================================


		FALIGN32

global _i331_
_i331_:		mov	edx, [edi + 4]
		call	[edi + 8]
		mov	ah, dl
		shl	ah, 4
		mov	al, dl
		and	al, 0fh
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		add	dx, ax
		mov	ebx, [edi + 12]
		call	[edi + 16]
		mov	eax, 4
		ret


;==============================================================================
; Instruction CALLM #x, <ea>			     
; Table use: 0 - routine, 1 - src reg, 2 - ea calculate rutine
;==============================================================================


;------------------------------------------------------------
; Validates the format of a module descriptor or stack frame
; Input:  edx - opt-type byte
; Output: ebp - opt,  edx - type
;------------------------------------------------------------


		FALIGN32

callm_validate_:mov	edx, ebp		; Check opt 
		shr	ebp, 5
		and	ebp, 7
		test	ebp, ebp
		jz	callm_optOK
		cmp	ebp, 4
		jne	callm_ferr
callm_optOK:	and	edx, 01fh		; Check type
		test	edx, edx
		jz	callm_typOK
		cmp	edx, 1
		jne	callm_ferr
callm_typOK:	ret
		

	
; CALLM format error below
	
callm_ferr:	ret		; Do nothing about it right now

	
		FALIGN32

global _i332_
_i332_:		mov	ecx, esi	; Load argument count
		fetchwopcode_inc_pos
		add	esi, 2
		mov	ebx, edx
		mov	edx, [edi + 4]	; Calulate module descriptor pointer 
		call	dword [edi + 8]
		mov	eax, ecx
		fetchb			; Fetch opt type field 
		call	callm_validate_
		
		mov	eax, 4
		ret


;==============================================================================
; Instruction RTM Rn
; Table use: 0 - routine, 1 - reg
;==============================================================================

; Just advance PC, no action now


		FALIGN32

global _i333_
_i333_:		mov	eax, dword [edi + 4]	 ; Module data area ptr 
		mov	ebx, dword [da_regs + 60]	 ; Get stack ptr 
		mov	ecx, ebx		  ; Get opt and type
		fetchb
		call	callm_validate_
		mov	ecx, ebx		  ; Get condition codes 
		add	ecx, 3
		fetchb
		mov	byte [sr], dl
		mov	ecx, ebx		  ; Update data area ptr 
		add	ecx, 12
		fetchl
		mov	dword [eax], edx
		mov	ecx, ebx		  ; Get saved PC
		add	ecx, 12
		fetchl
		mov	esi, edx
		PC_TO_PTR esi
%ifdef PREFETCH
		mov	edx, esi
		call	prefetch_fillASM
%endif
		mov	ecx, ebx		  ; Get saved stack ptr 
		add	ecx, 20
		fetchl
		mov	dword [da_regs + 60], edx	 ; Update stack ptr 
		mov	eax, 4
		ret


;==============================================================================
; Instruction MOVE16 (040 060)
;
; Type 1: move16 (Ax)+, (Ay)+	(post-increment format)
;
; Type 2: move16 $L, (An)   11	(absolute long address or destination)
;	  move16 $L, (An)+  01
;	  move16 (An), $L   10
;	  move16 (An)+, $L  00	  
;
; Table-usage:
;   0 - Routine-pointer	 1 - Pointer to address register
;==============================================================================


		FALIGN32

global _i334_1_
_i334_1_:	mov	eax, dword [edi + 4]
		mov	ebx, dword [eax]		; ebx - Src EA
		add	dword [eax], 16
		mov	ecx, esi
		fetchwopcode_inc_pos
		add	esi, 2
		shr	edx, 11
		and	edx, 03ch
		mov	eax, dword [da_regs + 32 + edx]	; eax - Dst EA
		and	eax, 0fffffff0h
		and	ebx, 0fffffff0h
		mov	ebp, 12
i334_1_a:	lea	ecx, [ebx + ebp]
		fetchl
		lea	ecx, [eax + ebp]
		writel
		sub	ebp, 4
		jns	near i334_1_a
		mov	eax, 4
		ret


		FALIGN32

global _i334_2_00_
_i334_2_00_:	mov	eax, dword [edi + 4]
		mov	ebx, dword [eax]		; ebx - Src EA
		add	dword [eax], 16
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	eax, edx			; eax - Dst EA
		and	eax, 0fffffff0h
		and	ebx, 0fffffff0h
		mov	ebp, 12
i334_2_00_a:	lea	ecx, [ebx + ebp]
		fetchl
		lea	ecx, [eax + ebp]
		writel
		sub	ebp, 4
		jns	near i334_2_00_a
		mov	eax, 4
		ret


		FALIGN32

global _i334_2_01_
_i334_2_01_:	mov	ebx, dword [edi + 4]
		mov	eax, dword [ebx]		; eax - Dst EA
		add	dword [ebx], 16
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ebx, edx			; ebx - Src EA
		mov	ebx, dword [da_regs + 32 + ebx]	; Dst EA
		and	eax, 0fffffff0h
		and	ebx, 0fffffff0h
		mov	ebp, 12
i334_2_01_a:	lea	ecx, [ebx + ebp]
		fetchl
		lea	ecx, [eax + ebp]
		writel
		sub	ebp, 4
		jns	near i334_2_01_a
		mov	eax, 4
		ret



		FALIGN32

global _i334_2_10_
_i334_2_10_:	mov	eax, dword [edi + 4]
		mov	ebx, dword [eax]		; ebx - Src EA
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	eax, edx			; eax - Dst EA
		and	eax, 0fffffff0h
		and	ebx, 0fffffff0h
		mov	ebp, 12
i334_2_10_a:	lea	ecx, [ebx + ebp]
		fetchl
		lea	ecx, [eax + ebp]
		writel
		sub	ebp, 4
		jns	near i334_2_10_a
		mov	eax, 4
		ret


		FALIGN32

global _i334_2_11_
_i334_2_11_:	mov	ebx, dword [edi + 4]
		mov	eax, dword [ebx]		; eax - Dst EA
		mov	ecx, esi
		fetchlopcode_inc_pos
		add	esi, 4
		mov	ebx, edx			; ebx - Src EA
		mov	ebx, dword [da_regs + 32 + ebx]	; Dst EA
		and	eax, 0fffffff0h
		and	ebx, 0fffffff0h
		mov	ebp, 12
i334_2_11_a:	lea	ecx, [ebx + ebp]
		fetchl
		lea	ecx, [eax + ebp]
		writel
		sub	ebp, 4
		jns	near i334_2_11_a
		mov	eax, 4
		ret


;==============================================================================
; Instruction CINV and CPUSH (040 040LC)
;
; NOP implementation
;
; Table-usage:
;   0 - Routine-pointer	 1 - reg ptr,  2 - cache spec,	3 - scope spec
;==============================================================================


		FALIGN32

global _CINV040_
_CINV040_:	mov	eax, 4			; NOP 
		ret


		FALIGN32

global _CPUSH040_
_CPUSH040_:	mov	eax, 4			; NOP
		ret


		FALIGN32


FCODESECTIONEND

FBSSSECTIONSTART
FBSSSECTIONEND

FASMFILEEND
