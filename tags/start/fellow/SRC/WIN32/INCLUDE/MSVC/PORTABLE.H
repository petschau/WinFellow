#ifndef PORTABLE_H
#define PORTABLE_H


/*====================================================*/
/* Wrapper definitions for system dependent functions */
/* Windows, MSVC++                                    */
/*====================================================*/

#include <windows.h>

/*=======*/
/* stdio */
/*=======*/

#include <stdio.h>
#define fileno _fileno

/*================================*/
/* stat structure and field names */
/*================================*/

#include <sys/stat.h>

/*=====================*/
/* string manipulation */
/*=====================*/

#include <string.h>

#define strcmpi _strcmpi
#define strnicmp _strnicmp


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

#define FELLOW_LONG_LONG __int64

#endif /* PORTABLE_H */

