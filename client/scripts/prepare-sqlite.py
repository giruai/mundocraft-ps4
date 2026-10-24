#!/usr/bin/env python3
"""Apply the Orbis-only macOS/proxy guards to a generated SQLite build copy."""
from pathlib import Path
import sys,subprocess,tempfile,os
source,destination,patch=map(Path,sys.argv[1:])
with tempfile.TemporaryDirectory(dir=destination.parent) as tmp:
 output=Path(tmp)/"sqlite3-orbis.c"
 subprocess.run(["patch","--fuzz=0","--batch","--output",str(output),str(source),"--input",str(patch)],check=True)
 if not destination.exists() or destination.read_bytes()!=output.read_bytes():os.replace(output,destination)
