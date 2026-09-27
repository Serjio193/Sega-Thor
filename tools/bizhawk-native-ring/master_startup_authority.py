"""MASTER V2 only startup validation and continuation boundary for M12 R4."""
from __future__ import annotations

from dataclasses import dataclass
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import sys
import time
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
if str(Path(__file__).parent) not in sys.path:
    sys.path.insert(0, str(Path(__file__).parent))

from master_canonical_view import MasterCanonicalView  # noqa: E402
from master_outcome_view import MasterOutcomeView, open_outcome_authority  # noqa: E402
from master_v2_shadow import (ROM_SHA, ROM_SIZE, SCHEMA, decode_master_v2,
                              LegacyState, discover, _json_bytes, _sha)  # noqa: E402

POINTER_SCHEMA = "oasis.m12.master-v2-startup.current.v1"
STOP_UNAVAILABLE = "STOP_MASTER_STARTUP_UNAVAILABLE"
STOP_INTEGRITY = "STOP_MASTER_STARTUP_INTEGRITY"


def _stop(code: str, detail: str) -> ValueError:
    return ValueError(f"{code}:{detail}")


def _rss() -> int | None:
    """Return the current process working set without adding a dependency."""
    if sys.platform != "win32":
        return None
    class Counters(ctypes.Structure):
        _fields_ = [("cb", ctypes.c_ulong), ("page_fault_count", ctypes.c_ulong),
                    ("peak_working_set", ctypes.c_size_t), ("working_set", ctypes.c_size_t),
                    ("quota_peak_paged", ctypes.c_size_t), ("quota_paged", ctypes.c_size_t),
                    ("quota_peak_nonpaged", ctypes.c_size_t), ("quota_nonpaged", ctypes.c_size_t),
                    ("pagefile", ctypes.c_size_t), ("peak_pagefile", ctypes.c_size_t)]
    counters = Counters(); counters.cb = ctypes.sizeof(counters)
    process_api = ctypes.WinDLL("kernel32", use_last_error=True)
    memory_api = ctypes.WinDLL("psapi", use_last_error=True)
    memory_api.GetProcessMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.POINTER(Counters), ctypes.c_ulong]
    memory_api.GetProcessMemoryInfo.restype = ctypes.c_int
    process = process_api.GetCurrentProcess()
    if memory_api.GetProcessMemoryInfo(process, ctypes.byref(counters), counters.cb):
        return int(counters.working_set)
    return None


def _hash(path: Path) -> tuple[str, int]:
    digest = hashlib.sha256(); size = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block); size += len(block)
    return digest.hexdigest(), size


def _read_pointer(path: Path) -> dict[str, Any]:
    try:
        pointer = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise _stop(STOP_UNAVAILABLE, "current-pointer") from error
    if not isinstance(pointer, dict) or pointer.get("schema") != POINTER_SCHEMA:
        raise _stop(STOP_INTEGRITY, "pointer-schema")
    required = ("generation_id", "path", "sha256", "logical_sha256", "rom_sha256",
                "rom_size", "master_schema")
    if any(not pointer.get(key) for key in required):
        raise _stop(STOP_INTEGRITY, "pointer-fields")
    return pointer


def _section_map(decoded: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(item["name"]): item for item in decoded.get("sections", [])}


def _validate_emission(view: MasterCanonicalView, expected_source_owned: int | None) -> dict[str, int]:
    rows = sorted(list(view.emission_partition), key=lambda row: (int(row["start"]), int(row["end"])))
    totals = {name: 0 for name in ("ASM", "DATA", "ASSET", "INCBIN")}
    cursor = 0
    for row in rows:
        start, end, kind = int(row["start"]), int(row["end"]), str(row["emission_type"])
        if start != cursor or end <= start or kind not in totals:
            raise _stop(STOP_INTEGRITY, "emission-partition")
        cursor = end; totals[kind] += end - start
    if cursor != ROM_SIZE or sum(totals.values()) != ROM_SIZE:
        raise _stop(STOP_INTEGRITY, "emission-size")
    owned = view.source_owned_bytes
    if expected_source_owned is not None and owned != int(expected_source_owned):
        raise _stop(STOP_INTEGRITY, "source-owned")
    return totals


@dataclass(frozen=True)
class StartupMetrics:
    master_bytes: int
    legacy_bytes_read: int
    startup_elapsed_ms: int
    peak_ram_bytes: int | None


@dataclass(frozen=True)
class MasterPointer:
    path: Path
    data: dict[str, Any]


@dataclass
class MasterStartupState:
    pointer: MasterPointer
    master_path: Path
    canonical: MasterCanonicalView
    outcomes: MasterOutcomeView
    provenance: Any
    metrics: StartupMetrics
    emission_bytes: dict[str, int]

    @property
    def generation_id(self) -> str:
        return self.canonical.generation_id

    @property
    def master_sha256(self) -> str:
        return str(self.pointer.data["sha256"])

    def prepare_candidate(self, delta: Any) -> dict[str, Any]:
        """Prepare a deterministic N+1 continuation descriptor without legacy reads."""
        encoded = _json_bytes(delta)
        delta_sha = hashlib.sha256(encoded).hexdigest()
        seed = (self.generation_id + self.master_sha256 + delta_sha).encode("ascii")
        generation = "master-v2-" + hashlib.sha256(seed).hexdigest()[:16]
        return {"status": "CANDIDATE_READY", "generation_id": generation,
                "parent_generation": self.generation_id,
                "parent_master_sha256": self.master_sha256,
                "delta_sha256": delta_sha, "source_owned_before": self.canonical.source_owned_bytes,
                "source_owned_after": self.canonical.source_owned_bytes}


