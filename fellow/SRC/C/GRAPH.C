/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Convert Amiga graphics data to temporary format suitable for fast          */
/* drawing on a chunky display.                                               */
/*                                                                            */
/* Authors: Petter Schau                                                      */
/*          Worfje                                                            */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"
#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "cpu.h"
#include "fmem.h"
#include "graphm.h"            /* Macros used */
#include "kbd.h"
#include "copper.h"
#include "blit.h"
#include "graph.h"


/*===========================================================================*/
/* Lookup-tables for planar to chunky routines                               */
/*===========================================================================*/

planar2chunkyroutine graph_decode_line_ptr;
planar2chunkyroutine graph_decode_line_tab[16];
planar2chunkyroutine graph_decode_line_dual_tab[16];

/*===========================================================================*/
/* Planar to chunky lookuptables for data                                    */
/*===========================================================================*/

ULO graph_deco1[256][2];
ULO graph_deco2[256][2];
ULO graph_deco3[256][2];
ULO graph_deco4[256][2];
ULO graph_deco5[256][2];
ULO graph_deco6[256][2];

ULO graph_deco320hi1[256];
ULO graph_deco320hi2[256];
ULO graph_deco320hi3[256];
ULO graph_deco320hi4[256];

ULO graph_decode_tmp;

ULO serper_debug_register_ecx;
ULO serper_debug_register_edx;

UBY graph_line1_tmp[1024];
UBY graph_line2_tmp[1024];


/*===========================================================================*/
/* Line description registers and data                                       */
/*===========================================================================*/

ULO graph_DDF_start;
ULO graph_DDF_word_count;
ULO graph_DIW_first_visible;
ULO graph_DIW_last_visible;
ULO graph_allow_bpl_line_skip;


/*===========================================================================*/
/* Translation tables for colors                                             */
/*===========================================================================*/

ULO graph_color_shadow[64];  /* Colors corresponding to the different Amiga- */
                             /* registers. Initialized from draw_color_table */
		             /* whenever WCOLORXX is written.                */

UWO graph_color[64];         /* Copy of Amiga version of colors */
BOOLE graph_playfield_on;


/*===========================================================================*/
/* IO-registers                                                              */
/*===========================================================================*/

ULO bpl1pt, bpl2pt, bpl3pt, bpl4pt, bpl5pt, bpl6pt;
ULO lof, ddfstrt, ddfstop, bplcon0, bplcon1, bplcon2, bpl1mod, bpl2mod;
ULO evenscroll, evenhiscroll, oddscroll, oddhiscroll;
ULO diwstrt, diwstop; 
ULO diwxleft, diwxright, diwytop, diwybottom;
ULO graph_raster_x, graph_raster_y;
ULO dmaconr, dmacon;


/*===========================================================================*/
/* Framebuffer data about each line, max triple buffering                    */
/*===========================================================================*/

graph_line graph_frame[3][314];


/*===========================================================================*/
/* Initializes the Planar 2 Chunky translation tables                        */
/*===========================================================================*/

void graphP2CTablesInit(void) {
  ULO i, j, d[2];
 
  for (i = 0; i < 256; i++) {
    d[0] = d[1] = 0;
    for (j = 0; j < 4; j++) {
      d[0] |= ((i & (0x80>>j))>>(4 + 3 - j))<<(j*8);
      d[1] |= ((i & (0x8>>j))>>(3 - j))<<(j*8);
    }
    for (j = 0; j < 2; j++) {
      graph_deco1[i][j] = d[j]<<2;
      graph_deco2[i][j] = d[j]<<3;
      graph_deco3[i][j] = d[j]<<4;
      graph_deco4[i][j] = d[j]<<5;
      graph_deco5[i][j] = d[j]<<6;
      graph_deco6[i][j] = d[j]<<7;
    }
    graph_deco320hi1[i] = ((d[0] & 0xff) |
                           ((d[0] & 0xff0000)>>8) |
		           ((d[1] & 0xff)<<16) |
			   ((d[1] & 0xff0000)<<8))<<2;
    graph_deco320hi2[i] = graph_deco320hi1[i]<<1;
    graph_deco320hi3[i] = graph_deco320hi1[i]<<2;
    graph_deco320hi4[i] = graph_deco320hi1[i]<<3;
  }
  graph_decode_tmp = 0;
}


