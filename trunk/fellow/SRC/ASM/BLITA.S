;===============================================================================
; Fellow Amiga Emulator      
; Blitter (OCS) Emulation
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
;Changelog:
;
; 2000 12 21 (PS) - Reorganized linemode using macros. No unsafe pointers.
;                   Changed doubtful handling of data from the B channel.
;
;===============================================================================


;===============================================================================
; Summary of what is in this file:
;
; * Get and Set functions for each blitter hardware register.
; * Start and finish code for a generic blitter operation. (Does all the work
;   in one fell swoop.) A blitter operation is an event type. Look busa.asm
; * Implementation of blitter line mode (The blitter uses a plain Bresenham)
; * Implementation of blitter fill mode.
; * Implementation of blitter copy mode.
; * Implementation of blitter D = A B C logic
; 
; Summary of optimizations:
;
; * The blitter always feed data from all three channels into its minterm logic,
;   even when some channels are turned off. (ie. some channels feed the same
;   word throughout the blit.) 
;   A number of common ABC channel combinations have been implemented in a
;   faster way, knowing that some channels contain static data.
;   Line mode is always generic though.
; * Blitter logic, each minterm combination could be reduced to a small
;   expression. Some common minterm expressions have been implemented.
;   The rest uses a generic routine that dumbly OR the result of the 8
;   minterms.
;   Implementing all 256 expressions by hand would be stupid. Much better to
;   use an automatic optimizer to generate the code. I have not made such an
;   expression optimizer.
;
; Status of the code in this file:
;
; Drawbacks:
;
; * Unreadable, dominated by slightly fuzzy thinking.
; * One known problem: A few demos use linemode to rotate images by drawing
;   lines that take image data as a texture from a blitter data register.
;   The way the blitter rotates data registers in linemode has some pitfalls.
;   I have also seen demos where I have verified that the demo use a similar
;   technique with apparently correct result, so it is not all wrong, I just
;   simply don't have full control over that particular detail.
;
; Advantages:
; 
; * It is uncommon to see programs fail to run correctly related to bugs in
;   the blitter code.  (Normally it is related to doing the full blitter op
;   at once, which makes it difficult to accurately calculate the actual
;   time the operation takes. But to do that, you need a cycle exact emulator
;   that dynamically monitors and compensates all side-effects.)
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

%define BLITTER_ASM
;%define BLIT_TSC_PROFILE

FASMFILESTART
FDATASECTIONSTART

%include "data/blit.inc"
%include "data/bus.inc"
%include "data/copper.inc"
%include "data/fmem.inc"
%include "data/cpu.inc"

%ifdef BLIT_TSC_PROFILE
CDATAEXTERN blit_tsc_tmp
CDATAEXTERN blit_tsc
CDATAEXTERN blit_tsc_times
%endif

FDATASECTIONEND
FCODESECTIONSTART
FALIGNHELPER

global _blitstart_
_blitstart_:				; Used to check code alignment

%include "func/blit.inc"
%include "func/bus.inc"
%include "func/copper.inc"
%include "func/fmem.inc"
%include "mac/bus.mac"
%include "mac/blit.mac"


;==========================
; Blitter IO register stubs
;==========================


;========
; BLTCON0
;========
; $dff040

; Common code for $040 and $05a
%macro MINTERM_SET 0
		and	edx, 0ffh
		mov	dword [blit_minterm], edx
		mov	edx, dword [blit_min_functable + 4*edx]
		mov	dword [blit_asm_minterm], edx
%endmacro

		FALIGN32

global _wbltcon0_
_wbltcon0_:	BLIT_FINISH
		mov	word [bltcon + 2], dx
		mov	ecx, edx
		MINTERM_SET
		and	ecx, 0ffffh
		shr	ecx, 12
		mov	edx, 16
		mov	dword [blit_a_shift_asc], ecx
		sub	edx, ecx
		mov	dword [blit_a_shift_desc], edx
		ret


;=========
; BLTCON0L
;=========
; $dff05A


		FALIGN32

global _wbltcon0l_
_wbltcon0l_:	BLIT_FINISH
		mov	byte [bltcon + 2], dl
		MINTERM_SET
		ret


;========
; BLTCON1
;========
; $dff042


		FALIGN32

