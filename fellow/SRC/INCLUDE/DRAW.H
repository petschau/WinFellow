#ifndef DRAW_H
#define DRAW_H

#ifndef LISTTREE_H
#include "listtree.h"
#endif

#ifndef GRAPH_H
#include "graph.h"
#endif

#include "CONFIG.H"

/*===========================================================================*/
/* Mode handling, describes the geometry of the host screen                  */
/*===========================================================================*/

typedef struct 
{
  ULO id;
  ULO width;      // Physical size of the mode in the host
  ULO height;
  ULO internal_width; // The size of the buffer that the core has access to
  ULO internal_height;
  ULO bits;
  ULO refresh;
  BOOLE windowed;
  ULO redsize;
  ULO redpos;           
  ULO greensize;
  ULO greenpos;
  ULO bluesize;
  ULO bluepos;
  ULO pitch;
  STR name[80];
} draw_mode;

typedef struct
{
  ULO x;
  ULO y;
} draw_point;

typedef struct
{
  ULO width;
  ULO height;
} draw_size;

typedef struct
{
  ULO left;
  ULO top;
  ULO right;
  ULO bottom;
} draw_rect;

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

extern UBY *draw_buffer_current_ptr;

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawModeAdd(draw_mode *modenode);
extern BOOLE drawSetMode(ULO width, ULO height, ULO colorbits, ULO refresh, BOOLE windowed);
extern felist *drawGetModes(void);
extern void drawModesFree();
extern void drawSetDisplayScale(DISPLAYSCALE displayscale);
extern DISPLAYSCALE drawGetDisplayScale(void);
extern void drawSetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy);
extern DISPLAYSCALE_STRATEGY drawGetDisplayScaleStrategy(void);
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

typedef enum 
{
  AUTOMATIC_CLIP = 0, // Calculate from screen/window
  FIXED_CLIP = 1      // Set by configuration
} DRAW_CLIP_MODE;

extern void drawSetClipMode(DRAW_CLIP_MODE clip_mode);
extern DRAW_CLIP_MODE drawGetClipMode();

extern ULO drawGetClipLeft();
extern ULO drawGetClipTop();
extern ULO drawGetClipRight();
extern ULO drawGetClipBottom();

extern void drawSetClipLeft(ULO left);
extern void drawSetClipTop(ULO top);
extern void drawSetClipRight(ULO right);
extern void drawSetClipBottom(ULO bottom);

extern ULO drawGetClipWidth();

extern void drawSetClipScroll(ULO event_id);

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
