;===============================================================================
; Fellow Amiga Emulator      
; Translation of Amiga graphics data to temporary format
; suitable for fast drawing on a chunky display.
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; * Some functions that do calculations about the current appearance of the
;   Amiga screen.
; * Graphics hardware register get and set functions.
; * Planar to chunky conversion. First stage of processing which ends in
;   the draw module. Refered to in that module as "normalized" Amiga graphics
;   data.
; * End of line handler which organizes storage of normalized graphics data
;   for a frame until the draw module can process it at a later stage.
;===============================================================================

%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "generic/defs.inc"
%include "mac/callconv.mac"


FASMFILESTART
FDATASECTIONSTART

%include "data/fmem.inc"
%include "data/fellow.inc"
%include "data/graph.inc"
%include "data/copper.inc"
%include "data/draw.inc"
%include "data/sprite.inc"
%include "data/bus.inc"
%include "data/cpu.inc"

CFUNCEXTERN blitFinishBlit
CFUNCEXTERN blitterCopy
CFUNCEXTERN graphCalculateWindow_C
CFUNCEXTERN graphPlayfieldOnOff
CFUNCEXTERN drawUpdateDrawmode
CFUNCEXTERN graphDecodeNOP_C

;-----------------------------------------------------------------------------
; Enable this for detailed profiling, log written to drawprofile.txt
; It can be imported into excel for better viewing
;
;%define GRAPH_TSC_PROFILE
;
;-----------------------------------------------------------------------------

%ifdef GRAPH_TSC_PROFILE
CDATAEXTERN gcc_tmp
CDATAEXTERN gcc
CDATAEXTERN gcc_times
CDATAEXTERN gcc_pixels
%endif

CDATAEXTERN bltzero
CDATAEXTERN blitter_ECS
CDATAEXTERN blitend
CDATAEXTERN blit_cycle_free
CDATAEXTERN blitterdmawaiting
CDATAEXTERN blit_started



FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _graphemstart_
_graphemstart_:				; Used to check code alignment


%include "mac/plan2c.mac"
%include "func/fmem.inc"
%include "func/sprite.inc"
%include "func/sound.inc"
%include "func/copper.inc"

;-------------------------------------------------------------------------------
; Sprite hack, try to delay effect of writes to sprxpt
;-------------------------------------------------------------------------------

_graphSpriteHack_:
		mov	ebx, dword [sprite_write_next]
		test	ebx, ebx
		jz	.no_sprite_hack
		push	eax
		xor	eax, eax
		mov	dword [sprite_write_next], eax
		mov	dword [sprite_write_real], 1

.loop_sprite_hack:

		mov	ecx, dword [sprite_write_buffer + eax]
		mov	edx, dword [sprite_write_buffer + 4 + eax]
		and	ecx, 01feh

		pushad
		push ecx
		push edx
		call	dword [memory_iobank_write + 2*ecx]
		pop edx
		pop ecx
		popad

		add	eax, 8
		cmp	eax, ebx
		jne	.loop_sprite_hack
		pop	eax

.no_sprite_hack:

		mov	dword [sprite_write_real], 0
		ret

;-------------------------------------------------------------------------------
; Copy data and compare
; [4 + esp] - source playfield
; [8 + esp] - destination playfield
; [12 + esp] - pixel count
; [16 + esp] - first pixel
; Return 1 = not equal (eax), 0 = equal
;-------------------------------------------------------------------------------

		FALIGN32

_graphCompareCopy_:
%ifdef GRAPH_TSC_PROFILE
		mov	ebx, dword [esp + 12]
		add	dword [gcc_pixels], ebx
		TSC_BEFORE gcc_tmp
%endif
		mov	eax, 0
		mov	esi, dword [esp + 4]
		mov	edx, dword [esp + 8]
		mov	ebp, dword [esp + 12]
		mov	ebx, dword [esp + 16]
		cmp	ebp, 0			; Check for 0 or less pixels
		jle	.lexit
		; Align on 4-byte boundrary
		test	ebx, 3
		jz	.dwloopsetup
.bloop1:	mov	cl, byte [esi + ebx]
		cmp	byte [edx + ebx], cl
		jne	near .copyrestb
		inc	ebx
		dec	ebp
		jz	.lexit
		test	ebx, 3
		jnz	.bloop1

