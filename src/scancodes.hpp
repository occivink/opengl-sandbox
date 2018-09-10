#pragma once

namespace ScanCode
{
enum DUMMY
{
    S_UNKNOWN = 0,

    /**
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     */
    /* @{ */

    S_A = 4,
    S_B = 5,
    S_C = 6,
    S_D = 7,
    S_E = 8,
    S_F = 9,
    S_G = 10,
    S_H = 11,
    S_I = 12,
    S_J = 13,
    S_K = 14,
    S_L = 15,
    S_M = 16,
    S_N = 17,
    S_O = 18,
    S_P = 19,
    S_Q = 20,
    S_R = 21,
    S_S = 22,
    S_T = 23,
    S_U = 24,
    S_V = 25,
    S_W = 26,
    S_X = 27,
    S_Y = 28,
    S_Z = 29,

    S_1 = 30,
    S_2 = 31,
    S_3 = 32,
    S_4 = 33,
    S_5 = 34,
    S_6 = 35,
    S_7 = 36,
    S_8 = 37,
    S_9 = 38,
    S_0 = 39,

    S_RETURN = 40,
    S_ESCAPE = 41,
    S_BACKSPACE = 42,
    S_TAB = 43,
    S_SPACE = 44,

    S_MINUS = 45,
    S_EQUALS = 46,
    S_LEFTBRACKET = 47,
    S_RIGHTBRACKET = 48,
    S_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    S_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate S_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    S_SEMICOLON = 51,
    S_APOSTROPHE = 52,
    S_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    S_COMMA = 54,
    S_PERIOD = 55,
    S_SLASH = 56,

    S_CAPSLOCK = 57,

    S_F1 = 58,
    S_F2 = 59,
    S_F3 = 60,
    S_F4 = 61,
    S_F5 = 62,
    S_F6 = 63,
    S_F7 = 64,
    S_F8 = 65,
    S_F9 = 66,
    S_F10 = 67,
    S_F11 = 68,
    S_F12 = 69,

    S_PRINTSCREEN = 70,
    S_SCROLLLOCK = 71,
    S_PAUSE = 72,
    S_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    S_HOME = 74,
    S_PAGEUP = 75,
    S_DELETE = 76,
    S_END = 77,
    S_PAGEDOWN = 78,
    S_RIGHT = 79,
    S_LEFT = 80,
    S_DOWN = 81,
    S_UP = 82,

    S_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    S_KP_DIVIDE = 84,
    S_KP_MULTIPLY = 85,
    S_KP_MINUS = 86,
    S_KP_PLUS = 87,
    S_KP_ENTER = 88,
    S_KP_1 = 89,
    S_KP_2 = 90,
    S_KP_3 = 91,
    S_KP_4 = 92,
    S_KP_5 = 93,
    S_KP_6 = 94,
    S_KP_7 = 95,
    S_KP_8 = 96,
    S_KP_9 = 97,
    S_KP_0 = 98,
    S_KP_PERIOD = 99,

    S_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    S_APPLICATION = 101, /**< windows contextual menu, compose */
    S_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    S_KP_EQUALS = 103,
    S_F13 = 104,
    S_F14 = 105,
    S_F15 = 106,
    S_F16 = 107,
    S_F17 = 108,
    S_F18 = 109,
    S_F19 = 110,
    S_F20 = 111,
    S_F21 = 112,
    S_F22 = 113,
    S_F23 = 114,
    S_F24 = 115,
    S_EXECUTE = 116,
    S_HELP = 117,
    S_MENU = 118,
    S_SELECT = 119,
    S_STOP = 120,
    S_AGAIN = 121,   /**< redo */
    S_UNDO = 122,
    S_CUT = 123,
    S_COPY = 124,
    S_PASTE = 125,
    S_FIND = 126,
    S_MUTE = 127,
    S_VOLUMEUP = 128,
    S_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     S_LOCKINGCAPSLOCK = 130,  */
/*     S_LOCKINGNUMLOCK = 131, */
/*     S_LOCKINGSCROLLLOCK = 132, */
    S_KP_COMMA = 133,
    S_KP_EQUALSAS400 = 134,

