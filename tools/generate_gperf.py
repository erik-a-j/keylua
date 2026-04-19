
from pathlib import Path
import subprocess
import re

IEC_SEARCH_PATHS = [Path("/usr/include/linux"), Path("/usr/local/include/linux")]
IEC_HEADER = "input-event-codes.h"

GPERF_OUT: Path
CPP_OUT: Path
H_OUT: Path

GPERF_CLASS_NAME: str
GPERF_SLOT_NAME = "key"
GPERF_LOOKUPN_FUNC_NAME = "lookupn"
GPERF_LOOKUP_FUNC_NAME = "lookup"
GPERF_HASH_FUNC_NAME = "hash"
GPERF_KW_STRUCT_NAME = "keyword"
GPERF_KW_STRUCT = f"struct {GPERF_KW_STRUCT_NAME} {{\n    const char *{GPERF_SLOT_NAME};\n    int code;\n}};"

GPERF_PAIRS = [
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

def generate_gperf() -> None:
    iec_path = None
    for p in IEC_SEARCH_PATHS:
        if Path(p / IEC_HEADER).exists():
            iec_path = p / IEC_HEADER
    
    if iec_path is None:
        print(f"couldn't find {IEC_HEADER}")
        exit(1)

    with open(iec_path, mode="rt", encoding="utf-8") as f:
        r = re.compile(r"^#define\s+(KEY_\w+|BTN_\w+)")
        iec_all_evcodes = [m.group(1) for line in f if (m := r.search(line))]

    with open(GPERF_OUT, mode="wt", encoding="utf-8") as f:
        f.write(f"%language=C++\n%struct-type\n%omit-struct-type\n%7bit\n%readonly-tables\n%compare-strncmp\n%compare-lengths\n"
                f"%define slot-name {GPERF_SLOT_NAME}\n%define class-name {GPERF_CLASS_NAME}\n"
                f"%define hash-function-name {GPERF_HASH_FUNC_NAME}\n%define lookup-function-name {GPERF_LOOKUPN_FUNC_NAME}\n"
                f"%{{\n#include \"{H_OUT.name}\"\n%}}\n{GPERF_KW_STRUCT}\n%%\n")
        for k, v in GPERF_PAIRS:
            if v not in iec_all_evcodes:
                print(f"WARNING: {v} is not valid")
            f.write(f"{k}, {v}\n")
        f.write("%%")

class_decl_lines_t = list[str]
def generate_cpp() -> class_decl_lines_t:
    res: subprocess.CompletedProcess = subprocess.run(
        ["gperf", "-m", "100", str(GPERF_OUT)], capture_output=True, text=True
    )
    if res.returncode != 0:
        print(res.stderr)
        exit(1)
    
    class_start = 0
    class_end = 0
    lines: list[str] = str(res.stdout).splitlines(keepends=True)
    with open(CPP_OUT, mode="wt", encoding="utf-8") as f:
        class_start_match = re.compile(rf"^class\s+{GPERF_CLASS_NAME}")
        i = 0
        while i < len(lines):
            if class_start_match.match(lines[i]):
                try:
                    class_start = i
                    class_end = lines.index("};\n", i + 1) + 1
                    i = class_end
                except ValueError as e:
                    print("Error:",e)
                    exit(1)
            else:
                f.write(f"{lines[i]}")
                i += 1
    
    return lines[class_start:class_end]

def generate_h(decl_lines: class_decl_lines_t) -> None:
    hguard = f"{H_OUT.stem.upper()}_H"

    lookupn_decl_match = re.compile(rf".*{GPERF_LOOKUPN_FUNC_NAME}.*")
    i = 0
    while i < len(decl_lines):
        if lookupn_decl_match.match(decl_lines[i]):
            m_return_type = "return_type"
            m_str_param_type = "str_param_type"
            m_str_param_name = "str_param_name"
            m = re.search(rf"(?P<{m_return_type}>.*struct\s*{GPERF_KW_STRUCT_NAME}\s*\*\s*){GPERF_LOOKUPN_FUNC_NAME}\s*\((?P<{m_str_param_type}>const\s+char\s*\*\s*)(?P<{m_str_param_name}>\w+).*\);", decl_lines[i])
            lookup_decl = f"{m.group(m_return_type)}{GPERF_LOOKUP_FUNC_NAME}({m.group(m_str_param_type)}{m.group(m_str_param_name)});\n"
            decl_lines.insert(i, lookup_decl)
            break
        i += 1

    with open(H_OUT, mode="wt", encoding="utf-8") as f:
        f.write(f"#ifndef {hguard}\n#define {hguard}\n\n#include <linux/input-event-codes.h>\n#include <cstddef>\n#include <cstring>\n\n")
        f.write(f"{GPERF_KW_STRUCT}\n\n")
        f.writelines(decl_lines)
        f.write(f"\n#endif /* #ifndef {hguard} */\n")


def generate(gperf_out: Path) -> None:
    globals()["GPERF_OUT"] = gperf_out
    globals()["CPP_OUT"] = gperf_out.with_suffix(".cpp")
    globals()["H_OUT"] = gperf_out.with_suffix(".h")
    globals()["GPERF_CLASS_NAME"] = "".join([word.capitalize() for word in GPERF_OUT.stem.split("_")])

    generate_gperf()
    generate_h(generate_cpp())
    
    

        

