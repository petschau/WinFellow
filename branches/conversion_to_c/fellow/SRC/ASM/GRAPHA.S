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

;===============================================================================
; Graphics Register Access routines 
;===============================================================================

;-------------------------------------------------------------------------------
; DMACONR - $dff002 Read
;
; return dmaconr | ((!((WOR)bltzero))<<13);
;-------------------------------------------------------------------------------


		FALIGN32

global _rdmaconr_
_rdmaconr_:	mov	edx, 0
		test	dword [bltzero], 0ffffh
		jnz	.l1
		mov	edx, 2000h	
.l1:		
		or	edx, dword [dmaconr]
		mov eax, edx
		ret


;-------------------------------------------------------------------------------
; VPOSR - $dff004 Read vpos and chipset ID bits
;
; return lof | (graph_raster_y>>8);
;-------------------------------------------------------------------------------


		FALIGN32

global _rvposr_
_rvposr_:	mov	edx, dword [lof]
		mov	dl, byte [graph_raster_y + 1]
		test	dword [blitter_ECS], 1
		jz	.l1
		or	edx, 02000h
.l1:	
		mov eax, edx	
		ret


;-------------------------------------------------------------------------------
; VHPOSR - $dff006 Read
;
; return (graph_raster_x>>1) | ((graph_raster_y & 0xff)<<8);
;-------------------------------------------------------------------------------


		FALIGN32

global _rvhposr_
_rvhposr_:	mov	edx, dword [graph_raster_x]
		shr	edx, 1
		mov	dh, byte [graph_raster_y]
		mov eax, edx
		ret


;-------------------------------------------------------------------------------
; ID - $dff07c Read
;
; return 0xffff
;-------------------------------------------------------------------------------


		FALIGN32

global _rid_
_rid_:	
		mov	edx, 0ffffh
		mov eax, edx
		ret


;-------------------------------------------------------------------------------
; VPOS - $dff02a Write
;
; lof = data & 0x8000;
;-------------------------------------------------------------------------------


		FALIGN32

global _wvpos_
_wvpos_:	and	edx, 08000h
		mov	dword [lof], edx
		ret


;-------------------------------------------------------------------------------
; SERDAT - $dff030 Write
;
; ;
;-------------------------------------------------------------------------------


		FALIGN32

global _wserdat_
_wserdat_:	;mov dword[serper_debug_register_edx], edx
			;mov dword[serper_debug_register_ecx], ecx
;			;call graphDebugFunction
		ret

;-------------------------------------------------------------------------------
; SERPER - $dff032 Write
;
; ;
;-------------------------------------------------------------------------------


		FALIGN32

global _wserper_
_wserper_:	;mov dword[serper_debug_register_edx], edx
			;mov dword[serper_debug_register_ecx], ecx
;			;call graphDebugFunction
		ret

;-------------------------------------------------------------------------------
; DIWSTRT - $dff08e Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wdiwstrt_
_wdiwstrt_:	mov	dword [diwstrt], edx
		mov	ecx, edx
		shr	ecx, 8
		and	ecx, 0ffh
		mov	dword [diwytop], ecx
		cmp	dword [diwxright], 472
		je	.l1
		and	edx, 0ffh
		jmp	.l2
.l1:		mov	edx, 88
.l2:		mov	dword [diwxleft], edx
		pushad
		call graphCalculateWindow_C
		popad
		ret
	
;-------------------------------------------------------------------------------
; DIWSTOP - $dff090 Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wdiwstop_
_wdiwstop_:	mov	ecx, edx
		shr	ecx, 8
		and	ecx, 0ffh
		mov	dword [diwstop] , edx
		test	ecx, 080h
		jnz	.l1
		add	ecx, 256
.l1:		mov	dword [diwybottom], ecx
		and	edx, 0ffh
		xor	edx, 0100h
		cmp	edx, 457
		jb	.l2
		mov	edx, 472
		mov	dword [diwxleft], 88
		jmp	.l3
.l2:		mov	ecx, dword [diwstrt]
		and	ecx, 0ffh
		mov	dword [diwxleft], ecx
.l3:		mov	dword [diwxright], edx
		pushad
		call graphCalculateWindow_C
		popad
		ret


	
;-------------------------------------------------------------------------------
; DDFSTRT - $dff092 Write
; When this value is written, the scroll (BPLCON1) also need to be reevaluated
; for _reversal_ of the scroll values.
; _wbplcon1_ calls _graphCalculateWindow_ so we don't need to here
;-------------------------------------------------------------------------------


		FALIGN32

global _wddfstrt_
_wddfstrt_:	and	edx, 0fch
		cmp	edx, 018h
		jae	.l1
		mov	edx, 018h
.l1:		mov	dword [ddfstrt], edx
		sub	edx, 014h
		mov	dword [sprite_ddf_kill], edx
		mov	edx, dword [bplcon1]
		jmp	_wbplcon1_


;-------------------------------------------------------------------------------
; DDFSTOP - $dff094 Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wddfstop_
_wddfstop_:	and	edx, 0fch
		cmp	edx, 0d8h
		jbe	.l1
		mov	edx, 0d8h
.l1:		mov	dword [ddfstop], edx
		pushad
		call graphCalculateWindow_C
		popad
		ret



