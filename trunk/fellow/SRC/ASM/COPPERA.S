;===============================================================================
; Fellow Amiga Emulator      
; Copper Emulation
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; This file implements the copper.
;
;
; Summary of what is in this file:
;
; * Get and Set functions for the copper hardware registers.
; * Functions that deal with start/stop of copper DMA, new frame restart etc.
; * Actual interpretation and execution of one copper operation.
; 
; State of this file:
; * The copper code was written before anything else in Fellow and it has
;   survived throughout the entire project. Not because this code
;   is excellent, it is not, but in lack of a decent replacement, this
;   works as good as anything.
; * This code has one problem, it can only approximate waiting positions.
;   The error is minimal for most programs, but in some rare cases the error
;   is quite large, depending on what kind of masks the copper must consider.
;   Since Fellow only samples graphics registers at the end of a line,
;   this has seldom been a problem. Additionally, the effect has been minimized
;   by letting the copper run ahead by a couple of cycles.
;   I have tried to come up with a table-method that would accurately
;   calculate waiting positions in constant time,
;   but given up on it since tables would be unreasonably large.
;   So the code has stayed since it works "well".
;   Maybe there is another way? (Besides brute force, running a loop over all
;   possible waiting positions to check for the next match?)
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "generic/defs.inc"

FASMFILESTART
FDATASECTIONSTART

%include "data/copper.inc"
%include "data/fmem.inc"
%include "data/bus.inc"
%include "data/blit.inc"
%include "data/cpu.inc"
%include "data/graph.inc"

FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _copperstart_
_copperstart_:			; Used to check code alignment

%include "func/bus.inc"
%include "mac/bus.mac"

%macro CPU_LOAD_ADD 0
		cmp	dword [cpu_next], -1
		je	%%nocpuload
		add	dword [cpu_next], 2
%%nocpuload:
%endmacro


;========
; COPCON
;========
; $dff02e

		FALIGN32

global _wcopcon_
_wcopcon_:	mov	dword [copcon], edx
		ret


;=======
; COP1LC
;=======

; $dff080 - WORD


		FALIGN32

global _wcop1lch_
_wcop1lch_:	and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [cop1lc + 2], dx
		ret


; $dff082 - WORD


		FALIGN32

global _wcop1lcl_
_wcop1lcl_:	and	edx, 0fffffffeh		    ; No odd adresses
		mov	word [cop1lc],dx
		ret


;=======
; COP2LC
;=======

; $dff084 - WORD


		FALIGN32

global _wcop2lch_
_wcop2lch_:	and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [cop2lc + 2], dx
		ret


; $dff086 - WORD


		FALIGN32

global _wcop2lcl_
_wcop2lcl_:	and	edx, 0fffffffeh		    ; No odd adresses
		mov	word [cop2lc], dx
		ret


;========
; COPJMP1
;========
; $dff088 - word


		FALIGN32

global _wcopjmp1_
_wcopjmp1_:	jmp    _copperLoad1_


		FALIGN32

global _rcopjmp1_
_rcopjmp1_:	call   _copperLoad1_
		xor    edx, edx
		ret

;========
; COPJMP2
;========
; $dff08A - word


		FALIGN32

global _wcopjmp2_
_wcopjmp2_:	jmp    _copperLoad2_
	

		FALIGN32

global _rcopjmp2_
_rcopjmp2_:	call   _copperLoad2_
		xor    edx, edx
		ret


;-------------------------------------------------------------------------------
; void copperUpdateDMA(void);
; Called by wdmacon every time that register is written
; This routine takes action when the copper DMA state is changed
;-------------------------------------------------------------------------------


		FALIGN32

global _copperUpdateDMA_
_copperUpdateDMA_:
		test	byte [copper_dma], 1
		jz	.copper_updatedma_wasoff

		; Here copper was on, test if it is still on.

		test	byte [dmacon], 080h
		jnz	.copper_updatedma_stillon

		; Here copper DMA is being turned off
	        ; Record which cycle the copper was waiting for
		; Remove copper from the event list

		push	edx
		mov	edx, dword [copper_next]
		mov	dword [copper_suspended_wait], edx
		mov	dword [copper_next], -1
		mov	byte [copper_dma], 0
		push	eax
		push	ebx
		SCAN_EVENTS_LVL2
		pop	ebx
		pop	eax
		pop	edx
.copper_updatedma_stillon:
		ret


		FALIGN32

.copper_updatedma_wasoff:	; Here copper was off, test if it is still off

		test	byte [dmacon], 080h
		jz	.copper_updatedma_stilloff

		; Here copper is being turned on
	        ; Reactivate the cycle the copper was waiting for the last time
		; it was on.
                ; If we have passed it in the mean-time, execute immediately.

		push	edx
		mov	edx, dword [copper_suspended_wait]
		cmp	edx, -1
		je	.copper_updatedma_hanging
		cmp	edx, dword [curcycle]
		ja	.copper_updatedma_hanging
		mov	edx, dword [curcycle]
		add	edx, 4
