;===============================================================================
; Fellow Amiga Emulator      
; Blitter (OCS) Emulation
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
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
%include "generic/defs.inc"

FASMFILESTART
FDATASECTIONSTART

%include "data/blit.inc"
%include "data/bus.inc"
%include "data/copper.inc"
%include "data/fmem.inc"
%include "data/cpu.inc"

FDATASECTIONEND
FCODESECTIONSTART
FALIGNHELPER

global _blitstart_
_blitstart_:				; Used to check code alignment

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


		FALIGN32

global _wbltcon0_
_wbltcon0_:	BLIT_FINISH
		mov	word [bltcon + 2], dx
		and	edx, 0ffh
		mov	edx, dword [blit_min_functable + 4*edx]
		mov	dword [bltminterm], edx
		ret


;=========
; BLTCON0L
;=========
; $dff05A


		FALIGN32

global _wbltcon0l_
_wbltcon0l_:	BLIT_FINISH
		mov	byte [bltcon + 2], dl
		and	edx, 0ffh
		mov	edx, dword [blit_min_functable + 4*edx]
		mov	dword [bltminterm], edx
		ret


;========
; BLTCON1
;========
; $dff042


		FALIGN32

global _wbltcon1_
_wbltcon1_:	BLIT_FINISH
		mov	dword [bltdesc], 2
		test	edx, 2				; Test descending mode
		jz	blits1
		mov	dword [bltdesc], -2
blits1:		mov	ecx, dword [bltcon]
		mov	word [bltcon], dx
		test	edx, 1				; Line mode?
		jnz	bconlines
		test	ecx, 1		; Going from linemode to desc mode?
		jz	bconnoprevlines

		; Here last mode was linemode, but new isn't
		; Mods need changing if new is desc mode

		test	edx, 2
		jz	bcon		; New is normal, mods already ok
		jmp	modrestore

bconnoprevlines:and	edx, 2
		and	ecx, 2
		cmp	edx, ecx
		je	bcon
modrestore:	neg	dword [bltamod]
		neg	dword [bltbmod]
		neg	dword [bltcmod]
		neg	dword [bltdmod]
bcon:		ret
bconlines:	test	ecx, 1	; Restore unless previous mode was linemode
		jnz	bcon
		test	ecx, 2
		jz	bcon
		jmp	modrestore


;========
; BLTAFWM
;========
; $dff044


		FALIGN32

global _wbltafwm_
_wbltafwm_:	BLIT_FINISH
		mov	word [bltafwm], dx
		mov	word [bltafwm + 2], dx
		ret


;========
; BLTALWM
;========
; $dff046


		FALIGN32

global _wbltalwm_
_wbltalwm_:	BLIT_FINISH
		mov	word [bltalwm], dx
		mov	word [bltalwm + 2], dx
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
		and	edx, 0fffffffeh
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
		and	edx, 0fffffffeh
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
		and	edx, 0fffffffeh
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
		and	edx, 0fffffffeh
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
		jnz	wbltsizea
		mov	ecx, 64
wbltsizea:	mov	dword [linelength], ecx
		shr	edx, 6			; Height
		and	edx, 3ffh
		jnz	wbltsizeb
		mov	edx, 1024
wbltsizeb:	mov	dword [linenum], edx
		mov	dword [linecount], edx
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
		jnz	wbltsizva
		mov	edx, 08000h
wbltsizva:	mov	dword [linenum], edx
		ret


;========
; BLTSIZH
;========
; $dff05E


		FALIGN32

global _wbltsizh_
_wbltsizh_:	BLIT_FINISH
		and	edx, 07ffh		; Width
		jnz	wbltsizha
		mov	edx, 0800h
wbltsizha:	mov	dword [linelength], edx
		mov	edx, dword [linenum]
		mov	dword [linecount], edx
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
		test	byte [bltcon], 1
		jnz	bcn
		test	byte [bltcon], 2
		jz	bcn
		neg	edx
bcn:		mov	dword [bltcmod], edx
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
		test	byte [bltcon], 1
		jnz	bbn
		test	byte [bltcon], 2
		jz	bbn
		neg	edx
bbn:		mov	dword [bltbmod], edx
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
		test	byte [bltcon], 1
		jnz	ban
		test	byte [bltcon], 2
		jz	ban
		neg	edx
ban:		mov	dword [bltamod], edx
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
		test	byte [bltcon], 1
		jnz	bdn
		test	byte [bltcon], 2
		jz	bdn
		neg	edx
bdn:		mov	dword [bltdmod], edx
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
		mov	dword [bltbdatoriginal], edx
		mov	ecx, dword [bltcon]
		shr	ecx, 12
		and	ecx, 0fh
		test	dword [bltcon], 2
		jnz	wbltbdatb
		shr	dx, cl
		and	edx, 0ffffh
		mov	dword [bltbdat], edx
		ret
wbltbdatb:	shl	dx, cl
		and	edx, 0ffffh
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
		mov	dword [bltadatoriginal], edx
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
		imul	ebx, dword [linelength]	; cycles per line
		imul	ebx, dword [linenum]	; total cyclecount
fastblit:	mov	eax, 1				; State 1, no bltpri
		mov	dword [thiscycle], 0
		test	dword [dmaconr], 0400h	; test bltpri
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

	
		FALIGN32

global _finish_blit_
_finish_blit_:	mov	dword [blitend], -1
		SCAN_EVENTS_LVL4
		mov	dword [blitterdmawaiting], 0
		mov	dword [blitterstatus], 0
		and	dword [dmaconr], 0bfffh	
		xor	eax, eax
		mov	dword [blit_leftoverA], eax
		mov	dword [blit_leftoverB], eax
		mov	ebx, dword [bltcon]
		test	ebx, 1
		jnz	blitterlinemode
		test	ebx, 18h
		jnz	near blitterfillmode
		shr	ebx, 22
		and	ebx, 03ch
		jmp	dword [blittermodes + ebx]


; ==============================
; Blitter line mode entry point
; ==============================


		FALIGN32


