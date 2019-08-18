#include "fellow/vm/Clocks.h"
#include "draw_interlace_control.h"
#include "fellow/scheduler/Scheduler.h"

using namespace fellow::api::vm;

namespace fellow::vm
{
  uint64_t Clocks::GetFrameNumber()
  {
    return scheduler.GetRasterFrameCount();
  }

  uint32_t Clocks::GetFrameCycle()
  {
    return scheduler.GetFrameCycle();
  }

  FrameLayoutType Clocks::GetFrameLayout()
  {
    return FrameLayoutType::PAL;
  }

  bool Clocks::GetFrameIsInterlaced()
  {
    return drawGetFrameIsInterlaced();
  }

  bool Clocks::GetFrameIsLong()
  {
    return drawGetFrameIsLong();
  }

  unsigned int Clocks::GetLineNumber()
  {
    return scheduler.GetRasterY();
  }

  unsigned int Clocks::GetLineCycle()
  {
    return scheduler.GetRasterX();
  }

  bool Clocks::GetLineIsLong()
  {
    return true;
  }
}