;-------------------------------------------------------------------------------
; DMACON
;-------------------------------------------------------------------------------
; $dff096  - Read from $dff002
; dmacon - zero if master dma bit is off
; dmaconr - is always correct.


		FALIGN32

global _wdmacon_
_wdmacon_:	mov	ecx, dword [dmaconr]
		test	edx, 08000h
		jz	near wdma1
		and	edx, 7ffh		; Set bits - Readonly bits

; Test if BLTHOG got turned on
		test	edx, 0400h		; Is BLTHOG on now?
		jz	.bhogend
		test	ecx, 0400h		; Was BLTHOG off before?
		jnz	.bhogend
		; BLTHOG got turned on, stop CPU until a blit is finished if
		; this is a blit that uses all cycles.
		cmp	dword [blitend], -1
		je	.bhogend
		cmp	dword [blit_cycle_free], 0
		jne	.bhogend
		push	edx
		mov	edx, dword [blitend]
		sub	edx, dword [curcycle]
		add	dword [thiscycle], edx	; Delays CPU additionally edx cycles
		pop	edx
.bhogend:

		or	ecx, edx			; ecx is new dmaconr
		mov	dword [dmaconr], ecx
		test	ecx, 0200h
		jnz	wdmanorm
		xor	ecx, ecx
wdmanorm:	mov	edx, dword [dmacon]	; get old dmacon (master dma adjusted)
		mov	dword [dmacon], ecx	; Store new dmacon --------"---------

		


; Do audio stuff
		test	ecx, 1			; Channel 0 turned on?
		jz	snden0
		test	edx, 1			; Already on?
		jnz	snden0
		push	edx
		push	ecx
		mov	edx, 0
		push	eax
		call	_soundState0_
		pop	eax
		pop	ecx
		pop	edx
snden0:		test	ecx, 2
		jz	snden1
		test	edx, 2
		jnz	snden1
		push	edx
		push	ecx
%ifndef SOUND_C
		mov	edx, 4
%else
		mov	edx, 1
%endif
		push	eax
		call	_soundState0_
		pop	eax
		pop	ecx
		pop	edx
snden1:		test	ecx, 4
		jz	snden2
		test	edx, 4
		jnz	snden2
		push	edx
		push	ecx
%ifndef SOUND_C
		mov	edx, 8
%else
		mov	edx, 2
%endif
		push	eax
		call	_soundState0_
		pop	eax
		pop	ecx
		pop	edx
snden2:
		test	ecx, 8
		jz	snden3
		test	edx, 8
		jnz	snden3
		push	edx
		push	ecx
%ifndef SOUND_C
		mov	edx, 12
%else
		mov	edx, 3
%endif
		push	eax
		call	_soundState0_
		pop	eax
		pop	ecx
		pop	edx
snden3:		test	dword [blitterdmawaiting], 1
		jz	wdmae1
		test	ecx, 0040h
		jz	wdmae1
		
		pushad
		call	blitterCopy
		popad

wdmae1:		call	_copperUpdateDMA_
		ret

wdma1:		and	edx, 7ffh		; Get rid of readonly bits
		not	edx
		and	ecx, edx			  ; Slett bits
		mov	dword [dmaconr], ecx
		test	ecx, 0200h
		jnz	wdmanormoff
		xor	ecx, ecx
wdmanormoff:	mov	edx, dword [dmacon]  ; get old dmacon
		mov	dword [dmacon], ecx  ; store new dmacon

; IF blitter dma is turned off in the middle of a blit
; finish the blit

		test	ecx, 040h
		jnz	wdmabrag
		test	dword [blit_started], 0ffh
		jz	wdmabrag
		pushad
		call	blitFinishBlit
		popad
wdmabrag:
; Do audio stuff when dma turns off
; Simply set state 0, (state not set if state = 3??)

		test	ecx, 1			; Channel 0 turned off?
		jnz	sndoff0
		test	edx, 1			; Already off?
		jz	sndoff0
		push	ecx
		mov	ecx, 0
		call	_soundChannelKill_
		pop	ecx
sndoff0:	test	ecx, 2
		jnz	sndoff1
		test	edx, 2
		jz	sndoff1
		push	ecx
		mov	ecx, 1
		call	_soundChannelKill_
		pop	ecx
sndoff1:	test	ecx, 4
		jnz	sndoff2
		test	edx, 4
		jz	sndoff2
		push	ecx
		mov	ecx, 2
		call	_soundChannelKill_
		pop	ecx
sndoff2:	test	ecx, 8
		jnz	sndoff3
		test	edx, 8
		jz	sndoff3
		push	ecx
		mov	ecx, 3
		call	_soundChannelKill_
		pop	ecx
sndoff3:	test	dword [blitterdmawaiting], 1
		jz	wdmae2
		test	ecx, 0040h
		jz	wdmae2
		pushad
		call	blitterCopy
		popad
wdmae2:		call	_copperUpdateDMA_
		ret


;-------------------------------------------------------------------------------
; BPL1PT - $dff0e0 Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl1pth_
_wbpl1pth_:	and	edx, 01fh
		mov	word [bpl1pt + 2], dx
		ret


		FALIGN32

global _wbpl1ptl_
_wbpl1ptl_:	and	edx, 0fffeh
  		mov	word [bpl1pt], dx
		ret


