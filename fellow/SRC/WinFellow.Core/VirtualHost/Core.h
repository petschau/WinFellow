#pragma once

#include "Driver/Drivers.h"
#include "CustomChipset/Sound/Sound.h"
#include "CustomChipset/Registers.h"
#include "CustomChipset/RegisterUtility.h"

class Core
{
public:
  CustomChipset::Registers Registers;
  CustomChipset::RegisterUtility RegisterUtility;
  Drivers Drivers;

  Sound *Sound;

  Core();
  ~Core();
};

extern Core _core;
