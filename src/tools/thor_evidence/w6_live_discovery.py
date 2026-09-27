"""W6 Long-Run Live Game Discovery Session Runner and Chunk Manager.

Executes a sustained >=10 minute discovery session with 128 Workers at depth 512
using canonical Beyond Oasis ROM and verified in-game gameplay savestate.
Preserves non-truncated chunked raw evidence files and tracks live health metrics.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from typing import Any, Callable

# Add repo root to import path
REPO_ROOT = Path(__file__).resolve().parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    RECORD_STRUCT,
    W3Record,
    iter_records,
    unpack_record,
)

CANONICAL_ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CANONICAL_ROM_SIZE = 3145728
CANONICAL_STATE_SHA = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
CANONICAL_WORKERS = 128
CANONICAL_DEPTH = 512
CANONICAL_MEMORY_BYTES = 524288
CANONICAL_RING_CAPACITY = 2097152
CANONICAL_SOURCE_OWNED = 1487388


def verify_rom(rom_path: Path) -> None:
    """Verifies the canonical ROM size and hash."""
    if not rom_path.is_file():
        raise FileNotFoundError(f"Canonical ROM not found: {rom_path}")
    data = rom_path.read_bytes()
    if len(data) != CANONICAL_ROM_SIZE:
        raise ValueError(f"Canonical ROM size mismatch: {len(data)} != {CANONICAL_ROM_SIZE}")
    actual_sha = hashlib.sha256(data).hexdigest()
    if actual_sha != CANONICAL_ROM_SHA:
        raise ValueError(f"Canonical ROM SHA mismatch: {actual_sha} != {CANONICAL_ROM_SHA}")


def verify_savestate(state_path: Path) -> None:
    """Verifies the QuickSave1 savestate hash."""
    if not state_path.is_file():
        raise FileNotFoundError(f"Savestate not found: {state_path}")
    data = state_path.read_bytes()
    actual_sha = hashlib.sha256(data).hexdigest()
    if actual_sha != CANONICAL_STATE_SHA:
        raise ValueError(f"Savestate SHA mismatch: {actual_sha} != {CANONICAL_STATE_SHA}")


def hash_file(path: Path) -> str:
    """Computes SHA-256 digest of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)
    return h.hexdigest()


def scan_raw_chunk_metadata(chunk_path: Path) -> dict[str, Any]:
    """Scans a W3 V2 48-byte record binary file and extracts bounded metadata."""
    size = chunk_path.stat().st_size
    record_count = size // RECORD_STRUCT.size
    first_record: W3Record | None = None
    last_record: W3Record | None = None

    if record_count > 0:
        with chunk_path.open("rb") as f:
            chunk = f.read(RECORD_STRUCT.size)
            if len(chunk) == RECORD_STRUCT.size:
                first_record = unpack_record(chunk)
            if record_count > 1:
                f.seek((record_count - 1) * RECORD_STRUCT.size)
                last_chunk = f.read(RECORD_STRUCT.size)
                if len(last_chunk) == RECORD_STRUCT.size:
                    last_record = unpack_record(last_chunk)
            else:
                last_record = first_record

    sha256 = hash_file(chunk_path)
    return {
        "chunk_file": chunk_path.name,
        "byte_size": size,
        "record_count": record_count,
        "sha256": sha256,
        "stream_sequence_start": first_record.stream_sequence if first_record else None,
        "stream_sequence_end": last_record.stream_sequence if last_record else None,
        "master_time_start": first_record.master_time if first_record else None,
        "master_time_end": last_record.master_time if last_record else None,
    }