;-------------------------------------------------------------------------------
; BPL2PT - $dff0e4 Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl2pth_
_wbpl2pth_:	and	edx, 01fh
		mov	word [bpl2pt + 2], dx
		ret


		FALIGN32

global _wbpl2ptl_
_wbpl2ptl_:	and	edx, 0fffeh
  		mov	word [bpl2pt], dx
		ret


;-------------------------------------------------------------------------------
; BPL3PT - $dff0e8 Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl3pth_
_wbpl3pth_:	and	edx, 01fh
		mov	word [bpl3pt + 2], dx
		ret


		FALIGN32

global _wbpl3ptl_
_wbpl3ptl_:	and	edx, 0fffeh
  		mov	word [bpl3pt], dx
		ret


;-------------------------------------------------------------------------------
; BPL4PT - $dff0ec Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl4pth_
_wbpl4pth_:	and	edx, 01fh
		mov	word [bpl4pt + 2], dx
		ret


		FALIGN32

global _wbpl4ptl_
_wbpl4ptl_:	and	edx, 0fffeh
		mov	word [bpl4pt], dx
		ret


;-------------------------------------------------------------------------------
; BPL5PT - $dff0f0 Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl5pth_
_wbpl5pth_:	and	edx, 01fh
		mov	word [bpl5pt + 2], dx
		ret


		FALIGN32

global _wbpl5ptl_
_wbpl5ptl_:	and	edx, 0fffeh
		mov	word [bpl5pt], dx
		ret


;-------------------------------------------------------------------------------
; BPL6PT - $dff0f4 Write
;-------------------------------------------------------------------------------


		FALIGN32
	
global _wbpl6pth_
_wbpl6pth_:	and	edx, 01fh
		mov	word [bpl6pt + 2], dx
		ret


		FALIGN32

global _wbpl6ptl_
_wbpl6ptl_:	and	edx, 0fffeh
		mov	word [bpl6pt], dx
		ret


;-------------------------------------------------------------------------------
; BPLCON0 - $dff100 Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wbplcon0_
_wbplcon0_:
		push	ecx
		mov	dword [bplcon0], edx
		shr	edx, 12
		and	edx, 0fh
		test	byte [bplcon0 + 1], 04h
		jz	.l1
		mov	ecx, dword [graph_decode_line_dual_tab + edx*4]
		jmp	.l2
.l1:		mov	ecx, dword [graph_decode_line_tab + edx*4]
.l2:		mov	dword [graph_decode_line_ptr], ecx
		test	byte [bplcon0 + 1], 08h
		jz	.l3
		mov	ecx, dword [draw_line_HAM_lores_routine]
		jmp	.lend
.l3:		test	byte [bplcon0 + 1], 04h
		jz	.l4
		cmp	edx, 1
		je	.l4
		cmp	edx, 9
		je	.l4
		mov	ecx, dword [draw_line_dual_lores_routine]
		cmp	edx, 7
		jb	.lend
		mov	ecx, dword [draw_line_dual_hires_routine]
		jmp	.lend
.l4:		mov	ecx, dword [draw_line_lores_routine]
		test	edx, 08h
		jz	.lend
		mov	ecx, dword [draw_line_hires_routine]
.lend:		mov	dword [draw_line_BPL_res_routine], ecx
		pushad
		call graphCalculateWindow_C
		popad
		pop	ecx
		ret


; When ddfstrt is (mod 8)+4, shift order is 8-f,0-7 (lores) (Example: New Zealand Story)
; When ddfstrt is (mod 8)+2, shift order is 4-7,0-3 (hires)


;-------------------------------------------------------------------------------
; BPLCON1 - $dff102 Write
;-------------------------------------------------------------------------------

; Extra variables
; oddscroll - dword with the odd lores scrollvalue
; evenscroll -dword with the even lores scrollvalue

; oddhiscroll - dword with the odd hires scrollvalue
; evenhiscroll - dword with the even hires scrollvalue


		FALIGN32

global _wbplcon1_
_wbplcon1_:	and	edx, 0ffh
		mov	dword [bplcon1], edx
		mov	ecx, edx
		and	edx, 0fh

		test	dword [ddfstrt], 4h	; Reverse shift order?
		jz	.bpc1normal3
		add	edx, 8
		and	edx, 0fh

.bpc1normal3:	mov	dword [oddscroll], edx

		and	edx, 7h
		test	dword [ddfstrt], 2h
		jz	.bpc1normal1
		add	edx, 4h
		and	edx, 7h
.bpc1normal1:	shl	edx, 1
		mov	dword [oddhiscroll], edx
		and	ecx, 0f0h
		shr	ecx, 4

		test	dword [ddfstrt], 4h	; Reverse shift order?
		jz	.bpc1normal4
		add	ecx, 8
		and	ecx, 15

.bpc1normal4:	mov	dword [evenscroll], ecx
		and	ecx, 7h
		test	dword [ddfstrt], 2h
		jz	.bpc1normal2
		add	ecx, 4h
		and	ecx, 7h
.bpc1normal2:	shl	ecx, 1
		mov	dword [evenhiscroll], ecx
		pushad
		call graphCalculateWindow_C
		popad
		ret


