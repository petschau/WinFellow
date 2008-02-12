;===============================================================================
; Fellow Amiga Emulator
; Sprite emulation
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; * Implements a sprite state machine for DMA driven sprites.
; * Implements normalization of sprite graphics data.
; * Get and Set functions for sprite hardware registers.
; * Functions which merge normalized sprite graphics data with the under or 
;   overlaying normalized bitplane data.
;
; Well-known restrictions:
; * Only 8 sprites per line, standard DMA-driven sprite support.
;   We all know games sometimes have more tricks they'd like to do with
;   sprites than that. Any implementation dealing with this should
;   completely replace the current implementation.
;===============================================================================

%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "generic/defs.inc"
%include "mac/callconv.mac"

FASMFILESTART
FDATASECTIONSTART

;----------------------------------------------------------------------------- 
; Include data from other modules
;----------------------------------------------------------------------------- 

%include "data/sprite.inc"
%include "data/graph.inc"
%include "data/fmem.inc"
%include "data/draw.inc"

	
FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

%define SPRITE_ASM

%include "func/sprite.inc"
%include "mac/sprite.mac"


	
;------------------------------------------------------------------------------
; Decode sprite 4 colors
; esi - buffer to decode to
; ecx - pointer to spritedata array
; ebx - sprite class (0,1,2,3)
;------------------------------------------------------------------------------

decode4sprite_:	xor	edx,edx
		mov	edi,dword [sprite4_0 + ebx*4]
		mov	ebp,dword [sprite4_1 + ebx*4]
		mov	dl,byte [ecx]
		mov	eax,dword [8*edx + edi]
		mov	ebx,dword [8*edx + edi + 4]
		mov	dl,byte [2 + ecx]
		or	eax,dword [8*edx + ebp]
		or	ebx,dword [8*edx + ebp + 4]
		mov	dl,byte [1 + ecx]
		mov	dword [esi],eax
		mov	dword [4 + esi],ebx
		mov	eax,dword [8*edx + edi]
		mov	ebx,dword [8*edx + edi + 4]
		mov	dl,byte [3 + ecx]
		or	eax,dword [8*edx + ebp]
		or	ebx,dword [8*edx + ebp + 4]
		mov	dword [8 + esi],eax
		mov	dword [12 + esi],ebx
		ret


		FALIGN32


;------------------------------------------------------------------------------
; Decode sprite 16 colors
; esi - buffer to decode to
; ecx - pointer to spritedata array
;------------------------------------------------------------------------------

decode16sprite_:xor	edx, edx
		mov	dl, byte [ecx]
		mov	eax, dword [sprite_deco01     + 8*edx]
		mov	ebx, dword [sprite_deco01 + 4 + 8*edx]
		mov	dl, byte [1 + ecx]
		mov	edi, dword [sprite_deco01     + 8*edx]
		mov	ebp, dword [sprite_deco01 + 4 + 8*edx]
		mov	dl, byte [2 + ecx]
		or	eax, dword [sprite_deco02     + 8*edx]
		or	ebx, dword [sprite_deco02 + 4 + 8*edx]
		mov	dl, byte [3 + ecx]
		or	edi, dword [sprite_deco02     + 8*edx]
		or	ebp, dword [sprite_deco02 + 4 + 8*edx]
		mov	dl, byte [4 + ecx]
		or	eax, dword [sprite_deco03     + 8*edx]
		or	ebx, dword [sprite_deco03 + 4 + 8*edx]
		mov	dl, byte [5 + ecx]
		or	edi, dword [sprite_deco03     + 8*edx]
		or	ebp, dword [sprite_deco03 + 4 + 8*edx]
		mov	dl, byte [6 + ecx]
		or	eax, dword [sprite_deco04     + 8*edx]
		or	ebx, dword [sprite_deco04 + 4 + 8*edx]
		mov	dl, byte [7 + ecx]
		or	edi, dword [sprite_deco04     +8*edx]
		or	ebp, dword [sprite_deco04 + 4 +8*edx]
		mov	dword [esi], eax
		mov	dword [4 + esi], ebx
		mov	dword [8 + esi], edi
		mov	dword [12 + esi], ebp
		ret