blitterlinemode:mov	dword [bltlinepointflag], 0
		mov	eax, dword [linecount]
		mov	dword [bltlineheight], eax	; Line height
		mov	eax, dword [bltcon]
		mov	ecx, eax
		shr	ecx, 28
		and	ecx, 00fh
		and	eax, 1ch
		mov	ebx, ecx
		mov	bp, word [bltapt]	; edi is decision variable
		mov	esi, dword [bltcpt]	  ; get C data adress
		mov	edi, dword [bltdpt]	  ; get destination adress
		add	esi, dword memory_chip
		add	edi, dword memory_chip
		sub	esi, 2
		mov	edx, dword [bltbdatoriginal]
		mov	ecx, dword [bltcon]
		shr	ecx, 12
		and	ecx, 0fh
		ror	dx, cl
		mov	dword [bltbdatline], edx
		xor	ecx, ecx
		test	edx, 1
		jz	bliq
		mov	ecx, -1
bliq:		jmp	dword [bltlinesudsulaul + eax]
		; Jump to correct draw routine


;===============================================
; SUD/SUL/AUL = 000
; Line goes down to the right, Y is independent
;
; Registers:
; ebp - D
; esi - cpt
; edi - dpt
; ebx - bltadat
; ecx - bltbdat
; edx - bltcdat
; eax - result
;===============================================


		FALIGN32

global _blitterlinemodeoctant0_
_blitterlinemodeoctant0_:
oct0c:		test	dword [bltcon], 02000000h
		jz	oct0u
		mov	edx, dword [esi]
		bswap	edx
oct0u:		push	ebx
		push	ecx
		mov	cl, bl
		mov	ebx, dword [bltadat]
		shr	bx, cl
		pop	ecx
		call	dword [bltminterm]
		pop	ebx
		mov	byte [edi], ah		; Store result
		mov	byte [1 + edi], al
		or	dword [bltzero], eax
		mov	eax, dword [bltcmod]
		or	bp, bp				; D 
		jns	oct0a
		add	ebp, dword [bltbmod] ; D < 0 , D = D + 2*sdelta
		jmp	oct0b
oct0a:		add	ebp, dword [bltamod]
		; D >= 0, D = D + 2*sdelta-2*ldelta
		inc	ebx
		cmp	ebx, 16
		jne	oct0b
		xor	ebx, ebx
		add	esi, 2
		add	edi, 2
oct0b:		add	esi, eax
		add	edi, eax
		mov	eax, dword [bltbdatline]
		rol	ax, 1
		mov	dword [bltbdatline], eax
		xor	ecx, ecx
		test	eax, 1
		jz	oct0q
		dec	ecx
oct0q:		dec	dword [bltlineheight]
		jnz	near oct0c
		jmp	blitterlinemodeend


;=============================================
; SUD/SUL/AUL = 001
; Line goes up to the right, Y is independent
;=============================================


		FALIGN32

global _blitterlinemodeoctant1_
_blitterlinemodeoctant1_: 
oct1c:		test	dword [bltcon], 02000000h
		jz	oct1u
		mov	edx, dword [esi]
		bswap	edx
oct1u:		push	ebx
		push	ecx
		mov	cl, bl
		mov	ebx, dword [bltadat]
		shr	bx, cl
		pop	ecx
		call	dword [bltminterm]
		pop	ebx
		mov	byte [edi], ah	; Store result
		mov	byte [1 + edi], al
		or	dword [bltzero], eax
		mov	eax, dword [bltcmod]
		or	bp, bp			; D < 0
		jns	oct1a		 
		add	ebp, dword [bltbmod]; D < 0 , D = D + 2*sdelta
		jmp	oct1b
oct1a:		add	ebp, dword [bltamod]; D >= 0, D=D+2*sdelta-2*ldelta
		inc	ebx
		cmp	ebx, 16
		jne	oct1b
		xor	ebx, ebx
		add	edi, 2
		add	esi, 2
oct1b:		sub	edi, eax
		sub	esi, eax
		mov	eax, dword [bltbdatline]
		rol	ax, 1
		mov	dword [bltbdatline], eax
		xor	ecx, ecx
		test	eax, 1
		jz	oct1q
		dec	ecx
oct1q:		dec	dword [bltlineheight]
		jnz	near oct1c
		jmp	blitterlinemodeend


;===============================================
; SUD/SUL/AUL = 010
; Line goes down, to the left, Y is independent
;===============================================


		FALIGN32

global _blitterlinemodeoctant2_
_blitterlinemodeoctant2_: 
oct2c:		test	dword [bltcon], 02000000h
		jz	oct2u
		mov	edx, dword [esi]
		bswap	edx
oct2u:		push	ebx
		push	ecx
		mov	cl, bl
		mov	ebx, dword [bltadat]
		shr	bx, cl
		pop	ecx
		call	dword [bltminterm]
		pop	ebx
		mov	byte [edi], ah    ; Store result
		mov	byte [1 + edi], al
		or	dword [bltzero], eax
		mov	eax, dword [bltcmod]
		or	bp, bp			; D < 0
		jns	oct2a		 
		add	ebp, dword [bltbmod]; D < 0 , D = D + 2*sdelta
		jmp	oct2b
oct2a:		add	ebp, dword [bltamod]; D >= 0, D=D+2*sdelta-2*ldelta
		dec	ebx
		jns	oct2b
		mov	ebx, 15
		sub	edi, 2
		sub	esi, 2
oct2b:		add	edi, eax
		add	esi, eax
		mov	eax, dword [bltbdatline]
		rol	ax, 1
		mov	dword [bltbdatline], eax
		xor	ecx, ecx
		test	eax, 1
		jz	oct2q
		dec	ecx
oct2q:		dec	dword [bltlineheight]
		jnz	near oct2c
		jmp	blitterlinemodeend


;============================================
; SUD/SUL/AUL = 011
; Line goes up to the left, Y is independent
;============================================


		FALIGN32

global _blitterlinemodeoctant3_
_blitterlinemodeoctant3_: 
oct3c:

		test	dword [bltcon],02000000h
		jz	oct3u
		mov	edx,dword [esi]
		bswap	edx
oct3u:

		push	ebx
		push	ecx
		mov	cl,bl
		mov	ebx,dword [bltadat]
		shr	bx,cl
		pop	ecx


		call	dword [bltminterm]

		pop	ebx

		mov	byte [edi],ah    ; Store result
		mov	byte [1 + edi],al
		or	dword [bltzero],eax
		mov	eax,dword [bltcmod]
		or	bp,bp			  ; D < 0
		jns	oct3a		 
		add	ebp,dword [bltbmod]	   ; D < 0 , D = D + 2*sdelta
		jmp	oct3b
