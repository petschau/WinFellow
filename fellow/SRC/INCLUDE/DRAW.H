#ifndef DRAW_H
#define DRAW_H

#ifndef LISTTREE_H
#include "listtree.h"
#endif

#ifndef GRAPH_H
#include "graph.h"
#endif

/*===========================================================================*/
/* Import from assembly, function renaming                                   */
/*===========================================================================*/

#define drawStart ASMRENAME(drawStart)         /* Code alignment check label */

#define drawLineBG1x8     ASMRENAME(drawLineBG1x8)
#define drawLineBG2x8     ASMRENAME(drawLineBG2x8)
#define drawLineBG1x16    ASMRENAME(drawLineBG1x16)
#define drawLineBG2x16    ASMRENAME(drawLineBG2x16)
#define drawLineBG1x24	  ASMRENAME(drawLineBG1x24)
#define drawLineBG2x24    ASMRENAME(drawLineBG2x24)
#define drawLineBG1x32    ASMRENAME(drawLineBG1x32)
#define drawLineBG2x32    ASMRENAME(drawLineBG2x32)

#define drawLineBG1x8mmx     ASMRENAME(drawLineBG1x8mmx)
#define drawLineBG2x8mmx     ASMRENAME(drawLineBG2x8mmx)
#define drawLineBG1x16mmx    ASMRENAME(drawLineBG1x16mmx)
#define drawLineBG2x16mmx    ASMRENAME(drawLineBG2x16mmx)
#define drawLineBG1x24mmx	  ASMRENAME(drawLineBG1x24mmx)
#define drawLineBG2x24mmx    ASMRENAME(drawLineBG2x24mmx)
#define drawLineBG1x32mmx    ASMRENAME(drawLineBG1x32mmx)
#define drawLineBG2x32mmx    ASMRENAME(drawLineBG2x32mmx)

#define drawLineBPL1x8     ASMRENAME(drawLineBPL1x8)
#define drawLineBPL2x8     ASMRENAME(drawLineBPL2x8)
#define drawLineBPL1x16    ASMRENAME(drawLineBPL1x16)
#define drawLineBPL2x16	   ASMRENAME(drawLineBPL2x16)
#define drawLineBPL1x24    ASMRENAME(drawLineBPL1x24)
#define drawLineBPL2x24    ASMRENAME(drawLineBPL2x24)
#define drawLineBPL1x32    ASMRENAME(drawLineBPL1x32)
#define drawLineBPL2x32	   ASMRENAME(drawLineBPL2x32)

#define drawLineBPL1x8mmx     ASMRENAME(drawLineBPL1x8mmx)
#define drawLineBPL2x8mmx     ASMRENAME(drawLineBPL2x8mmx)
#define drawLineBPL1x16mmx    ASMRENAME(drawLineBPL1x16mmx)
#define drawLineBPL2x16mmx	   ASMRENAME(drawLineBPL2x16mmx)
#define drawLineBPL1x24mmx    ASMRENAME(drawLineBPL1x24mmx)
#define drawLineBPL2x24mmx    ASMRENAME(drawLineBPL2x24mmx)
#define drawLineBPL1x32mmx    ASMRENAME(drawLineBPL1x32mmx)
#define drawLineBPL2x32mmx	   ASMRENAME(drawLineBPL2x32mmx)

#define drawLineNormal1x8     ASMRENAME(drawLineNormal1x8)
#define drawLineNormal2x8     ASMRENAME(drawLineNormal2x8)
#define drawLineNormal1x16    ASMRENAME(drawLineNormal1x16)
#define drawLineNormal2x16    ASMRENAME(drawLineNormal2x16)
#define drawLineNormal1x24    ASMRENAME(drawLineNormal1x24)
#define drawLineNormal2x24    ASMRENAME(drawLineNormal2x24)
#define drawLineNormal1x32    ASMRENAME(drawLineNormal1x32)
#define drawLineNormal2x32    ASMRENAME(drawLineNormal2x32)

