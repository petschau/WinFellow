/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Sprite emulation                                                           */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "graph.h"
#include "sprite.h"
#include "draw.h"


BOOLE sprite_improvement = TRUE;
ULO sprite_to_block = 0;

/*===========================================================================*/
/* Sprite registers and derived data                                         */
/*===========================================================================*/

ULO sprpt[8];
ULO sprx[8];
ULO spry[8];
ULO sprly[8];
ULO spratt[8];
UWO sprdat[8][2];

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

ULO sprite4_0[4] = {(ULO) sprite_deco01, (ULO) sprite_deco11,
                    (ULO) sprite_deco21, (ULO) sprite_deco31};
ULO sprite4_1[4] = {(ULO) sprite_deco02, (ULO) sprite_deco12,
                    (ULO) sprite_deco22, (ULO) sprite_deco32};

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
          sprite_deco4[m][n][q].i8[p] = ((q & (0x80>>p)) == 0) ? 0 : (((m + 4)<<4) | (1<<(n + 2)));

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



/*===========================================================================*/
/* Decide delay of sprite ptr update                                         */
/* ebx - cycle raster xpos must be past to trigger delay                     */
/*===========================================================================*/

BOOLE decidedelay_C(ULO data, ULO address, ULO cycle_raster_xpos)
{
  if (sprite_write_real != 1)
  {
    if (cycle_raster_xpos <= graph_raster_x)
    {
      *((ULO *) ((UBY *) sprite_write_buffer + sprite_write_next)) = address;
      *((ULO *) ((UBY *) sprite_write_buffer + sprite_write_next + 4)) = data;
      sprite_write_next += 8;
      return TRUE;      
    }
  }
  return FALSE;
}

/* SPR0PT - $dff120 and $dff122 */

void wspr0pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 2)) = data & 0x01f;
  }
}

void wspr0ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt)) = data & 0xfffe;
  }
}

/* SPR1PT - $dff124 and $dff126 */

void wspr1pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 6)) = data & 0x01f;
  }
}

void wspr1ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 4)) = data & 0xfffe;
  }
}

/* SPR2PT - $dff128 and $dff12a */

void wspr2pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 10)) = data & 0x01f;
  }
}

void wspr2ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 8)) = data & 0xfffe;
  }
}

/* SPR3PT - $dff128 and $dff12a */

void wspr3pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 14)) = data & 0x01f;
  }
}

void wspr3ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 12)) = data & 0xfffe;
  }
}

/* SPR3PT - $dff128 and $dff12a */

void wspr4pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 18)) = data & 0x01f;
  }
}

void wspr4ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 16)) = data & 0xfffe;
  }
}

/* SPR5PT - $dff120 and $dff122 */

void wspr5pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 22)) = data & 0x01f;
  }
}

void wspr5ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 20)) = data & 0xfffe;
  }
}

/* SPR1PT - $dff124 and $dff126 */

void wspr6pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 26)) = data & 0x01f;
  }
}

void wspr6ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 24)) = data & 0xfffe;
  }
}

/* SPR2PT - $dff128 and $dff12a */

void wspr7pth_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // store 21 bit (2 MB)
    *((UWO *) ((UBY *) sprpt + 30)) = data & 0x01f;
  }
}

void wspr7ptl_C(ULO data, ULO address)
{
  if (decidedelay_C(data, address, sprite_delay) == FALSE)
  {
    // no odd addresses
    *((UWO *) ((UBY *) sprpt + 28)) = data & 0xfffe;
  }
}

/* SPRXPOS - $dff140 to $dff178 */

void wsprxpos_C(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;

  if (sprite_improvement == TRUE)
  {
    // retrieve some of the horizontal and vertical position bits
    sprx[sprnr] = (sprx[sprnr] & 0x001) | ((data & 0xff) << 1);
    spry[sprnr] = (spry[sprnr] & 0x100) | ((data & 0xff00) >> 8);
  }
  else
  {
    sprx[sprnr] = (sprx[sprnr] & 1) | ((data & 0xff) << 1);
  }
}

