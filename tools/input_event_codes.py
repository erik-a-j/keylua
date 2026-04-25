
from pathlib import Path
import re

def input_event_codes() -> list[str]:
    iec_search_paths = [Path("/usr/include/linux"), Path("/usr/local/include/linux")]
    iec_header = "input-event-codes.h"
    iec_path = None
    for p in iec_search_paths:
        if Path(p / iec_header).exists():
            iec_path = p / iec_header
    
    if iec_path is None:
        print(f"couldn't find {iec_header}")
        return False

    r = re.compile(r"^\s*#\s*define\s+([A-Z0-9_]+)\s+(?=\S)")
    
    with open(iec_path, mode="rt", encoding="utf-8") as f:
        codes = [m.group(1) for line in f if (m := r.search(line))]

    return codes

__all__ = ["input_event_codes"]