global _wbltcon1_
_wbltcon1_:	BLIT_FINISH
		mov	word [bltcon], dx
		mov	ecx, 0
		test	edx, 2
		jz	.ascending
		mov	ecx, 1
.ascending:	mov	dword [blit_desc], ecx
		and	edx, 0ffffh
		shr	edx, 12
		mov	ecx, 16
		mov	dword [blit_b_shift_asc], edx
		sub	ecx, edx
		mov	dword [blit_b_shift_desc], ecx
		ret


;========
; BLTAFWM
;========
; $dff044


		FALIGN32

global _wbltafwm_
_wbltafwm_:	BLIT_FINISH
		mov	dword [bltafwm], edx
		ret


;========
; BLTALWM
;========
; $dff046


		FALIGN32

global _wbltalwm_
_wbltalwm_:	BLIT_FINISH
		mov	dword [bltalwm], edx
		ret


;=======
; BLTCPT
;=======
; $dff048 - WORD


		FALIGN32

global _wbltcpth_
_wbltcpth_:	BLIT_FINISH
		and	edx, 01fh
		mov	word [bltcpt + 2], dx
		ret


; $dff04a - WORD


		FALIGN32

global _wbltcptl_
_wbltcptl_:	BLIT_FINISH
		and	edx, 0fffeh
		mov	word [bltcpt], dx
		ret


;=======
; BLTBPT
;=======
; $dff04c - WORD


		FALIGN32

global _wbltbpth_
_wbltbpth_:	BLIT_FINISH
		and	edx, 01fh
		mov	word [bltbpt + 2], dx
		ret


; $dff04e - WORD


		FALIGN32

global _wbltbptl_
_wbltbptl_:	BLIT_FINISH
		and	edx, 0fffeh
		mov	word [bltbpt], dx
		ret


;=======
; BLTAPT
;=======
; $dff050 - WORD


		FALIGN32

global _wbltapth_
_wbltapth_:	BLIT_FINISH
		and	edx, 01fh
		mov	word [bltapt + 2], dx
		ret


; $dff052 - WORD


		FALIGN32

global _wbltaptl_
_wbltaptl_:	BLIT_FINISH
		and	edx, 0fffeh
		mov	word [bltapt], dx
		ret


;=======
; BLTDPT
;=======
; $dff054 - WORD


		FALIGN32

global _wbltdpth_
_wbltdpth_:	BLIT_FINISH
		and	edx, 01fh
		mov	word [bltdpt + 2], dx
		ret


; $dff056 - WORD


		FALIGN32

global _wbltdptl_
_wbltdptl_:	BLIT_FINISH
		and	edx, 0fffeh
		mov	word [bltdpt], dx
		ret


;========
; BLTSIZE
;========
; $dff058


		FALIGN32

global _wbltsize_
_wbltsize_:	BLIT_FINISH
		mov	ecx, edx
		and	ecx, 03fh		; Width
		jnz	.l1
		mov	ecx, 64
.l1:		mov	dword [blit_width], ecx
		shr	edx, 6			; Height
		and	edx, 3ffh
		jnz	.l2
		mov	edx, 1024
.l2:		mov	dword [blit_height], edx
		test	byte [dmacon], 40h
		jnz	near _blitter_copy_
		mov	dword [blitterdmawaiting], 1
		ret


;========
; BLTSIZV
;========
; $dff05C


		FALIGN32

global _wbltsizv_
_wbltsizv_:	BLIT_FINISH
		and	edx, 07fffh		; Height
		jnz	.l1
		mov	edx, 08000h
.l1:		mov	dword [blit_height], edx
		ret


;========
; BLTSIZH
;========
; $dff05E


		FALIGN32

global _wbltsizh_
_wbltsizh_:	BLIT_FINISH
		and	edx, 07ffh		; Width
		jnz	.l1
		mov	edx, 0800h
.l1:		mov	dword [blit_width], edx
		test	byte [dmacon], 40h
		jnz	near _blitter_copy_
		mov	dword [blitterdmawaiting], 1
		ret


;========
; BLTCMOD
;========
; $dff060


		FALIGN32

global _wbltcmod_
_wbltcmod_:	BLIT_FINISH
		and	edx, 0fffeh		; Even modulo
		movsx	edx, dx
		mov	dword [bltcmod], edx
		ret


;========
; BLTBMOD
;========
; $dff062


		FALIGN32

