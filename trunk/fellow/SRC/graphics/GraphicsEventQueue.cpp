#include "DEFS.H"

#ifdef GRAPH2

#include "Graphics.h"
#include "BUS.H"

void GraphicsEventQueue::Clear(void)
{
  _events = 0;
}

void GraphicsEventQueue::Remove(GraphicsEvent *graphics_event)
{
  bool found = false;
  for (GraphicsEvent *tmp = _events; tmp != 0; tmp = tmp->_next)
  {
    if (tmp == graphics_event)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    return;
  }

  if (graphics_event->_prev == 0)
  {
    _events = graphics_event->_next;
  }
  else
  {
    graphics_event->_prev->_next = graphics_event->_next;
  }
  if (graphics_event->_next != 0)
  {
    graphics_event->_next->_prev = graphics_event->_prev;
  }
  graphics_event->_prev = graphics_event->_next = 0;
}

void GraphicsEventQueue::Insert(GraphicsEvent *graphics_event)
{
  if (_events == 0)
  {
    graphics_event->_prev = graphics_event->_next = 0;
    _events = graphics_event;
  }
  else
  {
    GraphicsEvent *tmp;
    GraphicsEvent *tmp_prev = 0;
    for (tmp = _events; tmp != 0; tmp = tmp->_next)
    {
      if (graphics_event->_arriveTime < tmp->_arriveTime 
        || ((graphics_event->_arriveTime == tmp->_arriveTime) && (graphics_event->_priority > tmp->_priority)))
      {
        graphics_event->_next = tmp;
        graphics_event->_prev = tmp_prev;
        tmp->_prev = graphics_event;
        if (tmp_prev == 0)
        {
          _events = graphics_event; /* In front */
        }
        else
        {
          tmp_prev->_next = graphics_event;
        }
        return;
      }
      tmp_prev = tmp;
    }
    tmp_prev->_next = graphics_event; /* At end */
    graphics_event->_prev = tmp_prev;
    graphics_event->_next = 0;
  }
}

GraphicsEvent *GraphicsEventQueue::Pop(void)
{
  GraphicsEvent *tmp = _events;
  _events = tmp->_next;
  if (_events != 0)
  {
    _events->_prev = 0;
  }
  return tmp;
}

void GraphicsEventQueue::Run(ULO untilTime)
{
  while (_events->_arriveTime <= untilTime)
  {
    GraphicsEvent *graphics_event = Pop();
    ULO arrive_time = graphics_event->_arriveTime;
    graphics_event->Handler(arrive_time / GRAPHICS_CYLINDERS_PER_LINE, arrive_time % GRAPHICS_CYLINDERS_PER_LINE);
  }
  GraphicsContext.PixelSerializer.OutputCylindersUntil(untilTime / GRAPHICS_CYLINDERS_PER_LINE, untilTime % GRAPHICS_CYLINDERS_PER_LINE);
}

#endif
