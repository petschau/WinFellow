#ifndef SPRITE_H
#define SPRITE_H

#include "graph.h"

/*============================================================================*/
/* struct that holds the sprite register access during a line                 */
/*============================================================================*/

typedef void (*spr_register_func)(ULO data, ULO address);

typedef struct {
  ULO raster_y;
  ULO raster_x;
  spr_register_func called_function;
  ULO data;
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

/* Assembly declarations */
#define wspr0pth ASMRENAME(wspr0pth)
#define wspr0ptl ASMRENAME(wspr0ptl)
#define wspr1pth ASMRENAME(wspr1pth)
#define wspr1ptl ASMRENAME(wspr1ptl)
#define wspr2pth ASMRENAME(wspr2pth)
#define wspr2ptl ASMRENAME(wspr2ptl)
#define wspr3pth ASMRENAME(wspr3pth)
#define wspr3ptl ASMRENAME(wspr3ptl)
#define wspr4pth ASMRENAME(wspr4pth)
#define wspr4ptl ASMRENAME(wspr4ptl)
#define wspr5pth ASMRENAME(wspr5pth)
#define wspr5ptl ASMRENAME(wspr5ptl)
#define wspr6pth ASMRENAME(wspr6pth)
#define wspr6ptl ASMRENAME(wspr6ptl)
#define wspr7pth ASMRENAME(wspr7pth)
#define wspr7ptl ASMRENAME(wspr7ptl)
#define wsprxdata ASMRENAME(wsprxdata)
#define wsprxdatb ASMRENAME(wsprxdatb)
#define wsprxctl ASMRENAME(wsprxctl)
#define wsprxpos ASMRENAME(wsprxpos)
#define decidedelay ASMRENAME(decidedelay)

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

void aspr0pth(ULO data, ULO address);
void aspr0ptl(ULO data, ULO address);
void aspr1pth(ULO data, ULO address);
void aspr1ptl(ULO data, ULO address);
void aspr2pth(ULO data, ULO address);
void aspr2ptl(ULO data, ULO address);
void aspr3pth(ULO data, ULO address);
void aspr3ptl(ULO data, ULO address);
void aspr4pth(ULO data, ULO address);
void aspr4ptl(ULO data, ULO address);
void aspr5pth(ULO data, ULO address);
void aspr5ptl(ULO data, ULO address);
void aspr6pth(ULO data, ULO address);
void aspr6ptl(ULO data, ULO address);
void aspr7pth(ULO data, ULO address);
void aspr7ptl(ULO data, ULO address);

void asprxpos(ULO data, ULO address);
void asprxctl(ULO data, ULO address);
void asprxdata(ULO data, ULO address);
void asprxdatb(ULO data, ULO address);

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

extern void wspr0pth(ULO data, ULO address);
extern void wspr0ptl(ULO data, ULO address);
extern void wspr1pth(ULO data, ULO address);
extern void wspr1ptl(ULO data, ULO address);
extern void wspr2pth(ULO data, ULO address);
extern void wspr2ptl(ULO data, ULO address);
extern void wspr3pth(ULO data, ULO address);
extern void wspr3ptl(ULO data, ULO address);
extern void wspr4pth(ULO data, ULO address);
extern void wspr4ptl(ULO data, ULO address);
extern void wspr5pth(ULO data, ULO address);
extern void wspr5ptl(ULO data, ULO address);
extern void wspr6pth(ULO data, ULO address);
extern void wspr6ptl(ULO data, ULO address);
extern void wspr7pth(ULO data, ULO address);
extern void wspr7ptl(ULO data, ULO address);
extern void wsprxdata(ULO data, ULO address);
extern void wsprxdatb(ULO data, ULO address);
extern void wsprxctl(ULO data, ULO address);
extern void wsprxpos(ULO data, ULO address);
extern void decidedelay(void);

/* variable declerations */

extern ULO sprite_write_next;
extern ULO sprite_write_real;
extern ULO sprite_write_buffer[128][2];


#endif /* SPRITE_H */