/*===========================================================================*/
/* Decode tables for lores size displays                                     */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C1XInit(void) {
  graph_decode_line_tab[0] = graphDecode0_C;
  graph_decode_line_tab[1] = graphDecode1_C;
  graph_decode_line_tab[2] = graphDecode2_C;
  graph_decode_line_tab[3] = graphDecode3_C;
  graph_decode_line_tab[4] = graphDecode4_C;
  graph_decode_line_tab[5] = graphDecode5_C;
  graph_decode_line_tab[6] = graphDecode6_C;
  graph_decode_line_tab[7] = graphDecode0_C;
  graph_decode_line_tab[8] = graphDecode0_C;
  graph_decode_line_tab[9] = graphDecode1Hi320;
  graph_decode_line_tab[10] = graphDecode2Hi320;
  graph_decode_line_tab[11] = graphDecode3Hi320;
  graph_decode_line_tab[12] = graphDecode4Hi320;
  graph_decode_line_tab[13] = graphDecode0_C;
  graph_decode_line_tab[14] = graphDecode0_C;
  graph_decode_line_tab[15] = graphDecode0_C;
  graph_decode_line_dual_tab[0] = graphDecode0_C;
  graph_decode_line_dual_tab[1] = graphDecode1_C;
  graph_decode_line_dual_tab[2] = graphDecode2Dual;
  graph_decode_line_dual_tab[3] = graphDecode3Dual;
  graph_decode_line_dual_tab[4] = graphDecode4Dual;
  graph_decode_line_dual_tab[5] = graphDecode5Dual;
  graph_decode_line_dual_tab[6] = graphDecode6Dual;
  graph_decode_line_dual_tab[7] = graphDecode0_C;
  graph_decode_line_dual_tab[8] = graphDecode0_C;
  graph_decode_line_dual_tab[9] = graphDecode1Hi320;
  graph_decode_line_dual_tab[10] = graphDecode2HiDual320;
  graph_decode_line_dual_tab[11] = graphDecode3HiDual320;
  graph_decode_line_dual_tab[12] = graphDecode4HiDual320;
  graph_decode_line_dual_tab[13] = graphDecode0_C;
  graph_decode_line_dual_tab[14] = graphDecode0_C;
  graph_decode_line_dual_tab[15] = graphDecode0_C;
  graph_decode_line_ptr = graphDecode0_C;
}


/*===========================================================================*/
/* Decode tables for hires size tables                                       */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C2XInit(void) {
  graph_decode_line_tab[0] = graphDecode0_C;
  graph_decode_line_tab[1] = graphDecode1_C;
  graph_decode_line_tab[2] = graphDecode2_C;
  graph_decode_line_tab[3] = graphDecode3_C;
  graph_decode_line_tab[4] = graphDecode4_C;
  graph_decode_line_tab[5] = graphDecode5_C;
  graph_decode_line_tab[6] = graphDecode6_C;
  graph_decode_line_tab[7] = graphDecode0_C;
  graph_decode_line_tab[8] = graphDecode0_C;
  graph_decode_line_tab[9] = graphDecode1_C;
  graph_decode_line_tab[10] = graphDecode2_C;
  graph_decode_line_tab[11] = graphDecode3_C;
  graph_decode_line_tab[12] = graphDecode4_C;
  graph_decode_line_tab[13] = graphDecode0_C;
  graph_decode_line_tab[14] = graphDecode0_C;
  graph_decode_line_tab[15] = graphDecode0_C;
  graph_decode_line_dual_tab[0] = graphDecode0_C;
  graph_decode_line_dual_tab[1] = graphDecode1_C;
  graph_decode_line_dual_tab[2] = graphDecode2Dual;
  graph_decode_line_dual_tab[3] = graphDecode3Dual;
  graph_decode_line_dual_tab[4] = graphDecode4Dual;
  graph_decode_line_dual_tab[5] = graphDecode5Dual;
  graph_decode_line_dual_tab[6] = graphDecode6Dual;
  graph_decode_line_dual_tab[7] = graphDecode0_C;
  graph_decode_line_dual_tab[8] = graphDecode0_C;
  graph_decode_line_dual_tab[9] = graphDecode1_C;
  graph_decode_line_dual_tab[10] = graphDecode2Dual;
  graph_decode_line_dual_tab[11] = graphDecode3Dual;
  graph_decode_line_dual_tab[12] = graphDecode4Dual;
  graph_decode_line_dual_tab[13] = graphDecode0_C;
  graph_decode_line_dual_tab[14] = graphDecode0_C;
  graph_decode_line_dual_tab[15] = graphDecode0_C;
  graph_decode_line_ptr = graphDecode0_C;
}


