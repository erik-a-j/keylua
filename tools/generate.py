
from os import truncate
from typing import TypedDict
from dataclasses import dataclass, field
from pathlib import Path
import sys
import subprocess
import re

PROJECT_DIR = Path(__file__).parent.parent
IEC_SEARCH_PATHS = [Path("/usr/include/linux"), Path("/usr/local/include/linux")]
IEC_HEADER = "input-event-codes.h"
PRINT_GENERATED = "tools/generate.py: {}"

FilesInfo = TypedDict("FilesInfo", {
    "gperf-iterations": int, 
    "gperf-in": Path,
    "gperf-out": Path,
    "gperf-alias-in": Path,
    "gperf-alias-out": Path,
    "cpp-out": Path,
    "h-out": Path,
    "lua-in": Path,
    "lua-out": Path
})

GperfDefs = TypedDict("GperfDefs", {
    "slot-name": str,
    "class-name": str,
    "hash-function-name": str,
    "lookup-function-name": str,
    "r-lookup-function-name": str,
    "word-array-name": str,
    "constants-prefix": str
})

GperfInfo = TypedDict("GperfInfo", {
    "max-hash-value": int
})

@dataclass
class GenInfo:
    files: FilesInfo

    kw_struct: str = field(default_factory=str)
    gperf_defs: GperfDefs = field(default_factory=GperfDefs)
    gperf_alias_defs: GperfDefs = field(default_factory=GperfDefs)

    gperf_info: GperfInfo = field(default_factory=GperfInfo)
    gperf_alias_info: GperfInfo = field(default_factory=GperfInfo)

    gperf_pairs: list[str] = field(default_factory=list)
    gperf_alias_pairs: list[tuple[str]] = field(default_factory=list)

def generate_gperf(x: GenInfo, alias: bool) -> None:

    gperf_in = x.files["gperf-alias-in"] if alias else x.files["gperf-in"]
    gperf_out = x.files["gperf-alias-out"] if alias else x.files["gperf-out"]
    gperf_defs = x.gperf_alias_defs if alias else x.gperf_defs 

    defs_re = re.compile(
        rf'^%define\s(?P<def>{"|".join(GperfDefs.__annotations__.keys())})\s(?P<name>\w+)\s*\n$'
    )
    include_start_re = re.compile(r"^%\{\s*\n$")
    token_mark_re = re.compile(r"^%%\s*$")
    struct_start_re = re.compile(r"^struct\s+keyword\s*\{.*\};\s*\n$")

    with open(gperf_in, mode="rt", encoding="utf-8") as fin, open(gperf_out, mode="wt", encoding="utf-8") as fout:
        lines = fin.readlines()

        for i, line in enumerate(lines):
            if (m := defs_re.search(line)):
                define = m.group('def')
                name = m.group('name')

                gperf_defs[define] = name
                if define == "lookup-function-name":
                    gperf_defs["r-lookup-function-name"] = f"r{name}"
            elif not alias and struct_start_re.match(line):
                x.kw_struct = line
            
            fout.write(line)

            if include_start_re.match(line):
                fout.write(f'#include "{x.files["h-out"].name}"\n')
            elif token_mark_re.match(line):
                if alias:
                    for pair_line in lines[i+1:]:
                        if token_mark_re.match(pair_line):
                            break
                        pair = pair_line.strip().rsplit(", ")
                        x.gperf_alias_pairs.append((pair[0], pair[1]))
                else:
                    for tok in x.gperf_pairs:
                        fout.write(f"{tok}, {tok}\n")
                    fout.write("%%")
                    break


    print(PRINT_GENERATED.format(gperf_out))
    
def generate_cpp(x: GenInfo, alias: bool) -> list[str]:

    gperf_out = x.files["gperf-alias-out"] if alias else x.files["gperf-out"]
    cpp_out = x.files["cpp-out"]
    gperf_defs = x.gperf_alias_defs if alias else x.gperf_defs
    gperf_info = x.gperf_alias_info if alias else x.gperf_info

    res: subprocess.CompletedProcess = subprocess.run(
        ["gperf", "-m", str(x.files["gperf-iterations"]), str(gperf_out)], capture_output=True, text=True
    )
    if res.returncode != 0:
        print(res.stderr)
        exit(1)
    
    max_hash_value_re = re.compile(rf"^\s*{gperf_defs['constants-prefix']}MAX_HASH_VALUE\s*=\s*(?P<value>\d+)(?:,\s*|\s*)$")
    class_start_match = re.compile(rf"^class\s+{gperf_defs['class-name']}")
    class_start = 0
    class_end = 0
    lines: list[str] = str(res.stdout).splitlines(keepends=True)
    with open(cpp_out, mode="at", encoding="utf-8") as f:
        i = 0
        while i < len(lines):
            if (m := max_hash_value_re.search(lines[i])):
                gperf_info["max-hash-value"] = int(m.group("value"))
            elif class_start_match.match(lines[i]):
                depth = 0
                for j, line in enumerate(lines[i:], start=i):
                    depth += line.count('{') - line.count('}')
                    if depth == 0 and j > i:
                        class_end = j + 1
                        break
                class_start = i
                i = class_end + 1
            
            f.write(lines[i])
            i += 1
        
        f.write(f"const struct keyword* {gperf_defs['class-name']}::{gperf_defs['word-array-name'].upper()} = {gperf_defs['word-array-name']};\n")
    
    print(PRINT_GENERATED.format(cpp_out))
    return lines[class_start:class_end]