.dwloopsetup:	cmp	ebp, 4
		jl	.bloop2setup
		sub	ebp, 3
.dwloop:	mov	ecx, dword [esi + ebx]
		cmp	dword [edx + ebx], ecx
		jne	.copyrestdw
		add	ebx, 4
		sub	ebp, 4
		jge	.dwloop
		add	ebp, 3

.bloop2setup:	cmp	ebp, 0
		jle	.lexit

.bloop2:	mov	cl, byte [esi + ebx]
		cmp	byte [edx + ebx], cl
		je	.l1
		mov	eax, 1
		mov	byte [edx + ebx], cl
.l1:		inc	ebx
		dec	ebp
		jnz	.bloop2
.lexit:
%ifdef GRAPH_TSC_PROFILE
		TSC_AFTER gcc_tmp, gcc, gcc_times
%endif
		ret

.bloop3:	mov	cl, byte [esi + ebx]
.copyrestb:	mov	byte [edx + ebx], cl
		inc	ebx
		dec	ebp
		jz	.lexit2
		test	ebx, 3
		jnz	.bloop3

		cmp	ebp, 4
		jl	.bloop4setup
		sub	ebp, 3
.dwloop2:	mov	ecx, dword [esi + ebx]
.copyrestdw:	mov	dword [edx + ebx], ecx
		add	ebx, 4
		sub	ebp, 4
		jge	.dwloop2
		add	ebp, 3

.bloop4setup:	cmp	ebp, 0
		jle	.lexit2

.bloop4:	mov	cl, byte [esi + ebx]
		mov	byte [edx + ebx], cl
		inc	ebx
		dec	ebp
		jnz	.bloop4
.lexit2:	mov	eax, 1
%ifdef GRAPH_TSC_PROFILE
		TSC_AFTER gcc_tmp, gcc, gcc_times
%endif
		ret

;-------------------------------------------------------------------------------
; Copy color block to line description
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescColors_:
		mov	ebp, dword [4 + esp]
		mov	eax, 0
		test	byte [fellow_mmx_detected], 1
		jz	.l1
		; MMX Version
.l1mmx:		movq	mm0, [graph_color_shadow + 8*eax]
		movq	[ebp + 8*eax + graph_line_colors], mm0
		inc	eax
		cmp	eax, 32
		jne	.l1mmx
		ret
		; Non-MMX Version
.l1:		mov	ebx, dword [graph_color_shadow + 4*eax]
		mov	dword [ebp + 4*eax + graph_line_colors], ebx
		inc	eax
		cmp	eax, 64
		jne	.l1
		ret


;-------------------------------------------------------------------------------
; Smart copy color block to line description
; [4 + esp] - linedesc struct
; Return 1 if colors have changed (eax)
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescColorsSmart_:
		mov	ebp, dword [4 + esp]
		lea	edi, [ebp + graph_line_colors]
		mov	edx, 0
		mov	eax, 0
		; Check full brightness colors
.l1:		mov	ebx, dword [graph_color_shadow + eax]
		cmp	dword [edi + eax], ebx
		je	.l4
		mov	edx, 1
.l4:		mov	dword [edi + eax], ebx
		add	eax, 4
		cmp	eax, 128
		jne	.l1
		cmp	edx, 1
		jne	.l3
		; Skip halfbrite colors when there is no change
.l2:		mov	ebx, dword [graph_color_shadow + eax]
		mov	dword [edi + eax], ebx
		add	eax, 4
		cmp	eax, 256
		jne	.l2
.l3:		mov	eax, edx
		ret


;-------------------------------------------------------------------------------
; Sets line geometry data in line description
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescGeometry_:

		;==============================================================
		; Calculate first and last visible DIW and DIW pixel count
		;==============================================================

		mov	ebp, dword [4 + esp]
		mov	eax, dword [graph_DIW_first_visible]
		mov	ebx, dword [graph_DIW_last_visible]
		mov	edi, dword [graph_DDF_start]
		mov	esi, dword [draw_left]
		mov	edx, dword [draw_right]
		xor	ecx, ecx
		test	byte [bplcon0 + 1], 080h
		jz	.l1
		shr	eax, 1
		shr	ebx, 1
		shr	edi, 1
		cmp	byte [draw_hscale], 2
		jne	.l1
		inc	ecx
