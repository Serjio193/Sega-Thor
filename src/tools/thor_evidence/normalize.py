"""Normalize one bounded BizHawk V0 raw stream into sealed event JSONL."""
import argparse
import hashlib
import json
from pathlib import Path

from .events import write_capture
from .identity import ROM_SHA, ROM_SIZE, SCHEMA, canonical, digest

STATE_SHA = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
DEFAULT_ROM = Path("build/reference/Beyond Oasis (USA).bin")
DEFAULT_STATE = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Genesis\State\Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State")
DEFAULT_BIZHAWK = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64")


def file_hash(path):
    value = Path(path).read_bytes()
    return hashlib.sha256(value).hexdigest()


def records(path):
    result = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        value = json.loads(line)
        if value.get("kind") != "RAW_END":
            result.append(value)
    if not result or result[0].get("kind") != "EPOCH_BEGIN":
        raise ValueError("raw stream has no epoch")
    if result[-1].get("kind") != "EPOCH_END":
        raise ValueError("raw stream is not complete")
    return result


def normalize(raw_path, run_dir, output_path, mode, collector_path=None,
              harness_path=None, rom_path=DEFAULT_ROM, state_path=DEFAULT_STATE,
              bizhawk_dir=DEFAULT_BIZHAWK):
    raw = records(raw_path)
    run = Path(run_dir)
    rom = Path(rom_path)
    state = Path(state_path)
    if rom.stat().st_size != ROM_SIZE or file_hash(rom) != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    if file_hash(state) != STATE_SHA:
        raise ValueError("QuickSave1 identity mismatch")
    collector = Path(collector_path or __file__).resolve()
    harness = Path(harness_path or "build/bizhawk-controlled-harness/run.ps1").resolve()
    config = run / "config.ini"
    if not config.is_file():
        raise ValueError("run directory has no isolated config")
    core = Path(bizhawk_dir) / "dll" / "gpgx.wbx.zst"
    emulator = Path(bizhawk_dir) / "EmuHawk.exe"
    environment = {
        "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
        "emulator_sha256": file_hash(emulator), "core_sha256": file_hash(core),
        "config_sha256": file_hash(config), "collector_sha256": file_hash(collector),
        "watch_sha256": digest({"mode": mode, "watch": "TEE-V0-bounded-v1"}),
        "harness_sha256": file_hash(harness),
        "map_sha256": digest({"schema": "thor.m68k.bus-map.v0", "scope": "M68K BUS"}),
    }
    scenario = {"state_sha256": STATE_SHA, "specification": {
        "settle_frames": 3, "measurement_frames": 1, "input": "Right",
        "mode": mode, "restore_per_epoch": True, "rom_identity": ROM_SHA,
    }}
    header = {"schema": SCHEMA, "environment": environment, "scenario": scenario}
    events = []
    for raw_event in raw:
        kind = raw_event["kind"]
        events.append({"seq": raw_event["seq"], "epoch": raw_event["epoch"],
                       "frame": raw_event["frame"], "actor": raw_event["actor"],
                       "phase": raw_event["phase"], "kind": kind,
                       "data": raw_event.get("data", {}), "samples": []})
    return write_capture(output_path, header, events)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw")
    parser.add_argument("run_dir")
    parser.add_argument("output")
    parser.add_argument("--mode", default="probe")
    args = parser.parse_args(argv)
    print(normalize(args.raw, args.run_dir, args.output, args.mode))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
