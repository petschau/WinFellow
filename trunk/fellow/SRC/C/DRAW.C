/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Draws an Amiga screen in a host display buffer                             */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include <math.h>

#include "portable.h"
#include "renaming.h"
#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "cpu.h"
#include "fmem.h"
#include "graph.h"
#include "gfxdrv.h"
#include "listtree.h"
#include "timer.h"
#include "fonts.h"
#include "copper.h"


/*============================================================================*/
/* Mode list, nodes created by the graphics driver, and pointer to the mode   */
/* that is current                                                            */
/*============================================================================*/

felist *draw_modes;
draw_mode *draw_mode_current;


/*============================================================================*/
/* Fps counter and LED control                                                */
/*============================================================================*/

BOOLE draw_fps_counter_enabled;
ULO draw_fps_counter_value;


/*============================================================================*/
/* These constants define the LED symbol appearance                           */
/*============================================================================*/

#define DRAW_LED_COUNT      5
#define DRAW_LED_WIDTH     12
#define DRAW_LED_HEIGHT     4
#define DRAW_LED_FIRST_X   16
#define DRAW_LED_FIRST_Y    4
#define DRAW_LED_GAP        8
#define DRAW_LED_COLOR_ON  0x00FF00 /* Green */
#define DRAW_LED_COLOR_OFF 0x000000 /* Black */

BOOLE draw_LEDs_enabled;
BOOLE draw_LEDs_state[DRAW_LED_COUNT];


/*============================================================================*/
/* Event flags that trigger some action regarding the host display            */
/* Special keypresses normally trigger these events                           */
/*============================================================================*/

ULO draw_view_scroll;                        /* Scroll visible window runtime */


/*============================================================================*/
/* Scale control, both horisontally and vertically the pixels can be scaled   */
/* by the factors given in the variables below                                */
/*============================================================================*/

ULO draw_hscale;                                          /* Horisontal scale */
ULO draw_vscale;                                       /* Base vertical scale */
BOOLE draw_scanlines;                                        /* Use scanlines */
BOOLE draw_deinterlace;                           /* Remove interlace flicker */


/*============================================================================*/
/* Data concerning the dimensions of the Amiga screen to show                 */
/*============================================================================*/

ULO draw_width_amiga;                /* Width of screen in Amiga lores pixels */
ULO draw_height_amiga;                    /* Height of screen in Amiga pixels */
ULO draw_width_amiga_real;            /* Width of Amiga screen in real pixels */
ULO draw_height_amiga_real;          /* Height of Amiga screen in real pixels */


/*============================================================================*/
/* Data concerning the positioning of the Amiga screen in the host buffer     */               
/*============================================================================*/

ULO draw_hoffset;            /* X-position of the first amiga pixel in a line */
ULO draw_voffset;                       /* Y-position of the first amiga line */


/*============================================================================*/
/* Bounding box of the Amiga screen that is visible in the host buffer        */
/*============================================================================*/

ULO draw_left;
ULO draw_right;
ULO draw_top;
ULO draw_bottom;


/*============================================================================*/
/* FPS image buffer                                                           */
/*============================================================================*/

BOOLE draw_fps_buffer[5][20];
ULO draw_fps_time_last;


/*============================================================================*/
/* Color table, 12 bit Amiga color to host display color                      */
/*============================================================================*/

ULO draw_color_table[4096]; /* Translate Amiga 12-bit color to whatever color */
			    /* format used by the graphics-driver.            */
			    /* In 8-bit mode, the color is mapped to a        */
                            /* registernumber.                                */
                            /* In truecolor mode, the color is mapped to      */
			    /* a corresponding true-color bitpattern          */
			    /* If the color does not match longword size, the */
			    /* color is repeated to fill the longword, that is*/
			    /* 4 repeated color register numbers in 8-bit mode*/
			    /* etc.                                           */
			    /* In 24-bit mode, the last byte is blank         */

/*============================================================================*/
/* Pointers to drawing routines that handle the drawing of Amiga graphics     */
/* on the current host screen mode                                            */
/*============================================================================*/

draw_line_func draw_line_routine;
draw_line_func draw_line_BG_routine;
draw_line_func draw_line_BPL_manage_routine;
draw_line_func draw_line_BPL_res_routine;
draw_line_func draw_line_lores_routine;
draw_line_func draw_line_hires_routine;
draw_line_func draw_line_dual_lores_routine;
draw_line_func draw_line_dual_hires_routine;
draw_line_func draw_line_HAM_lores_routine;


/*============================================================================*/
/* Lookup tables that holds all the drawing routines for various Amiga and    */
/* host screen modes                                                          */
/*============================================================================*/