/*===========================================================================*/
/* Clear the line descriptions                                               */
/* This function is called on emulation start and from the debugger when     */
/* the emulator is stopped mid-line. It must create a valid dummy array.     */
/*===========================================================================*/

void graphLineDescClear(void) {
  int frame, line;

  memset(graph_frame, 0, sizeof(graph_line)*3*314);
  for (frame = 0; frame < 3; frame++)
    for (line = 0; line < 314; line++) {
      graph_frame[frame][line].linetype = GRAPH_LINE_BG;
      graph_frame[frame][line].draw_line_routine =
	                                        (void *) draw_line_BG_routine;
      graph_frame[frame][line].colors[0] = 0;
      graph_frame[frame][line].frames_left_until_BG_skip = 1; /* Ensures we draw once */
      graph_frame[frame][line].sprite_ham_slot = 0xffffffff;
    }
}


/*===========================================================================*/
/* Clear the line descriptions partially                                     */
/* Removes all the SKIP tags.                                                */
/*===========================================================================*/

void graphLineDescClearSkips(void) {
  int frame, line;

  for (frame = 0; frame < 3; frame++)
    for (line = 0; line < 314; line++) 
      if (graph_frame[frame][line].linetype == GRAPH_LINE_SKIP) {
        graph_frame[frame][line].linetype = GRAPH_LINE_BG;
        graph_frame[frame][line].frames_left_until_BG_skip = 1;
      }
}


/*===========================================================================*/
/* The mapping from Amiga colors to host colors are dependent on the host    */
/* resolution. We need to recalculate the colors to reflect the possibly new */
/* color format.                                                             */
/* This function is called from the drawEmulationStart() functions since we  */
/* don't know the color format yet in graphEmulationStart()                  */
/*===========================================================================*/

void graphInitializeShadowColors(void) {
  ULO i;
  
  for (i = 0; i < 64; i++)
    graph_color_shadow[i] = draw_color_table[graph_color[i] & 0xfff];
}


/*===========================================================================*/
/* Clear all register data                                                   */
/*===========================================================================*/

static void graphIORegistersClear(void) {
  ULO i;
  
  for (i = 0; i < 64; i++) graph_color_shadow[i] = graph_color[i] = 0;
  graph_playfield_on = FALSE;
  lof = 0x8000;       /* Long frame */
  bpl1mod = 0;
  bpl2mod = 0;
  bpl1pt = 0;
  bpl2pt = 0;
  bpl3pt = 0;
  bpl4pt = 0;
  bpl5pt = 0;
  bpl6pt = 0;
  bplcon0 = 0;
  bplcon1 = 0;
  bplcon2 = 0;
  ddfstrt = 0;
  ddfstop = 0;
  graph_DDF_start = 0;
  graph_DDF_word_count = 0;
  diwstrt = 0;
  diwstop = 0;
  diwxleft = 0;
  diwxright = 0; 
  diwytop = 0;   
  diwybottom = 0;
  graph_DIW_first_visible = 256;
  graph_DIW_last_visible = 256;
  graph_raster_x = 0;
  graph_raster_y = 0;
  evenscroll = 0;
  evenhiscroll = 0;
  oddscroll = 0;
  oddhiscroll = 0;
  dmaconr = 0;
  dmacon = 0;
}


/*===========================================================================*/
/* Property set/get for smart drawing                                       */
/*===========================================================================*/

void graphSetAllowBplLineSkip(BOOLE allow_bpl_line_skip) {
  graph_allow_bpl_line_skip = allow_bpl_line_skip;
}

BOOLE graphGetAllowBplLineSkip(void) {
  return graph_allow_bpl_line_skip;
}

/*===========================================================================*/
/* Registers the graphics IO register handlers                               */
/*===========================================================================*/

