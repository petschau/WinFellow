#ifndef GRAPH_H
#define GRAPH_H

/* Rename assembly functions */

#define graphSpriteHack ASMRENAME(graphSpriteHack)
#define graphComposeLineOutputSmart ASMRENAME(graphComposeLineOutputSmart)
#define graphComposeLineOutput ASMRENAME(graphComposeLineOutput)
#define spritesDecode ASMRENAME(spritesDecode)
#define spritesMerge ASMRENAME(spritesMerge)
#define graphCompareCopy ASMRENAME(graphCompareCopy)
#define graphLinedescMakeSmart ASMRENAME(graphLinedescMakeSmart)

#define graphLinedescColorsSmart ASMRENAME(graphLinedescColorsSmart)
#define graphLinedescGeometrySmart ASMRENAME(graphLinedescGeometrySmart)
#define graphLinedescRoutinesSmart ASMRENAME(graphLinedescRoutinesSmart)

#define graphemstart ASMRENAME(graphemstart)
#define graphDecode0 ASMRENAME(graphDecode0)
#define graphDecode1 ASMRENAME(graphDecode1)
#define graphDecode2 ASMRENAME(graphDecode2)
#define graphDecode3 ASMRENAME(graphDecode3)
#define graphDecode4 ASMRENAME(graphDecode4)
#define graphDecode5 ASMRENAME(graphDecode5)
#define graphDecode6 ASMRENAME(graphDecode6)
#define graphDecode2Dual ASMRENAME(graphDecode2Dual)
#define graphDecode3Dual ASMRENAME(graphDecode3Dual)
#define graphDecode4Dual ASMRENAME(graphDecode4Dual)
#define graphDecode5Dual ASMRENAME(graphDecode5Dual)
#define graphDecode6Dual ASMRENAME(graphDecode6Dual)
#define graphDecode0Hi320 ASMRENAME(graphDecode0Hi320)
#define graphDecode1Hi320 ASMRENAME(graphDecode1Hi320)
#define graphDecode2Hi320 ASMRENAME(graphDecode2Hi320)
#define graphDecode3Hi320 ASMRENAME(graphDecode3Hi320)
#define graphDecode4Hi320 ASMRENAME(graphDecode4Hi320)

#define graphDecode0Hi ASMRENAME(graphDecode0Hi)
#define graphDecode1Hi ASMRENAME(graphDecode1Hi)
#define graphDecode2Hi ASMRENAME(graphDecode2Hi)
#define graphDecode3Hi ASMRENAME(graphDecode3Hi)
#define graphDecode4Hi ASMRENAME(graphDecode4Hi)


#define graphDecode2HiDual320 ASMRENAME(graphDecode2HiDual320)
#define graphDecode3HiDual320 ASMRENAME(graphDecode3HiDual320)
#define graphDecode4HiDual320 ASMRENAME(graphDecode4HiDual320)
//#define wcolor ASMRENAME(wcolor)
#define wdiwstrt ASMRENAME(wdiwstrt)
#define wdiwstop ASMRENAME(wdiwstop)
#define wddfstrt ASMRENAME(wddfstrt)
#define wddfstop ASMRENAME(wddfstop)
#define wbpl1pth ASMRENAME(wbpl1pth)
#define wbpl1ptl ASMRENAME(wbpl1ptl)
#define wbpl2pth ASMRENAME(wbpl2pth)
#define wbpl2ptl ASMRENAME(wbpl2ptl)
#define wbpl3pth ASMRENAME(wbpl3pth)
#define wbpl3ptl ASMRENAME(wbpl3ptl)
#define wbpl4pth ASMRENAME(wbpl4pth)
#define wbpl4ptl ASMRENAME(wbpl4ptl)
#define wbpl5pth ASMRENAME(wbpl5pth)
#define wbpl5ptl ASMRENAME(wbpl5ptl)
#define wbpl6pth ASMRENAME(wbpl6pth)
#define wbpl6ptl ASMRENAME(wbpl6ptl)
#define wbplcon0 ASMRENAME(wbplcon0)
#define wbplcon1 ASMRENAME(wbplcon1)
#define wbplcon2 ASMRENAME(wbplcon2)
#define wbpl1mod ASMRENAME(wbpl1mod)
#define wbpl2mod ASMRENAME(wbpl2mod)
#define wvpos ASMRENAME(wvpos)
#define rid ASMRENAME(rid)
#define rvhposr ASMRENAME(rvhposr)
#define rvposr ASMRENAME(rvposr)
#define rdmaconr ASMRENAME(rdmaconr)