draw_line_func draw_line_BPL_manage_funcs[2][8] = {drawLineBPL1x8,
				                   drawLineBPL2x8,
					           drawLineBPL1x16,
					           drawLineBPL2x16,
						   drawLineBPL1x24,
						   drawLineBPL2x24,
						   drawLineBPL1x32,
						   drawLineBPL2x32,
						   drawLineBPL1x8,
				                   drawLineBPL2x8,
					           drawLineBPL1x16,
					           drawLineBPL2x16mmx,
						   drawLineBPL1x24,
						   drawLineBPL2x24,
						   drawLineBPL1x32,
						   drawLineBPL2x32};

draw_line_func draw_line_BG_funcs[2][8] = {drawLineBG1x8,
				           drawLineBG2x8,
					   drawLineBG1x16,
					   drawLineBG2x16,
					   drawLineBG1x24,
					   drawLineBG2x24,
					   drawLineBG1x32,
					   drawLineBG2x32,
					   drawLineBG1x8,
					   drawLineBG2x8,
					   drawLineBG1x16,
					   drawLineBG2x16mmx,
					   drawLineBG1x24,
					   drawLineBG2x24,
					   drawLineBG1x32,
					   drawLineBG2x32};

draw_line_func draw_line_lores_funcs[2][8] = {drawLineNormal1x8,
					      drawLineNormal2x8,
					      drawLineNormal1x16,
					      drawLineNormal2x16,
					      drawLineNormal1x24,
					      drawLineNormal2x24,
					      drawLineNormal1x32,
					      drawLineNormal2x32,
					      drawLineNormal1x8,
					      drawLineNormal2x8,
					      drawLineNormal1x16mmx,
					      drawLineNormal2x16mmx,
					      drawLineNormal1x24,
					      drawLineNormal2x24,
					      drawLineNormal1x32,
					      drawLineNormal2x32};

draw_line_func draw_line_hires_funcs[2][8] = {drawLineNormal1x8,
					      drawLineNormal1x8,
					      drawLineNormal1x16,
					      drawLineNormal1x16,
					      drawLineNormal1x24,
					      drawLineNormal1x24,
					      drawLineNormal1x32,
					      drawLineNormal1x32,
					      drawLineNormal1x8,
					      drawLineNormal1x8,
					      drawLineNormal1x16mmx,
					      drawLineNormal1x16mmx,
					      drawLineNormal1x24,
					      drawLineNormal1x24,
					      drawLineNormal1x32,
					      drawLineNormal1x32};

draw_line_func draw_line_dual_lores_funcs[2][8] = {drawLineDual1x8,
						   drawLineDual2x8,
						   drawLineDual1x16,
						   drawLineDual2x16,
						   drawLineDual1x24,
						   drawLineDual2x24,
						   drawLineDual1x32,
						   drawLineDual2x32,
						   drawLineDual1x8,
						   drawLineDual2x8,
						   drawLineDual1x16,
						   drawLineDual2x16,
						   drawLineDual1x24,
						   drawLineDual2x24,
						   drawLineDual1x32,
						   drawLineDual2x32};

draw_line_func draw_line_dual_hires_funcs[2][8] = {drawLineDual1x8,
						   drawLineDual1x8,
						   drawLineDual1x16,
						   drawLineDual1x16,
						   drawLineDual1x24,
						   drawLineDual1x24,
						   drawLineDual1x32,
						   drawLineDual1x32,
						   drawLineDual1x8,
						   drawLineDual1x8,
						   drawLineDual1x16,
						   drawLineDual1x16,
						   drawLineDual1x24,
						   drawLineDual1x24,
						   drawLineDual1x32,
						   drawLineDual1x32};

draw_line_func draw_line_HAM_lores_funcs[2][8] = {drawLineHAM1x8,
					          drawLineHAM2x8,
						  drawLineHAM1x16,
						  drawLineHAM2x16,
						  drawLineHAM1x24,
						  drawLineHAM2x24,
						  drawLineHAM1x32,
						  drawLineHAM2x32,
						  drawLineHAM1x8,
						  drawLineHAM2x8,
						  drawLineHAM1x16,
						  drawLineHAM2x16,
						  drawLineHAM1x24,
						  drawLineHAM2x24,
						  drawLineHAM1x32,
						  drawLineHAM2x32};


/*============================================================================*/
/* Framebuffer information                                                    */
/*============================================================================*/

