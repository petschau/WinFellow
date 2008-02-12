/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Cia emulation                                                              */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          Rainer Sinsch                                                     */
/*          Marco Nova (novamarco@hotmail.com)                                */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

/* ---------------- CHANGE LOG ----------------- 
Tuesday, September 19, 2000
- changed ciaReadApra in order to support autofire
*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "bus.h"
#include "gameport.h"
#include "fmem.h"
#include "floppy.h"
#include "cia.h"

#define CIA_NO_EVENT          0
#define CIAA_TA_TIMEOUT_EVENT 1
#define CIAA_TB_TIMEOUT_EVENT 2
#define CIAB_TA_TIMEOUT_EVENT 3
#define CIAB_TB_TIMEOUT_EVENT 4

#define CIA_TA_IRQ    1
#define CIA_TB_IRQ    2
#define CIA_ALARM_IRQ 4
#define CIA_KBD_IRQ   8
#define CIA_FLAG_IRQ  16

#define CIA_BUS_CYCLE_RATIO 5

LON cia_next_event_time; /* Cycle for cia-event, measured from sof */
ULO cia_next_event_type; /* What type of event */

/* Cia registers, index 0 is Cia A, index 1 is Cia B */

ULO cia_ta[2];
ULO cia_tb[2];              
ULO cia_ta_rem[2]; /* Preserves remainder when downsizing from bus cycles */
ULO cia_tb_rem[2];              
ULO cia_talatch[2];
ULO cia_tblatch[2];          
LON cia_taleft[2];
LON cia_tbleft[2];
ULO cia_icrreq[2];
ULO cia_icrmsk[2];           
ULO cia_evalarm[2];          
ULO cia_evlatch[2];               
ULO cia_evlatching[2];
ULO cia_evwritelatch[2];               
ULO cia_evwritelatching[2];               
ULO cia_evalarmlatch[2];
ULO cia_evalarmlatching[2];
ULO cia_ev[2];
ULO cia_cra[2];              
ULO cia_crb[2];              
ULO cia_pra[2];
ULO cia_prb[2];              
ULO cia_ddra[2];             
ULO cia_ddrb[2];             
ULO cia_sp[2];               

/* Translate timer -> cycles until timeout from current sof, start of frame */

ULO ciaUnstabilizeValue(ULO value, ULO remainder) {
  return (value*CIA_BUS_CYCLE_RATIO) + curcycle + remainder;
}

/* Translate cycles until timeout from current sof -> timer value */

ULO ciaStabilizeValue(ULO value) {
  return (value - curcycle) / CIA_BUS_CYCLE_RATIO;
}

ULO ciaStabilizeValueRemainder(ULO value) {
  return (value - curcycle) % CIA_BUS_CYCLE_RATIO;
}

void ciaTAStabilize(ULO i) {
  if (cia_cra[i] & 1) {
    cia_ta[i] = ciaStabilizeValue(cia_taleft[i]);
    cia_ta_rem[i] = ciaStabilizeValueRemainder(cia_taleft[i]);
  }
  cia_taleft[i] = -1;
}

void ciaTBStabilize(ULO i) {
  if ((cia_crb[i] & 0x41) == 1) {
    cia_tb[i] = ciaStabilizeValue(cia_tbleft[i]);
    cia_tb_rem[i] = ciaStabilizeValueRemainder(cia_tbleft[i]);
  }
  cia_tbleft[i] = 0xffffffff;
}

void ciaStabilize(ULO i) {
  ciaTAStabilize(i);
  ciaTBStabilize(i);
}

void ciaTAUnstabilize(ULO i) {
  if (cia_cra[i] & 1)
    cia_taleft[i] = ciaUnstabilizeValue(cia_ta[i], cia_ta_rem[i]);
}

void ciaTBUnstabilize(ULO i) {
  if ((cia_crb[i] & 0x41) == 1)
    cia_tbleft[i] = ciaUnstabilizeValue(cia_tb[i], cia_tb_rem[i]);
}

void ciaUnstabilize(ULO i) {
  ciaTAUnstabilize(i);
  ciaTBUnstabilize(i);
}  

/* This used to clear the corresponding bit in INTREQ when
/* no CIA IRQs were waiting (anymore), according to the HRM it will take */
/* the irq line from the CIA high (off), but the bit in INTREQ */
/* should probably remain set until it is cleared by other means. */