global _wbltbmod_
_wbltbmod_:	BLIT_FINISH
		and	edx, 0fffeh		; Even modulo
		movsx	edx, dx
		mov	dword [bltbmod], edx
		ret


;========
; BLTAMOD
;========
; $dff064


		FALIGN32

global _wbltamod_
_wbltamod_:	BLIT_FINISH
		and	edx, 0fffeh		; Even modulo
		movsx	edx, dx
		mov	dword [bltamod], edx
		ret


;========
; BLTDMOD
;========
; $dff066


		FALIGN32

global _wbltdmod_
_wbltdmod_:	BLIT_FINISH
		and	edx, 0fffeh		; Even modulo
		movsx	edx, dx
		mov	dword [bltdmod], edx
		ret


;========
; BLTCDAT
;========
; $dff070


		FALIGN32

global _wbltcdat_
_wbltcdat_:	BLIT_FINISH
		mov	dword [bltcdat], edx
		ret


;========
; BLTBDAT
;========
; $dff072


		FALIGN32

global _wbltbdat_
_wbltbdat_:	BLIT_FINISH
		and	edx, 0ffffh
		mov	ecx, dword [blit_b_shift_asc]
		mov	dword [bltbdat_original], edx
		test	dword [blit_desc], 1
		jnz	.l1
		shr	dx, cl
		mov	dword [bltbdat], edx
		ret
.l1:		shl	dx, cl
		mov	dword [bltbdat], edx
		ret


;========
; BLTADAT
;========
; $dff074


		FALIGN32

global _wbltadat_
_wbltadat_:	BLIT_FINISH
		mov	dword [bltadat], edx
		ret


; ===========================================
; Entrypoint for blitter operations
; ===========================================

; -------------------------------------------
; Assume never called when blitter DMA is off
; Only calculate the cyclecount
; Set blitend to the cycle when the blit ends
; Set the blitter not ready flag in dmacon
; If blitpri is on, write the cyclecount to
; thiscycle
; -------------------------------------------


		FALIGN32

global _blitter_copy_
_blitter_copy_:	push	ebx
		push	eax

		test	dword [blitter_operation_log], 0ffffffffh
		jz	.no_log
		BLITTEROPERATIONLOG_CWRAP
.no_log:
		mov	dword [bltzero], 0
		mov	eax, dword [bltcon]
		shr	eax, 24				; calc cycles
		and	eax, 0fh
		test	dword [bltcon], 1
		jnz	linecyc
		mov	ebx, dword [blit_cycletab + 4*eax]
		jmp	nolinecyc
linecyc:	mov	ebx, 6		 
nolinecyc:	test	byte [blitter_fast], 0ffh
		jnz	fastblit
		imul	ebx, dword [blit_width]		; cycles per line
		imul	ebx, dword [blit_height]	; total cyclecount
fastblit:	mov	eax, 1				; State 1, no bltpri
		mov	dword [thiscycle], 0
		test	dword [dmaconr], 0400h		; test bltpri
		jz	nobltpri
		mov	ah, byte [bltcon + 3]
		and	ah, 15
		cmp	ah, 1		
		je	nobltpri
		inc	al				; State 2, bltpri blt
		shr	ebx, 1				; Set blitend cycle
		test	byte [blitter_fast], 0ffh
		jnz	nobltpri
		mov	dword [thiscycle], ebx
nobltpri:	shr	ebx, 1
		add	ebx, dword [curcycle]
		mov	dword [blitend], ebx
		mov	ah, 0				; Set blitter busy bit
		or	dword [dmaconr], 04000h
		mov	dword [blitterstatus], eax
		SCAN_EVENTS_LVL4
		pop	eax
		pop	ebx
		ret


;=======================================================
; This function does the blit, called on the last cycle
; of the blit.
;=======================================================

CFUNCEXTERN blitterCopyABCD
	
		FALIGN32

global _finish_blit_
_finish_blit_:
		mov	dword [blitend], -1
		SCAN_EVENTS_LVL4
		mov	dword [blitterdmawaiting], 0
		mov	dword [blitterstatus], 0
		and	dword [dmaconr], 0bfffh	
		mov	ebx, dword [bltcon]
		test	ebx, 1
		jnz	blitterlinemode
		test	ebx, 18h
		jnz	near blitterfillmode_new