    S_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    S_INTERNATIONAL2 = 136,
    S_INTERNATIONAL3 = 137, /**< Yen */
    S_INTERNATIONAL4 = 138,
    S_INTERNATIONAL5 = 139,
    S_INTERNATIONAL6 = 140,
    S_INTERNATIONAL7 = 141,
    S_INTERNATIONAL8 = 142,
    S_INTERNATIONAL9 = 143,
    S_LANG1 = 144, /**< Hangul/English toggle */
    S_LANG2 = 145, /**< Hanja conversion */
    S_LANG3 = 146, /**< Katakana */
    S_LANG4 = 147, /**< Hiragana */
    S_LANG5 = 148, /**< Zenkaku/Hankaku */
    S_LANG6 = 149, /**< reserved */
    S_LANG7 = 150, /**< reserved */
    S_LANG8 = 151, /**< reserved */
    S_LANG9 = 152, /**< reserved */

    S_ALTERASE = 153, /**< Erase-Eaze */
    S_SYSREQ = 154,
    S_CANCEL = 155,
    S_CLEAR = 156,
    S_PRIOR = 157,
    S_RETURN2 = 158,
    S_SEPARATOR = 159,
    S_OUT = 160,
    S_OPER = 161,
    S_CLEARAGAIN = 162,
    S_CRSEL = 163,
    S_EXSEL = 164,

    S_KP_00 = 176,
    S_KP_000 = 177,
    S_THOUSANDSSEPARATOR = 178,
    S_DECIMALSEPARATOR = 179,
    S_CURRENCYUNIT = 180,
    S_CURRENCYSUBUNIT = 181,
    S_KP_LEFTPAREN = 182,
    S_KP_RIGHTPAREN = 183,
    S_KP_LEFTBRACE = 184,
    S_KP_RIGHTBRACE = 185,
    S_KP_TAB = 186,
    S_KP_BACKSPACE = 187,
    S_KP_A = 188,
    S_KP_B = 189,
    S_KP_C = 190,
    S_KP_D = 191,
    S_KP_E = 192,
    S_KP_F = 193,
    S_KP_XOR = 194,
    S_KP_POWER = 195,
    S_KP_PERCENT = 196,
    S_KP_LESS = 197,
    S_KP_GREATER = 198,
    S_KP_AMPERSAND = 199,
    S_KP_DBLAMPERSAND = 200,
    S_KP_VERTICALBAR = 201,
    S_KP_DBLVERTICALBAR = 202,
    S_KP_COLON = 203,
    S_KP_HASH = 204,
    S_KP_SPACE = 205,
    S_KP_AT = 206,
    S_KP_EXCLAM = 207,
    S_KP_MEMSTORE = 208,
    S_KP_MEMRECALL = 209,
    S_KP_MEMCLEAR = 210,
    S_KP_MEMADD = 211,
    S_KP_MEMSUBTRACT = 212,
    S_KP_MEMMULTIPLY = 213,
    S_KP_MEMDIVIDE = 214,
    S_KP_PLUSMINUS = 215,
    S_KP_CLEAR = 216,
    S_KP_CLEARENTRY = 217,
    S_KP_BINARY = 218,
    S_KP_OCTAL = 219,
    S_KP_DECIMAL = 220,
    S_KP_HEXADECIMAL = 221,

    S_LCTRL = 224,
    S_LSHIFT = 225,
    S_LALT = 226, /**< alt, option */
    S_LGUI = 227, /**< windows, command (apple), meta */
    S_RCTRL = 228,
    S_RSHIFT = 229,
    S_RALT = 230, /**< alt gr, option */
    S_RGUI = 231, /**< windows, command (apple), meta */

