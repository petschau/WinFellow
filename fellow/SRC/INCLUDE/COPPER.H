#ifndef COPPER_H
#define COPPER_H

void wcopcon_C(ULO data, ULO address);
void wcop1lch_C(ULO data, ULO address);
void wcop1lcl_C(ULO data, ULO address);
void wcop2lch_C(ULO data, ULO address);
void wcop2lcl_C(ULO data, ULO address);
void wcopjmp1_C(ULO data, ULO address);
void wcopjmp2_C(ULO data, ULO address);
ULO rcopjmp1_C(ULO address);
ULO rcopjmp2_C(ULO address);

/* C-declarations */

void copperUpdateDMA(void);

extern void copperHardReset(void);
extern void copperEmulationStart(void);
extern void copperEmulationStop(void);
extern void copperStartup(void);
extern void copperShutdown(void);

void copperLoad1C(void);
void copperLoad2C(void);
void copperEmulateC(void);


#endif
