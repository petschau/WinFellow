#ifndef BUS_H
#define BUS_H

//#define ENABLE_BUS_EVENT_LOGGING

/* Standard Fellow Module functions */

extern void busSaveState(FILE *F);
extern void busLoadState(FILE *F);
extern void busEmulationStart(void);
extern void busEmulationStop(void);
extern void busSoftReset(void);
extern void busHardReset(void);
extern void busStartup(void);
extern void busShutdown(void);

void busEndOfLine(void);
void busEndOfFrame(void);

#ifdef ENABLE_BUS_EVENT_LOGGING
extern void busLogCpu(STR *s);
extern void busLogCpuException(STR *s);
#endif

typedef void (*busEventHandler)();
typedef struct bus_event_struct
{
  struct bus_event_struct *next;
  struct bus_event_struct *prev;
  ULO cycle;
  ULO priority;
  busEventHandler handler;
} bus_event;

extern void busInsertEventWithNullCheck(bus_event *ev);
extern void busInsertEvent(bus_event *event);
extern void busRemoveEvent(bus_event *event);

typedef struct bus_state_
{
  ULL frame_no;
  ULO cycle;
  bus_event *events;
} bus_state;

extern bus_state bus;

extern void busRun(void);
extern void busDebugNew(void);

extern ULO busGetCycle(void);
extern ULO busGetRasterX(void);
extern ULO busGetRasterY(void);
extern ULL busGetRasterFrameCount(void);

#define BUS_CYCLE_PER_LINE 227
#define BUS_LINES_PER_FRAME 312
#define BUS_CYCLE_PER_FRAME (BUS_CYCLE_PER_LINE*BUS_LINES_PER_FRAME)
#define BUS_CYCLE_DISABLE 0xffffffff

extern bus_event cpuEvent;
extern bus_event copperEvent;
extern bus_event eolEvent;
extern bus_event eofEvent;
extern bus_event ciaEvent;
extern bus_event blitterEvent;

#endif