    S_MODE = 257,    /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 */

    /* @} *//* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    S_AUDIONEXT = 258,
    S_AUDIOPREV = 259,
    S_AUDIOSTOP = 260,
    S_AUDIOPLAY = 261,
    S_AUDIOMUTE = 262,
    S_MEDIASELECT = 263,
    S_WWW = 264,
    S_MAIL = 265,
    S_CALCULATOR = 266,
    S_COMPUTER = 267,
    S_AC_SEARCH = 268,
    S_AC_HOME = 269,
    S_AC_BACK = 270,
    S_AC_FORWARD = 271,
    S_AC_STOP = 272,
    S_AC_REFRESH = 273,
    S_AC_BOOKMARKS = 274,

    /* @} *//* Usage page 0x0C */

    /**
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     */
    /* @{ */

    S_BRIGHTNESSDOWN = 275,
    S_BRIGHTNESSUP = 276,
    S_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
    S_KBDILLUMTOGGLE = 278,
    S_KBDILLUMDOWN = 279,
    S_KBDILLUMUP = 280,
    S_EJECT = 281,
    S_SLEEP = 282,

    S_APP1 = 283,
    S_APP2 = 284,

    /* @} *//* Walther keys */

    /**
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    S_AUDIOREWIND = 285,
    S_AUDIOFASTFORWARD = 286,

    /* @} *//* Usage page 0x0C (additional media keys) */

    /* Add any other keys here. */

    NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
};

static const int keyCodeToScanCode[] = {
    /*  0 */    S_UNKNOWN,
    /*  1 */    S_UNKNOWN,
    /*  2 */    S_UNKNOWN,
    /*  3 */    S_CANCEL,
    /*  4 */    S_UNKNOWN,
    /*  5 */    S_UNKNOWN,
    /*  6 */    S_HELP,
    /*  7 */    S_UNKNOWN,
    /*  8 */    S_BACKSPACE,
    /*  9 */    S_TAB,
    /*  10 */   S_UNKNOWN,
    /*  11 */   S_UNKNOWN,
    /*  12 */   S_UNKNOWN,
    /*  13 */   S_RETURN,
    /*  14 */   S_UNKNOWN,
    /*  15 */   S_UNKNOWN,
    /*  16 */   S_LSHIFT,
    /*  17 */   S_LCTRL,
    /*  18 */   S_LALT,
    /*  19 */   S_PAUSE,
    /*  20 */   S_CAPSLOCK,
    /*  21 */   S_UNKNOWN,
    /*  22 */   S_UNKNOWN,
    /*  23 */   S_UNKNOWN,
    /*  24 */   S_UNKNOWN,
    /*  25 */   S_UNKNOWN,
    /*  26 */   S_UNKNOWN,
    /*  27 */   S_ESCAPE,
    /*  28 */   S_UNKNOWN,
    /*  29 */   S_UNKNOWN,
    /*  30 */   S_UNKNOWN,
    /*  31 */   S_UNKNOWN,
    /*  32 */   S_SPACE,
    /*  33 */   S_PAGEUP,
    /*  34 */   S_PAGEDOWN,
    /*  35 */   S_END,
    /*  36 */   S_HOME,
    /*  37 */   S_LEFT,
    /*  38 */   S_UP,
    /*  39 */   S_RIGHT,
    /*  40 */   S_DOWN,
    /*  41 */   S_UNKNOWN,
    /*  42 */   S_UNKNOWN,
    /*  43 */   S_UNKNOWN,
    /*  44 */   S_UNKNOWN,
    /*  45 */   S_INSERT,
    /*  46 */   S_DELETE,
    /*  47 */   S_UNKNOWN,
    /*  48 */   S_0,
    /*  49 */   S_1,
    /*  50 */   S_2,
    /*  51 */   S_3,
    /*  52 */   S_4,
    /*  53 */   S_5,
    /*  54 */   S_6,
    /*  55 */   S_7,
    /*  56 */   S_8,
    /*  57 */   S_9,
    /*  58 */   S_UNKNOWN,
    /*  59 */   S_SEMICOLON,
    /*  60 */   S_UNKNOWN,
    /*  61 */   S_EQUALS,
    /*  62 */   S_UNKNOWN,
    /*  63 */   S_UNKNOWN,
    /*  64 */   S_UNKNOWN,
    /*  65 */   S_A,
    /*  66 */   S_B,
    /*  67 */   S_C,
    /*  68 */   S_D,
    /*  69 */   S_E,
    /*  70 */   S_F,
    /*  71 */   S_G,
    /*  72 */   S_H,
    /*  73 */   S_I,
    /*  74 */   S_J,
    /*  75 */   S_K,
    /*  76 */   S_L,
    /*  77 */   S_M,
    /*  78 */   S_N,
    /*  79 */   S_O,
    /*  80 */   S_P,
    /*  81 */   S_Q,
    /*  82 */   S_R,
    /*  83 */   S_S,
    /*  84 */   S_T,
    /*  85 */   S_U,
    /*  86 */   S_V,
    /*  87 */   S_W,
    /*  88 */   S_X,
    /*  89 */   S_Y,
    /*  90 */   S_Z,
    /*  91 */   S_LGUI,
    /*  92 */   S_UNKNOWN,
    /*  93 */   S_APPLICATION,
    /*  94 */   S_UNKNOWN,
    /*  95 */   S_UNKNOWN,
    /*  96 */   S_KP_0,
    /*  97 */   S_KP_1,
    /*  98 */   S_KP_2,
    /*  99 */   S_KP_3,
    /* 100 */   S_KP_4,
    /* 101 */   S_KP_5,
    /* 102 */   S_KP_6,
    /* 103 */   S_KP_7,
    /* 104 */   S_KP_8,
    /* 105 */   S_KP_9,
    /* 106 */   S_KP_MULTIPLY,
    /* 107 */   S_KP_PLUS,
    /* 108 */   S_UNKNOWN,
    /* 109 */   S_KP_MINUS,
    /* 110 */   S_KP_PERIOD,
    /* 111 */   S_KP_DIVIDE,
    /* 112 */   S_F1,
    /* 113 */   S_F2,
    /* 114 */   S_F3,
    /* 115 */   S_F4,
    /* 116 */   S_F5,
    /* 117 */   S_F6,
    /* 118 */   S_F7,
    /* 119 */   S_F8,
    /* 120 */   S_F9,
    /* 121 */   S_F10,
    /* 122 */   S_F11,
    /* 123 */   S_F12,
    /* 124 */   S_F13,
    /* 125 */   S_F14,
    /* 126 */   S_F15,
    /* 127 */   S_F16,
    /* 128 */   S_F17,
    /* 129 */   S_F18,
    /* 130 */   S_F19,
    /* 131 */   S_F20,
    /* 132 */   S_F21,
    /* 133 */   S_F22,
    /* 134 */   S_F23,
    /* 135 */   S_F24,
    /* 136 */   S_UNKNOWN,
    /* 137 */   S_UNKNOWN,
    /* 138 */   S_UNKNOWN,
    /* 139 */   S_UNKNOWN,
    /* 140 */   S_UNKNOWN,
    /* 141 */   S_UNKNOWN,
    /* 142 */   S_UNKNOWN,
    /* 143 */   S_UNKNOWN,
    /* 144 */   S_NUMLOCKCLEAR,
    /* 145 */   S_SCROLLLOCK,
    /* 146 */   S_UNKNOWN,
    /* 147 */   S_UNKNOWN,
    /* 148 */   S_UNKNOWN,
    /* 149 */   S_UNKNOWN,
    /* 150 */   S_UNKNOWN,
    /* 151 */   S_UNKNOWN,
    /* 152 */   S_UNKNOWN,
    /* 153 */   S_UNKNOWN,
    /* 154 */   S_UNKNOWN,
    /* 155 */   S_UNKNOWN,
    /* 156 */   S_UNKNOWN,
    /* 157 */   S_UNKNOWN,
    /* 158 */   S_UNKNOWN,
    /* 159 */   S_UNKNOWN,
    /* 160 */   S_UNKNOWN,
    /* 161 */   S_UNKNOWN,
    /* 162 */   S_UNKNOWN,
    /* 163 */   S_UNKNOWN,
    /* 164 */   S_UNKNOWN,
    /* 165 */   S_UNKNOWN,
    /* 166 */   S_UNKNOWN,
    /* 167 */   S_UNKNOWN,
    /* 168 */   S_UNKNOWN,
    /* 169 */   S_UNKNOWN,
    /* 170 */   S_UNKNOWN,
    /* 171 */   S_UNKNOWN,
    /* 172 */   S_UNKNOWN,
    /* 173 */   S_MINUS, /*FX*/
    /* 174 */   S_VOLUMEDOWN, /*IE, Chrome*/
    /* 175 */   S_VOLUMEUP, /*IE, Chrome*/
    /* 176 */   S_AUDIONEXT, /*IE, Chrome*/
    /* 177 */   S_AUDIOPREV, /*IE, Chrome*/
    /* 178 */   S_UNKNOWN,
    /* 179 */   S_AUDIOPLAY, /*IE, Chrome*/
    /* 180 */   S_UNKNOWN,
    /* 181 */   S_AUDIOMUTE, /*FX*/
    /* 182 */   S_VOLUMEDOWN, /*FX*/
    /* 183 */   S_VOLUMEUP, /*FX*/
    /* 184 */   S_UNKNOWN,
    /* 185 */   S_UNKNOWN,
    /* 186 */   S_SEMICOLON, /*IE, Chrome, D3E legacy*/
    /* 187 */   S_EQUALS, /*IE, Chrome, D3E legacy*/
    /* 188 */   S_COMMA,
    /* 189 */   S_MINUS, /*IE, Chrome, D3E legacy*/
    /* 190 */   S_PERIOD,
    /* 191 */   S_SLASH,
    /* 192 */   S_GRAVE, /*FX, D3E legacy (S_APOSTROPHE in IE/Chrome)*/
    /* 193 */   S_UNKNOWN,
    /* 194 */   S_UNKNOWN,
    /* 195 */   S_UNKNOWN,
    /* 196 */   S_UNKNOWN,
    /* 197 */   S_UNKNOWN,
    /* 198 */   S_UNKNOWN,
    /* 199 */   S_UNKNOWN,
    /* 200 */   S_UNKNOWN,
    /* 201 */   S_UNKNOWN,
    /* 202 */   S_UNKNOWN,
    /* 203 */   S_UNKNOWN,
    /* 204 */   S_UNKNOWN,
    /* 205 */   S_UNKNOWN,
    /* 206 */   S_UNKNOWN,
    /* 207 */   S_UNKNOWN,
    /* 208 */   S_UNKNOWN,
    /* 209 */   S_UNKNOWN,
    /* 210 */   S_UNKNOWN,
    /* 211 */   S_UNKNOWN,
    /* 212 */   S_UNKNOWN,
    /* 213 */   S_UNKNOWN,
    /* 214 */   S_UNKNOWN,
    /* 215 */   S_UNKNOWN,
    /* 216 */   S_UNKNOWN,
    /* 217 */   S_UNKNOWN,
    /* 218 */   S_UNKNOWN,
    /* 219 */   S_LEFTBRACKET,
    /* 220 */   S_BACKSLASH,
    /* 221 */   S_RIGHTBRACKET,
    /* 222 */   S_APOSTROPHE, /*FX, D3E legacy*/
};

}