/* SPRXCTL $dff142 to $dff17a */

void wsprxctl_C(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;

  if (sprite_improvement == TRUE)
  {
//    if (sprnr != sprite_to_block) {
    // retrieve the rest of the horizontal and vertical position bits
    sprx[sprnr] = (sprx[sprnr] & 0x1fe) | (data & 0x1);
    spry[sprnr] = (spry[sprnr] & 0x0ff) | ((data & 0x4) << 6);

    // when the sprite number is odd, is this right? 
    //if (sprnr & 1)
    //  spratt[sprnr & 6] = spratt[sprnr] = !!(data & 0x80);
    spratt[sprnr] = !!(data & 0x80);
    sprly[sprnr] = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);
//    }
  }
  else
  {
    sprx[sprnr] = (sprx[sprnr] & 0x1fe) | (data & 1);
    if (sprnr & 1)
      spratt[sprnr & 6] = spratt[sprnr] = !!(data & 0x80);
  }
}


/* SPRXDATA $dff144 to $dff17c */
/* The statestuff comes out pretty wrong..... */

void wsprxdata_C(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;
  *((UWO *) &sprdat[sprnr]) =  (UWO) data;
  if (graph_raster_y >= 0x1a) {
    if (sprite_state[sprnr] <= 3)
      sprite_state_old[sprnr] = sprite_state[sprnr];
    if (sprite_state[sprnr] != 3 && sprite_state[sprnr] != 5)
      sprite_state[sprnr] = 4;
    else
      sprite_state[sprnr] = 5;
    }
}

