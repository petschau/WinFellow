/*=========================================================================*/
/* Fellow                                                                  */
/* Draws an Amiga screen in a host display buffer                          */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Worfje                                                         */
/*                                                                         */
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
#include <math.h>

#include "defs.h"
#include "fellow.h"
#include "bus.h"
#include "draw.h"
#include "fmem.h"
#include "graph.h"
#include "gfxdrv.h"
#include "listtree.h"
#include "timer.h"
#include "fonts.h"
#include "copper.h"
#include "wgui.h"
#include "fileops.h"
#include "sprite.h"
#include "CONFIG.H"

#include "draw_pixelrenderers.h"
#include "draw_interlace_control.h"

#ifdef GRAPH2
#include "Graphics.h"
#endif

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

cfg *draw_config;

/*============================================================================*/
/* Mode list, nodes created by the graphics driver, and pointer to the mode   */
/* that is current                                                            */
/*============================================================================*/

felist *draw_modes;
draw_mode *draw_mode_current;


/*============================================================================*/
/* Event flags that trigger some action regarding the host display            */
/* Special keypresses normally trigger these events                           */
/*============================================================================*/

ULO draw_view_scroll;                        /* Scroll visible window runtime */


/*============================================================================*/
/* Host display rendering properties                                          */
/*============================================================================*/


DISPLAYSCALE draw_displayscale;
DISPLAYSCALE_STRATEGY draw_displayscale_strategy;
DISPLAYDRIVER draw_displaydriver;

BOOLE draw_allow_multiple_buffers;          /* allows the use of more buffers */
ULO draw_clear_buffers;

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
/* Color table, 12 bit Amiga color to host display color                      */
/*============================================================================*/

ULO draw_color_table[4096]; /* Translate Amiga 12-bit color to whatever color */
/* format used by the graphics-driver.            */
/* In truecolor mode, the color is mapped to      */
/* a corresponding true-color bitpattern          */
/* If the color does not match longword size, the */
/* color is repeated to fill the longword, that is*/
/* 2 repeated color codes 16-bit mode             */
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
/* Framebuffer information                                                    */
/*============================================================================*/

UBY *draw_buffer_top_ptr;    /* Pointer to the top of the current host buffer */
UBY *draw_buffer_current_ptr;            /* Pointer to the next pixel to draw */
ULO draw_buffer_show;                 /* Number of the currently shown buffer */
ULO draw_buffer_draw;                 /* Number of the current drawing buffer */
ULO draw_buffer_count;                    /* Number of available framebuffers */
ULO draw_frame_count;                /* Counts frames, both skipped and drawn */
ULO draw_frame_skip_factor;            /* Frame-skip factor, 1 / (factor + 1) */
LON draw_frame_skip;                            /* Running frame-skip counter */
ULO draw_switch_bg_to_bpl;       /* Flag TRUE if on current line, switch from */
/* background color to bitplane data */

/*============================================================================*/
/* Centering changed                                                          */
/*============================================================================*/

static void drawViewScroll(void)
{
  if (draw_view_scroll == 80)
  {
    if (draw_top > 0x1a)
    {
      draw_top--;
      draw_bottom--;
    }
  }
  else if (draw_view_scroll == 72)
  {
    if (draw_bottom < 0x139)
    {
      draw_top++;
      draw_bottom++;
    }
  }
  else if (draw_view_scroll == 75)
  {
    if (draw_right < 472)
    {
      draw_right++;
      draw_left++;
    }
  }
  else if (draw_view_scroll == 77)
  {
    if (draw_left > 88)
    {
      draw_right--;
      draw_left--;
    }
  }
  draw_view_scroll = 0;
}

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

bool draw_LEDs_enabled;
bool draw_LEDs_state[DRAW_LED_COUNT];

/*============================================================================*/
/* Draws a LED symbol                                                         */
/*============================================================================*/