def split_unified_records_into_chunks(
    records_path: Path,
    chunk_dir: Path,
    records_per_chunk: int = 128 * 512,
    chunk_prefix: str = "live-discovery-wave",
) -> list[dict[str, Any]]:
    """Splits a large unified record file into deterministic wave chunks if needed."""
    chunk_dir.mkdir(parents=True, exist_ok=True)
    size = records_path.stat().st_size
    total_records = size // RECORD_STRUCT.size
    chunks_meta: list[dict[str, Any]] = []

    with records_path.open("rb") as src:
        wave_idx = 1
        record_idx = 0
        while record_idx < total_records:
            take = min(records_per_chunk, total_records - record_idx)
            chunk_file = chunk_dir / f"{chunk_prefix}-{wave_idx:06d}.bin"
            data = src.read(take * RECORD_STRUCT.size)
            chunk_file.write_bytes(data)
            meta = scan_raw_chunk_metadata(chunk_file)
            meta["wave_index"] = wave_idx
            chunks_meta.append(meta)
            record_idx += take
            wave_idx += 1

    return chunks_meta


def index_discovery_chunks(
    directory: Path,
    chunk_prefix: str = "live-discovery-wave",
    fallback_unified_file: Path | None = None,
) -> list[dict[str, Any]]:
    """Finds all wave chunk files or generates them from unified records."""
    chunk_files = sorted(directory.glob(f"{chunk_prefix}-*.bin"))
    if not chunk_files and fallback_unified_file and fallback_unified_file.is_file():
        return split_unified_records_into_chunks(
            fallback_unified_file, directory, chunk_prefix=chunk_prefix
        )

    results: list[dict[str, Any]] = []
    for idx, cfile in enumerate(chunk_files, start=1):
        meta = scan_raw_chunk_metadata(cfile)
        meta["wave_index"] = idx
        results.append(meta)
    return results


def run_w6_discovery_session(
    rom_path: Path,
    install_path: Path,
    output_dir: Path,
    state_path: Path | None = None,
    duration_seconds: float = 610.0,
    cadence_frames: int = 300,
    depth: int = CANONICAL_DEPTH,
    worker_count: int = CANONICAL_WORKERS,
    memory_bytes: int = CANONICAL_MEMORY_BYTES,
) -> dict[str, Any]:
    """Runs BizHawk sustained 10-minute discovery session with 128 workers and depth 512."""
    verify_rom(rom_path)
    if state_path:
        verify_savestate(state_path)

    # Prepare directories
    natural_dir = output_dir / "natural" / f"count-{worker_count}"
    natural_dir.mkdir(parents=True, exist_ok=True)
    end_game_path = natural_dir / "end-game-request.txt"
    end_game_path.unlink(missing_ok=True)

    # Set up environment variables
    script_path = REPO_ROOT / "tools" / "bizhawk-native-ring" / "live_forward_scaling.lua"
    scaling_py = REPO_ROOT / "tools" / "bizhawk-native-ring" / "live_forward_scaling_runtime.py"

    # Add tools/bizhawk-native-ring to path
    ring_tools_dir = REPO_ROOT / "tools" / "bizhawk-native-ring"
    if str(ring_tools_dir) not in sys.path:
        sys.path.insert(0, str(ring_tools_dir))

    from live_forward_scaling_runtime import resolve_runtime_paths, run_one

    args = argparse.Namespace(
        install=install_path.resolve(),
        rom=rom_path.resolve(),
        script=script_path.resolve(),
        output_dir=output_dir.resolve(),
        count=worker_count,
        depth=depth,
        memory_bytes=memory_bytes,
        rounds=0,
        until_closed=True,
        max_frames=1800,
        duration=duration_seconds,
        timeout=int(duration_seconds + 300),
        native_budget_bytes=256 * 1024 * 1024,
        process_budget_bytes=512 * 1024 * 1024,
        core_reserve_bytes=128 * 1024 * 1024,
        system_reserve_bytes=4 * 1024 * 1024 * 1024,
        end_game_path=end_game_path,
        sample_interval=5.0,
    )

    # Pass discovery specific environment flags
    if state_path:
        os.environ["LF_SAVESTATE"] = str(state_path.resolve())
    else:
        os.environ.pop("LF_SAVESTATE", None)
    os.environ["LF_WAVE_CADENCE_FRAMES"] = str(cadence_frames)
    os.environ["LF_CHUNK_PREFIX"] = "live-discovery-wave"
    os.environ["LF_NATURAL_INPUT"] = "1"
    os.environ["LF_END_GAME_PATH"] = str(end_game_path.resolve())

    state = {"end_game_triggered": False}

    def on_status(values: dict[str, str], process: Any, started: float,
                  audit_count: int, workers_summary: Any) -> None:
        now = time.monotonic()
        elapsed = now - started
        if elapsed >= duration_seconds and not state["end_game_triggered"]:
            state["end_game_triggered"] = True
            end_game_path.write_text("END_GAME\n", encoding="ascii")
            print(f"[W6-DISCOVERY] Elapsed {elapsed:.1f}s >= {duration_seconds:.1f}s. Triggered clean END_GAME.")

    print(f"Launching W6 Live Discovery Run ({worker_count} Workers, depth {depth}, cadence {cadence_frames} frames)...")
    summary = run_one(args, "natural", worker_count, depth, on_status=on_status)
    return summary


