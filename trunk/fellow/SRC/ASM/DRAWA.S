;===============================================================================
; Fellow Amiga Emulator
; Drawing of Amiga screen on host screen
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; These routines form the last stage of several which transforms Amiga
; graphics to something that can be drawn on a PC screen.
;
; * Contains routines to draw one line of normalized Amiga screen data.
; * Contains end of frame handler which will draw the graphics data collected
;   during the emulation of one frame onto the PC-screen.
; * File was changed in several ways to accomodate operation on Windows.
;   No unportable features were introduced, but since DirectX requires one to
;   allocate the framepointer, and deliver it back, Windows locks up while
;   you have it... + allocating it takes an awful lot of time :-(,
;   one additional level of copying data had to be introduced. The DOS code
;   could hold on to the framepointer forever, so in DOS, data was drawn 
;   immediately when it was produced.
; * The code has generally been cleaned up, and 24/32 bit color support has been
;   added. Together with draw.c, it also supports any screenwidth and height
;   using several scaling modes. The DOS version had some restrictions in that area.
; * The DOS code could also break the emulator in some rare cases if the
;   Amiga screen hardware registers contained really weird values.
;   Not anymore.
; * MMX is now generally used (still only if present) to write data to the
;   framebuffer using the maximum throughput. The DOS version only implemented 
;   this in selected resolutions.
;
;===============================================================================

%define DRAW_ASM

%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

FASMFILESTART
FDATASECTIONSTART

%include "data/draw.inc"
%include "data/fmem.inc"
%include "data/fellow.inc"
%include "data/graph.inc"
%include "data/draw.inc"

FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER
		
global _drawstart_
_drawstart_:				; Used to check code alignment

%include "func/graph.inc"
%include "func/copper.inc"
%include "func/sprite.inc"
%include "func/draw.inc"
%include "mac/draw.mac"
%include "mac/sprite.mac"


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x8 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	dword [esi], eax
		sub	ebx, 4
		add	esi, 4
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	byte [esi], al
		inc	esi
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x8_:
		DRAWLINESEGMENTBG1x8
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x8 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 2
		jb	%%l1
%%l2:		mov	dword [esi], eax
		sub	ebx, 2
		add	esi, 4
		cmp	ebx, 2
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	word [esi], ax
		add	esi, 2
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x8_:
		DRAWLINESEGMENTBG2x8
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	1x
; Pixel format:		16 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x16 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
		test	esi, 3
		jz	%%l2
		mov	word [esi], ax
		dec	ebx
		add	esi, 2
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	dword [esi], eax
		mov	dword [4 + esi], eax
		sub	ebx, 4
		add	esi, 8
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	word [esi], ax
		add	esi, 2
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x16_:
		DRAWLINESEGMENTBG1x16
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	2x
; Pixel format:		16 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x16 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	[esi], eax
		mov	[4 + esi], eax
		mov	[8 + esi], eax
		mov	[12 + esi], eax
		sub	ebx, 4
		add	esi, 16
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], eax
		add	esi, 4
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x16_:
		DRAWLINESEGMENTBG2x16
		ret


;==============================================================================
; Draw one line segment using background color with MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		16 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x16MMX 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 8
		jl	%%l1
		test	esi, 4
		jz	%%l5
		mov	[esi], eax
		dec	ebx
		add	esi, 4
		cmp	ebx, 8
		jl	%%l1
%%l5:		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[esi], mm0
		movq	[esi + 8], mm0
		movq	[esi + 16], mm0
		movq	[esi + 24], mm0
		sub	ebx, 8
		add	esi, 32
		cmp	ebx, 8
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], eax
		add	esi, 4
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x16mmx_:
		DRAWLINESEGMENTBG2x16MMX
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	1x
; Pixel format:		24 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================
; Terrible piece of code, please optimize....


%macro DRAWLINESEGMENTBG1x24 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]
		mov	ecx, eax
		shr	ecx, 16
		cmp	ebx, 4
		jb	%%l1
		test	esi, 3
		jz	%%l2
		mov	word [esi], ax
		mov	byte [esi + 2], cl
		dec	ebx
		add	esi, 3
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	word [esi], ax
		mov	byte [esi + 2], cl
		mov	word [esi + 3], ax
		mov	byte [esi + 5], cl
		mov	word [esi + 6], ax
		mov	byte [esi + 8], cl
		mov	word [esi + 9], ax
		mov	byte [esi + 11], cl
		sub	ebx, 4
		add	esi, 12
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	word [esi], ax
		mov	byte [esi + 2], cl
		add	esi, 3
		dec	ebx
		jnz	%%l4
