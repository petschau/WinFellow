#ifndef KBD_H
#define KBD_H

typedef enum {
  EVENT_INSERT_DF0,
  EVENT_INSERT_DF1,
  EVENT_INSERT_DF2,
  EVENT_INSERT_DF3,
  EVENT_EJECT_DF0,
  EVENT_EJECT_DF1,
  EVENT_EJECT_DF2,
  EVENT_EJECT_DF3,
  EVENT_EXIT,
  EVENT_BMP_DUMP,
  EVENT_RESOLUTION_NEXT,
  EVENT_RESOLUTION_PREV,
  EVENT_SCALEY_NEXT,
  EVENT_SCALEY_PREV,
  EVENT_SCALEX_NEXT,
  EVENT_SCALEX_PREV,
  EVENT_SCROLL_UP,
  EVENT_SCROLL_DOWN,
  EVENT_SCROLL_LEFT,
  EVENT_SCROLL_RIGHT,
  EVENT_HARD_RESET,
  EVENT_JOY0_UP_ACTIVE,
  EVENT_JOY0_UP_INACTIVE,
  EVENT_JOY0_DOWN_ACTIVE,
  EVENT_JOY0_DOWN_INACTIVE,
  EVENT_JOY0_LEFT_ACTIVE,
  EVENT_JOY0_LEFT_INACTIVE,
  EVENT_JOY0_RIGHT_ACTIVE,
  EVENT_JOY0_RIGHT_INACTIVE,
  EVENT_JOY0_FIRE0_ACTIVE,
  EVENT_JOY0_FIRE0_INACTIVE,
  EVENT_JOY0_FIRE1_ACTIVE,
  EVENT_JOY0_FIRE1_INACTIVE,
  EVENT_JOY1_UP_ACTIVE,
  EVENT_JOY1_UP_INACTIVE,
  EVENT_JOY1_DOWN_ACTIVE,
  EVENT_JOY1_DOWN_INACTIVE,
  EVENT_JOY1_LEFT_ACTIVE,
  EVENT_JOY1_LEFT_INACTIVE,
  EVENT_JOY1_RIGHT_ACTIVE,
  EVENT_JOY1_RIGHT_INACTIVE,
  EVENT_JOY1_FIRE0_ACTIVE,
  EVENT_JOY1_FIRE0_INACTIVE,
  EVENT_JOY1_FIRE1_ACTIVE,
  EVENT_JOY1_FIRE1_INACTIVE} kbd_event;

#define KBDBUFFERLENGTH 512
#define KBDBUFFERMASK   511

typedef struct {
  UBY buffer[KBDBUFFERLENGTH];
  ULO inpos;
  ULO outpos;
} kbd_buffer_type;

typedef struct {
  kbd_buffer_type scancodes;
  kbd_buffer_type eventsEOL;
  kbd_buffer_type eventsEOF;
} kbd_state_type;

extern kbd_state_type kbd_state;

/* Add an EOL event to the core, used by kbddrv */

#define kbdEventEOLAdd(EVENTID) {\
  kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.inpos & KBDBUFFERMASK] = EVENTID; \
  kbd_state.eventsEOL.inpos++; \
}

/* Add an EOF event to the core, used by kbddrv */

#define kbdEventEOFAdd(EVENTID) {\
  kbd_state.eventsEOF.buffer[kbd_state.eventsEOF.inpos & KBDBUFFERMASK] = EVENTID; \
  kbd_state.eventsEOF.inpos++; \
}

/* Add a key to the core, used by kbddrv */

#define kbdKeyAdd(KEYCODE) {\
  kbd_state.scancodes.buffer[(kbd_state.scancodes.inpos) & KBDBUFFERMASK] = (KEYCODE); \
  kbd_state.scancodes.inpos++; \
}

extern void kbdHardReset(void);
extern void kbdEmulationStart(void);
extern void kbdEmulationStop(void);
extern void kbdStartup(void);
extern void kbdShutdown(void);


extern volatile ULO f12pressed;
extern UBY insert_dfX[4];

#endif
