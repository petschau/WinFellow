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

/* IO register read and write */

UWO rdmaconr(uint32_t address);
UWO rvposr(uint32_t address);
UWO rvhposr(uint32_t address);
UWO rid(uint32_t address);

void wvpos(UWO data, uint32_t address);
void wdiwstrt(UWO data, uint32_t address);
void wdiwstop(UWO data, uint32_t address);
void wddfstrt(UWO data, uint32_t address);
void wddfstop(UWO data, uint32_t address);
void wdmacon(UWO data, uint32_t address);

void wbpl1pth(UWO data, uint32_t address);
void wbpl1ptl(UWO data, uint32_t address);
void wbpl2pth(UWO data, uint32_t address);
void wbpl2ptl(UWO data, uint32_t address);
void wbpl3pth(UWO data, uint32_t address);
void wbpl3ptl(UWO data, uint32_t address);
void wbpl4pth(UWO data, uint32_t address);
void wbpl4ptl(UWO data, uint32_t address);
void wbpl5pth(UWO data, uint32_t address);
void wbpl5ptl(UWO data, uint32_t address);
void wbpl6pth(UWO data, uint32_t address);
void wbpl6ptl(UWO data, uint32_t address);
void wbplcon0(UWO data, uint32_t address);
void wbplcon1(UWO data, uint32_t address);
void wbplcon2(UWO data, uint32_t address);
void wbpl1mod(UWO data, uint32_t address);
void wbpl2mod(UWO data, uint32_t address);
void wcolor(UWO data, uint32_t address);

extern planar2chunkyroutine graph_decode_line_ptr;
extern planar2chunkyroutine graph_decode_line_tab[16];
extern planar2chunkyroutine graph_decode_line_dual_tab[16];

extern uint32_t graph_color_shadow[];

extern uint32_t diwstrt;
extern uint32_t diwstop;
extern uint32_t ddfstrt;
extern uint32_t ddfstop;
extern uint32_t bpl1pt;
extern uint32_t bpl2pt;
extern uint32_t bpl3pt;
extern uint32_t bpl4pt;
extern uint32_t bpl5pt;
extern uint32_t bpl6pt;
extern uint32_t bplcon1;
extern uint32_t bpl1mod;
extern uint32_t bpl2mod;
extern uint32_t dmacon;
extern uint32_t lof;
extern uint32_t evenscroll;
extern uint32_t evenhiscroll;
extern uint32_t oddscroll;
extern uint32_t oddhiscroll;
extern uint32_t graph_DDF_start;
extern uint32_t graph_DDF_word_count;
extern uint32_t graph_DIW_first_visible;
extern uint32_t graph_DIW_last_visible;
extern uint32_t diwxleft;
extern uint32_t diwxright;
extern uint32_t diwytop;
extern uint32_t diwybottom;

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
  uint32_t colors[64];
  uint32_t DIW_first_draw;
  uint32_t DIW_pixel_count;
  uint32_t BG_pad_front;
  uint32_t BG_pad_back;
  void *draw_line_routine;  /* Actually of type draw_line_func, circular definition */
  void *draw_line_BPL_res_routine; /* Ditto */
  uint32_t DDF_start;
  uint32_t frames_left_until_BG_skip;
  uint32_t sprite_ham_slot;
  uint32_t bplcon2;
  bool has_ham_sprites_online;
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