%%l3:		pop	ecx
		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x24_:
		DRAWLINESEGMENTBG1x24
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	2x
; Pixel format:		24 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================
; Terrible piece of code, please optimize....


%macro DRAWLINESEGMENTBG2x24 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]
		mov	ecx, eax
		shr	ecx, 16
		cmp	ebx, 2
		jb	%%l1
%%l2:		mov	word [esi], ax
		mov	byte [esi + 2], cl
		mov	word [esi + 3], ax
		mov	byte [esi + 5], cl
		mov	word [esi + 6], ax
		mov	byte [esi + 8], cl
		mov	word [esi + 9], ax
		mov	byte [esi + 11], cl
		add	esi, 12
		sub	ebx, 2
		cmp	ebx, 2
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	word [esi], ax
		mov	byte [esi + 2], cl
		mov	word [esi + 3], ax
		mov	byte [esi + 5], cl
		add	esi, 6
		dec	ebx
		jnz	%%l3
%%l4:		pop	ecx
		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x24_:
		DRAWLINESEGMENTBG2x24
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	1x
; Pixel format:		32 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x32 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
		test	esi, 3
		jz	%%l2
		mov	dword [esi], eax
		dec	ebx
		add	esi, 4
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	dword [esi], eax
		mov	dword [4 + esi], eax
		mov	dword [8 + esi], eax
		mov	dword [12 + esi], eax
		sub	ebx, 4
		add	esi, 16
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	dword [esi], eax
		add	esi, 4
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x32_:
		DRAWLINESEGMENTBG1x32
		ret


;==============================================================================
; Draw one line segment using background color
;
; Horisontal Scale:	2x
; Pixel format:		32 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x32 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	[esi], eax
		mov	[4 + esi], eax
		mov	[8 + esi], eax
		mov	[12 + esi], eax
		mov	[16 + esi], eax
		mov	[20 + esi], eax
		mov	[24 + esi], eax
		mov	[28 + esi], eax
		add	esi, 32
		sub	ebx, 4
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], eax
		mov	dword [4 + esi], eax
		add	esi, 8
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x32_:
		DRAWLINESEGMENTBG2x32
		ret


;==============================================================================
; Draw one background line macro template
;==============================================================================

%macro drawLineBG 1
		mov	ebp, dword [4 + esp]
		mov	eax, dword [ebp + graph_line_colors]
		mov	ebx, dword [draw_width_amiga]
		%1
		ret
%endmacro


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x8_
_drawLineBG1x8_:
		drawLineBG	DRAWLINESEGMENTBG1x8


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x8_
_drawLineBG2x8_:
		drawLineBG	DRAWLINESEGMENTBG2x8


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	1x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x16_
_drawLineBG1x16_:
		drawLineBG	DRAWLINESEGMENTBG1x16


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	2x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x16_
_drawLineBG2x16_:
		drawLineBG	DRAWLINESEGMENTBG2x16


;==============================================================================
; Draw one background line using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x16mmx_
_drawLineBG2x16mmx_:
		drawLineBG	DRAWLINESEGMENTBG2x16MMX


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	1x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x24_
_drawLineBG1x24_:
		drawLineBG	DRAWLINESEGMENTBG1x24


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	2x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x24_
_drawLineBG2x24_:
		drawLineBG	DRAWLINESEGMENTBG2x24


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	1x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x32_
_drawLineBG1x32_:
		drawLineBG	DRAWLINESEGMENTBG1x32


;==============================================================================
; Draw one background line
;
; Horisontal Scale:	2x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x32_
_drawLineBG2x32_:
		drawLineBG	DRAWLINESEGMENTBG2x32


;==============================================================================
; Draw one bitplane line macro template
;==============================================================================

%macro drawLineBPL 1
		mov	ebp, dword [4 + esp]
		mov	ebx, dword [ebp + graph_line_BG_pad_front]
		mov	eax, dword [ebp + graph_line_colors]
		push	ebp
		%1
		call	dword [ebp + graph_line_draw_BPL_res_routine]
		pop	ebp
		mov	ebx, dword [ebp + graph_line_BG_pad_back]
		mov	eax, dword [ebp + graph_line_colors]
		%1
		ret
%endmacro



