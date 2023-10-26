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
#include <cmath>
#include <algorithm>

#include "defs.h"
#include "fellow.h"
#include "bus.h"
#include "draw.h"
#include "fmem.h"
#include "graph.h"
#include "gfxdrv.h"
#include "ListTree.h"
#include "timer.h"
#include "fonts.h"
#include "copper.h"
#include "sprite.h"
#include "CONFIG.H"

#include "draw_pixelrenderers.h"
#include "draw_interlace_control.h"

#include "graphics/Graphics.h"

#include <map>

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

cfg *draw_config;

/*============================================================================*/
/* Mode list, nodes created by the graphics driver, and pointer to the mode   */
/* that is current                                                            */
/*============================================================================*/

draw_mode_list draw_modes;
draw_mode *draw_mode_current;
draw_mode draw_mode_windowed;
draw_buffer_information draw_buffer_info;

/*============================================================================*/
/* Host display rendering properties                                          */
/*============================================================================*/

DISPLAYSCALE draw_displayscale;
DISPLAYSCALE_STRATEGY draw_displayscale_strategy;
DISPLAYDRIVER draw_displaydriver;
GRAPHICSEMULATIONMODE draw_graphicsemulationmode;

BOOLE draw_allow_multiple_buffers; /* allows the use of more buffers */
uint32_t draw_clear_buffers;

/*============================================================================*/
/* Data concerning the positioning of the Amiga screen in the host buffer     */
/*============================================================================*/

draw_rect draw_buffer_clip;

void drawSetBufferClip(const draw_rect &buffer_clip)
{
  draw_buffer_clip = buffer_clip;
}

const draw_rect &drawGetBufferClip()
{
  return draw_buffer_clip;
}

uint32_t drawGetBufferClipLeft()
{
  return draw_buffer_clip.left;
}

float drawGetBufferClipLeftAsFloat()
{
  return static_cast<float>(drawGetBufferClipLeft());
}

uint32_t drawGetBufferClipTop()
{
  return draw_buffer_clip.top;
}

float drawGetBufferClipTopAsFloat()
{
  return static_cast<float>(drawGetBufferClipTop());
}

uint32_t drawGetBufferClipWidth()
{
  return draw_buffer_clip.GetWidth();
}

float drawGetBufferClipWidthAsFloat()
{
  return static_cast<float>(drawGetBufferClipWidth());
}

uint32_t drawGetBufferClipHeight()
{
  return draw_buffer_clip.GetHeight();
}

float drawGetBufferClipHeightAsFloat()
{
  return static_cast<float>(drawGetBufferClipHeight());
}

/*============================================================================*/
/* Bounding box of the Amiga screen that is visible in the host buffer        */
/*============================================================================*/

draw_rect draw_clip_max_pal;

draw_rect draw_internal_clip;
draw_rect draw_output_clip;

void drawSetInternalClip(const draw_rect &internal_clip)
{
#ifdef _DEBUG
  _core.Log->AddLog("drawSetInternalClip(rectangle left=%d, top=%d, right=%d, bottom=%d)\n", internal_clip.left, internal_clip.top, internal_clip.right, internal_clip.bottom);
#endif
  draw_internal_clip = internal_clip;
}

const draw_rect &drawGetInternalClip()
{
  return draw_internal_clip;
}

void drawSetOutputClip(const draw_rect &output_clip)
{
#ifdef _DEBUG
  _core.Log->AddLog("drawSetOutputClip(rectangle left=%d, top=%d, right=%d, bottom=%d)\n", output_clip.left, output_clip.top, output_clip.right, output_clip.bottom);
#endif
  draw_output_clip = output_clip;
}

const draw_rect &drawGetOutputClip()
{
  return draw_output_clip;
}

void drawInitializePredefinedClipRectangles()
{
  draw_clip_max_pal.left = 88;
  draw_clip_max_pal.top = 26;
  draw_clip_max_pal.right = 472;
  draw_clip_max_pal.bottom = 314;
}

/*============================================================================*/
/* Add one mode to the list of useable modes                                  */
/*============================================================================*/

void drawAddMode(draw_mode *modenode)
{
  draw_modes.push_back(modenode);
}

/*============================================================================*/
/* Returns the first mode, or nullptr if list is empty                        */
/*============================================================================*/

static draw_mode *drawGetFirstMode()
{
  return (draw_modes.empty()) ? nullptr : draw_modes.front();
}

/*============================================================================*/
/* Free mode list                                                             */
/*============================================================================*/

void drawClearModeList()
{
  for (draw_mode *dm : draw_modes)
  {
    delete dm;
  }
  draw_modes.clear();
  draw_mode_current = &draw_mode_windowed;
}

static draw_mode *drawFindMode(uint32_t width, uint32_t height, uint32_t colorbits, uint32_t refresh, bool allow_any_refresh)
{
  auto item_iterator = std::find_if(draw_modes.begin(), draw_modes.end(), [width, height, colorbits, refresh, allow_any_refresh](draw_mode *dm) {
    return (dm->width == width) && (dm->height == height) && (dm->bits == colorbits) && (allow_any_refresh || (dm->refresh == refresh));
  });

  return (item_iterator != draw_modes.end()) ? *item_iterator : nullptr;
}

draw_mode_list &drawGetModes()
{
  return draw_modes;
}

/*============================================================================*/
/* Color table, 12 bit Amiga color to host display color                      */
/*============================================================================*/

uint32_t draw_color_table[4096]; /* Translate Amiga 12-bit color to whatever color */
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

uint32_t draw_buffer_show;       /* Number of the currently shown buffer */
uint32_t draw_buffer_draw;       /* Number of the current drawing buffer */
uint32_t draw_buffer_count;      /* Number of available framebuffers */
uint32_t draw_frame_count;       /* Counts frames, both skipped and drawn */
uint32_t draw_frame_skip_factor; /* Frame-skip factor, 1 / (factor + 1) */
int32_t draw_frame_skip;         /* Running frame-skip counter */
uint32_t draw_switch_bg_to_bpl;  /* Flag TRUE if on current line, switch from */
/* background color to bitplane data */

/*============================================================================*/
/* These constants define the LED symbol appearance                           */
/*============================================================================*/

#define DRAW_LED_COUNT 5
#define DRAW_LED_WIDTH 12
#define DRAW_LED_HEIGHT 4
#define DRAW_LED_FIRST_X 16
#define DRAW_LED_FIRST_Y 4
#define DRAW_LED_GAP 8
#define DRAW_LED_COLOR_ON 0x00FF00  /* Green */
#define DRAW_LED_COLOR_OFF 0x000000 /* Black */

bool draw_LEDs_enabled;
bool draw_LEDs_state[DRAW_LED_COUNT];

/*============================================================================*/
/* Draws a LED symbol                                                         */
/*============================================================================*/

static void drawLED16(int x, int y, int width, int height, uint32_t color)
{
  uint16_t *bufw = ((uint16_t *)(draw_buffer_info.top_ptr + draw_buffer_info.pitch * y)) + x;
  uint16_t color16 = (uint16_t)draw_color_table[((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4)];
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufw + x1) = color16;
    }
    bufw = (uint16_t *)(((uint8_t *)bufw) + draw_buffer_info.pitch);
  }
}

static void drawLED24(int x, int y, int width, int height, uint32_t color)
{
  uint8_t *bufb = draw_buffer_info.top_ptr + draw_buffer_info.pitch * y + x * 3;
  uint32_t color24 = draw_color_table[((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4)];
  uint8_t color24_1 = (uint8_t)((color24 & 0xff0000) >> 16);
  uint8_t color24_2 = (uint8_t)((color24 & 0x00ff00) >> 8);
  uint8_t color24_3 = (uint8_t)(color24 & 0x0000ff);
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufb + x1 * 3) = color24_1;
      *(bufb + x1 * 3 + 1) = color24_2;
      *(bufb + x1 * 3 + 2) = color24_3;
    }
    bufb = bufb + draw_buffer_info.pitch;
  }
}

static void drawLED32(int x, int y, int width, int height, uint32_t color)
{
  uint32_t *bufl = ((uint32_t *)(draw_buffer_info.top_ptr + draw_buffer_info.pitch * y)) + x;
  uint32_t color32 = draw_color_table[((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4)];
  for (int y1 = 0; y1 < height; y1++)
  {
    for (int x1 = 0; x1 < width; x1++)
    {
      *(bufl + x1) = color32;
    }
    bufl = (uint32_t *)(((uint8_t *)bufl) + draw_buffer_info.pitch);
  }
}

static void drawLED(int index, bool state)
{
  int x, y, height, width;

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    x = DRAW_LED_FIRST_X + (DRAW_LED_WIDTH + DRAW_LED_GAP) * index + (drawGetOutputClip().left - drawGetInternalClip().left) * drawGetOutputScaleFactor();
    y = DRAW_LED_FIRST_Y + (drawGetOutputClip().top - drawGetInternalClip().top) * drawGetOutputScaleFactor();
    height = DRAW_LED_HEIGHT;
    width = DRAW_LED_WIDTH;
#ifdef RETRO_PLATFORM
  }
  else
  {
    x = DRAW_LED_FIRST_X + (DRAW_LED_WIDTH + DRAW_LED_GAP) * index + (drawGetOutputClip().left / 2 - drawGetInternalClip().left) * 2;
    y = DRAW_LED_FIRST_Y + (drawGetOutputClip().top / 2 - drawGetInternalClip().top) * 2;
    height = DRAW_LED_HEIGHT / 2;
    width = DRAW_LED_WIDTH / 2;
  }
#endif

  uint32_t color = (state) ? DRAW_LED_COLOR_ON : DRAW_LED_COLOR_OFF;

  switch (draw_buffer_info.bits)
  {
    case 16: drawLED16(x, y, DRAW_LED_WIDTH, height, color); break;
    case 24: drawLED24(x, y, DRAW_LED_WIDTH, height, color); break;
    case 32: drawLED32(x, y, DRAW_LED_WIDTH, height, color); break;
    default: break;
  }
}

static void drawLEDs()
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
      draw_fps_buffer[i][x * 4 + j] = draw_fps_font[character][i][j];
    }
  }
}

/*============================================================================*/
/* Draws text in the FPS counter buffer                                       */
/*============================================================================*/

static void drawFpsText(char *text)
{
  for (int i = 0; i < 4; i++)
  {
    char c = *text++;
    switch (c)
    {
      case '0': drawFpsChar(9, i); break;
      case '1': drawFpsChar(0, i); break;
      case '2': drawFpsChar(1, i); break;
      case '3': drawFpsChar(2, i); break;
      case '4': drawFpsChar(3, i); break;
      case '5': drawFpsChar(4, i); break;
      case '6': drawFpsChar(5, i); break;
      case '7': drawFpsChar(6, i); break;
      case '8': drawFpsChar(7, i); break;
      case '9': drawFpsChar(8, i); break;
      case '%': drawFpsChar(10, i); break;
      case ' ':
      default: drawFpsChar(11, i); break;
    }
  }
}

/*============================================================================*/
/* Copy FPS buffer to a 16 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer16()
{
  uint16_t *bufw = ((uint16_t *)draw_buffer_info.top_ptr) + draw_buffer_info.width - 20;
  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      *(bufw + x) = draw_fps_buffer[y][x] ? 0xffff : 0;
    }
    bufw = (uint16_t *)(((uint8_t *)bufw) + draw_buffer_info.pitch);
  }
}

/*============================================================================*/
/* Copy FPS buffer to a 24 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer24()
{
  uint8_t *bufb = draw_buffer_info.top_ptr + (draw_buffer_info.width - 20) * 3;

  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      uint8_t color = draw_fps_buffer[y][x] ? 0xff : 0;
      *(bufb + x * 3) = color;
      *(bufb + x * 3 + 1) = color;
      *(bufb + x * 3 + 2) = color;
    }
    bufb += draw_buffer_info.pitch;
  }
}

/*============================================================================*/
/* Copy FPS buffer to a 32 bit screen                                         */
/*============================================================================*/

static void drawFpsToFramebuffer32()
{
  uint32_t *bufl = ((uint32_t *)draw_buffer_info.top_ptr) + draw_buffer_info.width - 20;

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    // move left to offset for clipping at the right; since width is adjusted dynamically for scale, reduce by that, all other values are static
    bufl -= RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2 - RP.GetScreenWidthAdjusted() / RP.GetDisplayScale() - RP.GetClippingOffsetLeftAdjusted();

    // move down to compensate for clipping at top
    bufl += RP.GetClippingOffsetTopAdjusted() * draw_buffer_info.pitch / 4;
  }
#endif

  for (int y = 0; y < 5; y++)
  {
    for (int x = 0; x < 20; x++)
    {
      *(bufl + x) = draw_fps_buffer[y][x] ? 0xffffffff : 0;
    }
    bufl = (uint32_t *)(((uint8_t *)bufl) + draw_buffer_info.pitch);
  }
}

/*============================================================================*/
/* Draws FPS counter in current framebuffer                                   */
/*============================================================================*/

static void drawFpsCounter()
{
  if (draw_fps_counter_enabled)
  {
    char s[16];

    sprintf(s, "%u", drawStatLast50FramesFps());
    drawFpsText(s);
    switch (draw_buffer_info.bits)
    {
      case 16: drawFpsToFramebuffer16(); break;
      case 24: drawFpsToFramebuffer24(); break;
      case 32: drawFpsToFramebuffer32(); break;
      default: break;
    }
  }
}

/*============================================================================*/
/* Draw module properties                                                     */
/*============================================================================*/

void drawSetFullScreenMode(uint32_t width, uint32_t height, uint32_t colorbits, uint32_t refresh)
{
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    height = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * 2;
    width = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2;
  }
#endif

  // Find with exact refresh
  draw_mode *mode_found = drawFindMode(width, height, colorbits, refresh, false);
  if (mode_found == nullptr)
  {
    // Try to ignore refresh
    mode_found = drawFindMode(width, height, colorbits, refresh, true);
  }

  if (mode_found != nullptr)
  {
    draw_mode_current = mode_found;
  }
  else
  {
    // No match, take the first in the list
    draw_mode_current = drawGetFirstMode();
  }
  gfxDrvGetBufferInformation(&draw_buffer_info);
}

void drawSetWindowedMode(uint32_t width, uint32_t height)
{
  draw_mode_windowed.width = width;
  draw_mode_windowed.height = height;
  draw_mode_windowed.bits = 32; // TODO, take from desktop
  draw_mode_windowed.refresh = 0;

  draw_mode_current = &draw_mode_windowed;

  gfxDrvGetBufferInformation(&draw_buffer_info);
}

void drawSetDisplayScale(DISPLAYSCALE displayscale)
{
  draw_displayscale = displayscale;
}

DISPLAYSCALE drawGetDisplayScale()
{
  return draw_displayscale;
}

static uint32_t drawGetAutomaticInternalScaleFactor()
{
  return (draw_mode_current->width < 1280) ? 2 : 4;
}

uint32_t drawGetInternalScaleFactor()
{
  if (drawGetDisplayScale() == DISPLAYSCALE_AUTO)
  {
    return drawGetAutomaticInternalScaleFactor();
  }

  return (draw_displayscale == DISPLAYSCALE_1X) ? 2 : 4;
}

uint32_t drawGetOutputScaleFactor()
{
  if (RP.GetHeadlessMode())
  {
    return RP.GetDisplayScale() * 2;
  }

  uint32_t output_scale_factor = 2;

  switch (drawGetDisplayScale())
  {
    case DISPLAYSCALE::DISPLAYSCALE_1X: output_scale_factor = 2; break;
    case DISPLAYSCALE::DISPLAYSCALE_2X: output_scale_factor = 4; break;
    case DISPLAYSCALE::DISPLAYSCALE_3X: output_scale_factor = 6; break;
    case DISPLAYSCALE::DISPLAYSCALE_4X: output_scale_factor = 8; break;
  }
  return output_scale_factor;
}

void drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy)
{
  draw_displayscale_strategy = displayscalestrategy;
}

DISPLAYSCALE_STRATEGY drawGetDisplayScaleStrategy()
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

void drawSetGraphicsEmulationMode(GRAPHICSEMULATIONMODE graphicsemulationmode)
{
  GRAPHICSEMULATIONMODE oldgraphicsemulationmode = draw_graphicsemulationmode;

  draw_graphicsemulationmode = graphicsemulationmode;

  if (oldgraphicsemulationmode != draw_graphicsemulationmode)
  {
    spriteInitializeFromEmulationMode();
    copperInitializeFromEmulationMode();
  }
}

GRAPHICSEMULATIONMODE drawGetGraphicsEmulationMode()
{
  return draw_graphicsemulationmode;
}

void drawSetFrameskipRatio(uint32_t frameskipratio)
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

BOOLE drawGetAllowMultipleBuffers()
{
  return draw_allow_multiple_buffers;
}

uint32_t drawGetBufferCount()
{
  return draw_buffer_count;
}

/*============================================================================*/
/* Color translation initialization                                           */
/*============================================================================*/

static void drawColorTranslationInitialize()
{
  /* create color translation table */
  for (uint32_t k = 0; k < 4096; k++)
  {
    uint32_t r = ((k & 0xf00) >> 8) << (draw_buffer_info.redpos + draw_buffer_info.redsize - 4);
    uint32_t g = ((k & 0xf0) >> 4) << (draw_buffer_info.greenpos + draw_buffer_info.greensize - 4);
    uint32_t b = (k & 0xf) << (draw_buffer_info.bluepos + draw_buffer_info.bluesize - 4);
    draw_color_table[k] = r | g | b;
    if (draw_buffer_info.bits <= 16)
    {
      draw_color_table[k] = draw_color_table[k] << 16 | draw_color_table[k];
    }
  }
}

/*============================================================================*/
/* Calculate the width of the screen in Amiga lores pixels                    */
/*============================================================================*/

std::pair<uint32_t, uint32_t> drawCalculateHorizontalOutputClip(uint32_t buffer_width, uint32_t buffer_scale_factor)
{
  uint32_t left, right;
  if (!RP.GetHeadlessMode() && drawGetDisplayScale() != DISPLAYSCALE::DISPLAYSCALE_AUTO)
  {
    // Output width must fit in the buffer or be reduced in width to fit
    uint32_t width_amiga = buffer_width / buffer_scale_factor;
    const draw_rect &internal_clip = drawGetInternalClip();

    if (width_amiga > internal_clip.GetWidth())
    {
      width_amiga = internal_clip.GetWidth();
    }

    if (width_amiga <= 343)
    {
      left = 129;
      // Is hardcoded left inside the internal clip?
      if (left < internal_clip.left || left >= internal_clip.right || (left + width_amiga) > internal_clip.right)
      {
        left = internal_clip.left;
      }
      right = left + width_amiga;
    }
    else
    {
      right = internal_clip.right;
      left = right - width_amiga;
    }
  }
  else
  {
    // Always use configured output width. RP always goes here.
    left = drawGetOutputClip().left;
    right = drawGetOutputClip().right;
  }
  return std::pair<uint32_t, uint32_t>(left, right);
}

/*============================================================================*/
/* Calculate the height of the screen in Amiga lores pixels                   */
/*============================================================================*/

std::pair<uint32_t, uint32_t> drawCalculateVerticalOutputClip(uint32_t buffer_height, uint32_t buffer_scale_factor)
{
  uint32_t top, bottom;
  if (!RP.GetHeadlessMode() && drawGetDisplayScale() != DISPLAYSCALE::DISPLAYSCALE_AUTO)
  {
    // Output height must fit in the buffer or be reduced in height to fit
    uint32_t height_amiga = buffer_height / buffer_scale_factor;
    const draw_rect &internal_clip = drawGetInternalClip();

    if (height_amiga > internal_clip.GetHeight())
    {
      height_amiga = internal_clip.GetHeight();
    }

    if (height_amiga <= 270)
    {
      top = 44;

      // Is hardcoded top inside the internal clip?
      if (top < internal_clip.top || top >= internal_clip.bottom || (top + height_amiga) > internal_clip.bottom)
      {
        top = internal_clip.top;
      }

      bottom = top + height_amiga;
    }
    else
    {
      bottom = internal_clip.bottom;
      top = bottom - height_amiga;
    }
  }
  else
  {
    // Always use configured output height
    top = drawGetOutputClip().top;
    bottom = drawGetOutputClip().bottom;
  }
  return std::pair<uint32_t, uint32_t>(top, bottom);
}

/*============================================================================*/
/* Calculate needed geometry information about Amiga screen                   */
/*============================================================================*/

static void drawAmigaScreenGeometry(uint32_t buffer_width, uint32_t buffer_height)
{
  uint32_t output_scale_factor = drawGetOutputScaleFactor();
  uint32_t internal_scale_factor = drawGetInternalScaleFactor();
  uint32_t buffer_clip_left, buffer_clip_top, buffer_clip_width, buffer_clip_height;

  const std::pair<uint32_t, uint32_t> horizontal_clip = drawCalculateHorizontalOutputClip(draw_mode_current->width, output_scale_factor);
  const std::pair<uint32_t, uint32_t> vertical_clip = drawCalculateVerticalOutputClip(draw_mode_current->height, output_scale_factor);

  draw_rect output_clip(horizontal_clip.first, vertical_clip.first, horizontal_clip.second, vertical_clip.second);
  drawSetOutputClip(output_clip);

  const draw_rect &internal_clip = drawGetInternalClip();

  if (!RP.GetHeadlessMode())
  {
    buffer_clip_left = (output_clip.left - internal_clip.left) * internal_scale_factor;
    buffer_clip_top = (output_clip.top - internal_clip.top) * internal_scale_factor;
    buffer_clip_width = output_clip.GetWidth() * internal_scale_factor;
    buffer_clip_height = output_clip.GetHeight() * internal_scale_factor;
  }
  else
  {
    buffer_clip_left = output_clip.left - (internal_clip.left * internal_scale_factor);
    buffer_clip_top = output_clip.top - (internal_clip.top * internal_scale_factor);
    buffer_clip_width = output_clip.GetWidth();
    buffer_clip_height = output_clip.GetHeight();
  }

  drawSetBufferClip(draw_rect(buffer_clip_left, buffer_clip_top, buffer_clip_left + buffer_clip_width, buffer_clip_top + buffer_clip_height));
}

/*============================================================================*/
/* Selects drawing routines for the current mode                              */
/*============================================================================*/

static void drawModeTablesInitialize()
{
  // initialize some values
  draw_buffer_draw = 0;
  draw_buffer_show = 0;

  drawModeFunctionsInitialize();

  wbplcon0((uint16_t)_core.Registers.BplCon0, 0xdff100);
  drawColorTranslationInitialize();
  graphInitializeShadowColors();
  draw_buffer_info.current_ptr = draw_buffer_info.top_ptr;
  drawHAMTableInit();
}

/*============================================================================*/
/* This routine flips to another drawing buffer, display the next complete    */
/*============================================================================*/

static void drawBufferFlip()
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

uint32_t drawStatLast50FramesFps();
uint32_t drawStatLastFrameFps();
uint32_t drawStatSessionFps();
*/

/* The fps number drawn in the top right corner of the emulation screen now use
drawStatLast50FramesFps()
*/

/* The stats functions use timerGetTimeMs() in the timer.c module.
There is a potential problem, on Win32 this function has a
accuracy of around 7 ms. This is very close to the time it takes to emulate
a single frame, which means that drawLastFrameFps() can report inaccurate
values. */

uint32_t draw_stat_first_frame_timestamp;
uint32_t draw_stat_last_frame_timestamp;
uint32_t draw_stat_last_50_timestamp;
uint32_t draw_stat_last_frame_ms;
uint32_t draw_stat_last_50_ms;
uint32_t draw_stat_frame_count;

/* Clear all statistics data */
void drawStatClear()
{
  draw_stat_last_50_ms = 0;
  draw_stat_last_frame_ms = 0;
  draw_stat_frame_count = 0;
}

/* New frame, take timestamp */
void drawStatTimestamp()
{
  uint32_t timestamp = timerGetTimeMs(); /* Get current time */
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

uint32_t drawStatLast50FramesFps()
{
  if (draw_stat_last_50_ms == 0)
  {
    return 0;
  }
  return 50000 / (draw_stat_last_50_ms);
}

uint32_t drawStatLastFrameFps()
{
  if (draw_stat_last_frame_ms == 0)
  {
    return 0;
  }
  return 1000 / draw_stat_last_frame_ms;
}

uint32_t drawStatSessionFps()
{
  uint32_t session_time = draw_stat_last_frame_timestamp - draw_stat_first_frame_timestamp;
  if (session_time == 0)
  {
    return 0;
  }
  return (draw_frame_count * 20) / (session_time + 14);
}

/*============================================================================*/
/* Validates the buffer pointer                                               */
/* Returns the width in bytes of one line                                     */
/* Returns zero if there is an error                                          */
/*============================================================================*/

uint32_t drawValidateBufferPointer(uint32_t amiga_line_number)
{
  uint32_t internal_scale_factor = drawGetInternalScaleFactor();

  draw_buffer_info.top_ptr = gfxDrvValidateBufferPointer();

  if (draw_buffer_info.top_ptr == nullptr)
  {
    _core.Log->AddLog("Buffer ptr is nullptr\n");
    return 0;
  }

  // Calculate a pointer to the first pixel on the requested line.
  draw_buffer_info.current_ptr = draw_buffer_info.top_ptr + draw_buffer_info.pitch * internal_scale_factor * (amiga_line_number - drawGetInternalClip().top);
  //+
  //  draw_buffer_info.pitch * draw_buffer_clip_offset.y +
  //  draw_buffer_clip_offset.x * (draw_mode_current->bits >> 3);

  if (drawGetUseInterlacedRendering())
  {
    if (!drawGetFrameIsLong())
    {
      // Draw on the short field.
      draw_buffer_info.current_ptr += (draw_buffer_info.pitch * internal_scale_factor) / 2;
    }
  }

  return draw_buffer_info.pitch * internal_scale_factor;
}

/*============================================================================*/
/* Invalidates the buffer pointer                                             */
/*============================================================================*/

void drawInvalidateBufferPointer()
{
  gfxDrvInvalidateBufferPointer();
}

/*============================================================================*/
/* Called on every end of frame                                               */
/*============================================================================*/

void drawHardReset()
{
  draw_switch_bg_to_bpl = FALSE;
}

/*============================================================================*/
/* Called on emulation start                                                  */
/*============================================================================*/

void drawEmulationStart()
{
  uint32_t gfxModeNumberOfBuffers = (drawGetAllowMultipleBuffers() && !cfgGetDeinterlace(draw_config)) ? 3 : 1;

  draw_switch_bg_to_bpl = FALSE;
  draw_frame_skip = 0;
  gfxDrvSetMode(draw_mode_current, draw_mode_current == &draw_mode_windowed);

  gfxDrvEmulationStart(gfxModeNumberOfBuffers);
  drawStatClear();

  drawSetDeinterlace(cfgGetDeinterlace(draw_config));
}

BOOLE drawEmulationStartPost()
{
  drawAmigaScreenGeometry(draw_buffer_info.width, draw_buffer_info.height);
  draw_buffer_count = gfxDrvEmulationStartPost();
  bool result = (draw_buffer_count != 0);
  if (result)
  {
    draw_buffer_show = 0;
    draw_buffer_draw = draw_buffer_count - 1;
    drawModeTablesInitialize();
  }
  else
  {
    fellowShowRequester(FELLOW_REQUESTER_TYPE_ERROR, "Failure: The graphics driver failed to start. See fellow.log for more details.");
  }
  _core.Log->AddLog("drawEmulationStartPost(): Buffer is (%d,%d,%d)\n", draw_buffer_info.width, draw_buffer_info.height, draw_buffer_info.bits);
  return result;
}

/*============================================================================*/
/* Called on emulation halt                                                   */
/*============================================================================*/

void drawEmulationStop()
{
  gfxDrvEmulationStop();
}

/*============================================================================*/
/* Things initialized once at startup                                         */
/*============================================================================*/

BOOLE drawStartup()
{
  draw_config = cfgManagerGetCurrentConfig(&cfg_manager);

  drawClearModeList();
  if (!gfxDrvStartup(cfgGetDisplayDriver(draw_config)))
  {
    return FALSE;
  }

  draw_mode_windowed.width = 640;
  draw_mode_windowed.height = 400;
  draw_mode_windowed.bits = 32;

  draw_mode_current = &draw_mode_windowed;
  drawDualTranslationInitialize();

  drawInitializePredefinedClipRectangles();

  if (!RP.GetHeadlessMode())
  {
    drawSetInternalClip(draw_clip_max_pal);
    drawSetOutputClip(draw_clip_max_pal);
  }

  draw_switch_bg_to_bpl = FALSE;
  draw_frame_count = 0;
  draw_clear_buffers = 0;

  drawSetDisplayScale(DISPLAYSCALE_1X);
  drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY_SOLID);
  drawSetFrameskipRatio(1);
  drawSetFPSCounterEnabled(false);
  drawSetLEDsEnabled(false);
  drawSetAllowMultipleBuffers(FALSE);
  drawSetGraphicsEmulationMode(cfgGetGraphicsEmulationMode(draw_config));

  drawInterlaceStartup();

  for (uint32_t i = 0; i < DRAW_LED_COUNT; i++)
  {
    drawSetLED(i, false);
  }
  return TRUE;
}

/*============================================================================*/
/* Things uninitialized at emulator shutdown                                  */
/*============================================================================*/

void drawShutdown()
{
  drawClearModeList();
  gfxDrvShutdown();
}

void drawUpdateDrawmode()
{
  draw_line_routine = draw_line_BG_routine;
  if (graph_playfield_on == 1)
  {
    // check if bit 8 of register dmacon is 1; check if bitplane DMA is enabled
    // check if bit 12, 13 and 14 of register bplcon0 is 1;
    // check if atleast one bitplane is active
    if (((dmacon & 0x0100) == 0x0100) && _core.RegisterUtility.GetEnabledBitplaneCount() != 0)
    {
      draw_switch_bg_to_bpl = TRUE;
      draw_line_routine = draw_line_BPL_manage_routine;
    }
  }
}

uint32_t drawGetNextLineOffsetInBytes(uint32_t pitch_in_bytes)
{
  uint32_t internal_scale_factor = drawGetInternalScaleFactor();
  if (internal_scale_factor == 2 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return 0; // 2x1 (ie. nothing to offset to)
  }
  else if (internal_scale_factor == 2 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SOLID)
  {
    return pitch_in_bytes / 2; // 2x2
  }
  else if (internal_scale_factor == 4 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return pitch_in_bytes / 4; // 4x2 (scanline - write two solid lines followed by two blank ones... Awful?)
  }
  return pitch_in_bytes / 4; // 4x4
}

void drawReinitializeRendering()
{
  drawModeTablesInitialize();
  graphLineDescClear();
}

/*==============================================================================*/
/* Drawing end of frame handler                                                 */
/*==============================================================================*/

void drawEndOfFrame()
{
  if (draw_frame_skip == 0)
  {
    if (draw_clear_buffers > 0)
    {
      gfxDrvClearCurrentBuffer();
      --draw_clear_buffers;
    }

    uint32_t pitch_in_bytes = drawValidateBufferPointer(drawGetInternalClip().top);

    // need to test for error
    if (draw_buffer_info.top_ptr != nullptr)
    {
      if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_LINEEXACT)
      {
        uint32_t height = drawGetInternalClip().GetHeight();
        uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
        for (uint32_t i = 0; i < height; i++)
        {
          graph_line *graph_frame_ptr = graphGetLineDesc(draw_buffer_draw, drawGetInternalClip().top + i);
          if (graph_frame_ptr != nullptr)
          {
            if (graph_frame_ptr->linetype != GRAPH_LINE_SKIP)
            {
              if (graph_frame_ptr->linetype != GRAPH_LINE_BPL_SKIP)
              {
                ((draw_line_func)(graph_frame_ptr->draw_line_routine))(graph_frame_ptr, drawGetNextLineOffsetInBytes(pitch_in_bytes));
              }
            }
          }
          draw_buffer_current_ptr_local += pitch_in_bytes;
          draw_buffer_info.current_ptr = draw_buffer_current_ptr_local;
        }
      }
      else
      {
        GraphicsContext.BitplaneDraw.TmpFrame(pitch_in_bytes);
      }

      drawLEDs();
      drawFpsCounter();
      drawInvalidateBufferPointer();

      //      drawClipScroll();
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