oct3a:		add	ebp,dword [bltamod]	   ; D >= 0, D = D + 2*sdelta-2*ldelta


		dec	ebx
		jns	oct3b
		mov	ebx,15

		sub	edi,2
		sub	esi,2
oct3b:		sub	edi,eax
		sub	esi,eax

		mov	eax,dword [bltbdatline]
		rol	ax,1
		mov	dword [bltbdatline],eax
		xor	ecx,ecx
		test	eax,1
		jz	oct3q
		dec	ecx
oct3q:

		dec	dword [bltlineheight]
		jnz	near oct3c
		jmp	blitterlinemodeend


;===============================================
; SUD/SUL/AUL = 100
; Line goes down to the right, X is independent
;===============================================


		FALIGN32

global _blitterlinemodeoctant4_
_blitterlinemodeoctant4_: 
oct4c:

		test	dword [bltcon],02000000h
		jz	oct4u
		mov	edx,dword [esi]
		bswap	edx
oct4u:

		test	dword [bltcon],2	   ; Test sing
		jz	oct4e
		test	dword [bltlinepointflag],1 ; No draw if bit already set
		jnz	oct4f
		mov	dword [bltlinepointflag],1
oct4e:

		push	ebx
		push	ecx
		mov	cl,bl
		mov	ebx,dword [bltadat]
		shr	bx,cl
		pop	ecx


		call	dword [bltminterm]

		pop	ebx

		mov	byte [edi],ah    ; Store result
		mov	byte [1 + edi],al
		or	dword [bltzero],eax
oct4f:		or	bp,bp			  ; D < 0
		jns	oct4a		 
		add	ebp,dword [bltbmod]	   ; D < 0 , D = D + 2*sdelta
		jmp	oct4b
oct4a:		mov	eax,dword [bltcmod]	   ; Y increases
		add	ebp,dword [bltamod]	   ; D >= 0, D = D + 2*sdelta-2*ldelta
		add	edi,eax
		add	esi,eax
		mov	dword [bltlinepointflag],0 ; enable dot on this line
oct4b:
		inc	ebx
		cmp	ebx,16
		jne	oct4d
		xor	ebx,ebx
		add	edi,2
		add	esi,2
oct4d:


		mov	eax,dword [bltbdatline]
		rol	ax,1
		mov	dword [bltbdatline],eax
		xor	ecx,ecx
		test	eax,1
		jz	oct4q
		dec	ecx
oct4q:

		dec	dword [bltlineheight]
		jnz	near oct4c
		jmp	blitterlinemodeend


;==============================================
; SUD/SUL/AUL = 101
; Line goes down to the left, X is independent
;==============================================


		FALIGN32

global _blitterlinemodeoctant5_
_blitterlinemodeoctant5_: 
oct5c:
		test	dword [bltcon],02000000h
		jz	oct5u
		mov	edx,dword [esi]
		bswap	edx
oct5u:
		test	dword [bltcon],2
		jz	oct5e
		test	dword [bltlinepointflag],1
		jnz	oct5f
		mov	dword [bltlinepointflag],1
oct5e:

		push	ebx
		push	ecx
		mov	cl,bl
		mov	ebx,dword [bltadat]
		shr	bx,cl
		pop	ecx


		call	dword [bltminterm]

		pop	ebx

		mov	byte [edi],ah    ; Store result
		mov	byte [1 + edi],al
		or	dword [bltzero],eax
oct5f:		or	bp,bp			  ; D < 0
		jns	oct5a		 
		add	ebp,dword [bltbmod]	   ; D < 0 , D = D + 2*sdelta
		jmp	oct5b
oct5a:		mov	eax,dword [bltcmod]	   ; Y increases
		add	ebp,dword [bltamod]	   ; D >= 0, D = D + 2*sdelta-2*ldelta
		add	edi,eax
		add	esi,eax
		mov	dword [bltlinepointflag],0
oct5b:
		dec	ebx
		jns	oct5d
		mov	ebx,15

		sub	edi,2
		sub	esi,2
oct5d:
		mov	eax,dword [bltbdatline]
		rol	ax,1
		mov	dword [bltbdatline],eax
		xor	ecx,ecx
		test	eax,1
		jz	oct5q
		dec	ecx
oct5q:


		dec	dword [bltlineheight]
		jnz	near oct5c
		jmp	blitterlinemodeend



;==============================================
; SUD/SUL/AUL = 110
; Line goes up, to the right, X is independent
;==============================================


		FALIGN32

global _blitterlinemodeoctant6_
_blitterlinemodeoctant6_: 
oct6c:
		test	dword [bltcon],02000000h
		jz	oct6u
		mov	edx,dword [esi]
		bswap	edx
oct6u:

		test	dword [bltcon],2
		jz	oct6e
		test	dword [bltlinepointflag],1
		jnz	oct6f
		mov	dword [bltlinepointflag],1
oct6e:
		push	ebx
		push	ecx
		mov	cl,bl
		mov	ebx,dword [bltadat]
		shr	bx,cl
		pop	ecx


		call	dword [bltminterm]

		pop	ebx
	
		mov	byte [edi],ah    ; Store result
		mov	byte [1 + edi],al
		or	dword [bltzero],eax
oct6f:		or	bp,bp			  ; D < 0
		jns	oct6a		 
		add	ebp,dword [bltbmod]	   ; D < 0 , D = D + 2*sdelta
		jmp	oct6b
oct6a:		mov	eax,dword [bltcmod]	   ; Y decreases
		add	ebp,dword [bltamod]	   ; D >= 0, D = D + 2*sdelta-2*ldelta
		sub	edi,eax
		sub	esi,eax
		mov	dword [bltlinepointflag],0
oct6b:
		inc	ebx
		cmp	ebx,16
		jne	oct6d
		xor	ebx,ebx
		add	edi,2
		add	esi,2
oct6d:
		mov	eax,dword [bltbdatline]
		rol	ax,1
		mov	dword [bltbdatline],eax
		xor	ecx,ecx
		test	eax,1
		jz	oct6q
		dec	ecx