;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBPL1x8_
_drawLineBPL1x8_:
		drawLineBPL	DRAWLINESEGMENTBG1x8


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBPL2x8_
_drawLineBPL2x8_:
		drawLineBPL	DRAWLINESEGMENTBG2x8


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	1x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x16_
_drawLineBPL1x16_:
		drawLineBPL	DRAWLINESEGMENTBG1x16


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	2x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x16_
_drawLineBPL2x16_:
		drawLineBPL	DRAWLINESEGMENTBG2x16


;==============================================================================
; Draw one bitplane line using MMX instructions for padding
;
; Horisontal Scale:	2x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x16mmx_
_drawLineBPL2x16mmx_:
		drawLineBPL	DRAWLINESEGMENTBG2x16MMX


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	1x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x24_
_drawLineBPL1x24_:
		drawLineBPL	DRAWLINESEGMENTBG1x24


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	2x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x24_
_drawLineBPL2x24_:
		drawLineBPL	DRAWLINESEGMENTBG2x24


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	1x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x32_
_drawLineBPL1x32_:
		drawLineBPL	DRAWLINESEGMENTBG1x32


;==============================================================================
; Draw one bitplane line
;
; Horisontal Scale:	2x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x32_
_drawLineBPL2x32_:
		drawLineBPL	DRAWLINESEGMENTBG2x32


;==============================================================================
; void drawLineNormal1x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal1x8_
_drawLineNormal1x8_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	near .last4
		test	edx, 3
		jz	.loop
.align4:	mov	al, byte [edi]
		mov	cl, byte [esi + eax]
		mov	byte [edx], cl
		inc	edi
		inc	edx
		dec	ebp
		test	edx, 3
		jnz	.align4
		cmp	ebp, 4
		jl	.last4
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	ecx, dword [esi + eax]
		mov	al, byte [edi + 2]
		mov	ch, byte [esi + ebx]
		mov	bl, byte [edi + 3]
		rol	ecx, 16
		mov	cl, byte [esi + eax]
		mov	ch, byte [esi + ebx]
		rol	ecx, 16
		mov	dword [edx], ecx
		add	edi, 4
		add	edx, 4
		sub	ebp, 4
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	byte [edx], cl
		inc	edi
		inc	edx
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal2x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal2x8_
_drawLineNormal2x8_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	near .last4
		test	edx, 2
		jz	.loop
		mov	al, byte [edi]
		mov	cx, word [esi + eax]
		mov	word [edx], cx
		inc	edi
		add	edx, 2
		dec	ebp
		cmp	ebp, 4
		jl	.last4
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	ecx, dword [esi + ebx]
		mov	bl, byte [edi + 2]
		mov	cx, word [esi + eax]
		mov	al, byte [edi + 3]
		mov	dword [edx], ecx
		mov	ecx, dword [esi + eax]
		mov	cx, word [esi + ebx]
		mov	dword [edx + 4], ecx
		add	edi, 4
		add	edx, 8
		sub	ebp, 4
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	word [edx], cx
		inc	edi
		add	edx, 2
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal1x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	1x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal1x16_
_drawLineNormal1x16_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	.last4
		test	edx, 2
		jz	.loop
		mov	bl, byte [edi]
		inc	edi
		dec	ebp
		mov	cx, word [esi + ebx]
		mov	word [edx], cx
		add	edx, 2
		cmp	ebp, 4
		jl	.last4