;-------------------------------------------------------------------------------
; BPLCON2 - $dff104 Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wbplcon2_
_wbplcon2_:	mov	dword [bplcon2], edx
		ret


;-------------------------------------------------------------------------------
; BPL1MOD - $dff108 Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wbpl1mod_
_wbpl1mod_:	and	edx, 0fffeh
		movsx	edx, dx
		mov	dword [bpl1mod], edx
		ret


;-------------------------------------------------------------------------------
; BPL2MOD - $dff10a Write
;-------------------------------------------------------------------------------


		FALIGN32

global _wbpl2mod_
_wbpl2mod_:	and	edx, 0fffeh
		movsx	edx, dx
		mov	dword [bpl2mod], edx
		ret


;-------------------------------------------------------------------------------
; COLOR - $dff180 to $dff1be
;-------------------------------------------------------------------------------


		FALIGN32

global _wcolor_
_wcolor_:	and	edx, 0fffh
		mov	word [graph_color - 0180h + ecx], dx
		push	edx
		mov	edx, dword [draw_color_table + edx*4] ; Translate color
		mov	dword [graph_color_shadow - 0300h + ecx*2], edx
		pop	edx
		and	edx, 0eeeh                             ; Halfbrite color
		shr	edx, 1
		mov	word [graph_color - 0180h + 64 + ecx], dx
		mov	edx, dword [draw_color_table + edx*4] ; Translate color
		mov	dword [graph_color_shadow - 0300h + 128 + ecx*2], edx
		ret


;-------------------------------------------------------------------------------
; Macro that clears the edges of a line, the portion that is not occupied by
; bitplane data
;-------------------------------------------------------------------------------

	
		FALIGN32


%macro CLEAR_EDGES 2 ;  ODDSCROLL, EVENSCROLL
		mov	ebx, dword [%1]
		mov	edx, dword [%2]
		cmp	edx, ebx
		ja	%%dontswitch
		mov	edx, ebx
%%dontswitch:	mov	ebx, dword [graph_DDF_start]
		mov	ebp, dword [graph_DDF_word_count]
		add	ebx, dword [4 + esp]
		shl	ebp, 4
		xor	ecx,ecx
%%theloop:	mov	dword [ebx], ecx
		mov	dword [ebx + ebp], ecx
		add	ebx, 4
		sub	edx, 4
		jns	%%theloop
%endmacro

%macro CLEAR_EDGES_DUAL 2 ; ODDSCROLL, EVENSCROLL
		mov	ebx, dword [%1]
		mov	edx, dword [%2]
		cmp	edx, ebx
		ja	%%dontswitch
		mov	edx, ebx
%%dontswitch:	mov	ebx, dword [graph_DDF_start]
		mov	ebp, dword [graph_DDF_word_count]
		add	ebx, dword [4 + esp]
		shl	ebp, 4
		xor	ecx, ecx
%%theloop:	mov	dword [ebx], ecx
		mov	dword [ebx - graph_line_line1 + graph_line_line2], ecx
		mov	dword [ebx + ebp], ecx
		mov	dword [ebx + ebp - graph_line_line1 + graph_line_line2], ecx
		add	ebx, 4
		sub	edx, 4
		jns	%%theloop
%endmacro



;===============================================================================
; Planar to chunky conversion
; The assembly routines take parameters line1 and line2, which are assumed to
; be placed on the stack, at [4 + esp] and [8 + esp]
;===============================================================================



;-------------------------------------------------------------------------------
; Planar to chunky conversion, 0 bitplanes
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode0_
_graphDecode0_:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 1 bitplane lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode1_
_graphDecode1_:		TEST_HIRES		_graphDecode1Hi_
			TEST_MMX		_graphDecode1MMX_
			LOAD_WORDS_TO_DECODE	.d1nowork
			CLEAR_EDGES		oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d1oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.d1oddloop
			MODULO1
.d1nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 1 bitplane lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode1MMX_
_graphDecode1MMX_:	LOAD_WORDS_TO_DECODE	.d1mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.d1mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d1mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			MODULO1
.d1mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2_
_graphDecode2_:		TEST_HIRES		_graphDecode2Hi_
			TEST_MMX		_graphDecode2MMX_
			LOAD_WORDS_TO_DECODE	.d2nowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d2equal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d2oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.d2oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [4 + esp]
.d2evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_DECODED_DATA
			ITERATE_LOOP		.d2evenloop
			MODULO2
.d2nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d2equal:		SETUP_LOOP		evenscroll, [4 + esp]
.d2equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.d2equalloop
			MODULO2
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2MMX_
_graphDecode2MMX_:	LOAD_WORDS_TO_DECODE	.d2mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d2mmxequal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.d2mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d2mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.d2mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.d2mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO2
.d2mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d2mmxequal:		SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
.d2mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d2mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			MODULO2
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3_
_graphDecode3_:		TEST_HIRES		_graphDecode3Hi_
			TEST_MMX		_graphDecode3MMX_
			LOAD_WORDS_TO_DECODE	.d3nowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d3equal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d3oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.d3oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [4 + esp]
.d3evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_DECODED_DATA
			ITERATE_LOOP		.d3evenloop
			MODULO3
