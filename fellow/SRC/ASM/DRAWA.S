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


;-----------------------------------------------------------------------------
; Enable this for detailed profiling, log written to drawprofile.txt
; It can be imported into excel for better viewing
;
;%define DRAW_TSC_PROFILE
;
;-----------------------------------------------------------------------------

%ifdef DRAW_TSC_PROFILE
CDATAEXTERN dlsbg1x8_tmp
CDATAEXTERN dlsbg1x8
CDATAEXTERN dlsbg1x8_times
CDATAEXTERN dlsbg1x8_pixels

CDATAEXTERN dlsbg1x8mmx_tmp
CDATAEXTERN dlsbg1x8mmx
CDATAEXTERN dlsbg1x8mmx_times
CDATAEXTERN dlsbg1x8mmx_pixels

CDATAEXTERN dlsbg2x8_tmp
CDATAEXTERN dlsbg2x8
CDATAEXTERN dlsbg2x8_times
CDATAEXTERN dlsbg2x8_pixels

CDATAEXTERN dlsbg2x8mmx_tmp
CDATAEXTERN dlsbg2x8mmx
CDATAEXTERN dlsbg2x8mmx_times
CDATAEXTERN dlsbg2x8mmx_pixels

CDATAEXTERN dlsbg1x16_tmp
CDATAEXTERN dlsbg1x16
CDATAEXTERN dlsbg1x16_times
CDATAEXTERN dlsbg1x16_pixels

CDATAEXTERN dlsbg1x16mmx_tmp
CDATAEXTERN dlsbg1x16mmx
CDATAEXTERN dlsbg1x16mmx_times
CDATAEXTERN dlsbg1x16mmx_pixels

CDATAEXTERN dlsbg2x16_tmp
CDATAEXTERN dlsbg2x16
CDATAEXTERN dlsbg2x16_times
CDATAEXTERN dlsbg2x16_pixels

CDATAEXTERN dlsbg2x16mmx_tmp
CDATAEXTERN dlsbg2x16mmx
CDATAEXTERN dlsbg2x16mmx_times
CDATAEXTERN dlsbg2x16mmx_pixels

CDATAEXTERN dlsbg1x24_tmp
CDATAEXTERN dlsbg1x24
CDATAEXTERN dlsbg1x24_times
CDATAEXTERN dlsbg1x24_pixels

CDATAEXTERN dlsbg1x24mmx_tmp
CDATAEXTERN dlsbg1x24mmx
CDATAEXTERN dlsbg1x24mmx_times
CDATAEXTERN dlsbg1x24mmx_pixels

CDATAEXTERN dlsbg2x24_tmp
CDATAEXTERN dlsbg2x24
CDATAEXTERN dlsbg2x24_times
CDATAEXTERN dlsbg2x24_pixels

CDATAEXTERN dlsbg2x24mmx_tmp
CDATAEXTERN dlsbg2x24mmx
CDATAEXTERN dlsbg2x24mmx_times
CDATAEXTERN dlsbg2x24mmx_pixels

CDATAEXTERN dlsbg1x32_tmp
CDATAEXTERN dlsbg1x32
CDATAEXTERN dlsbg1x32_times
CDATAEXTERN dlsbg1x32_pixels

CDATAEXTERN dlsbg1x32mmx_tmp
CDATAEXTERN dlsbg1x32mmx
CDATAEXTERN dlsbg1x32mmx_times
CDATAEXTERN dlsbg1x32mmx_pixels

CDATAEXTERN dlsbg2x32_tmp
CDATAEXTERN dlsbg2x32
CDATAEXTERN dlsbg2x32_times
CDATAEXTERN dlsbg2x32_pixels

CDATAEXTERN dlsbg2x32mmx_tmp
CDATAEXTERN dlsbg2x32mmx
CDATAEXTERN dlsbg2x32mmx_times
CDATAEXTERN dlsbg2x32mmx_pixels
%endif


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
		test	esi, 1
		jz	%%l7
		mov	byte [esi], al
		inc	esi
		dec	ebx
%%l7:		test	esi, 2
		jz	%%l5
		mov	word [esi], ax
		add	esi, 2
		sub	ebx, 2
%%l5:		cmp	ebx, 4
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
; Draw one line segment using background color using MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x8MMX 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 8
		jb	%%l1
		test	esi, 1
		jz	%%l7
		mov	byte [esi], al
		inc	esi
		dec	ebx
