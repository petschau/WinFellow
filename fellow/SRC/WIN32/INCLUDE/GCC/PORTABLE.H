#ifndef PORTABLE_H
#define PORTABLE_H


/*====================================================*/
/* Wrapper definitions for system dependent functions */
/* Windows, Borland C                                 */
/*====================================================*/

#include <windows.h>

/*=======*/
/* stdio */
/*=======*/

#include <stdio.h>

/*================================*/
/* stat structure and field names */
/*================================*/

#include <sys/stat.h>

/*=====================*/
/* string manipulation */
/*=====================*/

#include <string.h>

//#define _stricmp strcmpi
//#define stricmp _stricmp
//#define strnicmp _strnicmp


/*====================================*/
/* memory manipulation and allocation */
/*====================================*/

#include <memory.h>
#include <stdlib.h>


/*========*/
/* setjmp */
/*========*/

#include <setjmp.h>


/*=====================*/
/* Integer types       */
/*=====================*/

#define FELLOW_LONG_LONG long long

#endif /* PORTABLE_H */