.loop:		mov	bl, byte [edi + 1]
		sub	ebp, 4
		mov	al, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	bl, byte [edi + 3]
		mov	cx, word [esi + eax]
		mov	al, byte [edi + 2]
		mov	dword [edx], ecx
		mov	ecx, dword [esi + ebx]
		add	edi, 4
		mov	cx, word [esi + eax]
		mov	dword [4 + edx], ecx
		add	edx, 8
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	word [edx], cx
		inc	edi
		add	edx, 2
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal1x16mmx(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels and using MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Scratch
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
; mm0 - Pixel 1 bit-pattern and final pattern
; mm1 - Pixel 2 bit-pattern
; mm2 - Pixel 3 bit-pattern
; mm3 - Pixel 4 bit-pattern
; mm4 - Pixel 1 mask
; mm5 - Pixel 2 mask
; mm6 - Pixel 3 mask
; mm7 - Pixel 4 mask
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal1x16mmx_
_drawLineNormal1x16mmx_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	near .last4
		test	edx, 6
		jz	.loopsetup
.align8:	mov	bl, byte [edi]
		inc	edi
		dec	ebp
		mov	cx, word [esi + ebx]
		mov	word [edx], cx
		add	edx, 2
		test	edx, 6
		jnz	.align8
		cmp	ebp, 4
		jl	near .last4
.loopsetup:	mov	ecx, 0ffffh
		movd	mm4, ecx
		movq	mm5, mm4
		movq	mm6, mm4
		movq	mm7, mm4
		psllq	mm5, 16
		psllq	mm6, 32
		psllq	mm7, 48
		xor	ecx, ecx
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	cl, byte [edi + 2]
		movd	mm0, [esi + eax]
		mov	al, byte [edi + 3]
		add	edi, 4
		sub	ebp, 4			
		movd	mm1, [esi + ebx]
		movq	mm2, [esi - 4 + ecx]
		movq	mm3, [esi - 4 + eax]
		pand	mm0, mm4
		pand	mm1, mm5
		pand	mm2, mm6
		pand	mm3, mm7
		por	mm0, mm1
		por	mm2, mm3
		por	mm0, mm2
		movq	[edx], mm0
		add	edx, 8			      
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	word [edx], cx
		inc	edi
		add	edx, 2
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal2x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	2x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal2x16_
_drawLineNormal2x16_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	.last4
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	ecx, dword [esi + eax]
		mov	dword [edx], ecx
		mov	al, byte [edi + 2]
		mov	ecx, dword [esi + ebx]
		mov	dword [4 + edx], ecx
		mov	bl, byte [edi + 3]
		mov	ecx, dword [esi + eax]
		mov	dword [8 + edx], ecx
		add	edi, 4
		mov	ecx, dword [esi + ebx]
		sub	ebp, 4
		mov	dword [12 + edx], ecx
		add	edx, 16
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		inc	edi
		mov	ecx, dword [esi + ebx]
		mov	dword [edx], ecx
		add	edx, 4
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal2x16mmx(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels and using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Scratch
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
; mm0 - Pixel 1 and 2 bit-pattern
; mm1 - Pixel 3 and 4 bit-pattern
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal2x16mmx_
_drawLineNormal2x16mmx_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		xor	ebx, ebx
		xor	eax, eax
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	near .last4
		test	edx, 4
		jz	.loopsetup
.align8:	mov	bl, byte [edi]
		inc	edi
		dec	ebp
		mov	ecx, dword [esi + ebx]
		mov	dword [edx], ecx
		add	edx, 4
		test	edx, 4
		jnz	.align8
		cmp	ebp, 4
		jl	near .last4
.loopsetup:	mov	ecx, 0ffffffffh
		movd	mm4, ecx
		xor	ecx, ecx
		psllq	mm4, 32
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	cl, byte [edi + 2]
		sub	ebp, 4
		movd	mm0, [esi + eax]
		mov	al, byte [edi + 3]
		add	edi, 4
		movq	mm1, [esi + ebx - 4]
		movd	mm2, [esi + ecx]
		movq	mm3, [esi + eax - 4]
		pand	mm1, mm4
		pand	mm3, mm4
		por	mm0, mm1
		por	mm2, mm3
		movq	[edx], mm0
		movq	[edx + 8], mm2
		add	edx, 16		      
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		inc	edi
		mov	ecx, dword [esi + ebx]
		mov	dword [edx], ecx
		add	edx, 4
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal1x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	1x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================
; Terrible piece of code, please optimize....


		FALIGN32

global _drawLineNormal1x24_
_drawLineNormal1x24_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		mov	al, byte [edi]
		mov	ecx, dword [esi + eax]
		mov	word [edx], cx
		shr	ecx, 16
		mov	byte [edx + 2], cl
		add	edx, 3
		inc	edi
		dec	ebp
		jnz	.loop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal2x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	2x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal2x24_
_drawLineNormal2x24_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 0
		jle	.exit
.loop:		mov	al, byte [edi]
		mov	ecx, dword [esi + eax]
		mov	ebx, ecx
		shr	ebx, 16
		mov	word [edx], cx
		mov	byte [edx + 2], bl
		mov	word [edx + 3], cx
		mov	byte [edx + 5], bl
		inc	edi
		add	edx, 6
		dec	ebp
		jnz	.loop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal1x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	1x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal1x32_
_drawLineNormal1x32_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	.last4
.loop:		mov	al, byte [edi]
		sub	ebp, 4
		mov	bl, byte [edi + 1]
		mov	ecx, dword [esi + eax]
		mov	al, byte [edi + 2]
		mov	dword [edx], ecx
		mov	ecx, dword [esi + ebx]
		mov	bl, byte [edi + 3]
		mov	dword [edx + 4], ecx
		mov	ecx, dword [esi + eax]
		add	edi, 4
		mov	dword [edx + 8], ecx
		mov	ecx, dword [esi + ebx]
		mov	dword [edx + 12], ecx
		add	edx, 16
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		mov	ecx, dword [esi + ebx]
		mov	dword [edx], ecx
		inc	edi
		add	edx, 4
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineNormal2x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line using normal pixels
;
; Horisontal Scale:	2x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineNormal2x32_
_drawLineNormal2x32_:
		mov	esi, dword [4 + esp]
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [edi + esi + graph_line_line1]
		add	esi, graph_line_colors
		cmp	ebp, 4
		jl	.last4
.loop:		mov	al, byte [edi]
		mov	bl, byte [edi + 1]
		mov	ecx, dword [esi + eax]
		mov	dword [edx], ecx
		mov	dword [edx + 4], ecx
		mov	al, byte [edi + 2]
		mov	ecx, dword [esi + ebx]
		mov	dword [edx + 8], ecx
		mov	dword [edx + 12], ecx
		mov	bl, byte [edi + 3]
		mov	ecx, dword [esi + eax]
		mov	dword [edx + 16], ecx
		mov	dword [edx + 20], ecx
		add	edi, 4
		mov	ecx, dword [esi + ebx]
		sub	ebp, 4
		mov	dword [edx + 24], ecx
		mov	dword [edx + 28], ecx
		add	edx, 32
		cmp	ebp, 4
		jae	.loop
.last4:		test	ebp, ebp
		jz	.exit
.lastloop:	mov	bl, byte [edi]
		inc	edi
		mov	ecx, dword [esi + ebx]
		mov	dword [edx], ecx
		mov	dword [edx + 4], ecx
		add	edx, 8
		dec	ebp
		jnz	.lastloop
.exit:		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual1x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual1x8_
_drawLineDual1x8_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 4
		jl	near .last4
		test	edx, 3		; Align VGA-pointer
		jz	.loop
.l2:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]
		mov	byte [edx], cl
		inc	edx
		inc	ebp
		dec	dword [esp]
		test	edx, 3
		jnz	.l2
		cmp	dword [esp], 2
		jl	.last4
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 3]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 3]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 2]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 2]
		mov	al, byte [edi + ebx]
		mov	cl, byte [esi + eax]
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 1]
		shl	ecx, 16
		mov	al, byte [edi + ebx]
		mov	ch, byte [esi + eax]	  
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	cl, byte [esi + eax]	  
		mov	dword [edx], ecx 
		add	ebp, 4
		add	edx, 4
		sub	dword [esp], 4
		cmp	dword [esp], 4
		jae	.loop
