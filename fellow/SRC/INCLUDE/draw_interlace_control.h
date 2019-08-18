#pragma once

#include "fellow/application/HostRenderConfiguration.h"

bool drawGetUseInterlacedRendering();
bool drawGetFrameIsLong();
bool drawGetFrameIsInterlaced();

void drawDecideInterlaceStatusForNextFrame();
void drawInterlaceStartup();
void drawInterlaceEndOfFrame();
void drawInterlaceConfigure(bool deinterlace, DisplayScaleStrategy displayScaleStrategy);