def load_startup(project_root: Path, rom_path: Path,
                 pointer_path: Path | None = None,
                 legacy_shadow_compare: bool = False,
                 legacy_state: LegacyState | None = None) -> MasterStartupState:
    started = time.perf_counter(); peak = _rss()
    project_root, rom_path = Path(project_root).resolve(), Path(rom_path).resolve()
    pointer_path = pointer_path or project_root / "build" / "thor-evidence" / "master-v2" / "current.json"
    pointer = _read_pointer(pointer_path)
    master_path = (pointer_path.parent / str(pointer["path"])).resolve()
    if not master_path.is_file():
        raise _stop(STOP_UNAVAILABLE, "master-file")
    file_sha, file_bytes = _hash(master_path)
    if file_sha != pointer["sha256"] or file_bytes != int(pointer.get("bytes", file_bytes)):
        raise _stop(STOP_INTEGRITY, "master-file-hash")
    decoded = decode_master_v2(master_path)
    if decoded["schema"] != pointer["master_schema"] or decoded["overall_sha256"] != pointer["logical_sha256"]:
        raise _stop(STOP_INTEGRITY, "logical-master-hash")
    sections = _section_map(decoded)
    expected_sections = pointer.get("sections", {})
    if expected_sections and any(expected_sections.get(name) != item["sha256"]
                                 for name, item in sections.items()):
        raise _stop(STOP_INTEGRITY, "section-hash")
    rom_sha, rom_bytes = _hash(rom_path) if rom_path.is_file() else ("", 0)
    if rom_sha != pointer["rom_sha256"] or rom_sha != ROM_SHA or rom_bytes != int(pointer["rom_size"]):
        raise _stop(STOP_INTEGRITY, "rom-identity")
    canonical = MasterCanonicalView(master_path)
    if canonical.rom_identity != rom_sha or canonical.generation_id != pointer["generation_id"]:
        raise _stop(STOP_INTEGRITY, "canonical-identity")
    if not canonical.parent_generation or len(canonical.parent_master_sha256) != 64:
        raise _stop(STOP_INTEGRITY, "lineage")
    emission = _validate_emission(canonical, pointer.get("source_owned_bytes"))
    outcomes = open_outcome_authority(master_path)
    provenance = outcomes.provenance
    for stage in ("STAGE_5", "STAGE_6", "STAGE_7", "STAGE_8", "STAGE_9"):
        outcomes.require_stage(stage)
    peak = max(filter(None, (peak, _rss())), default=None)
    if legacy_shadow_compare:
        if legacy_state is None:
            raise _stop(STOP_UNAVAILABLE, "legacy-shadow-inputs")
        outcomes_run = outcomes.run_id
        if legacy_state.run_id != outcomes_run:
            raise _stop(STOP_INTEGRITY, "legacy-shadow-run-id")
        from master_outcome_view import verify_projection
        verify_projection(legacy_state, master_path)
        canonical.verify_legacy_shadow(legacy_state.canonical_root)
    metrics = StartupMetrics(file_bytes, 0, int((time.perf_counter() - started) * 1000), peak)
    return MasterStartupState(MasterPointer(pointer_path.resolve(), pointer), master_path,
                              canonical, outcomes, provenance, metrics, emission)


def write_pointer(pointer_path: Path, master_path: Path, rom_path: Path) -> dict[str, Any]:
    decoded = decode_master_v2(master_path)
    canonical = MasterCanonicalView(master_path)
    emission = _validate_emission(canonical, None)
    rom_sha, rom_size = _hash(rom_path)
    file_sha, file_bytes = _hash(master_path)
    value = {"schema": POINTER_SCHEMA, "generation_id": canonical.generation_id,
             "path": os.path.relpath(master_path.resolve(), pointer_path.parent.resolve()),
             "sha256": file_sha, "bytes": file_bytes, "logical_sha256": decoded["overall_sha256"],
             "master_schema": SCHEMA, "rom_sha256": rom_sha, "rom_size": rom_size,
             "source_owned_bytes": canonical.source_owned_bytes, "emission_bytes": emission,
             "sections": {item["name"]: item["sha256"] for item in decoded["sections"]}}
    pointer_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = pointer_path.with_suffix(pointer_path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(pointer_path)
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=ROOT)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--pointer", type=Path)
    parser.add_argument("--legacy-shadow-compare", action="store_true")
    parser.add_argument("--legacy-rolling-root", type=Path)
    parser.add_argument("--legacy-canonical-root", type=Path)
    parser.add_argument("--legacy-campaign-root", type=Path)
    parser.add_argument("--run-id", type=int)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    try:
        legacy = None
        if args.legacy_shadow_compare:
            if not all((args.legacy_rolling_root, args.legacy_canonical_root,
                        args.legacy_campaign_root, args.run_id)):
                raise _stop(STOP_UNAVAILABLE, "legacy-shadow-inputs")
            legacy = discover(args.legacy_rolling_root, args.legacy_canonical_root,
                              args.legacy_campaign_root, args.run_id)
        state = load_startup(args.project_root, args.rom, args.pointer,
                             args.legacy_shadow_compare, legacy)
        result = {"status": "PASS", "startup_authority": "MASTER_V2",
                  "legacy_reads": 0, "legacy_fallback": "DISABLED",
                  "generation_id": state.generation_id, "master_sha256": state.master_sha256,
                  "rom_sha256": state.pointer.data["rom_sha256"],
                  "source_owned": state.canonical.source_owned_bytes,
                  "emission_bytes": state.emission_bytes,
                  "metrics": state.metrics.__dict__}
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True); args.report.write_text(text, encoding="utf-8")
        print(text, end="")
        return 0
    except ValueError as error:
        print(str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
