#ifndef CIA_H
#define CIA_H


/* Rename assembly functions referenced from C */

#define ciaReadByte ASMRENAME(ciaReadByte)
#define ciaReadWord ASMRENAME(ciaReadWord)
#define ciaReadLong ASMRENAME(ciaReadLong)
#define ciaWriteByte ASMRENAME(ciaWriteByte)
#define ciaWriteWord ASMRENAME(ciaWriteWord)
#define ciaWriteLong ASMRENAME(ciaWriteLong)
#define ciaSetupEventWrapper ASMRENAME(ciaSetupEventWrapper)

/* Declare assembly functions referenced from C */

extern ULO ciaReadByte(ULO ea);
extern ULO ciaReadWord(ULO ea);
extern ULO ciaReadLong(ULO ea);
extern void ciaWriteByte(ULO data, ULO ea);
extern void ciaWriteWord(ULO data, ULO ea);
extern void ciaWriteLong(ULO data, ULO ea);
extern void ciaSetupEventWrapper(void);

/* Cia functions written in C */

extern void ciaHardReset(void);
extern void ciaEmulationStart(void);
extern void ciaEmulationStop(void);
extern void ciaStartup(void);
extern void ciaShutdown(void);
extern void ciaMemoryMap(void);
extern void ciaHandleEventC(void);
extern void ciaRaiseIndexIRQ(void); /* For the floppy loader */
extern void ciaRaiseIRQC(ULO i, ULO req); /* For kbd.c */
extern void ciaWritesp(ULO i, ULO data); /* For kbd.c */
extern void ciaUpdateEventCounterC(ULO);

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
extern ULO cia_icrreq[2];
extern ULO cia_icrmsk[2];
extern ULO cia_evalarm[2];
extern ULO cia_evlatch[2];
extern ULO cia_evlatching[2];
extern ULO cia_evwritelatch[2];
extern ULO cia_evwritelatching[2];
extern ULO cia_evalarmlatch[2];
extern ULO cia_evalarmlatching[2];
extern ULO cia_ev[2];
extern ULO cia_cra[2];
extern ULO cia_crb[2];
extern ULO cia_pra[2];
extern ULO cia_prb[2];
extern ULO cia_ddra[2];
extern ULO cia_ddrb[2];
extern ULO cia_sp[2];

#endif