static void drawLED16(int x, int y, int width, int height, ULO color)
{
  UWO *bufw = ((UWO *) (draw_buffer_top_ptr + draw_mode_current->pitch*y)) + x;
  UWO color16 = (UWO) draw_color_table[((color & 0xf00000) >> 12) |
		      ((color & 0x00f000) >> 8) |
		      ((color & 0x0000f0) >> 4)];
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufw + x1) = color16;
    }
    bufw = (UWO *) (((UBY *) bufw) + draw_mode_current->pitch);
  }
}


static void drawLED24(int x, int y, int width, int height, ULO color)
{
  UBY *bufb = draw_buffer_top_ptr + draw_mode_current->pitch*y + x*3;
  ULO color24 = draw_color_table[((color & 0xf00000) >> 12) |
		((color & 0x00f000) >> 8) |
		((color & 0x0000f0) >> 4)];
  UBY color24_1 = (UBY) ((color24 & 0xff0000) >> 16);
  UBY color24_2 = (UBY) ((color24 & 0x00ff00) >> 8);
  UBY color24_3 = (UBY) (color24 & 0x0000ff);
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufb + x1*3) = color24_1;
      *(bufb + x1*3 + 1) = color24_2;
      *(bufb + x1*3 + 2) = color24_3;
    }
    bufb = bufb + draw_mode_current->pitch;
  }
}

static void drawLED32(int x, int y, int width, int height, ULO color)
{
  ULO *bufl = ((ULO *) (draw_buffer_top_ptr + draw_mode_current->pitch*y)) + x;
  ULO color32 = draw_color_table[((color & 0xf00000) >> 12) |
		((color & 0x00f000) >> 8) |
		((color & 0x0000f0) >> 4)];
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufl + x1) = color32;
    }
    bufl = (ULO *) (((UBY *) bufl) + draw_mode_current->pitch);
  }
}

static void drawLED(int index, bool state)
{
  int x = DRAW_LED_FIRST_X + (DRAW_LED_WIDTH + DRAW_LED_GAP)*index;
  int y = DRAW_LED_FIRST_Y;
  ULO color = (state) ? DRAW_LED_COLOR_ON : DRAW_LED_COLOR_OFF;
  int height = DRAW_LED_HEIGHT;

  switch (draw_mode_current->bits)
  {
    case 16:
      drawLED16(x, y, DRAW_LED_WIDTH, height, color);
      break;
    case 24:
      drawLED24(x, y, DRAW_LED_WIDTH, height, color);
      break;
    case 32:
      drawLED32(x, y, DRAW_LED_WIDTH, height, color);
      break;
    default:
      break;
  }
}

static void drawLEDs(void)
{
  if (draw_LEDs_enabled)
  {
    for (int i = 0; i < DRAW_LED_COUNT; i++)
    {
      drawLED(i, draw_LEDs_state[i]);
    }
  }
}


/*============================================================================*/
/* FPS image buffer                                                           */
/*============================================================================*/

bool draw_fps_counter_enabled;
bool draw_fps_buffer[5][20];


/*============================================================================*/
/* Draws one char in the FPS counter buffer                                   */
/*============================================================================*/

static void drawFpsChar(int character, int x)
{
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      draw_fps_buffer[i][x*4 + j] = draw_fps_font[character][i][j];
    }
  }
}


/*============================================================================*/
/* Draws text in the FPS counter buffer                                       */
/*============================================================================*/