; Decode sprites

; The following 1000 lines of code are pretty terrible.....

; We know dma is on, so read all the needed datawords
; if sprite_state is 0, read two control words
; else if sprite_state is 1, we are waiting for first line
; else if sprite_state is 2 , read two data words and decode them
; else if sprite_state is 3, sprite is off
; state 4 is when data is written directly to the data registers
;	  and real state is 2
; state 5 is when sprite is really off, but data is written.

		FALIGN32

global _spritesDecode_
_spritesDecode_:
		push	esi		; Esi used in nxt_inst loop copper part
		push	eax
		xor	esi, esi	; Counts sprites*4
					; No sprites initially
		mov	dword [sprites_online], 0
	
dsloop:		mov	ebx, dword [sprite_state + esi]; state for sprite
		mov	dword [sprite_online + esi], 0; Not displayed
		mov	dword [sprite_16col + esi], 0; Initially 4 color
		cmp	ebx, 3
		je	near dsloopend	; State 3, skip sprite, this is OK
		cmp	ebx, 1
		jne	dsrwr0

; State 1, waiting for first line, check if this line is the one

		mov	eax, dword [graph_raster_y]; Next line the first line?
		cmp	eax, dword [spry + esi]
		jne	near dsloopend	; No, skip statechange and sprite
		mov	dword [sprite_state + esi], 2	; Statechange
		mov	ebx, 2
dsrwr0:		; State 2 or 0 or 4 or 5, Here two words must be read
		mov	ecx, dword [sprpt + esi]
		cmp	ebx, 4	; State 4, words replaced by CPU
		je	dsrwr0a
		cmp	ebx, 5	; State 5, words written out of order by CPU
		je	dsrwr0b
		mov	edx, dword [memory_chip + ecx]
		mov	dword [sprdat + esi], edx
dsrwr0a:	add	ecx, 4
		and	ecx, 01ffffeh
		mov	dword [sprpt + esi], ecx
		test	ebx, ebx
		jz	near dsrwc0
		mov	ecx, dword [sprly + esi]
		cmp	ecx, dword [graph_raster_y]
		je	near dsrwc0

		;--------------------------------------------------------------
		; State 2, Data words
		; If sprite is attached, and the other too, let odd sprite
		; do the decoding
		;--------------------------------------------------------------

dsrwr0b:
  		test	esi, 04h	; test wether we are odd or even sprite
		jnz	dsoddsprite

		; Even attached sprites does nothing, the odd sprite handles it

                ; Funny quirk, the attach bit in even sprites seems to have
		; no effect. If the odd sprite is attached, both are.

		; Another quirk, if the positions don't match, the sprites
		; still take color as if they were part of a 16 color image.
		; The missing bitplanes has value 0, the overlapping part
		; forms a 16-color image. Since sprite positions have
		; hi-res resolutions, while the sprite image is lores,
		; the overlapping parts can probably cover two half-pixels
		; in the other sprite. I am not sure what happens in this
		; case, since this quirk is not emulated at all, it only has
		; curiosity interest.

		test	dword [spratt + 4 + esi], 1
					; Even sprite is attached if
		jz	dsrwrr		; odd sprite is attached
		
		; OK we are attached and even, let odd sprite do the job
		; But we need a kludge here.....
		; It seems KSV1.2 does not end its sprite lists properly...??
		; So the att bit is pretty random sometimes.
		; So if it seems likely that the odd sprite is not operating
		; properly, we show the sprite as a 4 color sprite anyway
		; If state is 0 or 1, we need to check if the odd sprite
		; becomes active on this line.

		cmp	dword [sprite_state + esi + 4], 3
		je	dsrwrr

		cmp	dword [sprite_state + esi + 4], 2
		jae	near dsresumeattached
			
		mov	eax, dword [graph_raster_y]; Next line the first line?
		cmp	eax, dword [spry + esi + 4]
		jne	dsrwrr

		jmp	dsresumeattached