UBY *draw_buffer_top_ptr;    /* Pointer to the top of the current host buffer */
UBY *draw_buffer_current_ptr;            /* Pointer to the next pixel to draw */
ULO draw_buffer_show;                 /* Number of the currently shown buffer */
ULO draw_buffer_draw;                 /* Number of the current drawing buffer */
ULO draw_buffer_count;                    /* Number of available framebuffers */
ULO draw_frame_count;                /* Counts frames, both skipped and drawn */
ULO draw_frame_skip_factor;            /* Frame-skip factor, 1 / (factor + 1) */
ULO draw_frame_skip;                            /* Running frame-skip counter */
ULO draw_switch_bg_to_bpl;       /* Flag TRUE if on current line, switch from */
                                         /* background color to bitplane data */

/*============================================================================*/
/* Dual playfield translation table                                           */
/* Syntax: dualtranslate[0 - PF1 behind, 1 - PF2 behind][PF1data][PF2data]    */
/*============================================================================*/

UBY draw_dual_translate[2][256][256];


/*============================================================================*/
/* HAM color modify helper table                                              */
/*============================================================================*/

ULO draw_HAM_modify_table[4][2];


/*============================================================================*/
/* 8-bit color mapping table                                                  */
/*============================================================================*/

ULO draw_8bit_to_color[256];


/*============================================================================*/
/* Initializes the dual playfield translation table                           */
/*============================================================================*/

static void drawDualTranslationInitialize(void) {
  LON i,j,k,l;

  for (k = 0; k < 2; k++) {
    for (i = 0; i < 256; i++) {
      for (j = 0; j < 256; j++) {
        if (k == 0) {                            /* PF1 behind, PF2 in front */
          if (j == 0) l = i;                 /* PF2 transparent, PF1 visible */
          else {                                              /* PF2 visible */
                              /* If color is higher than 0x3c it is a sprite */
            if (j < 0x40) l = j + 0x20;
            else l = j;
            }
          }
        else {                                   /* PF1 in front, PF2 behind */
          if (i == 0) {                      /* PF1 transparent, PF2 visible */
            if (j == 0) l = 0;
            else {
              if (j < 0x40) l = j + 0x20;
              else l = j;
              }
            }
          else l = i;                     /* PF1 visible amd not transparent */
          }
        draw_dual_translate[k][i][j] = l;
        }
      }
    }
}


/*============================================================================*/
/* Centering changed                                                          */
/*============================================================================*/

static void drawViewScroll(void) {
  if (draw_view_scroll == 80) {
    if (draw_top > 0x1a) {
      draw_top--;
      draw_bottom--;
      }
    }
  else if (draw_view_scroll == 72) {
    if (draw_bottom < 0x139) {
      draw_top++;
      draw_bottom++;
      }
    }
  else if (draw_view_scroll == 75) {
    if (draw_right < 472) {
      draw_right++;
      draw_left++;
      }
    }
  else if (draw_view_scroll == 77) {
    if (draw_left > 88) {
      draw_right--;
      draw_left--;
      }
    }
  draw_view_scroll = 0;
}


/*============================================================================*/
/* Draws a LED symbol                                                         */
/*============================================================================*/

static void drawLED8(ULO x, ULO y, ULO width, ULO height, ULO color) {
  UBY *bufb;
  ULO x1, y1;
  UBY color8 = draw_color_table[((color & 0xf00000) >> 12) |
                                ((color & 0x00f000) >> 8) |
                                ((color & 0x0000f0) >> 4)];
  ULO pitch = drawValidateBufferPointer(0);
  if (pitch == 0) return;
  bufb = draw_buffer_top_ptr + pitch*y + x;
  for (y1 = 0; y1 < height; y1++) {
    for (x1 = 0; x1 < width; x1++) *(bufb + x1) = color8;
    bufb = bufb + pitch;
  }
  drawInvalidateBufferPointer();
}

static void drawLED16(ULO x, ULO y, ULO width, ULO height, ULO color) {
  UWO *bufw;
  ULO x1, y1;
  UWO color16 = draw_color_table[((color & 0xf00000) >> 12) |
                                 ((color & 0x00f000) >> 8) |
                                 ((color & 0x0000f0) >> 4)];
  ULO pitch = drawValidateBufferPointer(0);
  if (pitch == 0) return;
  bufw = ((UWO *) (draw_buffer_top_ptr + pitch*y)) + x;
  for (y1 = 0; y1 < height; y1++) {
    for (x1 = 0; x1 < width; x1++) *(bufw + x1) = color16;
    bufw = (UWO *) (((UBY *) bufw) + pitch);
  }
  drawInvalidateBufferPointer();
}


