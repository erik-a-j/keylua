
from typing import NamedTuple
from dataclasses import dataclass, field
from pathlib import Path
import sys
import subprocess
import re

PROJECT_DIR = Path(__file__).parent.parent
IEC_SEARCH_PATHS = [Path("/usr/include/linux"), Path("/usr/local/include/linux")]
IEC_HEADER = "input-event-codes.h"
PRINT_GENERATED = "tools/generate.py: {}"

class MapClassInfo(NamedTuple):
    name: str
    f_hash_name: str
    f_table_size_name: str
    f_table_name: str
    f_lookup_name: str
    f_mylookup_name: str

@dataclass
class KwStructInfo:
    name: str
    slot_name: str
    body: str = field(init=False)

    def __post_init__(self):
        self.body = f"struct {self.name} {{\n    const char *{self.slot_name};\n    int code;\n}};"

@dataclass
class GenInfo:
    out_gperf: Path
    out_cpp: Path
    out_h: Path
    in_lua: Path
    out_lua: Path

    map_class: MapClassInfo
    kw_struct: KwStructInfo

    gperf_word_array_name: str
    gperf_iterations: int
    gperf_pairs: list[tuple[str, str]]
    iec_all_evcodes: list[str] = field(default_factory=list)

def generate_gperf(x: GenInfo) -> None:
    iec_path = None
    for p in IEC_SEARCH_PATHS:
        if Path(p / IEC_HEADER).exists():
            iec_path = p / IEC_HEADER
    
    if iec_path is None:
        print(f"couldn't find {IEC_HEADER}")
        exit(1)

    with open(iec_path, mode="rt", encoding="utf-8") as f:
        r = re.compile(r"^#define\s+(KEY_\w+|BTN_\w+)")
        x.iec_all_evcodes = [m.group(1) for line in f if (m := r.search(line))]

    with open(x.out_gperf, mode="wt", encoding="utf-8") as f:
        f.write(f"""\
%language=C++
%struct-type
%omit-struct-type
%7bit
%global-table
%readonly-tables
%compare-strncmp
%compare-lengths
%define slot-name {x.kw_struct.slot_name}
%define class-name {x.map_class.name}
%define hash-function-name {x.map_class.f_hash_name}
%define lookup-function-name {x.map_class.f_lookup_name}
%define word-array-name {x.gperf_word_array_name}
%define initializer-suffix ,-1
%{{
#include "{x.out_h.name}"
%}}
{x.kw_struct.body}
%%
""")
        for k, v in x.gperf_pairs:
            if v not in x.iec_all_evcodes:
                print(f"WARNING: {v} is not valid")
            f.write(f"{k}, {v}\n")
        for v in x.iec_all_evcodes:
            f.write(f"{v}, {v}\n")
        f.write("%%")

        print(PRINT_GENERATED.format(x.out_gperf))

def generate_cpp(x: GenInfo) -> list[str]:

    table_functions = f"""\
size_t {x.map_class.name}::{x.map_class.f_table_size_name}()
{{
    return sizeof({x.gperf_word_array_name}) / sizeof(*{x.gperf_word_array_name});
}}
const struct {x.kw_struct.name}* {x.map_class.name}::{x.map_class.f_table_name}()
{{
    return {x.gperf_word_array_name};
}}
"""

    res: subprocess.CompletedProcess = subprocess.run(
        ["gperf", "-m", str(x.gperf_iterations), str(x.out_gperf)], capture_output=True, text=True
    )
    if res.returncode != 0:
        print(res.stderr)
        exit(1)
    
    class_start = 0
    class_end = 0
    lines: list[str] = str(res.stdout).splitlines(keepends=True)
    with open(x.out_cpp, mode="wt", encoding="utf-8") as f:
        class_start_match = re.compile(rf"^class\s+{x.map_class.name}")
        i = 0
        while i < len(lines):
            if class_start_match.match(lines[i]):
                depth = 0
                for j, line in enumerate(lines[i:], start=i):
                    depth += line.count('{') - line.count('}')
                    if depth == 0 and j > i:
                        class_end = j + 1
                        break
                class_start = i
                i = class_end
            else:
                f.write(lines[i])
                i += 1
        f.write(table_functions)
    
    print(PRINT_GENERATED.format(x.out_cpp))
    return lines[class_start:class_end]

