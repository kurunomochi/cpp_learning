"""Use SQLite's online backup API; never silently overwrite an existing backup."""
import argparse
from pathlib import Path
import sqlite3

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("destination", type=Path)
args = parser.parse_args()
source = args.source.resolve()
destination = args.destination.resolve()
if not source.is_file():
    parser.error("source database does not exist")
if destination.exists():
    parser.error("destination already exists; choose a new filename")
destination.parent.mkdir(parents=True, exist_ok=True)
src = sqlite3.connect(source.as_uri() + "?mode=ro", uri=True, timeout=5)
try:
    dst = sqlite3.connect(destination)
    try:
        src.backup(dst, pages=128, sleep=0.05)
        result = dst.execute("PRAGMA integrity_check").fetchone()[0]
        if result != "ok":
            raise RuntimeError("backup integrity check failed: " + result)
    finally:
        dst.close()
finally:
    src.close()
print("Backup verified:", destination)
