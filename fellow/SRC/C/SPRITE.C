/* @(#) $Id: SPRITE.C,v 1.7 2008-11-03 21:12:10 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Sprite emulation                                                        */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*         Worfje                                                          */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "graph.h"
#include "sprite.h"
#include "draw.h"
#include "listtree.h"
#include "bus.h"


//#define DRAW_TSC_PROFILE

#ifdef DRAW_TSC_PROFILE

LLO spritemergelores_tmp = 0;
LLO spritemergelores = 0;
LON spritemergelores_times = 0;

LLO spritemergehires_tmp = 0;
LLO spritemergehires = 0;
LON spritemergehires_times = 0;

LLO spritemergedual_tmp = 0;
LLO spritemergedual = 0;
LON spritemergedual_times = 0;

LLO spritewsprpt_tmp = 0;
LLO spritewsprpt = 0;
LON spritewsprpt_times = 0;

LLO spritewsprpos_tmp = 0;
LLO spritewsprpos = 0;
LON spritewsprpos_times = 0;

LLO spritewsprctl_tmp = 0;
LLO spritewsprctl = 0;
LON spritewsprctl_times = 0;

LLO spritewsprdata_tmp = 0;
LLO spritewsprdata = 0;
LON spritewsprdata_times = 0;

LLO spritewsprdatb_tmp = 0;
LLO spritewsprdatb = 0;
LON spritewsprdatb_times = 0;



/*============================================================================*/
/* profiling help functions                                                   */
/*============================================================================*/

static __inline spriteTscBefore(LLO* a)
{
  LLO local_a = *a;
  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx,10h
    rdtsc
    pop     ecx
    mov     dword ptr [local_a], eax
    mov     dword ptr [local_a + 4], edx
    pop     edx
    pop     eax
  }
  *a = local_a;
}

static __inline spriteTscAfter(LLO* a, LLO* b, ULO* c)
{
  LLO local_a = *a;
  LLO local_b = *b;
  ULO local_c = *c;

  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx, 10h
    rdtsc
    pop     ecx
    sub     eax, dword ptr [local_a]
    sbb     edx, dword ptr [local_a + 4]
    add     dword ptr [local_b], eax
    adc     dword ptr [local_b + 4], edx
    inc     dword ptr [local_c]
    pop     edx
    pop     eax
  }
  *a = local_a;
  *b = local_b;
  *c = local_c;
}


#endif

ULO sprite_to_block = 0;
BOOLE output_sprite_log = FALSE;	
BOOLE output_action_sprite_log = FALSE;

/*===========================================================================*/
/* Sprite registers and derived data                                         */
/*===========================================================================*/

ULO sprpt[8];
ULO sprpt_debug[8];
ULO sprx[8];
ULO sprx_debug[8];
ULO spry[8];
ULO spry_debug[8];
ULO sprly[8];
ULO sprly_debug[8];
ULO spratt[8];
UWO sprdat[8][2];
BOOLE spr_arm_data[8];
BOOLE spr_arm_comparator[8];

spr_action_list_master spr_action_list[8];
spr_action_list_master spr_dma_action_list[8];
spr_merge_list_master spr_merge_list[8];

/* Increases the item count with 1 and returns the new (uninitialized) item */
spr_action_list_item* spriteActionListAddLast(spr_action_list_master* l)
{
  return &l->items[l->count++];
}

/* Returns the number of items in the list */
ULO spriteActionListCount(spr_action_list_master* l)
{
  return l->count;
}

/* Returns the list item at position i */
spr_action_list_item* spriteActionListGet(spr_action_list_master* l, ULO i)
{
  if (i >= l->count) return NULL;
  return &l->items[i];
}

/* Clears the list */
void spriteActionListClear(spr_action_list_master* l)
{
  l->count = 0;
}

/* Makes room for an item in the list based on the raster x position of the action */
/* Returns the new uninitialized item. */
spr_action_list_item* spriteActionListAddSorted(spr_action_list_master* l, ULO raster_x, ULO raster_y)
{
  ULO i;
  for (i = 0; i < l->count; i++)
  {
    if (l->items[i].raster_y >= raster_y && l->items[i].raster_x > raster_x)
    {
      ULO j;
      for (j = l->count; j > i; --j) l->items[j] = l->items[j - 1];
      l->count++;
      return &l->items[i];
    }
  }
  return spriteActionListAddLast(l);
}

/* Increases the item count with 1 and returns the new (uninitialized) item */
spr_merge_list_item* spriteMergeListAddLast(spr_merge_list_master* l)
{
  return &l->items[l->count++];
}

/* Returns the number of items in the list */
ULO spriteMergeListCount(spr_merge_list_master* l)
{
  return l->count;
}

/* Returns the list item at position i */
spr_merge_list_item* spriteMergeListGet(spr_merge_list_master* l, ULO i)
{
  if (i >= l->count) return NULL;
  return &l->items[i];
}

/* Clears the list */
void spriteMergeListClear(spr_merge_list_master* l)
{
  l->count = 0;
}

spr_register_func sprxptl_functions[8] =
{
	aspr0ptl,
	aspr1ptl,
	aspr2ptl,
	aspr3ptl,
	aspr4ptl,
	aspr5ptl,
	aspr6ptl,
	aspr7ptl
};

spr_register_func sprxpth_functions[8] =
{
	aspr0pth,
	aspr1pth,
	aspr2pth,
	aspr3pth,
	aspr4pth,
	aspr5pth,
	aspr6pth,
	aspr7pth
};

// NOT FOR COMMIT
UBY buffer[128];
// END NOT FOR COMMIT

ULO sprite_state[8];
ULO sprite_state_old[8];
ULO sprite_16col[8];
ULO sprite_online[8];
ULO sprites_online;
ULO sprite_ddf_kill;

/*===========================================================================*/
/* Sprite appearance data                                                    */
/*===========================================================================*/

UBY sprite[8][16];

typedef struct {
  UBY data[8][16];
  ULO x[8];
  BOOLE online[8];
} sprite_ham_slot;

sprite_ham_slot sprite_ham_slots[313];
ULO sprite_ham_slot_next;

ULO sprite_delay;

UBY sprite_deco01[256][8];
UBY sprite_deco11[256][8];
UBY sprite_deco21[256][8];
UBY sprite_deco31[256][8];
UBY sprite_deco02[256][8];
UBY sprite_deco12[256][8];
UBY sprite_deco22[256][8];
UBY sprite_deco32[256][8];
UBY sprite_deco03[256][8];
UBY sprite_deco04[256][8];

typedef union sprite_deco_
{
  UBY i8[8];
  ULO i32[2];
} sprite_deco;

sprite_deco sprite_deco4[4][2][256];
sprite_deco sprite_deco16[4][256];

ULO sprite_write_buffer[128][2];
ULO sprite_write_next;
ULO sprite_write_real;


/*===========================================================================*/
/* Used when sprites are merged with the line1 arrays                        */
/* syntax: spritetranslate[0 - behind, 1 - front][bitplanedata][spritedata]  */
/*===========================================================================*/

UBY sprite_translate[2][256][256];


/*===========================================================================*/
/* Initialize the sprite translation tables                                  */
/*===========================================================================*/

void spriteTranslationTableInit(void) {
  ULO i, j, k, l;

  for (k = 0; k < 2; k++)
    for (i = 0; i < 256; i++)
      for (j = 0; j < 256; j++) {
        if (k == 0) l = (i == 0) ? j : i;
        else l = (j == 0) ? i : j;
        sprite_translate[k][i][j] = (UBY) l;
      }
}

