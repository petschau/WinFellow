#ifndef SPRITE_H
#define SPRITE_H

#include "graph.h"

/*============================================================================*/
/* struct that holds the sprite register access during a line                 */
/*============================================================================*/

typedef void (*spr_register_func)(UWO data, ULO address);

typedef struct {
  ULO raster_y;
  ULO raster_x;
  spr_register_func called_function;
  UWO data;
  ULO address;
} spr_action_list_item;

typedef struct {
  ULO count;
  spr_action_list_item items[1000]; /* How many is actually needed? */
} spr_action_list_master;


typedef struct {
	UBY sprite_data[16];
	ULO sprx;
} spr_merge_list_item;

typedef struct {
  ULO count;
  spr_merge_list_item items[1000]; /* How many is actually needed? */
} spr_merge_list_master;

/* C-declarations */

void wspr0pth(UWO data, ULO address);
void wspr0ptl(UWO data, ULO address);
void wspr1pth(UWO data, ULO address);
void wspr1ptl(UWO data, ULO address);
void wspr2pth(UWO data, ULO address);
void wspr2ptl(UWO data, ULO address);
void wspr3pth(UWO data, ULO address);
void wspr3ptl(UWO data, ULO address);
void wspr4pth(UWO data, ULO address);
void wspr4ptl(UWO data, ULO address);
void wspr5pth(UWO data, ULO address);
void wspr5ptl(UWO data, ULO address);
void wspr6pth(UWO data, ULO address);
void wspr6ptl(UWO data, ULO address);
void wspr7pth(UWO data, ULO address);
void wspr7ptl(UWO data, ULO address);
extern void wsprxpos(UWO,ULO);
extern void wsprxctl(UWO,ULO);
extern void wsprxdata(UWO,ULO);
extern void wsprxdatb(UWO,ULO);


void aspr0pth(UWO data, ULO address);
void aspr0ptl(UWO data, ULO address);
void aspr1pth(UWO data, ULO address);
void aspr1ptl(UWO data, ULO address);
void aspr2pth(UWO data, ULO address);
void aspr2ptl(UWO data, ULO address);
void aspr3pth(UWO data, ULO address);
void aspr3ptl(UWO data, ULO address);
void aspr4pth(UWO data, ULO address);
void aspr4ptl(UWO data, ULO address);
void aspr5pth(UWO data, ULO address);
void aspr5ptl(UWO data, ULO address);
void aspr6pth(UWO data, ULO address);
void aspr6ptl(UWO data, ULO address);
void aspr7pth(UWO data, ULO address);
void aspr7ptl(UWO data, ULO address);

void asprxpos(UWO data, ULO address);
void asprxctl(UWO data, ULO address);
void asprxdata(UWO data, ULO address);
void asprxdatb(UWO data, ULO address);


int spriteCompareRasterX(spr_action_list_item *item_from_list, spr_action_list_item *to_be_inserted);
//void spriteDMAActionListClear(ULO sprnr);
void spriteActionListsClear();
//void spriteActionListClear(ULO sprnr);
//void spriteMergeListClear(ULO sprnr);

void spriteMergeDualPlayfield(graph_line* current_graph_line);
void spriteMergeHires(graph_line* current_graph_line);
void spriteMergeHires320(graph_line* current_graph_line);
void spriteMergeLores(graph_line* current_graph_line);

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

extern void spritesMerge(graph_line* current_graph_line);
extern void spriteProcessActionList(void);
extern void spritesDMASpriteHandler(void);
extern void spriteEndOfLine(void);


#endif /* SPRITE_H */
