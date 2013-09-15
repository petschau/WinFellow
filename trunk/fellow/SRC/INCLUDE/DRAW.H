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

typedef struct {
  ULO id;
  ULO width;
  ULO height;
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

extern ULO draw_hoffset;
extern ULO draw_voffset;
extern ULO draw_width_amiga_real;
extern ULO draw_height_amiga_real;
extern ULO draw_width_amiga;
extern ULO draw_height_amiga;
extern ULO draw_frame_count;
extern ULO draw_view_scroll;
extern ULO draw_color_table[4096];
extern ULO draw_left;
extern ULO draw_right;
extern ULO draw_top;
extern ULO draw_bottom;
extern ULO draw_buffer_draw;                 /* Number of the current drawing buffer */

extern LON draw_frame_skip;
extern ULO draw_switch_bg_to_bpl;

extern UBY *draw_buffer_current_ptr;

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawModeAdd(draw_mode *modenode);
extern BOOLE drawSetMode(ULO width, ULO height, ULO colorbits, ULO refresh, BOOLE windowed);
extern felist *drawGetModes(void);
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
extern bool drawGetFrameIsInterlaced(void);

/*===========================================================================*/
/* When switching rendering (progressive->interlaced)                        */
/*===========================================================================*/

extern void drawReintitializeRendering(void);


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
