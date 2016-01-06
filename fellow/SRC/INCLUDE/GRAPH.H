#ifndef GRAPH_H
#define GRAPH_H

/* C declarations */

extern void graphLineDescClear(void);
extern void graphInitializeShadowColors(void);
extern void graphHardReset(void);
extern void graphEmulationStart(void);
extern void graphEmulationStop(void);
extern void graphStartup(void);
extern void graphShutdown(void);
extern void graphCalculateWindow(void);
extern void graphCalculateWindowHires(void);
extern void graphEndOfLine(void);
extern void graphEndOfFrame(void);
extern void graphPlayfieldOnOff(void);
extern bool graphIsPal(void);

/* IO register read and write */

UWO rdmaconr(ULO address);
UWO rvposr(ULO address);
UWO rvhposr(ULO address);
UWO rid(ULO address);

void wvpos(UWO data, ULO address);
void wdiwstrt(UWO data, ULO address);
void wdiwstop(UWO data, ULO address);
void wserdat(UWO data, ULO address);
void wserper(UWO data, ULO address);
void wddfstrt(UWO data, ULO address);
void wddfstop(UWO data, ULO address);
void wdmacon(UWO data, ULO address);

void wbpl1pth(UWO data, ULO address);
void wbpl1ptl(UWO data, ULO address);
void wbpl2pth(UWO data, ULO address);
void wbpl2ptl(UWO data, ULO address);
void wbpl3pth(UWO data, ULO address);
void wbpl3ptl(UWO data, ULO address);
void wbpl4pth(UWO data, ULO address);
void wbpl4ptl(UWO data, ULO address);
void wbpl5pth(UWO data, ULO address);
void wbpl5ptl(UWO data, ULO address);
void wbpl6pth(UWO data, ULO address);
void wbpl6ptl(UWO data, ULO address);
void wbplcon0(UWO data, ULO address);
void wbplcon1(UWO data, ULO address);
void wbplcon2(UWO data, ULO address);
void wbpl1mod(UWO data, ULO address);
void wbpl2mod(UWO data, ULO address);
void wcolor(UWO data, ULO address);

extern planar2chunkyroutine graph_decode_line_ptr;
extern planar2chunkyroutine graph_decode_line_tab[16];
extern planar2chunkyroutine graph_decode_line_dual_tab[16];

extern ULO dmaconr;
extern ULO graph_color_shadow[];

extern ULO diwstrt;
extern ULO diwstop;
extern ULO ddfstrt;
extern ULO ddfstop;
extern ULO bpl1pt;
extern ULO bpl2pt;
extern ULO bpl3pt;
extern ULO bpl4pt;
extern ULO bpl5pt;
extern ULO bpl6pt;
extern ULO bplcon0;
extern ULO bplcon1;
extern ULO bplcon2;
extern ULO bpl1mod;
extern ULO bpl2mod;
extern ULO dmacon;
extern ULO lof;
extern ULO evenscroll;
extern ULO evenhiscroll;
extern ULO oddscroll;
extern ULO oddhiscroll;
extern ULO graph_DDF_start;
extern ULO graph_DDF_word_count;
extern ULO graph_DIW_first_visible;
extern ULO graph_DIW_last_visible;
extern ULO diwxleft;
extern ULO diwxright;
extern ULO diwytop;
extern ULO diwybottom;

extern BOOLE graph_playfield_on;

typedef enum {
  GRAPH_LINE_BG = 0,
  GRAPH_LINE_BPL = 1,
  GRAPH_LINE_SKIP = 2,
  GRAPH_LINE_BPL_SKIP = 3
} graph_linetypes;

typedef struct {
  graph_linetypes linetype;
  UBY line1[1024];
  UBY line2[1024];
  ULO colors[64];
  ULO DIW_first_draw;
  ULO DIW_pixel_count;
  ULO BG_pad_front;
  ULO BG_pad_back;
  void *draw_line_routine;  /* Actually of type draw_line_func, circular definition */
  void *draw_line_BPL_res_routine; /* Ditto */
  ULO DDF_start;
  ULO frames_left_until_BG_skip;
  ULO sprite_ham_slot;
  ULO bplcon2;
} graph_line;


graph_line* graphGetLineDesc(int buffer_no, int currentY);

extern graph_line graph_frame[3][628];
extern BOOLE graph_buffer_lost;

#define graph_line_type                          0
#define graph_line_line1                         4
#define graph_line_line2                      1028
#define graph_line_colors                     2052
#define graph_line_DIW_first_draw             2308
#define graph_line_DIW_pixel_count            2312
#define graph_line_BG_pad_front               2316
#define graph_line_BG_pad_back                2320
#define graph_line_draw_routine               2324
#define graph_line_draw_BPL_res_routine       2328
#define graph_line_DDF_start                  2332
#define graph_line_frames_left_until_BG_skip  2336
#define graph_line_sprite_ham_slot            2340
#define graph_line_bplcon2                    2344
#define graph_line_reserved1                  2346
#define graph_line_end                        2348

#endif
