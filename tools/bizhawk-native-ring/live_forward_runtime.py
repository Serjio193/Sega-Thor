#!/usr/bin/env python3
"""Run and verify the bounded single-worker FLOW_V1 BizHawk proof."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


RECORD_WIDTH = 7
RESULT_WIDTH = 20
STATE_WIDTH = 20
REASON_DEPTH = 1
REASON_MEMORY = 2
FLAG_INSTRUCTION = 1
FLAG_COMPLETE = 2
FLAG_FAULTED = 4
FLAG_CONTROL_FLOW = 8
FLAG_EXCEPTION_EVENT = 256
FLAG_CPU_STOP_EVENT = 1024
SW_SHOWNORMAL = 1


def parse_lines(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.exists():
        return values
    for line in path.read_text(encoding="ascii", errors="replace").splitlines():
        key, separator, value = line.partition("=")
        if separator:
            values[key] = value
    return values


def parse_csv(value: str, width: int, label: str) -> list[int]:
    fields = [int(field, 10) for field in value.split(",") if field]
    if len(fields) != width:
        raise ValueError(f"{label}: expected {width} integers, got {len(fields)}")
    return fields


def parse_records(value: str) -> list[list[int]]:
    if not value:
        raise ValueError("record export is empty")
    records = [parse_csv(row, RECORD_WIDTH, "record") for row in value.split(";")]
    return records


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def state_hash(state: list[int]) -> str:
    canonical = ",".join(str(value) for value in state).encode("ascii")
    return sha256(canonical)


def segment_payload_hash(rom_sha256: str, native_sha256: str,
                         meta: list[int], entry: list[int], exit_state: list[int],
                         records: list[list[int]]) -> str:
    envelope = {
        "schema": "FLOW_V1",
        "core": "BizHawk-2.11.1/GPGX-native-live-forward-worker-1A",
        "rom_sha256": rom_sha256,
        "native_artifact_sha256": native_sha256,
        "metadata": meta,
        "entry_state": entry,
        "exit_state": exit_state,
        "records": records,
    }
    payload = json.dumps(envelope, sort_keys=True, separators=(",", ":"))
    return sha256(payload.encode("ascii"))


def verify_segment(values: dict[str, str], prefix: str,
                   expected_capture: int, expected_generation: int,
                   expected_run_id: int, rom_sha256: str,
                   native_sha256: str, reason: int) -> dict[str, object]:
    meta = parse_csv(values[f"{prefix}_META"], RESULT_WIDTH, f"{prefix} metadata")
    entry = parse_csv(values[f"{prefix}_ENTRY"], STATE_WIDTH, f"{prefix} ENTRY")
    exit_state = parse_csv(values[f"{prefix}_EXIT"], STATE_WIDTH, f"{prefix} EXIT")
    records = parse_records(values[f"{prefix}_RECORDS"])
    (capture_id, generation, run_id, epoch, entry_stream, exit_stream,
     entry_instruction, exit_instruction, entry_flow, exit_flow, worker_id,
     termination, configured_depth, configured_memory, consumed_depth,
     consumed_memory, record_count, records_bytes, valid, copy_ns) = meta

    if (capture_id, generation, worker_id) != (expected_capture, expected_generation, 0):
        raise ValueError(f"{prefix}: unexpected worker/capture/generation identity")
    if run_id != expected_run_id or epoch <= 0 or valid != 1:
        raise ValueError(f"{prefix}: missing identity or invalid segment")
    if termination != reason:
        raise ValueError(f"{prefix}: termination reason {termination}, expected {reason}")
    if record_count != len(records) or records_bytes != record_count * 32:
        raise ValueError(f"{prefix}: record count/byte bounds disagree")
    if exit_stream - entry_stream != record_count:
        raise ValueError(f"{prefix}: stream range is not exact")
    if records[0][0] != entry_stream or records[-1][0] + 1 != exit_stream:
        raise ValueError(f"{prefix}: stream endpoints disagree with records")
    if any(right[0] != left[0] + 1 for left, right in zip(records, records[1:])):
        raise ValueError(f"{prefix}: stream records are not contiguous")
    instruction_records = [record for record in records
                           if record[5] & FLAG_INSTRUCTION]
    if exit_instruction - entry_instruction != len(instruction_records):
        raise ValueError(f"{prefix}: instruction sequence bounds disagree")
    if any(record[1] != entry_instruction + offset
           for offset, record in enumerate(instruction_records)):
        raise ValueError(f"{prefix}: instruction sequence contains a gap")
    if exit_flow - entry_flow != consumed_depth:
        raise ValueError(f"{prefix}: control-flow cursor disagrees with depth")
    if any(not (record[5] & (FLAG_INSTRUCTION | FLAG_EXCEPTION_EVENT |
                             FLAG_CPU_STOP_EVENT))
           for record in records):
        raise ValueError(f"{prefix}: untyped execution record")
    if any(record[5] & FLAG_FAULTED for record in records):
        raise ValueError(f"{prefix}: faulted instruction cannot enter a valid segment")
    if any(record[5] & FLAG_INSTRUCTION and not record[5] & FLAG_COMPLETE
           for record in records):
        raise ValueError(f"{prefix}: instruction is neither complete nor visibly faulted")
    if reason == REASON_DEPTH:
        if configured_depth != 20 or consumed_depth != 20:
            raise ValueError(f"{prefix}: DEPTH=20 was not exactly reached")
        if exit_state[16] != records[-1][3]:
            raise ValueError(f"{prefix}: EXIT PC disagrees with the final record")
        first_instruction = next(
            (record for record in records if record[5] & FLAG_INSTRUCTION), None)
        if first_instruction is None or entry[16] != first_instruction[2]:
            raise ValueError(f"{prefix}: ENTRY PC disagrees with the first instruction")
    if reason == REASON_MEMORY and consumed_memory > configured_memory:
        raise ValueError(f"{prefix}: memory termination exceeded its configured bound")

    return {
        "worker_id": worker_id,
        "capture_id": capture_id,
        "generation": generation,
        "run_id": run_id,
        "epoch": epoch,
        "entry_stream_sequence": entry_stream,
        "exit_stream_sequence": exit_stream,
        "entry_instruction_sequence": entry_instruction,
        "exit_instruction_sequence": exit_instruction,
        "entry_control_flow_sequence": entry_flow,
        "exit_control_flow_sequence": exit_flow,
        "entry_pc": entry[16],
        "exit_pc": exit_state[16],
        "entry_state_sha256": state_hash(entry),
        "exit_state_sha256": state_hash(exit_state),
        "instruction_count": sum(bool(row[5] & FLAG_INSTRUCTION) for row in records),
        "record_count": record_count,
        "result_bytes": consumed_memory,
        "record_bytes": records_bytes,
        "configured_depth": configured_depth,
        "consumed_depth": consumed_depth,
        "configured_memory_bytes": configured_memory,
        "consumed_memory_bytes": consumed_memory,
        "copy_duration_ns": copy_ns,
        "termination_reason": termination,
        "content_sha256": segment_payload_hash(
            rom_sha256, native_sha256, meta, entry, exit_state, records),
        "records": records,
    }


def acknowledge(path: Path, segment: dict[str, object]) -> None:
    identity = "|".join(str(segment[field]) for field in
                        ("capture_id", "generation", "run_id", "epoch"))
    path.write_text(identity, encoding="ascii")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=240)
    args = parser.parse_args()

    executable = args.install / "EmuHawk.exe"
    if not executable.is_file() or not args.rom.is_file() or not args.script.is_file():
        parser.error("install, ROM, or Lua script path does not exist")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path = args.output_dir / "live-forward-runtime.txt"
    ack_path = args.output_dir / "live-forward-ack.txt"
    log_path = args.output_dir / "bizhawk-lua.log"
    config_path = args.output_dir / "isolated-bizhawk-config.ini"
    receipt_path = args.output_dir / "live-forward-flow-v1-receipt.json"
    for path in (raw_path, ack_path, log_path, receipt_path):
        path.unlink(missing_ok=True)

    config_template = args.install / "config.ini"
    if not config_template.is_file():
        raise FileNotFoundError(f"isolated BizHawk config template missing: {config_template}")
    config = json.loads(config_template.read_text(encoding="utf-8-sig"))
    config["SingleInstanceMode"] = False
    config["RunInBackground"] = True
    config["AcceptBackgroundInput"] = False
    config["StartPaused"] = False
    config["AutoLoadLastSaveSlot"] = False
    config["AutoSaveLastSaveSlot"] = False
    config["AutosaveSaveRAM"] = False
    config.setdefault("Rewind", {})["Enabled"] = False
    for section in ("RecentRoms", "RecentMovies", "RecentLua", "RecentLuaSession"):
        config.setdefault(section, {})["AutoLoad"] = False
    for tool in config.get("CommonToolSettings", {}).values():
        if isinstance(tool, dict):
            tool["AutoLoad"] = False
    for path_entry in config.get("PathEntries", {}).get("Paths", []):
        if path_entry.get("Type") in ("Save RAM", "Savestates", "State"):
            path_entry["Path"] = str(args.output_dir)
    config_path.write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")

    rom_sha256 = sha256(args.rom.read_bytes())
    native_artifact = args.install / "dll" / "gpgx.wbx"
    native_sha256 = sha256(native_artifact.read_bytes())
    run_id = int(time.time())
    env = os.environ.copy()
    env.update({
        "LF_OUTPUT": str(raw_path),
        "LF_ACK": str(ack_path),
        "LF_RUN_ID": str(run_id),
        "LF_MAX_FRAMES": "1800",
        "BH_TEST_LOG": str(log_path),
    })
    command = [
        str(executable), f"--config={config_path}", f"--lua={args.script}",
        str(args.rom),
    ]
    startup_info = subprocess.STARTUPINFO()
    startup_info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup_info.wShowWindow = SW_SHOWNORMAL
    process = subprocess.Popen(command, cwd=args.install, env=env,
                               startupinfo=startup_info,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    segments: dict[str, dict[str, object]] = {}
    handled: set[str] = set()
    started = time.monotonic()
    try:
        while time.monotonic() - started < args.timeout:
            values = parse_lines(raw_path)
            if values.get("RESULT") == "FAIL":
                raise RuntimeError(values.get("ERROR", "Lua runtime failed"))
            for name, ready, capture, generation, reason in (
                ("first", "FIRST_READY", 1, 1, REASON_DEPTH),
                ("second", "SECOND_READY", 2, 2, REASON_DEPTH),
                ("memory", "MEMORY_READY", 3, 3, REASON_MEMORY),
            ):
                if name in handled or values.get(ready) != "true":
                    continue
                if name == "first":
                    if values.get("FIRST_IMMUTABLE_EQUAL") != "true":
                        raise ValueError("first COMPLETE result changed after ring advancement")
                    if values.get("FIRST_RING_WRAPPED") != "true":
                        raise ValueError("shared execution ring did not advance through wrap")
                elif values.get(f"{name.upper()}_IMMUTABLE_EQUAL") != "true":
                    raise ValueError(f"{name} COMPLETE result changed after publication")
                prefix = {
                    "first": "FIRST_BEFORE",
                    "second": "SECOND_BEFORE",
                    "memory": "MEMORY_BEFORE",
                }[name]
                segment = verify_segment(values, prefix, capture, generation,
                                         run_id, rom_sha256, native_sha256, reason)
                if name == "second":
                    previous = segments["first"]
                    if (segment["capture_id"] == previous["capture_id"] or
                            segment["generation"] == previous["generation"] or
                            segment["epoch"] != previous["epoch"] or
                            segment["entry_stream_sequence"] <= previous["exit_stream_sequence"]):
                        raise ValueError("reconnected Worker did not attach to later execution")
                segments[name] = segment
                acknowledge(ack_path, segment)
                handled.add(name)
            if values.get("RESULT") == "PASS":
                break
            if process.poll() is not None:
                raise RuntimeError(f"EmuHawk exited early with code {process.returncode}")
            time.sleep(0.02)
        else:
            raise TimeoutError(f"BizHawk runtime exceeded {args.timeout}s")
        if process.wait(timeout=10) != 0:
            raise RuntimeError(f"EmuHawk exited with code {process.returncode}")
        if handled != {"first", "second", "memory"}:
            raise ValueError(f"missing host analyses/ACKs: {sorted({'first', 'second', 'memory'} - handled)}")
        values = parse_lines(raw_path)
        if values.get("FIRST_IMMUTABLE_EQUAL") != "true" or values.get("SECOND_IMMUTABLE_EQUAL") != "true":
            raise ValueError("Lua immutability comparisons failed")
        performance_keys = {"PERF_BASELINE", "PERF_RECORDER", "PERF_WORKER"}
        if not performance_keys.issubset(values):
            raise ValueError("baseline/recorder/Worker timing measurements are incomplete")
        timings = {key: value for key, value in values.items()
                   if key.startswith("PERF_") or key.endswith("_COPY_MS") or
                   key.endswith("_EXPORT_MS")}
        receipt = {
            "status": "PASS_LIVE_FORWARD_SINGLE_WORKER_FLOW_V1",
            "rom_path": str(args.rom.resolve()),
            "rom_sha256": rom_sha256,
            "native_artifact_sha256": native_sha256,
            "run_id": run_id,
            "segments": segments,
            "immutable_checks": {
                "first_hash_equal": segments["first"]["content_sha256"] ==
                                    verify_segment(values, "FIRST_AFTER", 1, 1,
                                                   run_id, rom_sha256, native_sha256,
                                                   REASON_DEPTH)["content_sha256"],
                "second_hash_equal": segments["second"]["content_sha256"] ==
                                     verify_segment(values, "SECOND_AFTER", 2, 2,
                                                    run_id, rom_sha256, native_sha256,
                                                    REASON_DEPTH)["content_sha256"],
                "memory_hash_equal": segments["memory"]["content_sha256"] ==
                                     verify_segment(values, "MEMORY_AFTER", 3, 3,
                                                    run_id, rom_sha256, native_sha256,
                                                    REASON_MEMORY)["content_sha256"],
            },
            "performance": {
                **timings,
                "measurement_note": "p50 and maximum are per-frame host duration over 120 frames; native copy and Lua export are separate",
            },
            "native_runtime_log": str(raw_path.resolve()),
        }
        if not all(receipt["immutable_checks"].values()):
            raise ValueError("host SHA-256 changed across immutable result reread")
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"receipt": str(receipt_path), "status": receipt["status"]}, indent=2))
        return 0
    except Exception as exc:  # preserve evidence on runtime failure
        failure = {
            "status": "STOP_LIVE_FORWARD_RUNTIME_FAILURE",
            "error": str(exc),
            "emuhawk_exit_code": process.poll(),
            "runtime_output": str(raw_path),
            "lua_log": str(log_path),
            "handled_segments": sorted(handled),
        }
        receipt_path.write_text(json.dumps(failure, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(failure, indent=2), file=sys.stderr)
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