#define drawLineNormal1x8mmx     ASMRENAME(drawLineNormal1x8mmx)
#define drawLineNormal2x8mmx     ASMRENAME(drawLineNormal2x8mmx)
#define drawLineNormal1x16mmx    ASMRENAME(drawLineNormal1x16mmx)
#define drawLineNormal2x16mmx    ASMRENAME(drawLineNormal2x16mmx)
#define drawLineNormal1x24mmx    ASMRENAME(drawLineNormal1x24mmx)
#define drawLineNormal2x24mmx    ASMRENAME(drawLineNormal2x24mmx)
#define drawLineNormal1x32mmx    ASMRENAME(drawLineNormal1x32mmx)
#define drawLineNormal2x32mmx    ASMRENAME(drawLineNormal2x32mmx)

#define drawLineDual1x8     ASMRENAME(drawLineDual1x8)
#define drawLineDual2x8     ASMRENAME(drawLineDual2x8)
#define drawLineDual1x16    ASMRENAME(drawLineDual1x16)
#define drawLineDual2x16    ASMRENAME(drawLineDual2x16)
#define drawLineDual1x24    ASMRENAME(drawLineDual1x24)
#define drawLineDual2x24    ASMRENAME(drawLineDual2x24)
#define drawLineDual1x32    ASMRENAME(drawLineDual1x32)
#define drawLineDual2x32    ASMRENAME(drawLineDual2x32)

#define drawLineDual1x8mmx     ASMRENAME(drawLineDual1x8mmx)
#define drawLineDual2x8mmx     ASMRENAME(drawLineDual2x8mmx)
#define drawLineDual1x16mmx    ASMRENAME(drawLineDual1x16mmx)
#define drawLineDual2x16mmx    ASMRENAME(drawLineDual2x16mmx)
#define drawLineDual1x24mmx    ASMRENAME(drawLineDual1x24mmx)
#define drawLineDual2x24mmx    ASMRENAME(drawLineDual2x24mmx)
#define drawLineDual1x32mmx    ASMRENAME(drawLineDual1x32mmx)
#define drawLineDual2x32mmx    ASMRENAME(drawLineDual2x32mmx)

#define drawLineHAM1x8  ASMRENAME(drawLineHAM1x8)
#define drawLineHAM2x8  ASMRENAME(drawLineHAM2x8)
#define drawLineHAM1x16 ASMRENAME(drawLineHAM1x16)
#define drawLineHAM2x16 ASMRENAME(drawLineHAM2x16)
#define drawLineHAM1x24 ASMRENAME(drawLineHAM1x24)
#define drawLineHAM2x24 ASMRENAME(drawLineHAM2x24)
#define drawLineHAM1x32 ASMRENAME(drawLineHAM1x32)
#define drawLineHAM2x32 ASMRENAME(drawLineHAM2x32)


/*===========================================================================*/
/* Assembly function declarations                                            */
/* These functions have their address taken in C                             */
/*===========================================================================*/

extern void drawStart(void);                   /* Code alignment check label */

extern void drawLineBG1x8(graph_line *linedesc);
extern void drawLineBG2x8(graph_line *linedesc);
extern void drawLineBG1x16(graph_line *linedesc);
extern void drawLineBG2x16(graph_line *linedesc);
extern void drawLineBG1x24(graph_line *linedesc);
extern void drawLineBG2x24(graph_line *linedesc);
extern void drawLineBG1x32(graph_line *linedesc);
extern void drawLineBG2x32(graph_line *linedesc);

extern void drawLineBG1x8mmx(graph_line *linedesc);
extern void drawLineBG2x8mmx(graph_line *linedesc);
extern void drawLineBG1x16mmx(graph_line *linedesc);
extern void drawLineBG2x16mmx(graph_line *linedesc);
extern void drawLineBG1x24mmx(graph_line *linedesc);
extern void drawLineBG2x24mmx(graph_line *linedesc);
extern void drawLineBG1x32mmx(graph_line *linedesc);
extern void drawLineBG2x32mmx(graph_line *linedesc);

