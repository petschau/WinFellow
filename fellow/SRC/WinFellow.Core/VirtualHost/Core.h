#pragma once

#include "Driver/Drivers.h"
#include "Service/ILog.h"
#include "Service/IFileops.h"
#include "CustomChipset/Sound/Sound.h"
#include "CustomChipset/Registers.h"
#include "CustomChipset/RegisterUtility.h"
#include "IO/Uart.h"
#include "IO/RtcOkiMsm6242rs.h"

class Core
{
public:
  CustomChipset::Registers Registers;
  CustomChipset::RegisterUtility RegisterUtility;
  Sound* Sound;
  Uart* Uart;
  RtcOkiMsm6242rs* RtcOkiMsm6242rs;

  Drivers Drivers;
  Service::ILog* Log;
  Service::IFileops* Fileops;

  Core();
  ~Core();
};

extern Core _core;
