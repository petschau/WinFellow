#ifndef SPRITE_H
#define SPRITE_H

/* Rename assembly functions */

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
#define wsprxpos ASMRENAME(wsprxpos)
#define wsprxctl ASMRENAME(wsprxctl)
#define wsprxdata ASMRENAME(wsprxdata)
#define wsprxdatb ASMRENAME(wsprxdatb)

/* Declare asm functions */

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
extern void wsprxpos(ULO data, ULO address);
extern void wsprxctl(ULO data, ULO address);
extern void wsprxdata(ULO data, ULO address);
extern void wsprxdatb(ULO data, ULO address);

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

/* variable declerations */

extern ULO sprite_write_next;
extern ULO sprite_write_real;
extern ULO sprite_write_buffer[128][2];


#endif /* SPRITE_H */