oct6q:
		dec	dword [bltlineheight]
		jnz	near oct6c
		jmp	blitterlinemodeend


;===============================================
; SUD/SUL/AUL = 111
; Line goes up to the left, X is independent
;===============================================


		FALIGN32

global _blitterlinemodeoctant7_
_blitterlinemodeoctant7_: 
oct7c:

		test	dword [bltcon],02000000h
		jz	oct7u
		mov	edx,dword [esi]
		bswap	edx
oct7u:
		test	dword [bltcon],2
		jz	oct7e
		test	dword [bltlinepointflag],1
		jnz	oct7f
		mov	dword [bltlinepointflag],1
oct7e:
		push	ebx
		push	ecx
		mov	cl,bl
		mov	ebx,dword [bltadat]
		shr	bx,cl
		pop	ecx


		call	dword [bltminterm]

		pop	ebx

		mov	byte [edi],ah    ; Store result
		mov	byte [1 + edi],al
		or	dword [bltzero],eax
oct7f:		or	bp,bp			  ; D < 0
		jns	oct7a		 
		add	ebp,dword [bltbmod]	   ; D < 0 , D = D + 2*sdelta
		jmp	oct7b
oct7a:		mov	eax,dword [bltcmod]	   ; Y decreases
		add	ebp,dword [bltamod]	   ; D >= 0, D = D + 2*sdelta-2*ldelta
		sub	edi,eax
		sub	esi,eax
		mov	dword [bltlinepointflag],0
oct7b:
		dec	ebx
		jns	oct7d
		mov	ebx,15

		sub	edi,2
		sub	esi,2
oct7d:

		mov	eax,dword [bltbdatline]
		rol	ax,1
		mov	dword [bltbdatline],eax
		xor	ecx,ecx
		test	eax,1
		jz	oct7q
		dec	ecx
oct7q:

		dec	dword [bltlineheight]
		jnz	near oct7c

blitterlinemodeend:
		sub	esi,dword memory_chip
		sub	edi,dword memory_chip
		add	esi,2
		mov	dword [bltdpt],edi
		mov	dword [bltcpt],esi
		mov	edx,08040h
		mov	ecx,0dff09ch
		call	wriw
		ret




; =================
; Blitter fill mode
; =================


		FALIGN32


; Jump to fill routine

blitterfillmode:shr	ebx,22
		and	ebx,03ch
		jmp	dword [blitterfillmodes + ebx]


		FALIGN32


; General fill routine that handles any combination of ABCD


global _blitterfillline_
_blitterfillline_:
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		test	eax,8
		jz	near blitterexfillline

blitterincfillline:
		mov	esi,dword [linelength]
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		mov	ebp,dword [bltdesc]
blitincfillword:
blitincfill1:
		test	eax,08000000h			; Test dma A
		jz	blitincfill2
		mov	ecx,dword [bltapt]
		mov	edx,dword [memory_chip+ecx]
		bswap	edx				; Now, bits 31-16
		cmp	esi,dword [linelength]	; Do first word masking
		jne	blitincfilld
		and	edx,dword [bltafwm]
blitincfilld:
		cmp	esi,1				; Last mask + modulo
		jne	blitincfillf
		mov	ebx,dword [bltamod]
		add	ecx,ebx
		and	edx,dword [bltalwm]
blitincfillf:
		add	ecx,ebp				; Add +2 or -2 to ptA
		and	edx,0ffff0000h			; Take away garbage
		and	ecx,01ffffeh
		mov	dword [bltapt],ecx
		mov	ecx,eax				; Saved bltcon to ecx
		shr	ecx,28				; Exctract shift for A

		test	eax,2				; Test desc mode
		jz	blitincnofdesc1
		rol	edx,cl				; To left in desc mode
		jmp	blitincnofdesc2
blitincnofdesc1:
		shr	edx,cl				; Right in forward mode
blitincnofdesc2:
		mov	ecx,dword [blit_leftoverA]	; get last leftover
		mov	dword [blit_leftoverA],edx	; Store leftover
		shr	edx,16
		or	edx,ecx				; Merge with leftover
		mov	dword [bltadat],edx	; Store data from A
		jmp	blitincfill2x

		; DMA A off
blitincfill2:
		mov	edx,dword [bltadatoriginal]
		shl	edx,16
		cmp	esi,dword [linelength]	; First word masking
		jne	bifd
		and	edx,dword [bltafwm]
bifd:
		cmp	esi,1				; Last mask + modulo
		jne	biff
		mov	ebx,dword [bltamod]
		add	ecx,ebx
		and	edx,dword [bltalwm]
biff:
		mov	ecx,eax				; Saved bltcon to ecx
		shr	ecx,28				; Exctract shift for A

		test	eax,2				; Test desc mode
		jz	binofdesc1
		rol	edx,cl
		jmp	binofdesc2
binofdesc1:
		shr	edx,cl
binofdesc2:
		mov	ecx,dword [blit_leftoverA]	; get last leftover
		mov	dword [blit_leftoverA],edx	; Store leftover from A
		shr	edx,16
		or	edx,ecx
		mov	dword [bltadat],edx	; Store data from A
blitincfill2x:
		test	eax,04000000h		   ; Test dma B
		jz	blitincfill3
		mov	ecx,dword [bltbpt]
		mov	edx,dword [memory_chip-2+ecx]
		bswap	edx			   ; My data in lo word
		cmp	esi,1			   ; Do modulo if last word on line 
		jne	blitincfillg
		mov	ebx,dword [bltbmod]	  ; BLTBMOD
		add	ecx,ebx
blitincfillg:
		add	ecx,ebp			   ; Add ptB with +2 or -2
		and	ecx,01ffffeh		   ; For safety
		mov	dword [bltbpt],ecx	  ; Save ptB
		mov	ecx,eax			   ; Extract shift for B
		shr	ecx,12
		and	ecx,0fh
		shl	edx,16			   ; Move my data up, lo is clean

		test	eax,2			   ; Rotate or shift
		jz	blitincnofdesc3
		rol	edx,cl
		jmp	blitincnofdesc4
blitincnofdesc3:
		shr	edx,cl
blitincnofdesc4:


		mov	ecx,dword [blit_leftoverB]    ; Old leftover
		mov	dword [blit_leftoverB],edx    ; Store leftover from B
		shr	edx,16			   ; Move rest of data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltbdat],edx	  ; Store data from B