%%l7:		test	esi, 2
		jz	%%l5
		mov	word [esi], ax
		add	esi, 2
		sub	ebx, 2
%%l5:		test	esi, 4
		jz	%%l6
		mov	dword [esi], eax
		sub	ebx, 4
		add	esi, 4
%%l6:		cmp	ebx, 8
		jb	%%l1
		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[esi], mm0
		sub	ebx, 8
		add	esi, 8
		cmp	ebx, 8
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

_drawLineSegmentBG1x8mmx_:
		DRAWLINESEGMENTBG1x8MMX
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
; Draw one line segment using background color using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x8MMX 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 8
		jb	%%l1
		test	esi, 2
		jz	%%l5
		mov	word [esi], ax
		add	esi, 2
		dec	ebx
%%l5:		test	esi, 4
		jz	%%l6
		mov	dword [esi], eax
		sub	ebx, 2
		add	esi, 4
%%l6:		cmp	ebx, 8
		jb	%%l1
		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[esi], mm0
		movq	[esi + 8], mm0
		sub	ebx, 8
		add	esi, 16
		cmp	ebx, 8
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

_drawLineSegmentBG2x8mmx_:
		DRAWLINESEGMENTBG2x8MMX
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
; Draw one line segment using background color with MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		16 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x16MMX 0
		mov	esi, dword [draw_buffer_current_ptr]
		cmp	ebx, 8
		jl	%%l1
		test	esi, 2
		jz	%%l5
		mov	word [esi], ax
		dec	ebx
		add	esi, 2
%%l5:		test	esi, 4
		jz	%%l6
		mov	dword [esi], eax
		sub	ebx, 2
		add	esi, 4
%%l6:		cmp	ebx, 8
		jb	%%l1
		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[esi], mm0
		movq	[esi + 8], mm0
		sub	ebx, 8
		add	esi, 16
		cmp	ebx, 8
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	word [esi], ax
		add	esi, 2
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x16mmx_:
		DRAWLINESEGMENTBG1x16MMX
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


%macro DRAWLINESEGMENTBG1x24 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]
		; eax + ecx + edx is 4 1x pixels (12 bytes)
		; edi has the fourth byte set to zero for single pixel drawing
		mov	edi, eax
		mov	ecx, eax
		mov	edx, eax
		shl	ecx, 8
		shl	edx, 8
		mov	cx, ax
		rol	eax, 8
		ror	ecx, 16   ; ecx done
		mov	al, ah
		mov	dl, ch	  ; edx done
		ror	eax, 8	  ; eax done
		cmp	ebx, 8
		jb	%%l1
		test	esi, 3
		jz	%%l2
%%l5:		mov	dword [esi], edi
		dec	ebx
		add	esi, 3
		test	esi, 3
		jnz	%%l5
%%l2:		mov	dword [esi], eax
		mov	dword [esi + 4], ecx
		mov	dword [esi + 8], edx
		sub	ebx, 4
		add	esi, 12
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], edi
		add	esi, 3
		dec	ebx
		jnz	%%l3
%%l4:		pop	ecx
		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x24_:
		DRAWLINESEGMENTBG1x24
		ret


;==============================================================================
; Draw one line segment using background color using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		24 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x24MMX 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]

		mov	edx, eax
		mov	ecx, eax
		ror	edx, 8
		and	edx, 0ff000000h
		or	eax, edx

		cmp	ebx, 8
		jb	near %%l1

		; Single pixels until framepointer is aligned by 8
		; It takes maximum 7 pixels and we have at least 8 here

		test	esi, 7
		jz	%%l6

%%l7:		mov	dword [esi], ecx
		dec	ebx
		add	esi, 3
		test	esi, 7
		jnz	%%l7

%%l5:		cmp	ebx, 8
		jb	%%l1

		; Prepares 24 bytes of color data containing 8 full pixels
%%l6:		movd	mm0, ecx
		movd	mm1, ecx
		movd	mm2, ecx
		movd	mm3, ecx
		movd	mm4, ecx
		psllq	mm1, 8
		psllq	mm2, 16
		psllq	mm3, 24
		por	mm0, mm3
		psllq	mm3, 8
		por	mm1, mm3
		psllq	mm3, 8
		por	mm2, mm3
		psllq	mm3, 8
		por	mm0, mm3
		psllq	mm3, 8
		por	mm1, mm3
		psrlq	mm4, 8
		por	mm2, mm4
		psrlq	mm4, 8
		por	mm1, mm4