static void drawFpsText(STR *text)
{
  for (int i = 0; i < 4; i++)
  {
    STR c = *text++;
    switch (c)
    {
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
/* Copy FPS buffer to a 16 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer16(void)
{
  UWO *bufw = ((UWO *) draw_buffer_top_ptr) + draw_mode_current->width - 20;
  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      *(bufw + x) = draw_fps_buffer[y][x] ? 0xffff : 0;
    }
    bufw = (UWO *) (((UBY *) bufw) + draw_mode_current->pitch);
  }
}


/*============================================================================*/
/* Copy FPS buffer to a 24 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer24(void)
{
  UBY *bufb = draw_buffer_top_ptr + (draw_mode_current->width - 20)*3;

  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      UBY color = draw_fps_buffer[y][x] ? 0xff : 0;
      *(bufb + x*3) = color;
      *(bufb + x*3 + 1) = color;
      *(bufb + x*3 + 2) = color;
    }
    bufb += draw_mode_current->pitch;
  }
}


/*============================================================================*/
/* Copy FPS buffer to a 32 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer32(void)
{
  ULO *bufl = ((ULO *) draw_buffer_top_ptr) + draw_mode_current->width - 20;

#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode()) {
    // move left to offset for clipping at the right; since width is adjusted dynamically for scale, reduce by that, all other values are static
    bufl -= RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2 - RetroPlatformGetScreenWidthAdjusted() / RetroPlatformGetDisplayScale() - RetroPlatformGetClippingOffsetLeftAdjusted();

    // move down to compensate for clipping at top
    bufl += RetroPlatformGetClippingOffsetTopAdjusted() * draw_mode_current->pitch / 4;
  }
#endif

  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      *(bufl + x) = draw_fps_buffer[y][x] ? 0xffffffff : 0;
    }
    bufl = (ULO *) (((UBY *) bufl) + draw_mode_current->pitch);
  }
}


/*============================================================================*/
/* Draws FPS counter in current framebuffer                                   */
/*============================================================================*/

static void drawFpsCounter(void)
{
  if (draw_fps_counter_enabled)
  {
    STR s[16];

    sprintf(s, "%u", drawStatLast50FramesFps());
    drawFpsText(s);
    switch (draw_mode_current->bits)
    {
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
/* Draw module properties                                                     */
/*============================================================================*/

BOOLE drawSetMode(ULO width,
		  ULO height, 
		  ULO colorbits, 
		  ULO refresh,
		  BOOLE windowed)
{
  felist *l;
  ULO allow_any_refresh;

#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode())
  {
    height = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * 2;
    width  = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH  * 2;
  }
#endif

  for (allow_any_refresh = 0; allow_any_refresh < 2; allow_any_refresh++)
  {
    for (l = draw_modes; l != NULL; l = listNext(l)) {
      draw_mode *dm = (draw_mode*) listNode(l);
      if ((dm->width == width) &&
	(dm->height == height) &&
	(windowed || (dm->bits == colorbits)) &&
	(allow_any_refresh || (dm->refresh == refresh)) &&
	(dm->windowed == windowed))
      {
	draw_mode_current = dm;
	return TRUE;
      }
    }
  }
  draw_mode_current = (draw_mode*) listNode(draw_modes);
  return FALSE;
}

felist *drawGetModes(void)
{
  return draw_modes;
}

void drawSetDisplayScale(DISPLAYSCALE displayscale)
{
  draw_displayscale = displayscale;
}

DISPLAYSCALE drawGetDisplayScale(void)
{
  return draw_displayscale;
}

ULO drawGetDisplayScaleFactor(void)
{
  return (draw_displayscale == DISPLAYSCALE_1X) ? 2 : 4;
}

void drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy)
{
  draw_displayscale_strategy = displayscalestrategy;
}

DISPLAYSCALE_STRATEGY drawGetDisplayScaleStrategy(void)
{
  return draw_displayscale_strategy;
}

void drawSetDisplayDriver(DISPLAYDRIVER displaydriver)
{
  draw_displaydriver = displaydriver;
}

DISPLAYDRIVER drawGetDisplayDriver()
{
  return draw_displaydriver;
}

void drawSetFrameskipRatio(ULO frameskipratio)
{
  draw_frame_skip_factor = frameskipratio;
}

void drawSetFPSCounterEnabled(bool enabled)
{
  draw_fps_counter_enabled = enabled;
}

void drawSetLEDsEnabled(bool enabled)
{
  draw_LEDs_enabled = enabled;
}

void drawSetLED(int index, bool state)
{
  if (index < DRAW_LED_COUNT)
  {
    draw_LEDs_state[index] = state;
  }
}

void drawSetAllowMultipleBuffers(BOOLE allow_multiple_buffers)
{
  draw_allow_multiple_buffers = allow_multiple_buffers;
}

BOOLE drawGetAllowMultipleBuffers(void)
{
  return draw_allow_multiple_buffers;
}

ULO drawGetBufferCount(void)
{
  return draw_buffer_count;
}

/*============================================================================*/
/* Add one mode to the list of useable modes                                  */
/*============================================================================*/

void drawModeAdd(draw_mode *modenode)
{
  draw_modes = listAddLast(draw_modes, listNew(modenode));
}


/*============================================================================*/
/* Clear mode list (on startup)                                               */
/*============================================================================*/

static void drawModesClear()
{
  draw_modes = NULL;
  draw_mode_current = NULL;
}


/*============================================================================*/
/* Free mode list                                                             */
/*============================================================================*/

void drawModesFree()
{
  listFreeAll(draw_modes, TRUE);
  drawModesClear();
}


/*============================================================================*/
/* Dump mode list to log                                                      */
/*============================================================================*/

static void drawModesDump(void)
{
  felist *tmp = draw_modes;
  fellowAddLog("Draw module mode list:\n");
  while (tmp != NULL)
  {
    fellowAddLog(((draw_mode *) listNode(tmp))->name);
    fellowAddLog("\n");
    tmp = listNext(tmp);
  }
}


/*============================================================================*/
/* Color translation initialization                                           */
/*============================================================================*/

static void drawColorTranslationInitialize(void)
{
  ULO k, r, g, b;

  /* create color translation table */
  for (k = 0; k < 4096; k++) 
  {
    r = ((k & 0xf00) >> 8) << (draw_mode_current->redpos + draw_mode_current->redsize - 4);
    g = ((k & 0xf0) >> 4) << (draw_mode_current->greenpos + draw_mode_current->greensize - 4);
    b = (k & 0xf) << (draw_mode_current->bluepos + draw_mode_current->bluesize - 4);
    draw_color_table[k] = r | g | b;
    if (draw_mode_current->bits <= 16)
    {
      draw_color_table[k] = draw_color_table[k]<<16 | draw_color_table[k];
    }
  }
}


/*============================================================================*/
/* Calculate the width of the screen in Amiga lores pixels                    */
/*============================================================================*/

static void drawAmigaScreenWidth(draw_mode *dm)
{
  ULO totalscale = drawGetDisplayScaleFactor();

  draw_width_amiga = dm->width / totalscale;
  if (draw_width_amiga > 384)
  {
    draw_width_amiga = 384;
  }
  if (draw_width_amiga <= 343)
  {
#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
      draw_left = 125;
    else
#endif
      draw_left = 129;
    draw_right = draw_left + draw_width_amiga;
  }
  else
  {
#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
      draw_right = 468;
    else
#endif
      draw_right = 472;
    draw_left = draw_right - draw_width_amiga;
  }
  draw_width_amiga_real = draw_width_amiga*totalscale;
}




/*============================================================================*/
/* Calculate the height of the screen in Amiga lores pixels                   */
/*============================================================================*/

static void drawAmigaScreenHeight(draw_mode *dm)
{
  ULO totalscale = drawGetDisplayScaleFactor();

  draw_height_amiga = dm->height/totalscale;
  if (draw_height_amiga > 288)
  {
    draw_height_amiga = 288;
  }
  if (draw_height_amiga <= 270)
  {
    draw_top = 44;
    draw_bottom = 44 + draw_height_amiga;
  }
  else
  {
    draw_bottom = busGetMaxLinesInFrame();
    draw_top = busGetMaxLinesInFrame() - draw_height_amiga;
  }
  draw_height_amiga_real = draw_height_amiga*totalscale;
}


/*============================================================================*/
/* Center the Amiga screen                                                    */
/*============================================================================*/

static void drawAmigaScreenCenter(draw_mode *dm)
{
  draw_hoffset = ((dm->width - draw_width_amiga_real)/2) & (~7);
  draw_voffset = (dm->height - draw_height_amiga_real)/2 & (~1);
}


/*============================================================================*/
/* Calulate needed geometry information about Amiga screen                    */
/*============================================================================*/

static void drawAmigaScreenGeometry(draw_mode *dm)
{
  drawAmigaScreenWidth(dm);
  drawAmigaScreenHeight(dm);
  drawAmigaScreenCenter(dm);
}


/*============================================================================*/
/* Selects drawing routines for the current mode                              */
/*============================================================================*/

static void drawModeTablesInitialize(draw_mode *dm)
{
  // initialize some values
  draw_buffer_draw = 0;
  draw_buffer_show = 0;

  drawModeFunctionsInitialize(dm);

  memoryWriteWord((UWO) bplcon0, 0xdff100);
  drawAmigaScreenGeometry(draw_mode_current);
  drawColorTranslationInitialize();
  graphInitializeShadowColors();
  draw_buffer_current_ptr = draw_buffer_top_ptr;
  drawHAMTableInit(draw_mode_current);
}

/*============================================================================*/
/* This routine flips to another drawing buffer, display the next complete    */
/*============================================================================*/

static void drawBufferFlip(void)
{
  if (++draw_buffer_show >= draw_buffer_count)
  {
    draw_buffer_show = 0;
  }
  if (++draw_buffer_draw >= draw_buffer_count)
  {
    draw_buffer_draw = 0;
  }
  gfxDrvBufferFlip();
}


/*============================================================================*/
/* Performance statistics interface					      */
/*============================================================================*/

/* Background, make it possible to make a simple report with key-numbers about*/
/* the frames per seconds performance. */

/* The statistics measure time in ms for one frame, the last 50 frames, and */
/* the current session */

/* The statistic getters return 0 if a division by zero is detected. */
/* The stats are cleared every time emulation is started. */

/*
The interface for getting stats are: 

ULO drawStatLast50FramesFps(void);
ULO drawStatLastFrameFps(void);
ULO drawStatSessionFps(void);
*/

/* The fps number drawn in the top right corner of the emulation screen now use
drawStatLast50FramesFps()
*/

/* The stats functions use timerGetTimeMs() in the timer.c module.
There is a potential problem, on Win32 this function has a
accuracy of around 7 ms. This is very close to the time it takes to emulate
a single frame, which means that drawLastFrameFps() can report inaccurate
values. */

ULO draw_stat_first_frame_timestamp;
ULO draw_stat_last_frame_timestamp;
ULO draw_stat_last_50_timestamp;
ULO draw_stat_last_frame_ms;
ULO draw_stat_last_50_ms;
ULO draw_stat_frame_count;

/* Clear all statistics data */
void drawStatClear(void)
{
  draw_stat_last_50_ms = 0;
  draw_stat_last_frame_ms = 0;
  draw_stat_frame_count = 0;
}

/* New frame, take timestamp */
static void drawStatTimestamp(void)
{
  ULO timestamp = timerGetTimeMs(); /* Get current time */
  if (draw_stat_frame_count == 0)
  {
    draw_stat_first_frame_timestamp = timerGetTimeMs();
    draw_stat_last_frame_timestamp = draw_stat_first_frame_timestamp;
    draw_stat_last_50_timestamp = draw_stat_first_frame_timestamp;
  }
  else
  {
    /* Time for last frame */
    draw_stat_last_frame_ms = timestamp - draw_stat_last_frame_timestamp;
    draw_stat_last_frame_timestamp = timestamp;

    /* Update stats for last 50 frames, 1 Amiga 500 PAL second */
    if ((draw_stat_frame_count % 50) == 0)
    {
      draw_stat_last_50_ms = timestamp - draw_stat_last_50_timestamp;
      draw_stat_last_50_timestamp = timestamp;
    }
  }
  draw_stat_frame_count++;
}

ULO drawStatLast50FramesFps(void)
{
  if (draw_stat_last_50_ms == 0)
  {
    return 0;
  }
  return 50000 / (draw_stat_last_50_ms);
}

ULO drawStatLastFrameFps(void)
{
  if (draw_stat_last_frame_ms == 0)
  {
    return 0;
  }
  return 1000 / draw_stat_last_frame_ms;
}

ULO drawStatSessionFps(void)
{
  ULO session_time = draw_stat_last_frame_timestamp - draw_stat_first_frame_timestamp;
  if (session_time == 0)
  {
    return 0;
  }
  return (draw_frame_count*20) / (session_time + 14);
}


/*============================================================================*/
/* Validates the buffer pointer                                               */
/* Returns the width in bytes of one line                                     */
/* Returns zero if there is an error                                          */
/*============================================================================*/

ULO drawValidateBufferPointer(ULO amiga_line_number)
{
  ULO scale = drawGetDisplayScaleFactor();

  draw_buffer_top_ptr = gfxDrvValidateBufferPointer();
  if (draw_buffer_top_ptr == NULL)
  {
    fellowAddLog("Buffer ptr is NULL\n");
    return 0;
  }

  draw_buffer_current_ptr = 
    draw_buffer_top_ptr + (draw_mode_current->pitch * scale * (amiga_line_number - draw_top)) +
    (draw_mode_current->pitch * draw_voffset) + (draw_hoffset * (draw_mode_current->bits >> 3));

  if (drawGetUseInterlacedRendering())
  {
    if (!drawGetFrameIsLong())
    {
      draw_buffer_current_ptr += ((draw_mode_current->pitch * scale) / 2);
    }
  }

  return draw_mode_current->pitch * scale;
}


/*============================================================================*/
/* Invalidates the buffer pointer                                             */
/*============================================================================*/

void drawInvalidateBufferPointer(void)
{
  gfxDrvInvalidateBufferPointer();
}


/*============================================================================*/
/* Called on every end of frame                                               */
/*============================================================================*/

void drawHardReset(void)
{
  draw_switch_bg_to_bpl = FALSE;
}

/*============================================================================*/
/* Called on emulation start                                                  */
/*============================================================================*/

void drawEmulationStart(void)
{
  ULO gfxModeNumberOfBuffers = (drawGetAllowMultipleBuffers() && !cfgGetDeinterlace(draw_config)) ? 3 : 1;

  draw_switch_bg_to_bpl = FALSE;
  draw_frame_skip = 0;
  gfxDrvSetMode(draw_mode_current);

  gfxDrvEmulationStart(gfxModeNumberOfBuffers);
  drawStatClear();

  drawSetDeinterlace(cfgGetDeinterlace(draw_config));
}

BOOLE drawEmulationStartPost(void)
{
  BOOLE result;

  draw_buffer_count = gfxDrvEmulationStartPost();
  result = (draw_buffer_count != 0);
  if (result)
  {
    drawModeTablesInitialize(draw_mode_current);
    draw_buffer_show = 0;
    draw_buffer_draw = draw_buffer_count - 1;
  }
  else
  {
    fellowAddLogRequester(FELLOW_REQUESTER_TYPE_ERROR, 
      "Failure: The graphics driver failed to allocate enough graphics card memory");
  }
  return result;
}


/*============================================================================*/
/* Called on emulation halt                                                   */
/*============================================================================*/

void drawEmulationStop(void)
{
  gfxDrvEmulationStop();
}


/*============================================================================*/
/* Things initialized once at startup                                         */
/*============================================================================*/

BOOLE drawStartup(void)
{
  draw_config = cfgManagerGetCurrentConfig(&cfg_manager);

  drawModesClear();
  if (!gfxDrvStartup(cfgGetDisplayDriver(draw_config)))
  {
    return FALSE;
  }
  draw_mode_current = (draw_mode *) listNode(draw_modes);
  drawDualTranslationInitialize();
  draw_left = 120;
  draw_right = 440;
  draw_top = 0x45;
  draw_bottom = 0x10d;
  draw_view_scroll = 0;
  draw_switch_bg_to_bpl = FALSE;
  draw_frame_count = 0;
  draw_clear_buffers = 0;
  drawSetDisplayScale(DISPLAYSCALE_1X);
  drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY_SOLID);
  drawSetFrameskipRatio(1);
  drawSetFPSCounterEnabled(false);
  drawSetLEDsEnabled(false);
  drawSetAllowMultipleBuffers(FALSE);

  drawInterlaceStartup();

  for (ULO i = 0; i < DRAW_LED_COUNT; i++)
  {
    drawSetLED(i, false);
  }
  return TRUE;
}

/*============================================================================*/
/* Things uninitialized at emulator shutdown                                  */
/*============================================================================*/

void drawShutdown(void)
{
  drawModesFree();
  gfxDrvShutdown();

  drawWriteProfilingResultsToFile();
}

void drawUpdateDrawmode(void) 
{
  draw_line_routine = draw_line_BG_routine;
  if (graph_playfield_on == 1) 
  {
    // check if bit 8 of register dmacon is 1; check if bitplane DMA is enabled
    // check if bit 12, 13 and 14 of register bplcon0 is 1; 
    // check if atleast one bitplane is active
    if (((dmacon & 0x0100) == 0x0100) && ((bplcon0 & 0x7000) > 0x0001))
    {
      draw_switch_bg_to_bpl = TRUE;
      draw_line_routine = draw_line_BPL_manage_routine;
    }
  }
}

ULO drawGetNextLineOffsetInBytes(ULO pitch_in_bytes)
{
  if (drawGetDisplayScale() == DISPLAYSCALE_1X && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return 0; // 2x1 (ie. nothing to offset to)
  }
  else if (drawGetDisplayScale() == DISPLAYSCALE_1X && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SOLID)
  {
    return pitch_in_bytes / 2; // 2x2
  }
  else if (drawGetDisplayScale() == DISPLAYSCALE_2X && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return pitch_in_bytes / 4; // 4x2 (scanline - write two solid lines followed by two blank ones... Awful?)
  }
  return pitch_in_bytes / 4; // 4x4
}

void drawReinitializeRendering(void)
{
  drawModeTablesInitialize(draw_mode_current);
  graphLineDescClear();
}

/*==============================================================================*/
/* Drawing end of frame handler                                                 */
/*==============================================================================*/

void drawEndOfFrame(void)
{
  if (draw_frame_skip == 0)
  {
    if (draw_clear_buffers > 0)
    {
      gfxDrvClearCurrentBuffer();
      --draw_clear_buffers;
    }

    ULO pitch_in_bytes = drawValidateBufferPointer(draw_top);

    // need to test for error
    if (draw_buffer_top_ptr != NULL)
    {
#ifndef GRAPH2
      UBY *draw_buffer_current_ptr_local = draw_buffer_current_ptr;
      for (ULO i = 0; i < (draw_bottom - draw_top); i++) {
        graph_line *graph_frame_ptr = graphGetLineDesc(draw_buffer_draw, draw_top + i);
        if (graph_frame_ptr != NULL)
        {
          if (graph_frame_ptr->linetype != GRAPH_LINE_SKIP)
          {
            if (graph_frame_ptr->linetype != GRAPH_LINE_BPL_SKIP)
            {
              ((draw_line_func) (graph_frame_ptr->draw_line_routine))(graph_frame_ptr, drawGetNextLineOffsetInBytes(pitch_in_bytes));
	    }
          }
        }
        draw_buffer_current_ptr_local += pitch_in_bytes;
        draw_buffer_current_ptr = draw_buffer_current_ptr_local;
      }
#else
      GraphicsContext.BitplaneDraw.TmpFrame(pitch_in_bytes);
#endif

      drawLEDs();
      drawFpsCounter();
      drawInvalidateBufferPointer();

      drawViewScroll();
      drawStatTimestamp();
      drawBufferFlip();
    }
  }

  draw_frame_count++; // count frames
  draw_frame_skip--;  // frame skipping

  if (draw_frame_skip < 0) 
  {
    draw_frame_skip = draw_frame_skip_factor;
  }
}
