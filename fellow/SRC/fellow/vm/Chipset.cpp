#include "fellow/vm/Chipset.h"

using namespace fellow::api::vm;

namespace fellow::vm
{

  Chipset::Chipset(ICopper &copper, IDisplay &display) : IChipset(copper, display)
  {
  }

  Chipset::~Chipset() = default;
}