.last4:		cmp	dword [esp], 0
		jle	.exit
.last4loop:	mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	byte [edx], cl 
		inc	edx
		inc	ebp
		dec	dword [esp]
		jnz	.last4loop
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual2x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual2x8_
_drawLineDual2x8_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 4
		jl	near .last4
		test	edx, 2		; Align VGA-pointer
		jz	.loop
.l2:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]
		mov	word [edx], cx
		add	edx, 2
		inc	ebp
		dec	dword [esp]
		test	edx, 3
		jnz	.l2
		cmp	dword [esp], 2
		jl	.last4
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 1]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	cx, word [esi + eax]
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 3]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 3]
		mov	dword [edx], ecx
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 2]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 2]
		mov	al, byte [edi + ebx]
		mov	cx, word [esi + eax]
		add	ebp, 4
		mov	dword [edx + 4], ecx 
		add	edx, 8
		sub	dword [esp], 4
		cmp	dword [esp], 4
		jae	.loop
.last4:		cmp	dword [esp], 0
		jle	.exit
.last4loop:	mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	word [edx], cx 
		add	edx, 2
		inc	ebp
		dec	dword [esp]
		jnz	.last4loop
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual1x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	1x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual1x16_
_drawLineDual1x16_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 2
		jl	.last2
		test	edx, 2		; Align VGA-pointer
		jz	.loop
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]
		mov	word [edx], cx
		add	edx, 2
		inc	ebp
		dec	dword [esp]
		cmp	dword [esp], 2
		jl	.last2
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 1]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	cx, word [esi + eax]	  
		mov	dword [edx], ecx 
		add	ebp, 2
		add	edx, 4
		sub	dword [esp], 2
		cmp	dword [esp], 2
		jae	.loop
