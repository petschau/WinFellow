#ifndef WGUI_H
#define WGUI_H

/*===========================================================================*/
/* This is the generic interface that must be implemented to create a GUI    */
/* Fellow                                                                    */
/*===========================================================================*/

#ifdef WGUI

extern void wguiPreStartup(void);
extern void wguiStartup(void);
extern void wguiShutdown(void);
extern BOOLE wguiEnter(void);
extern void wguiRequester(STR *line1, STR *line2, STR *line3);

#else

/* null implementation */

static void wguiPreStartup(void) {}
static void wguiStartup(void) {}
static void wguiShutdown(void) {}
static void wguiEnter(void) {}
static void wguiRequester(STR *line1, STR *line2, STR *line3) {}


#endif /* End of WGUI */

#endif /* End of WGUI_H */