.l1:		cmp	eax, esi
		jae	.l2
		mov	eax, esi
.l2:		cmp	ebx, edx
		jbe	.l3
		mov	ebx, edx
.l3:		sub	ebx, eax
		jns	.l4
		xor	ebx, ebx
.l4:		shl	eax, cl
		shl	ebx, cl
		mov	dword [ebp + graph_line_DIW_first_draw], eax
		mov	dword [ebp + graph_line_DIW_pixel_count], ebx
		mov	dword [ebp + graph_line_DDF_start], edi
		shr	eax, cl
		shr	ebx, cl

		;==============================================================
		; Calculate BG front and back pad count
		;==============================================================

		sub	edx, eax	; draw_right - DIW_first_draw
		sub	eax, esi	; DIW_first_draw - draw_left
		sub	edx, ebx	; draw_right - DIW_first_draw - DIW_pixel_count
		mov	dword [ebp + graph_line_BG_pad_front], eax
		mov	dword [ebp + graph_line_BG_pad_back], edx

		;==============================================================
		; Need to remember playfield priorities to sort dual pf
		;==============================================================
		
		mov   edx, dword [bplcon2]
		mov   dword [ebp + graph_line_bplcon2], edx

		ret


;-------------------------------------------------------------------------------
; Sets line geometry data in line description
; [4 + esp] - linedesc struct
; Return 1 if geometry has changed (eax)
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescGeometrySmart_:

		;==============================================================
		; Calculate first and last visible DIW and DIW pixel count
		;==============================================================

		mov	ebp, dword [4 + esp]
		mov	eax, dword [graph_DIW_first_visible]
		mov	ebx, dword [graph_DIW_last_visible]
		mov	edi, dword [graph_DDF_start]
		mov	esi, dword [draw_left]
		mov	edx, dword [draw_right]
		xor	ecx, ecx
		test	byte [bplcon0 + 1], 080h
		jz	.l1
		shr	eax, 1
		shr	ebx, 1
		shr	edi, 1
		cmp	byte [draw_hscale], 2
		jne	.l1
		inc	ecx
.l1:		cmp	eax, esi
		jae	.l2
		mov	eax, esi
.l2:		cmp	ebx, edx
		jbe	.l3
		mov	ebx, edx
.l3:		sub	ebx, eax
		jns	.l4
		xor	ebx, ebx
.l4:		shl	eax, cl
		shl	ebx, cl
		cmp	dword [ebp + graph_line_DDF_start], edi
		mov	dword [ebp + graph_line_DDF_start], edi
		mov	edi, 0
		je	.ls1
		mov	edi, 1
.ls1:		cmp	dword [ebp + graph_line_DIW_first_draw], eax
		mov	dword [ebp + graph_line_DIW_first_draw], eax
		je	.ls2
		mov	edi, 1
.ls2:		cmp	dword [ebp + graph_line_DIW_pixel_count], ebx
		mov	dword [ebp + graph_line_DIW_pixel_count], ebx
		je	.ls3
		mov	edi, 1
.ls3:		shr	eax, cl
		shr	ebx, cl

		;==============================================================
		; Calculate BG front and back pad count
		;==============================================================

		sub	edx, eax	; draw_right - DIW_first_draw
		sub	eax, esi	; DIW_first_draw - draw_left
		sub	edx, ebx	; draw_right - DIW_first_draw - DIW_pixel_count
		cmp	dword [ebp + graph_line_BG_pad_front], eax
		mov	dword [ebp + graph_line_BG_pad_front], eax
		je	.ls5
		mov	edi, 1
.ls5:		cmp	dword [ebp + graph_line_BG_pad_back], edx
		mov	dword [ebp + graph_line_BG_pad_back], edx
		je	.ls6
		mov	edi, 1
.ls6:
		;==============================================================
		; Need to remember playfield priorities to sort dual pf
		;==============================================================
		
		mov   edx, dword [bplcon2]
		cmp   dword [ebp + graph_line_bplcon2], edx
		mov   dword [ebp + graph_line_bplcon2], edx
		je	.ls7
		mov	edi, 1
.ls7:		mov	eax, edi
		ret


