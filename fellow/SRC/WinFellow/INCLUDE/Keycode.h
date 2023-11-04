#pragma once

/* Amiga keys */

#define A_NONE 0xff

#define A_ESCAPE 0x45 /* First row */
#define A_F1 0x50
#define A_F2 0x51
#define A_F3 0x52
#define A_F4 0x53
#define A_F5 0x54
#define A_F6 0x55
#define A_F7 0x56
#define A_F8 0x57
#define A_F9 0x58
#define A_F10 0x59

#define A_GRAVE 0x00 /* Second row */
#define A_1 0x01
#define A_2 0x02
#define A_3 0x03
#define A_4 0x04
#define A_5 0x05
#define A_6 0x06
#define A_7 0x07
#define A_8 0x08
#define A_9 0x09
#define A_0 0x0A
#define A_MINUS 0x0B
#define A_EQUALS 0x0C
#define A_BACKSLASH2 0x0D
#define A_BACKSPACE 0x41

#define A_TAB 0x42 /* Third row */
#define A_Q 0x10
#define A_W 0x11
#define A_E 0x12
#define A_R 0x13
#define A_T 0x14
#define A_Y 0x15
#define A_U 0x16
#define A_I 0x17
#define A_O 0x18
#define A_P 0x19
#define A_LEFT_BRACKET 0x1a
#define A_RIGHT_BRACKET 0x1b
#define A_RETURN 0x44

#define A_CTRL 0x63 /* Fourth row */
#define A_CAPS_LOCK 0x62
#define A_A 0x20
#define A_S 0x21
#define A_D 0x22
#define A_F 0x23
#define A_G 0x24
#define A_H 0x25
#define A_J 0x26
#define A_K 0x27
#define A_L 0x28
#define A_SEMICOLON 0x29
#define A_APOSTROPHE 0x2A
#define A_BACKSLASH 0x2B

#define A_LEFT_SHIFT 0x60 /* Fifth row */
#define A_LESS_THAN 0x30
#define A_Z 0x31
#define A_X 0x32
#define A_C 0x33
#define A_V 0x34
#define A_B 0x35
#define A_N 0x36
#define A_M 0x37
#define A_COMMA 0x38
#define A_PERIOD 0x39
#define A_SLASH 0x3A
#define A_RIGHT_SHIFT 0x61

#define A_LEFT_ALT 0x64 /* Sixth row */
#define A_LEFT_AMIGA 0x66
#define A_SPACE 0x40
#define A_RIGHT_AMIGA 0x67
#define A_RIGHT_ALT 0x65

#define A_NUMPAD_LEFT_BRACKET 0xff  /* Don't know the scancode for this one */
#define A_NUMPAD_RIGHT_BRACKET 0xff /* Don't know the scancode for this one */
#define A_NUMPAD_DIVIDE 0x5c
#define A_NUMPAD_MULTIPLY 0x5d
#define A_NUMPAD_7 0x3d
#define A_NUMPAD_8 0x3e
#define A_NUMPAD_9 0x3f
#define A_NUMPAD_MINUS 0x4a
#define A_NUMPAD_4 0x2d
#define A_NUMPAD_5 0x2e
#define A_NUMPAD_6 0x2f
#define A_NUMPAD_PLUS 0x5e
#define A_NUMPAD_1 0x1d
#define A_NUMPAD_2 0x1e
#define A_NUMPAD_3 0x1f
#define A_NUMPAD_ENTER 0x43
#define A_NUMPAD_0 0x0f
#define A_NUMPAD_DOT 0x3c

#define A_HELP 0x5f
#define A_DELETE 0x46

#define A_UP 0x4c
#define A_LEFT 0x4f
#define A_DOWN 0x4d
#define A_RIGHT 0x4e

/*=======================================================*/
/* Symbolic representation for PC-keys                   */
/* Names given are based on US keyboard                  */
/* The important thing is which scancodes they represent */
/*=======================================================*/

