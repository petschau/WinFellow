#ifndef DRAW_H
#define DRAW_H

#ifndef LISTTREE_H
#include "listtree.h"
#endif

#ifndef GRAPH_H
#include "graph.h"
#endif

/*===========================================================================*/
/* function declarations                                                     */
/*===========================================================================*/

// drawLineNormal

void drawLineNormal1x1__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x1__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal1x2__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x2__8bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineNormal1x1_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal1x2_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x1_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x2_16bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineNormal1x1_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x1_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal1x2_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x2_24bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineNormal1x1_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x1_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal1x2_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineNormal2x2_32bit(graph_line *linedescription, ULO nextlineoffset);

// drawLineDual

void drawLineDual1x1__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x1__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual1x2__8bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x2__8bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineDual1x1_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x1_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual1x2_16bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x2_16bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineDual1x1_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x1_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual1x2_24bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x2_24bit(graph_line *linedescription, ULO nextlineoffset);

void drawLineDual1x1_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x1_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual1x2_32bit(graph_line *linedescription, ULO nextlineoffset);
void drawLineDual2x2_32bit(graph_line *linedescription, ULO nextlineoffset);

// drawLineBPL

void drawLineBPL1x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL1x2__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x2__8bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBPL1x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL1x2_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x2_16bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBPL1x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL1x2_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x2_24bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBPL1x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL1x2_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBPL2x2_32bit(graph_line *linedesc, ULO nextlineoffset);

// one line segment (normal or double) with background color

void drawLineSegmentBG1x1__8bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x1__8bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG1x2__8bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x2__8bit(ULO topad, ULO bgcolor, ULO nextlineoffset);

void drawLineSegmentBG1x1_16bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x1_16bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG1x2_16bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x2_16bit(ULO topad, ULO bgcolor, ULO nextlineoffset);

void drawLineSegmentBG1x1_24bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x1_24bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG1x2_24bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x2_24bit(ULO topad, ULO bgcolor, ULO nextlineoffset);

void drawLineSegmentBG1x1_32bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x1_32bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG1x2_32bit(ULO topad, ULO bgcolor, ULO nextlineoffset);
void drawLineSegmentBG2x2_32bit(ULO topad, ULO bgcolor, ULO nextlineoffset);

// one full line (normal or double) with background color

void drawLineBG1x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG1x2__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x2__8bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBG1x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG1x2_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x2_16bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBG1x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG1x2_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x2_24bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineBG1x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG1x2_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineBG2x2_32bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineHAM1x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x1__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM1x2__8bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x2__8bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineHAM1x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x1_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM1x2_16bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x2_16bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineHAM1x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x1_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM1x2_24bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x2_24bit(graph_line *linedesc, ULO nextlineoffset);

void drawLineHAM1x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x1_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM1x2_32bit(graph_line *linedesc, ULO nextlineoffset);
void drawLineHAM2x2_32bit(graph_line *linedesc, ULO nextlineoffset);

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

typedef void (*draw_line_func)(graph_line *linedesc, ULO linelength);

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

extern ULO draw_hscale;
extern BOOLE draw_deinterlace;

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawModeAdd(draw_mode *modenode);
extern BOOLE drawSetMode(ULO width, ULO height, ULO colorbits, ULO refresh, BOOLE windowed);
extern felist *drawGetModes(void);
extern void drawSetDeinterlace(BOOLE deinterlace);
extern void drawSetHorizontalScale(ULO horisontalscale);
extern void drawSetVerticalScale(ULO verticalscale);
extern void drawSetVerticalStretch(ULO verticalstretch);
extern void drawSetScanlines(BOOLE scanlines);
extern void drawSetFrameskipRatio(ULO frameskipratio);
extern void drawSetFPSCounterEnabled(BOOLE enabled);
extern void drawSetLEDsEnabled(BOOLE enabled);
extern void drawSetLED(ULO index, BOOLE state);
extern void drawSetAllowMultipleBuffers(BOOLE allow_multiple_buffers);
extern BOOLE drawGetAllowMultipleBuffers(void);



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

extern void drawHardReset(void);
extern void drawEmulationStart(void);
extern BOOLE drawEmulationStartPost(void);
extern void drawEmulationStop(void);
extern BOOLE drawStartup(void);
extern void drawShutdown(void);
void drawUpdateDrawmode(void);

#endif
