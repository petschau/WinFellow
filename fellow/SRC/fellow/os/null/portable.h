#pragma once

// "stricmp" is Microsoft library specific
// Posix equivalent is "strcasecmp"
#define stricmp strcasecmp

// Fix Microsoft localtime_s parameter reversal vs C11 standard
// This version of portable.h expects localtime_s to follow the standard, no parameter reversal
// Mac does not have localtime_s (?), so fall back to localtime_r
#define localtime_s_wrapper(thetime, timedata) localtime_r(thetime, timedata)
