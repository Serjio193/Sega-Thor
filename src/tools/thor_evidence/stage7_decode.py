"""Bounded parallel MAP candidate decoding for Stage 7."""

from __future__ import annotations

from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
import hashlib
import json
from pathlib import Path
import time
from typing import Any, Callable

from stage7_subprocess import DEFAULT_TIMEOUT_SECONDS, STAGE7_TIMEOUT_CODE, run

RANGE_DECODER_CAPABILITY = "oasis.stage7.range.v1"
STOP_DECODER_CONTRACT = "STOP_STAGE7_DECODER_CONTRACT"
STOP_DECODER_TOOL_FAILURE = "STOP_STAGE7_DECODER_TOOL_FAILURE"
STOP_DECODER_OUTPUT = "STOP_STAGE7_DECODER_OUTPUT_INVALID"
STOP_DECODER_INPUT = "STOP_STAGE7_DECODER_INPUT"


class Stage7DecodeFailure(ValueError):
    """A deterministic, fail-closed Stage 7 decoder failure."""

    def __init__(self, code: str, detail: str = "") -> None:
        self.code = code
        super().__init__(code + ((":" + detail) if detail else ""))


def validate_range_decoder(range_tool: Path) -> dict[str, str]:
    """Verify the executable role before any candidate decode is scheduled."""
    if not range_tool.is_file():
        raise Stage7DecodeFailure(STOP_DECODER_CONTRACT, "range_tool_missing")
    completed = run([range_tool, "--capabilities"],
                    timeout_seconds=DEFAULT_TIMEOUT_SECONDS,
                    timeout_code=STAGE7_TIMEOUT_CODE,
                    label="Stage 7 decoder capability probe")
    marker = completed.stdout.strip()
    if completed.returncode != 0 or marker != RANGE_DECODER_CAPABILITY:
        detail = f"expected={RANGE_DECODER_CAPABILITY};actual={marker or completed.stderr.strip()[:200]}"
        raise Stage7DecodeFailure(STOP_DECODER_CONTRACT, detail)
    return {"tool": str(range_tool.resolve()), "capability": marker}