extern void drawLineBPL1x8(graph_line *linedesc);
extern void drawLineBPL2x8(graph_line *linedesc);
extern void drawLineBPL1x16(graph_line *linedesc);
extern void drawLineBPL2x16(graph_line *linedesc);
extern void drawLineBPL1x24(graph_line *linedesc);
extern void drawLineBPL2x24(graph_line *linedesc);
extern void drawLineBPL1x32(graph_line *linedesc);
extern void drawLineBPL2x32(graph_line *linedesc);

extern void drawLineBPL1x8mmx(graph_line *linedesc);
extern void drawLineBPL2x8mmx(graph_line *linedesc);
extern void drawLineBPL1x16mmx(graph_line *linedesc);
extern void drawLineBPL2x16mmx(graph_line *linedesc);
extern void drawLineBPL1x24mmx(graph_line *linedesc);
extern void drawLineBPL2x24mmx(graph_line *linedesc);
extern void drawLineBPL1x32mmx(graph_line *linedesc);
extern void drawLineBPL2x32mmx(graph_line *linedesc);

extern void drawLineNormal1x8(graph_line *linedesc);
extern void drawLineNormal2x8(graph_line *linedesc);
extern void drawLineNormal1x16(graph_line *linedesc);
extern void drawLineNormal2x16(graph_line *linedesc);
extern void drawLineNormal1x24(graph_line *linedesc);
extern void drawLineNormal2x24(graph_line *linedesc);
extern void drawLineNormal1x32(graph_line *linedesc);
extern void drawLineNormal2x32(graph_line *linedesc);

extern void drawLineNormal1x8mmx(graph_line *linedesc);
extern void drawLineNormal2x8mmx(graph_line *linedesc);
extern void drawLineNormal1x16mmx(graph_line *linedesc);
extern void drawLineNormal2x16mmx(graph_line *linedesc);
extern void drawLineNormal1x24mmx(graph_line *linedesc);
extern void drawLineNormal2x24mmx(graph_line *linedesc);
extern void drawLineNormal1x32mmx(graph_line *linedesc);
extern void drawLineNormal2x32mmx(graph_line *linedesc);

extern void drawLineDual1x8(graph_line *linedesc);
extern void drawLineDual2x8(graph_line *linedesc);
extern void drawLineDual1x16(graph_line *linedesc);
extern void drawLineDual2x16(graph_line *linedesc);
extern void drawLineDual1x24(graph_line *linedesc);
extern void drawLineDual2x24(graph_line *linedesc);
extern void drawLineDual1x32(graph_line *linedesc);
extern void drawLineDual2x32(graph_line *linedesc);

extern void drawLineDual1x8mmx(graph_line *linedesc);
extern void drawLineDual2x8mmx(graph_line *linedesc);
extern void drawLineDual1x16mmx(graph_line *linedesc);
extern void drawLineDual2x16mmx(graph_line *linedesc);
extern void drawLineDual1x24mmx(graph_line *linedesc);
extern void drawLineDual2x24mmx(graph_line *linedesc);
extern void drawLineDual1x32mmx(graph_line *linedesc);
extern void drawLineDual2x32mmx(graph_line *linedesc);

extern void drawLineHAM1x8(graph_line *linedesc);
extern void drawLineHAM2x8(graph_line *linedesc);
extern void drawLineHAM1x16(graph_line *linedesc);
extern void drawLineHAM2x16(graph_line *linedesc);
extern void drawLineHAM1x24(graph_line *linedesc);
extern void drawLineHAM2x24(graph_line *linedesc);
extern void drawLineHAM1x32(graph_line *linedesc);
extern void drawLineHAM2x32(graph_line *linedesc);

void drawLineNormal1x8_C(graph_line *linedescription);
void drawLineNormal2x8_C(graph_line *linedescription);
void drawLineNormal1x16_C(graph_line *linedescription);
void drawLineNormal2x16_C(graph_line *linedescription);
void drawLineNormal1x24_C(graph_line *linedescription);
void drawLineNormal2x24_C(graph_line *linedescription);
void drawLineNormal1x32_C(graph_line *linedescription);
void drawLineNormal2x32_C(graph_line *linedescription);