blitincfill3:



blitincfill3x:
		test	eax,02000000h			    ; Test dma C
		jz	blitincfill4
		mov	ecx,dword [bltcpt]
		mov	edx,dword [memory_chip-2+ecx]
		bswap	edx			       ; Move my data down
		cmp	esi,1			       ; last word??
		jne	blitincfillh
		mov	ebx,dword [bltcmod]		; BLTCMOD
		add	ecx,ebx
blitincfillh:
		add	ecx,ebp
		and	ecx,01ffffeh
		mov	dword [bltcpt],ecx
		mov	dword [bltcdat],edx		  ; Store data from C

; Now the data lies in BLTXDAT registers
blitincfill4:
		push	eax
		mov	edx,dword [bltcdat]
		mov	ecx,dword [bltbdat]
		mov	ebx,dword [bltadat]
		call	dword [bltminterm]

		mov	edx,eax
		pop	eax
		and	edx,0ffffh
; Now we have the result
; Do fill

		test	eax,4			; test if FC bit is set
		jnz	blitincafc1
		mov	bx,word [fillincfc0 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillincfc0after +edx] ; get fc bit after
		jmp	blitincfillafter
blitincafc1:
		mov	bx,word [fillincfc1 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillincfc1after + edx] ; get fc bit after
blitincfillafter:
		shl	dl,2		; Inject FC bit into saved bltcon
		and	al,0fbh
		or	al,dl
		mov	byte [bltfillbltconsave],al
		mov	eax,dword [bltfillbltconsave]
		or	dword [bltzero],ebx
	
		test	eax,01000000h				; Test DMA D
		jz	blitincfillDoff

		mov	ecx,dword [bltdpt]
		mov	byte [memory_chip + ecx],bh
		mov	byte [memory_chip + 1 + ecx],bl
		add	ecx,ebp

	
		cmp	esi,1
		jne	blitincfillj
		mov	ebx,dword [bltdmod]
		add	ecx,ebx
blitincfillj:
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx
blitincfilli:
blitincfillDoff:		
		dec	esi
		jnz	near blitincfillword
		dec	dword [linecount]
		jnz	near blitterincfillline
		jmp	blitterfillend


		FALIGN32

blitterexfillline:
		mov	esi,dword [linelength]
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		mov	ebp,dword [bltdesc]
blitexfillword:
blitexfill1:
		test	eax,08000000h			    ; Test dma A
		jz	blitexfill2
		mov	ecx,dword [bltapt]
		mov	edx,dword [memory_chip+ecx]  ;[memory_chip-2+ecx]
		bswap	edx			; My word is now in bits 31-16
		cmp	esi,dword [linelength]  ; Do first word masking
		jne	blitexfilld
		and	edx,dword [bltafwm]
blitexfilld:
		cmp	esi,1			   ; Do last word masking+ modulo
		jne	blitexfillf
		mov	ebx,dword [bltamod]		 ; BLTMOD oppdateres DESC
		add	ecx,ebx
		and	edx,dword [bltalwm]	  ; Mask is mirrored in lo/hi word
blitexfillf:
		add	ecx,ebp			   ; Add +2 or -2 to ptA
		and	edx,0ffff0000h		   ; Take away garbage
		and	ecx,01ffffeh		   ; For safety
		mov	dword [bltapt],ecx
		mov	ecx,eax			   ; Move saved bltcon to ecx
		shr	ecx,28			   ; Exctract shift for A

		test	eax,2	     ; Test desc mode
		jz	blitexnofdesc1
		rol	edx,cl			   ; Rotate to left in desc mode
		jmp	blitexnofdesc2
blitexnofdesc1:
		shr	edx,cl			   ; Shift to right in forward mode
blitexnofdesc2:


		mov	ecx,dword [blit_leftoverA]    ; get last leftover
		mov	dword [blit_leftoverA],edx    ; Store leftover from A
		shr	edx,16			   ; Move my shifted  data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltadat],edx	  ; Store data from A
		jmp	blitexfill2a

; DMA A off
blitexfill2:
		mov	edx,dword [bltadatoriginal]
		shl	edx,16
		cmp	esi,dword [linelength]  ; Do first word masking
		jne	befd
		and	edx,dword [bltafwm]
befd:
		cmp	esi,1			   ; Do last word masking+ modulo
		jne	beff
		mov	ebx,dword [bltamod]		 ; BLTMOD oppdateres DESC
		add	ecx,ebx
		and	edx,dword [bltalwm]	  ; Mask is mirrored in lo/hi word
beff:
		mov	ecx,eax			   ; Move saved bltcon to ecx
		shr	ecx,28			   ; Exctract shift for A
	
		test	eax,2	     ; Test desc mode
		jz	befdesc1
		rol	edx,cl			   ; Rotate to left in desc mode
		jmp	befdesc2
befdesc1:
		shr	edx,cl			   ; Shift to right in forward mode
befdesc2:


		mov	ecx,dword [blit_leftoverA]    ; get last leftover
		mov	dword [blit_leftoverA],edx    ; Store leftover from A
		shr	edx,16			   ; Move my shifted  data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltadat],edx	  ; Store data from A


blitexfill2a:


		test	eax,04000000h		   ; Test dma B
		jz	blitexfill3
		mov	ecx,dword [bltbpt]
		mov	edx,dword [memory_chip-2+ecx]
		bswap	edx			   ; My data in lo word
		cmp	esi,1			   ; Do modulo if last word on line 
		jne	blitexfillg
		mov	ebx,dword [bltbmod]	  ; BLTBMOD
		add	ecx,ebx
blitexfillg:
		add	ecx,ebp			   ; Add ptB with +2 or -2
		and	ecx,01ffffeh		   ; For safety
		mov	dword [bltbpt],ecx	  ; Save ptB
		mov	ecx,eax			   ; Extract shift for B
		shr	ecx,12
		and	ecx,0fh
		shl	edx,16			   ; Move my data up, lo is clean
	
		test	eax,2			   ; Rotate or shift
		jz	blitexnofdesc3
		rol	edx,cl
		jmp	blitexnofdesc4
blitexnofdesc3:
		shr	edx,cl