.d3nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d3equal:		SETUP_LOOP		evenscroll, [4 + esp]
.d3equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.d3equalloop
			MODULO3
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3MMX_
_graphDecode3MMX_:	LOAD_WORDS_TO_DECODE	.d3mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d3mmxequal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.d3mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d3mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.d3mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.d3mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO3
.d3mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d3mmxequal:		SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
.d3mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d3mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			MODULO3
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4_
_graphDecode4_:		TEST_HIRES		_graphDecode4Hi_
			TEST_MMX		_graphDecode4MMX_
			LOAD_WORDS_TO_DECODE	.d4nowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d4equal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d4oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.d4oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [4 + esp]
.d4evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_DECODED_DATA
			ITERATE_LOOP		.d4evenloop
			MODULO4
.d4nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d4equal:		SETUP_LOOP		evenscroll, [4 + esp]
.d4equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			STORE_DECODED_DATA
			ITERATE_LOOP		.d4equalloop
			MODULO4
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4MMX_
_graphDecode4MMX_:	LOAD_WORDS_TO_DECODE	.d4mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d4mmxequal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.d4mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d4mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.d4mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_BITPLANE_WORD_MMX	edx, graph_deco4
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.d4mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO4
.d4mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d4mmxequal:		SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl4pt
.d4mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			OR_BITPLANE_WORD_MMX	ebp, graph_deco4
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d4mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl4pt
			MODULO4
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 5 bitplanes lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode5_
_graphDecode5_:		TEST_MMX		_graphDecode5MMX_
			LOAD_WORDS_TO_DECODE	.d5nowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d5equal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d5oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl5pt, graph_deco5
			STORE_DECODED_DATA
			ITERATE_LOOP		.d5oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [4 + esp]
.d5evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_DECODED_DATA
			ITERATE_LOOP		.d5evenloop
			MODULO5
.d5nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d5equal:		SETUP_LOOP		evenscroll, [4 + esp]
.d5equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_BITPLANE_WORD	bpl5pt, graph_deco5
			STORE_DECODED_DATA
			ITERATE_LOOP		.d5equalloop
			MODULO5
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 5 bitplanes lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode5MMX_
_graphDecode5MMX_:	LOAD_WORDS_TO_DECODE	.d5mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d5mmxequal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
.d5mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			OR_BITPLANE_WORD_MMX	ebp, graph_deco5
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d5mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl5pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.d5mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_BITPLANE_WORD_MMX	edx, graph_deco4
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.d5mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO5
.d5mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d5mmxequal:		SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
.d5mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			LOAD_BITPLANE_PTR	ebp, bpl4pt
			OR_BITPLANE_WORD_MMX	ebp, graph_deco4
			SAVE_BITPLANE_PTR	ebp, bpl4pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
			OR_BITPLANE_WORD_MMX	ebp, graph_deco5
			SAVE_BITPLANE_PTR	ebp, bpl5pt
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d5mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			MODULO5
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 6 bitplanes lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode6_
_graphDecode6_:		TEST_MMX		_graphDecode6MMX_
			LOAD_WORDS_TO_DECODE	.d6nowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d6equal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.d6oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl5pt, graph_deco5
			STORE_DECODED_DATA
			ITERATE_LOOP		.d6oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [4 + esp]
.d6evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_BITPLANE_WORD	bpl6pt, graph_deco6
			OR_DECODED_DATA
			ITERATE_LOOP		.d6evenloop
			MODULO6
.d6nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d6equal:		SETUP_LOOP		evenscroll, [4 + esp]
.d6equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_BITPLANE_WORD	bpl5pt, graph_deco5
			OR_BITPLANE_WORD	bpl6pt, graph_deco6
			STORE_DECODED_DATA
			ITERATE_LOOP		.d6equalloop
			MODULO6
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 6 bitplanes lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode6MMX_
_graphDecode6MMX_:	LOAD_WORDS_TO_DECODE	.d6mmxnowork
			CLEAR_EDGES		oddscroll, evenscroll
			DECIDE_ODDEVEN_EQUAL	.d6mmxequal, oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
.d6mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			OR_BITPLANE_WORD_MMX	ebp, graph_deco5
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d6mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl5pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
			LOAD_BITPLANE_PTR	edi, bpl6pt
.d6mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_BITPLANE_WORD_MMX	edx, graph_deco4
			OR_BITPLANE_WORD_MMX	edi, graph_deco6
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.d6mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			SAVE_BITPLANE_PTR	edi, bpl6pt
			MODULO6
.d6mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d6mmxequal:		SETUP_LOOP_MMX		evenscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
.d6mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			LOAD_BITPLANE_PTR	ebp, bpl4pt
			OR_BITPLANE_WORD_MMX	ebp, graph_deco4
			SAVE_BITPLANE_PTR	ebp, bpl4pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
			OR_BITPLANE_WORD_MMX	ebp, graph_deco5
			SAVE_BITPLANE_PTR	ebp, bpl5pt
			LOAD_BITPLANE_PTR	ebp, bpl6pt
			OR_BITPLANE_WORD_MMX	ebp, graph_deco6
			SAVE_BITPLANE_PTR	ebp, bpl6pt
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.d6mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			MODULO6
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 1 bitplane hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode1Hi_
_graphDecode1Hi_:	TEST_MMX		_graphDecode1HiMMX_
			LOAD_WORDS_TO_DECODE	.dh1nowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.dh1oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh1oddloop
			MODULO1
