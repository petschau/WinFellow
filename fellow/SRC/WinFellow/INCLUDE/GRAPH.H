#pragma once

/* C declarations */

extern void graphLineDescClear();
extern void graphInitializeShadowColors();
extern void graphHardReset();
extern void graphEmulationStart();
extern void graphEmulationStop();
extern void graphStartup();
extern void graphShutdown();
extern void graphCalculateWindow();
extern void graphCalculateWindowHires();
extern void graphEndOfLine();
extern void graphEndOfFrame();
extern void graphPlayfieldOnOff();

/* IO register read and write */

uint16_t rdmaconr(uint32_t address);
uint16_t rvposr(uint32_t address);
uint16_t rvhposr(uint32_t address);
uint16_t rid(uint32_t address);

void wvpos(uint16_t data, uint32_t address);
void wdiwstrt(uint16_t data, uint32_t address);
void wdiwstop(uint16_t data, uint32_t address);
void wddfstrt(uint16_t data, uint32_t address);
void wddfstop(uint16_t data, uint32_t address);
void wdmacon(uint16_t data, uint32_t address);

void wbpl1pth(uint16_t data, uint32_t address);
void wbpl1ptl(uint16_t data, uint32_t address);
void wbpl2pth(uint16_t data, uint32_t address);
void wbpl2ptl(uint16_t data, uint32_t address);
void wbpl3pth(uint16_t data, uint32_t address);
void wbpl3ptl(uint16_t data, uint32_t address);
void wbpl4pth(uint16_t data, uint32_t address);
void wbpl4ptl(uint16_t data, uint32_t address);
void wbpl5pth(uint16_t data, uint32_t address);
void wbpl5ptl(uint16_t data, uint32_t address);
void wbpl6pth(uint16_t data, uint32_t address);
void wbpl6ptl(uint16_t data, uint32_t address);
void wbplcon0(uint16_t data, uint32_t address);
void wbplcon1(uint16_t data, uint32_t address);
void wbplcon2(uint16_t data, uint32_t address);
void wbpl1mod(uint16_t data, uint32_t address);
void wbpl2mod(uint16_t data, uint32_t address);
void wcolor(uint16_t data, uint32_t address);

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

typedef enum
{
  GRAPH_LINE_BG = 0,
  GRAPH_LINE_BPL = 1,
  GRAPH_LINE_SKIP = 2,
  GRAPH_LINE_BPL_SKIP = 3
} graph_linetypes;

typedef struct
{
  graph_linetypes linetype;
  uint8_t line1[1024];
  uint8_t line2[1024];
  uint32_t colors[64];
  uint32_t DIW_first_draw;
  uint32_t DIW_pixel_count;
  uint32_t BG_pad_front;
  uint32_t BG_pad_back;
  void *draw_line_routine;         /* Actually of type draw_line_func, circular definition */
  void *draw_line_BPL_res_routine; /* Ditto */
  uint32_t DDF_start;
  uint32_t frames_left_until_BG_skip;
  uint32_t sprite_ham_slot;
  uint32_t bplcon2;
  bool has_ham_sprites_online;
} graph_line;

graph_line *graphGetLineDesc(int buffer_no, int currentY);

extern graph_line graph_frame[3][628];
extern BOOLE graph_buffer_lost;

#define graph_line_type 0
#define graph_line_line1 4
#define graph_line_line2 1028
#define graph_line_colors 2052
#define graph_line_DIW_first_draw 2308
#define graph_line_DIW_pixel_count 2312
#define graph_line_BG_pad_front 2316
#define graph_line_BG_pad_back 2320
#define graph_line_draw_routine 2324
#define graph_line_draw_BPL_res_routine 2328
#define graph_line_DDF_start 2332
#define graph_line_frames_left_until_BG_skip 2336
#define graph_line_sprite_ham_slot 2340
#define graph_line_bplcon2 2344
#define graph_line_reserved1 2346
#define graph_line_end 2348
