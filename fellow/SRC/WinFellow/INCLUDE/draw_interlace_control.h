#pragma once

bool drawGetUseInterlacedRendering(void);
bool drawGetFrameIsLong(void);

void drawDecideInterlaceStatusForNextFrame(void);
void drawInterlaceStartup(void);
void drawInterlaceEndOfFrame(void);
void drawSetDeinterlace(bool);