.last2:		cmp	dword [esp], 0
		jle	.exit
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	word [edx], cx 
		add	edx, 2
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual2x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	2x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual2x16_
_drawLineDual2x16_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		lea	edi, [draw_dual_translate]
		xor	ebx, ebx
		sub	esp, 4
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	ecx, dword [esi + graph_line_DIW_pixel_count]
		xor	eax, eax
		mov	dword [esp], ecx
		test	dword [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		mov	ebx, dword [ebp + esi - graph_line_colors + graph_line_line2]
		mov	ecx, dword [ebp + esi - graph_line_colors + graph_line_line1 - 1]

		and	ebx, 0ffh
		and	ecx, 0ff00h

		or	ebx, ecx

		cmp	dword [esp], 2
		jl	near .last2

.loop:

		sub	dword [esp], 2
		mov	ecx, dword [ebp + esi - graph_line_colors + graph_line_line1]

		and	ecx, 0ff00h
		mov	eax, dword [edi + ebx]							; Get Amiga color of pixel 1

		and	eax, 0ffh
		mov	ebx, dword [ebp + esi - graph_line_colors + graph_line_line2 + 1]

		add	ebp, 2
		and	ebx, 0ffh

		mov	eax, dword [esi + eax]							; Get host color of pixel 1
		or	ebx, ecx

		mov	dword [edx], eax							; Write color of pixel 1 
		add	edx, 8

		mov	ecx, dword [ebp + esi - graph_line_colors + graph_line_line1 - 1]
		mov	eax, dword [edi + ebx]							; Get Amiga color of pixel 2

		and	eax, 0ffh
		mov	ebx, dword [ebp + esi - graph_line_colors + graph_line_line2]

		and	ecx, 0ff00h
		and	ebx, 0ffh

		mov	eax, dword [esi + eax]							; Get host color of pixel 2
		or	ebx, ecx

		cmp	dword [esp], 2
		mov	dword [edx - 4], eax							; Write color of pixel 2 

		jae	.loop

.last2:		cmp	dword [esp], 0
		jle	.exit

		mov	eax, dword [edi + ebx]
		and	eax, 0ffh

		mov	ecx, dword [esi + eax]	  
		mov	dword [edx], ecx 
		add	edx, 4
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual1x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	1x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual1x24_
_drawLineDual1x24_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 0
		jle	.exit
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	ebx, ecx
		shr	ebx, 16
		mov	word [edx], cx
		mov	byte [edx + 2], bl
		add	edx, 3
		inc	ebp
		dec	dword [esp]
		jnz	.loop
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual2x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	2x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual2x24_
_drawLineDual2x24_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 0
		jle	.exit
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	ebx, ecx
		shr	ebx, 16
		mov	word [edx], cx
		mov	byte [edx + 2], bl
		mov	word [edx + 3], cx
		mov	byte [edx + 5], bl
		add	edx, 6
		inc	ebp
		dec	dword [esp]
		jnz	.loop
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual1x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	1x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual1x32_
_drawLineDual1x32_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 2
		jl	.last2
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 1]
		mov	al, byte [edi + ebx]
		mov	dword [edx], ecx 
		mov	ecx, dword [esi + eax]	  
		add	ebp, 2
		mov	dword [edx + 4], ecx 
		add	edx, 8
		sub	dword [esp], 2
		cmp	dword [esp], 2
		jae	.loop
.last2:		cmp	dword [esp], 0
		jle	.exit
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	dword [edx], ecx 
		add	edx, 4
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineDual2x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line mixing two playfields
;
; Horisontal Scale:	2x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax   - Scratch
; ebx   - Dual translate table index
; ecx   - Final PC pixel bitpattern
; edx   - Framebuffer pointer
; [esp] - Pixels left to draw
; ebp   - graph_raster_x
; edi   - Dual translate table ptr
; esi   - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineDual2x32_
_drawLineDual2x32_:
		mov	edx, dword [draw_buffer_current_ptr]
		mov	esi, dword [4 + esp]
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	eax, dword [esi + graph_line_DIW_pixel_count]
		sub	esp, 4
		mov	dword [esp], eax
		xor	ebx, ebx
		xor	eax, eax
		lea	edi, [draw_dual_translate]
		test	byte [esi + graph_line_bplcon2], 040h
		jnz	.l1
		add	edi, 010000h