%%l2:
		movq	[esi], mm0
		movq	[esi + 8], mm1
		movq	[esi + 16], mm2
		sub	ebx, 8
		add	esi, 24
		cmp	ebx, 8
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], ecx
		add	esi, 3
		dec	ebx
		jnz	%%l3
%%l4:		pop	ecx
		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG1x24mmx_:
		DRAWLINESEGMENTBG1x24MMX
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


%macro DRAWLINESEGMENTBG2x24 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]
		; eax + ecx + edx is 2 2x pixels (12 bytes)
		mov	ecx, eax
		mov	edx, eax
		shl	ecx, 8
		shl	edx, 8
		mov	cx, ax
		rol	eax, 8
		ror	ecx, 16   ; ecx done
		mov	al, ah
		mov	dl, ch	  ; edx done
		ror	eax, 8	  ; eax done
		cmp	ebx, 4
		jb	%%l1
		test	esi, 2
		jz	%%l2
%%l5:		mov	dword [esi], eax
		mov	word [esi + 4], cx
		dec	ebx
		add	esi, 6
		test	esi, 2
		jnz	%%l5
%%l2:		mov	dword [esi], eax
		mov	dword [esi + 4], ecx
		mov	dword [esi + 8], edx
		sub	ebx, 2
		add	esi, 12
		cmp	ebx, 2
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], eax
		mov	word [esi + 4], cx
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
; Draw one line segment using background color using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		24 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG2x24MMX 0
		push	ecx
		mov	esi, dword [draw_buffer_current_ptr]

		; eax + cx is one 2x pixel (6 bytes)
		mov	edx, eax
		mov	ecx, eax
		ror	edx, 8
		shr	ecx, 8
		and	edx, 0ff000000h
		or	eax, edx

		cmp	ebx, 4
		jb	near %%l1

		; Single pixels until framepointer is aligned by 8
		; It takes maximum 3 pixels and we have at least 4 here

		test	esi, 6
		jz	%%l6

%%l7:		mov	dword [esi], eax
		mov	word [esi + 4], cx
		dec	ebx
		add	esi, 6
		test	esi, 6
		jnz	%%l7

%%l5:		cmp	ebx, 4
		jb	%%l1

		; Prepares 24 bytes of color data containing 4 full 2x pixels
%%l6:		movd	mm0, eax
		movd	mm1, eax
		movd	mm2, eax
		movd	mm3, eax
		movd	mm4, eax
		psllq	mm1, 8
		psllq	mm2, 16
		psllq	mm3, 24
		por	mm0, mm3
		psllq	mm3, 8
		por	mm1, mm3
		psllq	mm3, 8
		por	mm2, mm3
		psllq	mm3, 8
		por	mm0, mm3
		psllq	mm3, 8
		por	mm1, mm3
		psrlq	mm4, 8
		por	mm2, mm4
		psrlq	mm4, 8
		por	mm1, mm4

%%l2:
		movq	[esi], mm0
		movq	[esi + 8], mm1
		movq	[esi + 16], mm2
		sub	ebx, 4
		add	esi, 24
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [esi], eax
		mov	word [esi + 4], cx
		add	esi, 6
		dec	ebx
		jnz	%%l3
%%l4:		pop	ecx
		mov	dword [draw_buffer_current_ptr], esi
%endmacro


		FALIGN32

_drawLineSegmentBG2x24mmx_:
		DRAWLINESEGMENTBG2x24MMX
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
		mov	edi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
		test	edi, 3
		jz	%%l2
		mov	dword [edi], eax
		dec	ebx
		add	edi, 4
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	dword [edi], eax
		mov	dword [4 + edi], eax
		mov	dword [8 + edi], eax
		mov	dword [12 + edi], eax
		sub	ebx, 4
		add	edi, 16
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	dword [edi], eax
		add	edi, 4
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], edi
%endmacro


		FALIGN32

_drawLineSegmentBG1x32_:
		DRAWLINESEGMENTBG1x32
		ret


;==============================================================================
; Draw one line segment using background color using MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		32 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;==============================================================================