void graphIOHandlersInstall(void) {
  ULO i;

  memorySetIOReadStub(0x002, rdmaconr);
  memorySetIOReadStub(0x004, rvposr);
  memorySetIOReadStub(0x006, rvhposr);
  memorySetIOReadStub(0x07c, rid);
  memorySetIOWriteStub(0x02a, wvpos);
  memorySetIOWriteStub(0x08e, wdiwstrt);
  memorySetIOWriteStub(0x090, wdiwstop);
  memorySetIOWriteStub(0x092, wddfstrt);
  memorySetIOWriteStub(0x094, wddfstop);
  memorySetIOWriteStub(0x096, wdmacon);
  memorySetIOWriteStub(0x0e0, wbpl1pth);
  memorySetIOWriteStub(0x0e2, wbpl1ptl);
  memorySetIOWriteStub(0x0e4, wbpl2pth);
  memorySetIOWriteStub(0x0e6, wbpl2ptl);
  memorySetIOWriteStub(0x0e8, wbpl3pth);
  memorySetIOWriteStub(0x0ea, wbpl3ptl);
  memorySetIOWriteStub(0x0ec, wbpl4pth);
  memorySetIOWriteStub(0x0ee, wbpl4ptl);
  memorySetIOWriteStub(0x0f0, wbpl5pth);
  memorySetIOWriteStub(0x0f2, wbpl5ptl);
  memorySetIOWriteStub(0x0f4, wbpl6pth);
  memorySetIOWriteStub(0x0f6, wbpl6ptl);
  memorySetIOWriteStub(0x100, wbplcon0);
  memorySetIOWriteStub(0x102, wbplcon1);
  memorySetIOWriteStub(0x104, wbplcon2);
  memorySetIOWriteStub(0x108, wbpl1mod);
  memorySetIOWriteStub(0x10a, wbpl2mod);
  for (i = 0x180; i < 0x1c0; i += 2) memorySetIOWriteStub(i, wcolor);
}


/*===========================================================================*/
/* Graphics Register Access routines                                         */
/*===========================================================================*/

/*===========================================================================*/
/* DMACONR - $dff002 Read                                                    */
/*                                                                           */
/* return dmaconr | ((!((WOR)bltzero))<<13);                                 */
/*===========================================================================*/

ULO rdmaconr_C(ULO address)
{
  if ((bltzero & 0x0000FFFF) != 0x00000000)
  {
    return dmaconr;
  }
  return (dmaconr | 0x00002000);
}

/*===========================================================================*/
/* VPOSR - $dff004 Read vpos and chipset ID bits                             */
/*                                                                           */
/* return lof | (graph_raster_y>>8);                                         */
/*===========================================================================*/

ULO rvposr_C(ULO address)
{
  if (blitter_ECS == 1)
  {
    return (lof | (graph_raster_y  >> 8)) | 0x2000;
  }
  return (lof | (graph_raster_y >> 8));  
}

/*===========================================================================*/
/* VHPOSR - $dff006 Read                                                     */
/*                                                                           */
/* return (graph_raster_x>>1) | ((graph_raster_y & 0xff)<<8);                */
/*===========================================================================*/

ULO rvhposr_C(ULO address)
{
  return (graph_raster_x >> 1) | ((graph_raster_y & 0x000000FF) << 8);
}

/*===========================================================================*/
/* ID - $dff07c Read                                                         */
/*                                                                           */
/* return 0xffff                                                             */
/*===========================================================================*/

ULO rid_C(ULO address)
{
  return 0x0000FFFF;
}

/*===========================================================================*/
/* VPOS - $dff02a Write                                                      */
/*                                                                           */
/* lof = data & 0x8000;                                                      */
/*===========================================================================*/

void wvpos_C(ULO data, ULO address)
{
  lof = data & 0x00008000;
}

/*===========================================================================*/
/* SERDAT - $dff030 Write                                                    */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wserdat_C(ULO data, ULO address)
{
}

/*===========================================================================*/
/* SERPER - $dff032 Write                                                    */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wserper_C(ULO data, ULO address)
{
}

/*===========================================================================*/
/* DIWSTRT - $dff08e Write                                                   */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wdiwstrt_C(ULO data, ULO address)
{
  diwstrt = data;
  diwytop = (data >> 8) & 0x000000FF;
  if (diwxright == 472)
  {
    diwxleft = 88;
  }
  else
  {
    diwxleft = data & 0x000000FF;
  }
  graphCalculateWindow_C();
}