blitexnofdesc4:


		mov	ecx,dword [blit_leftoverB]    ; Old leftover
		mov	dword [blit_leftoverB],edx    ; Store leftover from B
		shr	edx,16			   ; Move rest of data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltbdat],edx	  ; Store data from B
	
blitexfill3:

blitexfill3a:
		test	eax,02000000h			    ; Test dma C
		jz	blitexfill4
		mov	ecx,dword [bltcpt]
		mov	edx,dword [memory_chip-2+ecx]
		bswap	edx			       ; Move my data down
		cmp	esi,1			       ; last word??
		jne	blitexfillh
		mov	ebx,dword [bltcmod]		; BLTCMOD
		add	ecx,ebx
blitexfillh:
		add	ecx,ebp
		and	ecx,01ffffeh
		mov	dword [bltcpt],ecx
		mov	dword [bltcdat],edx		  ; Store data from C
	
; Now the data lies in BLTXDAT registers
blitexfill4:

		push	eax
		mov	edx,dword [bltcdat]
		mov	ecx,dword [bltbdat]
		mov	ebx,dword [bltadat]
		call	dword [bltminterm]

		mov	edx,eax
		pop	eax
		and	edx,0ffffh

; Now we have the result
; Do fill

		test	eax,4			; test if FC bit is set
		jnz	blitexafc1
		mov	bx,word [fillexcfc0 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillexcfc0after + edx] ; get fc bit after
		jmp	blitexfillafter
blitexafc1:
		mov	bx,word [fillexcfc1 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillexcfc1after + edx] ; get fc bit after
blitexfillafter:
		shl	dl,2		; Inject FC bit into saved bltcon
		and	al,0fbh
		or	al,dl
		mov	byte [bltfillbltconsave],al
		mov	eax,dword [bltfillbltconsave]
		or	dword [bltzero],ebx

		test	eax,01000000h					; Test DMA D 
		jz	blitexfillDoff
		
		mov	ecx,dword [bltdpt]
		mov	byte [memory_chip+ecx],bh
		mov	byte [memory_chip+1+ecx],bl
		add	ecx,ebp
		cmp	esi,1
		jne	blitexfillj
		mov	ebx,dword [bltdmod]
		add	ecx,ebx
blitexfillj:
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx
blitexfilli:
blitexfillDoff:
		dec	esi
		jnz	near blitexfillword
		dec	dword [linecount]
		jnz	near blitterexfillline
		jmp	blitterfillend



		FALIGN32


; Fill routine optimized for AD


global _ADblitterfillline_
_ADblitterfillline_:
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		test	eax,8
		jz	near ADblitterfillexclusive

ADblitterincfillline:
		mov	esi,dword [linelength]
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		mov	ebp,dword [bltdesc]
ADblitterincfillword:
ADblitincfill1:
		mov	ecx,dword [bltapt]
		mov	edx,dword [memory_chip+ecx]  ;[memory_chip-2+ecx]
		bswap	edx			; My word is now in bits 31-16

		cmp	esi,dword [linelength]  ; Do first word masking
		jne	ADblitincfilld
		and	edx,dword [bltafwm]
ADblitincfilld:
		cmp	esi,1			   ; Do last word masking+ modulo
		jne	ADblitincfillf
		mov	ebx,dword [bltamod]		 ; BLTMOD oppdateres DESC
		add	ecx,ebx
		and	edx,dword [bltalwm]	  ; Mask is mirrored in lo/hi word
ADblitincfillf:
		add	ecx,ebp			   ; Add +2 or -2 to ptA
		and	edx,0ffff0000h		   ; Take away garbage
		and	ecx,01ffffeh		   ; For safety
		mov	dword [bltapt],ecx
		mov	ecx,eax			   ; Move saved bltcon to ecx
		shr	ecx,28			   ; Exctract shift for A

		test	eax,2	     ; Test desc mode
		jz	ADblitincnofdesc1
		rol	edx,cl			   ; Rotate to left in desc mode
		jmp	ADblitincnofdesc2
ADblitincnofdesc1:
		shr	edx,cl			   ; Shift to right in forward mode
ADblitincnofdesc2:


		mov	ecx,dword [blit_leftoverA]    ; get last leftover
		mov	dword [blit_leftoverA],edx    ; Store leftover from A
		shr	edx,16			   ; Move my shifted  data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltadat],edx	  ; Store data from A

ADblitincfill2:

; Now the data lies in BLTXDAT registers
ADblitincfill4:

		push	eax
		mov	edx,dword [bltcdat]
		mov	ecx,dword [bltbdat]
		mov	ebx,dword [bltadat]
		call	dword [bltminterm]
		mov	edx,eax
		pop	eax
		and	edx,0ffffh

; Now we have the result


		test	eax,4			; test if FC bit is set
		jnz	ADblitincafc1
		mov	bx,word [fillincfc0 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillincfc0after + edx] ; get fc bit after
		jmp	ADblitincfillafter
ADblitincafc1:
		mov	bx,word [fillincfc1 + edx*2] ; Get inc,fc0
		mov	dl,byte [fillincfc1after + edx] ; get fc bit after
ADblitincfillafter:
		shl	dl,2		; Inject FC bit into saved bltcon
		and	al,0fbh
		or	al,dl
		mov	byte [bltfillbltconsave],al


		mov	eax,dword [bltfillbltconsave]

		mov	ecx,dword [bltdpt]
		or	dword [bltzero],ebx
		mov	byte [memory_chip + ecx],bh
		mov	byte [memory_chip+1+ecx],bl
		add	ecx,ebp
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx
ADblitincfilli:
		dec	esi
		jnz	near ADblitterincfillword
		mov	ebx,dword [bltdmod]
		add	ecx,ebx
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx

		dec	dword [linecount]
		jnz	near ADblitterincfillline
		jmp	blitterfillend


		FALIGN32


ADblitterfillexclusive:
ADblitterexcfillline:
		mov	esi,dword [linelength]
		mov	eax,dword [bltcon]
		mov	dword [bltfillbltconsave],eax
		mov	ebp,dword [bltdesc]
