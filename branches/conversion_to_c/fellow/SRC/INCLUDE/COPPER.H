#ifndef COPPER_H
#define COPPER_H

/* Rename assembly functions */

#define wcopcon ASMRENAME(wcopcon)
#define wcop1lch ASMRENAME(wcop1lch)
#define wcop1lcl ASMRENAME(wcop1lcl)
#define wcop2lch ASMRENAME(wcop2lch)
#define wcop2lcl ASMRENAME(wcop2lcl)
#define wcopjmp1 ASMRENAME(wcopjmp1)
#define wcopjmp2 ASMRENAME(wcopjmp2)

#define copperUpdateDMA ASMRENAME(copperUpdateDMA)

/* Declare exported functions from assembly */

extern void wcopcon(ULO data, ULO address);
extern void wcop1lch(ULO data, ULO address);
extern void wcop1lcl(ULO data, ULO address);
extern void wcop2lch(ULO data, ULO address);
extern void wcop2lcl(ULO data, ULO address);
extern void wcopjmp1(ULO data, ULO address);
extern void wcopjmp2(ULO data, ULO address);
extern void copperUpdateDMA(void);

ULO rcopjmp1_C(ULO address);
ULO rcopjmp2_C(ULO address);

/* C-declarations */

extern void copperHardReset(void);
extern void copperEmulationStart(void);
extern void copperEmulationStop(void);
extern void copperStartup(void);
extern void copperShutdown(void);

void copperLoad1C(void);
void copperLoad2C(void);

#endif
