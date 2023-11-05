#include "VirtualHost/Core.h"

// For the time being this has to be a singleton
Core _core = Core();

Core::Core()
  : Registers(),
    RegisterUtility(Registers),
    ChipsetInformation(),
    Sound(nullptr),
    Uart(nullptr),
    RtcOkiMsm6242rs(nullptr),
    HardfileHandler(nullptr),
    Drivers(),
    Log(nullptr),
    Fileops(nullptr),
    FileInformation(nullptr),
    Hud(nullptr),
    RP(nullptr)
{
}

Core::~Core()
{
}
