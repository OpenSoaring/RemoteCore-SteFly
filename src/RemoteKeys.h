#pragma once

// ============================================================================
//  Key definitions shared by all SteFly remote devices.
//
//  A layout entry (key_struct) holds three sKey values per button, one for
//  each gesture: Click, LongClick and DblClick. A plain key is sent as a
//  keyboard stroke; an extended key (ext = KEY_EXTEND) selects one of the
//  built-in functions below, which need more than a single keystroke.
// ============================================================================

struct sKey {
  uint8_t ext;
  uint8_t key;
};

#define KEY_STANDARD        0
#define KEY_EXTEND          1

#define MOUSE_SWITCH        1
#define APP_SWITCH          2
#define APP_SWITCH2         3
#define APP_HIDE            4
#define TOOGLE_STF_VARIO    5
#define STICK_RESET         6

#define KEY_MOUSE_SWITCH {KEY_EXTEND, MOUSE_SWITCH}
#define KEY_APP_SWITCH   {KEY_EXTEND, APP_SWITCH}
#define KEY_APP_SWITCH2  {KEY_EXTEND, APP_SWITCH2}
#define KEY_APP_HIDE     {KEY_EXTEND, APP_HIDE}
#define KEY_TOGGLE_S_V   {KEY_EXTEND, TOOGLE_STF_VARIO}
#define KEY_STICK_RESET  {KEY_EXTEND, STICK_RESET}

struct key_struct {
    sKey Click;
    sKey LongClick;
    sKey DblClick;   
};

uint8_t *
key_code(sKey key) {
    static uint8_t buffer[8];
    if (key.ext) {
      snprintf_P(buffer, sizeof(buffer), PSTR("{%02X-%02X}"), key.ext, key.key );
    } else switch(key.key) {
      case KEY_F1: strncpy_P(buffer, PSTR("F1"), sizeof(buffer)); break;
      case KEY_F2: strncpy_P(buffer, PSTR("F2"), sizeof(buffer)); break;
      case KEY_F3: strncpy_P(buffer, PSTR("F3"), sizeof(buffer)); break;
      case KEY_F4: strncpy_P(buffer, PSTR("F4"), sizeof(buffer)); break;
      case KEY_F5: strncpy_P(buffer, PSTR("F5"), sizeof(buffer)); break;
      case KEY_F6: strncpy_P(buffer, PSTR("F6"), sizeof(buffer)); break;
      case KEY_F7: strncpy_P(buffer, PSTR("F7"), sizeof(buffer)); break;
      case KEY_F8: strncpy_P(buffer, PSTR("F8"), sizeof(buffer)); break;
      case KEY_F9: strncpy_P(buffer, PSTR("F9"), sizeof(buffer)); break;
      case KEY_F10: strncpy_P(buffer, PSTR("F10"), sizeof(buffer)); break;
      case KEY_F11: strncpy_P(buffer, PSTR("F11"), sizeof(buffer)); break;
      case KEY_F12: strncpy_P(buffer, PSTR("F12"), sizeof(buffer)); break;
      case KEY_RETURN: strncpy_P(buffer, PSTR("Return"), sizeof(buffer)); break;
      case KEY_ESC: strncpy_P(buffer, PSTR("Esc"), sizeof(buffer)); break;
      case KEY_UP_ARROW: strncpy_P(buffer, PSTR("Up"), sizeof(buffer)); break;
      case KEY_DOWN_ARROW: strncpy_P(buffer, PSTR("Down"), sizeof(buffer)); break;
      case KEY_LEFT_ARROW: strncpy_P(buffer, PSTR("Left"), sizeof(buffer)); break;
      case KEY_RIGHT_ARROW: strncpy_P(buffer, PSTR("Right"), sizeof(buffer)); break;
    default:
      if (isalnum(key.key)) {
        buffer[0] = key.key;
        buffer[1] = 0;
      }
      else
        //return nullptr;
        strncpy_P(buffer, PSTR("(null)"), sizeof(buffer)); 
      break;
    }
    return buffer;
}
