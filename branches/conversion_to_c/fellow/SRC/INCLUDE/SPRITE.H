#ifndef SPRITE_H
#define SPRITE_H

#include "graph.h"

/* C-declarations */

void wspr0pth_C(ULO data, ULO address);
void wspr0ptl_C(ULO data, ULO address);
void wspr1pth_C(ULO data, ULO address);
void wspr1ptl_C(ULO data, ULO address);
void wspr2pth_C(ULO data, ULO address);
void wspr2ptl_C(ULO data, ULO address);
void wspr3pth_C(ULO data, ULO address);
void wspr3ptl_C(ULO data, ULO address);
void wspr4pth_C(ULO data, ULO address);
void wspr4ptl_C(ULO data, ULO address);
void wspr5pth_C(ULO data, ULO address);
void wspr5ptl_C(ULO data, ULO address);
void wspr6pth_C(ULO data, ULO address);
void wspr6ptl_C(ULO data, ULO address);
void wspr7pth_C(ULO data, ULO address);
void wspr7ptl_C(ULO data, ULO address);

extern void spriteSetDelay(ULO delay);
extern void spriteEndOfLine(void);
extern void spriteEndOfFrame(void);
extern void spriteHardReset(void);
extern void spriteEmulationStart(void);
extern void spriteEmulationStop(void);
extern void spriteStartup(void);
extern void spriteShutdown(void);

extern void spriteMergeHAM1x8(UBY *frameptr, graph_line *linedesc);
extern void spriteMergeHAM2x8(UWO *frameptr, graph_line *linedesc);
extern void spriteMergeHAM1x16(UWO *frameptr, graph_line *linedesc);
extern void spriteMergeHAM2x16(ULO *frameptr, graph_line *linedesc);
extern void spriteMergeHAM1x24(UBY *frameptr, graph_line *linedesc);
extern void spriteMergeHAM2x24(UBY *frameptr, graph_line *linedesc);
extern void spriteMergeHAM1x32(ULO *frameptr, graph_line *linedesc);
extern void spriteMergeHAM2x32(ULO *frameptr, graph_line *linedesc);


/* variable declerations */

extern ULO sprite_write_next;
extern ULO sprite_write_real;
extern ULO sprite_write_buffer[128][2];


#endif /* SPRITE_H */
