#ifndef GRAPH_H
#define GRAPH_H

/* Rename assembly functions */

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
#define graphDecode2HiDual320 ASMRENAME(graphDecode2HiDual320)
#define graphDecode3HiDual320 ASMRENAME(graphDecode3HiDual320)
#define graphDecode4HiDual320 ASMRENAME(graphDecode4HiDual320)
#define wcolor ASMRENAME(wcolor)
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

#define graphDecodeNOP0 ASMRENAME(graphDecodeNOP0)
#define graphDecodeNOP1 ASMRENAME(graphDecodeNOP1)
#define graphDecodeNOP2 ASMRENAME(graphDecodeNOP2)
#define graphDecodeNOP3 ASMRENAME(graphDecodeNOP3)
#define graphDecodeNOP4 ASMRENAME(graphDecodeNOP4)
#define graphDecodeNOP5 ASMRENAME(graphDecodeNOP5)
#define graphDecodeNOP6 ASMRENAME(graphDecodeNOP6)

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
extern void graphDecode2HiDual320(void);
extern void graphDecode3HiDual320(void);
extern void graphDecode4HiDual320(void);
extern void wcolor(ULO data, ULO address);
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

extern void graphDecodeNOP0(void);
extern void graphDecodeNOP1(void);
extern void graphDecodeNOP2(void);
extern void graphDecodeNOP3(void);
extern void graphDecodeNOP4(void);
extern void graphDecodeNOP5(void);
extern void graphDecodeNOP6(void);

/* C declarations */

extern void wsprxpos(ULO,ULO);
extern void wsprxctl(ULO,ULO);
extern void wsprxdata(ULO,ULO);
extern void wsprxdatb(ULO,ULO);

extern void graphP2C1XInit(void);
extern void graphP2C2XInit(void);
extern void graphLineDescClear(void);
extern void graphLineDescClearSkips(void);
extern void graphInitializeShadowColors(void);
extern void graphEmulationStart(void);
extern void graphEmulationStop(void);
extern void graphStartup(void);
extern void graphShutdown(void);

extern planar2chunkyroutine graph_decode_line_ptr;
extern planar2chunkyroutine graph_decode_line_tab[16];
extern planar2chunkyroutine graph_decode_line_dual_tab[16];

extern ULO graph_DDF_start;
extern ULO graph_DDF_word_count;
extern ULO graph_DIW_first_visible;
extern ULO graph_DIW_last_visible;
extern ULO bplcon0;
extern ULO bplcon2;
extern ULO graph_raster_x;
extern ULO graph_raster_y;
extern ULO dmacon;
extern ULO dmaconr;
extern ULO lof;
extern ULO graph_color_shadow[];

typedef enum {
  GRAPH_LINE_BG = 0,
  GRAPH_LINE_BPL = 1,
  GRAPH_LINE_SKIP = 2
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

#endif
