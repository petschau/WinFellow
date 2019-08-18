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
