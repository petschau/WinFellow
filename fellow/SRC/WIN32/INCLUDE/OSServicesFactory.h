#pragma once

#include "fellow/api/service/IFileops.h"
#include "fellow/api/service/IPerformanceCounter.h"

namespace fellow::os
{
  fellow::api::IFileops &GetIFileopsImplementation();
  fellow::api::IPerformanceCounterFactory &GetIPerformanceCounterFactoryImplementation();
}