static void drawLED24(ULO x, ULO y, ULO width, ULO height, ULO color) {
  UBY *bufb;
  ULO x1, y1;
  ULO color24 = draw_color_table[((color & 0xf00000) >> 12) |
                                 ((color & 0x00f000) >> 8) |
                                 ((color & 0x0000f0) >> 4)];
  UBY color24_1 = (color24 & 0xff0000) >> 16;
  UBY color24_2 = (color24 & 0x00ff00) >> 8;
  UBY color24_3 = (color24 & 0x0000ff);
  ULO pitch = drawValidateBufferPointer(0);
  if (pitch == 0) return;
  bufb = draw_buffer_top_ptr + pitch*y + x*3;
  for (y1 = 0; y1 < height; y1++) {
    for (x1 = 0; x1 < width; x1++) {
      *(bufb + x1*3) = color24_1;
      *(bufb + x1*3 + 1) = color24_2;
      *(bufb + x1*3 + 2) = color24_3;
    }
    bufb = bufb + pitch;
  }
  drawInvalidateBufferPointer();
}

static void drawLED32(ULO x, ULO y, ULO width, ULO height, ULO color) {
  ULO *bufl;
  ULO x1, y1;
  ULO color32 = draw_color_table[((color & 0xf00000) >> 12) |
                                 ((color & 0x00f000) >> 8) |
                                 ((color & 0x0000f0) >> 4)];
  ULO pitch = drawValidateBufferPointer(0);
  if (pitch == 0) return;
  bufl = ((ULO *) (draw_buffer_top_ptr + pitch*y)) + x;
  for (y1 = 0; y1 < height; y1++) {
    for (x1 = 0; x1 < width; x1++) *(bufl + x1) = color32;
    bufl = (ULO *) (((UBY *) bufl) + pitch);
  }
  drawInvalidateBufferPointer();
}

static void drawLED(ULO index, BOOLE state) {
  ULO x = DRAW_LED_FIRST_X + (DRAW_LED_WIDTH + DRAW_LED_GAP)*index;
  ULO y = DRAW_LED_FIRST_Y;
  ULO color = (state) ? DRAW_LED_COLOR_ON : DRAW_LED_COLOR_OFF;
  
  switch (draw_mode_current->bits) {
    case 8:
      drawLED8(x, y, DRAW_LED_WIDTH, DRAW_LED_HEIGHT, color);
      break;
    case 16:
      drawLED16(x, y, DRAW_LED_WIDTH, DRAW_LED_HEIGHT, color);
      break;
    case 24:
      drawLED24(x, y, DRAW_LED_WIDTH, DRAW_LED_HEIGHT, color);
      break;
    case 32:
      drawLED32(x, y, DRAW_LED_WIDTH, DRAW_LED_HEIGHT, color);
      break;
    default:
      break;
  }
}


static void drawLEDs(void) {
  if (draw_LEDs_enabled) {
    ULO i;
    for (i = 0; i < DRAW_LED_COUNT; i++) drawLED(i, draw_LEDs_state[i]);
  }
}

  
/*============================================================================*/
/* Draws one char in the FPS counter buffer                                   */
/*============================================================================*/

static void drawFpsChar(ULO character, ULO x) {
  ULO i, j;

  for (i = 0; i < 5; i++) 
    for (j = 0; j < 4; j++)
      draw_fps_buffer[i][x*4 + j] = draw_fps_font[character][i][j]; 
}


/*============================================================================*/
/* Draws text in the FPS counter buffer                                       */
/*============================================================================*/

static void drawFpsText(STR *text) {
  ULO i, c, fg;

  for (i = 0; i < 4; i++) {
    c = *text++;
    switch (c) {
      case '0': drawFpsChar(9, i);
                break;
      case '1': drawFpsChar(0, i);
                break;
      case '2': drawFpsChar(1, i);
                break;
      case '3': drawFpsChar(2, i);
                break;
      case '4': drawFpsChar(3, i);
                break;
      case '5': drawFpsChar(4, i);
                break;
      case '6': drawFpsChar(5, i);
                break;
      case '7': drawFpsChar(6, i);
                break;
      case '8': drawFpsChar(7, i);
                break;
      case '9': drawFpsChar(8, i);
                break;
      case '%': drawFpsChar(10, i);
                break;
      case ' ':
      default:  drawFpsChar(11, i);
                break;
    }
  }
}


/*============================================================================*/
/* Copy FPS buffer to a 8 bit screen                                          */
/*============================================================================*/