def main() -> int:
    """CLI entrypoint for W6 Live Discovery runner and analyzer."""
    parser = argparse.ArgumentParser(description="W6 Live Discovery Runner and Analyzer")
    parser.add_argument("--rom", type=Path, default=Path("local-roms/Beyond Oasis (USA).md"))
    parser.add_argument("--install", type=Path,
                        default=Path(r"C:\Dev\SegaThorTools\BizHawk-m12-w2-1-frame-coherent-20260920"))
    parser.add_argument("--output-dir", type=Path, default=Path("build/m12-w6-live-discovery"))
    parser.add_argument("--duration", type=float, default=610.0)
    parser.add_argument("--cadence", type=int, default=300)
    parser.add_argument("--workers", type=int, default=CANONICAL_WORKERS)
    parser.add_argument("--depth", type=int, default=CANONICAL_DEPTH)
    parser.add_argument("--memory-bytes", type=int, default=CANONICAL_MEMORY_BYTES)
    parser.add_argument("--analyze-only", action="store_true")
    args = parser.parse_args()

    out_dir = args.output_dir.resolve()
    natural_dir = out_dir / "natural" / f"count-{args.workers}"

    if not args.analyze_only:
        summary = run_w6_discovery_session(
            rom_path=args.rom,
            install_path=args.install,
            output_dir=out_dir,
            duration_seconds=args.duration,
            cadence_frames=args.cadence,
            depth=args.depth,
            worker_count=args.workers,
            memory_bytes=args.memory_bytes,
        )
        print(f"Session finished with outcome: {summary.get('outcome')}")

    receipt_path = natural_dir / "receipt.json"
    if not receipt_path.is_file():
        raise FileNotFoundError(f"Run receipt not found: {receipt_path}")
    run_receipt = json.loads(receipt_path.read_text(encoding="utf-8"))

    print("Indexing discovery chunks...")
    chunks_meta = index_discovery_chunks(natural_dir, chunk_prefix="live-discovery-wave")
    chunk_paths = [natural_dir / m["chunk_file"] for m in chunks_meta]
    print(f"Found {len(chunk_paths)} wave chunk files.")

    from src.tools.thor_evidence.w6_discovery_analysis import (
        analyze_discovery_records,
        generate_all_w6_artifacts,
    )
    print("Analyzing discovery records...")
    analysis = analyze_discovery_records(chunk_paths, run_receipt, out_dir)
    print("Generating all 13 W6 artifacts...")
    stage_receipt = generate_all_w6_artifacts(run_receipt, chunks_meta, analysis, out_dir)
    print(f"PASS_LONG_LIVE_GAME_DISCOVERY_RUN_V1 complete. Hash: {stage_receipt.get('receipt_sha256')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