/* Declare asm functions */

extern void graphemstart(void); /* Code alignment check label */
extern void graphDecode0(void);
extern void graphDecode1(void);
extern void graphDecode2(void);
extern void graphDecode3(void);
extern void graphDecode4(void);
extern void graphDecode5(void);
extern void graphDecode6(void);
extern void graphDecode2Dual(void);
extern void graphDecode3Dual(void);
extern void graphDecode4Dual(void);
extern void graphDecode5Dual(void);
extern void graphDecode6Dual(void);
extern void graphDecode0Hi320(void);
extern void graphDecode1Hi320(void);
extern void graphDecode2Hi320(void);
extern void graphDecode3Hi320(void);
extern void graphDecode4Hi320(void);

extern void graphDecode0Hi(void);
extern void graphDecode1Hi(void);
extern void graphDecode2Hi(void);
extern void graphDecode3Hi(void);
extern void graphDecode4Hi(void);

extern void graphDecode2HiDual320(void);
extern void graphDecode3HiDual320(void);
extern void graphDecode4HiDual320(void);
//extern void wcolor(ULO data, ULO address);
extern void wdiwstrt(ULO,ULO);
extern void wdiwstop(ULO,ULO);
extern void wddfstrt(ULO,ULO);
extern void wddfstop(ULO,ULO);
extern void wbpl1pth(ULO,ULO);
extern void wbpl1ptl(ULO,ULO);
extern void wbpl2pth(ULO,ULO);
extern void wbpl2ptl(ULO,ULO);
extern void wbpl3pth(ULO,ULO);
extern void wbpl3ptl(ULO,ULO);
extern void wbpl4pth(ULO,ULO);
extern void wbpl4ptl(ULO,ULO);
extern void wbpl5pth(ULO,ULO);
extern void wbpl5ptl(ULO,ULO);
extern void wbpl6pth(ULO,ULO);
extern void wbpl6ptl(ULO,ULO);
extern void wbplcon0(ULO data, ULO address);
extern void wbplcon1(ULO data, ULO address);
extern void wbplcon2(ULO data, ULO address);
extern void wbpl1mod(ULO data, ULO address);
extern void wbpl2mod(ULO data, ULO address);
extern void wvpos(ULO,ULO);
extern ULO rid(ULO);
extern ULO rvhposr(ULO);
extern ULO rvposr(ULO);
extern ULO rdmaconr(ULO);

extern void graphSpriteHack(void);
extern void graphComposeLineOutputSmart(void);
extern void graphComposeLineOutput(void);
extern void spritesDecode(void);
extern void spritesMerge(void);
extern void graphCompareCopy(void);
extern void graphLinedescMakeSmart(void);

extern void graphLinedescColorsSmart(void);
extern void graphLinedescGeometrySmart(void);
extern void graphLinedescRoutinesSmart(void);

/* C declarations */

extern void wsprxpos(ULO,ULO);
extern void wsprxctl(ULO,ULO);
extern void wsprxdata(ULO,ULO);
extern void wsprxdatb(ULO,ULO);

extern void graphSetAllowBplLineSkip(BOOLE allow_bpl_line_skip);
extern BOOLE graphGetAllowBplLineSkip(void);

