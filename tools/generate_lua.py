from typing import TypedDict
from pathlib import Path
import sys
import re
from input_event_codes import *

FilesInfo = TypedDict("FilesInfo", {
    "lua-in": Path,
    "lua-out": Path
})

def main(x: FilesInfo) -> None:
    x["lua-out"].parent.mkdir(parents=True, exist_ok=True)
    
    with open(x["lua-in"], mode="rt", encoding="utf-8") as f:
        lines = [l.strip('\n') for l in f]

    codes = [c for c in input_event_codes() if c.startswith("KEY_") or c.startswith("BTN_")]
    ns = x["lua-out"].stem
    insert_after_re = re.compile(rf"---@alias {ns}.KeyName")

    with open(x["lua-out"], mode="wt", encoding="utf-8") as f:
        for l in lines:
            f.write(f"{l}\n")
            if insert_after_re.match(l):
                for k in codes:
                    f.write(f'---| "{k}"\n')

    print(f"tools/generate_lua.py: {x["lua-out"]}")

if __name__ == "__main__":
    expected_args = [f"--{s}" for s in FilesInfo.__annotations__.keys()]
    args = FilesInfo()

    for i in range(1, len(sys.argv), 2):
        if i+1 == len(sys.argv) or sys.argv[i] not in expected_args:
            print(f"Invalid argument: {sys.argv[i]}")
            exit(1)
        
        args[sys.argv[i][2:]] = Path(sys.argv[i+1])

    if len(args) != len(expected_args):
        print("Error: missing arguments")
        exit(1)

    main(args)

