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
extern void busDetermineCpuInstructionEventHandler(void);

void busEndOfLine(void);
void busEndOfFrame(void);

#ifdef ENABLE_BUS_EVENT_LOGGING
extern void busLogCpu(char *s);
extern void busLogCpuException(char *s);
#endif

typedef void (*busEventHandler)(void);
typedef struct bus_event_struct
{
  struct bus_event_struct *next;
  struct bus_event_struct *prev;
  uint32_t cycle;
  uint32_t priority;
  busEventHandler handler;
} bus_event;

extern void busInsertEventWithNullCheck(bus_event *ev);
extern void busInsertEvent(bus_event *event);
extern void busRemoveEvent(bus_event *event);

typedef struct bus_screen_limits_
{
  uint32_t cycles_in_this_line;
  uint32_t cycles_in_this_frame;
  uint32_t lines_in_this_frame;
  uint32_t max_cycles_in_line;
  uint32_t max_lines_in_frame;
} bus_screen_limits;

typedef struct bus_state_
{
  uint64_t frame_no;
  uint32_t cycle;
  bus_screen_limits *screen_limits;
  bus_event *events;
} bus_state;

extern bus_state bus;

extern void busRun(void);
extern void busDebugStepOneInstruction(void);

extern uint32_t busGetCycle(void);
extern uint32_t busGetRasterX(void);
extern uint32_t busGetRasterY(void);
extern uint64_t busGetRasterFrameCount(void);

extern uint32_t busGetCyclesInThisLine(void);
extern uint32_t busGetLinesInThisFrame(void);
extern uint32_t busGetCyclesInThisFrame(void);

extern uint32_t busGetMaxLinesInFrame(void);

extern void busSetScreenLimits(bool is_long_frame);

//#define BUS_CYCLE_PER_FRAME (BUS_CYCLE_PER_LINE*BUS_LINES_PER_FRAME)
#define BUS_CYCLE_DISABLE 0xffffffff

extern bus_event cpuEvent;
extern bus_event copperEvent;
extern bus_event eolEvent;
extern bus_event eofEvent;
extern bus_event ciaEvent;
extern bus_event blitterEvent;
extern bus_event interruptEvent;

#endif
