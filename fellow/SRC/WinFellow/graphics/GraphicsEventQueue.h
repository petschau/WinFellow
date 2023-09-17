#ifndef GRAPHICS_EVENT_QUEUE_H
#define GRAPHICS_EVENT_QUEUE_H

#include "DEFS.H"
#include "GraphicsEvent.h"

class GraphicsEventQueue
{
private:
  GraphicsEvent *_events;

  void RunQueue(uint32_t untilTime);

public:
  const static uint32_t GRAPHICS_ARRIVE_TIME_NONE = 0xffffffff;

  static uint32_t GetCylindersPerLine() {return busGetCyclesInThisLine()*2;}

  void Clear(void);
  GraphicsEvent* Pop(void);
  void Insert(GraphicsEvent *graphics_event);
  void Remove(GraphicsEvent *graphics_event);

  void Run(uint32_t untilTime);
};

#endif