%macro DRAWLINESEGMENTBG1x32MMX 0
		mov	edi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
		test	edi, 3
		jz	%%l5
		mov	dword [edi], eax
		dec	ebx
		add	edi, 4
		cmp	ebx, 4
		jb	%%l1
%%l5:		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[edi], mm0
		movq	[8 + edi], mm0
		sub	ebx, 4
		add	edi, 16
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l3
%%l4:		mov	dword [edi], eax
		add	edi, 4
		dec	ebx
		jnz	%%l4
%%l3:		mov	dword [draw_buffer_current_ptr], edi
%endmacro


		FALIGN32

_drawLineSegmentBG1x32mmx_:
		DRAWLINESEGMENTBG1x32MMX
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
;
; Performance: (AMD K6-2 500mhz, Geforce2 AGP 95.9 cycles per pixel
;==============================================================================


%macro DRAWLINESEGMENTBG2x32 0
		mov	edi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
%%l2:		mov	[edi], eax
		mov	[4 + edi], eax
		mov	[8 + edi], eax
		mov	[12 + edi], eax
		mov	[16 + edi], eax
		mov	[20 + edi], eax
		mov	[24 + edi], eax
		sub	ebx, 4
		mov	[28 + edi], eax
		add	edi, 32
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [edi], eax
		mov	dword [4 + edi], eax
		add	edi, 8
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], edi
%endmacro


		FALIGN32

_drawLineSegmentBG2x32_:
		DRAWLINESEGMENTBG2x32
		ret


;==============================================================================
; Draw one line segment using background color using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		32 bit RGB
;
; Input:
; ebx - Number of lores pixels to draw
; eax - Color
;
; Performance: (AMD K6-2 500mhz, Geforce2 AGP 75.3 cycles per pixel (21.5% better)
;==============================================================================


%macro DRAWLINESEGMENTBG2x32MMX 0
		mov	edi, dword [draw_buffer_current_ptr]
		cmp	ebx, 4
		jb	%%l1
		movd	mm0, eax
		movd	mm1, eax
		psllq	mm1, 32
		por	mm0, mm1
%%l2:		movq	[edi], mm0
		movq	[8 + edi], mm0
		movq	[16 + edi], mm0
		sub	ebx, 4
		movq	[24 + edi], mm0
		add	edi, 32
		cmp	ebx, 4
		jae	%%l2
%%l1:		test	ebx, ebx
		jz	%%l4
%%l3:		mov	dword [edi], eax
		mov	dword [4 + edi], eax
		add	edi, 8
		dec	ebx
		jnz	%%l3
%%l4:		mov	dword [draw_buffer_current_ptr], edi
%endmacro


		FALIGN32

_drawLineSegmentBG2x32mmx_:
		DRAWLINESEGMENTBG2x32MMX
		ret


;==============================================================================
; Draw one background line macro template
;==============================================================================

%macro drawLineBG 1
		mov	ebp, dword [4 + esp]
		mov	eax, dword [ebp + graph_line_colors]
		%1
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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x8_pixels], ebx
		TSC_BEFORE dlsbg1x8_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x8
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x8_tmp, dlsbg1x8, dlsbg1x8_times
%endif
		ret


;==============================================================================
; Draw one background line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x8mmx_
_drawLineBG1x8mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x8mmx_pixels], ebx
		TSC_BEFORE dlsbg1x8mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x8MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x8mmx_tmp, dlsbg1x8mmx, dlsbg1x8mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x8_pixels], ebx
		TSC_BEFORE dlsbg2x8_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x8
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x8_tmp, dlsbg2x8, dlsbg2x8_times
%endif
		ret


;==============================================================================
; Draw one background line using mmx instructions
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x8mmx_
_drawLineBG2x8mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x8mmx_pixels], ebx
		TSC_BEFORE dlsbg2x8mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x8MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x8mmx_tmp, dlsbg2x8mmx, dlsbg2x8mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x16_pixels], ebx
		TSC_BEFORE dlsbg1x16_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x16
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x16_tmp, dlsbg1x16, dlsbg1x16_times
%endif
		ret