void wsprxdatb_C(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address >> 3) & 7;
  *(((UWO *) &sprdat[sprnr]) + 1) = (UWO) data;
  if (graph_raster_y >= 0x1a) {
    if (sprite_state[sprnr] <= 3)
      sprite_state_old[sprnr] = sprite_state[sprnr];
    if (sprite_state[sprnr] != 3 && sprite_state[sprnr] != 5)
      sprite_state[sprnr] = 4;
    else
      sprite_state[sprnr] = 5;
    }
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
							graph_raster_y,
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

  memorySetIOWriteStub(0x120, wspr0pth_C);
  memorySetIOWriteStub(0x122, wspr0ptl_C);
  memorySetIOWriteStub(0x124, wspr1pth_C);
  memorySetIOWriteStub(0x126, wspr1ptl_C);
  memorySetIOWriteStub(0x128, wspr2pth_C);
  memorySetIOWriteStub(0x12a, wspr2ptl_C);
  memorySetIOWriteStub(0x12c, wspr3pth_C);
  memorySetIOWriteStub(0x12e, wspr3ptl_C);
  memorySetIOWriteStub(0x130, wspr4pth_C);
  memorySetIOWriteStub(0x132, wspr4ptl_C);
  memorySetIOWriteStub(0x134, wspr5pth_C);
  memorySetIOWriteStub(0x136, wspr5ptl_C);
  memorySetIOWriteStub(0x138, wspr6pth_C);
  memorySetIOWriteStub(0x13a, wspr6ptl_C);
  memorySetIOWriteStub(0x13c, wspr7pth_C);
  memorySetIOWriteStub(0x13e, wspr7ptl_C);
  for (i = 0; i < 8; i++) {
    memorySetIOWriteStub(0x140 + i*8, wsprxpos_C);
    memorySetIOWriteStub(0x142 + i*8, wsprxctl_C);
    memorySetIOWriteStub(0x144 + i*8, wsprxdata_C);
    memorySetIOWriteStub(0x146 + i*8, wsprxdatb_C);
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
    sprx[i] = 0;
    spry[i] = 0;
    sprly[i] = 0;
    spratt[i] = 0;
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

void spriteEndOfLine(void) {
}


/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void spriteEndOfFrame(void) {
  ULO i;

  for (i = 0; i < 8; i++) sprite_state[i] = 0;
  sprite_ham_slot_next = 0;
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
}

void spriteDecode4Sprite_C(UBY spriteno)
{
  ULO sprite_class = spriteno >> 1;
  ULO* chunky_dest = (ULO *) (&sprite[spriteno][0]);
  UBY* word[2] = {
    (UBY *) &sprdat[spriteno][0], 
    (UBY *) &sprdat[spriteno][1]
  };
  ULO planardata;

  planardata = *word[0]++;
  chunky_dest[0] = sprite_deco4[sprite_class][0][planardata].i32[0];
  chunky_dest[1] = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = *word[0];
  chunky_dest[2] = sprite_deco4[sprite_class][0][planardata].i32[0];
  chunky_dest[3] = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = *word[1]++;
  chunky_dest[0] |= sprite_deco4[sprite_class][1][planardata].i32[0];
  chunky_dest[1] |= sprite_deco4[sprite_class][1][planardata].i32[1];

  planardata = *word[1];
  chunky_dest[2] |= sprite_deco4[sprite_class][1][planardata].i32[0];
  chunky_dest[3] |= sprite_deco4[sprite_class][1][planardata].i32[1];
}

void spriteDecode16Sprite_C(ULO spriteno)
{
  ULO *chunky_dest = (ULO *) (&sprite[spriteno][0]);
  UBY *word[4] = {(UBY*) &sprdat[spriteno & 0xfe][0], (UBY*) &sprdat[spriteno & 0xfe][1],
                  (UBY*) &sprdat[spriteno][0], (UBY*)&sprdat[spriteno][1]};
  ULO planardata;
  UBY bpl;

  if (spriteno == 7)
  {
    spriteno = spriteno;
  }

  planardata = *word[0]++;
  chunky_dest[0] = sprite_deco16[0][planardata].i32[0];
  chunky_dest[1] = sprite_deco16[0][planardata].i32[1];

  planardata = *word[0];
  chunky_dest[2] = sprite_deco16[0][planardata].i32[0];
  chunky_dest[3] = sprite_deco16[0][planardata].i32[1];

  for (bpl = 1; bpl < 4; bpl++)
  {  
    planardata = *word[bpl]++;
    chunky_dest[0] |= sprite_deco16[bpl][planardata].i32[0];
    chunky_dest[1] |= sprite_deco16[bpl][planardata].i32[1];

    planardata = *word[bpl];
    chunky_dest[2] |= sprite_deco16[bpl][planardata].i32[0];
    chunky_dest[3] |= sprite_deco16[bpl][planardata].i32[1];
  }
}

void spritesDecode_C(void) {
  // when sprite is in state 4, edx should hold the two control words
  // used when debugging emulator?
  // edx = edh:edl:dh:dl 
  // = AT,0,0,0,0,E8,L8,H0:L7,L6,L5,L4,L3,L2,L1,L0:
  //   H8,H7,H6,H5,H4,H3,H2,H1:E7,E6,E5,E4,E3,E2,E1,E0
  
  UBY spriteno;
//  UWO data1, data2;

 

  sprites_online = FALSE;
  spriteno = 0;

  while ((spriteno < 8) && (spriteno * 4 <= sprite_ddf_kill)) 
  {
    sprite_online[spriteno] = FALSE;
    sprite_16col[spriteno] = FALSE;
    
    switch(sprite_state[spriteno]) 
    {
    case 0: // state 0, reading two control words
      // .dsrwc0
      spratt[spriteno] = !!(memory_chip[sprpt[spriteno] + 3] & 0x80);  // get bit AT
      sprx[spriteno]   = (memory_chip[sprpt[spriteno] + 1] << 1) | (memory_chip[sprpt[spriteno] + 3] & 0x01); // get H8 to H0
      sprx[spriteno]   = sprx[spriteno] + 1;  // delay x position with 1 cycle
      spry[spriteno]   = memory_chip[sprpt[spriteno]] | ((memory_chip[sprpt[spriteno] + 3] & 0x04) << 6); // get E8 to E0
      sprly[spriteno]  = memory_chip[sprpt[spriteno] + 2] | ((memory_chip[sprpt[spriteno] + 3] & 0x02) << 7); // get L8 to L0
      sprite_state[spriteno] = 1;
      sprdat[spriteno][0] = ((memory_chip[sprpt[spriteno] + 1]) << 8) + memory_chip[sprpt[spriteno]]; 
      sprdat[spriteno][1] = ((memory_chip[sprpt[spriteno] + 3]) << 8) + memory_chip[sprpt[spriteno] + 2];
      sprpt[spriteno] = sprpt[spriteno] + 4; // point to next two data words
      break;

    case 1: // state 1, waiting for first line and in case of first line, decode first two data words
      if (graph_raster_y == spry[spriteno]) 
      {
        sprite_state[spriteno] = 2;
        
        // read next two data words
        sprdat[spriteno][0] = ((memory_chip[sprpt[spriteno] + 1]) << 8) + memory_chip[sprpt[spriteno]]; 
        sprdat[spriteno][1] = ((memory_chip[sprpt[spriteno] + 3]) << 8) + memory_chip[sprpt[spriteno] + 2];
        sprpt[spriteno] = sprpt[spriteno] + 4; // point to next two data words

        if ((spriteno & 0x01) == 0)
        {
          // even sprite 
          if (spratt[spriteno + 1] == 0) // if attached, work is done when handling odd sprite 
          {
            // even sprite not attached to next odd sprite -> 3 color decode 
            spriteDecode4Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;

            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
            }
          }
        }
        else
        {
          // odd sprite (formerly dsoddsprite)
          if (spratt[spriteno] == 0) 
          {
            // odd sprite not attached to previous even sprite -> 3 color decode 
            spriteDecode4Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;

            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
            }
          }
          else
          {
            // odd sprite is attached to even sprite 
            spriteDecode16Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;
            sprite_16col[spriteno] = TRUE;
            
            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
              sprite_state[spriteno - 1] = 0;
            }
          }
        }
      }
      break;

    case 2: // state 2, read two data words and decode them
      if (graph_raster_y == sprly[spriteno])
      {
        if (spratt[spriteno] == 1) {
          sprite_state[spriteno - 1] = 0;
          sprx[spriteno - 1]         = 0;
          spry[spriteno - 1]         = 0;
          sprly[spriteno - 1]        = 0;
          spratt[spriteno - 1]       = 0;
        }  
        sprite_state[spriteno] = 0;
        sprx[spriteno]         = 0;
        spry[spriteno]         = 0;
        sprly[spriteno]        = 0;
        spratt[spriteno]       = 0;
      } 
      else 
      {
        // read next two data words (if in state 2)
        if (sprite_state[spriteno] == 2) {
        sprdat[spriteno][0] = ((memory_chip[sprpt[spriteno] + 1]) << 8) + memory_chip[sprpt[spriteno]]; 
        sprdat[spriteno][1] = ((memory_chip[sprpt[spriteno] + 3]) << 8) + memory_chip[sprpt[spriteno] + 2];
        }
        sprpt[spriteno] = sprpt[spriteno] + 4; // point to next two data words

        if ((spriteno & 0x01) == 0)
        {
          // even sprite 
          if (spratt[spriteno + 1] == 0) // if attached, work is done when handling odd sprite 
          {
            // even sprite not attached to next odd sprite -> 3 color decode 
            spriteDecode4Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;

            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
            }
          }
        }
        else
        {
          // odd sprite (formerly dsoddsprite)
          if (spratt[spriteno] == 0) 
          {
            // odd sprite not attached to previous even sprite -> 3 color decode 
            spriteDecode4Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;

            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
            }
          }
          else
          {
            spriteDecode16Sprite_C(spriteno);
            sprites_online = TRUE;
            sprite_online[spriteno] = TRUE;
            sprite_16col[spriteno] = TRUE;
            
            // check for last line
            if ((graph_raster_y + 1) == sprly[spriteno])
            {
              sprite_state[spriteno] = 0; // return to state 'read two control words'
              sprite_state[spriteno  - 1] = 0; // return to state 'read two control words'
            }
          }
        }
      }
      break;

    case 3:
      break;
    }
    spriteno++;
  }
}
