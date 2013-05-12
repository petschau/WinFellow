#ifndef GRAPHICS_EVENT_QUEUE_H
#define GRAPHICS_EVENT_QUEUE_H

#include "DEFS.H"
#include "GraphicsEvent.h"

class GraphicsEventQueue
{
private:
  GraphicsEvent *_events;

  void RunQueue(ULO untilTime);

public:
  const static ULO GRAPHICS_ARRIVE_TIME_NONE = 0xffffffff;
  const static ULO GRAPHICS_CYLINDERS_PER_FRAME = BUS_CYCLE_PER_FRAME*2;
  const static ULO GRAPHICS_CYLINDERS_PER_LINE = BUS_CYCLE_PER_LINE*2;

  void Clear(void);
  GraphicsEvent* Pop(void);
  void Insert(GraphicsEvent *graphics_event);
  void Remove(GraphicsEvent *graphics_event);

  void Run(ULO untilTime);
};

#endif