void ciaUpdateIRQ(ULO i) {
  if (cia_icrreq[i] & cia_icrmsk[i]) {
    cia_icrreq[i] |= 0x80;
    wriw((i == 0) ? 0x8008 : 0xa000, 0xdff09c);
  }
  else { /* Spaceballs State of the Art requires this. */
    wriw((i == 0) ? 8 : 0x2000, 0xdff09c);
  }
}  

void ciaRaiseIRQC(ULO i, ULO req) {
  cia_icrreq[i] |= (req & 0x1f);
  ciaUpdateIRQ(i);
}

/* Helps the floppy loader, Cia B Flag IRQ */

void ciaRaiseIndexIRQ(void) {
  ciaRaiseIRQC(1, CIA_FLAG_IRQ);
}

/* Timeout handlers */

void ciaHandleTBTimeout(ULO i) {
  cia_tb[i] = cia_tblatch[i];      /* Reload from latch */
  if (cia_tb[i] == 0) cia_tb[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
  if (cia_crb[i] & 8) {            /* One Shot Mode */
    cia_crb[i] &= 0xfe;            /* Stop timer */
    cia_tbleft[i] = 0xffffffff;
  }
  else if (!(cia_crb[i] & 0x40))   /* Continuous mode, no attach */
    cia_tbleft[i] = ciaUnstabilizeValue(cia_tb[i], 0);
  ciaRaiseIRQC(i, CIA_TB_IRQ);     /* Raise irq */
}

void ciaHandleTATimeout(ULO i) {
  cia_ta[i] = cia_talatch[i];      /* Reload from latch */
  if (cia_ta[i] == 0) cia_ta[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
  if ((cia_crb[i] & 0x41) == 0x41){/* Timer B attached and started */
    cia_tb[i] = (cia_tb[i] - 1) & 0xffff;
    if (cia_tb[i] == 0)
      ciaHandleTBTimeout(i);
  }
  if (cia_cra[i] & 8) {            /* One Shot Mode */
    cia_cra[i] &= 0xfe;            /* Stop timer */
    cia_taleft[i] = -1;
  }
  else                             /* Continuous mode */
    cia_taleft[i] = ciaUnstabilizeValue(cia_ta[i], 0);
  ciaRaiseIRQC(i, CIA_TA_IRQ);    /* Raise irq */
}

/* Called from eol-handler (B) or eof handler (A) */

void ciaUpdateEventCounterC(ULO i) {
  if (!cia_evwritelatching[i]) {
    cia_ev[i] = (cia_ev[i] + 1) & 0xffffff;
    if (cia_evalarm[i] == cia_ev[i])
      ciaRaiseIRQC(i, CIA_ALARM_IRQ);
  }
}

/* Called from the eof-handler to update timers */

void ciaUpdateTimersEOFC(void) {
  int i;
  for (i = 0; i < 2; i++) {
    if (cia_taleft[i] >= 0)
      if ((cia_taleft[i] -= CYCLESPERFRAME) < 0)
	cia_taleft[i] = 0;
    if (cia_tbleft[i] >= 0)
      if ((cia_tbleft[i] -= CYCLESPERFRAME) < 0)
	cia_tbleft[i] = 0;
  }
  if (cia_next_event_time >= 0)
    if ((cia_next_event_time -= CYCLESPERFRAME) < 0)
      cia_next_event_time = 0;
  ciaUpdateEventCounterC(0);
}

/* Record next timer timeout */

void ciaSetupNextEvent(void) {
  ULO nextevtime = 0xffffffff, nextevtype = CIA_NO_EVENT, j;
  for (j = 0; j < 2; j++) {
    if (((ULO) cia_taleft[j]) < nextevtime) {
      nextevtime = cia_taleft[j];
      nextevtype = (j*2) + 1;
    }
    if (((ULO) cia_tbleft[j]) < nextevtime) {
      nextevtime = cia_tbleft[j];
      nextevtype = (j*2) + 2;
    }
  }  
  cia_next_event_time = nextevtime;
  cia_next_event_type = nextevtype;
  ciaSetupEventWrapper();
}

void ciaHandleEventC(void) {
  switch (cia_next_event_type) {
  case CIAA_TA_TIMEOUT_EVENT:
    ciaHandleTATimeout(0);
    break;
  case CIAA_TB_TIMEOUT_EVENT:
    ciaHandleTBTimeout(0);
    break;
  case CIAB_TA_TIMEOUT_EVENT:
    ciaHandleTATimeout(1);
    break;
  case CIAB_TB_TIMEOUT_EVENT:
    ciaHandleTBTimeout(1);
    break;
  default:
    break;
  }
  ciaSetupNextEvent();
}

/* PRA */

ULO ciaReadApra(void) {
  ULO result = 0;
  ULO drivesel;
  
  if( gameport_autofire0[0] )
	gameport_fire0[0] = !gameport_fire0[0];
  if( gameport_autofire0[1] )
	gameport_fire0[1] = !gameport_fire0[1];

  if (!gameport_fire0[0])
    result |= 0x40;	/* Two firebuttons on port 1 */
  if (!gameport_fire0[1])
    result |= 0x80;
  drivesel = floppySelectedGet();       /* Floppy bits */
  if (!floppyIsReady(drivesel))
    result |= 0x20;
  if (!floppyIsTrack0(drivesel))
    result |= 0x10;
  if (!floppyIsWriteProtected(drivesel))
    result |= 8;
  if (!floppyIsChanged(drivesel))
    result |= 4;    
  return result | (cia_pra[0] & 2);
}

ULO ciaReadpra(ULO i) {
  if (i == 0)
    return ciaReadApra();
  return cia_pra[1];
}

void ciaWriteApra(ULO data) {
  if (cia_pra[0] != data)
    cia_pra[0] = data;
}

void ciaWritepra(ULO i, ULO data) {
  if (i == 0)
    ciaWriteApra(data);
  else
    cia_pra[1] = data;
}

/* PRB */

ULO ciaReadprb(ULO i) {
  return cia_prb[i];
}

/* Motor, drive latches this value when SEL goes from high to low */

void ciaWriteBprb(ULO data) {
  int i, j;

  j = 0;
  for (i = 8; i < 0x80; i <<= 1, j++)
    if ((cia_prb[1] & i) && !(data & i))
      floppyMotorSet(j, (data & 0x80)>>7);
  cia_prb[1] = data;
  floppySelectedSet((data & 0x78)>>3);
  floppySideSet((data & 4)>>2);
  floppyDirSet((data & 2)>>1);
  floppyStepSet(data & 1);  
}

void ciaWriteprb(ULO i, ULO data) {
  if (i == 0)
    cia_prb[0] = data;
  else
    ciaWriteBprb(data);
}

/* DDRA */

ULO ciaReadddra(ULO i) {
  if (i == 0)
    return 3;
  return 0xc0;
}

void ciaWriteddra(ULO i, ULO data) {
}

/* DDRB */

ULO ciaReadddrb(ULO i) {
  if (i == 0)
    return cia_ddrb[0];
  return 0xff;
}

void ciaWriteddrb(ULO i, ULO data) {
  if (i == 0)
    cia_ddrb[0] = data;
}

/* SP (Keyboard serial data on Cia A) */

ULO ciaReadsp(ULO i) {
  return cia_sp[i];
}

void ciaWritesp(ULO i, ULO data) {
  cia_sp[i] = data;
}

/* Timer A */

ULO ciaReadtalo(ULO i) {
  if (cia_cra[i] & 1)
    return ciaStabilizeValue(cia_taleft[i]) & 0xff;
  return cia_ta[i] & 0xff;
}

ULO ciaReadtahi(ULO i) {
  if (cia_cra[i] & 1)
    return (ciaStabilizeValue(cia_taleft[i])>>8) & 0xff;
  return (cia_ta[i]>>8) & 0xff;
}  

void ciaWritetalo(ULO i, ULO data) {
  cia_talatch[i] = (cia_talatch[i] & 0xff00) | (data & 0xff);
}

void ciaWritetahi(ULO i, ULO data) {
  cia_talatch[i] = (cia_talatch[i] & 0xff) | ((data & 0xff)<<8);
  if ((cia_cra[i] & 8) || !(cia_cra[i] & 1)) {
    ciaStabilize(i);
    cia_ta[i] = cia_talatch[i];
    cia_ta_rem[i] = 0;
    if (cia_ta[i] == 0) cia_ta[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
    if (cia_cra[i] & 8) cia_cra[i] |= 1;
    ciaUnstabilize(i);
    ciaSetupNextEvent();
  }
}

/* Timer B */

ULO ciaReadtblo(ULO i) {
  if ((cia_crb[i] & 1) && !(cia_crb[i] & 0x40))
    return ciaStabilizeValue(cia_tbleft[i]) & 0xff;
  return cia_tb[i] & 0xff;
}

ULO ciaReadtbhi(ULO i) {
  if ((cia_crb[i] & 1) && !(cia_crb[i] & 0x40))
    return (ciaStabilizeValue(cia_tbleft[i])>>8) & 0xff;
  return (cia_tb[i]>>8) & 0xff;
}

void ciaWritetblo(ULO i, ULO data) {
  cia_tblatch[i] = (cia_tblatch[i] & 0xff00) | (data & 0xff);
}

void ciaWritetbhi(ULO i, ULO data) {
  cia_tblatch[i] = (cia_tblatch[i] & 0xff) | ((data & 0xff)<<8);
  if ((cia_crb[i] & 8) || !(cia_crb[i] & 1)) {
    ciaStabilize(i);
    cia_tb[i] = cia_tblatch[i];
    cia_tb_rem[i] = 0;
    if (cia_tb[i] == 0) cia_tb[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
    if (cia_crb[i] & 8)
      cia_crb[i] |= 1;
    ciaUnstabilize(i);
    ciaSetupNextEvent();
  }
}

/* Event counter */

ULO ciaReadevlo(ULO i) {
  if (cia_evlatching[i]) {
    cia_evlatching[i] = FALSE;
    return cia_evlatch[i] & 0xff;
  }
  return cia_ev[i] & 0xff;
}

ULO ciaReadevmi(ULO i) {
  if (cia_evlatching[i])
    return (cia_evlatch[i]>>8) & 0xff;
  return (cia_ev[i]>>8) & 0xff;
}

ULO ciaReadevhi(ULO i) {
  cia_evlatching[i] = TRUE;
  cia_evlatch[i] = cia_ev[i];
  return (cia_ev[i]>>16) & 0xff;
}

void ciaWriteevlo(ULO i, ULO data) {
  cia_evwritelatching[i] = FALSE;
  cia_evwritelatch[i] = (cia_evwritelatch[i] & 0xffff00) | (data & 0xff);
  if (cia_crb[i] & 0x80)
    cia_evalarm[i] = cia_evwritelatch[i];
  else
    cia_ev[i] = cia_evwritelatch[i];
  if (cia_ev[i] == cia_evalarm[i])
    ciaRaiseIRQC(i, CIA_ALARM_IRQ);
}

void ciaWriteevmi(ULO i, ULO data) {
  cia_evwritelatching[i] = TRUE;
  cia_evwritelatch[i] = (cia_evwritelatch[i] & 0xff00ff) | ((data & 0xff)<<8);
}

void ciaWriteevhi(ULO i, ULO data) {
  cia_evwritelatching[i] = TRUE;
  cia_evwritelatch[i] = (cia_evwritelatch[i] & 0xffff) | ((data & 0xff)<<16);
}

/* ICR */

ULO ciaReadicr(ULO i) {
  int tmp = cia_icrreq[i];
  cia_icrreq[i] = 0;
  return tmp;
}

void ciaWriteicr(ULO i, ULO data) {
  if (data & 0x80) {
    cia_icrmsk[i] |= (data & 0x1f);
    if (cia_icrreq[i] & cia_icrmsk[i])
      ciaRaiseIRQC(i, 0);
  }
  else {
    cia_icrmsk[i] &= ~(data & 0x1f);
    ciaUpdateIRQ(i);
  }
}

/* CRA */

ULO ciaReadcra(ULO i) {
  return cia_cra[i] & 0xff;
}

void ciaWritecra(ULO i, ULO data) {
  ciaStabilize(i);
  if (data & 0x10) {
    cia_ta[i] = cia_talatch[i];
    cia_ta_rem[i] = 0;
    if (cia_ta[i] == 0) cia_ta[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
    data &= 0xef;
  }
  cia_cra[i] = data & 0xff;
  ciaUnstabilize(i);
  ciaSetupNextEvent();
}  

/* CRB */

ULO ciaReadcrb(ULO i) {
  return cia_crb[i] & 0xff;
}

void ciaWritecrb(ULO i, ULO data) {
  ciaStabilize(i);
  if (data & 0x10) {
    cia_tb[i] = cia_tblatch[i];
    cia_tb_rem[i] = 0;
    if (cia_tb[i] == 0) cia_tb[i] = 65535; /* Need to do this to avoid endless timeout loop, but is it correct? */
    data &= 0xef;
  }
  cia_crb[i] = data & 0xff;
  ciaUnstabilize(i);
  ciaSetupNextEvent();
}  

/* Dummy read and write */

ULO ciaReadnothing(ULO i) {
  return 0xff;
}

void ciaWritenothing(ULO i, ULO data) {
}
	
/* Table of CIA read/write functions */

ciareadfunc cia_read[16] = {ciaReadpra, ciaReadprb, ciaReadddra,ciaReadddrb,
			    ciaReadtalo,ciaReadtahi,ciaReadtblo,ciaReadtbhi,
			    ciaReadevlo,ciaReadevmi,ciaReadevhi,ciaReadnothing,
			    ciaReadsp,  ciaReadicr, ciaReadcra, ciaReadcrb};
ciawritefunc cia_write[16]={
                         ciaWritepra, ciaWriteprb, ciaWriteddra,ciaWriteddrb,
			 ciaWritetalo,ciaWritetahi,ciaWritetblo,ciaWritetbhi,
			 ciaWriteevlo,ciaWriteevmi,ciaWriteevhi,ciaWritenothing,
			 ciaWritesp,  ciaWriteicr, ciaWritecra, ciaWritecrb};

/* Routine callbacks called by memory system, these are in C, but have */
/* assembly wrappers that are called first                             */

ULO ciaReadByteC(ULO address) {
  if ((address & 0xa01001) == 0xa00001)
    return cia_read[(address & 0xf00)>>8](0);
  else if ((address & 0xa02001) == 0xa00000)
    return cia_read[(address & 0xf00)>>8](1);
  return 0xff;
}

ULO ciaReadWordC(ULO address) {
  return (ciaReadByteC(address)<<8) | ciaReadByteC(address + 1);
}

ULO ciaReadLongC(ULO address) {
  return (ciaReadByteC(address)<<24)    | (ciaReadByteC(address + 1)<<16) |
         (ciaReadByteC(address + 2)<<8) | ciaReadByteC(address + 3);
}

void ciaWriteByteC(ULO data, ULO address) {
   if ((memory_wriorgadr & 0xa01001) == 0xa00001)
    cia_write[(memory_wriorgadr & 0xf00)>>8](0, data & 0xff);
  else if ((memory_wriorgadr & 0xa02001) == 0xa00000)
    cia_write[(memory_wriorgadr & 0xf00)>>8](1, data & 0xff);
}

void ciaWriteWordC(ULO data, ULO address) {
  ciaWriteByteC(memory_wriorgadr, data>>8);
  ciaWriteByteC(memory_wriorgadr + 1, data);
}

void ciaWriteLongC(ULO data, ULO address) {
  ciaWriteByteC(memory_wriorgadr, data>>24);
  ciaWriteByteC(memory_wriorgadr + 1, data>>16);
  ciaWriteByteC(memory_wriorgadr + 2, data>>8);
  ciaWriteByteC(memory_wriorgadr + 3, data);
}


/*============================================================================*/
/* Map cia banks into the memory table                                        */
/*============================================================================*/

void ciaMemoryMap(void) {
  ULO bank;

  for (bank = 0xa00000>>16; bank < (0xc00000>>16); bank++)
    memoryBankSet(ciaReadByte,
		  ciaReadWord,
		  ciaReadLong,
		  ciaWriteByte,
		  ciaWriteWord,
		  ciaWriteLong,
		  NULL,
		  bank,
		  0xa00000>>16,
		  FALSE);
}


/*============================================================================*/
/* Cia state zeroing                                                          */
/*============================================================================*/

void ciaStateClear(void) {
  ULO i;

  for (i = 0; i < 2; i++) {
    cia_ev[i] = 0;		/* Zero out event counters */
    cia_evlatch[i] = 0;
    cia_evlatching[i] = 0;
    cia_evalarm[i] = 0;
    cia_evalarmlatch[i] = 0;
    cia_evalarmlatching[i] = 0;
    cia_evwritelatch[i] = 0;
    cia_evwritelatching[i] = 0;
    cia_taleft[i] = -1;		/* Zero out timers */
    cia_tbleft[i] = -1;
    cia_ta[i] = 0xffff;        
    cia_tb[i] = 0xffff;
    cia_talatch[i] = 0xffff;
    cia_tblatch[i] = 0xffff;
    cia_pra[i] = 0xff;
    cia_prb[i] = 0;
    cia_ddra[i] = 0;
    cia_ddrb[i] = 0;
    cia_icrreq[i] = 0;
    cia_icrmsk[i] = 0;
    cia_cra[i] = 0;
    cia_crb[i] = 0;
  }
  cia_next_event_time = -1;
  cia_next_event_type = 0;
}

  
/*============================================================================*/
/* Cia module control                                                         */
/*============================================================================*/

void ciaEmulationStart(void) {
}

void ciaEmulationStop(void) {
}

void ciaHardReset(void) {
  ciaStateClear();
  ciaMemoryMap();
}

void ciaStartup(void) {
  ciaStateClear();
}

void ciaShutdown(void) {
}