/*===========================================================================*/
/* DIWSTOP - $dff090 Write                                                   */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wdiwstop_C(ULO data, ULO address)
{
  diwstop = data;
  if ((((data >> 8) & 0xff) & 0x80) == 0x0)
  {
    diwybottom = ((data >> 8) & 0xff) + 256;  
  }
  else
  {
    diwybottom = (data >> 8) & 0xff;
  }

  if (((data & 0xff) ^ 0x100) < 457)
  {
    diwxleft = diwstrt & 0xff;
    diwxright = (data & 0xff) ^ 0x100;
  }
  else
  {
    diwxleft = 88;
    diwxright = 472;
  }
  graphCalculateWindow_C();
}

/*


	
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


  */


/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void graphEndOfFrame(void) {
  graph_playfield_on = FALSE;
}

/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void graphHardReset(void) {
  graphIORegistersClear();
  graphLineDescClear();
}


/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void graphEmulationStart(void) {
  graphLineDescClear();
  graphIOHandlersInstall();
}


/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void graphEmulationStop(void) {
}


/*===========================================================================*/
/* Initializes the graphics module                                           */
/*===========================================================================*/

void graphStartup(void) {
  graphP2CTablesInit();
  graphLineDescClear();
  graphIORegistersClear();
  graphSetAllowBplLineSkip(TRUE);
}


/*===========================================================================*/
/* Release resources taken by the graphics module                            */
/*===========================================================================*/

void graphShutdown(void) {
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 to 4 (or 6) bitplanes hires (or lores)     */
/*===========================================================================*/

// Decode the odd part of the first 4 pixels
static __inline ULO graphDecodeOdd1(int bitplanes, ULO dat1, ULO dat3, ULO dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][0]; 
    case 3:
    case 4: return graph_deco1[dat1][0] | graph_deco3[dat3][0]; 
	case 5:
    case 6: return graph_deco1[dat1][0] | graph_deco3[dat3][0] | graph_deco5[dat5][0]; 
  }
  return 0;
}

// Decode the odd part of the last 4 pixels
static __inline ULO graphDecodeOdd2(int bitplanes, ULO dat1, ULO dat3, ULO dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][1]; 
    case 3:
    case 4: return graph_deco1[dat1][1] | graph_deco3[dat3][1]; 
	case 5:
    case 6: return graph_deco1[dat1][1] | graph_deco3[dat3][1] | graph_deco5[dat5][1]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeEven1(int bitplanes, ULO dat2, ULO dat4, ULO dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][0]; 
    case 4: 
    case 5: return graph_deco2[dat2][0] | graph_deco4[dat4][0];  
    case 6: return graph_deco2[dat2][0] | graph_deco4[dat4][0] | graph_deco6[dat6][0]; 

  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeEven2(int bitplanes, ULO dat2, ULO dat4, ULO dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][1]; 
    case 4:  
	case 5: return graph_deco2[dat2][1] | graph_deco4[dat4][1]; 
    case 6: return graph_deco2[dat2][1] | graph_deco4[dat4][1] | graph_deco6[dat6][1]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeDual1(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][0]; 
    case 4: 
    case 5: return graph_deco1[datA][0] | graph_deco2[datB][0];  
    case 6: return graph_deco1[datA][0] | graph_deco2[datB][0] | graph_deco3[datC][0]; 

  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeDual2(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][1]; 
    case 4:  
	case 5: return graph_deco1[datA][1] | graph_deco2[datB][1]; 
    case 6: return graph_deco1[datA][1] | graph_deco2[datB][1] | graph_deco3[datC][1]; 
  }
  return 0;
}

