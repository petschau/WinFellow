#pragma once

#include <cstdint>

namespace fellow::api::vm
{
  enum class FrameLayoutType
  {
    PAL = 0,
    NTSC = 1,
    Custom = 2
  };

  class IClocks
  {
  public:
    virtual uint64_t GetFrameNumber() = 0;
    virtual uint32_t GetFrameCycle() = 0;
    virtual FrameLayoutType GetFrameLayout() = 0;
    virtual bool GetFrameIsInterlaced() = 0;
    virtual bool GetFrameIsLong() = 0;

    virtual unsigned int GetLineNumber() = 0;
    virtual unsigned int GetLineCycle() = 0;
    virtual bool GetLineIsLong() = 0;

    virtual ~IClocks() = default;
  };
}
