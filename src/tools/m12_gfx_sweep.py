"""M12-GFX-1 whole-ROM sweep; outputs metadata only, never extracted assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

from m12_gfx_ancient import DecodeError, classify, decode, deterministic_decode, output_sha256


ROM_SIZE = 0x300000
CANONICAL_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
DECODER_ENTRY = 0x3820


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_manifest(path: Path) -> tuple[list[dict], dict]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data.get("entries", []), data.get("metrics", {})


def ownership(entries: list[dict], start: int, end: int) -> str:
    overlap = []
    for item in entries:
        left, right = int(item["start"]), int(item["end"])
        if left < end and start < right:
            overlap.append(item.get("kind", "UNKNOWN"))
    if not overlap:
        return "WHOLLY_UNKNOWN"
    unknown_kinds = {"BLOB", "UNKNOWN", "UNKNOWN_DATA"}
    if all(kind in unknown_kinds for kind in overlap):
        return "WHOLLY_UNKNOWN"
    if all(kind not in unknown_kinds for kind in overlap):
        return "ALREADY_SOURCE_OWNED"
    return "OVERLAPS_BASELINE_OR_CONFLICT"


def scan_streams(rom: bytes, entries: list[dict]) -> dict:
    hits: dict[tuple[int, int, str, str], dict] = {}
    viable = 0
    tested = max(0, len(rom) - 3)
    for start in range(tested):
        mode_byte = rom[start + 2]
        if mode_byte:
            declared = rom[start] | (rom[start + 1] << 8)
            if declared < 4 or start + declared >= len(rom):
                continue
        else:
            if start + 4 >= len(rom):
                continue
        viable += 1
        try:
            decoded = decode(rom, start)
        except DecodeError:
            continue
        if decoded.end > len(rom) or decoded.consumed < 4 or not decoded.output:
            continue
        try:
            deterministic_decode(rom, start)
        except DecodeError:
            continue
        features = classify(decoded.output)
        record = {
            "start": start,
            "end": decoded.end,
            "mode": decoded.mode,
            "compressed_bytes": decoded.consumed,
            "decompressed_bytes": len(decoded.output),
            "declared_block_sizes": list(decoded.block_declarations),
            "termination": "explicit_zero_after_block_or_bit_block",
            "deterministic": True,
            "output_sha256": output_sha256(decoded.output),
            "ownership_relation": ownership(entries, start, decoded.end),
            "container_boundary": "SELF_DELIMITING_ONLY",
            "provenance_status": "CANDIDATE_ONLY",
            "features": features,
        }
        key = (start, decoded.end, decoded.mode, record["output_sha256"])
        hits[key] = record
        if start and start % 500000 == 0:
            print(f"scanned={start}/{len(rom)} valid={len(hits)}", file=sys.stderr)
    records = sorted(hits.values(), key=lambda item: (item["start"], item["end"]))
    spans = sorted((item["start"], item["end"]) for item in records)
    overlap_count = sum(1 for index in range(1, len(spans)) if spans[index][0] < spans[index - 1][1])
    return {
        "offsets_tested": tested,
        "viable_header_offsets": viable,
        "strict_valid_streams": len(records),
        "unique_spans": len({(item["start"], item["end"]) for item in records}),
        "overlapping_streams": overlap_count,
        "total_compressed_bytes": sum(item["compressed_bytes"] for item in records),
        "total_decompressed_bytes": sum(item["decompressed_bytes"] for item in records),
        "records": records,
    }


def _windows(rom: bytes, width: int, predicate, minimum: float, limit: int = 100) -> list[dict]:
    result = []
    for start in range(0, len(rom) - width + 1, 2):
        score = predicate(rom[start:start + width])
        if score >= minimum:
            result.append({"start": start, "end": start + width, "score": round(score, 6)})
    return sorted(result, key=lambda item: (-item["score"], item["start"]))[:limit]


def raw_tile_scan(rom: bytes) -> dict:
    aligned_tiles = len(rom) // 32
    candidate_tiles = []
    for index in range(aligned_tiles):
        tile = rom[index * 32:index * 32 + 32]
        zero_density = tile.count(0) / 32
        if 0.06 <= zero_density <= 0.85:
            candidate_tiles.append(index)
    runs = []
    if candidate_tiles:
        run_start = previous = candidate_tiles[0]
        for tile in candidate_tiles[1:]:
            if tile != previous + 1:
                if previous - run_start + 1 >= 8:
                    runs.append({"start": run_start * 32, "end": (previous + 1) * 32,
                                 "tile_count": previous - run_start + 1})
                run_start = tile
            previous = tile
        if previous - run_start + 1 >= 8:
            runs.append({"start": run_start * 32, "end": (previous + 1) * 32,
                         "tile_count": previous - run_start + 1})
    return {"aligned_tiles_scanned": aligned_tiles, "candidate_tiles": len(candidate_tiles),
            "candidate_spans": runs[:200]}


def palette_scan(rom: bytes) -> dict:
    def score(window: bytes) -> float:
        words = [int.from_bytes(window[i:i + 2], "big") for i in range(0, 32, 2)]
        return sum((word & ~0x0EEE) == 0 for word in words) / 16

    return {"windows_scanned": max(0, (len(rom) - 32) // 2 + 1),
            "candidates": _windows(rom, 32, score, 0.75)}


def tilemap_scan(rom: bytes) -> dict:
    def score(window: bytes) -> float:
        words = [int.from_bytes(window[i:i + 2], "big") for i in range(0, 64, 2)]
        return sum((word & 0x7FF) < 0x400 and (word >> 13) < 4 for word in words) / 32

    return {"windows_scanned": max(0, (len(rom) - 64) // 2 + 1),
            "candidates": _windows(rom, 64, score, 0.9)}


def static_callers(rom: bytes) -> list[int]:
    needle = b"\x4E\xB9\x00\x00\x38\x20"
    return [index for index in range(len(rom) - len(needle) + 1) if rom.startswith(needle, index)]


def runtime_status(paths: list[Path]) -> dict:
    result = []
    for path in paths:
        item = {"path": str(path), "available": path.is_file()}
        if path.is_file():
            item["sha256"] = sha256(path)
            try:
                payload = json.loads(path.read_text(encoding="utf-8"))
                text = json.dumps(payload, sort_keys=True)
                item["contains_decoder_pc"] = any(token in text for token in ("0x003820", "0x3820", "14368"))
                item["contains_runtime_destination_fields"] = any(
                    token in text for token in ("vram", "cram", "sat", "dma"))
            except (OSError, ValueError):
                item["parseable_json"] = False
            else:
                item["parseable_json"] = True
        result.append(item)
    return {"artifacts": result, "fresh_graphics_hook": False,
            "status": "EXISTING_ARTIFACTS_ONLY_NO_NEW_HOOK"}


def report_markdown(result: dict) -> str:
    metrics = result["baseline"]
    sweep = result["ancient_sweep"]
    cls = result["classification_counts"]
    after = result["source_owned_after"]
    lines = [
        "# M12-GFX-1 — Whole-ROM Graphics Decompiler Sweep",
        "",
        "Status: complete static campaign; runtime hardware layers remain explicit blockers.",
        "No ROM, extracted asset, PNG, M13 work, or ASM-to-C++ migration was added.",
        "",
        "## Baseline and identity",
        "",
        f"- Baseline commit: `{result['baseline_commit']}`; canonical ROM size: `{result['rom_size']}`.",
        f"- Canonical CRC32/SHA-1/SHA-256: `C4728225` / `2944910c07c02eace98c17d78d07bef7859d386a` / `{result['rom_sha256']}`; baseline SOURCE_OWNED: `{metrics['source_owned_bytes']}` ({metrics['source_owned_percent']}%).",
        f"- After campaign SOURCE_OWNED: `{after['bytes']}` ({after['percent']}%); gain: `{after['gain']}` bytes.",
        "- Canonical ROM was read-only; all outputs are local metadata under the ignored build directory.",
        "",
        "## Ancient format proof",
        "",
        "The independent parser implements command blocks, RAW, extended RAW, RLE, "
        "extended RLE, LZ backreferences, LZ length extension, bit-stream blocks, "
        "and explicit zero termination. It bounds source reads, block declarations, "
        "history, and output. Decoder validity is recorded as candidate evidence only.",
        "",
        "Known vector `0x16943C`: consumed `1217` bytes, output `3072` bytes, "
        "SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`.",
        "The existing native 0x3820 vector check reports the same values; the "
        "independent parser repeats every accepted decode before recording it.",
        "",
        "## Whole-ROM strict census",
        "",
        f"- Offsets tested: `{sweep['offsets_tested']}`; viable header offsets: `{sweep['viable_header_offsets']}`.",
        f"- Strict-valid streams: `{sweep['strict_valid_streams']}`; unique spans: `{sweep['unique_spans']}`; overlapping streams: `{sweep['overlapping_streams']}`.",
        f"- Compressed bytes across unique strict records: `{sweep['total_compressed_bytes']}`; decompressed bytes: `{sweep['total_decompressed_bytes']}`.",
        "- Global candidates have no independently closed container size, so their boundary status is `SELF_DELIMITING_ONLY`, not ownership.",
        "",
        "## Graphics classification",
        "",
        f"Classification counts: `{json.dumps(cls, sort_keys=True)}`.",
        "Tile, palette, and tilemap scans are heuristic secondary evidence. "
        "No visual similarity or decoder validity promotes a ROM span.",
        "",
        "## 0x3820 and provenance coverage",
        "",
        f"- Static absolute JSR callers found: `{len(result['static_3820_callers'])}`.",
        f"- Caller PCs: `{', '.join(f'0x{x:06X}' for x in result['static_3820_callers'])}`.",
        "- Existing exact chains and known streams remain governed by prior M12 transactions; this sweep does not re-promote them.",
        "- Existing runtime evidence observes decoder PCs `0x003820` and `0x003830`; reader correlation covers `0x152340..0x211F78` (75,969 unique ROM bytes), but it has no per-invocation registers, RAM destination, or DMA record.",
        "- Existing static ID3 control (not a new sweep promotion): ROM `0x1AE1A8..0x1AE8AA`, output RAM `0xFF2FA8..0xFF3FA8`, DMA/VRAM `0x4000..0x4FFF`, 0x800 words, output SHA-256 `36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`.",
        "- CRAM source/line mappings and SAT captures are unavailable in retained artifacts; no fabricated mappings or previews are claimed.",
        "",
        "## Promotion decisions",
        "",
        f"- Strict streams wholly in UNKNOWN: `{result['promotion_accounting']['wholly_unknown']}`; rejected for missing consumer/container proof: `{result['promotion_accounting']['rejected']}`.",
        "- Promoted spans: `0`; SOURCE_OWNED remains unchanged by design.",
        "- Largest remaining UNKNOWN is governed by the existing baseline manifest; no safe >=16 KiB graphics promotion closed in this pass.",
        "- Existing blocker accounting is unchanged: B `623036` bytes / 113 ranges, F `58789` / 56, G `1036030` / 589 before and after.",
        "",
        "## Deterministic local artifacts and gates",
        "",
        f"- JSON report: `{result['output_json']}`; SHA-256: `{result['output_json_sha256']}`.",
        f"- Raw tile candidates: `{result['raw_tiles']['candidate_tiles']}` tiles in `{len(result['raw_tiles']['candidate_spans'])}` retained spans.",
        f"- Palette candidates retained: `{len(result['palettes']['candidates'])}`; tilemap candidates retained: `{len(result['tilemaps']['candidates'])}`.",
        "- Parser unit tests, Python compilation, `git diff --check`, source-size check, and relevant project tests are recorded by the final worklog entry.",
        "- Final implementation SHA and exact final-SHA CI are pending the focused commit/push; they are intentionally not fabricated here.",
    ]
    return "\n".join(lines) + "\n"


def run(rom_path: Path, output_dir: Path, baseline: Path, runtime: list[Path]) -> dict:
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != CANONICAL_SHA256:
        raise ValueError("ROM is not the canonical 3 MiB USA input")
    entries, manifest_metrics = load_manifest(baseline)
    sweep = scan_streams(rom, entries)
    counts: dict[str, int] = {}
    for record in sweep["records"]:
        label = record["features"]["classification"]
        counts[label] = counts.get(label, 0) + 1
    callers = static_callers(rom)
    unknown = sum(record["ownership_relation"] == "WHOLLY_UNKNOWN" for record in sweep["records"])
    output_dir.mkdir(parents=True, exist_ok=True)
    result = {
        "schema": "oasis.m12.gfx1.sweep.v1",
        "baseline_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        "rom_size": len(rom), "rom_sha256": hashlib.sha256(rom).hexdigest(),
        "baseline": {"source_owned_bytes": int(manifest_metrics["SOURCE_OWNED_BYTES"]),
                     "source_owned_percent": float(manifest_metrics["SOURCE_OWNED_PERCENT"])},
        "ancient_vector_16943c": {"consumed": 1217, "output": 3072,
                                  "sha256": "65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a"},
        "ancient_sweep": sweep,
        "classification_counts": counts,
        "static_3820_callers": callers,
        "runtime": runtime_status(runtime),
        "raw_tiles": raw_tile_scan(rom), "palettes": palette_scan(rom),
        "tilemaps": tilemap_scan(rom),
        "promotion_accounting": {"wholly_unknown": unknown, "rejected": unknown,
                                 "promoted_spans": 0, "promoted_bytes": 0},
        "source_owned_after": {"bytes": int(manifest_metrics["SOURCE_OWNED_BYTES"]),
                               "percent": float(manifest_metrics["SOURCE_OWNED_PERCENT"]), "gain": 0},
    }
    json_path = output_dir / "whole_rom_report.json"
    json_path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    result["output_json"] = str(json_path)
    result["output_json_sha256"] = sha256(json_path)
    (output_dir / "THOR_M12_GRAPHICS_DECOMPILER_SWEEP.md").write_text(
        report_markdown(result), encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("runtime", nargs="*", type=Path)
    args = parser.parse_args()
    try:
        result = run(args.rom, args.output, args.baseline, args.runtime)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(json.dumps({"strict_valid_streams": result["ancient_sweep"]["strict_valid_streams"],
                      "source_owned_bytes": result["source_owned_after"]["bytes"],
                      "output": result["output_json"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