%ifdef BLIT_TSC_PROFILE
		TSC_BEFORE blit_tsc_tmp
%endif
		call	blitterCopyABCD
%ifdef BLIT_TSC_PROFILE
		TSC_AFTER blit_tsc_tmp, blit_tsc, blit_tsc_times
%endif
		ret

blitterfillmode_new:
		call	blitterCopyABCD
		ret

; ==============================
; Blitter line mode entry point
; ==============================

; On entry the stack contains the following variables:
; 
; [esp]		- blit_asm_minterm
; 4[esp]	- bltlinedecision flag
; 8[esp]	- blit_height
; 12[esp]	- bltamod
; 16[esp]	- bltbmod
; 20[esp]	- bltcmod
; 24[esp]	- bltadat
; 28[esp]	- bltcdat
; 32[esp]	- bltlinepointflag
; 36[esp]	- bltzero
; 40[esp]	- bltcon0 and 1


		FALIGN32


blitterlinemode:mov	eax, dword [bltcon]
		mov	ebx, 0
		push	eax
		push	ebx
		push	ebx
		push	dword [bltcdat]
		push	dword [bltadat]
		push	dword [bltcmod]
		push	dword [bltbmod]
		push	dword [bltamod]
		push	dword [blit_height]

		; Initial line decision

		mov	ecx, eax
		shr	ecx, 6
		and	ecx, 1
		push	ecx

		and	eax, 1ch			; SUL SUD AUL

		; Minterm calculation function

		push	dword [blit_asm_minterm]

		; Prepare ebx, the A shift count

		mov	ebx, dword [blit_a_shift_asc]

		mov	ebp, dword [bltapt]
		mov	esi, dword [bltcpt]	  ; get C data adress
		mov	edi, dword [bltdpt]	  ; get destination adress

		; Prepare ecx, the B data

		mov	edx, dword [bltbdat_original]
		mov	ecx, dword [blit_b_shift_asc]
		ror	dx, cl
		mov	ecx, edx

		; Call routine which performs the line

		jmp	dword [bltlinesulsudaul + eax]



%macro BLITTER_LINEMODE_READ 0
		mov	edx, dword [28 + esp]
		test	dword [40 + esp], 02000000h   ; If C is enabled, read one word from bplcpt (esi)
		jz	near %%l1
		mov	edx, dword [memory_chip - 2 + esi]
		bswap	edx
%%l1:
%endmacro


%macro BLITTER_LINEMODE_PREPARE_A 0
		; Prepare data from channel A
		; It is static data from bltadat, shifted by the current A shift count (ebx)
		push	ebx
		push	ecx
		mov	cl, bl
		mov	ebx, dword [24 + 8 + esp]
		shr	bx, cl
%endmacro

%macro BLITTER_LINEMODE_PREPARE_B 0
		mov	eax,  dword [esp]
		mov	ecx, 0
		test	eax, 1
		jz	%%l1
		dec	ecx
%%l1:
%endmacro

%macro BLITTER_LINEMODE_MAKE_D 0
		; Calculate D
		call	dword [8 + esp]
		pop	ecx
		pop	ebx
%endmacro


%macro BLITTER_LINEMODE_STORE_D 0
		; Store D
		mov	byte [memory_chip + edi], ah		; Store result
		mov	byte [memory_chip + 1 + edi], al
		or	dword [36 + esp], eax
%endmacro


; Label to jump to when there is no increase
%macro BLITTER_LINEMODE_TEST_DECISION 1
		; Line logic
		test	dword [4 + esp], 1
		jz	near %%l1
		; Do not yet increase, has sign		
		mov	eax, 1
		add	bp, word [16 + esp] ; D < 0 , D = D + 2*sdelta
		js	near %%l3
		dec	eax
%%l3:		mov	dword [4 + esp], eax
		jmp	near %1
%%l1:		
		; Increase, no sign
		; D >= 0, D = D + 2*sdelta-2*ldelta
		mov	eax, 1
		add	bp, word [12 + esp]
		js	near %%l4
		dec	eax
%%l4:		mov	dword [4 + esp], eax
%%l2:
%endmacro

%macro BLITTER_LINEMODE_INCREASE_X 0
		inc	ebx
		cmp	ebx, 16
		jne	near %%l1
		xor	ebx, ebx
		add	esi, 2
		add	edi, 2
		and	esi, 01ffffeh
		and	edi, 01ffffeh