void drawLineDual1x8_C(graph_line *linedescription);
void drawLineDual2x8_C(graph_line *linedescription);
void drawLineDual1x16_C(graph_line *linedescription);
void drawLineDual1x16_C_faster(graph_line *linedescription);
void drawLineDual2x16_C(graph_line *linedescription);
void drawLineDual1x24_C(graph_line *linedescription);
void drawLineDual2x24_C(graph_line *linedescription);
void drawLineDual1x32_C(graph_line *linedescription);
void drawLineDual2x32_C(graph_line *linedescription);

void drawLineBPL1x8_C(graph_line *linedesc);
void drawLineBPL2x8_C(graph_line *linedesc);
void drawLineBPL1x16_C(graph_line *linedesc);
void drawLineBPL2x16_C(graph_line *linedesc);
void drawLineBPL1x24_C(graph_line *linedesc);
void drawLineBPL2x24_C(graph_line *linedesc);
void drawLineBPL1x32_C(graph_line *linedesc);
void drawLineBPL2x32_C(graph_line *linedesc);

void drawLineBPL1x8mmx(graph_line *linedesc);
void drawLineBPL2x8mmx(graph_line *linedesc);
void drawLineBPL1x16mmx(graph_line *linedesc);
void drawLineBPL2x16mmx(graph_line *linedesc);
void drawLineBPL1x24mmx(graph_line *linedesc);
void drawLineBPL2x24mmx(graph_line *linedesc);
void drawLineBPL1x32mmx(graph_line *linedesc);
void drawLineBPL2x32mmx(graph_line *linedesc);

void drawLineSegmentBG1x8(ULO topad, ULO bgcolor);
void drawLineSegmentBG2x8(ULO topad, ULO bgcolor);
void drawLineSegmentBG1x16(ULO topad, ULO bgcolor);
void drawLineSegmentBG2x16(ULO topad, ULO bgcolor);
void drawLineSegmentBG1x24(ULO topad, ULO bgcolor);
void drawLineSegmentBG2x24(ULO topad, ULO bgcolor);
void drawLineSegmentBG1x32(ULO topad, ULO bgcolor);
void drawLineSegmentBG2x32(ULO topad, ULO bgcolor);

void drawLineBG1x8_C(graph_line *linedesc);
void drawLineBG2x8_C(graph_line *linedesc);
void drawLineBG1x16_C(graph_line *linedesc);
void drawLineBG2x16_C(graph_line *linedesc);
void drawLineBG1x24_C(graph_line *linedesc);
void drawLineBG2x24_C(graph_line *linedesc);
void drawLineBG1x32_C(graph_line *linedesc);
void drawLineBG2x32_C(graph_line *linedesc);

void drawLineHAM1x8_C(graph_line *linedesc);
void drawLineHAM2x8_C(graph_line *linedesc);
void drawLineHAM1x16_C(graph_line *linedesc);
void drawLineHAM2x16_C(graph_line *linedesc);
void drawLineHAM1x24_C(graph_line *linedesc);
void drawLineHAM2x24_C(graph_line *linedesc);
void drawLineHAM1x32_C(graph_line *linedesc);
void drawLineHAM2x32_C(graph_line *linedesc);

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

typedef void (*draw_line_func)(graph_line *linedesc);

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

extern ULO draw_frame_skip;
extern ULO draw_switch_bg_to_bpl;

extern ULO draw_hscale;
extern BOOLE draw_deinterlace;

/*===========================================================================*/
/* Module properties                                                         */
/*===========================================================================*/

extern void drawModeAdd(draw_mode *modenode);
extern BOOLE drawSetMode(ULO width,
			 ULO height, 
			 ULO colorbits, 
			 ULO refresh,
			 BOOLE windowed);
extern felist *drawGetModes(void);
extern void drawSetDeinterlace(BOOLE deinterlace);
extern void drawSetHorizontalScale(ULO horisontalscale);
extern void drawSetVerticalScale(ULO verticalscale);
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