.copper_updatedma_hanging:
		mov	dword [copper_next], edx
		push	eax
		push	ebx
		SCAN_EVENTS_LVL2
		pop	ebx
		pop	eax
		pop	edx
		mov	byte [copper_dma], 1
.copper_updatedma_stilloff:
		ret


;-------------------------------------------------------------------------------
; Called every end of frame, it restarts the copper
;-------------------------------------------------------------------------------


		FALIGN32

global _copperEndOfFrame_
_copperEndOfFrame_:
		mov	edx, dword [cop1lc]
		mov	dword [copper_ptr], edx
		mov	dword [copper_suspended_wait], 40
		test	byte [copper_dma], 1
		jz	.copper_restart_not
		mov	dword [copper_next], 40
		push	eax
		push	ebx
		SCAN_EVENTS_LVL2
		pop	ebx
		pop	eax
.copper_restart_not:
		ret


;-------------------------------------------------------------------------------
; Initializes the copper from ptr 1
;-------------------------------------------------------------------------------


		FALIGN32

global _copperLoad1_
_copperLoad1_:	mov	edx, dword [cop1lc]
		mov	dword [copper_ptr], edx
		test	byte [copper_dma], 1
		jz	.copper_load1_not
		push	edx
		mov	edx,dword [curcycle]
		add	edx, 4
		mov	dword [copper_next], edx
		push	eax
		push	ebx
		SCAN_EVENTS_LVL2
		pop	ebx
		pop	eax
		pop	edx
.copper_load1_not:
		ret


;-------------------------------------------------------------------------------
; Initializes the copper from ptr 2
;-------------------------------------------------------------------------------


		FALIGN32

global _copperLoad2_
_copperLoad2_:	mov	edx, dword [cop2lc]
		mov	dword [copper_ptr], edx
		test	byte [copper_dma], 1
		jz	.copper_load2_not
		push	edx
		mov	edx, dword [curcycle]
		add	edx, 4
		mov	dword [copper_next], edx
		push	eax
		push	ebx
		SCAN_EVENTS_LVL2
		pop	ebx
		pop	eax
		pop	edx
.copper_load2_not:
		ret


;-------------------------------------------------------------------------------
; Emulates one copper instruction
;-------------------------------------------------------------------------------


		FALIGN32

global _copperEmulate_
_copperEmulate_:
  		push	ebx
		push	ecx
		push	edx
		CPU_LOAD_ADD
		mov	ecx, dword [copper_ptr]
		mov	edx, dword [memory_chip + ecx]
		bswap	edx
		add	ecx, 4
		and	ecx, 01ffffeh
		mov	dword [copper_ptr], ecx
		cmp	edx, 0fffffffeh
		je	near .notokay
		mov	ecx, edx
		and	edx, 0ffffh
		shr	ecx, 16
		test	ecx, 01h				 ; Zero is MOVE
		jnz	near .cwait
		and	ecx, 000001feh			; Move (This works)
		mov	ebx, dword [bplcon0]
		shr	ebx, 12
		and	ebx, 0fh
		mov	ebx, dword [copper_cycletable + 4*ebx]
		add	ebx, dword [curcycle]
		mov	dword [copper_next], ebx
		SCAN_EVENTS_LVL2
		cmp	ecx, 80h
		jae	.okay
		test	dword [copcon], 0ffffh     ; Access to $40-$7e?
		jz	.notokay
		cmp	ecx, 40h
		jb	.notokay
.okay:		call	dword [memory_iobank_write + ecx*2]
		pop	edx
		pop	ecx
		pop	ebx
		ret
.notokay:	mov	dword [copper_next], -1
		SCAN_EVENTS_LVL2
		pop	edx
		pop	ecx
		pop	ebx
		ret
	

		FALIGN32

.cwait:		and	ecx, 0fffeh
		test	edx, 08000h
		jnz	.cnoblit
		test	byte [blitterstatus], 0ffh
		jz	.cnoblit
		sub	dword [copper_ptr], 4
		mov	ebx, dword [blitend]
		add	ebx, 4
		cmp	ebx, dword [curcycle]
		ja	near .cwaitexit
		mov	ebx, dword [curcycle]
		add	ebx, 4
		jmp	.cwaitexit
