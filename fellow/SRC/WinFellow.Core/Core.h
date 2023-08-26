#pragma once

#include "CustomChipset/Registers.h"
#include "CustomChipset/RegisterUtility.h"

class Core
{
public:
  CustomChipset::Registers Registers;
  CustomChipset::RegisterUtility RegisterUtility;

  Core();
  ~Core();
};