%%l1:		
%endmacro

%macro BLITTER_LINEMODE_DECREASE_X 0
		dec	ebx
		jns	near %%l1
		mov	ebx, 15
		sub	esi, 2
		sub	edi, 2
		and	esi, 01ffffeh
		and	edi, 01ffffeh
%%l1:		
%endmacro

%macro BLITTER_LINEMODE_INCREASE_Y 0
		mov	edx, dword [20 + esp]
		add	esi, edx
		add	edi, edx
		and	esi, 01ffffeh
		and	edi, 01ffffeh
%endmacro

%macro BLITTER_LINEMODE_DECREASE_Y 0
		mov	edx, dword [20 + esp]
		sub	esi, edx
		sub	edi, edx
		and	esi, 01ffffeh
		and	edi, 01ffffeh
%endmacro

%macro BLITTER_LINEMODE_ROTATE_B 0
		; Rotate B data to the left 
		rol	cx, 1
%endmacro

; Label of the top of the loop
%macro BLITTER_LINEMODE_ITERATE_LOOP 1
		dec	dword [8 + esp]
		jnz	near %1
		jmp	blitterlinemodeend
%endmacro

%macro BLITTER_LINEMODE_ALLOW_DOT 0
		mov	dword [32 + esp], 0 ; enable dot on this line
%endmacro

; Label of code to jump to when no dot is allowed
%macro BLITTER_LINEMODE_CHECK_SINGLE_DOT 0
		test	dword [40 + 8 + esp], 2	   ; Test sing
		jz	near %%l1
		test	dword [32 + 8 + esp], 1 ; No draw if bit already set
		jnz	near %%l2
		mov	dword [32 + 8 + esp], 1
		jmp	near %%l1
%%l2:		mov	ebx, 0
%%l1:
%endmacro


;===============================================
; SUL/SUD/AUL = 000
; Line goes down to the right, Y is independent
; 
; Input registers:
; ebx - Initial channel A shift count
; ecx - Data for channel B
; esi - bltcpt
; edi - bltdpt
;===============================================


		FALIGN32

global _blitterlinemodeoctant0_
_blitterlinemodeoctant0_:
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_INCREASE_X
.noincrease:	BLITTER_LINEMODE_INCREASE_Y
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;=============================================
; SUL/SUD/AUL = 001
; Line goes up to the right, Y is independent
; 
; Input registers:
; ebx - Initial channel A shift count
; ecx - Data for channel B
; esi - bltcpt
; edi - bltdpt
;=============================================


		FALIGN32

global _blitterlinemodeoctant1_
_blitterlinemodeoctant1_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_INCREASE_X
.noincrease:	BLITTER_LINEMODE_DECREASE_Y
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;===============================================
; SUL/SUD/AUL = 010
; Line goes down, to the left, Y is independent
;===============================================

		FALIGN32

global _blitterlinemodeoctant2_
_blitterlinemodeoctant2_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_DECREASE_X
.noincrease:	BLITTER_LINEMODE_INCREASE_Y
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;============================================
; SUL/SUD/AUL = 011
; Line goes up to the left, Y is independent
;============================================


		FALIGN32

global _blitterlinemodeoctant3_
_blitterlinemodeoctant3_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_DECREASE_X
.noincrease:	BLITTER_LINEMODE_DECREASE_Y
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;===============================================
; SUL/SUD/AUL = 100
; Line goes down to the right, X is independent
;===============================================


		FALIGN32

global _blitterlinemodeoctant4_
_blitterlinemodeoctant4_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_CHECK_SINGLE_DOT
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_INCREASE_Y
		BLITTER_LINEMODE_ALLOW_DOT
.noincrease:	BLITTER_LINEMODE_INCREASE_X
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;==============================================
; SUL/SUD/AUL = 101
; Line goes down to the left, X is independent
;==============================================


		FALIGN32

global _blitterlinemodeoctant5_
_blitterlinemodeoctant5_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_CHECK_SINGLE_DOT
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_INCREASE_Y
		BLITTER_LINEMODE_ALLOW_DOT
.noincrease:	BLITTER_LINEMODE_DECREASE_X
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


;==============================================
; SUL/SUD/AUL = 110
; Line goes up, to the right, X is independent
;==============================================


		FALIGN32