void spriteP2CTablesInitialize_ALSO(void)
{
  ULO m, n, q, p;

  for (m = 0; m < 4; m++)
    for (n = 0; n < 2; n++)
      for (q = 0; q < 256; q++) 
        for (p = 0; p < 8; p++)
          sprite_deco4[m][n][q].i8[p] = (UBY) (((q & (0x80>>p)) == 0) ? 0 : (((m + 4)<<4) | (1<<(n + 2))));

  for (n = 0; n < 4; n++)
    for (q = 0; q < 256; q++) 
      for (p = 0; p < 8; p++)
        sprite_deco16[n][q].i8[p] = ((q & (0x80>>p)) == 0) ? 0 :  (64 | (1<<(n + 2)));
}

void spriteP2CTablesInitialize(void)
{
  ULO i, j;

  for (i = 0; i < 256; i++) 
    for (j = 0; j < 8; j++) {
      sprite_deco01[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x44;
      sprite_deco02[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x48;
      sprite_deco03[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x50;
      sprite_deco04[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x60;
      sprite_deco11[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x54;
      sprite_deco12[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x58;
      sprite_deco21[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x64;
      sprite_deco22[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x68;
      sprite_deco31[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x74;
      sprite_deco32[i][j] =  (i & (0x80>>j)) == 0 ? 0:0x78;
    }
}


/*===========================================================================*/
/* Save sprite data for later processing on HAM bitmaps                      */
/*===========================================================================*/

void spriteMergeHAM(graph_line *linedesc) {
  sprite_ham_slot *ham_slot = &sprite_ham_slots[sprite_ham_slot_next];
  linedesc->sprite_ham_slot = sprite_ham_slot_next;
  sprite_ham_slot_next++;
  memcpy(ham_slot->data, sprite, 128);
  memcpy(ham_slot->online, sprite_online, 32);
  memcpy(ham_slot->x, sprx, 32);
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 8-bit pixels, 1x horisontal scale                                         */
/*===========================================================================*/

void spriteMergeHAM1x8(UBY *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    UBY *frame_ptr = frameptr + (first_visible_cylinder -
					 DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

            while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) *frame_ptr = (UBY) graph_color_shadow[pixel>>2];
	      frame_ptr++;
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 8-bit pixels, 2x horisontal scale                                         */
/*===========================================================================*/

void spriteMergeHAM2x8(UWO *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    UWO *frame_ptr = frameptr + (first_visible_cylinder -
					 DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) *frame_ptr = (UWO) graph_color_shadow[pixel>>2];
	      frame_ptr++;
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 1x horisontal scale                                        */
/*===========================================================================*/

void spriteMergeHAM1x16(UWO *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    UWO *frame_ptr = frameptr + (first_visible_cylinder -
					 DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) *frame_ptr = (UWO) graph_color_shadow[pixel>>2];
	      frame_ptr++;
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void spriteMergeHAM2x16(ULO *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    ULO *frame_ptr = frameptr + (first_visible_cylinder -
					 DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) *frame_ptr = (ULO) graph_color_shadow[pixel>>2];
	      frame_ptr++;
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 1x horisontal scale                                        */
/*===========================================================================*/

union sprham24helper {
  ULO color_i;
  UBY color_b[4];
};

void spriteMergeHAM1x24(UBY *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    UBY *frame_ptr = frameptr + 3*(first_visible_cylinder -
					   DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) {
		union sprham24helper color;
		color.color_i = graph_color_shadow[pixel>>2];
	        *frame_ptr++ = color.color_b[0];
	        *frame_ptr++ = color.color_b[1];
	        *frame_ptr++ = color.color_b[2];
	      }
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void spriteMergeHAM2x24(UBY *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    UBY *frame_ptr = frameptr + 6*(first_visible_cylinder -
					   DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) {
		union sprham24helper color;
		color.color_i = graph_color_shadow[pixel>>2];
	        *frame_ptr++ = color.color_b[0];
	        *frame_ptr++ = color.color_b[1];
	        *frame_ptr++ = color.color_b[2];
	        *frame_ptr++ = color.color_b[0];
	        *frame_ptr++ = color.color_b[1];
	        *frame_ptr++ = color.color_b[2];
	      }
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 1x horisontal scale                                        */
/*===========================================================================*/

void spriteMergeHAM1x32(ULO *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    ULO *frame_ptr = frameptr + (first_visible_cylinder -
					   DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) *frame_ptr = graph_color_shadow[pixel>>2];
	      frame_ptr++;
	    }
	  }					
	}
      }
    }
  }
}


/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void spriteMergeHAM2x32(ULO *frameptr, graph_line *linedesc) {
  if (linedesc->sprite_ham_slot != 0xffffffff) {
    ULO i;
    sprite_ham_slot *ham_slot = &sprite_ham_slots[linedesc->sprite_ham_slot];
    ULO DIW_first_visible = linedesc->DIW_first_draw;
    ULO DIW_last_visible = DIW_first_visible + linedesc->DIW_pixel_count;
    
    linedesc->sprite_ham_slot = 0xffffffff;
    for (i = 0; i < 8; i++) {
      if (ham_slot->online[i]) {
        if ((ham_slot->x[i] < DIW_last_visible) &&
	    ((ham_slot->x[i] + 16) > DIW_first_visible)) {
	  ULO first_visible_cylinder = ham_slot->x[i];
	  ULO last_visible_cylinder = first_visible_cylinder + 16;

	  if (first_visible_cylinder < DIW_first_visible)
	    first_visible_cylinder = DIW_first_visible;
	  if (last_visible_cylinder > DIW_last_visible)
	    last_visible_cylinder = DIW_last_visible;
	  {
	    UBY *spr_ptr = &(ham_slot->data[i][first_visible_cylinder -
	                                       ham_slot->x[i]]);
	    /* frameptr points to the first visible HAM pixel in the framebuffer */
	    ULO *frame_ptr = frameptr + 2*(first_visible_cylinder -
					   DIW_first_visible);
	    LON pixel_count = last_visible_cylinder - first_visible_cylinder;

	    while (--pixel_count >= 0) {
	      UBY pixel = *spr_ptr++;
	      if (pixel != 0) {
		ULO color = graph_color_shadow[pixel>>2];
		*frame_ptr = color;
		*(frame_ptr + 1) = color;
	      }
	      frame_ptr += 2;
	    }
	  }			
	}
      }
    }
  }
}

/*===========================================================================*/
/* Sprite control registers                                                  */
/*===========================================================================*/

static void spriteBuildItem(spr_action_list_item ** item)
{
  ULO currentX = busGetRasterX();
  if (currentX >= 18) 
  {
    // Petter has put an delay in the Copper calls of 16 cycles, we need to compensate for that
    if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // hires (x position is cycle position times four)
      (*item)->raster_x = (currentX - 16) * 4;
    }
    else
    {
      // lores (x position is cycle position times two)
      (*item)->raster_x = (currentX - 20) * 2;
    }
  }
  else
  {
    if ((bplcon0 & 0x8000) == 0x8000)
    {
      // hires
      (*item)->raster_x = 8;
    }
    else
    {
      // lores
      (*item)->raster_x = 4;
    }
  }
  (*item)->raster_y = busGetRasterY();
}

/* SPRXPT */
/* Makes a log of the writes to the wsprpt registers */
/* PETTER */

void wsprpt(UWO data, ULO address)
{
  spr_action_list_item * item;
  ULO sprnr = (address >> 2) & 7;

#ifdef DRAW_TSC_PROFILE
  spriteTscBefore(&spritewsprpt_tmp);
#endif

  item = spriteActionListAddLast(&spr_dma_action_list[sprnr]);
  spriteBuildItem(&item);
  item->called_function = (address & 0x2) ? sprxptl_functions[sprnr] : sprxpth_functions[sprnr];
  item->data = data;
  item->address = address;

  // for debugging only
  if (output_sprite_log == TRUE) {
    *((UWO *) ((UBY *) sprpt_debug + sprnr * 4 + 2)) = (UWO) data & 0x01f;
    sprintf(buffer, "(y, x) = (%d, %d): call to spr%dpth (sprx = %d, spry = %d, sprly = %d)\n", 
			busGetRasterY(), 2*(busGetRasterX() - 16), sprnr, (memory_chip[sprpt_debug[sprnr] + 1] << 1) | (memory_chip[sprpt_debug[sprnr] + 3] & 0x01), memory_chip[sprpt_debug[sprnr]] | ((memory_chip[sprpt_debug[sprnr] + 3] & 0x04) << 6), memory_chip[sprpt_debug[sprnr] + 2] | ((memory_chip[sprpt_debug[sprnr] + 3] & 0x02) << 7));
    fellowAddLog2(buffer);
  }
#ifdef DRAW_TSC_PROFILE
  spriteTscAfter(&spritewsprpt_tmp, &spritewsprpt, &spritewsprpt_times);
#endif
}

void aspr0pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 2)) = data & 0x01f;
}
void aspr0ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt)) = data & 0xfffe;
}

void aspr1pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 6)) = data & 0x01f;
}
void aspr1ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 4)) = data & 0xfffe;
}

