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
%include "data/fmem.inc"

CDATAEXTERN draw_hscale;
CDATAEXTERN graph_raster_y;
CDATAEXTERN bplcon0;
CDATAEXTERN bplcon2;
CDATAEXTERN graph_DIW_last_visible;
CDATAEXTERN graph_raster_x;

CFUNCEXTERN spriteDecode4Sprite_C;
CFUNCEXTERN spriteDecode16Sprite_C;

%define graph_line_type                         0
%define graph_line_line1			4
%define graph_line_line2			1028
%define graph_line_colors			2052
%define graph_line_DIW_first_draw		2308
%define graph_line_DIW_pixel_count		2312
%define graph_line_BG_pad_front			2316
%define graph_line_BG_pad_back			2320
%define graph_line_draw_routine			2324
%define graph_line_draw_BPL_res_routine		2328
%define graph_line_DDF_start			2332
%define graph_line_frames_left_until_BG_skip	2336
%define graph_line_sprite_ham_slot		2340
%define graph_line_bplcon2			2344
%define graph_line_reserved1			2346
%define graph_line_end				2348
	
FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

%define SPRITE_ASM

%include "func/sprite.inc"
%include "mac/sprite.mac"





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



FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