def generate_h(x: GenInfo, decl_lines: list[str]) -> None:
    hguard = f"{x.out_h.stem.upper()}_H"

    m_indent = "indent"
    m_str_param_type = "str_param_type"
    m_str_param_name = "str_param_name"
    lookup_decl_re = re.compile(rf"(?P<{m_indent}>^\s*).*struct\s+{x.kw_struct.name}\s*\*\s*{x.map_class.f_lookup_name}\s*\((?P<{m_str_param_type}>const\s+char\s*\*\s*)(?P<{m_str_param_name}>\w+).*\);")
    i = 0
    while i < len(decl_lines):
        m = lookup_decl_re.search(decl_lines[i])
        if m is not None:
            indent = m.group(m_indent)
            str_param_type = m.group(m_str_param_type)
            str_param_name = m.group(m_str_param_name)

            table_size_decl = f"{indent}static size_t {x.map_class.f_table_size_name}();\n"
            table_decl = f"{indent}static const struct {x.kw_struct.name}* {x.map_class.f_table_name}();\n"
            mylookup = f"{indent}static const struct {x.kw_struct.name}* {x.map_class.f_mylookup_name}({str_param_type}{str_param_name}) {{ return {x.map_class.f_lookup_name}({str_param_name}, std::strlen({str_param_name})); }}\n"
            decl_lines[i:i] = [table_size_decl, table_decl, mylookup]

            break
        i += 1

    with open(x.out_h, mode="wt", encoding="utf-8") as f:
        f.write(f"#ifndef {hguard}\n#define {hguard}\n\n#include <linux/input-event-codes.h>\n#include <cstddef>\n#include <cstring>\n\n")
        f.write(f"{x.kw_struct.body}\n\n")
        f.writelines(decl_lines)
        f.write(f"\n#endif /* #ifndef {hguard} */\n")
    
    print(PRINT_GENERATED.format(x.out_h))

def generate_lua(x: GenInfo) -> None:
    ns = x.out_lua.stem
    #---@alias keylua.KeyName

    with open(x.in_lua, mode="rt", encoding="utf-8") as f:
        lines = [l.strip('\n') for l in f]
    
    m = re.compile(rf"---@alias {ns}.KeyName")

    with open(x.out_lua, mode="wt", encoding="utf-8") as f:
        for l in lines:
            f.write(f"{l}\n")
            if m.match(l):
                for k, _ in x.gperf_pairs:
                    f.write(f'---| "{k}"\n')
                for k in x.iec_all_evcodes:
                    f.write(f'---| "{k}"\n')

    print(PRINT_GENERATED.format(x.out_lua))

def main(gperf: Path, gperf_iterations: int, lua_in: Path, lua_out: Path) -> None:

    x = GenInfo(
        out_gperf=gperf,
        out_cpp=gperf.with_suffix(".cpp"),
        out_h=gperf.with_suffix(".h"),
        in_lua=lua_in,
        out_lua=lua_out,

        map_class=MapClassInfo(
            name="".join([word.capitalize() for word in gperf.stem.split("_")]),
            f_hash_name="hash",
            f_table_size_name="table_size",
            f_table_name="table",
            f_lookup_name="lookupn",
            f_mylookup_name="lookup"
        ),
        kw_struct=KwStructInfo(
            name="keyword",
            slot_name="key"
        ),

        gperf_word_array_name="wordlist",
        gperf_iterations=gperf_iterations,
        gperf_pairs=[
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
    )

    Path.mkdir(x.out_gperf.parent, parents=True, exist_ok=True)
    Path.mkdir(x.out_lua.parent, parents=True, exist_ok=True)

    generate_gperf(x)
    generate_h(x, generate_cpp(x))
    generate_lua(x)

    
if __name__ == "__main__":
    gperf_iterations: int = 100
    gperf: Path = None
    lua_in: Path = None
    lua_out: Path = None
    #--gperf ${EVENT_CODES_MAP_GPERF} --iterations ${GPERF_ITERATIONS} --lua-in ${KEYLUA_TEMPLATE_LUA} --lua-out ${KEYLUA_LUA}
    for i in range(1, len(sys.argv), 2):
        if i+1 == len(sys.argv):
            break
        if sys.argv[i] == "--gperf":
            gperf = Path(sys.argv[i+1])
        elif sys.argv[i] == "--iterations":
            gperf_iterations = int(sys.argv[i+1])
        elif sys.argv[i] == "--lua-in":
            lua_in = Path(sys.argv[i+1])
        elif sys.argv[i] == "--lua-out":
            lua_out = Path(sys.argv[i+1])
    
    if gperf is None or lua_in is None or lua_out is None:
        print("Invalid arguments")
        exit(1)

    main(gperf=gperf, gperf_iterations=gperf_iterations, lua_in=lua_in, lua_out=lua_out)