void aspr2pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 10)) = data & 0x01f;
}
void aspr2ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 8)) = data & 0xfffe;
}

void aspr3pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 14)) = data & 0x01f;
}
void aspr3ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 12)) = data & 0xfffe;
}

void aspr4pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 18)) = data & 0x01f;
}
void aspr4ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 16)) = data & 0xfffe;
}

void aspr5pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 22)) = data & 0x01f;
}
void aspr5ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 20)) = data & 0xfffe;
}
void aspr6pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 26)) = data & 0x01f;
}
void aspr6ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 24)) = data & 0xfffe;
}

void aspr7pth(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 30)) = data & 0x01f;
}
void aspr7ptl(UWO data, ULO address)
{
  *((UWO *) ((UBY *) sprpt + 28)) = data & 0xfffe;
}

/* SPRXPOS - $dff140 to $dff178 */

void wsprxpos(UWO data, ULO address) 
{
  ULO sprnr;
  spr_action_list_item * item;

#ifdef DRAW_TSC_PROFILE
  spriteTscBefore(&spritewsprpos_tmp);
#endif

  sprnr = (address >> 3) & 7;
  item = spriteActionListAddLast(&spr_action_list[sprnr]);
  spriteBuildItem(&item);
  item->called_function = asprxpos;
  item->data = data;
  item->address = address;

  // for debugging only
  sprx_debug[sprnr] = (sprx_debug[sprnr] & 0x001) | ((data & 0xff) << 1);
  spry_debug[sprnr] = (spry_debug[sprnr] & 0x100) | ((data & 0xff00) >> 8);
  if (output_sprite_log == TRUE) {
    sprintf(buffer, "(y, x) = (%d, %d): call to spr%dpos (sprx = %d, spry = %d)\n", busGetRasterY(), 2*(busGetRasterX() - 16), sprnr, sprx_debug[sprnr], sprly_debug[sprnr]);
    fellowAddLog2(buffer);
  }
#ifdef DRAW_TSC_PROFILE
  spriteTscAfter(&spritewsprpos_tmp, &spritewsprpos, &spritewsprpos_times);
#endif
}

void asprxpos(UWO data, ULO address)
{
  ULO sprnr;

  sprnr = (address >> 3) & 7;

  // retrieve some of the horizontal and vertical position bits
  sprx[sprnr] = (sprx[sprnr] & 0x001) | ((data & 0xff) << 1);
  spry[sprnr] = (spry[sprnr] & 0x100) | ((data & 0xff00) >> 8);
}

/* SPRXCTL $dff142 to $dff17a */

void wsprxctl(UWO data, ULO address)
{
  ULO sprnr;
  spr_action_list_item * item;

#ifdef DRAW_TSC_PROFILE
  spriteTscBefore(&spritewsprctl_tmp);
#endif

  sprnr = (address >> 3) & 7;
  item = spriteActionListAddLast(&spr_action_list[sprnr]);
  spriteBuildItem(&item);
  item->called_function = asprxctl;
  item->data = data;
  item->address = address;

  // for debugging only
  sprx_debug[sprnr] = (sprx_debug[sprnr] & 0x1fe) | (data & 0x1);
  spry_debug[sprnr] = (spry_debug[sprnr] & 0x0ff) | ((data & 0x4) << 6);
  sprly_debug[sprnr] = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);
  if (output_sprite_log == TRUE) {
    sprintf(buffer, "(y, x) = (%d, %d): call to spr%dctl (sprx = %d, spry = %d, sprly = %d)\n", busGetRasterY(), 2*(busGetRasterX() - 16), sprnr, sprx_debug[sprnr], spry_debug[sprnr], sprly_debug[sprnr]);
    fellowAddLog2(buffer);
  }
#ifdef DRAW_TSC_PROFILE
  spriteTscAfter(&spritewsprctl_tmp, &spritewsprctl, &spritewsprctl_times);
#endif
}


void asprxctl(UWO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;

  // retrieve the rest of the horizontal and vertical position bits
  sprx[sprnr] = (sprx[sprnr] & 0x1fe) | (data & 0x1);
  spry[sprnr] = (spry[sprnr] & 0x0ff) | ((data & 0x4) << 6);

  // attach bit only applies when having an odd sprite
  if (sprnr & 1) 
  {
    spratt[sprnr & 6] = !!(data & 0x80);
  }
  spratt[sprnr] = !!(data & 0x80);
  sprly[sprnr] = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);

	spr_arm_data[sprnr] = FALSE;
}

/* SPRXDATA $dff144 to $dff17c */

void wsprxdata(UWO data, ULO address)
{
  ULO sprnr;
  spr_action_list_item * item;

#ifdef DRAW_TSC_PROFILE
  spriteTscBefore(&spritewsprdata_tmp);
#endif

  sprnr = (address >> 3) & 7;
  item = spriteActionListAddLast(&spr_action_list[sprnr]);
  spriteBuildItem(&item);
  item->called_function = asprxdata;
  item->data = data;
  item->address = address;

	// for debugging only
  if (output_sprite_log == TRUE) {
    sprintf(buffer, "(y, x) = (%d, %d): call to spr%ddata\n", busGetRasterY(), 2*(busGetRasterX() - 16), sprnr);
    fellowAddLog2(buffer);
  }
#ifdef DRAW_TSC_PROFILE
  spriteTscAfter(&spritewsprdata_tmp, &spritewsprdata, &spritewsprdata_times);
#endif
}

void asprxdata(UWO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;
  *((UWO *) &sprdat[sprnr]) = (UWO) data;

	spr_arm_data[sprnr] = TRUE;
}

void wsprxdatb(UWO data, ULO address)
{
  ULO sprnr;
  spr_action_list_item * item;

#ifdef DRAW_TSC_PROFILE
  spriteTscBefore(&spritewsprdatb_tmp);
#endif

  sprnr = (address >> 3) & 7;
  item = spriteActionListAddLast(&spr_action_list[sprnr]);
  spriteBuildItem(&item);
  item->called_function = asprxdatb;
  item->data = data;
  item->address = address;

	// for debugging only
  if (output_sprite_log == TRUE) {
    sprintf(buffer, "(y, x) = (%d, %d): call to spr%ddatb\n", busGetRasterY(), 2*(busGetRasterX() - 16), sprnr);
    fellowAddLog2(buffer);
  }
#ifdef DRAW_TSC_PROFILE
  spriteTscAfter(&spritewsprdatb_tmp, &spritewsprdatb, &spritewsprdatb_times);
#endif
}

