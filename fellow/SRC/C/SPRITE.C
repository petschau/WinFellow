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
#include "fmem.h"
#include "graph.h"
#include "sprite.h"
#include "draw.h"


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
        sprite_translate[k][i][j] = l;
      }
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
  ULO i;
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

/* SPRXPOS - $dff140 to $dff178 */

void wsprxposC(ULO data, ULO address) {
  ULO sprnr;
  sprnr = (address>>3) & 7;
  sprx[sprnr] = (sprx[sprnr] & 1) | ((data & 0xff)<<1);
}

/* SPRXCTL $dff142 to $dff17a */

void wsprxctlC(ULO data, ULO address) {
  ULO sprnr;
  sprnr = (address>>3) & 7;
  sprx[sprnr] = (sprx[sprnr] & 0x1fe) | (data & 1);
  if (sprnr & 1)
    spratt[sprnr & 6] = spratt[sprnr] = !!(data & 0x80);
}


/* SPRXDATA $dff144 to $dff17c */
/* The statestuff comes out pretty wrong..... */

void wsprxdataC(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address>>3) & 7;
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

void wsprxdatbC(ULO data, ULO address) {
  ULO sprnr;

  sprnr = (address>>3) & 7;
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

void spritesDecodeC(void) {
  ULO spriteno;
  UWO data1, data2;
  sprites_online = FALSE;
  spriteno = 0;
  while (spriteno < 8 && spriteno*4 < sprite_ddf_kill) {
    sprite_online[spriteno] = FALSE;
    sprite_16col[spriteno] = FALSE;
    if (sprite_state[spriteno] == 3) {  /* State 3, sprite disabled */
      spriteno++;
      continue; /* Next sprite */
    }
    if (sprite_state[spriteno] == 1) { /* State 1, waiting for first line in sprite */
      if (spry[spriteno] != graph_raster_y) {
	spriteno++;
	continue;  /* Not first line of sprite, check next sprite */
      }
      sprite_state[spriteno] = 2; /* Now read control words */      
    }
    /* Here we are in state 2, 0, 4 or 5, read two data words */
    data1 = fetw(sprpt[spriteno]);
    data2 = fetw(sprpt[spriteno] + 2);
    sprpt[spriteno] = ((sprpt[spriteno] + 4) & 0x1fffff);
//    if (graph_raster_y == sprly[spriteno])
  }
}

/*===========================================================================*/
/* Map IO registers into memory space                                        */
/*===========================================================================*/

void spriteIOHandlersInstall(void) {
  ULO i;

  memorySetIOWriteStub(0x120, wspr0pth);
  memorySetIOWriteStub(0x122, wspr0ptl);
  memorySetIOWriteStub(0x124, wspr1pth);
  memorySetIOWriteStub(0x126, wspr1ptl);
  memorySetIOWriteStub(0x128, wspr2pth);
  memorySetIOWriteStub(0x12a, wspr2ptl);
  memorySetIOWriteStub(0x12c, wspr3pth);
  memorySetIOWriteStub(0x12e, wspr3ptl);
  memorySetIOWriteStub(0x130, wspr4pth);
  memorySetIOWriteStub(0x132, wspr4ptl);
  memorySetIOWriteStub(0x134, wspr5pth);
  memorySetIOWriteStub(0x136, wspr5ptl);
  memorySetIOWriteStub(0x138, wspr6pth);
  memorySetIOWriteStub(0x13a, wspr6ptl);
  memorySetIOWriteStub(0x13c, wspr7pth);
  memorySetIOWriteStub(0x13e, wspr7ptl);
  for (i = 0; i < 8; i++) {
    memorySetIOWriteStub(0x140 + i*8, wsprxpos);
    memorySetIOWriteStub(0x142 + i*8, wsprxctl);
    memorySetIOWriteStub(0x144 + i*8, wsprxdata);
    memorySetIOWriteStub(0x146 + i*8, wsprxdatb);
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
