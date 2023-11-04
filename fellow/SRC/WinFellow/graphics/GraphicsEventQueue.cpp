#include "Defs.h"

#include "Graphics.h"
#include "BUS.H"

void GraphicsEventQueue::Clear()
{
  _events = nullptr;
}

void GraphicsEventQueue::Remove(GraphicsEvent *graphics_event)
{
  bool found = false;
  for (GraphicsEvent *tmp = _events; tmp != nullptr; tmp = tmp->_next)
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

  if (graphics_event->_prev == nullptr)
  {
    _events = graphics_event->_next;
  }
  else
  {
    graphics_event->_prev->_next = graphics_event->_next;
  }
  if (graphics_event->_next != nullptr)
  {
    graphics_event->_next->_prev = graphics_event->_prev;
  }
  graphics_event->_prev = graphics_event->_next = nullptr;
}

void GraphicsEventQueue::Insert(GraphicsEvent *graphics_event)
{
  if (_events == nullptr)
  {
    graphics_event->_prev = graphics_event->_next = nullptr;
    _events = graphics_event;
  }
  else
  {
    GraphicsEvent *tmp;
    GraphicsEvent *tmp_prev = nullptr;
    for (tmp = _events; tmp != nullptr; tmp = tmp->_next)
    {
      if (graphics_event->_arriveTime < tmp->_arriveTime || ((graphics_event->_arriveTime == tmp->_arriveTime) && (graphics_event->_priority > tmp->_priority)))
      {
        graphics_event->_next = tmp;
        graphics_event->_prev = tmp_prev;
        tmp->_prev = graphics_event;
        if (tmp_prev == nullptr)
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
    graphics_event->_next = nullptr;
  }
}

GraphicsEvent *GraphicsEventQueue::Pop()
{
  GraphicsEvent *tmp = _events;
  _events = tmp->_next;
  if (_events != nullptr)
  {
    _events->_prev = nullptr;
  }
  return tmp;
}

void GraphicsEventQueue::Run(uint32_t untilTime)
{
  while (_events->_arriveTime <= untilTime)
  {
    GraphicsEvent *graphics_event = Pop();
    uint32_t arrive_time = graphics_event->_arriveTime;
    graphics_event->Handler(arrive_time / GetCylindersPerLine(), arrive_time % GetCylindersPerLine());
  }
  GraphicsContext.PixelSerializer.OutputCylindersUntil(untilTime / GetCylindersPerLine(), untilTime % GetCylindersPerLine());
}
