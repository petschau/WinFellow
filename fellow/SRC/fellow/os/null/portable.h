#pragma once

// stricmp is typically Microsoft library specific
// Posix equivalent is strcasecmp
#define stricmp strcasecmp

// Microsoft uses io.h for what is unistd.h on other systems, it has access() etc.
#include <unistd.h>
