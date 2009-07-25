#ifndef COPPER_H
#define COPPER_H

void wcopcon(UWO data, ULO address);
void wcop1lch(UWO data, ULO address);
void wcop1lcl(UWO data, ULO address);
void wcop2lch(UWO data, ULO address);
void wcop2lcl(UWO data, ULO address);
void wcopjmp1(UWO data, ULO address);
void wcopjmp2(UWO data, ULO address);
UWO rcopjmp1(ULO address);
UWO rcopjmp2(ULO address);

void copperUpdateDMA(void);

extern void copperSaveState(FILE *F);
extern void copperLoadState(FILE *F);
extern void copperEndOfFrame(void);
extern void copperHardReset(void);
extern void copperEmulationStart(void);
extern void copperEmulationStop(void);
extern void copperStartup(void);
extern void copperShutdown(void);

void copperLoad1(void);
void copperLoad2(void);
void copperEmulate(void);

#endif