ADblitterexcfillword:
ADblitexcfill1:
		mov	ecx,dword [bltapt]
		mov	edx,dword [memory_chip+ecx]  ;[memory_chip-2+ecx]
		bswap	edx			; My word is now in bits 31-16

		cmp	esi,dword [linelength]  ; Do first word masking
		jne	ADblitexcfilld
		and	edx,dword [bltafwm]
ADblitexcfilld:
		cmp	esi,1			   ; Do last word masking+ modulo
		jne	ADblitexcfillf
		mov	ebx,dword [bltamod]		 ; BLTMOD oppdateres DESC
		add	ecx,ebx
		and	edx,dword [bltalwm]	  ; Mask is mirrored in lo/hi word
ADblitexcfillf:
		add	ecx,ebp			   ; Add +2 or -2 to ptA
		and	edx,0ffff0000h		   ; Take away garbage
		and	ecx,01ffffeh		   ; For safety
		mov	dword [bltapt],ecx
		mov	ecx,eax			   ; Move saved bltcon to ecx
		shr	ecx,28			   ; Exctract shift for A

		test	eax,2	     ; Test desc mode
		jz	ADblitexcnofdesc1
		rol	edx,cl			   ; Rotate to left in desc mode
		jmp	ADblitexcnofdesc2
ADblitexcnofdesc1:
		shr	edx,cl			   ; Shift to right in forward mode
ADblitexcnofdesc2:


		mov	ecx,dword [blit_leftoverA]    ; get last leftover
		mov	dword [blit_leftoverA],edx    ; Store leftover from A
		shr	edx,16			   ; Move my shifted  data down
		or	edx,ecx			   ; Merge with leftover
		mov	dword [bltadat],edx	  ; Store data from A

ADblitexcfill2:

; Now the data lies in BLTXDAT registers
ADblitexcfill4:

		push	eax
		mov	edx,dword [bltcdat]
		mov	ecx,dword [bltbdat]
		mov	ebx,dword [bltadat]
		call	dword [bltminterm]

		mov	edx,eax
		pop	eax
		and	edx,0ffffh

; Now we have the result


		test	eax,4			; test if FC bit is set
		jnz	ADblitexcafc1
		mov	bx,word [fillexcfc0 + edx*2] ; Get exc,fc0
		mov	dl,byte [fillexcfc0after + edx] ; get fc bit after
		jmp	ADblitexcfillafter
ADblitexcafc1:
		mov	bx,word [fillexcfc1 + edx*2] ; Get exc,fc0
		mov	dl,byte [fillexcfc1after + edx] ; get fc bit after
ADblitexcfillafter:
		shl	dl,2		; Inject FC bit into saved bltcon
		and	al,0fbh
		or	al,dl
		mov	byte [bltfillbltconsave],al


		mov	eax,dword [bltfillbltconsave]

		mov	ecx,dword [bltdpt]
		or	dword [bltzero],ebx
		mov	byte [memory_chip+ecx],bh
		mov	byte [memory_chip+1+ecx],bl
		add	ecx,ebp
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx
ADblitexcfilli:
		dec	esi
		jnz	near ADblitterexcfillword
		mov	ebx,dword [bltdmod]
		add	ecx,ebx
		and	ecx,01ffffeh
		mov	dword [bltdpt],ecx

		dec	dword [linecount]
		jnz	near ADblitterexcfillline



blitterfillend:
		test	dword [bltcon],008000000h
		jz	bliend1
		mov	edx,dword [bltadatoriginal]
		mov	dword [bltadat],edx
bliend1:
		mov	edx,08040h
		mov	ecx,0dff09ch
		call	wriw

		ret


; ===========================================
; Blitter COPY
; ===========================================

; -----------------------------------------------------------------------------
; Do one line of blitter copy operation
; Not LINEMODE!	 Stupid labelname.
; This is the general routine called in case there isn't an optimized routine.
; -----------------------------------------------------------------------------

		FALIGN32

blitter_normal:

global _blitterline_
_blitterline_:	mov	esi, dword [linelength]
		mov	ebp, dword [bltdesc]
blitterword:	mov	eax, dword [bltcon]
blit1:		test	eax, 01000000h
		jz	near bliti
		test	eax, 08000000h				; Test dma A
		jz	blit2
		mov	ecx, dword [bltapt]
		mov	edx, dword [memory_chip + ecx]
		bswap	edx
		cmp	esi, dword [linelength]
		jne	blitd
		and	edx, dword [bltafwm]
blitd:		cmp	esi, 1
		jne	blitf
		mov	ebx, dword [bltamod]
		add	ecx, ebx
		and	edx, dword [bltalwm]
blitf:		add	ecx, ebp
		and	edx, 0ffff0000h
		mov	dword [bltapt], ecx
		mov	ecx, eax
		shr	ecx, 28
		test	eax, 2
		jz	nodesc1
		rol	edx, cl
		jmp	nodesc2
nodesc1:	shr	edx, cl
nodesc2		mov	ecx, dword [blit_leftoverA]
		mov	dword [blit_leftoverA], edx	; Store leftover from A
		shr	edx, 16
		or	edx, ecx			; Merge leftover
		and	edx, 0ffffh
		mov	dword [bltadat], edx	; Store data from A
		jmp	blit90
blit2:		mov	edx, dword [bltadatoriginal]
		shl	edx, 16
		cmp	esi, dword [linelength]	; Do first word masking
		jne	bnorbd
		and	edx, dword [bltafwm]
bnorbd:		cmp	esi, 1			; Do last word masking+ modulo
		jne	bnorbf
		mov	ebx, dword [bltamod]
		add	ecx, ebx
		and	edx, dword [bltalwm]; Mask mirrored in lo/hi word
bnorbf:		mov	ecx, eax		; Move saved bltcon to ecx
		shr	ecx, 28			; Exctract shift for A
		test	eax, 2			; Test desc mode
		jz	bnordesc1
		rol	edx, cl			; Rotate to left in desc mode
		jmp	bnordesc2
bnordesc1:	shr	edx, cl		; Shift to right in forward mode
bnordesc2:	mov	ecx, dword [blit_leftoverA]	; get last leftover
		mov	dword [blit_leftoverA], edx	; Store leftover from A
		shr	edx, 16				; Shifted  data down
		or	edx, ecx			; Merge with leftover
		mov	dword [bltadat], edx	; Store data from A