;-------------------------------------------------------------------------------
; Sets line routines for this line
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescRoutines_:


		;==============================================================
		; Set drawing routines
		;==============================================================

		mov	ebp, dword [esp + 4]
		mov	eax, dword [draw_line_routine]
		mov	ebx, dword [draw_line_BPL_res_routine]
		mov	dword [ebp + graph_line_draw_routine], eax
		mov	dword [ebp + graph_line_draw_BPL_res_routine], ebx
		ret


;-------------------------------------------------------------------------------
; Smart sets line routines for this line
; [4 + esp] - linedesc struct
; Return 1 if routines have changed (eax)
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescRoutinesSmart_:


		;==============================================================
		; Set drawing routines
		;==============================================================

		mov	ebp, dword [esp + 4]
		mov	eax, dword [draw_line_routine]
		mov	ebx, dword [draw_line_BPL_res_routine]
		
		cmp	dword [ebp + graph_line_draw_routine], eax
		mov	dword [ebp + graph_line_draw_routine], eax
		mov	eax, 0
		je	.ls1
		mov	eax, 1
.ls1:		cmp	dword [ebp + graph_line_draw_BPL_res_routine], ebx
		mov	dword [ebp + graph_line_draw_BPL_res_routine], ebx
		je	.ls2
		mov	eax, 1
.ls2:		ret


;-------------------------------------------------------------------------------
; Makes a description of this line
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescMake_:

		;==============================================================
		; Is this a bitplane or background line?
		;==============================================================

		mov	ebp, dword [esp + 4]
		mov	eax, dword [draw_line_BG_routine]
		cmp	eax, dword [draw_line_routine]
		je	.l1

		;==============================================================
		; This is a bitplane line
		;==============================================================

		mov	dword [ebp + graph_line_type], GRAPH_LINE_BPL
		push	ebp
		call	_graphLinedescColors_
		call	_graphLinedescGeometry_
		call	_graphLinedescRoutines_
		add	esp, 4
		ret

		;==============================================================
		; This is a background line
		;==============================================================

.l1:		mov	eax, dword [graph_color_shadow]
		cmp	eax, dword [ebp + graph_line_colors]
		jne	.l2
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_SKIP
		je	.lskip
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_BG
		jne	.l2

		;==============================================================
		; This line was a "background" line during the last drawing of
		; this frame buffer,
		; and it has the same color as last time.
		; We might be able to skip drawing it, we need to check the
		; flag that says it has been drawn in all of our buffers.
		; The flag is only of importance when "deinterlacing"
		;==============================================================
		
		cmp	dword [ebp + graph_line_frames_left_until_BG_skip], 0
		jne	.lnoskipyet
.lskip:		mov	dword [ebp + graph_line_type], GRAPH_LINE_SKIP
		ret
.lnoskipyet:	dec	dword [ebp + graph_line_frames_left_until_BG_skip]
		ret

		;==============================================================
		; This background line is different from the one in the buffer
		;==============================================================

.l2:		mov	dword [ebp + graph_line_type], GRAPH_LINE_BG
		mov	dword [ebp + graph_line_colors], eax
		mov	eax, 0; dword [draw_buffer_count]
		cmp	dword [draw_deinterlace], 0
		je	.lnoilace
		mov	eax, 1
.lnoilace:	mov	dword [ebp + graph_line_frames_left_until_BG_skip], eax
		push	ebp
		call	_graphLinedescRoutines_
		add	esp, 4
		ret


;-------------------------------------------------------------------------------
; Smart makes a description of this line
; [4 + esp] - linedesc struct
; Return 1 if linedesc has changed (eax)
;-------------------------------------------------------------------------------


		FALIGN32

_graphLinedescMakeSmart_:

		;==============================================================
		; Is this a bitplane or background line?
		;==============================================================

		mov	ebp, dword [esp + 4]
		mov	eax, dword [draw_line_BG_routine]
		cmp	eax, dword [draw_line_routine]
		je	.l1

		;==============================================================
		; This is a bitplane line
		;==============================================================

		cmp	dword [ebp + graph_line_type], GRAPH_LINE_BPL
		je	.besmart
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_BPL_SKIP
		je	.besmart

		mov	dword [ebp + graph_line_type], GRAPH_LINE_BPL
		push	ebp
		call	_graphLinedescColors_
		call	_graphLinedescGeometry_
		call	_graphLinedescRoutines_
		add	esp, 4
		mov	eax, 1 ; It changed
		ret

