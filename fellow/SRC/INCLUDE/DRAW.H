#ifndef DRAW_H
#define DRAW_H

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
  ULO id;
  ULO width;
  ULO height;
  ULO bits;
  ULO refresh;
  STR name[80];
};

typedef std::list<draw_mode*> draw_mode_list;

struct draw_rect
{
  ULO left;
  ULO top;
  ULO right;
  ULO bottom;

  ULO GetWidth() const { return right - left; }
  ULO GetHeight() const { return bottom - top; }

  draw_rect(ULO clip_left, ULO clip_top, ULO clip_right, ULO clip_bottom)
    : left(clip_left), top(clip_top), right(clip_right), bottom(clip_bottom) {}

  draw_rect() : left(0), top(0), right(0), bottom(0) {}
};

struct draw_buffer_information
{
  UBY *top_ptr;
  UBY *current_ptr;
  ULO width;
  ULO height;
  ULO pitch;
  ULO bits;
  ULO redsize;
  ULO redpos;
  ULO greensize;
  ULO greenpos;
  ULO bluesize;
  ULO bluepos;
};

/*===========================================================================*/
/* Draw line routines and data                                               */
/*===========================================================================*/

typedef void (*draw_line_func)(graph_line *linedescription, ULO linelength);

extern draw_line_func draw_line_routine;
extern draw_line_func draw_line_BG_routine;
extern draw_line_func draw_line_BPL_manage_routine;
extern draw_line_func draw_line_BPL_res_routine;
extern draw_line_func draw_line_lores_routine;
extern draw_line_func draw_line_hires_routine;
extern draw_line_func draw_line_dual_hires_routine;
extern draw_line_func draw_line_dual_lores_routine;
extern draw_line_func draw_line_HAM_lores_routine;

extern ULO draw_frame_count;
extern ULO draw_color_table[4096];
extern ULO draw_buffer_draw;                 /* Number of the current drawing buffer */

extern LON draw_frame_skip;
extern ULO draw_switch_bg_to_bpl;
extern ULO draw_clear_buffers;

extern draw_buffer_information draw_buffer_info;
extern draw_mode draw_mode_windowed;

extern ULL drawMake64BitColorFrom32Bit(ULO color);

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawAddMode(draw_mode *modenode);
extern void drawClearModeList();
extern draw_mode_list& drawGetModes();

extern void drawSetFullScreenMode(ULO width, ULO height, ULO colorbits, ULO refresh);
extern void drawSetWindowedMode(ULO width, ULO height);
extern void drawSetDisplayScale(DISPLAYSCALE displayscale);
extern DISPLAYSCALE drawGetDisplayScale(void);
extern void drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy);
extern DISPLAYSCALE_STRATEGY drawGetDisplayScaleStrategy(void);
extern ULO drawGetInternalScaleFactor();
extern ULO drawGetOutputScaleFactor();
extern void drawSetFrameskipRatio(ULO frameskipratio);
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

//extern ULO drawGetClipWidth();

//extern void drawSetClipScroll(ULO event_id);

extern const std::pair<ULO, ULO> drawCalculateHorizontalOutputClip(ULO buffer_width, ULO buffer_scale_factor);
extern const std::pair<ULO, ULO> drawCalculateVerticalOutputClip(ULO buffer_height, ULO buffer_scale_factor);


/*===========================================================================*/
/* Where the visible part of the Amiga screen is located in the host buffer  */
/* Units are host pixels                                                     */
/*===========================================================================*/

extern ULO drawGetBufferCount();

extern ULO drawGetBufferClipLeft();
extern float drawGetBufferClipLeftAsFloat();
extern ULO drawGetBufferClipTop();
extern float drawGetBufferClipTopAsFloat();
extern ULO drawGetBufferClipWidth();
extern float drawGetBufferClipWidthAsFloat();
extern ULO drawGetBufferClipHeight();
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
extern ULO drawStatLast50FramesFps(void);
extern ULO drawStatLastFrameFps(void);
extern ULO drawStatSessionFps(void);


/*===========================================================================*/
/* Framebuffer pointer locking                                               */
/*===========================================================================*/

extern ULO drawValidateBufferPointer(ULO amiga_line_number);
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

#endif
