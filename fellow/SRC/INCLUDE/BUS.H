#ifndef BUS_H
#define BUS_H

/*==================================*/
/* Functions exported from assembly */

/* Rename assembly functions */

#define endOfLine ASMRENAME(endOfLine)
#define busstart ASMRENAME(busstart)
#define cia_setup_event_wrapper ASMRENAME(cia_setup_event_wrapper)
#define bus_run ASMRENAME(bus_run)
#define bus_debug ASMRENAME(bus_debug)

#define endOfFrameASM ASMRENAME(endOfFrameASM)

#define cpuSetUpInterrupt ASMRENAME(cpuSetUpInterrupt)
//#define finish_blit ASMRENAME(finish_blit)
#define endOfLine ASMRENAME(endOfLine)
//#define copperEmulate ASMRENAME(copperEmulate)
#define endOfFrame ASMRENAME(endOfFrame)
//#define graphEndOfLine ASMRENAME(graphEndOfLine)


/* Assembly functions called from C */

extern void cia_setup_event_wrapper(void);
extern void bus_run(void);
extern void bus_debug(void);

/* Functions that have their address taken in C */

extern void endOfLine(void);
extern void busstart(void);  /* Code alignment label */

extern void cpuSetUpInterrupt(void);
//extern void finish_blit(void);
extern void endOfLine(void);
//extern void copperEmulate(void);
//extern void graphEndOfLine(void);

extern void endOfFrameASM(void);



/* End of assembly export */
/*========================*/

/* Standard Fellow Module functions */

extern void busEmulationStart(void);
extern void busEmulationStop(void);
extern void busHardReset(void);
extern void busStartup(void);
extern void busShutdown(void);

void busScanLevel6(ULO* lvlx_next, buseventfunc* lvlx_ptr);
void busScanLevel5(ULO* lvlx_next, buseventfunc* lvlx_ptr);
void busScanLevel4(ULO* lvlx_next, buseventfunc* lvlx_ptr);
void busScanLevel3(ULO* lvlx_next, buseventfunc* lvlx_ptr);
void busScanLevel2(ULO* lvlx_next, buseventfunc* lvlx_ptr);

void busScanEventsLevel2(void);
void busScanEventsLevel3(void);
void busScanEventsLevel4(void);
void busScanEventsLevel5(void);
void busScanEventsLevel6(void);

void endOfLineC(void);
void endOfFrameC(void);

extern ULO eol_next, eof_next;
extern LON irq_next, cpu_next;

extern ULO lvl2_next, lvl3_next;
extern ULO lvl4_next, lvl5_next;
extern ULO lvl6_next, lvl7_next;
extern ULO curcycle;

#endif