// Add modulo to the bitplane ptrs
static __inline void graphDecodeModulo(int bitplanes, ULO bpl_length_in_bytes)
{
  switch (bitplanes)
  {
    case 6: bpl6pt = (bpl6pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 5: bpl5pt = (bpl5pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
    case 4: bpl4pt = (bpl4pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 3: bpl3pt = (bpl3pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
    case 2: bpl2pt = (bpl2pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 1: bpl1pt = (bpl1pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
  }
}

static __inline void graphDecodeGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;

  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0) 
  {
    ULO *dest_odd;
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_odd;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6; 
	int maxscroll;
	ULO temp = 0;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;
    
	  if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
	  {
		  // high resolution
		  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddhiscroll);		

		  // Find out how many pixels the bitplane is scrolled
		  // the first pixels must then be zeroed to avoid garbage.
		  maxscroll = (evenhiscroll > oddhiscroll) ? evenhiscroll : oddhiscroll; 
	  } 
	  else 
	  {
		  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddscroll);			

		  // Find out how many pixels the bitplane is scrolled
		  // the first pixels must then be zeroed to avoid garbage.
		  maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 
	  }

	while (maxscroll > 0)
	{
	  graph_line1_tmp[graph_DDF_start + temp] = 0;
	  graph_line1_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
	  maxscroll--;
	  temp++;
	}
    end_odd = dest_odd + bpl_length_in_bytes * 2; 
    
	if (bitplanes > 1)
    {
		if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
		{
			// high resolution
			dest_even = (ULO*) (graph_line1_tmp + graph_DDF_start + evenhiscroll);
		}
		else
		{
			// low resolution
			dest_even = (ULO*) (graph_line1_tmp + graph_DDF_start + evenscroll);
		}
		end_even = dest_even + bpl_length_in_bytes * 2; 
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2) 
    {
      if (bitplanes >= 1) dat1 = *pt1_tmp++;
      if (bitplanes >= 3) dat3 = *pt3_tmp++;
	  if (bitplanes >= 5) dat5 = *pt5_tmp++;
      dest_tmp[0] = graphDecodeOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        if (bitplanes >= 2) dat2 = *pt2_tmp++;
        if (bitplanes >= 4) dat4 = *pt4_tmp++;
		if (bitplanes >= 6) dat6 = *pt6_tmp++;
			dest_tmp[0] |= graphDecodeEven1(bitplanes, dat2, dat4, dat6);
			dest_tmp[1] |= graphDecodeEven2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

static __inline void graphDecodeDualGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;
  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0) 
  {
    ULO *dest_odd;
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_odd;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6; 
  
	  int maxscroll;
	  ULO temp;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

	// clear edges
    maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 

	  temp = 0;
	  while (maxscroll > 0) {
		  graph_line1_tmp[graph_DDF_start + temp] = 0;
		  graph_line2_tmp[graph_DDF_start + temp] = 0;
		  graph_line1_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
		  graph_line2_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
		  maxscroll--;
		  temp++;
	  }
	  
	  // setup loop
	  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddscroll);			
    end_odd = dest_odd + bpl_length_in_bytes * 2; 
    
	if (bitplanes > 1)
    {
		// low resolution
		dest_even = (ULO*) (graph_line2_tmp + graph_DDF_start + evenscroll);
		end_even = dest_even + bpl_length_in_bytes * 2; 
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2) 
    {
      if (bitplanes >= 1) dat1 = *pt1_tmp++;
      if (bitplanes >= 3) dat3 = *pt3_tmp++;
	    if (bitplanes >= 5) dat5 = *pt5_tmp++;
      dest_tmp[0] = graphDecodeDual1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeDual2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        if (bitplanes >= 2) dat2 = *pt2_tmp++;
        if (bitplanes >= 4) dat4 = *pt4_tmp++;
		    if (bitplanes >= 6) dat6 = *pt6_tmp++;
			  dest_tmp[0] = graphDecodeDual1(bitplanes, dat2, dat4, dat6);
			  dest_tmp[1] = graphDecodeDual2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode0_C(void)
{
  graphDecodeGeneric(0);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode1_C(void)
{
  graphDecodeGeneric(1);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode2_C(void)
{
  graphDecodeGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode3_C(void)
{
  graphDecodeGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode4_C(void)
{
  graphDecodeGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode5_C(void)
{
  graphDecodeGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode6_C(void)
{
  graphDecodeGeneric(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode2Dual_C(void)
{
	graphDecodeDualGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode3Dual_C(void)
{
	graphDecodeDualGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode4Dual_C(void)
{
	graphDecodeDualGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode5Dual_C(void)
{
	graphDecodeDualGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode6Dual_C(void)
{
	graphDecodeDualGeneric(6);
}

/*===========================================================================*/
/* Calculate all variables connected to the screen emulation                 */
/* First calculate the ddf variables graph_DDF_start and graph_DDF_word_count*/
/*   Check if stop is bigger than start                                      */
/*   Check if strt and stop is mod 8 aligned                                 */
/*   If NOK then set numberofwords and startpos to 0 and                     */
/*   DIWvisible vars to 128                                                  */
/*                                                                           */
/* NOTE: The following is (was? worfje) (sadly) a prime example of spaghetti code.... (PS) */
/*       Though it somehow works, so just leave it and hope it does not break*/
/*===========================================================================*/


void graphCalculateWindow_C(void) 
{
	ULO ddfstop_aligned, ddfstrt_aligned, last_position_in_line;

	if ((bplcon0 & 0x8000) == 0x8000) // check if Hires bit is set (bit 15 of BPLCON0)
	{
		graphCalculateWindowHires_C();
	} 
	else 
	{
		// set DDF (Display Data Fetch Start) variables
		if (ddfstop < ddfstrt) 
		{
			graph_DDF_start = graph_DDF_word_count = 0;
			graph_DIW_first_visible = graph_DIW_last_visible = 256;
		}
		
		ddfstop_aligned = ddfstop & 0x07;
		ddfstrt_aligned = ddfstrt & 0x07;

		graph_DDF_word_count = ((ddfstop - ddfstrt) >> 3) + 1;
		if (ddfstop_aligned != ddfstrt_aligned) 
		{
			graph_DDF_word_count++;
		} 
		
		graph_DDF_start = ((ddfstrt << 1) + 0x11);	// 0x11 = 17 pixels = 8.5 bus cycles

		// set DIW (Display Window Start) variables (only used with drawing routines)
		
		if (diwxleft >= diwxright) 
		{
			graph_DDF_start = graph_DDF_word_count = 0;
			graph_DIW_first_visible = graph_DIW_last_visible = 256;
		}

		graph_DIW_first_visible = draw_left;
		if (diwxleft < graph_DDF_start) // cmp dword[diwxleft], dword[graph_DDF_start]
		{  
			// jb  .cwdiwless
			if (graph_DDF_start > draw_left) // cmp dword[graph_DDF_start], dword[draw_left]
			{  
				// ja  .cwdiwnoclip2
				graph_DIW_first_visible = graph_DDF_start;
			} 
		} 
		else 
		{
			if (diwxleft > draw_left) // cmp word[diwxleft], word[draw_left]
			{  
				// ja  .cwdiwnoclip
				graph_DIW_first_visible = diwxleft;
			} 
		}

		// .cwdiwlastpos:
		last_position_in_line = (graph_DDF_word_count << 4) + graph_DDF_start;
		if (oddscroll > evenscroll) // cmp word[oddscroll], word[evenscroll]
		{
			// ja  .cwaddodd
			last_position_in_line += oddscroll;
		} 
		else 
		{
			last_position_in_line += evenscroll;
		}

		// .cwdiwlastpos2:
		graph_DIW_last_visible = draw_right;
		if (last_position_in_line < diwxright) // cmp ecx, word[diwxright]
		{
			// jb  .cwdiwxx
			if (last_position_in_line < draw_right) // cmp ecx, word[draw_right]
			{
				// jb  .cwdiwnoclip4
				graph_DIW_last_visible = last_position_in_line;
			}
		} 
		else 
		{
			if (diwxright < draw_right) // cmp word[diwxright], word[draw_right]
			{
				// jb  .cwdiwnoclip3
				graph_DIW_last_visible = diwxright;
			}
		}
	}
}

void graphCalculateWindowHires_C(void)
{
	ULO ddfstop_aligned, ddfstrt_aligned, last_position_in_line;

	if (ddfstrt > ddfstop) // cmp dword[ddfstrt], dword[ddfstop]
	{
		// ja near cwdiwerrh
		graph_DDF_start = graph_DDF_word_count = 0;
		graph_DIW_first_visible = graph_DIW_last_visible = 256;
	}

	ddfstop_aligned = ddfstop & 0x07;
	ddfstrt_aligned = ddfstrt & 0x07;

	graph_DDF_word_count = (((ddfstop - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
	graph_DDF_start = (ddfstrt << 2) + 18;

	// DDF variables done, now check DIW variables

	if ((diwxleft << 1) >= (diwxright << 1)) 
	{
		graph_DDF_start = graph_DDF_word_count = 0;
		graph_DIW_first_visible = graph_DIW_last_visible = 256;
	}

	if ((diwxleft << 1) < graph_DDF_start) 
	{
		if (graph_DDF_start > (draw_left << 1)) 
		{
			graph_DIW_first_visible = graph_DDF_start;
		} 
		else
		{
			graph_DIW_first_visible = draw_left << 1;
		}
	}
	else
	{
		if ((diwxleft << 1) > (draw_left << 1))
		{
			graph_DIW_first_visible = diwxleft << 1;
		}
		else
		{
			graph_DIW_first_visible = draw_left << 1;
		}
	}
	
	last_position_in_line = graph_DDF_start + (graph_DDF_word_count << 4);
	if (oddhiscroll > evenhiscroll) // cmp word[oddhiscroll], word[evenhiscroll]
	{
		// ja  .cwaddoddh
		last_position_in_line += oddhiscroll;
	} 
	else 
	{
		last_position_in_line += evenhiscroll;
	}

	if (last_position_in_line < (diwxright << 1)) 
	{
		if (last_position_in_line < (draw_right << 1))
		{
			graph_DIW_last_visible = last_position_in_line;
		}
		else
		{
			graph_DIW_last_visible = draw_right << 1;
		}
	}
	else
	{
		if ((diwxright << 1) < (draw_right << 1)) 
		{
			graph_DIW_last_visible = diwxright << 1;
		}
		else
		{
			graph_DIW_last_visible = draw_right << 1;
		}
	}
}

void graphPlayfieldOnOff(void)
{
	if (graph_playfield_on != 0) 
	{
		// Playfield on, check if top has moved below graph_raster_y
		// Playfield on, check if playfield is turned off at this line
		if (!(graph_raster_y != diwybottom))
		{
			// %%nopfspecial:
			graph_playfield_on = 0;
		}
	} 
	else 
	{
		// Playfield off, Check if playfield is turned on at this line
		if (!(graph_raster_y != diwytop))
		{
			// Don't turn on when top > bottom
			if (diwytop < diwybottom)
			{
				graph_playfield_on = 1;
			}
		}
	}
	// OK, here the state of the playfield is taken care of
}

void graphDecodeNOP_C(void)
{
    switch ((bplcon0 >> 12) & 0x07) {
    case 0:
      break;
    case 6:
      bpl6pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 5:
      bpl5pt += (graph_DDF_word_count << 1) + bpl1mod;
    case 4:
      bpl4pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 3:
      bpl3pt += (graph_DDF_word_count << 1) + bpl1mod;
    case 2:
      bpl2pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 1:
      bpl1pt += (graph_DDF_word_count << 1) + bpl1mod;
      break;
    }
}

/*===========================================================================*/
/* End of line handler for graphics                                          */
/*===========================================================================*/

void graphEndOfLine_C(void)
{
  
}


/*
void graphEndOfLine(void)
{

	graph_line* current_graph_line;

	// skip this frame?
	if (draw_frame_skip != 0) 
	{
		// update diw state
		graphPlayfieldOnOff();

		// skip lines before line $12
		if (graph_raster_y >= 0x12) {

			// make pointer to linedesc for this line
			current_graph_line = &graph_frame[draw_buffer_draw][graph_raster_y];
			// (graph_raster_y * graph_line_end) + (graph_line_end * 314 * draw_buffer_draw));

			// decode sprites if DMA is enabled and raster is after line $18
			if ((dmacon & 0x20) == 0x20)
			{
				if (graph_raster_y >= 0x18) 
				{
					spritesDecode();
				}
			}
			
			// sprites decoded, sprites_onlineflag is set if there are any
			
			// update pointer to drawing routine
			update_drawmode();

			// check if we are clipped

			if ((graph_raster_y >= draw_top) || (graph_raster_y < draw_bottom))
			{
				// .decode
				// visible line, either background or bitplanes
				if (graph_allow_bpl_line_skip == 0) 
				{
					_graphComposeLineOutput_
				}
				else
				{
					_graphComposeLineOutputSmart_
				}
				// add	esp, 4 ????????????????

			}
			else
			{
				// .nop_line
				// do nop decoding, no screen output needed
		
				// pop ebp ?????????????????
				if (draw_line_BG_routine != draw_line_routine)
				{
					// push dword .drawend
					graphDecodeNOP;
				}
			}

			// if diw state changing from background to bitplane output,
		    // set new drawing routine pointer

			// .draw_end
			if (draw_switch_bg_to_bpl != 0) {

				draw_line_BPL_manage_routine = draw_line_routine;
				draw_switch_bg_to_bpl = 0

			}

		}
	}
	

	// .skip_frame
	_graphSpriteHack_

		// no return??? or did something get pushed on the stack?
}
*/