void asprxdatb(UWO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;
  *(((UWO *) &sprdat[sprnr]) + 1) = data; 
}

/*===========================================================================*/
/* Sprite decode, C-version                                                  */
/*===========================================================================*/

typedef enum {
  SPRITE_STATE_CONTROL_WORDS = 0,
  SPRITE_STATE_WAITING_FOR_FIRST_LINE = 1,
  SPRITE_STATE_
} sprite_states;

void spritesLog(void) {
  ULO no;

  for (no = 0; no < 8; no++) {
    char s[80];
//    if (sprite_online[no]) {
      sprintf(s, "%d %d, sprite %d fy %d ly %d x %d state %d att %d atto %d pt %.6X\n", draw_frame_count,
							busGetRasterY(),
							no,
							spry[no],
							sprly[no],
							sprx[no],
							sprite_state[no],
							spratt[no & 6],
							spratt[no | 1],
							sprpt[no]);
      fellowAddLog(s);
  //  }
  }
}

/*===========================================================================*/
/* Map IO registers into memory space                                        */
/*===========================================================================*/

void spriteIOHandlersInstall(void) {
  ULO i;

  for (i = 0x120; i < 0x140; i = i + 2)
    memorySetIoWriteStub(i, wsprpt);

  for (i = 0; i < 8; i++)
  {
    memorySetIoWriteStub(0x140 + i*8, wsprxpos);
    memorySetIoWriteStub(0x142 + i*8, wsprxctl);
    memorySetIoWriteStub(0x144 + i*8, wsprxdata);
    memorySetIoWriteStub(0x146 + i*8, wsprxdatb);
  }
}


/*===========================================================================*/
/* Delays any sprite operation by a given amount of cycles, depending on how */
/* the copper operates.                                                      */
/*===========================================================================*/

void spriteSetDelay(ULO delay) {
  sprite_delay = delay;
}


/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void spriteIORegistersClear(void) {
  ULO i, j;

  for (i = 0; i < 8; i++) {
    sprpt[i] = 0;
		sprpt_debug[i] = 0;
    sprx[i] = 0;
    spry[i] = 0;
    sprly[i] = 0;
    spratt[i] = 0;
		spr_arm_data[i] = FALSE;
		spr_arm_comparator[i] = FALSE;
    for (j = 0; j < 2; j++) sprdat[i][j];
    sprite_state[i] = 0;
    sprite_state_old[i] = 0;
    sprite_16col[i] = FALSE;
    sprite_online[i] = FALSE;  
    for (j = 0; j < 16; j++) sprite[i][j];
  }
  sprites_online = FALSE;
  for (i = 0; i < 128; i++)
    for (j = 0; j < 2; j++)
      sprite_write_buffer[i][j];
  sprite_write_next = 0;
  sprite_write_real = 0;

}

void spriteLogActiveSprites(void) {
  char s[128];
  int i;
  for (i = 0; i < 8; i++) {
    if (sprite_online[i]) {
      sprintf(s, "Sprite %d position %d\n", i, sprx[i]);
      fellowAddLog(s);
    }
  }
}


/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void spriteHardReset(void) {
  spriteIORegistersClear();
}


/*===========================================================================*/
/* Called on emulation end of line                                           */
/*===========================================================================*/

void spriteEndOfLine(void)
{
  ULO i;

  for (i = 0; i < 8; i++) 
  {
    spriteMergeListClear(&spr_merge_list[i]);
  }
}


/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void spriteEndOfFrame(void) {
  ULO i;

  for (i = 0; i < 8; i++) 
  {
    sprite_state[i] = 0;
    spr_arm_data[i] = FALSE;
    spr_arm_comparator[i] = FALSE;
    spriteActionListClear(&spr_action_list[i]);
    spriteActionListClear(&spr_dma_action_list[i]);
//    spriteDMAActionListClear(i);
  }
  sprite_ham_slot_next = 0;
//  spriteActionListsClear();
}


/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void spriteEmulationStart(void) {
  spriteIOHandlersInstall();
}


/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void spriteEmulationStop(void) {
}


/*===========================================================================*/
/* Initializes the graphics module                                           */
/*===========================================================================*/

void spriteStartup(void) {
  ULO i;

  for (i = 0; i < 8; i++) 
  {
    sprite_state[i] = 0;
    spr_arm_data[i] = FALSE;
    spr_arm_comparator[i] = FALSE;
    spriteActionListClear(&spr_action_list[i]);
    spriteActionListClear(&spr_dma_action_list[i]);
    spriteMergeListClear(&spr_merge_list[i]);
//    spriteDMAActionListClear(i);
  }
  sprite_ham_slot_next = 0;
//  spriteActionListsClear();


  spriteP2CTablesInitialize();
  spriteP2CTablesInitialize_ALSO();
  spriteTranslationTableInit();
  spriteIORegistersClear();
  sprite_ddf_kill = 32;                     /* Not modified during emulation */
  sprite_delay = 40;                     /* This is controlled by the copper */
}


/*===========================================================================*/
/* Release resources taken by the sprites module                             */
/*===========================================================================*/

void spriteShutdown(void) {
#ifdef DRAW_TSC_PROFILE
  {
  FILE *F = fopen("spriteprofile.txt", "w");
  fprintf(F, "FUNCTION\tTOTALCYCLES\tCALLEDCOUNT\tAVGCYCLESPERCALL\n");
  fprintf(F, "SpriteMergeLores()\t%I64d\t%d\t%I64d\n", spritemergelores, spritemergelores_times, (spritemergelores_times == 0) ? 0 : (spritemergelores / spritemergelores_times));
  fprintf(F, "SpriteMergeHires()\t%I64d\t%d\t%I64d\n", spritemergehires, spritemergehires_times, (spritemergehires_times == 0) ? 0 : (spritemergehires / spritemergehires_times));
  fprintf(F, "SpriteMergeDual()\t%I64d\t%d\t%I64d\n", spritemergedual, spritemergedual_times, (spritemergedual_times == 0) ? 0 : (spritemergedual / spritemergedual_times));
  fprintf(F, "WSPRPT()\t%I64d\t%d\t%I64d\n", spritewsprpt, spritewsprpt_times, (spritewsprpt_times == 0) ? 0 : (spritewsprpt / spritewsprpt_times));
  fprintf(F, "WSPRPOS()\t%I64d\t%d\t%I64d\n", spritewsprpos, spritewsprpos_times, (spritewsprpos_times == 0) ? 0 : (spritewsprpos / spritewsprpos_times));
  fprintf(F, "WSPRCTL()\t%I64d\t%d\t%I64d\n", spritewsprctl, spritewsprctl_times, (spritewsprctl_times == 0) ? 0 : (spritewsprctl / spritewsprctl_times));
  fprintf(F, "WSPRDATA()\t%I64d\t%d\t%I64d\n", spritewsprdata, spritewsprdata_times, (spritewsprdata_times == 0) ? 0 : (spritewsprdata / spritewsprdata_times));
  fprintf(F, "WSPRDATB()\t%I64d\t%d\t%I64d\n", spritewsprdatb, spritewsprdatb_times, (spritewsprdatb_times == 0) ? 0 : (spritewsprdatb / spritewsprdatb_times));
  fclose(F);
  }
#endif
}

