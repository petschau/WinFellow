#pragma once

#include "portable.h"

// Maximum values for memory, don't change
constexpr auto CHIPMEM = 0x200000;
constexpr auto FASTMEM = 0x800000;
constexpr auto BOGOMEM = 0x1c0000;
constexpr auto KICKMEM = 0x080000;

// Fellow types to ensure correct sizes
// Kinda obsolete as C++ now has sized types, but lots of code use the typedef ones
#include <cstdint>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

// Include ptrdiff_t
#include <cstddef>
using std::ptrdiff_t;

typedef uint8_t UBY;
typedef uint16_t UWO;
typedef uint32_t ULO;
typedef uint64_t ULL;
typedef int8_t BYT;
typedef int16_t WOR;
typedef int32_t LON;
typedef int64_t LLO;
typedef int BOOLE;

#define FALSE 0
#define TRUE 1
typedef char STR;

#ifndef X64
#define PTR_TO_INT(i) ((ULO)i)
#define PTR_TO_INT_MASK_TYPE(i) ((ULO)i)
#endif
#ifdef X64
#define PTR_TO_INT(i) ((ULL)i)
#define PTR_TO_INT_MASK_TYPE(i) ((ULL)i)
#endif

// Filename length used throughout the code

#define CFG_FILENAME_LENGTH 256

extern UBY configromname[];

typedef void (*planar2chunkyroutine)();

#ifdef _DEBUG
#define F_ASSERT(expr)                                                                                                                                                                                 \
  if (!(expr)) throw;
#else
#define F_ASSERT(expr) ;
#endif

#include <setjmp.h>

#include <cstring>
using std::strlen;

#include <cstdlib>
using std::malloc;