static void drawFpsToFramebuffer8(void) {
  UBY *bufb;
  ULO x, y;

  if (!drawValidateBufferPointer(0)) return;
  bufb = draw_buffer_top_ptr + draw_mode_current->width - 20;
  for (y = 0; y < 5; y++) {
    for (x = 0; x < 20; x++)
      *(bufb + x) = draw_fps_buffer[y][x] ? 252 : 0;
    bufb = bufb + draw_mode_current->pitch;
  }
  drawInvalidateBufferPointer();
}


/*============================================================================*/
/* Copy FPS buffer to a 16 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer16(void) {
  UWO *bufw;
  ULO x, y;
  if (!drawValidateBufferPointer(0)) return;
  bufw = ((UWO *) draw_buffer_top_ptr) + draw_mode_current->width - 20;
  for (y = 0; y < 5; y++) {
    for (x = 0; x < 20; x++)
      *(bufw + x) = draw_fps_buffer[y][x] ? 0xffff : 0;
    bufw = (UWO *) (((UBY *) bufw) + draw_mode_current->pitch);
  }
  drawInvalidateBufferPointer();
}


/*============================================================================*/
/* Copy FPS buffer to a 24 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer24(void) {
  UBY *bufb;
  ULO x, y;
  if (!drawValidateBufferPointer(0)) return;
  bufb = draw_buffer_top_ptr;
  bufb += (draw_mode_current->width - 20)*3;
  for (y = 0; y < 5; y++) {
    for (x = 0; x < 60; x += 3) {
      UBY color = draw_fps_buffer[y][x] ? 0xff : 0;
      *(bufb + x) = color;
      *(bufb + x + 1) = color;
      *(bufb + x + 2) = color;
    }
    bufb += draw_mode_current->pitch;
  }
  drawInvalidateBufferPointer();
}


/*============================================================================*/
/* Copy FPS buffer to a 32 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer32(void) {
  ULO *bufl;
  ULO x, y;
  if (!drawValidateBufferPointer(0)) return;
  bufl = ((ULO *) draw_buffer_top_ptr) + draw_mode_current->width - 20;
  for (y = 0; y < 5; y++) {
    for (x = 0; x < 20; x++)
      *(bufl + x) = draw_fps_buffer[y][x] ? 0xffffffff : 0;
    bufl = (ULO *) (((UBY *) bufl) + draw_mode_current->pitch);
  }
  drawInvalidateBufferPointer();
}


/*============================================================================*/
/* Draws FPS counter in current framebuffer                                   */
/*============================================================================*/

static void drawFpsCounter(void) {
  if (draw_fps_counter_enabled) {
    ULO fps;
    ULO fps_time;
    STR s[16];

    fps_time = timerTimeGet();
    fps = (fps_time - draw_fps_time_last);
    draw_fps_time_last = fps_time;
    sprintf(s, "%d", fps);
    drawFpsText(s);
    switch (draw_mode_current->bits) {
      case 8:
        drawFpsToFramebuffer8();
        break;
      case 16:
        drawFpsToFramebuffer16();
        break;
      case 24:
        drawFpsToFramebuffer24();
        break;
      case 32:
        drawFpsToFramebuffer32();
        break;
      default:
        break;
    }
  }
}


/*============================================================================*/
/* Add one mode to the list of useable modes                                  */
/*============================================================================*/

void drawModeAdd(draw_mode *modenode) {
  draw_modes = listAddLast(draw_modes, listNew(modenode));
}


/*============================================================================*/
/* Free mode list                                                             */
/*============================================================================*/

static void drawModesFree(void) {
  listFreeAll(draw_modes, TRUE);
  draw_modes = NULL;
}


/*============================================================================*/
/* Clear mode list (on startup)                                               */
/*============================================================================*/

static void drawModesClear(void) {
  draw_modes = NULL;
  draw_mode_current = NULL;
}

  
/*============================================================================*/
/* Dump mode list to log                                                      */
/*============================================================================*/

static void drawModesDump(void) {
  felist *tmp = draw_modes;
  fellowAddLog("Draw module mode list:\n");
  while (tmp != NULL) {
    fellowAddLog(((draw_mode *) listNode(tmp))->name);
    fellowAddLog("\n");
    tmp = listNext(tmp);
  }
}


/*============================================================================*/
/* Set the palette entries in the gfx driver                                  */
/*============================================================================*/

static void drawPaletteInitialize(void) {
  ULO r, g, b;

    for (r = 0; r < 6; r++) 
      for (g = 0; g < 6; g++)
	for (b = 0; b < 6; b++)
	  gfxDrvSet8BitColor(r + g*6 + b*36 + 10, r*51, g*51, b*51);
}


/*============================================================================*/
/* Color translation initialization                                           */
/*============================================================================*/