dsoddsprite:
; We are odd sprite, if attached, both are.

		test	dword [spratt + esi], 1 ; Attached?
		jz	dsrwrr

		; OK we are attached and odd, we must do the job

		lea	ecx, [sprdat - 4 + esi]
		push	esi
		lea	esi, [sprite + 4*esi]
		call	decode16sprite_
		pop	esi
		mov	dword [sprites_online], 1
		mov	dword [sprite_online + esi], 1
		mov	dword [sprite_16col + esi], 1
		jmp	dsresumeattached

dsrwrr:	 ; No attached sprites, 4 color decode

		lea	ecx, [sprdat + esi]
		mov	ebx, esi
		shr	ebx, 3
		push	esi
		lea	esi, [sprite + 4*esi]

		call	decode4sprite_
		pop	esi
		mov	dword [sprites_online], 1
		mov	dword [sprite_online + esi], 1

dsresumeattached:
		cmp	dword [sprite_state + esi], 4
		jb	drest
		mov	eax, dword [sprite_state_old + esi]
		mov	dword [sprite_state + esi], eax
drest:		cmp	dword [sprite_state + esi], 5	
		je	dres1
		mov	eax,dword [graph_raster_y]    ; Check if this was the last line
		inc	eax
		cmp	eax,dword [sprly + esi]
		jne	near dsloopend
		mov	dword [sprite_state + esi], 0
		jmp	dsloopend
dres1:		mov	dword [sprite_state + esi], 3
		jmp	dsloopend
dsrwc0: ; State 0, Control words
		test	edx, edx
		jnz	dsrwcc
		mov	dword [sprite_state + esi], 3
		mov	dword [sprx + esi], edx
		mov	dword [spry + esi], edx
		mov	dword [sprly + esi], edx
		mov	dword [spratt + esi], edx
		jmp	dsloopend
dsrwcc:		mov	eax, edx
		rol	eax, 1
		and	eax, 1
		mov	dword [spratt + esi], eax
		xor	eax, eax			; Find x position
		mov	al, dh
		shl	eax, 1
		mov	ebx, edx
		shr	ebx, 24
		and	ebx, 1
		or	eax, ebx
		inc	eax			; Sprites delayed 1 cycle
		mov	dword [sprx + esi], eax  ; Now found x position
		xor	eax, eax			; Find first y position
		mov	al, dl
		mov	ebx, edx
		shr	ebx, 16
		and	ebx, 0400h
		shr	ebx, 2
		or	eax, ebx
		mov	dword [spry + esi], eax
						; Now found first y position
		xor	eax, eax		; Now find last y position
		ror	edx, 16
		mov	al, dl
		mov	ah, dh
		and	eax, 02ffh
		shr	ah, 1
		mov	dword [sprly + esi], eax
						; Now found last y position
		cmp	eax,dword [graph_raster_y]
		je	dsloopend
		mov	dword [sprite_state + esi], 1
						; Set waiting for first line
dsloopend:	add	esi, 4
		cmp	esi, dword [sprite_ddf_kill]
		jae	dsexit
		cmp	esi, 32
		jne	near dsloop
dsexit:		pop	eax
		pop	esi
		ret


;==============================================================================
; Merge decoded sprites with the line arrays
;
; Input:
; [esp + 4] - Pointer to the linedesc struct
;==============================================================================


		FALIGN32

global _spritesMerge_
_spritesMerge_:	xor	esi, esi
		mov	dword [sprites_online], 0

		test	byte [bplcon0 + 1], 8
		jz	spritesMergeOrdinary

		mov	edx, dword [esp + 4]
		SPRITEMERGEHAM_CWRAP
		ret

spritesMergeOrdinary:

		; The big question is, should PF1 or PF2 be the right one to use
		; Abacus suggestes PF1 in non-dual mode. That fails. So use PF2

		test	dword [bplcon0], 00400h
		jnz	near mergedualsprites
		mov	ecx, dword [bplcon2]
		and	ecx, 38h
		test	dword [bplcon0], 08000h
		jz	near mergeloresloop
		cmp	byte [draw_hscale], 2
		je	near mergehiresnewsprites

; Merge sprites in 1x mode hires

