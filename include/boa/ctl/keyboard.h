#ifndef BOA_CTL_KEYBOARD_H
#define BOA_CTL_KEYBOARD_H

#include "boa/ctl/action.h"
#include <bitset>
#include <functional>

namespace boa::ctl {

enum Modifier {
    ModShift                = 0x0001,
    ModControl              = 0x0002,
    ModAlt                  = 0x0004,
    ModSuper                = 0x0008,
    ModCapsLock             = 0x0010,
    ModNumLock              = 0x0020,
};

enum Key {
    /* The unknown key */
    KeyUnknown              = -1,

    /* Printable keys */
    KeySpace                = 32,
    KeyApostrophe           = 39,  /* ' */
    KeyComma                = 44,  /* , */
    KeyMinus                = 45,  /* - */
    KeyPeriod               = 46,  /* . */
    KeySlash                = 47,  /* / */
    Key0                    = 48,
    Key1                    = 49,
    Key2                    = 50,
    Key3                    = 51,
    Key4                    = 52,
    Key5                    = 53,
    Key6                    = 54,
    Key7                    = 55,
    Key8                    = 56,
    Key9                    = 57,
    KeySemicolon            = 59,  /* ; */
    KeyEqual                = 61,  /* = */
    KeyA                    = 65,
    KeyB                    = 66,
    KeyC                    = 67,
    KeyD                    = 68,
    KeyE                    = 69,
    KeyF                    = 70,
    KeyG                    = 71,
    KeyH                    = 72,
    KeyI                    = 73,
    KeyJ                    = 74,
    KeyK                    = 75,
    KeyL                    = 76,
    KeyM                    = 77,
    KeyN                    = 78,
    KeyO                    = 79,
    KeyP                    = 80,
    KeyQ                    = 81,
    KeyR                    = 82,
    KeyS                    = 83,
    KeyT                    = 84,
    KeyU                    = 85,
    KeyV                    = 86,
    KeyW                    = 87,
    KeyX                    = 88,
    KeyY                    = 89,
    KeyZ                    = 90,
    KeyLeftBracket          = 91,  /* [ */
    KeyBackslash            = 92,  /* \ */
    KeyRightBracket         = 93,  /* ] */
    KeyGraveAccent          = 96,  /* ` */
    KeyWorld1               = 161, /* non-US #1 */
    KeyWorld2               = 162, /* non-US #2 */

    /* Function keys */
    KeyEscape               = 256,
    KeyEnter                = 257,
    KeyTab                  = 258,
    KeyBackspace            = 259,
    KeyInsert               = 260,
    KeyDelete               = 261,
    KeyRight                = 262,
    KeyLeft                 = 263,
    KeyDown                 = 264,
    KeyUp                   = 265,
    KeyPageUp               = 266,
    KeyPageDown             = 267,
    KeyHome                 = 268,
    KeyEnd                  = 269,
    KeyCapsLock             = 280,
    KeyScrollLock           = 281,
    KeyNumLock              = 282,
    KeyPrintScreen          = 283,
    KeyPause                = 284,
    KeyF1                   = 290,
    KeyF2                   = 291,
    KeyF3                   = 292,
    KeyF4                   = 293,
    KeyF5                   = 294,
    KeyF6                   = 295,
    KeyF7                   = 296,
    KeyF8                   = 297,
    KeyF9                   = 298,
    KeyF10                  = 299,
    KeyF11                  = 300,
    KeyF12                  = 301,
    KeyF13                  = 302,
    KeyF14                  = 303,
    KeyF15                  = 304,
    KeyF16                  = 305,
    KeyF17                  = 306,
    KeyF18                  = 307,
    KeyF19                  = 308,
    KeyF20                  = 309,
    KeyF21                  = 310,
    KeyF22                  = 311,
    KeyF23                  = 312,
    KeyF24                  = 313,
    KeyF25                  = 314,
    KeyKp0                  = 320,
    KeyKp1                  = 321,
    KeyKp2                  = 322,
    KeyKp3                  = 323,
    KeyKp4                  = 324,
    KeyKp5                  = 325,
    KeyKp6                  = 326,
    KeyKp7                  = 327,
    KeyKp8                  = 328,
    KeyKp9                  = 329,
    KeyKpDecimal            = 330,
    KeyKpDivide             = 331,
    KeyKpMultiply           = 332,
    KeyKpSubtract           = 333,
    KeyKpAdd                = 334,
    KeyKpEnter              = 335,
    KeyKpEqual              = 336,
    KeyLeftShift            = 340,
    KeyLeftControl          = 341,
    KeyLeftAlt              = 342,
    KeyLeftSuper            = 343,
    KeyRightShift           = 344,
    KeyRightControl         = 345,
    KeyRightAlt             = 346,
    KeyRightSuper           = 347,
    KeyMenu                 = 348,

    Key_Last                = KeyMenu
};

class Keyboard {
public:
    static void keyboard_callback(void *ptrs_v, int key, int scancode, int action, int mods);

    void set_callback(std::function<void(int, int, int)> callback);

    bool key(uint32_t key) const;
    void flip_key(uint32_t key);
    void set_key(uint32_t key, bool pressed);

private:
    std::bitset<348> m_keys;
    std::function<void(int, int, int)> m_callback;
};

}

#endif