global _blitterlinemodeoctant6_
_blitterlinemodeoctant6_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_CHECK_SINGLE_DOT
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_DECREASE_Y
		BLITTER_LINEMODE_ALLOW_DOT
.noincrease:	BLITTER_LINEMODE_INCREASE_X
		BLITTER_LINEMODE_ITERATE_LOOP .octloop



;===============================================
; SUL/SUD/AUL = 111
; Line goes up to the left, X is independent
;===============================================


		FALIGN32

global _blitterlinemodeoctant7_
_blitterlinemodeoctant7_: 
.octloop:
		BLITTER_LINEMODE_READ
		BLITTER_LINEMODE_PREPARE_A
		BLITTER_LINEMODE_CHECK_SINGLE_DOT
		BLITTER_LINEMODE_PREPARE_B
		BLITTER_LINEMODE_MAKE_D
		BLITTER_LINEMODE_STORE_D
		BLITTER_LINEMODE_ROTATE_B
		BLITTER_LINEMODE_TEST_DECISION .noincrease
		BLITTER_LINEMODE_DECREASE_Y
		BLITTER_LINEMODE_ALLOW_DOT
.noincrease:	BLITTER_LINEMODE_DECREASE_X
		BLITTER_LINEMODE_ITERATE_LOOP .octloop


blitterlinemodeend:
		and	dword [bltcon], 0ffffffbfh
		test	dword [4 + esp], 1
		jz	no_sign
		or	dword [bltcon], 040h
no_sign:
		mov	dword [bltapt], ebp
		mov	dword [bltcpt], esi
		mov	dword [bltdpt], edi
		mov	eax, dword [36 + esp]
		mov	dword [bltzero], eax
		add	esp, 44
		mov	edx, 08040h
		mov	ecx, 0dff09ch
		jmp	wriw


; ========================================================
; Blitter logic functions
; ========================================================

; -------------------------------------
; The general blitter logic function
; Takes A in ebx, B in ecx and C in edx
; Result in eax 
; -------------------------------------


		FALIGN32


global _blit_min_generic_
_blit_min_generic_:
		push	ebp
		push	edi
		xor	ebp, ebp
		mov	edi, dword [bltcon + 2]
		xor	eax, eax
		test	edi, 80h			; Minterm ABC
		jz	blit5
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blit5:		not	edx				; minterm ABc
		test	edi, 40h			   
		jz	blit6
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blit6:		not	edx				; minterm AbC
		not	ecx
		test	edi, 20h			   
		jz	blit7
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blit7:		not	edx				; minterm Abc
		test	edi, 10h			   
		jz	blit8
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blit8:		not	edx				; minterm aBC
		not	ecx
		not	ebx
		test	edi, 8
		jz	blit9
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blit9:		not	edx				; minterm aBc
		test	edi, 4
		jz	blita
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blita:		not	edx				; minterm abC
		not	ecx
		test	edi, 2
		jz	blitb
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blitb:		not	edx				; minterm abc
		test	edi, 1
		jz	blitcc
		mov	ebp, ebx
		and	ebp, ecx
		and	ebp, edx
		or	eax, ebp
blitcc:		not	ebx
		not	ecx
		not	edx
		pop	edi
		pop	ebp
		ret


; -------------------------------------------------
; Optimized logic functions for some common cases
; Must not destroy the parameters
; in:  A - ebx	 B - ecx  C - edx
; out: D - eax 
; -------------------------------------------------


		FALIGN32

; 0

global _blit_min_00_
_blit_min_00_:	xor	eax, eax
		ret


		FALIGN32

; !(A+B+C)

global _blit_min_01_
_blit_min_01_:	mov	eax, ebx
		or	eax, ecx
		or	eax, edx
		not	eax
		ret


		FALIGN32

; abC

global _blit_min_02_
_blit_min_02_:	not	ebx
		mov	eax, edx
		not	ecx
		and	eax, ebx
		not	ebx
		and	eax, ecx
		not	ecx
		ret


		FALIGN32

; !(A+B)

global _blit_min_03_
_blit_min_03_:	mov	eax, ebx
		or	eax, ecx
		not	eax
		ret


		FALIGN32

; aBc

global _blit_min_04_
_blit_min_04_:	not	ebx
		mov	eax, ecx
		not	edx
		and	eax, ebx
		not	ebx
		and	eax, edx
		not	edx
		ret


		FALIGN32

