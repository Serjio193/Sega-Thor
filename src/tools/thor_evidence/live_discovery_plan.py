"""Build a read-only, local watch plan from accumulated M12 evidence."""
import argparse
import hashlib
import json
import sqlite3
from pathlib import Path


ROM_SIZE = 0x300000
PAGE_SIZE = 0x1000


def db_addresses(paths, kind):
    result = set()
    traces = []
    for path in paths:
        db = sqlite3.connect(f"file:{Path(path).resolve()}?mode=ro", uri=True)
        for row in db.execute("SELECT id FROM trace ORDER BY id"):
            traces.append(row[0])
        for (payload,) in db.execute("SELECT payload FROM event WHERE kind=?", (kind,)):
            data = json.loads(payload).get("data", {})
            if isinstance(data.get("address"), int):
                result.add(data["address"])
        db.close()
    return result, traces


def make_plan(manifest_path, database_paths, gpgx_path):
    manifest = json.loads(Path(manifest_path).read_text())
    unknown = [False] * (ROM_SIZE // PAGE_SIZE)
    for entry in manifest["entries"]:
        if entry["kind"] != "UNKNOWN":
            continue
        for page in range(entry["start"] // PAGE_SIZE, (entry["end"] + PAGE_SIZE - 1) // PAGE_SIZE):
            unknown[page] = True
    execs, traces = db_addresses(database_paths, "EXEC")
    reads, read_traces = db_addresses(database_paths, "READ")
    writes, write_traces = db_addresses(database_paths, "WRITE")
    traces = sorted(set(traces + read_traces + write_traces))
    gpgx = json.loads(Path(gpgx_path).read_text())
    gpgx_execs = {int(value, 0) for value in gpgx.get("executed_addresses", [])}
    execs.update(gpgx_execs)
    payload = {
        "schema": "oasis.m68k.m12-live-watch-plan.v1",
        "rom_sha256": manifest["rom_sha256"],
        "manifest": str(Path(manifest_path).resolve()),
        "database_traces": traces,
        "known_exec_count": len(execs),
        "known_db_read_count": len(reads),
        "known_db_write_count": len(writes),
        "gpgx_known_exec_count": len(gpgx_execs),
        "unknown_rom_page_count": sum(unknown),
    }
    lua = ["return {", '  schema = "oasis.m68k.m12-live-watch-plan.v1",']
    for name, values in (("known_exec", sorted(execs)),
                         ("known_read", sorted(reads)),
                         ("known_write", sorted(writes))):
        lua.append(f"  {name} = {{")
        lua.extend(f"    [{value}] = true," for value in values)
        lua.append("  },")
    lua.append("  unknown_rom_pages = {")
    lua.extend(f"    [{page}] = true," for page, value in enumerate(unknown) if value)
    lua.append("  },")
    lua.append("}\n")
    lua_bytes = "\n".join(lua).encode()
    payload["watch_plan_sha256"] = hashlib.sha256(lua_bytes).hexdigest()
    return payload, lua_bytes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--database", action="append", required=True)
    parser.add_argument("--gpgx", required=True)
    parser.add_argument("--json", required=True)
    parser.add_argument("--lua", required=True)
    args = parser.parse_args()
    payload, lua = make_plan(args.manifest, args.database, args.gpgx)
    Path(args.json).write_text(json.dumps(payload, indent=2) + "\n")
    Path(args.lua).write_bytes(lua)
    print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