static void drawColorTranslationInitialize(void) {
  ULO k, r, g, b;

  /* Create color translation table */

  if (draw_mode_current->bits == 8) {

    /* Use 6 levels of red and blue, 7 levels of green */
    
    for (k = 0; k < 4096; k++) {
      b = ((((k & 0xf)<<4)*(16.0/15.0)) + 25) / 51;
      g = (((k & 0xf0)*(16.0/15.0)) + 25) / 51;
      r = ((((k & 0xf00)>>4)*(16.0/15.0)) + 25) / 51;
      draw_color_table[k] = r + g*6 + b*36 + 10;
      draw_8bit_to_color[draw_color_table[k]] = k | (k<<16);
      draw_color_table[k] = draw_color_table[k]<<24 |
			    draw_color_table[k]<<16 |
			    draw_color_table[k]<<8 |
			    draw_color_table[k];
    }
  }
  else {
    for (k = 0; k < 4096; k++) {
      r = ((k & 0xf00) >> 8) << (draw_mode_current->redpos + draw_mode_current->redsize - 4);
      g = ((k & 0xf0) >> 4) << (draw_mode_current->greenpos + draw_mode_current->greensize - 4);
      b = (k & 0xf) << (draw_mode_current->bluepos + draw_mode_current->bluesize - 4);
      draw_color_table[k] = r | g | b;
      if (draw_mode_current->bits <= 16)
	draw_color_table[k] = draw_color_table[k]<<16 | draw_color_table[k];
    }
  }
}


/*============================================================================*/
/* Calculate the width of the screen in Amiga lores pixels                    */
/*============================================================================*/

static void drawAmigaScreenWidth(draw_mode *dm) {
  ULO totalscale;

  totalscale = draw_hscale; /* Base scale */
  draw_width_amiga = dm->width/totalscale;
  if (draw_width_amiga > 384)
    draw_width_amiga = 384;
  if (draw_width_amiga <= 343) {
    draw_left = 129;
    draw_right = 129 + draw_width_amiga;
  }
  else {
    draw_right = 472;
    draw_left = 472 - draw_width_amiga;
  }
  draw_width_amiga_real = draw_width_amiga*totalscale;
}


/*============================================================================*/
/* Calculate the height of the screen in Amiga lores pixels                   */
/*============================================================================*/

static void drawAmigaScreenHeight(draw_mode *dm) {
  ULO totalscale;

  totalscale = draw_vscale; /* Base scale */
  if (drawGetScanlines())
    totalscale *= 2;        /* Scanlines */
  if (drawGetDeinterlace())
    totalscale *= 2;        /* Interlace compensation */
  draw_height_amiga = dm->height/totalscale;
  if (draw_height_amiga > 287)
    draw_height_amiga = 287;
  if (draw_height_amiga <= 269) {
    draw_top = 44;
    draw_bottom = 44 + draw_height_amiga;
  }
  else {
    draw_bottom = 313;
    draw_top = 313 - draw_height_amiga;
  }
  draw_height_amiga_real = draw_height_amiga*totalscale;
}


/*============================================================================*/
/* Center the Amiga screen                                                    */
/*============================================================================*/

static void drawAmigaScreenCenter(draw_mode *dm) {
  draw_hoffset = ((dm->width - draw_width_amiga_real)/2) & (~7);
  draw_voffset = (dm->height - draw_height_amiga_real)/2;
}


/*============================================================================*/
/* Calulate needed geometry information about Amiga screen                    */
/*============================================================================*/

static void drawAmigaScreenGeometry(draw_mode *dm) {
  drawAmigaScreenWidth(dm);
  drawAmigaScreenHeight(dm);
  drawAmigaScreenCenter(dm);
}


/*============================================================================*/
/* Calulate data needed to draw HAM                                           */
/*============================================================================*/

static ULO drawMakeHoldMask(ULO pos, ULO size, BOOLE longdestination) {
  ULO i, holdmask;

  holdmask = 0;
  for (i = pos; i < (pos + size); i++)
    holdmask |= (1<<i);
  if (!longdestination)
    return ((~holdmask) & 0xffff) | ((~holdmask)<<16);
  return (~holdmask);
}

