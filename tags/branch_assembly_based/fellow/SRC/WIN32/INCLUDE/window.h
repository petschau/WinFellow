#ifndef WINDOW_H
#define WINDOW_H

#define NEW_WINDOW

typedef void (*window_create_listener_func)(HWND hwnd);
typedef void (*window_destroy_listener_func)(HWND hwnd);
typedef void (*window_active_listener_func)(HWND hwnd, BOOLE active_status, BOOLE iconic, BOOLE syskey);
typedef void (*window_palette_listener_func)(HWND hwnd);
typedef void (*window_close_listener_func)(HWND hwnd);
typedef void (*window_displaychange_listener_func)(HWND hwnd);

RECT *windowGetClientRectScreen(void);
HWND windowHwnd(void);
void windowShow(void);
void windowHide(void);
BOOLE windowAddCreateListener(window_create_listener_func listener_func);
BOOLE windowAddDestroyListener(window_destroy_listener_func listener_func);
BOOLE windowAddActiveListener(window_active_listener_func listener_func);
BOOLE windowAddPaletteListener(window_palette_listener_func listener_func);
BOOLE windowAddCloseListener(window_close_listener_func listener_func);
BOOLE windowAddDisplayChangeListener(window_displaychange_listener_func listener_func);
void windowRemoveListeners(void);
void windowActiveWait(void);
BOOLE windowEmulationStart(BOOLE on_desktop, ULO width, ULO height, ULO depth);
void windowEmulationStop(void);
void windowStartup(HINSTANCE hinstance, STR *title);
void windowShutdown(void);

#endif
