#!/usr/bin/env python3
import json
from pathlib import Path
import platform
import resource
import subprocess
import sys
import time

if len(sys.argv) != 3:
    raise SystemExit("usage: measure.py EXECUTABLE OUTPUT_JSON")

started = time.monotonic()
completed = subprocess.run([sys.argv[1]], check=False)
elapsed = time.monotonic() - started
usage = resource.getrusage(resource.RUSAGE_CHILDREN)
result = {
    "scope": "one normal test executable invocation on isolated Linux host; not ESP32-P4",
    "platform": platform.platform(),
    "wall_seconds": elapsed,
    "maximum_resident_set_kib": usage.ru_maxrss,
    "maximum_resident_set_note": "ru_maxrss is KiB on Linux",
    "returncode": completed.returncode,
}
Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
raise SystemExit(completed.returncode)