mergehires320loop:
		test	dword [sprite_online + esi], 1
		jz	near mergehires320skipsprite
		mov	edi, dword [sprx + esi]
		mov	eax, dword [graph_DIW_last_visible]
		shr	eax, 1
		cmp	edi, eax	       ;  dword _graph_DIW_last_visible
		ja	near mergehires320skipsprite
		mov	ebp, dword [esp + 4]
		lea	edi, [edi + ebp + graph_line_line1]
		lea	ebp, [sprite + 4*esi]

; determine whether this sprite is in front or behind the playfield
; if ecx is bigger than esi, then sprite must be drawn in front
		xor	eax, eax
		cmp	ecx, esi
		jbe	mergehires320setuploop
		mov	eax, 010000h
mergehires320setuploop:
		push	ecx
		push	esi
		lea	esi, [sprite_translate + eax]
		xor	eax, eax
		xor	ebx, ebx
mergehires320loop2:
		mov	edx, dword [ebp]
		mov	ecx, dword [edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [edi], ecx

		mov	edx, dword [4 + ebp]
		mov	ecx, dword [4 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [4 + edi], ecx

		mov	edx, dword [8 + ebp]
		mov	ecx, dword [8 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [8 + edi], ecx

		mov	edx, dword [12 + ebp]
		mov	ecx, dword [12 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [12 + edi], ecx

		pop	esi
		pop	ecx
mergehires320skipsprite:
		add	esi, 4
		cmp	esi, 32
		jne	near mergehires320loop
		ret




		FALIGN32

mergeloresloop:
		test	dword [sprite_online + esi], 1
		jz	near mergeloresskipsprite
		mov	edi, dword [sprx + esi]
		cmp	edi, dword [graph_DIW_last_visible]
		ja	near mergeloresskipsprite
		mov	ebp, dword [esp + 4]
		lea	edi, [edi + ebp + graph_line_line1]
		lea	ebp, [sprite + 4*esi]

	; determine whether this sprite is in front or behind the playfield
	; if ecx is bigger than esi, then sprite must be drawn in front
		xor	eax, eax
		cmp	ecx, esi
		jbe	mergeloressetuploop
		mov	eax, 010000h
mergeloressetuploop:
		push	ecx
		push	esi
		lea	esi, [sprite_translate + eax]
		xor	eax, eax
		xor	ebx, ebx
mergeloresloop2:mov	edx, dword [ebp]
		mov	ecx, dword [edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [edi], ecx

		mov	edx, dword [4 + ebp]
		mov	ecx, dword [4 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [4 + edi], ecx

		mov	edx, dword [8 + ebp]
		mov	ecx, dword [8 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [8 + edi], ecx

		mov	edx, dword [12 + ebp]
		mov	ecx, dword [12 + edi]
		mov	al, dl
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		ror	edx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		mov	bl, dh
		ror	ecx, 16
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [12 + edi], ecx

		pop	esi
		pop	ecx

mergeloresskipsprite:
		add	esi, 4
		cmp	esi, 32
		jne	near mergeloresloop
		ret


		FALIGN32

mergehiresnewsprites:
mergehiresnewloop:
		test	dword [sprite_online + esi], 1
		jz	near mergehiresnewskipsprite
		mov	edi, dword [sprx + esi]
		add	edi, edi
		cmp	edi, dword [graph_DIW_last_visible]
		ja	near mergehiresnewskipsprite
		mov	ebp, dword [esp + 4]
		lea	edi, [edi + ebp + graph_line_line1]
		lea	ebp, [sprite + 4*esi]

; determine whether this sprite is in front or behind the playfield
; if ecx is bigger than esi, then sprite must be drawn in front

		xor	eax, eax
		cmp	ecx, esi
		jbe	mergehiresnewsetuploop
		mov	eax, 010000h
mergehiresnewsetuploop:
		push	ecx
		push	esi
		lea	esi, [sprite_translate+eax]
		xor	eax, eax
		xor	ebx, ebx
mergehiresnewloop2:
		mov	edx, dword [ebp]
		mov	ecx, dword [edi]
		mov	al, dl
		mov	bl, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	al, dh
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		ror	edx, 16
		mov	dword [edi], ecx
		mov	ecx, dword [4 + edi]
		mov	bl, dl
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	bl, dh
		mov	al, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [4 + edi], ecx

		mov	edx, dword [4 + ebp]
		mov	ecx, dword [8 + edi]
		mov	al, dl
		mov	bl, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	al, dh
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		ror	edx, 16
		mov	dword [8 + edi], ecx
		mov	ecx, dword [12 + edi]
		mov	bl, dl
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	bl, dh
		mov	al, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [12 + edi], ecx

		mov	edx, dword [8 + ebp]
		mov	ecx, dword [16 + edi]
		mov	al, dl
		mov	bl, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	al, dh
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		ror	edx, 16
		mov	dword [16 + edi], ecx
		mov	ecx, dword [20 + edi]
		mov	bl, dl
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	bl, dh
		mov	al, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [20 + edi], ecx

		mov	edx, dword [12 + ebp]
		mov	ecx, dword [24 + edi]
		mov	al, dl
		mov	bl, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	al, dh
		mov	bl, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		ror	edx, 16
		mov	dword [24 + edi], ecx
		mov	ecx, dword [28 + edi]
		mov	bl, dl
		mov	al, dl
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	bl, dh
		mov	al, dh
		mov	ah, cl
		mov	bh, ch
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		ror	ecx, 16
		mov	dword [28 + edi], ecx

		pop	esi
		pop	ecx

mergehiresnewskipsprite:
		add	esi, 4
		cmp	esi, 32
		jne	near mergehiresnewloop
		ret


		FALIGN32

mergedualsprites:
	
mergedualloresloop:
		test	dword [sprite_online + esi], 1
		jz	near mergedualloresloopend
		mov	edi, dword [sprx + esi]
		cmp	edi, dword [graph_DIW_last_visible]
		ja	near mergedualloresloopend
		mov	ebp, dword [esp + 4]
		lea	edi, [edi + ebp + graph_line_line1]
		lea	eax, [sprite + 4*esi]
		xor	ebp, ebp



; Merge with PF1
; determine whether this sprite is in front or behind the playfield
; if ecx is bigger than esi, then sprite must be drawn in front

; test, sprite in front of PF1? Draw it there
;	sprite in front of PF2? Draw it there
;	else draw it behind PF2
mergedualloresPF1:
		mov	ecx, dword [bplcon2]
		test	ecx, 040h
		jnz	mergedualloresevenpri
		and	ecx, 7h
		shl	ecx, 3
		cmp	ecx, esi
		ja	near mergedualloresPF1loopinfront2
		mov	ecx, dword [bplcon2]
		and	ecx, 038h
		cmp	ecx, esi
		ja	near mergedualloresPF2loopinfront2
		jmp	mergedualloresPF2loopbehind2
mergedualloresevenpri:
		and	ecx, 38h
		cmp	ecx, esi
		ja	near mergedualloresPF2loopinfront2
		mov	ecx, dword [bplcon2]
		and	ecx, 07h
		shl	ecx, 3
		cmp	ecx, esi
		ja	mergedualloresPF1loopinfront2

mergedualloresPF1loopbehind2:
		mov	edx, dword [eax + ebp]
		mov	ecx, dword [edi + ebp]
		test	cl, cl
		jnz	mdlb1
		test	dl, dl
		jz	mdlb1
		mov	cl, dl
mdlb1:
		test	ch, ch
		jnz	mdlb2
		test	dh, dh
		jz	mdlb2
		mov	ch, dh
mdlb2:
		ror	edx, 16
		ror	ecx, 16
		test	cl, cl
		jnz	mdlb3
		test	dl, dl
		jz	mdlb3
		mov	cl, dl
mdlb3:
		test	ch, ch
		jnz	mdlb4
		test	dh, dh
		jz	mdlb4
		mov	ch, dh
mdlb4:
		ror	ecx, 16
		mov	dword [edi + ebp], ecx
		add	ebp, 4
		cmp	ebp, 16
		jne	mergedualloresPF1loopbehind2
		jmp	mergedualloresloopend

		FALIGN32

mergedualloresPF1loopinfront2:
		mov	edx, dword [eax + ebp]
		mov	ecx, dword [edi + ebp]
		test	dl, dl
		jz	mdl1
		mov	cl, dl
mdl1:
		test	dh, dh
		jz	mdl2
		mov	ch, dh
mdl2:
		ror	edx, 16
		ror	ecx, 16
		test	dl, dl
		jz	mdl3
		mov	cl, dl
mdl3:
		test	dh, dh
		jz	mdl4
		mov	ch, dh
mdl4:
		ror	ecx, 16
		mov	dword [edi + ebp], ecx
		add	ebp, 4
		cmp	ebp, 16
		jne	mergedualloresPF1loopinfront2
		jmp	mergedualloresloopend

		FALIGN32

mergedualloresPF2loopbehind2:
		mov	edx, dword [eax + ebp]
		mov	ecx, dword [edi + ebp - graph_line_line1 + graph_line_line2]
		test	cl, cl
		jnz	mdlpf2b1
		test	dl, dl
		jz	mdlpf2b1
		mov	cl, dl
mdlpf2b1:
		test	ch, ch
		jnz	mdlpf2b2
		test	dh, dh
		jz	mdlpf2b2
		mov	ch, dh
mdlpf2b2:
		ror	edx, 16
		ror	ecx, 16
		test	cl, cl
		jnz	mdlpf2b3
		test	dl, dl
		jz	mdlpf2b3
		mov	cl, dl
mdlpf2b3:
		test	ch, ch
		jnz	mdlpf2b4
		test	dh, dh
		jz	mdlpf2b4
		mov	ch, dh
mdlpf2b4:
		ror	ecx, 16
		mov	dword [edi + ebp - graph_line_line1 + graph_line_line2], ecx
		add	ebp, 4
		cmp	ebp, 16
		jne	mergedualloresPF2loopbehind2

		jmp	mergedualloresloopend

		FALIGN32

mergedualloresPF2loopinfront2:
		mov	edx, dword [eax + ebp]
		mov	ecx, dword [edi + ebp - graph_line_line1 + graph_line_line2]
		test	dl, dl
		jz	mdlpf21
		mov	cl, dl
mdlpf21:
		test	dh, dh
		jz	mdlpf22
		mov	ch, dh
mdlpf22:
		ror	edx, 16
		ror	ecx, 16
		test	dl, dl
		jz	mdlpf23
		mov	cl, dl
mdlpf23:
		test	dh, dh
		jz	mdlpf24
		mov	ch, dh
mdlpf24:
		ror	ecx, 16
		mov	dword [edi + ebp - graph_line_line1 + graph_line_line2], ecx
		add	ebp, 4
		cmp	ebp, 16
		jne	mergedualloresPF2loopinfront2
mergedualloresloopend:
skipdualloressprite:
		add	esi, 4
		cmp	esi, 32
		jne	near mergedualloresloop
		ret


;-------------------------------------------------------------------------------
; Decide delay of sprite ptr update
; ebx - cycle raster xpos must be past to trigger delay
;-------------------------------------------------------------------------------


		FALIGN32

decidedelay:	cmp	dword [sprite_write_real], 1
		je	ddnodelay
		cmp	ebx, dword [graph_raster_x]
		ja	ddnodelay
		mov	ebx, dword [sprite_write_next]
		mov	dword [sprite_write_buffer + ebx], ecx
		mov	dword [sprite_write_buffer + 4 + ebx], edx
		add	ebx, 8
		mov	dword [sprite_write_next], ebx
		mov	ebx, 1
		ret
ddnodelay:	xor	ebx, ebx
		ret


;=======
; SPR0PT
;=======

; $dff120 - WORD


		FALIGN32

global _wspr0pth_
_wspr0pth_:	push	ebx
		mov	ebx, 0
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr0h
		pop	ebx
		ret
.wspr0h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2], dx
		ret


; $dff122 - WORD


		FALIGN32

global _wspr0ptl_
_wspr0ptl_:	push	ebx
		mov	ebx, 0
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr0l
		pop	ebx
		ret
.wspr0l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt], dx
		ret


;=======
; SPR1PT
;=======

; $dff124 - WORD


		FALIGN32

global _wspr1pth_
_wspr1pth_:	push	ebx
		mov	ebx, 4
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr1h
		pop	ebx
		ret
.wspr1h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 + 4], dx
		ret


; $dff126 - WORD


		FALIGN32

global _wspr1ptl_
_wspr1ptl_:	push	ebx
		mov	ebx, 4
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr1l
		pop	ebx
		ret
.wspr1l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 4], dx
		ret


;=======
; SPR2PT
;=======

; $dff128 - WORD


		FALIGN32

global _wspr2pth_
_wspr2pth_:	push	ebx
		mov	ebx, 8
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr2h
		pop	ebx
		ret
.wspr2h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 + 8], dx
		ret


; $dff12a - WORD

		FALIGN32

global _wspr2ptl_
_wspr2ptl_:	push	ebx
		mov	ebx, 8
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr2l
		pop	ebx
		ret
.wspr2l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 8], dx
		ret


;=======
; SPR3PT
;=======

; $dff12c - WORD


		FALIGN32

global _wspr3pth_
_wspr3pth_:	push	ebx
		mov	ebx, 12
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr3h
		pop	ebx
		ret
.wspr3h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 + 12],dx
		ret


