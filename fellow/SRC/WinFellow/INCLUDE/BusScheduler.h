#pragma once

#include <cstdio>
#include <cstdint>

// #define ENABLE_BUS_EVENT_LOGGING

/* Standard Fellow Module functions */

extern void busSaveState(FILE *F);
extern void busLoadState(FILE *F);
extern void busEmulationStart();
extern void busEmulationStop();
extern void busSoftReset();
extern void busHardReset();
extern void busStartup();
extern void busShutdown();
extern void busDetermineCpuInstructionEventHandler();

void busEndOfLine();
void busEndOfFrame();

#ifdef ENABLE_BUS_EVENT_LOGGING
extern void busLogCpu(char *s);
extern void busLogCpuException(char *s);
#endif

typedef void (*busEventHandler)();
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

extern void busRun();
extern void busDebugStepOneInstruction();

extern uint32_t busGetCycle();
extern uint32_t busGetRasterX();
extern uint32_t busGetRasterY();
extern uint64_t busGetRasterFrameCount();

extern uint32_t busGetCyclesInThisLine();
extern uint32_t busGetLinesInThisFrame();
extern uint32_t busGetCyclesInThisFrame();

extern uint32_t busGetMaxLinesInFrame();

extern void busSetScreenLimits(bool is_long_frame);

// #define BUS_CYCLE_PER_FRAME (BUS_CYCLE_PER_LINE*BUS_LINES_PER_FRAME)
#define BUS_CYCLE_DISABLE 0xffffffff

extern bus_event cpuEvent;
extern bus_event copperEvent;
extern bus_event eolEvent;
extern bus_event eofEvent;
extern bus_event ciaEvent;
extern bus_event blitterEvent;
extern bus_event interruptEvent;
