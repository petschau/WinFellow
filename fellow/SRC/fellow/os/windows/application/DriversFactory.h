#pragma once

#include "fellow/api/Drivers.h"

class DriversFactory
{
public:
  static fellow::api::Drivers *Create();
  static void Delete(fellow::api::Drivers *drivers);
};