; $dff12e - WORD


		FALIGN32

global _wspr3ptl_
_wspr3ptl_:	push	ebx
		mov	ebx, 12
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr3l
		pop	ebx
		ret
.wspr3l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 12], dx
		ret


;=======
; SPR4PT
;=======

; $dff130 - WORD


		FALIGN32

global _wspr4pth_
_wspr4pth_:	push	ebx
		mov	ebx, 16
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr4h
		pop	ebx
		ret
.wspr4h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 + 16], dx
		ret


; $dff132 - WORD


		FALIGN32

global _wspr4ptl_
_wspr4ptl_:	push	ebx
		mov	ebx, 16
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr4l
		pop	ebx
		ret
.wspr4l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 16], dx
		ret


;=======
; SPR5PT
;=======

; $dff134 - WORD


		FALIGN32

global _wspr5pth_
_wspr5pth_:	push	ebx
		mov	ebx, 20
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr5h
		pop	ebx
		ret
.wspr5h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 + 20], dx
		ret


; $dff136 - WORD


		FALIGN32

global _wspr5ptl_
_wspr5ptl_:	push	ebx
		mov	ebx, 20
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr5l
		pop	ebx
		ret
.wspr5l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 20], dx
		ret


;=======
; SPR6PT
;=======

; $dff138 - WORD


		FALIGN32