;==============================================================================
; Draw one background line using MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x16mmx_
_drawLineBG1x16mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x16mmx_pixels], ebx
		TSC_BEFORE dlsbg1x16mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x16MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x16mmx_tmp, dlsbg1x16mmx, dlsbg1x16mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x16_pixels], ebx
		TSC_BEFORE dlsbg2x16_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x16
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x16_tmp, dlsbg2x16, dlsbg2x16_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x16mmx_pixels], ebx
		TSC_BEFORE dlsbg2x16mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x16MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x16mmx_tmp, dlsbg2x16mmx, dlsbg2x16mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x24_pixels], ebx
		TSC_BEFORE dlsbg1x24_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x24
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x24_tmp, dlsbg1x24, dlsbg1x24_times
%endif
		ret


;==============================================================================
; Draw one background line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x24mmx_
_drawLineBG1x24mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x24mmx_pixels], ebx
		TSC_BEFORE dlsbg1x24mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x24MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x24mmx_tmp, dlsbg1x24mmx, dlsbg1x24mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x24_pixels], ebx
		TSC_BEFORE dlsbg2x24_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x24
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x24_tmp, dlsbg2x24, dlsbg2x24_times
%endif
		ret


;==============================================================================
; Draw one background line using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x24mmx_
_drawLineBG2x24mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x24mmx_pixels], ebx
		TSC_BEFORE dlsbg2x24mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x24MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x24mmx_tmp, dlsbg2x24mmx, dlsbg2x24mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x32_pixels], ebx
		TSC_BEFORE dlsbg1x32_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x32
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x32_tmp, dlsbg1x32, dlsbg1x32_times
%endif
		ret


;==============================================================================
; Draw one background line using MMX instructions
;
; Horisontal Scale:	1x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG1x32mmx_
_drawLineBG1x32mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg1x32mmx_pixels], ebx
		TSC_BEFORE dlsbg1x32mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG1x32MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg1x32mmx_tmp, dlsbg1x32mmx, dlsbg1x32mmx_times
%endif
		ret


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
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x32_pixels], ebx
		TSC_BEFORE dlsbg2x32_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x32
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x32_tmp, dlsbg2x32, dlsbg2x32_times
%endif
		ret


;==============================================================================
; Draw one background line using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBG2x32mmx_
_drawLineBG2x32mmx_:
		mov	ebx, dword [draw_width_amiga]
%ifdef DRAW_TSC_PROFILE
		add	dword [dlsbg2x32mmx_pixels], ebx
		TSC_BEFORE dlsbg2x32mmx_tmp
%endif
		drawLineBG	DRAWLINESEGMENTBG2x32MMX
%ifdef DRAW_TSC_PROFILE
		TSC_AFTER dlsbg2x32mmx_tmp, dlsbg2x32mmx, dlsbg2x32mmx_times
%endif
		ret


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBPL1x8mmx_
_drawLineBPL1x8mmx_:
		drawLineBPL	DRAWLINESEGMENTBG1x8MMX


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	2x
; Pixel format:		8 bit indexed RGB
;
; Input:
; Line description - [4 + esp]
;==============================================================================


		FALIGN32

global _drawLineBPL2x8mmx_
_drawLineBPL2x8mmx_:
		drawLineBPL	DRAWLINESEGMENTBG2x8MMX


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		16 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x16mmx_
_drawLineBPL1x16mmx_:
		drawLineBPL	DRAWLINESEGMENTBG1x16MMX


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x24mmx_
_drawLineBPL1x24mmx_:
		drawLineBPL	DRAWLINESEGMENTBG1x24MMX


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
; Draw one bitplane line using MMX instructions
;
; Horisontal Scale:	2x
; Pixel format:		24 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x24mmx_
_drawLineBPL2x24mmx_:
		drawLineBPL	DRAWLINESEGMENTBG2x24MMX


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	1x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL1x32mmx_
_drawLineBPL1x32mmx_:
		drawLineBPL	DRAWLINESEGMENTBG1x32MMX


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
; Draw one bitplane line using mmx instructions
;
; Horisontal Scale:	2x
; Pixel format:		32 bit
;
; Input:
; Line description - [4 + esp]
;==============================================================================

		
		FALIGN32

global _drawLineBPL2x32mmx_
_drawLineBPL2x32mmx_:
		drawLineBPL	DRAWLINESEGMENTBG2x32MMX


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
		test	dword [draw_frame_skip], -1	; Maybe skip frame
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
		cmp	dword [ebp + graph_line_type], GRAPH_LINE_BPL_SKIP
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
