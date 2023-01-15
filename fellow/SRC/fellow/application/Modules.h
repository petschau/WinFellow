#pragma once

#include "fellow/chipset/Keyboard.h"
#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/scheduler/Scheduler.h"

class Modules
{
public:
  Keyboard *Keyboard{};
  fellow::api::modules::IHardfileHandler *HardfileHandler{};
  BitplaneRegisters *BitplaneRegisters{};
};