.dh1nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 1 bitplane hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode1HiMMX_
_graphDecode1HiMMX_:	LOAD_WORDS_TO_DECODE	.dh1mmxnowork

			CLEAR_EDGES		oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.dh1mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh1mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			MODULO1
.dh1mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2Hi_
_graphDecode2Hi_:	TEST_MMX		_graphDecode2HiMMX_
			LOAD_WORDS_TO_DECODE	.dh2nowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll
			DECIDE_ODDEVEN_EQUAL	.dh2equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.dh2oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh2oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [4 + esp]
.dh2evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_DECODED_DATA
			ITERATE_LOOP		.dh2evenloop
			MODULO2
.dh2nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh2equal:		SETUP_LOOP		evenhiscroll, [4 + esp]
.dh2equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh2equalloop
			MODULO2
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2HiMMX_
_graphDecode2HiMMX_:	LOAD_WORDS_TO_DECODE	.dh2mmxnowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll

			DECIDE_ODDEVEN_EQUAL	.dh2mmxequal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.dh2mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh2mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.dh2mmxevenloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.dh2mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO2
.dh2mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh2mmxequal:		SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
.dh2mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh2mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			MODULO2
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3Hi_
_graphDecode3Hi_:	TEST_MMX		_graphDecode3HiMMX_
			LOAD_WORDS_TO_DECODE	.dh3nowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll
			DECIDE_ODDEVEN_EQUAL	.dh3equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.dh3oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh3oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [4 + esp]
.dh3evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_DECODED_DATA
			ITERATE_LOOP		.dh3evenloop
			MODULO3
.dh3nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh3equal:		SETUP_LOOP		evenhiscroll, [4 + esp]
.dh3equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh3equalloop
			MODULO3
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3HiMMX_
_graphDecode3HiMMX_:	LOAD_WORDS_TO_DECODE	.dh3mmxnowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll
			DECIDE_ODDEVEN_EQUAL	.dh3mmxequal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.dh3mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh3mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.dh3mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.dh3mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO3
.dh3mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh3mmxequal:		SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
.dh3mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh3mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			MODULO3
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4Hi_
_graphDecode4Hi_:	TEST_MMX		_graphDecode4HiMMX_
			LOAD_WORDS_TO_DECODE	.dh4nowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll
			DECIDE_ODDEVEN_EQUAL	.dh4equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.dh4oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh4oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [4 + esp]
.dh4evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			OR_DECODED_DATA
			ITERATE_LOOP		.dh4evenloop
			MODULO4
.dh4nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh4equal:		SETUP_LOOP		evenhiscroll, [4 + esp]
.dh4equalloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl2pt, graph_deco2
			OR_BITPLANE_WORD	bpl3pt, graph_deco3
			OR_BITPLANE_WORD	bpl4pt, graph_deco4
			STORE_DECODED_DATA
			ITERATE_LOOP		.dh4equalloop
			MODULO4
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4HiMMX_
_graphDecode4HiMMX_:	LOAD_WORDS_TO_DECODE	.dh4mmxnowork
			CLEAR_EDGES		oddhiscroll, evenhiscroll
			DECIDE_ODDEVEN_EQUAL	.dh4mmxequal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.dh4mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh4mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.dh4mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco2
			OR_BITPLANE_WORD_MMX	edx, graph_deco4
			OR_DECODED_DATA_MMX
			ITERATE_LOOP		.dh4mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO4
.dh4mmxnowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.dh4mmxequal:		SETUP_LOOP_MMX		evenhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl2pt
			LOAD_BITPLANE_PTR	edi, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl4pt
.dh4mmxequalloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			OR_BITPLANE_WORD_MMX	ebp, graph_deco4
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dh4mmxequalloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl2pt
			SAVE_BITPLANE_PTR	edi, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl4pt
			MODULO4
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes dual lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2Dual_
_graphDecode2Dual_:	TEST_HIRES		_graphDecode2HiDual_
			TEST_MMX		_graphDecode2DualMMX_
			LOAD_WORDS_TO_DECODE	.dd2nowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.dd2oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd2oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [8 + esp]
.dd2evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd2evenloop
			MODULO2
.dd2nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes dual lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2DualMMX_
_graphDecode2DualMMX_:
			LOAD_WORDS_TO_DECODE	.dd2mmxnowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.dd2mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd2mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.dd2mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd2mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO2
.dd2mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes dual lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3Dual_
_graphDecode3Dual_:	TEST_HIRES		_graphDecode3HiDual_
			TEST_MMX		_graphDecode3DualMMX_
			LOAD_WORDS_TO_DECODE	.dd3nowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.dd3oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd3oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [8 + esp]
.dd3evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd3evenloop
			MODULO3
.dd3nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes dual lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3DualMMX_
_graphDecode3DualMMX_:
			LOAD_WORDS_TO_DECODE	.dd3mmxnowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.dd3mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd3mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.dd3mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd3mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO3
