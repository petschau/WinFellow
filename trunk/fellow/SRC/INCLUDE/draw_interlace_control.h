#ifndef DRAW_INTERLACE_CONTROL
#define DRAW_INTERLACE_CONTROL

bool drawGetFrameIsInterlaced(void);
bool drawGetFrameIsLong(void);

bool drawDecideInterlaceStatusForNextFrame(void);
void drawClearInterlaceStatus(void);
void drawInterlaceEndOfFrame(void);

#endif