void spriteDecode4Sprite_C(ULO sprnr)
{
	spr_merge_list_item * item;
	ULO sprite_class = sprnr >> 1;
	//ULO* chunky_dest = (ULO *) (&sprite[sprnr][0]);
	ULO* chunky_dest;
	UBY* word[2] = {
		(UBY *) &sprdat[sprnr][0], 
		(UBY *) &sprdat[sprnr][1]
	};
	ULO planardata;

	item = spriteMergeListAddLast(&spr_merge_list[sprnr]);
	item->sprx = sprx[sprnr];
	chunky_dest = (ULO *) (item->sprite_data);

	planardata = *word[0]++;
	chunky_dest[2] = sprite_deco4[sprite_class][0][planardata].i32[0];
	chunky_dest[3] = sprite_deco4[sprite_class][0][planardata].i32[1];

	planardata = *word[0];
	chunky_dest[0] = sprite_deco4[sprite_class][0][planardata].i32[0];
	chunky_dest[1] = sprite_deco4[sprite_class][0][planardata].i32[1];

	planardata = *word[1]++;
	chunky_dest[2] |= sprite_deco4[sprite_class][1][planardata].i32[0];
	chunky_dest[3] |= sprite_deco4[sprite_class][1][planardata].i32[1];

	planardata = *word[1];
	chunky_dest[0] |= sprite_deco4[sprite_class][1][planardata].i32[0];
	chunky_dest[1] |= sprite_deco4[sprite_class][1][planardata].i32[1];
}

void spriteDecode16Sprite_C(ULO sprnr)
{
	spr_merge_list_item * item;
	//ULO *chunky_dest = (ULO *) (&sprite[sprnr][0]);
	ULO *chunky_dest;
	UBY *word[4] = {
		(UBY*) &sprdat[sprnr & 0xfe][0], 
		(UBY*) &sprdat[sprnr & 0xfe][1],
		(UBY*) &sprdat[sprnr][0], 
		(UBY*)&sprdat[sprnr][1]
	};
	ULO planardata;
	UBY bpl;

	item = spriteMergeListAddLast(&spr_merge_list[sprnr]);
	item->sprx = sprx[sprnr];
	chunky_dest = (ULO *) (item->sprite_data);

	planardata = *word[0]++;
	chunky_dest[2] = sprite_deco16[0][planardata].i32[0];
	chunky_dest[3] = sprite_deco16[0][planardata].i32[1];

	planardata = *word[0];
	chunky_dest[0] = sprite_deco16[0][planardata].i32[0];
	chunky_dest[1] = sprite_deco16[0][planardata].i32[1];

	for (bpl = 1; bpl < 4; bpl++)
	{  
		planardata = *word[bpl]++;
		chunky_dest[2] |= sprite_deco16[bpl][planardata].i32[0];
		chunky_dest[3] |= sprite_deco16[bpl][planardata].i32[1];

		planardata = *word[bpl];
		chunky_dest[0] |= sprite_deco16[bpl][planardata].i32[0];
		chunky_dest[1] |= sprite_deco16[bpl][planardata].i32[1];
	}
}

