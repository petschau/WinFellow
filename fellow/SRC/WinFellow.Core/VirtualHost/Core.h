#pragma once

#include "Driver/Drivers.h"
#include "Service/ILog.h"
#include "Service/IFileops.h"
#include "CustomChipset/Sound/Sound.h"
#include "CustomChipset/Registers.h"
#include "CustomChipset/RegisterUtility.h"
#include "IO/Uart.h"

class Core
{
public:
  CustomChipset::Registers Registers;
  CustomChipset::RegisterUtility RegisterUtility;
  Sound* Sound;
  Uart* Uart;

  Drivers Drivers;
  Service::ILog* Log;
  Service::IFileops* Fileops;

  Core();
  ~Core();
};

extern Core _core;