; !(A+C)

global _blit_min_05_
_blit_min_05_:	mov	eax, edx
		or	eax, ebx
		not	eax
		ret


		FALIGN32

; a(B xor C)

global _blit_min_06_
_blit_min_06_:	mov	eax, ebx
		xor	ecx, edx
		not	eax
		and	eax, ecx
		xor	ecx, edx
		ret


		FALIGN32

; !(A+BC)

global _blit_min_07_
_blit_min_07_:	mov	eax, ecx
		and	eax, edx
		or	eax, ebx
		not	eax
		ret


		FALIGN32

; aBC

global _blit_min_08_
_blit_min_08_:	not	ebx
		mov	eax, edx
		and	eax, ebx
		not	ebx
		and	eax, ecx
		ret	 


		FALIGN32

; !(A+(B xor C))

global _blit_min_09_
_blit_min_09_:	mov	eax, ecx
		xor	eax, edx
		or	eax, ebx
		not	eax
		ret


		FALIGN32

; Ca

global _blit_min_0a_
_blit_min_0a_:	mov	eax, ebx
		not	eax
		and	eax, edx
		ret


		FALIGN32

; a(b + C)

global _blit_min_0b_
_blit_min_0b_:	push	ebx
		mov	eax, ecx
		not	eax
		or	eax, edx
		not	ebx
		and	eax, ebx
		pop	ebx
		ret


		FALIGN32

; Ba

global _blit_min_0c_
_blit_min_0c_:	mov	eax, ebx
		not	eax
		and	eax, ecx
		ret


		FALIGN32

; !(A+(bC))

global _blit_min_0d_
_blit_min_0d_:	mov	eax, ecx
		not	eax
		and	eax, edx
		or	eax, ebx
		not	eax
		ret


		FALIGN32

; a(B+C)

global _blit_min_0e_
_blit_min_0e_:	mov	eax, ecx
		not	ebx
		or	eax, edx
		and	eax, ebx
		not	ebx
		ret


		FALIGN32

; a

global _blit_min_0f_
_blit_min_0f_:	mov	eax, ebx
		not	eax
		ret


		FALIGN32

; (!AB)C

global _blit_min_2a_
_blit_min_2a_:	mov	eax, ebx
		and	eax, ecx
		not	eax
		and	eax, edx
		ret


		FALIGN32

; aC + ABc

global _blit_min_4a_
_blit_min_4a_:	push	edx
		mov	eax, ebx
		not	eax
		and	eax, edx
		not	edx
		and	edx, ebx
		and	edx, ecx
		or	eax, edx
		pop	edx
		ret


		FALIGN32

; AB + aC

global _blit_min_ca_
_blit_min_ca_:	push	ebx
		mov	eax, ebx
		not	ebx
		and	eax, ecx
		and	ebx, edx
		or	eax, ebx
		pop	ebx
		ret
		

		FALIGN32

; Ac + BC

global _blit_min_d8_
_blit_min_d8_:	push	ebx
		mov	eax, edx
		not	edx
		and	eax, ecx
		and	ebx, edx
		or	eax, ebx
		not	edx
		pop	ebx
		ret


		FALIGN32

; AB+aC+AbC

global _blit_min_ea_
_blit_min_ea_:	push	ecx
		push	ebx
		mov	eax, ebx
		and	eax, ecx
		not	ecx
		and	ecx, ebx
		and	ecx, edx
		not	ebx
		and	ebx, edx
		or	eax, ebx
		or	eax, ecx
		pop	ebx
		pop	ecx
		ret


		FALIGN32

;A

global _blit_min_f0_
_blit_min_f0_:	mov	eax, ebx
		ret


		FALIGN32

; C + A

global _blit_min_fa_
_blit_min_fa_:	mov	eax, ebx
		or	eax, edx
		ret


		FALIGN32

;A + aB

global _blit_min_fc_
_blit_min_fc_:	mov	eax, ebx
		not	eax
		and	eax, ecx
		or	eax, ebx
		ret


		FALIGN32

;========================================================
; 1
;========================================================


global _blit_min_ff_
_blit_min_ff_:	mov	eax, -1
		ret


; ============================================================================
; Blitter logic ENDS
; ============================================================================


FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND

FASMFILEEND