void spritesDMASpriteHandler(void) {
  spr_action_list_item * dma_action_item;
  spr_action_list_item * item;
  ULO local_sprx;
  ULO local_spry;
  ULO local_sprly;
  ULO local_data_ctl;
  ULO local_data_pos;
  ULO i, count;
  ULO sprnr;
  ULO currentY = busGetRasterY();

  sprites_online = FALSE;
  sprnr = 0;
  
  while (sprnr < 8) 
  {
    switch(sprite_state[sprnr]) 
    {
    case 0:
      count = spriteActionListCount(&spr_dma_action_list[sprnr]);
      // inactive (waiting for a write to sprxptl)
      for (i = 0; i < count; i++)
      {
        // move through DMA action list
        dma_action_item = spriteActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
          // a write to sprxptl was made during this line, execute it now
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);

          // data from sprxpos
	  local_data_pos = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	  local_spry = ((local_data_pos & 0xff00) >> 8);
          local_sprx = ((local_data_pos & 0xff) << 1);

          // data from sprxctl
	  local_data_ctl = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	  local_sprx = (local_sprx & 0x1fe) | (local_data_ctl & 0x1);
          local_spry = (local_spry & 0x0ff) | ((local_data_ctl & 0x4) << 6);
	  local_sprly = ((local_data_ctl & 0xff00) >> 8) | ((local_data_ctl & 0x2) << 7);

	  if (((local_spry < local_sprly) ) || ((local_data_ctl == 0) && (local_data_pos == 0)))
            //((spr_action_list_item *) (dma_action_item->node))->raster_x < 71
            //if ((graph_raster_y < 24) || (((spr_action_list_item *) (dma_action_item->node))->raster_x < 71))
	  {
	    // insert a write to sprxpos at time raster_x
	    item = spriteActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);
            
	    item->raster_x = dma_action_item->raster_x;
	    item->raster_y = dma_action_item->raster_y;
	    item->called_function = asprxpos;
	    item->data = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	    item->address = sprnr << 3;
            
	    // insert a write to sprxctl at time raster_x
	    item = spriteActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);
	    item->raster_x = dma_action_item->raster_x;
	    item->raster_y = dma_action_item->raster_y;
	    item->called_function = asprxctl;
	    item->data = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	    item->address = sprnr << 3;
	  }

          if ((local_spry < local_sprly) && (local_sprx > 40))
          {
            // point to next two data words
	    sprpt[sprnr] = sprpt[sprnr] + 4; 
          }

          if ((currentY < 25) && ((local_data_ctl == 0) && (local_data_pos == 0)))
          {
            sprite_state[sprnr] = 0;
          }
          else
          {
            sprite_state[sprnr] = 1;
          }
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);
        }
      }
      spriteActionListClear(&spr_dma_action_list[sprnr]);
      break;

    case 1: 
      // waiting for (graph_raster_y == spry)
      if ((currentY >= spry[sprnr]) && (currentY < sprly[sprnr]))
      {
	//if (graph_raster_y != sprly[sprnr])
	if (TRUE)
	{
	  // we can start to display the first line of the sprite
   
	  // insert a write to sprxdatb 
	  item = spriteActionListAddSorted(&spr_action_list[sprnr], 60, currentY);
	  item->raster_x = 60;
	  item->raster_y = currentY;
	  item->called_function = asprxdatb;
	  item->data = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	  item->address = sprnr << 3;

	  // insert a write to sprxdata 
	  item = spriteActionListAddSorted(&spr_action_list[sprnr], 61, currentY);
	  item->raster_x = 61;
	  item->raster_y = currentY;
	  item->called_function = asprxdata;
	  item->data = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	  item->address = sprnr << 3;

	  // point to next two data words
	  sprpt[sprnr] = sprpt[sprnr] + 4; 

	  // we move to state 2 to wait for the last line sprly
	  sprite_state[sprnr] = 2;
	}
      }

      // handle writes to sprxptl and sprxpth
      count = spriteActionListCount(&spr_dma_action_list[sprnr]);
      for (i = 0; i < count; i++)
      {
	// move through DMA action list
	dma_action_item = spriteActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
	  // a write to sprxptl was made during this line, execute it now
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);
        }
      }
      spriteActionListClear(&spr_dma_action_list[sprnr]);
      break;

    case 2: 
      // waiting for (graph_raster_y == sprly)
      if (currentY == sprly[sprnr]) 
      {
	// we interpret the next two data words as the next two control words
	local_data_ctl = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	local_spry = ((local_data_ctl & 0xff00) >> 8);
	local_data_pos = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	local_spry = (local_spry & 0x0ff) | ((local_data_pos & 0x4) << 6);
	local_sprly = ((local_data_pos & 0xff00) >> 8) | ((local_data_pos & 0x2) << 7);

	//if ((local_spry <= local_sprly) || ((local_data_ctl == 0) && (local_data_pos == 0)))
        if (TRUE)
	{
	  // insert a write to sprxpos at time raster_x
	  item = spriteActionListAddSorted(&spr_action_list[sprnr], 0, currentY);
	  item->raster_x = 0;
	  item->raster_y = currentY;
	  item->called_function = asprxpos;
	  item->data = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	  item->address = sprnr << 3;
        
	  // insert a write to sprxctl at time raster_x
	  item = spriteActionListAddSorted(&spr_action_list[sprnr], 1, currentY);
	  item->raster_x = 1;
	  item->raster_y = currentY;
	  item->called_function = asprxctl;
	  item->data = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	  item->address = sprnr << 3;
        } 
 
        if (local_spry < local_sprly)
        {
          // point to next two data words
	  sprpt[sprnr] = sprpt[sprnr] + 4; 
        }

        if ((local_data_ctl == 0) && (local_data_pos == 0))
        {
          sprite_state[sprnr] = 0;
        }
        else
        {
          sprite_state[sprnr] = 1;
        }
      }
      else
      {
	// we can continue to display the next line of the sprite
	// insert a write to sprxdatb 
	item = spriteActionListAddSorted(&spr_action_list[sprnr], 60, currentY);
	item->raster_x = 60;
	item->raster_y = currentY;
	item->called_function = asprxdatb;
	item->data = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	item->address = sprnr << 3;

	// insert a write to sprxdata 
	item = spriteActionListAddSorted(&spr_action_list[sprnr], 61, currentY);
	item->raster_x = 61;
	item->raster_y = currentY;
	item->called_function = asprxdata;
	item->data = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	item->address = sprnr << 3;

	// point to next two data words
	sprpt[sprnr] = sprpt[sprnr] + 4; 
      }

      // handle writes to sprxptl and sprxpth
      count = spriteActionListCount(&spr_dma_action_list[sprnr]);
      for (i = 0; i < count; i++)
      {
        // move through DMA action list
        dma_action_item = spriteActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
          // a write to sprxptl was made during this line, execute it now
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);

          // data from sprxpos
	  local_data_pos = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	  local_spry = ((local_data_pos & 0xff00) >> 8);
          local_sprx = ((local_data_pos & 0xff) << 1);

          // data from sprxctl
	  local_data_ctl = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	  local_sprx = (local_sprx & 0x1fe) | (local_data_ctl & 0x1);
          local_spry = (local_spry & 0x0ff) | ((local_data_ctl & 0x4) << 6);
	  local_sprly = ((local_data_ctl & 0xff00) >> 8) | ((local_data_ctl & 0x2) << 7);

	  if (((local_spry < local_sprly) ))
          //if ((graph_raster_y < 24) || (((spr_action_list_item *) (dma_action_item->node))->raster_x < 71))
	  {
	    // insert a write to sprxpos at time raster_x
	    item = spriteActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);
	    item->raster_x = dma_action_item->raster_x;
	    item->raster_y = dma_action_item->raster_y;
	    item->called_function = asprxpos;
	    item->data = ((memory_chip[sprpt[sprnr]]) << 8) + memory_chip[sprpt[sprnr] + 1];
	    item->address = sprnr << 3;
           
	    // insert a write to sprxctl at time raster_x
	    item = spriteActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x + 1, dma_action_item->raster_y);
	    item->raster_x = dma_action_item->raster_x + 1;
	    item->raster_y = dma_action_item->raster_y;
	    item->called_function = asprxctl;
	    item->data = ((memory_chip[sprpt[sprnr] + 2]) << 8) + memory_chip[sprpt[sprnr] + 3];
	    item->address = sprnr << 3;
	  }

          if ((local_spry < local_sprly) && (local_sprx > 40))
          {
            // point to next two data words
	    sprpt[sprnr] = sprpt[sprnr] + 4; 
          }

          if ((currentY < 25) && ((local_data_ctl == 0) && (local_data_pos == 0)))
          {
            sprite_state[sprnr] = 0;
          }
          else
          {
	    sprite_state[sprnr] = 1;
          }					
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          dma_action_item->called_function(dma_action_item->data, dma_action_item->address);
        }
      }
      spriteActionListClear(&spr_dma_action_list[sprnr]);
      break;  
    }
    sprnr++;
  }
}

void spriteProcessActionList(void)
{
  ULO sprnr;
  spr_action_list_item * action_item;
  ULO x_pos;
  ULO i, count;

  sprites_online = FALSE;
  sprnr = 0;
  while (sprnr < 8) 
  {
    x_pos = 0;
    sprite_online[sprnr] = FALSE;
    sprite_16col[sprnr] = FALSE;

    count = spriteActionListCount(&spr_action_list[sprnr]);
    for (i = 0; i < count; i++)
    {
      action_item = spriteActionListGet(&spr_action_list[sprnr], i);
      if (spr_arm_data[sprnr] == TRUE)
      {
	// flipflop is armed by a write to sprxdata
	if (action_item->raster_x > sprx[sprnr])
	{
	  if (sprx[sprnr] > x_pos)
	  {
	    // for lores the horizontal blanking is until 71
	    if (sprx[sprnr] >= 71) 
	    {
	      // the comparator is armed before the coming action item, we may display the sprite data
	      if ((sprnr & 0x01) == 0)
	      {
		// even sprite 
		if (spratt[sprnr + 1] == 0) // if attached, work is done when handling odd sprite 
		{
		    // even sprite not attached to next odd sprite -> 3 color decode 
		    spriteDecode4Sprite_C(sprnr);
		    sprites_online = TRUE;
		    sprite_online[sprnr] = TRUE;
		}
	      }
	      else
	      {
		// odd sprite 
		if (spratt[sprnr] == 0) 
		{
		  // odd sprite not attached to previous even sprite -> 3 color decode 
		  spriteDecode4Sprite_C(sprnr);
		  sprites_online = TRUE;
		  sprite_online[sprnr] = TRUE;
		}
		else
		{
		  spriteDecode16Sprite_C(sprnr);
		  sprites_online = TRUE;
		  sprite_online[sprnr] = TRUE;
		  sprite_16col[sprnr] = TRUE;
		}
	      }

	      x_pos = sprx[sprnr];

	      // for debugging only
	      if (output_action_sprite_log == TRUE) {
		sprintf((char *) &buffer, "sprite %d data displayed on (y, x) = (%d, %d)\n", 
				sprnr, busGetRasterY(), sprx[sprnr]);
				fellowAddLog2(buffer);
	      }
	    }
	  }
	}
      }
      // we can execute the coming action item
      action_item->called_function(action_item->data, action_item->address);
      x_pos = action_item->raster_x;
    }

    // check if flipflop is armed, maybe the comparator will also get armed in time left
    if ((spr_arm_data[sprnr] == TRUE) && (x_pos <= sprx[sprnr]))
    {
      // for lores the horizontal blanking is until 71
      if (sprx[sprnr] >= 71) 
      {
	// the comparator becomes armed in time left, we may display the sprite data
	if ((sprnr & 0x01) == 0)
	{
	  // even sprite 
	  if (spratt[sprnr + 1] == 0) // if attached, work is done when handling odd sprite 
	  {
	    // even sprite not attached to next odd sprite -> 3 color decode 
	    spriteDecode4Sprite_C(sprnr);
	    sprites_online = TRUE;
	    sprite_online[sprnr] = TRUE;
	  }
	}
	else
	{
	  // odd sprite 
	  if (spratt[sprnr] == 0) 
	  {
	    // odd sprite not attached to previous even sprite -> 3 color decode 
	    spriteDecode4Sprite_C(sprnr);
	    sprites_online = TRUE;
	    sprite_online[sprnr] = TRUE;
	  }
	  else
	  {
	    spriteDecode16Sprite_C(sprnr);
	    sprites_online = TRUE;
	    sprite_online[sprnr] = TRUE;
	    sprite_16col[sprnr] = TRUE;
	  }
	}

	// for debugging only
	if (output_action_sprite_log == TRUE) {
	  sprintf((char *) &buffer, "sprite %d data displayed on (y, x) = (%d, %d)\n", 
				sprnr, busGetRasterY(), sprx[sprnr]);
				fellowAddLog2(buffer);
	}
      }
    }
    // clear the list at the end
    spriteActionListClear(&spr_action_list[sprnr]);
    sprnr++;
  }
}

