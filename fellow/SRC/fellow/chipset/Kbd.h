#pragma once

enum class kbd_event
{
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
  EVENT_JOY0_AUTOFIRE0_ACTIVE,
  EVENT_JOY0_AUTOFIRE0_INACTIVE,
  EVENT_JOY0_AUTOFIRE1_ACTIVE,
  EVENT_JOY0_AUTOFIRE1_INACTIVE,
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
  EVENT_JOY1_FIRE1_INACTIVE,
  EVENT_JOY1_AUTOFIRE0_ACTIVE,
  EVENT_JOY1_AUTOFIRE0_INACTIVE,
  EVENT_JOY1_AUTOFIRE1_ACTIVE,
  EVENT_JOY1_AUTOFIRE1_INACTIVE,
  EVENT_DF1_INTO_DF0,
  EVENT_DF2_INTO_DF0,
  EVENT_DF3_INTO_DF0,
  EVENT_DEBUG_TOGGLE_RENDERER,
  EVENT_DEBUG_TOGGLE_LOGGING,
  EVENT_NONE
};

constexpr unsigned int KBDBUFFERLENGTH = 512;
constexpr unsigned int KBDBUFFERMASK = 511;

template <typename T> struct kbd_buffer_type
{
  T buffer[KBDBUFFERLENGTH];
  ULO inpos;
  ULO outpos;
};

struct kbd_state_type
{
  kbd_buffer_type<UBY> scancodes;
  kbd_buffer_type<kbd_event> eventsEOL;
  kbd_buffer_type<kbd_event> eventsEOF;
};

extern kbd_state_type kbd_state;

extern void kbdEventEOLAdd(kbd_event eventId);
extern void kbdEventEOFAdd(kbd_event eventId);
extern void kbdKeyAdd(UBY keyCode);

extern void kbdHardReset();
extern void kbdEmulationStart();
extern void kbdEmulationStop();
extern void kbdStartup();
extern void kbdShutdown();

extern void kbdQueueHandler();
extern void kbdEventEOLHandler();

void kbdEventEOFHandler();

extern void kbdQueueHandler();
extern void kbdEventEOLHandler();

extern UBY insert_dfX[4];