static void drawHAMTableInit(draw_mode *dm) {
  draw_HAM_modify_table[0][0] = 0;
  draw_HAM_modify_table[0][1] = 0;
  if (dm->bits == 8) {
    draw_HAM_modify_table[1][0] = 0;				      /* Blue */
    draw_HAM_modify_table[1][1] = drawMakeHoldMask(0, 4, FALSE);
    draw_HAM_modify_table[2][0] = 8;				       /* Red */
    draw_HAM_modify_table[2][1] = drawMakeHoldMask(8, 4, FALSE);
    draw_HAM_modify_table[3][0] = 4;				     /* Green */
    draw_HAM_modify_table[3][1] = drawMakeHoldMask(4, 4, FALSE);  
  }
  else {
    BOOLE longdestination = (dm->bits > 16);
    draw_HAM_modify_table[1][0] = dm->bluepos + dm->bluesize - 4;     /* Blue */
    draw_HAM_modify_table[1][1] = drawMakeHoldMask(dm->bluepos,
						   dm->bluesize,
						   longdestination);
    draw_HAM_modify_table[2][0] = dm->redpos + dm->redsize - 4;        /* Red */
    draw_HAM_modify_table[2][1] = drawMakeHoldMask(dm->redpos,
						   dm->redsize,
						   longdestination);
    draw_HAM_modify_table[3][0] = dm->greenpos + dm->greensize - 4;  /* Green */
    draw_HAM_modify_table[3][1] = drawMakeHoldMask(dm->greenpos,
						   dm->greensize,
						   longdestination);
  }
}


/*============================================================================*/
/* Selects drawing routines for the current mode                              */
/*============================================================================*/

static void drawModeTablesInitialize(draw_mode *dm) {
  ULO i, funcindex;
  
  draw_buffer_draw = draw_buffer_show = 0;
  for (i = 0x180; i < 0x1c0; i += 2) memorySetIOWriteStub(i, wcolor);
  funcindex = (draw_hscale == 1) ? 0 : 1;
  if (dm->bits == 32) funcindex += 6;
  else if (dm->bits == 24) funcindex += 4;
  else if (dm->bits >= 15) funcindex += 2;
  draw_line_BPL_manage_routine = draw_line_BPL_manage_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_routine = draw_line_BG_routine = draw_line_BG_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_BPL_res_routine = draw_line_lores_routine = draw_line_lores_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_hires_routine = draw_line_hires_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_dual_lores_routine = draw_line_dual_lores_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_dual_hires_routine = draw_line_dual_hires_funcs[fellowGetMMXDetected()][funcindex];
  draw_line_HAM_lores_routine = draw_line_HAM_lores_funcs[fellowGetMMXDetected()][funcindex];
  if (draw_hscale == 1) graphP2C1XInit();
  else graphP2C2XInit();
  wriw(bplcon0,0xdff100);
  drawAmigaScreenGeometry(draw_mode_current);
  drawColorTranslationInitialize();
  graphInitializeShadowColors();
  draw_buffer_current_ptr = draw_buffer_top_ptr;
  copperYTableInit();
  drawHAMTableInit(draw_mode_current);
}


/*============================================================================*/
/* This routine flips to another drawing buffer, display the next complete    */
/*============================================================================*/

static void drawBufferFlip(void) {
  if (++draw_buffer_show >= draw_buffer_count)
    draw_buffer_show = 0;
  if (++draw_buffer_draw >= draw_buffer_count)
    draw_buffer_draw = 0;
  gfxDrvBufferFlip();
}


/*============================================================================*/
/* Draw module properties                                                     */
/*============================================================================*/

BOOLE drawSetMode(ULO width,
		  ULO height, 
		  ULO colorbits, 
		  ULO refresh,
		  BOOLE windowed)
{
  felist *l;
  for (l = draw_modes; l != NULL; l = listNext(l)) {
    draw_mode *dm = (draw_mode*) listNode(l);
    if ((dm->width == width) &&
        (dm->height == height) &&
	(windowed || (dm->bits == colorbits)) &&
	(dm->refresh == refresh) &&
	(dm->windowed == windowed)) {
      draw_mode_current = dm;
      return TRUE;
    }
  }
  draw_mode_current = (draw_mode*) listNode(draw_modes);
  return FALSE;
}

felist *drawGetModes(void) {
  return draw_modes;
}

void drawSetDeinterlace(BOOLE deinterlace) {
  draw_deinterlace = deinterlace;
}

BOOLE drawGetDeinterlace(void) {
  return draw_deinterlace;
}

void drawSetHorisontalScale(ULO horisontalscale) {
  draw_hscale = horisontalscale;
}

void drawSetVerticalScale(ULO verticalscale) {
  draw_vscale = verticalscale;
}

ULO drawGetVerticalScale(void) {
  return draw_vscale;
}

void drawSetScanlines(BOOLE scanlines) {
  draw_scanlines = scanlines;
}

BOOLE drawGetScanlines(void) {
  return draw_scanlines;
}

void drawSetFrameskipRatio(ULO frameskipratio) {
  draw_frame_skip_factor = frameskipratio;
}

