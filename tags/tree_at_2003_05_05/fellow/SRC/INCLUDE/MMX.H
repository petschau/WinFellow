#ifndef MMX_H
#define MMX_H

/* Rename assembly functions */

#define detectMMX ASMRENAME(detectMMX)

/* Declare assembly functions */

extern BOOLE detectMMX(void);

/* End of assembly function declarations */

#endif