.l1:		add	esi, graph_line_colors
		cmp	dword [esp], 2
		jl	.last2
.loop:		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1 + 1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2 + 1]
		mov	dword [edx], ecx 
		mov	dword [edx + 4], ecx 
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		add	ebp, 2
		mov	dword [edx + 8], ecx 
		mov	dword [edx + 12], ecx 
		add	edx, 16
		sub	dword [esp], 2
		cmp	dword [esp], 2
		jae	.loop
.last2:		cmp	dword [esp], 0
		jle	.exit
		mov	bh, byte [ebp + esi - graph_line_colors + graph_line_line1]
		mov	bl, byte [ebp + esi - graph_line_colors + graph_line_line2]
		mov	al, byte [edi + ebx]
		mov	ecx, dword [esi + eax]	  
		mov	dword [edx], ecx 
		mov	dword [edx + 4], ecx 
		add	edx, 8
.exit:		add	esp, 4
		mov	dword [draw_buffer_current_ptr], edx
		ret


;==============================================================================
; void drawLineHAM1x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineHAM1x8_
_drawLineHAM1x8_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup

		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		and	ecx, 0ffh
		mov	ecx, dword [draw_8bit_to_color + 4*ecx]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		push	edi
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		and	ecx, 0ffh
		mov	ecx, dword [draw_8bit_to_color + 4*ecx]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		push	ecx
		and	ecx, 0fffh
		mov	ecx, dword [draw_color_table + ecx*4]
		mov	byte [edx], cl
		pop	ecx
		inc	edi
		inc	edx
		dec	ebp
		jnz	.loop
		pop	edi
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM1x8_CWRAP
		ret


