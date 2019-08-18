#pragma once

#include <cstdint>

/* C declarations */

extern void graphLineDescClear();
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

extern void graphNotifyBplCon0Changed();
extern void graphNotifyBplCon1Changed();
extern void graphNotifyDDFStrtChanged();
extern void graphNotifyDDFStopChanged();
extern void graphNotifyDIWStrtChanged();
extern void graphNotifyDIWStopChanged();

/* IO register read and write */

UWO rdmaconr(ULO address);
void wdmacon(UWO data, ULO address);

extern planar2chunkyroutine graph_decode_line_ptr;
extern planar2chunkyroutine graph_decode_line_tab[16];
extern planar2chunkyroutine graph_decode_line_dual_tab[16];

extern ULO dmaconr;
extern ULO dmacon;
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

extern bool graph_playfield_on;

enum class graph_linetypes
{
  GRAPH_LINE_BG = 0,
  GRAPH_LINE_BPL = 1,
  GRAPH_LINE_SKIP = 2,
  GRAPH_LINE_BPL_SKIP = 3
};

struct graph_line;
typedef void (*draw_line_func)(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t amigaLinePitch_312p);
typedef uint8_t *(*draw_line_BPL_segment_func)(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t amigaLinePitch_312p);

struct graph_line
{
  graph_linetypes linetype;
  UBY line1[1024];
  UBY line2[1024];
  ULL colors[64];
  ULO DIW_first_draw;
  ULO DIW_pixel_count;
  ULO BG_pad_front;
  ULO BG_pad_back;
  draw_line_func draw_line_routine;
  draw_line_BPL_segment_func draw_line_BPL_res_routine;
  ULO DDF_start;
  ULO frames_left_until_BG_skip;
  ULO sprite_ham_slot;
  ULO bplcon2;
  uint32_t MaxClipWidth;
  bool has_ham_sprites_online;
};

graph_line *graphGetLineDesc(int buffer_no, unsigned int currentY);

extern graph_line graph_frame[3][628];
extern bool graph_buffer_lost;
