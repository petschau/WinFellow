#pragma once

#include "fellow/application/HostRenderConfiguration.h"

void drawHAMTableInit(const GfxDrvColorBitsInformation &colorBitsInformation);
void drawModeFunctionsInitialize(const unsigned int activeBufferColorBits, const unsigned int chipsetBufferScaleFactor, DisplayScaleStrategy displayScaleStrategy);
void drawSetLineRoutine(draw_line_func drawLineFunction);

extern draw_line_func draw_line_routine;
extern draw_line_func draw_line_BG_routine;
extern draw_line_func draw_line_BPL_manage_routine;
extern draw_line_BPL_segment_func draw_line_BPL_res_routine;
extern draw_line_BPL_segment_func draw_line_lores_routine;
extern draw_line_BPL_segment_func draw_line_hires_routine;
extern draw_line_BPL_segment_func draw_line_dual_hires_routine;
extern draw_line_BPL_segment_func draw_line_dual_lores_routine;
extern draw_line_BPL_segment_func draw_line_HAM_lores_routine;

class PixelRenderers
{
private:
  constexpr unsigned int draw_HAM_modify_table_bitindex = 0;
  constexpr unsigned int draw_HAM_modify_table_holdmask = 4;
  uint32_t draw_HAM_modify_table[4][2]{};

  //=======================================================================
  // Pointers to drawing routines that handle the drawing of Amiga graphics
  // on the current host screen mode
  //=======================================================================

  draw_line_func draw_line_routine;
  draw_line_func draw_line_BG_routine;
  draw_line_func draw_line_BPL_manage_routine;
  draw_line_BPL_segment_func draw_line_BPL_res_routine;
  draw_line_BPL_segment_func draw_line_lores_routine;
  draw_line_BPL_segment_func draw_line_hires_routine;
  draw_line_BPL_segment_func draw_line_dual_lores_routine;
  draw_line_BPL_segment_func draw_line_dual_hires_routine;
  draw_line_BPL_segment_func draw_line_HAM_lores_routine;

  static uint32_t Make32BitColorFrom16Bit(uint16_t color);
  static uint64_t Make64BitColorFrom16Bit(uint16_t color);
  static uint64_t Make64BitColorFrom32Bit(uint32_t color);

  static const uint8_t *GetDualTranslatePtr(const graph_line *linedescription);
  static uint8_t GetDualColorIndex(const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value);
  static uint16_t GetDual16BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value);
  static uint32_t GetDual32BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value);
  static uint64_t GetDual64BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value);

  static uint32_t MakeHoldMask(unsigned int pos, unsigned int size, bool longdestination);
  void HAMTableInit(const GfxDrvColorBitsInformation &colorBitsInformation);
  uint32_t UpdateHAMPixel(uint32_t hampixel, uint8_t pixelValue);
  uint32_t MakeHAMPixel(const uint64_t *colors, uint32_t hampixel, uint8_t pixelValue);
  uint32_t ProcessNonVisibleHAMPixels(const graph_line *linedescription, int32_t pixelCount);
  uint32_t GetFirstHamPixelFromInitialInvisibleHAMPixels(const graph_line *linedescription);

  static void SetPixel1x1_16Bit(uint16_t *framebuffer, uint16_t pixelColor);
  static void SetPixel1x2_16Bit(uint16_t *framebuffer, ptrdiff_t nextlineoffset, uint16_t pixelColor);
  static void SetPixel2x1_16Bit(uint32_t *framebuffer, uint32_t pixelColor);
  static void SetPixel2x2_16Bit(uint32_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor);
  static void SetPixel2x4_16Bit(uint32_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor);
  static void SetPixel4x2_16Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset, uint64_t pixelColor);
  static void SetPixel4x4_16Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint64_t pixelColor);

  static uint8_t *DrawLineNormal1x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal1x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  static uint8_t *DrawLineDual1x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual1x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  uint8_t *DrawLineHAM2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  static uint8_t *DrawLineSegmentBG2x1_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG2x2_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG4x2_16Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG4x4_16Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  void DrawLineBPL2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  void DrawLineBG2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  static void SetPixel1x1_24Bit(uint8_t *framebuffer, uint32_t pixelColor);
  static void SetPixel1x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor);
  static void SetPixel2x1_24Bit(uint8_t *framebuffer, uint32_t pixelColor);
  static void SetPixel2x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor);
  static void SetPixel2x4_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor);
  static void SetPixel4x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor);
  static void SetPixel4x4_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor);

  static uint8_t *DrawLineNormal1x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal1x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal2x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineNormal4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  static uint8_t *DrawLineDual1x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual1x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual2x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineDual4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  uint8_t *DrawLineHAM2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  uint8_t *DrawLineHAM4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  static uint8_t *DrawLineSegmentBG2x1_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG2x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG4x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  static uint8_t *DrawLineSegmentBG4x4_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  void DrawLineBPL2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBPL4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);

  void DrawLineBG2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
  void DrawLineBG4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset);
};