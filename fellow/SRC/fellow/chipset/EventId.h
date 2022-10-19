#pragma once

#include "fellow/api/defs.h"

#define EVENTCOUNT 16384
#define EVENTMASK 0x3fff

#define IRQ1_0_TBE 0
#define IRQ1_1_DSKBLK 1
#define IRQ1_2_SOFT 2
#define IRQ2_CIAA 3
#define IRQ3_0_COPPER 4
#define IRQ3_1_VBL 5
#define IRQ3_2_BLIT 6
#define IRQ4_AUD 7
#define IRQ5_0_RBF 11
#define IRQ5_1_DSKSYN 12
#define IRQ6_0_CIAB 13
#define EXBUSERROR 25
#define EXODDADDRESS 26
#define EXILLEGAL 27
#define EXDIVBYZERO 28
#define EXPRIV 31
#define EXTRAP 32
#define EVSTOP 33
#define EXFLINE 34

extern ULO logbuffer[EVENTCOUNT][16];
extern int logfirst;
extern int loglast;

/* Logging flags */
extern BOOLE logserialtransmitbufferemptyirq;
extern BOOLE logdiskdmatransferdoneirq;
extern BOOLE logsoftwareirq;
extern BOOLE logciaairq;
extern BOOLE logcopperirq;
extern BOOLE logverticalblankirq;
extern BOOLE logblitterreadyirq;
extern BOOLE logaudioirq;
extern BOOLE logserialreceivebufferfullirq;
extern BOOLE logdisksyncvaluerecognizedirq;
extern BOOLE logciabirq;
extern BOOLE logstop;
extern BOOLE logbuserrorex;
extern BOOLE logoddaddressex;
extern BOOLE logillegalinstructionex;
extern BOOLE logdivisionby0ex;
extern BOOLE logprivilegieviolationex;
extern BOOLE logtrapex;
extern BOOLE logfline;
