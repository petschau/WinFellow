#include "VirtualHost/Core.h"

// For the time being this has to be a singleton
Core _core = Core();

Core::Core() : Registers(), RegisterUtility(Registers), Drivers()
{
}

Core::~Core()
{
}
