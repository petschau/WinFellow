#if !defined( USE_DX3 ) && !defined( USE_DX5 )
	#error "you must define USE_DX3 or USE_DX5
#endif

#ifdef USE_DX3
	#define DIRECTINPUT_VERSION 0x0300
#endif // USE_DX3

#ifdef USE_DX5
	#define DIRECTINPUT_VERSION 0x0500
#endif // USE_DX5

#include <dinput.h>