void spriteSetDebugging()
{
  if (output_sprite_log == TRUE)
  {
    output_sprite_log = FALSE;
    output_action_sprite_log = FALSE;
  }
  else
  {
    output_sprite_log = TRUE;
    output_action_sprite_log = TRUE;
  }
}

// current sprite is in front of playfield 2, and thus also in front of playfield 1
static void spriteMergeDualLoresPF2loopinfront2(graph_line* current_graph_line, ULO sprnr)
{
	ULO i, j;
	UBY *line2; 
	UBY *sprite_data; 
	UBY line2_buildup[4];
	spr_merge_list_item *next_item;

	ULO count = spriteMergeListCount(&spr_merge_list[sprnr]);
	for (j = 0; j < count; j++)
	{
		next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);
		line2 = current_graph_line->line2 + next_item->sprx + 1;
		sprite_data = next_item->sprite_data;

		for (i = 0; i < 4; i++)
		{
			*((ULO *) line2_buildup) = *((ULO *) line2);
			if ((UBY) (*((ULO *) sprite_data)) != 0)
			{
				//cl = dl; 
				line2_buildup[0] = (UBY) *((ULO *) sprite_data);
			}

			// mdlpf21:
			if ((UBY) ((*((ULO *) sprite_data)) >> 8) != 0)
			{
				//ch = dh; 
				line2_buildup[1] = (UBY) ((*((ULO *) sprite_data) >> 8));
			}

			// mdlph22:
			if ((UBY) ((*((ULO *) sprite_data)) >> 16) != 0)
			{
				//cl = dl; 
				line2_buildup[2] = (UBY) ((*((ULO *) sprite_data) >> 16));
			}

			// mdlpf23:
			if ((UBY) ((*((ULO *) sprite_data)) >> 24) != 0)
			{
				//ch = dh; 
				line2_buildup[3] = (UBY) ((*((ULO *) sprite_data) >> 24));
			}

			// mdlpf24:
			*((ULO *) line2) = *((ULO *) line2_buildup);
			sprite_data += 4;
			line2 += 4;
		}
	}
}

// current sprite is behind of playfield 2, but in front of playfield 1
static void spriteMergeDualLoresPF1loopinfront2(graph_line* current_graph_line, ULO sprnr)
{
	ULO i, j;
	UBY *line1; 
	UBY *sprite_data; 
	UBY line_buildup[4];
	spr_merge_list_item *next_item;

	ULO count = spriteMergeListCount(&spr_merge_list[sprnr]);
	for (j = 0; j < count; j++)
	{
		next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);
		line1 = current_graph_line->line1 + next_item->sprx + 1;
		sprite_data = next_item->sprite_data;

		for (i = 0; i < 4; i++)
		{
			*((ULO *) line_buildup) = *((ULO *) line1);
			if ((UBY) (*((ULO *) sprite_data)) != 0)
			{
				//cl = dl; 
				line_buildup[0] = (UBY) *((ULO *) sprite_data);
			}

			// mdlpf21:
			if ((UBY) ((*((ULO *) sprite_data)) >> 8) != 0)
			{
				//ch = dh; 
				line_buildup[1] = (UBY) ((*((ULO *) sprite_data) >> 8));
			}

			// mdlph22:
			if ((UBY) ((*((ULO *) sprite_data)) >> 16) != 0)
			{
				//cl = dl; 
				line_buildup[2] = (UBY) ((*((ULO *) sprite_data) >> 16));
			}

			// mdlpf23:
			if ((UBY) ((*((ULO *) sprite_data)) >> 24) != 0)
			{
				//ch = dh; 
				line_buildup[3] = (UBY) ((*((ULO *) sprite_data) >> 24));
			}

			// mdlpf24:
			*((ULO *) line1) = *((ULO *) line_buildup);
			sprite_data += 4;
			line1 += 4;
		}
	}
}

// current sprite is behind of playfield 2, and also behind playfield 1
static void spriteMergeDualLoresPF1loopbehind2(graph_line* current_graph_line, ULO sprnr)
{
	ULO i, j;
	UBY *line1;
	UBY *sprite_data; 
	UBY line_buildup[4];
	spr_merge_list_item *next_item;

	ULO count = spriteMergeListCount(&spr_merge_list[sprnr]);
	for (j = 0; j < count; j++)
	{
		next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);
		line1 = current_graph_line->line1 + next_item->sprx + 1;
		sprite_data = next_item->sprite_data;

		for (i = 0; i < 4; i++)
		{
			*((ULO *) line_buildup) = *((ULO *) line1);
			if ((UBY) (*((ULO *) line1)) == 0)
			{
				if ((UBY) (*((ULO *) sprite_data)) != 0)
				{
					line_buildup[0] = (UBY) *((ULO *) sprite_data);
				}
			}

			// mdlb1:
			if ((UBY) ((*((ULO *) line1)) >> 8) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 8) != 0)
				{
					//ch = dh; 
					line_buildup[1] = (UBY) ((*((ULO *) sprite_data) >> 8));
				}
			}

			// mdlb2:
			if ((UBY) ((*((ULO *) line1)) >> 16) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 16) != 0)
				{
					//cl = dl; 
					line_buildup[2] = (UBY) ((*((ULO *) sprite_data) >> 16));
				}
			}

			// mdlb3:
			if ((UBY) ((*((ULO *) line1)) >> 24) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 24) != 0)
				{
					//ch = dh; 
					line_buildup[3] = (UBY) ((*((ULO *) sprite_data) >> 24));
				}
			}

			// mdlb4:
			*((ULO *) line1) = *((ULO *) line_buildup);
			sprite_data += 4;
			line1 += 4;
		}
	}
}

