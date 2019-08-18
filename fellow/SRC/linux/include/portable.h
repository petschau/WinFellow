#ifndef PORTABLE_H
#define PORTABLE_H


/*====================================================*/
/* Wrapper definitions for system dependent functions */
/*====================================================*/

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

#define stricmp strcasecmp


/*====================================*/
/* memory manipulation and allocation */
/*====================================*/

#include <memory.h>
#include <stdlib.h>


/*========*/
/* setjmp */
/*========*/

#include <setjmp.h>

// Functions such as access()
#include <unistd.h>

#define localtime_s(X, Y) localtime_r(Y, X)

/*=====================*/
/* Integer types       */
/*=====================*/

#define FELLOW_LONG_LONG long long

#endif /* PORTABLE_H */

