"""Chapter 26 exercise: add priority to an existing tutorial DB. Stop the app and back up first.
This changes the schema only; implementing priority in the C++ API remains the exercise.
"""
import argparse
from pathlib import Path
import sqlite3

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("database", type=Path)
args = parser.parse_args()
path = args.database.resolve()
if not path.is_file():
    parser.error("database must already exist")
db = sqlite3.connect(path.as_uri() + "?mode=rw", uri=True, isolation_level=None, timeout=5)
try:
    db.execute("BEGIN IMMEDIATE")
    version = db.execute("PRAGMA user_version").fetchone()[0]
    columns = {row[1] for row in db.execute("PRAGMA table_info(tasks)")}
    if version == 0 and "priority" not in columns and {"id", "title", "completed"} <= columns:
        db.execute("ALTER TABLE tasks ADD COLUMN priority INTEGER NOT NULL DEFAULT 1 CHECK(priority BETWEEN 0 AND 2)")
        db.execute("PRAGMA user_version=1")
        print("Schema migrated: 0 -> 1")
    elif version == 1 and "priority" in columns:
        print("Schema already at version 1")
    else:
        raise RuntimeError("unexpected schema/version; review manually before migrating")
    db.execute("COMMIT")
except BaseException:
    if db.in_transaction:
        db.execute("ROLLBACK")
    raise
finally:
    db.close()
