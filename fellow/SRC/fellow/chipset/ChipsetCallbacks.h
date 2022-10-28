#pragma once

#include "fellow/api/defs.h"

class IBitplaneShifter_AddDataCallback
{
public:
  virtual void operator()(const UWO *data) = 0;
  virtual ~IBitplaneShifter_AddDataCallback() = default;
};

class IBitplaneShifter_FlushCallback
{
public:
  virtual void operator()() = 0;
  virtual ~IBitplaneShifter_FlushCallback() = default;
};

class IColorChangeEventHandler
{
public:
  virtual void ColorChangedHandler(const unsigned int colorIndex, const UWO color12Bit, const UWO halfbriteColor12Bit) = 0;
  virtual ~IColorChangeEventHandler() = default;
};