.besmart:
		sub	esp, 4
		push	ebp
		call	_graphLinedescColorsSmart_
		mov	dword [esp + 4], eax
		call	_graphLinedescGeometrySmart_
		or	dword [esp + 4], eax
		call	_graphLinedescRoutinesSmart_
		or	eax, dword [esp + 4] ; Prepare return value
		add	esp, 8
		ret

		;==============================================================
		; This is a background line
		;==============================================================

.l1:		mov	eax, dword [graph_color_shadow]
		cmp	eax, dword [ebp + graph_line_colors]
		jne	.l2
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_SKIP
		je	.lskip
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_BG
		jne	.l2

		;==============================================================
		; This line was a "background" line during the last drawing of
		; this frame buffer,
		; and it has the same color as last time.
		; We might be able to skip drawing it, we need to check the
		; flag that says it has been drawn in all of our buffers.
		; The flag is only of importance when "deinterlacing"
		;==============================================================
		
		cmp	dword [ebp + graph_line_frames_left_until_BG_skip], 0
		jne	.lnoskipyet
.lskip:		mov	dword [ebp + graph_line_type], GRAPH_LINE_SKIP
		mov	eax, 0 ; It did not change
		ret
.lnoskipyet:	dec	dword [ebp + graph_line_frames_left_until_BG_skip]
		mov	eax, 1 ; It changed
		ret

		;==============================================================
		; This background line is different from the one in the buffer
		;==============================================================

.l2:		mov	dword [ebp + graph_line_type], GRAPH_LINE_BG
		mov	dword [ebp + graph_line_colors], eax
		mov	eax, 0; dword [draw_buffer_count]
		cmp	dword [draw_deinterlace], 0
		je	.lnoilace
		mov	eax, 1
.lnoilace:	mov	dword [ebp + graph_line_frames_left_until_BG_skip], eax
		push	ebp
		call	_graphLinedescRoutines_
		add	esp, 4
		mov	eax, 1 ; It changed
		ret


;-------------------------------------------------------------------------------
; Compose the visible layout of the line
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------

		FALIGN32


_graphComposeLineOutput_:

		;==============================================================
		; Check if we need to decode bitplane data
		;==============================================================

		mov	eax, dword [draw_line_BG_routine]
		cmp	eax, dword [draw_line_routine]
		je	near .eol_draw

		;==============================================================
		; Do the planar to chunky conversion
		;==============================================================

		pushad
		call	dword [graph_decode_line_ptr]
		popad

		;==============================================================
		; Add sprites to the line image
		;==============================================================

		test	byte [sprites_online], 1
		jz	.eol_draw
		mov	ebp, dword [esp + 4]
		push	ebp
  		call	_spritesMerge_
		mov	byte [sprites_online], 0
		add	esp, 4

		;==============================================================
		; Remember line geometry for later drawing
		;==============================================================

.eol_draw:	mov	ebp, dword [esp + 4]
		push	ebp
  		call	_graphLinedescMake_
		add	esp, 4
		ret



;-------------------------------------------------------------------------------
; Smart compose the visible layout of the line
; [4 + esp] - linedesc struct
;-------------------------------------------------------------------------------

		FALIGN32


_graphComposeLineOutputSmart_:

		;==============================================================
		; Remember the basic properties of the line
		;==============================================================

		mov	ebp, dword [esp + 4]
		sub	esp, 4
		push	ebp
  		call	_graphLinedescMakeSmart_
		mov	dword [esp + 4], eax ; Remember changed state

		;==============================================================
		; Check if we need to decode bitplane data
		;==============================================================

		mov	eax, dword [draw_line_BG_routine]
		cmp	eax, dword [draw_line_routine]
		je	near .done

		;==============================================================
		; Do the planar to chunky conversion
		; Stuff the data into a temporary array
		; Then copy it and compare
		;==============================================================

		pushad
		call	dword [graph_decode_line_ptr]
		popad

		; Compare line data to old data
		mov	esi, dword graph_line1_tmp
		mov	ebp, dword [esp]
		push	dword [ebp + graph_line_DIW_first_draw]
		push	dword [ebp + graph_line_DIW_pixel_count]
		add	ebp, graph_line_line1
		push	ebp
		push	esi
		call	_graphCompareCopy_
		add	esp, 16
		or	dword [esp + 4], eax

		; If the line is dual playfield, compare second playfield too
		test	dword [bplcon0], 0400h
		jz	.no_dpf

		mov	esi, dword graph_line2_tmp
		mov	ebp, dword [esp]
		push	dword [ebp + graph_line_DIW_first_draw]
		push	dword [ebp + graph_line_DIW_pixel_count]
		add	ebp, graph_line_line2
		push	ebp
		push	esi
		call	_graphCompareCopy_
		add	esp, 16
		or	dword [esp + 4], eax

