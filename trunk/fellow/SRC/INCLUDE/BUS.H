#ifndef BUS_H
#define BUS_H

/*==================================*/
/* Functions exported from assembly */

/* Rename assembly functions */

#define endOfLine ASMRENAME(endOfLine)
#define endOfFrame ASMRENAME(endOfFrame)
#define busstart ASMRENAME(busstart)
#define cia_setup_event_wrapper ASMRENAME(cia_setup_event_wrapper)
#define bus_run ASMRENAME(bus_run)
#define bus_debug ASMRENAME(bus_debug)

/* Assembly functions called from C */

extern void cia_setup_event_wrapper(void);
extern void bus_run(void);
extern void bus_debug(void);

/* Functions that have their address taken in C */

extern void endOfLine(void);
extern void endOfFrame(void);
extern void busstart(void);  /* Code alignment label */


/* End of assembly export */
/*========================*/

/* Standard Fellow Module functions */

extern void busEmulationStart(void);
extern void busEmulationStop(void);
extern void busHardReset(void);
extern void busStartup(void);
extern void busShutdown(void);

extern ULO eol_next, eof_next, cpu_next;
extern ULO irq_next;

extern ULO lvl2_next, lvl3_next;
extern ULO lvl4_next, lvl5_next;
extern ULO lvl6_next, lvl7_next;
extern ULO curcycle;

#endif