void drawSetFPSCounterEnabled(BOOLE enabled) {
  draw_fps_counter_enabled = enabled;
}

void drawSetFPSCounter(ULO value) {
  draw_fps_counter_value = value;
}

void drawSetLEDsEnabled(BOOLE enabled) {
  draw_LEDs_enabled = enabled;
}

void drawSetLED(ULO index, BOOLE state) {
  if (index < DRAW_LED_COUNT) draw_LEDs_state[index] = state;
}


/*============================================================================*/
/* Validates the buffer pointer                                               */
/* Returns the width in bytes of one line                                     */
/* Returns zero if there is an error                                          */
/*============================================================================*/

ULO drawValidateBufferPointer(ULO amiga_line_number) {
  ULO scale;

  draw_buffer_top_ptr = gfxDrvValidateBufferPointer();
  if (draw_buffer_top_ptr == NULL) {
    fellowAddLog("Buffer ptr is NULL\n");
    return 0;
  }
  scale = 1;  /* Soild scaling is handled by the graphics driver */
  if (drawGetScanlines()) scale *= 2;
  if (drawGetDeinterlace()) scale *= 2;

  draw_buffer_current_ptr = draw_buffer_top_ptr +
                            (draw_mode_current->pitch * 
			     scale *
			     (amiga_line_number - draw_top)) +
			    (draw_mode_current->pitch*draw_voffset) +
			    (draw_hoffset*(draw_mode_current->bits >> 3));
  if (drawGetDeinterlace() && !lof) draw_buffer_current_ptr += (scale/2)*draw_mode_current->pitch;
  return draw_mode_current->pitch*scale;
}


/*============================================================================*/
/* Invalidates the buffer pointer                                             */
/*============================================================================*/

void drawInvalidateBufferPointer(void) {
  gfxDrvInvalidateBufferPointer();
}


/*============================================================================*/
/* Called on every end of frame                                               */
/*============================================================================*/

void drawHardReset(void) {
  draw_switch_bg_to_bpl = FALSE;
}


/*============================================================================*/
/* Called on every end of frame                                               */
/*============================================================================*/

void drawEndOfFrameC(void) {
  drawLEDs();
  drawFpsCounter();
  drawViewScroll();
  drawBufferFlip();
}


/*============================================================================*/
/* Called on emulation start                                                  */
/*============================================================================*/

void drawEmulationStart(void) {
  draw_switch_bg_to_bpl = FALSE;
  draw_frame_skip = 0;
  gfxDrvSetMode(draw_mode_current, drawGetVerticalScale());
  /* Use 1 buffer when deinterlacing, else 3 */
  gfxDrvEmulationStart((drawGetDeinterlace()) ? 1 : 3);
  draw_fps_time_last = timerTimeGet();
}

BOOLE drawEmulationStartPost(void) {
  BOOLE result;
  
  draw_buffer_count = gfxDrvEmulationStartPost();
  if ((result = (draw_buffer_count != 0))) {
    drawModeTablesInitialize(draw_mode_current);
    draw_buffer_show = 0;
    draw_buffer_draw = draw_buffer_count - 1;
  }
  return result;
}


/*============================================================================*/
/* Called on emulation halt                                                   */
/*============================================================================*/

void drawEmulationStop(void) {
  gfxDrvEmulationStop();
}


/*============================================================================*/
/* Things initialized once at startup                                         */
/*============================================================================*/

BOOLE drawStartup(void) {
  ULO i;

  drawModesClear();
  if (!gfxDrvStartup()) return FALSE;
  draw_mode_current = (draw_mode *) listNode(draw_modes);
  drawDualTranslationInitialize();
  drawPaletteInitialize();
  draw_left = 120;
  draw_right = 440;
  draw_top = 0x45;
  draw_bottom = 0x10d;
  draw_view_scroll = 0;
  draw_switch_bg_to_bpl = FALSE;
  draw_frame_count = 0;
  drawSetDeinterlace(FALSE);
  drawSetHorisontalScale(1);
  drawSetVerticalScale(1);
  drawSetScanlines(FALSE);
  drawSetFrameskipRatio(1);
  drawSetFPSCounterEnabled(FALSE);
  drawSetFPSCounter(0);
  drawSetLEDsEnabled(FALSE);
  for (i = 0; i < DRAW_LED_COUNT; i++) drawSetLED(i, FALSE);
  return TRUE;
}


/*============================================================================*/
/* Things uninitialized at emulator shutdown                                  */
/*============================================================================*/

void drawShutdown(void) {
  drawModesFree();
  gfxDrvShutdown();
}