// current sprite is in behind of playfield 2, and thus also behind playfield 1
static void spriteMergeDualLoresPF2loopbehind2(graph_line* current_graph_line, ULO sprnr)
{
	ULO i, j;
	UBY *line2;
	UBY *sprite_data; 
	UBY line_buildup[4];
	spr_merge_list_item *next_item;

	ULO count = spriteMergeListCount(&spr_merge_list[sprnr]);
	for (j = 0; j < count; j++)
	{
		next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);
		line2 = current_graph_line->line2 + next_item->sprx + 1;
		sprite_data = next_item->sprite_data;

		for (i = 0; i < 4; i++)
		{
			*((ULO *) line_buildup) = *((ULO *) line2);
			if ((UBY) (*((ULO *) line2)) == 0)
			{
				if ((UBY) (*((ULO *) sprite_data)) != 0)
				{
					line_buildup[0] = (UBY) *((ULO *) sprite_data);
				}
			}

			// mdlpfb1:
			if ((UBY) ((*((ULO *) line2)) >> 8) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 8) != 0)
				{
					//ch = dh; 
					line_buildup[1] = (UBY) ((*((ULO *) sprite_data) >> 8));
				}
			}

			// mdlpfb2:
			if ((UBY) ((*((ULO *) line2)) >> 16) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 16) != 0)
				{
					//cl = dl; 
					line_buildup[2] = (UBY) ((*((ULO *) sprite_data) >> 16));
				}
			}

			// mdlpfb3:
			if ((UBY) ((*((ULO *) line2)) >> 24) == 0)
			{
				if ((UBY) ((*((ULO *) sprite_data)) >> 24) != 0)
				{
					//ch = dh; 
					line_buildup[3] = (UBY) ((*((ULO *) sprite_data) >> 24));
				}
			}

			// mdlpfb4:
			*((ULO *) line2) = *((ULO *) line_buildup);
			sprite_data += 4;
			line2 += 4;
		}
	}
}

static void spriteMergeDualPlayfield(graph_line* current_graph_line)
{
	ULO sprnr;

#ifdef DRAW_TSC_PROFILE
		spriteTscBefore(&spritemergedual_tmp);
  #endif

	for (sprnr = 0; sprnr < 8; sprnr++)
	{
		if (sprite_online[sprnr] == TRUE)
		{
			// there is sprite data waiting within this line
			//if (sprx[sprnr] <= graph_DIW_last_visible)
			//{
				// set destination and source 
				//line1 = ((current_graph_line->line1) + sprx[sprnr]);
				//sprite_data = sprite[sprnr];

				// determine whetever this sprite is in front of playfield 1 and/or in front or behind playfield 2
				if ((bplcon2 & 0x40) == 0x40)
				{
					// playfield 2 is in front of playfield 1
					if ((bplcon2 & 0x38) > sprnr*4)
					{
						// current sprite is in front of playfield 2, and thus also in front of playfield 1
						spriteMergeDualLoresPF2loopinfront2(current_graph_line, sprnr);
					}
					else
					{
						// current sprite is behind of playfield 2
						if (((bplcon2 & 0x7) << 1) > sprnr)
						{
							// current sprite is behind of playfield 2, but in front of playfield 1
							spriteMergeDualLoresPF1loopinfront2(current_graph_line, sprnr);
						}
						else
						{
							// current sprite is behind of playfield 2, and also behind playfield 1
							spriteMergeDualLoresPF1loopbehind2(current_graph_line, sprnr);
						}
					}
				} 
				else
				{
					// playfield 1 is in front of playfield 2
					if (((bplcon2 & 0x7) << 1) > sprnr)
					{
						// current sprite is in front of playfield 1, and thus also in front of playfield 2
						spriteMergeDualLoresPF1loopinfront2(current_graph_line, sprnr);
					}
					else
					{
						if ((bplcon2 & 0x38) > sprnr*4)
						{
							// current sprite is in front of playfield 2, but behind playfield 1 (in between)
							spriteMergeDualLoresPF2loopinfront2(current_graph_line, sprnr);
						}
						else
						{
							// current sprite is in behind of playfield 2, and thus also behind playfield 1
							spriteMergeDualLoresPF2loopbehind2(current_graph_line, sprnr);
						}
					}
				}
			//}
		}
	}
  #ifdef DRAW_TSC_PROFILE
		spriteTscAfter(&spritemergedual_tmp, &spritemergedual, &spritemergedual_times);
  #endif
}

static void spriteMergeHires(graph_line* current_graph_line)
{
	ULO sprnr;
	UBY *line1;
	UBY *sprite_data;
	spr_merge_list_item *next_item;
	ULO i;
	ULO in_front;
	ULO j, count;

  #ifdef DRAW_TSC_PROFILE
		spriteTscBefore(&spritemergehires_tmp);
  #endif

	for (sprnr = 0; sprnr < 8; sprnr++)
	{
		if (sprite_online[sprnr] == TRUE)
		{
			count = spriteMergeListCount(&spr_merge_list[sprnr]);
			for (j = 0; j < count; j++)
			{
				next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);

				// there is sprite data waiting within this line
				if (next_item->sprx <= graph_DIW_last_visible)
				{
					// set destination and source 
					line1 = (current_graph_line->line1 + 2*(next_item->sprx + 1));
					sprite_data = next_item->sprite_data;

					// determine whetever this sprite is in front or behind the playfield
					in_front = ((bplcon2 & 0x38) > (4 * sprnr)) ? 1 : 0;

					// merge sprite data within the line
					for (i = 0; i < 16; ++i)
					{
						ULO sdat = *sprite_data++;
						line1[0] = sprite_translate[in_front][line1[0]][sdat];
						line1[1] = sprite_translate[in_front][line1[1]][sdat];
						line1 += 2;
					}
				}
			}
		}
	}
  #ifdef DRAW_TSC_PROFILE
		spriteTscAfter(&spritemergehires_tmp, &spritemergehires, &spritemergehires_times);
  #endif
}

static void spriteMergeHires320(graph_line* current_graph_line)
{
	spriteMergeLores(current_graph_line);
}

static void spriteMergeLores(graph_line* current_graph_line)
{
	BYT sprnr;
	UBY *line1;
	UBY *sprite_data;
	spr_merge_list_item *next_item;
	ULO i;
	ULO in_front;
	ULO j, count;

  #ifdef DRAW_TSC_PROFILE
		spriteTscBefore(&spritemergelores_tmp);
  #endif

	for (sprnr = 7; sprnr >= 0; sprnr--)
	{
		if (sprite_online[sprnr] == TRUE)
		{
			count = spriteMergeListCount(&spr_merge_list[sprnr]);
			for (j = 0; j < count; j++)
			{
				next_item = spriteMergeListGet(&spr_merge_list[sprnr], j);
				// there is sprite data waiting within this line
				if (next_item->sprx <= graph_DIW_last_visible)
				{
					// set destination and source 
					line1 = (current_graph_line->line1 + (next_item->sprx) + 1);
					sprite_data = next_item->sprite_data;

					// determine whetever this sprite is in front or behind the playfield
					in_front = ((bplcon2 & 0x38) > (4 * (ULO)sprnr)) ? 1 : 0;

  					// merge sprite data within the line
					for (i = 0; i < 16; ++i)
					{
					  *line1++ = sprite_translate[in_front][*line1][*sprite_data++];
					}
				}
			}
		}
	}
  #ifdef DRAW_TSC_PROFILE
		spriteTscAfter(&spritemergelores_tmp, &spritemergelores, &spritemergelores_times);
  #endif

}

void spritesMerge(graph_line* current_graph_line)
{
	sprites_online = 0;
	
	if ((bplcon0 & 0x800) == 0x800)
	{
		// HAM mode bit is set
		spriteMergeHAM(current_graph_line);
		return;
	}

	if ((bplcon0 & 0x400) == 0x400)
	{
		// dual playfield bit is set
		spriteMergeDualPlayfield(current_graph_line);
		return;
	}

	if ((bplcon0 & 0x8000) == 0x8000)
	{
		// hires bit is set
		if (draw_hscale == 2)
		{
			// scaling set to two times			
			spriteMergeHires(current_graph_line);
		}
		else
		{
			// no scaling
			spriteMergeHires320(current_graph_line);
		}
	}
	else
	{
		// lores 
		spriteMergeLores(current_graph_line);
	}
}