PAIRS = [
    ('1', 'KEY_1'),
    ('2', 'KEY_2'),
    ('3', 'KEY_3'),
    ('4', 'KEY_4'),
    ('5', 'KEY_5'),
    ('6', 'KEY_6'),
    ('7', 'KEY_7'),
    ('8', 'KEY_8'),
    ('9', 'KEY_9'),
    ('0', 'KEY_0'),
    ('q', 'KEY_Q'),
    ('w', 'KEY_W'),
    ('e', 'KEY_E'),
    ('r', 'KEY_R'),
    ('t', 'KEY_T'),
    ('y', 'KEY_Y'),
    ('u', 'KEY_U'),
    ('i', 'KEY_I'),
    ('o', 'KEY_O'),
    ('p', 'KEY_P'),
    ('a', 'KEY_A'),
    ('s', 'KEY_S'),
    ('d', 'KEY_D'),
    ('f', 'KEY_F'),
    ('g', 'KEY_G'),
    ('h', 'KEY_H'),
    ('j', 'KEY_J'),
    ('k', 'KEY_K'),
    ('l', 'KEY_L'),
    ('z', 'KEY_Z'),
    ('x', 'KEY_X'),
    ('c', 'KEY_C'),
    ('v', 'KEY_V'),
    ('b', 'KEY_B'),
    ('n', 'KEY_N'),
    ('m', 'KEY_M'),
    ('-', 'KEY_MINUS'),
    ('=', 'KEY_EQUAL'),
    ('{', 'KEY_LEFTBRACE'),
    ('}', 'KEY_RIGHTBRACE'),
    (';', 'KEY_SEMICOLON'),
    ("'", 'KEY_APOSTROPHE'),
    ('`', 'KEY_GRAVE'),
    ('\\', 'KEY_BACKSLASH'),
    ('","', 'KEY_COMMA'),
    ('.', 'KEY_DOT'),
    ('/', 'KEY_SLASH'),

    ('C', 'KEY_LEFTCTRL'),
    ('ctrl', 'KEY_LEFTCTRL'),
    ('lctrl', 'KEY_LEFTCTRL'),
    ('S', 'KEY_LEFTSHIFT'),
    ('shift', 'KEY_LEFTSHIFT'),
    ('lshift', 'KEY_LEFTSHIFT'),
    ('rshift', 'KEY_RIGHTSHIFT'),
    ('A', 'KEY_LEFTALT'),
    ('alt', 'KEY_LEFTALT'),
    ('lalt', 'KEY_LEFTALT'),

    ('ESC', 'KEY_ESC'),
    ('esc', 'KEY_ESC'),
    ('BS', 'KEY_BACKSPACE'),
    ('bs', 'KEY_BACKSPACE'),
    ('TAB', 'KEY_TAB'),
    ('tab', 'KEY_TAB'),
    ('CR', 'KEY_ENTER'),
    ('cr', 'KEY_ENTER'),
    (' ', 'KEY_SPACE'),
    ('space', 'KEY_SPACE'),
    ('capslock', 'KEY_CAPSLOCK'),

    ('F1', 'KEY_F1'),
    ('F2', 'KEY_F2'),
    ('F3', 'KEY_F3'),
    ('F4', 'KEY_F4'),
    ('F5', 'KEY_F5'),
    ('F6', 'KEY_F6'),
    ('F7', 'KEY_F7'),
    ('F8', 'KEY_F8'),
    ('F9', 'KEY_F9'),
    ('F10', 'KEY_F10'),
    ('F11', 'KEY_F11'),
    ('F12', 'KEY_F12'),
    
    ('<', 'KEY_102ND'),
]

BEGIN = """\
%language=C++
%struct-type
%define slot-name key
%define class-name InputEventCodesMap
%define lookup-function-name evcode
%7bit
%readonly-tables
%compare-strncmp
%compare-lengths
%omit-struct-type
%{
#define INPUT_EVENT_CODES_MAP_GPERF
#include "input_event_codes_map.h"
%}
struct keyword {
  const char *key;
  int code;
};
%%
"""

END = "%%"