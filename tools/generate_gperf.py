
import gperf 
from pathlib import Path
import sys

IEC_SEARCH_PATHS = [Path("/usr/include/linux"), Path("/usr/local/include/linux")]
IEC_HEADER = "input-event-codes.h"

def write_gperf(out: str, valid_evcodes: list[str]) -> None:
    
    with open(out, mode="wt", encoding="utf-8") as f:
        f.write(gperf.BEGIN)
        for k, v in gperf.PAIRS:
            if v not in valid_evcodes:
                print(f"WARNING: {v} is not valid")
            f.write(f"{k}, {v}\n")
        f.write(gperf.END)
        

def main(out: str) -> None:
    iec_path = None
    for p in IEC_SEARCH_PATHS:
        if Path(p / IEC_HEADER).exists():
            iec_path = p / IEC_HEADER
    
    if iec_path is None:
        print(f"couldn't find {IEC_HEADER}")
        exit(1)

    with open(iec_path, mode="rt", encoding="utf-8") as f:
        iec_all_evcodes = [l[8:l.find('\t', 8)] for l in f if l.find("#define KEY_") != -1 or l.find("#define BTN_") != -1]
    
    write_gperf(out, iec_all_evcodes)
    
    
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Expected arg for output")
        exit(1)
    main(sys.argv[1])
