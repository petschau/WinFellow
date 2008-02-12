#ifndef SSE_H
#define SSE_H

/* Rename assembly functions */

#define detectSSE ASMRENAME(detectSSE)

/* Declare assembly functions */

extern BOOLE detectSSE(void);

/* End of assembly function declarations */

#endif