global _wspr6pth_
_wspr6pth_:	push	ebx
		mov	ebx, 24
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr6h
		pop	ebx
		ret
.wspr6h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 +24], dx
		ret


; $dff13a - WORD


		FALIGN32

global _wspr6ptl_
_wspr6ptl_:	push	ebx
		mov	ebx, 24
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr6l
		pop	ebx
		ret
.wspr6l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 24], dx
		ret


;=======
; SPR7PT
;=======

; $dff13c - WORD


		FALIGN32

global _wspr7pth_
_wspr7pth_:	push	ebx
		mov	ebx, 28
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr7h
		pop	ebx
		ret
.wspr7h:	pop	ebx
		and	edx, 01fh		; Store 21 bit (2 MB)
		mov	word [sprpt + 2 +28], dx
		ret


; $dff13e - WORD


		FALIGN32

global _wspr7ptl_
_wspr7ptl_:	push	ebx
		mov	ebx, 28
		add	ebx, dword [sprite_delay]
		call	decidedelay
		test	ebx, 1
		jz	.wspr7l
		pop	ebx
		ret
.wspr7l:	pop	ebx
		and	edx, 0fffeh		; No odd adresses
		mov	word [sprpt + 28], dx
		ret


;==============================================================================
; Sprite IO-register access wrappers, calls routines in graph.c
;==============================================================================


		FALIGN32

global _wsprxpos_
_wsprxpos_:
		WSPRXPOS_CWRAP
		ret


		FALIGN32

global _wsprxctl_
_wsprxctl_:
		WSPRXCTL_CWRAP
		ret


		FALIGN32

global _wsprxdata_
_wsprxdata_:
		WSPRXDATA_CWRAP
		ret


		FALIGN32

global _wsprxdatb_
_wsprxdatb_:
		WSPRXDATB_CWRAP
		ret


FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
