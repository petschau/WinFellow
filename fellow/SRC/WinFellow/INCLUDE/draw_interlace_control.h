#ifndef DRAW_INTERLACE_CONTROL
#define DRAW_INTERLACE_CONTROL

bool drawGetUseInterlacedRendering(void);
bool drawGetFrameIsLong(void);

void drawDecideInterlaceStatusForNextFrame(void);
void drawInterlaceStartup(void);
void drawInterlaceEndOfFrame(void);
void drawSetDeinterlace(bool);

#endif