def generate_h(x: GenInfo, decl: list[str], alias_decl: list[str]) -> None:
    if len(decl) != len(alias_decl):
        print("Error: len(decl) != len(alias_decl)")
        exit(1)

    hguard = f"{x.files["h-out"].stem.upper()}_H"
    lookup_name = x.gperf_defs['lookup-function-name']
    lookup_alias_name = x.gperf_alias_defs['lookup-function-name']
    lookup_re = re.compile(
        rf"(?P<indent>^\s*)static\s+(?P<rtype>const\s+struct\s+keyword\s*\*\s*)(?P<fname>{lookup_alias_name})(?P<params>\s*\(const\s*char\s*\*\s*(?P<p_str>\w+),\s*size_t\s*(?P<p_len>\w+)\));\s*\n$"
    )
    func_indent = ""

    with open(x.files["h-out"], mode="wt", encoding="utf-8") as f:
        includes = ["<linux/input-event-codes.h>", "<cstddef>", "<cstdint>", "<cstring>", "<unordered_map>"]
        f.write(f"#ifndef {hguard}\n#define {hguard}\n\n")
        for inc in includes:
            f.write(f"#include {inc}\n")
        f.write("\n")

        f.write(f"{x.kw_struct}\n")

        for i in range(0, len(decl)):
            f.write(decl[i])
            if alias_decl[i] != decl[i]:
                f.write(alias_decl[i])
                if (m := lookup_re.search(alias_decl[i])):
                    indent = m.group('indent')
                    func_indent = indent
                    rtype = m.group('rtype')
                    fname = m.group('fname')[0:m.group('fname').find('_')]
                    params = m.group('params')
                    p_str = m.group('p_str')
                    p_len = m.group('p_len')
                    f.write(f"\n{indent}static constexpr size_t MAX_HASH_VALUE_CODE = {x.gperf_info['max-hash-value']};\n")
                    f.write(f"{indent}static constexpr size_t MAX_HASH_VALUE_ALIAS = {x.gperf_alias_info['max-hash-value']};\n")
                    f.write(f"{indent}static {rtype}{x.gperf_defs['word-array-name'].upper()};\n")
                    f.write(f"{indent}static {rtype}{x.gperf_alias_defs['word-array-name'].upper()};\n\n")
                    f.write(f"{indent}static {rtype}{fname}{params};\n")
                    f.write(f"{indent}static std::unordered_map<uint16_t, const char*> {x.gperf_defs['r-lookup-function-name']}();\n")
                    f.write(f"{indent}static std::unordered_map<uint16_t, const char*> {x.gperf_alias_defs['r-lookup-function-name']}();\n")

        f.write(f"\n#endif /* #ifndef {hguard} */\n")
    
    print(PRINT_GENERATED.format(x.files["h-out"]))

def generate_lua(x: GenInfo) -> None:
    lua_in = x.files["lua-in"]
    lua_out = x.files["lua-out"]
    ns = lua_out.stem
    #---@alias keylua.KeyName

    with open(lua_in, mode="rt", encoding="utf-8") as f:
        lines = [l.strip('\n') for l in f]
    
    m = re.compile(rf"---@alias {ns}.KeyName")

    with open(lua_out, mode="wt", encoding="utf-8") as f:
        for l in lines:
            f.write(f"{l}\n")
            if m.match(l):
                for k, _ in x.gperf_alias_pairs:
                    if k == '","':
                        k = ','
                    f.write(f'---| "{k}"\n')
                for k in x.gperf_pairs:
                    f.write(f'---| "{k}"\n')

    print(PRINT_GENERATED.format(lua_out))

def main(x: GenInfo) -> None:

    iec_path = None
    for p in IEC_SEARCH_PATHS:
        if Path(p / IEC_HEADER).exists():
            iec_path = p / IEC_HEADER
    
    if iec_path is None:
        print(f"couldn't find {IEC_HEADER}")
        exit(1)

    with open(iec_path, mode="rt", encoding="utf-8") as f:
        r = re.compile(r"^#define\s+(KEY_\w+|BTN_\w+)")
        x.gperf_pairs = [m.group(1) for line in f if (m := r.search(line))]

    Path.mkdir(x.files["gperf-out"].parent, parents=True, exist_ok=True)
    Path.mkdir(x.files["gperf-alias-out"].parent, parents=True, exist_ok=True)
    Path.mkdir(x.files["lua-out"].parent, parents=True, exist_ok=True)
    with open(x.files["cpp-out"], mode="w"):
        pass

    generate_gperf(x, alias=False)
    generate_gperf(x, alias=True)
    generate_h(x, generate_cpp(x, alias=False), generate_cpp(x, alias=True))
    generate_lua(x)



if __name__ == "__main__":
    expected_args = [f"--{s}" for s in FilesInfo.__annotations__.keys()]
    files = FilesInfo()

    for i in range(1, len(sys.argv), 2):
        if i+1 == len(sys.argv) or sys.argv[i] not in expected_args:
            print(f"Invalid argument: {sys.argv[i]}")
            exit(1)
        if sys.argv[i] == expected_args[0]:
            files[sys.argv[i][2:]] = int(sys.argv[i+1])
        else:
            files[sys.argv[i][2:]] = Path(sys.argv[i+1])
    
    if len(files) != len(expected_args):
        print("Error: not missing arguments")
        exit(1)

    x = GenInfo(files)

    main(x)