.dd3mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes dual lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4Dual_
_graphDecode4Dual_:	TEST_HIRES		_graphDecode4HiDual_
			TEST_MMX		_graphDecode4DualMMX_
			LOAD_WORDS_TO_DECODE	.dd4nowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.dd4oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd4oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [8 + esp]
.dd4evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			OR_BITPLANE_WORD	bpl4pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd4evenloop
			MODULO4
.dd4nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes dual lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4DualMMX_
_graphDecode4DualMMX_:
			LOAD_WORDS_TO_DECODE	.dd4mmxnowork
			CLEAR_EDGES_DUAL        oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.dd4mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd4mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.dd4mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd4mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO4
.dd4mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 5 bitplanes dual lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode5Dual_
_graphDecode5Dual_:	TEST_MMX		_graphDecode5DualMMX_
			LOAD_WORDS_TO_DECODE	.dd5nowork
			CLEAR_EDGES_DUAL        oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.dd5oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			OR_BITPLANE_WORD	bpl5pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd5oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [8 + esp]
.dd5evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			OR_BITPLANE_WORD	bpl4pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd5evenloop
			MODULO5
.dd5nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 5 bitplanes dual lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode5DualMMX_
_graphDecode5DualMMX_:
			LOAD_WORDS_TO_DECODE	.dd5mmxnowork
			CLEAR_EDGES_DUAL        oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
.dd5mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	ebp, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd5mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl5pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.dd5mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd5mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO5
.dd5mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 6 bitplanes dual lores
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode6Dual_
_graphDecode6Dual_:	TEST_MMX		_graphDecode6DualMMX_
			LOAD_WORDS_TO_DECODE	.dd6nowork
			CLEAR_EDGES_DUAL	oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP		oddscroll, [4 + esp]
.dd6oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			OR_BITPLANE_WORD	bpl5pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd6oddloop

			; Decode even planes

			SETUP_LOOP		evenscroll, [8 + esp]
.dd6evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			OR_BITPLANE_WORD	bpl4pt, graph_deco2
			OR_BITPLANE_WORD	bpl6pt, graph_deco3
			STORE_DECODED_DATA
			ITERATE_LOOP		.dd6evenloop
			MODULO6
.dd6nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 6 bitplanes dual lores MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode6DualMMX_
_graphDecode6DualMMX_:
			LOAD_WORDS_TO_DECODE	.dd6mmxnowork
			CLEAR_EDGES_DUAL        oddscroll, evenscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
			LOAD_BITPLANE_PTR	ebp, bpl5pt
.dd6mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	ebp, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd6mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt
			SAVE_BITPLANE_PTR	ebp, bpl5pt

			; Decode even planes

			SETUP_LOOP_MMX		evenscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
			LOAD_BITPLANE_PTR	edi, bpl6pt
.dd6mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			OR_BITPLANE_WORD_MMX	edi, graph_deco3
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.dd6mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			SAVE_BITPLANE_PTR	edi, bpl6pt
			MODULO6
.dd6mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes dual hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2HiDual_
_graphDecode2HiDual_:
			TEST_MMX		_graphDecode2HiDualMMX_
			LOAD_WORDS_TO_DECODE	.ddh2nowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.ddh2oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh2oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [8 + esp]
.ddh2evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh2evenloop
			MODULO2
.ddh2nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes dual hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2HiDualMMX_
_graphDecode2HiDualMMX_:
			LOAD_WORDS_TO_DECODE	.ddh2mmxnowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
.ddh2mmxoddloop:		LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh2mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.ddh2mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh2mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO2
.ddh2mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes dual hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3HiDual_
_graphDecode3HiDual_:
			TEST_MMX		_graphDecode3HiDualMMX_
			LOAD_WORDS_TO_DECODE	.ddh3nowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.ddh3oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh3oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [8 + esp]
.ddh3evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh3evenloop
			MODULO3
.ddh3nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes dual hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3HiDualMMX_
_graphDecode3HiDualMMX_:
			LOAD_WORDS_TO_DECODE	.ddh3mmxnowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.ddh3mmxoddloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh3mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
.ddh3mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh3mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			MODULO3
.ddh3mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes dual hires
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4HiDual_
_graphDecode4HiDual_:
			TEST_MMX		_graphDecode4HiDualMMX_
			LOAD_WORDS_TO_DECODE	.ddh4nowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP		oddhiscroll, [4 + esp]
.ddh4oddloop:		LOAD_BITPLANE_WORD	bpl1pt, graph_deco1
			OR_BITPLANE_WORD	bpl3pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh4oddloop

			; Decode even planes

			SETUP_LOOP		evenhiscroll, [8 + esp]
.ddh4evenloop:		LOAD_BITPLANE_WORD	bpl2pt, graph_deco1
			OR_BITPLANE_WORD	bpl4pt, graph_deco2
			STORE_DECODED_DATA
			ITERATE_LOOP		.ddh4evenloop
			MODULO4
