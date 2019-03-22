from pathlib import Path as P
import sys

with open(sys.argv[1], "wb") as f:
    for filename in sys.argv[2:]:
        f.write(P(filename).read_bytes())