typedef enum
{
  PCK_NONE = 0,
  PCK_ESCAPE = 1,
  PCK_F1 = 2,
  PCK_F2 = 3,
  PCK_F3 = 4,
  PCK_F4 = 5,
  PCK_F5 = 6,
  PCK_F6 = 7,
  PCK_F7 = 8,
  PCK_F8 = 9,
  PCK_F9 = 10,
  PCK_F10 = 11,
  PCK_F11 = 12,
  PCK_F12 = 13,
  PCK_PRINT_SCREEN = 14,
  PCK_SCROLL_LOCK = 15,
  PCK_PAUSE = 16,
  PCK_GRAVE = 17,
  PCK_1 = 18,
  PCK_2 = 19,
  PCK_3 = 20,
  PCK_4 = 21,
  PCK_5 = 22,
  PCK_6 = 23,
  PCK_7 = 24,
  PCK_8 = 25,
  PCK_9 = 26,
  PCK_0 = 27,
  PCK_MINUS = 28,
  PCK_EQUALS = 29,
  PCK_BACKSPACE = 30,
  PCK_INSERT = 31,
  PCK_HOME = 32,
  PCK_PAGE_UP = 33,
  PCK_NUM_LOCK = 34,
  PCK_NUMPAD_DIVIDE = 35,
  PCK_NUMPAD_MULTIPLY = 36,
  PCK_NUMPAD_MINUS = 37,
  PCK_TAB = 38,
  PCK_Q = 39,
  PCK_W = 40,
  PCK_E = 41,
  PCK_R = 42,
  PCK_T = 43,
  PCK_Y = 44,
  PCK_U = 45,
  PCK_I = 46,
  PCK_O = 47,
  PCK_P = 48,
  PCK_LBRACKET = 49,
  PCK_RBRACKET = 50,
  PCK_RETURN = 51,
  PCK_DELETE = 52,
  PCK_END = 53,
  PCK_PAGE_DOWN = 54,
  PCK_NUMPAD_7 = 55,
  PCK_NUMPAD_8 = 56,
  PCK_NUMPAD_9 = 57,
  PCK_NUMPAD_PLUS = 58,
  PCK_CAPS_LOCK = 59,
  PCK_A = 60,
  PCK_S = 61,
  PCK_D = 62,
  PCK_F = 63,
  PCK_G = 64,
  PCK_H = 65,
  PCK_J = 66,
  PCK_K = 67,
  PCK_L = 68,
  PCK_SEMICOLON = 69,
  PCK_APOSTROPHE = 70,
  PCK_BACKSLASH = 71,
  PCK_NUMPAD_4 = 72,
  PCK_NUMPAD_5 = 73,
  PCK_NUMPAD_6 = 74,
  PCK_LEFT_SHIFT = 75,
  PCK_NONAME1 = 76,
  PCK_Z = 77,
  PCK_X = 78,
  PCK_C = 79,
  PCK_V = 80,
  PCK_B = 81,
  PCK_N = 82,
  PCK_M = 83,
  PCK_COMMA = 84,
  PCK_PERIOD = 85,
  PCK_SLASH = 86,
  PCK_RIGHT_SHIFT = 87,
  PCK_UP = 88,
  PCK_NUMPAD_1 = 89,
  PCK_NUMPAD_2 = 90,
  PCK_NUMPAD_3 = 91,
  PCK_NUMPAD_ENTER = 92,
  PCK_LEFT_CTRL = 93,
  PCK_LEFT_WINDOWS = 94,
  PCK_LEFT_ALT = 95,
  PCK_SPACE = 96,
  PCK_RIGHT_ALT = 97,
  PCK_RIGHT_WINDOWS = 98,
  PCK_START_MENU = 99,
  PCK_RIGHT_CTRL = 100,
  PCK_LEFT = 101,
  PCK_DOWN = 102,
  PCK_RIGHT = 103,
  PCK_NUMPAD_0 = 104,
  PCK_NUMPAD_DOT = 105,
  PCK_LAST_KEY = 106
} kbd_drv_pc_symbol;