.ddh4nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes dual hires MMX
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4HiDualMMX_
_graphDecode4HiDualMMX_:
			LOAD_WORDS_TO_DECODE	.ddh4mmxnowork
			CLEAR_EDGES_DUAL	oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_MMX		oddhiscroll, [4 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl1pt
			LOAD_BITPLANE_PTR	edx, bpl3pt
.ddh4mmxoddloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh4mmxoddloop
			SAVE_BITPLANE_PTR	ecx, bpl1pt
			SAVE_BITPLANE_PTR	edx, bpl3pt

			; Decode even planes

			SETUP_LOOP_MMX		evenhiscroll, [8 + esp]
			LOAD_BITPLANE_PTR	ecx, bpl2pt
			LOAD_BITPLANE_PTR	edx, bpl4pt
.ddh4mmxevenloop:	LOAD_BITPLANE_WORD_MMX	ecx, graph_deco1
			OR_BITPLANE_WORD_MMX	edx, graph_deco2
			STORE_DECODED_DATA_MMX
			ITERATE_LOOP		.ddh4mmxevenloop
			SAVE_BITPLANE_PTR	ecx, bpl2pt
			SAVE_BITPLANE_PTR	edx, bpl4pt
			MODULO4
.ddh4mmxnowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 1 bitplane hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode1Hi320_
_graphDecode1Hi320_:	LOAD_WORDS_TO_DECODE	.d320h1nowork

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.d320h1oddloop:		LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h1oddloop
			MODULO1
.d320h1nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2Hi320_
_graphDecode2Hi320_:	LOAD_WORDS_TO_DECODE	.d320h2nowork
			DECIDE_ODDEVEN_EQUAL	.d320h2equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.d320h2oddloop:		LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h2oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h2evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			OR_DECODED_DATA_320
			ITERATE_LOOP		.d320h2evenloop
			MODULO2
.d320h2nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d320h2equal:		SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h2equalloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h2equalloop
			MODULO2
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3Hi320_
_graphDecode3Hi320_:	LOAD_WORDS_TO_DECODE	.d320h3nowork
			DECIDE_ODDEVEN_EQUAL	.d320h3equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.d320h3oddloop:		LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi3
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h3oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h3evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			OR_DECODED_DATA_320
			ITERATE_LOOP		.d320h3evenloop
			MODULO3
.d320h3nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d320h3equal:		SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h3equalloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi3
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h3equalloop
			MODULO3
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4Hi320_
_graphDecode4Hi320_:	LOAD_WORDS_TO_DECODE	.d320h4nowork
			DECIDE_ODDEVEN_EQUAL	.d320h4equal, oddhiscroll, evenhiscroll

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.d320h4oddloop:		LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi3
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h4oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h4evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			OR_BITPLANE_WORD_320	bpl4pt, graph_deco320hi4
			OR_DECODED_DATA_320
			ITERATE_LOOP		.d320h4evenloop
			MODULO4
.d320h4nowork:		ret


			FALIGN32

			; Decode odd and even planes, scroll equal

.d320h4equal:		SETUP_LOOP_320		evenhiscroll, [4 + esp]
.d320h4equalloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl2pt, graph_deco320hi2
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi3
			OR_BITPLANE_WORD_320	bpl4pt, graph_deco320hi4
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.d320h4equalloop
			MODULO4
			ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 2 bitplanes dual hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode2HiDual320_
_graphDecode2HiDual320_:
			LOAD_WORDS_TO_DECODE	.dd320h2nowork

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.dd320h2oddloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h2oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [8 + esp]
.dd320h2evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi1
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h2evenloop
			MODULO2
.dd320h2nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 3 bitplanes dual hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode3HiDual320_
_graphDecode3HiDual320_:
			LOAD_WORDS_TO_DECODE	.dd320h3nowork

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.dd320h3oddloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi2
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h3oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [8 + esp]
.dd320h3evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi1
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h3evenloop
			MODULO3
.dd320h3nowork:		ret


;-------------------------------------------------------------------------------
; Planar to chunky conversion, 4 bitplanes dual hires for lores VGA
;-------------------------------------------------------------------------------


			FALIGN32

global _graphDecode4HiDual320_
_graphDecode4HiDual320_:
			LOAD_WORDS_TO_DECODE	.dd320h4nowork

			; Decode odd planes

			SETUP_LOOP_320		oddhiscroll, [4 + esp]
.dd320h4oddloop:	LOAD_BITPLANE_WORD_320	bpl1pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl3pt, graph_deco320hi2
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h4oddloop

			; Decode even planes

			SETUP_LOOP_320		evenhiscroll, [8 + esp]
.dd320h4evenloop:	LOAD_BITPLANE_WORD_320	bpl2pt, graph_deco320hi1
			OR_BITPLANE_WORD_320	bpl4pt, graph_deco320hi2
			STORE_DECODED_DATA_320
			ITERATE_LOOP		.dd320h4evenloop
			MODULO4
.dd320h4nowork:		ret





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
		call	dword [memory_iobank_write + 2*ecx]
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

		mov	ebp, dword [esp + 4]
		mov	esi, dword [esp + 4]
		add	ebp, graph_line_line2
		add	esi, graph_line_line1
		push	ebp
		push	esi
		call	dword [graph_decode_line_ptr]
		add	esp, 8

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

		mov	ebp, dword graph_line2_tmp
		mov	esi, dword graph_line1_tmp
		push	ebp
		push	esi
		call	dword [graph_decode_line_ptr]
		add	esp, 8

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
.no_bpl_skip:	call	_graphComposeLineOutput_
.after_line:	add	esp, 4

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