.cnoblit:	test	edx, 01h		; Zero is Wait (Works??)
		jnz	near .cskip			;	(Not really!)

	
	
		; Calculate our line, masked with vmask
		; bl - masked graph_raster_y 

		mov	ebx, dword [graph_raster_y]
		or	edx, 00008000h		; use mask on graph_raster_y
		xor	ebp, ebp		; Used to indicate
					; correct line or not
		and	bl, dh
		cmp	bl, ch		; Compare masked y with wait line.
		ja	near .cexit		; We have passed the line, set up
					; next instruction immediately.
		jb	.notnever	; We are above the line, calculate
					; the cycle when wait is true.

		; Here we are on the correct line

		; Calculate our xposition, masked with hmask
		; al - masked graph_raster_x
	

		mov	eax, dword [graph_raster_x]
		inc	ebp
		and	al, dl			; Use mask on graph_raster_x
		cmp	al, cl			; Compare masked x with wait x
		jae	near .cexit			; Position reached, set up
						; next instruction immediately.

		; Here the wait position is not reached yet, calculate
		; cycle when wait is true.
		; Previous position checks should assure that
		; a calculated position is not less than the current cycle.

.notnever:	not	edx			; Invert masks
		mov	ebx, dword [graph_raster_y]
		and	bl, dh		; Get bits that is not masked out
		or	bl, ch		; Mask in bits from waitgraph_raster_y
		mov	ebx, dword [copper_ytable + ebx*4]
	
		; When wait is on same line, use masking stuff on graph_raster_x,
		; else on graph_raster_x = 0

		and	ecx, 0feh		; Prepare waitxpos
		and	edx, 0feh
		xor	eax, eax
		test	ebp, ebp
		jz	.copwaitnotsameline
		cmp	edx, dword [graph_raster_x]
		jae	.copwaitnotsameline
		mov	eax, dword [graph_raster_x]
.copwaitnotsameline:
		and	eax, edx	; Get unmasked bits from current x
		or	ecx, eax		; Mask in horizontal waitpos
		add	ebx, ecx
		add	ebx, 4
.cwaitexit:	mov	dword [copper_next], ebx
		SCAN_EVENTS_LVL2
		pop	edx
		pop	ecx
		pop	ebx
		ret
	


		FALIGN32

; to help the copper to wait line 256, do some tests here
; Here we have detected a wait position that we are past.
; Check if current line is 255, then if line we are supposed
; to wait for are less than 60, do the wait afterall.

; Here we have detected a wait that has been passed.

.cexit:
		cmp	dword [graph_raster_y], 255
		jne	near .ctrueexit	; Do some tests if line is 255
		cmp	ch, 40h		; Test line to be waited for
		ja	near .ctrueexit	; Line is 256-313, wrong to not wait


		; Better to do the wait afterall.
		; Here we recalculate the wait stuff,

		not	edx			; Invert masks
		mov	ebx, dword [graph_raster_y]
		or	ebx, 0100h
		and	bl, dh			; Get missing bits
		or	bl, ch			; Mask them into vertical position
		imul	ebx, CYCLESPERLINE
		and	ecx, 0feh
		mov	eax, dword [graph_raster_x]
		and	edx, 0feh
		and	eax, edx
		or	ecx, eax			; Mask in horizontal
		add	ecx, ebx
		add	ecx, 4
		cmp	ecx, dword [curcycle]
		ja	.cnofix
		mov	ecx, dword [curcycle]
		add	ecx, 4
.cnofix:
.fluks:		mov	dword [copper_next], ecx
		SCAN_EVENTS_LVL2
		pop	edx
		pop	ecx
		pop	ebx
		ret
	

		FALIGN32
; Here we must do a new copper access immediately (in 4 cycles)
.ctrueexit:	mov	ebx, dword [curcycle]
		add	ebx, 4
		mov	dword [copper_next], ebx
		SCAN_EVENTS_LVL2
		pop	edx
		pop	ecx
		pop	ebx
		ret
	
; New skip, copied from cwait just to try something

		FALIGN32
.cskip: 


		; Calculate our line, masked with vmask
		; bl - masked graph_raster_y 

		mov	ebx, dword [graph_raster_y]
		or	edx, 00008000h	; use mask on graph_raster_y
		xor	ebp, ebp	; Used to indicate
					; correct line or not
		and	bl, dh
		cmp	bl, ch		; Compare masked y with wait line.
		ja	.cdoskip	; We have passed the line, set up
					; next instruction immediately.
		jb	.cdontskip	; Above line, don't skip

		; Here we are on the correct line

		; Calculate our xposition, masked with hmask
		; al - masked graph_raster_x
	

		mov	eax, dword [graph_raster_x]
		inc	ebp
		and	al, dl			; Use mask on graph_raster_x
		cmp	al, cl			; Compare masked x with wait x
		jb	.cdontskip		; Position reached, set up
					; next instruction immediately.
.cdoskip:	add	dword [copper_ptr], 4
.cdontskip:	mov	ebx, dword [curcycle]
		add	ebx, 4
		mov	dword [copper_next], ebx
		SCAN_EVENTS_LVL2
		pop	edx
		pop	ecx
		pop	ebx
		ret

		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND


