#pragma once

#ifndef GRAPH_H
#include "graph.h"
#endif

#include "CONFIG.H"
#include <map>
#include <list>

/*===========================================================================*/
/* Mode handling, describes the geometry of the host screen                  */
/*===========================================================================*/

struct draw_mode
{
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint32_t bits;
  uint32_t refresh;
  char name[80];
};

typedef std::list<draw_mode*> draw_mode_list;

struct draw_rect
{
  uint32_t left;
  uint32_t top;
  uint32_t right;
  uint32_t bottom;

  uint32_t GetWidth() const { return right - left; }
  uint32_t GetHeight() const { return bottom - top; }

  draw_rect(uint32_t clip_left, uint32_t clip_top, uint32_t clip_right, uint32_t clip_bottom)
    : left(clip_left), top(clip_top), right(clip_right), bottom(clip_bottom) {}

  draw_rect() : left(0), top(0), right(0), bottom(0) {}
};

struct draw_buffer_information
{
  uint8_t *top_ptr;
  uint8_t *current_ptr;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bits;
  uint32_t redsize;
  uint32_t redpos;
  uint32_t greensize;
  uint32_t greenpos;
  uint32_t bluesize;
  uint32_t bluepos;
};

/*===========================================================================*/
/* Draw line routines and data                                               */
/*===========================================================================*/

typedef void (*draw_line_func)(graph_line *linedescription, uint32_t linelength);

extern draw_line_func draw_line_routine;
extern draw_line_func draw_line_BG_routine;
extern draw_line_func draw_line_BPL_manage_routine;
extern draw_line_func draw_line_BPL_res_routine;
extern draw_line_func draw_line_lores_routine;
extern draw_line_func draw_line_hires_routine;
extern draw_line_func draw_line_dual_hires_routine;
extern draw_line_func draw_line_dual_lores_routine;
extern draw_line_func draw_line_HAM_lores_routine;

extern uint32_t draw_frame_count;
extern uint32_t draw_color_table[4096];
extern uint32_t draw_buffer_draw;                 /* Number of the current drawing buffer */

extern int32_t draw_frame_skip;
extern uint32_t draw_switch_bg_to_bpl;
extern uint32_t draw_clear_buffers;

extern draw_buffer_information draw_buffer_info;
extern draw_mode draw_mode_windowed;

extern uint64_t drawMake64BitColorFrom32Bit(uint32_t color);

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawAddMode(draw_mode *modenode);
extern void drawClearModeList();
extern draw_mode_list& drawGetModes();

extern void drawSetFullScreenMode(uint32_t width, uint32_t height, uint32_t colorbits, uint32_t refresh);
extern void drawSetWindowedMode(uint32_t width, uint32_t height);
extern void drawSetDisplayScale(DISPLAYSCALE displayscale);
extern DISPLAYSCALE drawGetDisplayScale(void);
extern void drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy);
extern DISPLAYSCALE_STRATEGY drawGetDisplayScaleStrategy(void);
extern uint32_t drawGetInternalScaleFactor();
extern uint32_t drawGetOutputScaleFactor();
extern void drawSetFrameskipRatio(uint32_t frameskipratio);
extern void drawSetFPSCounterEnabled(bool enabled);
extern void drawSetLEDsEnabled(bool enabled);
extern void drawSetLED(int index, bool state);
extern void drawSetAllowMultipleBuffers(BOOLE allow_multiple_buffers);
extern BOOLE drawGetAllowMultipleBuffers(void);
extern void drawSetDisplayDriver(DISPLAYDRIVER displaydriver);
extern DISPLAYDRIVER drawGetDisplayDriver();
extern void drawSetGraphicsEmulationMode(GRAPHICSEMULATIONMODE graphicsemulationmode);
extern GRAPHICSEMULATIONMODE drawGetGraphicsEmulationMode();

/*===========================================================================*/
/* Which part of the Amiga screen is visible in the host buffer              */
/* Units are cylinders (lores-pixels, non-interlaced height)                 */
/*===========================================================================*/

extern void drawSetInternalClip(const draw_rect& internal_clip);
extern const draw_rect& drawGetInternalClip();
extern void drawSetOutputClip(const draw_rect& output_clip);
extern const draw_rect& drawGetOutputClip();

extern const std::pair<uint32_t, uint32_t> drawCalculateHorizontalOutputClip(uint32_t buffer_width, uint32_t buffer_scale_factor);
extern const std::pair<uint32_t, uint32_t> drawCalculateVerticalOutputClip(uint32_t buffer_height, uint32_t buffer_scale_factor);


/*===========================================================================*/
/* Where the visible part of the Amiga screen is located in the host buffer  */
/* Units are host pixels                                                     */
/*===========================================================================*/

extern uint32_t drawGetBufferCount();

extern uint32_t drawGetBufferClipLeft();
extern float drawGetBufferClipLeftAsFloat();
extern uint32_t drawGetBufferClipTop();
extern float drawGetBufferClipTopAsFloat();
extern uint32_t drawGetBufferClipWidth();
extern float drawGetBufferClipWidthAsFloat();
extern uint32_t drawGetBufferClipHeight();
extern float drawGetBufferClipHeightAsFloat();

/*===========================================================================*/
/* When switching rendering (progressive->interlaced)                        */
/*===========================================================================*/

extern void drawReinitializeRendering(void);


/*===========================================================================*/
/* Draw statistics                                                           */
/*===========================================================================*/

extern void drawStatClear(void);
extern void drawStatTimestamp(void);
extern uint32_t drawStatLast50FramesFps(void);
extern uint32_t drawStatLastFrameFps(void);
extern uint32_t drawStatSessionFps(void);


/*===========================================================================*/
/* Framebuffer pointer locking                                               */
/*===========================================================================*/

extern uint32_t drawValidateBufferPointer(uint32_t amiga_line_number);
extern void drawInvalidateBufferPointer(void);


/*===========================================================================*/
/* Standard Fellow functions                                                 */
/*===========================================================================*/

extern void drawEndOfFrame(void);
extern void drawHardReset(void);
extern void drawEmulationStart(void);
extern BOOLE drawEmulationStartPost(void);
extern void drawEmulationStop(void);
extern BOOLE drawStartup(void);
extern void drawShutdown(void);
void drawUpdateDrawmode(void);