def decode_candidate(range_tool: Path, rom: Path, candidate: dict[str, Any],
                     output: Path) -> tuple[dict[str, Any], Path]:
    start, end = candidate["intervals"][0]
    asm, decoded = output / "candidate.asm", output / "decode.json"
    completed = run([range_tool, rom, hex(start), hex(end), asm, decoded],
                    timeout_seconds=DEFAULT_TIMEOUT_SECONDS,
                    timeout_code=STAGE7_TIMEOUT_CODE,
                    label=f"Stage 7 decode 0x{start:06X}-0x{end:06X}")
    if completed.returncode:
        detail = (completed.stderr or completed.stdout).strip()[-500:]
        if completed.returncode == 2 or "usage:" in detail.lower():
            raise Stage7DecodeFailure(STOP_DECODER_CONTRACT, detail)
        if "boundary_uncertain" in detail.lower():
            raise Stage7DecodeFailure("STOP_BOUNDARY_UNPROVEN", detail)
        if "unsupported_form" in detail.lower():
            raise Stage7DecodeFailure("STOP_UNSUPPORTED_M68K_DECODE", detail)
        if "invalid even bounded range" in detail.lower() or \
                "invalid bounded m68k slice options" in detail.lower():
            raise Stage7DecodeFailure(STOP_DECODER_INPUT, detail)
        raise Stage7DecodeFailure(STOP_DECODER_TOOL_FAILURE, detail)
    if not asm.is_file() or not decoded.is_file():
        raise Stage7DecodeFailure(STOP_DECODER_OUTPUT,
                                  "missing_asm_or_json_output")
    try:
        value = json.loads(decoded.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise Stage7DecodeFailure(STOP_DECODER_OUTPUT, "malformed_json") from error
    try:
        metadata_matches = isinstance(value, dict) and \
            value.get("source_decoder") == "re_slice_decoder" and \
            int(value.get("start", -1)) == start and int(value.get("end", -1)) == end
    except (TypeError, ValueError):
        metadata_matches = False
    if not metadata_matches:
        raise Stage7DecodeFailure(STOP_DECODER_OUTPUT, "range_metadata_mismatch")
    try:
        unsupported = {int(item) for item in value.get("unsupported_instruction_addresses", [])}
        instructions = value.get("instructions", [])
        if not isinstance(instructions, list):
            raise ValueError("instructions is not a list")
        for instruction in instructions:
            if not isinstance(instruction, dict):
                raise ValueError("instruction is not an object")
            address = int(instruction["address"])
            operation = str(instruction.get("operation", "")).lower()
            instruction["address"] = address
            instruction["opcode"] = int(instruction["opcode"])
            instruction["supported"] = address not in unsupported
            target = instruction.get("branch_target")
            if operation in {"rts", "rtr", "rte"}:
                flow = "return"
            elif operation == "jsr":
                flow = "direct_call" if target is not None else "indirect_call"
            elif operation == "jmp":
                flow = "direct_jump" if target is not None else "indirect_jump"
            elif operation.startswith("b"):
                flow = "direct_branch"
            else:
                flow = "none"
            instruction["flow"] = flow
    except (TypeError, ValueError, KeyError) as error:
        raise Stage7DecodeFailure(STOP_DECODER_OUTPUT, "malformed_decode_metadata") from error
    value["source_decoder"] = "re_slice_decoder"
    value["start"], value["end"] = start, end
    return value, asm


def _file_identity(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def decode_request_key(range_tool: Path, rom: Path,
                       candidate: dict[str, Any]) -> tuple[str, ...]:
    """Return every immutable input that can affect the external decode."""
    start, end = (int(value) for value in candidate["intervals"][0])
    tool_id = _file_identity(range_tool) if range_tool.is_file() else str(range_tool.resolve())
    rom_id = _file_identity(rom) if rom.is_file() else str(rom.resolve())
    rom_size = str(rom.stat().st_size) if rom.is_file() else "missing"
    return ("m68k-range-v1", rom_id, rom_size, tool_id, f"{start:08X}",
            f"{end:08X}", str(end - start), "instruction_budget=default")


def _decode_timed(range_tool: Path, rom: Path, candidate: dict[str, Any],
                  output: Path) -> tuple[dict[str, Any] | None, Path | None,
                                          float, BaseException | None]:
    started = time.monotonic()
    try:
        decoded, asm = decode_candidate(range_tool, rom, candidate, output)
        return decoded, asm, time.monotonic() - started, None
    except BaseException as error:
        return None, None, time.monotonic() - started, error


def decode_candidates(jobs: list[tuple[int, Path, dict[str, Any]]],
                      range_tool: Path, rom: Path,
                      heartbeat: Callable[[int, int], None] | None = None,
                      max_workers: int = 4,
                      metrics: dict[str, Any] | None = None) -> dict[int, tuple[dict[str, Any] | None,
                                                                                Path | None, BaseException | None]]:
    """Decode independent candidates concurrently and return results by stable ordinal."""
    if not jobs:
        return {}
    results: dict[int, tuple[dict[str, Any] | None, Path | None, BaseException | None]] = {}
    key_started = time.monotonic()
    keys = [decode_request_key(range_tool, rom, candidate) for _, _, candidate in jobs]
    key_elapsed = time.monotonic() - key_started
    frequencies: dict[tuple[str, ...], int] = {}
    for key in keys:
        frequencies[key] = frequencies.get(key, 0) + 1
    worker_count = max(1, min(max_workers, len(jobs)))
    batch_started = time.monotonic()
    executor = ThreadPoolExecutor(max_workers=worker_count,
                                  thread_name_prefix="stage7-decode")
    futures = {executor.submit(_decode_timed, range_tool, rom, candidate, output): number
               for number, output, candidate in jobs}
    key_by_number = {number: key for (number, _, _), key in zip(jobs, keys)}
    pending = set(futures)
    completed = 0
    external_wall = 0.0
    duplicate_wall = 0.0
    coordinator_wait = 0.0
    try:
        while pending:
            done, pending = wait(pending, timeout=0.25, return_when=FIRST_COMPLETED)
            for future in done:
                number = futures[future]
                result_started = time.monotonic()
                decoded, asm, elapsed, error = future.result()
                coordinator_wait += time.monotonic() - result_started
                external_wall += elapsed
                if frequencies[key_by_number[number]] > 1:
                    duplicate_wall += elapsed
                results[number] = (decoded, asm, error)
                completed += 1
            if heartbeat:
                heartbeat(completed, len(jobs))
    finally:
        executor.shutdown(wait=True, cancel_futures=False)
    if metrics is not None:
        repeated = {key: count for key, count in frequencies.items() if count > 1}
        key_digests = [hashlib.sha256("\x1f".join(key).encode()).hexdigest()
                       for key in keys]
        metrics.clear()
        metrics.update({
            "decode_request_total": len(jobs),
            "decode_unique_keys": len(frequencies),
            "decode_duplicate_requests": len(jobs) - len(frequencies),
            "decode_duplicate_rate": round((len(jobs) - len(frequencies)) / len(jobs), 6),
            "external_decoder_invocations": len(jobs),
            "unique_decoder_invocations": len(frequencies),
            "avoidable_invocations": len(jobs) - len(frequencies),
            "decode_cache_hits": 0,
            "inflight_coalesced_requests": 0,
            "decoder_invocation_reduction": round(
                (len(jobs) - len(frequencies)) / len(jobs), 6),
            "repeated_request_frequency_distribution": {
                str(count): sum(value == count for value in frequencies.values())
                for count in sorted(set(frequencies.values())) if count > 1},
            "top_repeated_keys": [
                {"key": hashlib.sha256("\x1f".join(key).encode()).hexdigest(),
                 "frequency": count} for key, count in
                sorted(repeated.items(), key=lambda item: (-item[1], item[0]))[:10]],
            "duplicate_request_bytes": sum(
                int(job[2]["intervals"][0][1]) - int(job[2]["intervals"][0][0])
                for job, key in zip(jobs, keys) if frequencies[key] > 1),
            "external_decode_wall_seconds": round(external_wall, 6),
            "batch_wall_seconds": round(time.monotonic() - batch_started, 6),
            "duplicate_decode_wall_seconds": round(duplicate_wall, 6),
            "pool_workers": worker_count,
            "pool_peak_occupancy": worker_count,
            "coordinator_wait_seconds": round(coordinator_wait, 6),
            "key_construction_seconds": round(key_elapsed, 6),
            "cache_peak_entries": 0,
            "cache_peak_bytes": 0,
            "inflight_peak_entries": 0,
            "key_schema": "m68k-range-v1:rom+tool+start+end+decode-config",
            "_key_digests": key_digests,
        })
    return results


def summarize_decode_metrics(batches: list[dict[str, Any]]) -> dict[str, Any]:
    """Merge per-batch telemetry while treating repeated keys across batches as duplicates."""
    digests = {digest for batch in batches for digest in batch.get("_key_digests", [])}
    total = sum(int(batch.get("decode_request_total", 0)) for batch in batches)
    duplicate = total - len(digests)
    return {
        "decode_request_total": total,
        "decode_unique_keys": len(digests),
        "decode_duplicate_requests": duplicate,
        "decode_duplicate_rate": round(duplicate / total, 6) if total else 0.0,
        "external_decoder_invocations": sum(int(batch.get("external_decoder_invocations", 0))
                                             for batch in batches),
        "unique_decoder_invocations": len(digests),
        "avoidable_invocations": duplicate,
        "decode_cache_hits": 0,
        "inflight_coalesced_requests": 0,
        "decoder_invocation_reduction": round(duplicate / total, 6) if total else 0.0,
        "duplicate_request_bytes": sum(int(batch.get("duplicate_request_bytes", 0))
                                        for batch in batches),
        "duplicate_decode_wall_seconds": round(sum(
            float(batch.get("duplicate_decode_wall_seconds", 0.0)) for batch in batches), 6),
        "external_decode_wall_seconds": round(sum(
            float(batch.get("external_decode_wall_seconds", 0.0)) for batch in batches), 6),
        "batch_wall_seconds": round(sum(float(batch.get("batch_wall_seconds", 0.0))
                                        for batch in batches), 6),
        "pool_workers": max((int(batch.get("pool_workers", 0)) for batch in batches), default=0),
        "pool_peak_occupancy": max((int(batch.get("pool_peak_occupancy", 0))
                                     for batch in batches), default=0),
        "coordinator_wait_seconds": round(sum(float(batch.get("coordinator_wait_seconds", 0.0))
                                              for batch in batches), 6),
        "key_construction_seconds": round(sum(float(batch.get("key_construction_seconds", 0.0))
                                               for batch in batches), 6),
        "cache_peak_entries": 0,
        "cache_peak_bytes": 0,
        "inflight_peak_entries": 0,
        "key_schema": "m68k-range-v1:rom+tool+start+end+decode-config",
    }


__all__ = ["decode_candidate", "decode_candidates", "decode_request_key",
           "summarize_decode_metrics"]
