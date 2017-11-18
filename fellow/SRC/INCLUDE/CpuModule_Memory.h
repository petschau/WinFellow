#ifndef CPUMODULE_MEMORY
#define CPUMODULE_MEMORY

// This header pulls in the memory interface functions

#ifdef CPUMODULE_MEMORY_TEST

// For testing

#include "FMEM_test.h"

#else

#include "FMEM.H"

// For trapping into the "UAE filesystem" driver
#include "uae2fell.h"
#include "autoconf.h"

#endif


#endif