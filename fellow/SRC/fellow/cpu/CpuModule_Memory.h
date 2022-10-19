#pragma once

// This header pulls in the memory interface functions

#ifdef CPUMODULE_MEMORY_TEST

// For testing

#include "FMEM_test.h"

#else

#include "FMEM.H"

// For trapping into the "UAE filesystem" driver
#include "UAE2FELL.H"
#include "AUTOCONF.H"

#endif
