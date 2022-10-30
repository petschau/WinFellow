#pragma once

/*====================================================*/
/* Wrapper definitions for system dependent functions */
/* Windows, MSVC++                                    */
/*====================================================*/

#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>

/*=======*/
/* stdio */
/*=======*/

#include <cstdio>
#include <io.h>

#define fileno _fileno
#define access _access

//====================
// string manipulation
//====================

#include <cstring>

// These assume the compiler uses Microsoft library
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strlwr _strlwr

/*====================================*/
/* memory manipulation and allocation */
/*====================================*/

#include <memory.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <cstdlib>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <crtdbg.h>

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW

#endif

// 64-bit signed integer
#define FELLOW_LONG_LONG __int64
