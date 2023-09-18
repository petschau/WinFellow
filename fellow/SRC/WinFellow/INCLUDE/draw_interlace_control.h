#pragma once

bool drawGetUseInterlacedRendering();
bool drawGetFrameIsLong();

void drawDecideInterlaceStatusForNextFrame();
void drawInterlaceStartup();
void drawInterlaceEndOfFrame();
void drawSetDeinterlace(bool);