.no_dpf:

		;==============================================================
		; Add sprites to the line image
		;==============================================================

		test	byte [sprites_online], 1
		jz	.test_bpl_skip

		mov	dword [esp + 4], 1
		call	_spritesMerge_
		mov	byte [sprites_online], 0

.test_bpl_skip:
		; Final test for line skip
		mov	eax, GRAPH_LINE_BPL
		test	dword [esp + 4], 1
		jnz	.no_bpl_skip
		mov	eax, GRAPH_LINE_BPL_SKIP
.no_bpl_skip:   mov	ebp, dword [esp]
		mov	dword [ebp + graph_line_type], eax
.done:		add	esp, 8
		ret



;-------------------------------------------------------------------------------
; End of line handler for graphics
;-------------------------------------------------------------------------------


		FALIGN32

global _graphEndOfLine_
_graphEndOfLine_:

		pushad

		;==============================================================
		; Skip this frame?
		;==============================================================

		test	dword [draw_frame_skip], 0ffffffffh
		jnz	near .skip_frame

		;==============================================================
		; Update diw state
		;==============================================================

		pushad
		call graphPlayfieldOnOff
		popad

		;==============================================================
		; Skip lines before line $12
		;==============================================================

		cmp	dword [graph_raster_y], 12h
		jb	near .skip_frame

		;==============================================================
		; Make pointer to linedesc for this line
		;==============================================================

		mov	ebp, dword [graph_raster_y]
		mov	eax, graph_line_end
		imul	ebp, eax
		imul	eax, 314
		imul	eax, dword [draw_buffer_draw]
		lea	ebp, [ebp + eax + graph_frame]
		push	ebp

		;==============================================================
		; Decode sprites if DMA is enabled and raster is after line $18
		;==============================================================

		test	byte [dmacon], 20h
		jz	.skip_sprites
		cmp	dword [graph_raster_y], 18h
		jb	.skip_sprites
		call	_spritesDecode_
	
		;==============================================================
		; Sprites decoded, sprites_onlineflag is set if there are any
		;==============================================================

.skip_sprites:

		;==============================================================
		; Update pointer to drawing routine
		;==============================================================

		pushad
		call drawUpdateDrawmode
		popad

		;==============================================================
		; Check if we are clipped
		;==============================================================
		
		mov	ebp, dword [graph_raster_y]
		cmp	ebp, dword [draw_top]
		jb	near .nop_line
		cmp	ebp, dword [draw_bottom]
		jb	near .decode
		
		;==============================================================
		; Do nop decoding, no screen output needed
		;==============================================================

.nop_line:

		mov	eax, dword [draw_line_BG_routine]
		pop	ebp
		cmp	eax, dword [draw_line_routine]
		je	.draw_end
		push	dword .draw_end
		pushad
		call graphDecodeNOP_C
		popad
		ret

		;==============================================================
		; Visible line, either background or bitplanes
		;==============================================================

.decode:

		test	dword [graph_allow_bpl_line_skip], 0ffffffffh
		jz	.no_bpl_skip
		call	_graphComposeLineOutputSmart_
		jmp	.after_line
.no_bpl_skip:	
		call	_graphComposeLineOutput_
.after_line:	
		add	esp, 4

		;==============================================================
		; If diw state changing from background to bitplane output,
		; set new drawing routine pointer
		;==============================================================

.draw_end:
		
		test	byte [draw_switch_bg_to_bpl], 1
		jz	.skip_frame
		mov	edx, dword [draw_line_BPL_manage_routine]
		mov	dword [draw_line_routine], edx
		mov	byte [draw_switch_bg_to_bpl], 0

.skip_frame:

		call	_graphSpriteHack_

		popad		
		ret

		FALIGN32


FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
