/*===========================================================================*/
/* Windows data, misc.                                                       */
/*===========================================================================*/

#ifndef wdbg_H
#define wdbg_H

extern void wdbgDebugSessionRun(HWND parent);

typedef enum {
  DBG_STEP,
  DBG_STEP_OVER,
  DBG_RUN
} dbg_operations;



#endif /* wdbg_H */

