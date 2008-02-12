#ifndef COPPER_H
#define COPPER_H

/* Rename assembly functions */

#define rcopjmp1 ASMRENAME(rcopjmp1)
#define rcopjmp2 ASMRENAME(rcopjmp2)
#define wcopcon ASMRENAME(wcopcon)
#define wcop1lch ASMRENAME(wcop1lch)
#define wcop1lcl ASMRENAME(wcop1lcl)
#define wcop2lch ASMRENAME(wcop2lch)
#define wcop2lcl ASMRENAME(wcop2lcl)
#define wcopjmp1 ASMRENAME(wcopjmp1)
#define wcopjmp2 ASMRENAME(wcopjmp2)

/* Declare exported functions from assembly */

extern ULO rcopjmp1(ULO address);
extern ULO rcopjmp2(ULO address);
extern void wcopcon(ULO data, ULO address);
extern void wcop1lch(ULO data, ULO address);
extern void wcop1lcl(ULO data, ULO address);
extern void wcop2lch(ULO data, ULO address);
extern void wcop2lcl(ULO data, ULO address);
extern void wcopjmp1(ULO data, ULO address);
extern void wcopjmp2(ULO data, ULO address);


/* C-declarations */


extern void copperHardReset(void);
extern void copperEmulationStart(void);
extern void copperEmulationStop(void);
extern void copperStartup(void);
extern void copperShutdown(void);

#endif