blit90:		test	eax, 04000000h			; Test dma B
		jz	blit3
		mov	ecx, dword [bltbpt]
		mov	edx, dword [memory_chip + ecx]
		bswap	edx
		cmp	esi, 1				; Last word??
		jne	blitg
		mov	ebx, dword [bltbmod]
		add	ecx, ebx
blitg:		add	ecx, ebp
		mov	dword [bltbpt], ecx
		mov	ecx, eax
		shr	ecx, 12
		and	ecx, 0fh 
		and	edx, 0ffff0000h
		test	eax, 2
		jz	nodesc3
		rol	edx, cl
		jmp	nodesc4
nodesc3:	shr	edx, cl
nodesc4:	mov	ecx, dword [blit_leftoverB]
		mov	dword [blit_leftoverB], edx	; Store leftover from B
		shr	edx, 16
		or	edx, ecx
		and	edx, 0ffffh
		mov	dword [bltbdat], edx	; Store data from B
blit3:
blit80:		test	eax, 02000000h			; Test dma C
		jz	blit4
		mov	ecx, dword [bltcpt]
		mov	edx, dword [memory_chip - 2 + ecx]
		bswap	edx
		cmp	esi, 1				; Last word??
		jne	blith
		mov	ebx, dword [bltcmod]
		add	ecx, ebx
blith:		add	ecx, ebp
		mov	dword [bltcpt], ecx
		and	edx, 0ffffh
		mov	dword [bltcdat], edx	; Store data from C
blit4:		mov	edx, dword [bltcdat]	; Data lies in BLTXDAT
		mov	ecx, dword [bltbdat]
		mov	ebx, dword [bltadat]
		call	dword [bltminterm]
		or	dword [bltzero],eax
		test	byte [bltcon + 3], 1	; We have the result
		jz	blitDoff	
		mov	ecx, dword [bltdpt]
		mov	byte [memory_chip + ecx], ah
		mov	byte [memory_chip + 1 + ecx], al
		add	ecx, ebp
		cmp	esi, 1
		jne	blitj
		mov	ebx, dword [bltdmod]
		add	ecx, ebx
blitj:		mov	dword [bltdpt], ecx
blitDoff:
bliti:		dec	esi
		jnz	near blitterword
		dec	dword [linecount]
		jnz	near _blitterline_
		test	dword [bltcon], 008000000h	; restore registers
		jz	bliend3
		mov	edx, dword [bltadatoriginal]
		mov	dword [bltadat], edx
bliend3:	test	dword [bltcon], 004000000h
		jz	bliend4
		mov	edx, dword [bltbdatoriginal]
		mov	dword [bltbdat], edx
bliend4:	mov	edx, 08040h
		mov	ecx, 0dff09ch
		call	wriw
		ret


; ------------------------------------------
; Do one line of blitter operation
; Optimized for AD
; ------------------------------------------

		FALIGN32

global _ADblitterline_
_ADblitterline_:mov	esi, dword [linelength]
		mov	ebp, dword [bltdesc]
ADblitterword:	mov	eax, dword [bltcon]
		mov	ecx, dword [bltapt]
		mov	edx, dword [memory_chip + ecx]
		bswap	edx
		cmp	esi, dword [linelength]
		jne	ADblitd
		and	edx, dword [bltafwm]
ADblitd:	cmp	esi, 1				; Last word in line
		jne	ADblitf				; so
		mov	ebx, dword [bltamod]	; add modulo
		add	ecx, ebx
		and	edx, dword [bltalwm]
ADblitf:	add	ecx, ebp
ADblargh2:	and	edx, 0ffff0000h
		mov	dword [bltapt], ecx
		mov	ecx, eax
		shr	ecx, 28
		test	eax, 2
		jz	nodesc5
		rol	edx, cl
		jmp	nodesc6
nodesc5:	shr	edx, cl
nodesc6:	mov	ecx, dword [blit_leftoverA]
		mov	dword [blit_leftoverA], edx	; Store leftover from A
		shr	edx, 16
		or	edx, ecx			; Merge leftover
		mov	dword [bltadat], edx	; Store data from A
		mov	edx, dword [bltcdat]	; Data lies in BLTXDAT
		mov	ecx, dword [bltbdat]
		mov	ebx, dword [bltadat]
		call	dword [bltminterm]		; Do logic
		mov	ecx, dword [bltdpt]		; We have the result
ADblargh3:	or	dword [bltzero], eax
		mov	byte [memory_chip + ecx], ah
		mov	byte [memory_chip + 1 + ecx], al
		add	ecx, ebp
ADblargh4:	cmp	esi, 1
		jne	ADblitj
		mov	ebx, dword [bltdmod]
		add	ecx, ebx
ADblitj:	mov	dword [bltdpt], ecx
ADbliti:	dec	esi
		jnz	near ADblitterword
		dec	dword [linecount]
		jnz	near _ADblitterline_
		mov	edx, 08040h
		mov	ecx, 0dff09ch
		call	wriw
		ret

; ---------------------------------------
; Do one line of blitter operation
; Optimized for D
; ---------------------------------------

		FALIGN32


global _Dblitterline_
_Dblitterline_:	mov	edx, dword [bltcdat]
		mov	ecx, dword [bltbdat]
		mov	ebx, dword [bltadat]
		call	dword [bltminterm]
		bswap	eax
		shr	eax, 16
		or	dword [bltzero], eax
		mov	ecx, dword [bltdpt]
		mov	ebx, dword [bltdmod]
		mov	ebp, dword [bltdesc]
Dblitterline2:	mov	esi, dword [linelength]
Dblitterword:
Dblit1:
Dblargh2:	mov	word [memory_chip + ecx], ax
		add	ecx, ebp
Dblargh:	dec	esi
		jnz	Dblitterword
		add	ecx, ebx
		dec	dword [linecount]
		jnz	Dblitterline2
		mov	dword [bltdpt], ecx
		mov	edx, 08040h
		mov	ecx, 0dff09ch
		call	wriw
		ret

; ===========================================================
; Blitter COPY ENDS
; ===========================================================


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

; !(B+C) + Ca

global _blit_min_0b_
_blit_min_0b_:	push	ebx
		mov	eax, edx
		or	eax, ecx
		not	eax
		not	ebx
		and	ebx, edx
		or	eax, ebx
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

