#pragma once

#include "Driver/Drivers.h"
#include "Module/Hardfile/IHardfileHandler.h"
#include "Service/ILog.h"
#include "Service/IFileops.h"
#include "Service/IFileInformation.h"
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
  Sound *Sound;
  Uart *Uart;
  RtcOkiMsm6242rs *RtcOkiMsm6242rs;

  Module::Hardfile::IHardfileHandler *HardfileHandler;

  Drivers Drivers;
  Service::ILog *Log;
  Service::IFileops *Fileops;
  Service::IFileInformation *FileInformation;

  Core();
  ~Core();
};

extern Core _core;