extern void graphP2C1XInit(void);
extern void graphP2C2XInit(void);
extern void graphLineDescClear(void);
extern void graphLineDescClearSkips(void);
extern void graphInitializeShadowColors(void);
extern void graphHardReset(void);
extern void graphEmulationStart(void);
extern void graphEmulationStop(void);
extern void graphStartup(void);
extern void graphShutdown(void);
void graphCalculateWindow_C(void);
void graphCalculateWindowHires_C(void);
void graphEndOfLine_C(void);
extern void graphPlayfieldOnOff(void);

void graphDecode0_C(void);
void graphDecode1_C(void);
void graphDecode2_C(void);
void graphDecode3_C(void);
void graphDecode4_C(void);
void graphDecode5_C(void);
void graphDecode6_C(void);
void graphDecode2Dual_C(void);
void graphDecode3Dual_C(void);
void graphDecode4Dual_C(void);
void graphDecode5Dual_C(void);
void graphDecode6Dual_C(void);
void graphDecodeHi0_C(void);
void graphDecodeHi1_C(void);
void graphDecodeHi2_C(void);
void graphDecodeHi3_C(void);
void graphDecodeHi4_C(void);
void graphDecodeHi5_C(void);
void graphDecodeHi6_C(void);
void graphDecodeHi2Dual_C(void);
void graphDecodeHi3Dual_C(void);
void graphDecodeHi4Dual_C(void);


ULO rdmaconr_C(ULO address);
ULO rvposr_C(ULO address);
ULO rvhposr_C(ULO address);
ULO rid_C(ULO address);



void wvpos_C(ULO data, ULO address);
void wdiwstrt_C(ULO data, ULO address);
void wdiwstop_C(ULO data, ULO address);
void wserdat_C(ULO data, ULO address);
void wserper_C(ULO data, ULO address);
void wddfstrt_C(ULO data, ULO address);
void wddfstop_C(ULO data, ULO address);
void wdmacon_C(ULO data, ULO address);

void wbpl1pth_C(ULO data, ULO address);
void wbpl1ptl_C(ULO data, ULO address);
void wbpl2pth_C(ULO data, ULO address);
void wbpl2ptl_C(ULO data, ULO address);
void wbpl3pth_C(ULO data, ULO address);
void wbpl3ptl_C(ULO data, ULO address);
void wbpl4pth_C(ULO data, ULO address);
void wbpl4ptl_C(ULO data, ULO address);
void wbpl5pth_C(ULO data, ULO address);
void wbpl5ptl_C(ULO data, ULO address);
void wbpl6pth_C(ULO data, ULO address);
void wbpl6ptl_C(ULO data, ULO address);
void wbplcon0_C(ULO data, ULO address);
void wbplcon1_C(ULO data, ULO address);
void wbplcon2_C(ULO data, ULO address);
void wbpl1mod_C(ULO data, ULO address);
void wbpl2mod_C(ULO data, ULO address);
void wcolor_C(ULO data, ULO address);

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
extern ULO graph_raster_x;
extern ULO graph_raster_y;
extern ULO diwxleft;
extern ULO diwxright;
extern ULO diwytop;
extern ULO diwybottom;

extern ULO sprites_online;

extern UBY graph_line1_tmp[1024];
extern UBY graph_line2_tmp[1024];

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

extern graph_line graph_frame[3][314];
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


void graphLinedescRoutines_C(graph_line* current_graph_line);
void graphLinedescColors_C(graph_line* current_graph_line);
void graphLinedescGeometry_C(graph_line* current_graph_line);
void graphComposeLineOutput_C(graph_line* current_graph_line);

BOOLE graphLinedescRoutinesSmart_C(graph_line* current_graph_line);
BOOLE graphLinedescColorsSmart_C(graph_line* current_graph_line);
BOOLE graphLinedescGeometrySmart_C(graph_line* current_graph_line);
BOOLE graphLinedescMakeSmart_C(graph_line* current_graph_line);
void graphComposeLineOutputSmart_C(graph_line* current_graph_line);
BOOLE graphCompareCopy_C(ULO first_pixel, LON pixel_count, ULO* dest_line, ULO* source_line);
void graphSpriteHack_C(void);

#endif
