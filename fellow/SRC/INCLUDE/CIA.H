#ifndef CIA_H
#define CIA_H

extern void ciaHardReset(void);
extern void ciaEmulationStart(void);
extern void ciaEmulationStop(void);
extern void ciaStartup(void);
extern void ciaShutdown(void);
extern void ciaMemoryMap(void);
extern void ciaHandleEvent(void);
extern void ciaRaiseIndexIRQ(void); /* For the floppy loader */
extern void ciaRaiseIRQ(ULO i, ULO req); /* For kbd.c */
extern void ciaWritesp(ULO i, UBY data); /* For kbd.c */
extern void ciaUpdateEventCounter(ULO);
extern void ciaUpdateTimersEOF(void);

/* Cia event data */

extern LON cia_next_event_time;  /* Cycle when next cia-event happen, measured from sof */
extern ULO cia_next_event_type;   /* What type of event */

/* Cia registers, index 0 is Cia A, index 1 is Cia B */

extern ULO cia_ta[2];
extern ULO cia_tb[2];
extern ULO cia_talatch[2];
extern ULO cia_tblatch[2];
extern LON cia_taleft[2];
extern LON cia_tbleft[2];
extern ULO cia_evalarm[2];
extern ULO cia_evlatch[2];
extern ULO cia_evlatching[2];
extern ULO cia_evwritelatch[2];
extern ULO cia_evwritelatching[2];
extern ULO cia_evalarmlatch[2];
extern ULO cia_evalarmlatching[2];
extern ULO cia_ev[2];
extern UBY cia_icrreq[2];
extern UBY cia_icrmsk[2];
extern UBY cia_cra[2];
extern UBY cia_crb[2];
extern UBY cia_pra[2];
extern UBY cia_prb[2];
extern UBY cia_ddra[2];
extern UBY cia_ddrb[2];
extern UBY cia_sp[2];

#endif
