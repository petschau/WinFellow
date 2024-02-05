#pragma once

#include <cstdint>

enum class graph_linetypes
{
  GRAPH_LINE_BG = 0,
  GRAPH_LINE_BPL = 1,
  GRAPH_LINE_SKIP = 2,
  GRAPH_LINE_BPL_SKIP = 3
};

struct graph_line
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
};

extern graph_line graph_frame[3][628];