;==============================================================================
; void drawLineHAM2x8(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineHAM2x8_
_drawLineHAM2x8_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup

		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		and	ecx, 0ffh
		mov	ecx, dword [draw_8bit_to_color + 4*ecx]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		and	ecx, 0ffh
		mov	ecx, dword [draw_8bit_to_color + 4*ecx]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		push	ecx
		and	ecx, 0fffh
		mov	ecx, dword [draw_color_table + ecx*4]
		mov	word [edx], cx
		pop	ecx
		inc	edi
		add	edx, 2
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM2x8_CWRAP
		ret


;==============================================================================
; void drawLineHAM1x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	1x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineHAM1x16_
_drawLineHAM1x16_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup

		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		mov	word [edx], cx
		inc	edi
		add	edx, 2
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM1x16_CWRAP
		ret


;==============================================================================
; void drawLineHAM2x16(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	2x
; Pixel format:		15/16 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32


global _drawLineHAM2x16_
_drawLineHAM2x16_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup


		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		mov	ecx, eax
		shl	eax, 16
		or	eax, ecx
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		mov	ecx, eax
		shl	eax, 16
		or	eax, ecx
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		mov	dword [edx], ecx
		inc	edi
		add	edx, 4
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM2x16_CWRAP
		ret


;==============================================================================
; void drawLineHAM1x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	1x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineHAM1x24_
_drawLineHAM1x24_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup

		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		push	ecx
		mov	word [edx], cx
		shr 	ecx, 16
		mov	byte [edx + 2], cl
		pop	ecx
		inc	edi
		add	edx, 3
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM1x24_CWRAP
		ret


;==============================================================================
; void drawLineHAM2x24(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	2x
; Pixel format:		24 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32


global _drawLineHAM2x24_
_drawLineHAM2x24_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup


		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		mov	ecx, eax
		shl	eax, 16
		or	eax, ecx
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		mov	ecx, eax
		shl	eax, 16
		or	eax, ecx
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		push	ecx
		push	ebx
		mov	ebx, ecx
		shr	ebx, 16
		mov	word [edx], cx
		mov	byte [edx + 2], bl
		mov	word [edx + 3], cx
		mov	byte [edx + 5], bl
		pop	ebx
		pop	ecx
		inc	edi
		add	edx, 6
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM2x24_CWRAP
		ret

;==============================================================================
; void drawLineHAM1x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	1x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32

global _drawLineHAM1x32_
_drawLineHAM1x32_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup

		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		mov	dword [edx], ecx
		inc	edi
		add	edx, 4
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM1x32_CWRAP
		ret


;==============================================================================
; void drawLineHAM2x32(graph_line *linedescription);
;
; Description:
; ------------
; Draw one line of HAM data
; HAM state must be accumulated from the start of the line, even if pixels are
; not visible.
;
; Horisontal Scale:	2x
; Pixel format:		32 bit RGB
;
; Input:
; ------
; Line description - [4 + esp]
;
; Register usage:
; ---------------
; eax - Scratch
; ebx - Scratch
; ecx - Hold color, final PC pixel bitpattern
; edx - Framebuffer pointer
; ebp - Pixels left to draw
; edi - graph_raster_x + linedescription->line1
; esi - linedescription->colors
;
; Returns:
; --------
; Nothing
;==============================================================================


		FALIGN32


global _drawLineHAM2x32_
_drawLineHAM2x32_:
		mov	esi, dword [4 + esp]
		push	edx
		mov	ebp, dword [esi + graph_line_DIW_first_draw]
		mov	edi, dword [esi + graph_line_DDF_start]
		xor	ecx, ecx
		sub	ebp, edi
		jle	.realsetup


		;==============================================================
		; Preprocess ebp pixels
		;==============================================================

		mov	edi, dword [esi + graph_line_DDF_start]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.preloop:	xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.premodify
		mov	ecx, dword [esi + eax]
		jmp	.predraw
.premodify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.predraw:	inc	edi
		dec	ebp
		jnz	.preloop

		;==============================================================
		; Draw visible HAM pixels
		;==============================================================

.realsetup:	mov	esi, dword [8 + esp]
		mov	edi, dword [esi + graph_line_DIW_first_draw]
		mov	ebp, dword [esi + graph_line_DIW_pixel_count]
		mov	edx, dword [draw_buffer_current_ptr]
		lea	edi, [esi + edi + graph_line_line1]
		add	esi, graph_line_colors
.loop:		xor	eax, eax
		mov	al, byte [edi]
		test	al, 0c0h
		jnz	.modify
		mov	ecx, dword [esi + eax]
		jmp	.draw
.modify:	mov	ebx, eax
		and	eax, 03ch
		and	ebx, 0c0h
		shr	eax, 2
		shr	ebx, 3
		lea	ebx, [ebx + draw_HAM_modify_table]
		push	ecx
		mov	ecx, dword [ebx + draw_HAM_modify_table_bitindex]
		shl	eax, cl
		pop	ecx
		mov	ebx, dword [ebx + draw_HAM_modify_table_holdmask]
		and	ecx, ebx
		or	ecx, eax
.draw:		mov	dword [edx], ecx
		mov	dword [edx + 4], ecx
		inc	edi
		add	edx, 8
		dec	ebp
		jnz	.loop
		mov	dword [draw_buffer_current_ptr], edx
		pop	edx
		mov	ecx, dword [4 + esp]
		SPRITEMERGEHAM2x32_CWRAP
		ret


;==============================================================================
; Drawing end of frame handler
;==============================================================================


		ALIGN32

global _drawEndOfFrame_
_drawEndOfFrame_:
		test	dword [draw_frame_skip], -1	; Frame drawn?
		jnz	near .l3
		mov	esi, dword [graph_raster_y]
		push	esi
		mov	ebp, dword [draw_top]
		mov	dword [graph_raster_y], ebp
		push	ebp
		DRAWVALIDATEBUFFERPOINTER_CWRAP
		pop	ebp
		mov	edx, dword [draw_buffer_top_ptr]
		test	edx, edx    ; Need to test for error
		jz	near .l4
		mov	edx, dword [draw_buffer_current_ptr]
		mov	edi, dword [draw_bottom]
		sub	edi, ebp
		push	eax
		mov	eax, graph_line_end
		imul	ebp, eax
		imul	eax, 314
		imul	eax, dword [draw_buffer_draw]
		lea	ebp, [ebp + eax + graph_frame]
		pop	eax
.l1:		cmp	dword [ebp + graph_line_type], GRAPH_LINE_SKIP
		je	.l2
		push	eax
		push	edx
		push	edi
		push	ebp
		call	dword [ebp + graph_line_draw_routine]
		pop	ebp
		pop	edi
		pop	edx
		pop	eax
.l2:		add	edx, eax
		mov	dword [draw_buffer_current_ptr], edx
		add	ebp, graph_line_end
		dec	edi
		jnz	.l1
		DRAWINVALIDATEBUFFERPOINTER_CWRAP
		DRAWENDOFFRAME_CWRAP
.l4:		pop	esi
		mov	dword [graph_raster_y], esi
.l3:		ret
				



		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
