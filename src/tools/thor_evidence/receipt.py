"""Local launch receipt for the bounded V0.1 capture contract."""
import argparse
import hashlib
import json
import uuid
from pathlib import Path

from .identity import ROM_SHA, ROM_SIZE, SCHEMA, STATE_SHA, canonical, digest

RECEIPT_SCHEMA = "thor.evidence.launch-receipt.v0.1"
RAW_SCHEMA = "thor.evidence.raw.v0.1"
MUTABLE = {"status", "raw_sha256", "raw_bytes", "config_sha256", "launch_sha256", "result_sha256"}


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def watch_plan(mode, reverse):
    plan = [["EXEC", 0xA372, "exec-A372"], ["WRITE", 0xFF13CC, "w0"]]
    if mode == "probe":
        plan += [["EXEC", pc, f"exec-{pc:X}"] for pc in
                 (0xA36C, 0xA36E, 0xA370, 0xA374, 0xA376, 0xA378,
                  0xA37A, 0xA37C, 0x1FCA, 0x1FD0, 0xA42A, 0xA430,
                  0xA436, 0xA358, 0xA35E)]
        plan += [["WRITE", 0xFF13CC + i, f"sat+{i}"] for i in range(1, 8)]
        plan += [["WRITE", 0xFF188A + i, f"field+{i}"] for i in range(4)]
        plan += [["WRITE", 0xFF13CC, "duplicate-start"]]
        plan += [["READ", address, f"read-{address:X}"] for address in
                 (0xA438, 0xA439, 0xA43A, 0xA43C, 0xFF188A, 0xFF188C, 0xFF1858)]
    ordered = list(reversed(plan)) if reverse else plan
    return ordered + [["INPUT", 0, "input-poll"]]


def plan_hash(plan):
    return digest({"schema": "thor.evidence.watch-plan.v0.1", "ordered": plan})


def _immutable(payload):
    return {key: value for key, value in payload.items()
            if key not in MUTABLE and key != "receipt_sha256"}


def receipt_hash(payload):
    return digest({"schema": RECEIPT_SCHEMA, "identity": _immutable(payload)})


def create(path, raw_path, run_dir, mode, reverse, collector, normalizer,
           harness, rom, state, bizhawk):
    mode = str(mode)
    if mode not in {"minimal", "probe", "uninstrumented"}:
        raise ValueError("unsupported capture mode")
    collector, normalizer, harness, rom, state, bizhawk = map(Path,
        (collector, normalizer, harness, rom, state, bizhawk))
    emulator, core = bizhawk / "EmuHawk.exe", bizhawk / "dll" / "gpgx.wbx.zst"
    plan = watch_plan(mode, bool(reverse))
    payload = {
        "schema": RECEIPT_SCHEMA, "capture_id": str(uuid.uuid4()),
        "raw_path": str(Path(raw_path).resolve()), "run_dir": str(Path(run_dir).resolve()),
        "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE, "state_sha256": STATE_SHA,
        "collector_path": str(collector.resolve()), "collector_sha256": file_hash(collector),
        "normalizer_path": str(normalizer.resolve()), "normalizer_sha256": file_hash(normalizer),
        "harness_path": str(harness.resolve()), "harness_sha256": file_hash(harness),
        "emulator_sha256": file_hash(emulator), "core_sha256": file_hash(core),
        "map_sha256": digest({"schema": "thor.m68k.bus-map.v0", "scope": "M68K BUS"}),
        "mode": mode, "reverse": bool(reverse), "watch_plan": plan,
        "watch_plan_sha256": plan_hash(plan),
        "scenario": {"settle_frames": 3, "measurement_frames": 1, "input": "Right",
                      "restore_per_epoch": True, "expected_epochs": 2,
                      "mode": mode, "reverse": bool(reverse)},
        "status": "PLANNED",
    }
    payload["receipt_sha256"] = receipt_hash(payload)
    Path(path).write_text(canonical(payload) + "\n", encoding="utf-8")
    return payload


def load(path):
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    if payload.get("schema") != RECEIPT_SCHEMA:
        raise ValueError("unsupported launch receipt")
    if payload.get("receipt_sha256") != receipt_hash(payload):
        raise ValueError("launch receipt identity mismatch")
    return payload


def finalize(path, result_log=None):
    receipt = load(path)
    raw = Path(receipt["raw_path"])
    run = Path(receipt["run_dir"])
    config = run / "config.ini"
    launch = run / "launch.json"
    if not raw.is_file() or not config.is_file() or not launch.is_file():
        raise ValueError("launch did not produce required receipt artifacts")
    receipt.update({"status": "COMPLETED", "raw_sha256": file_hash(raw),
                    "raw_bytes": raw.stat().st_size, "config_sha256": file_hash(config),
                    "launch_sha256": file_hash(launch)})
    if result_log and Path(result_log).is_file():
        receipt["result_sha256"] = file_hash(result_log)
    Path(path).write_text(canonical(receipt) + "\n", encoding="utf-8")
    return receipt


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    c = sub.add_parser("create")
    for name in ("path", "raw_path", "run_dir", "collector", "normalizer", "harness", "rom", "state", "bizhawk"):
        c.add_argument("--" + name.replace("_", "-"), required=True)
    c.add_argument("--mode", required=True); c.add_argument("--reverse", action="store_true")
    f = sub.add_parser("finalize"); f.add_argument("path"); f.add_argument("--result-log")
    args = parser.parse_args(argv)
    if args.command == "create":
        result = create(args.path, args.raw_path, args.run_dir, args.mode, args.reverse,
                        args.collector, args.normalizer, args.harness, args.rom,
                        args.state, args.bizhawk)
    else:
        result = finalize(args.path, args.result_log)
    print(canonical(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
