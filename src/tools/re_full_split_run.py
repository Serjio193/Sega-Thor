"""Build and verify a full-ROM split using only trusted bounded code ranges."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zlib


def run(command, cwd=None, check=True):
    result = subprocess.run([str(value) for value in command], cwd=cwd,
                            text=True, capture_output=True, check=False)
    if check and result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


def tool_path(value):
    found = shutil.which(value)
    path = Path(found or value).resolve()
    if not path.is_file():
        raise ValueError(f"Tool unavailable: {value}")
    return path


def build_entries(rom_size, routines):
    """Return a deterministic, contiguous entry list without zero-size ranges."""
    entries = []
    cursor = 0
    for routine in sorted(routines, key=lambda item: item["start"]):
        start, end = routine["start"], routine["end"]
        if start < cursor or end <= start or end > rom_size:
            raise ValueError("routine ranges overlap or exceed ROM")
        if cursor < start:
            entries.append({"start": cursor, "end": start, "kind": "UNKNOWN",
                            "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                            "emitted_artifact_type": "blob"})
        entries.append({"start": start, "end": end, "kind": "CODE_VERIFIED",
                        "source": "shared_decoder_exact_asm",
                        "confidence": routine["confidence"],
                        "emitted_artifact_type": "asm", "asm": routine["asm"]})
        cursor = end
    if cursor < rom_size:
        entries.append({"start": cursor, "end": rom_size, "kind": "UNKNOWN",
                        "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                        "emitted_artifact_type": "blob"})
    for entry in entries:
        entry["size"] = entry["end"] - entry["start"]
    for index, entry in enumerate(entries):
        entry["manifest_index"] = index
    return entries


def materialize_entries(output, entries, corpus, original):
    (output / "code").mkdir()
    (output / "blobs").mkdir()
    for index, entry in enumerate(entries):
        if entry["kind"] == "CODE_VERIFIED":
            source = corpus / entry["asm"]
            destination = output / "code" / Path(entry["asm"]).name
            shutil.copyfile(source, destination)
            entry["artifact"] = f"code/{destination.name}"
        else:
            filename = f"{entry['start']:06X}_{entry['end']:06X}.bin"
            (output / "blobs" / filename).write_bytes(original[entry["start"]:entry["end"]])
            entry["artifact"] = f"blobs/{filename}"
        entry["manifest_index"] = index


def write_layout(output, entries):
    lines = ["; Full-ROM split baseline; generated from the local canonical ROM.",
             "    org $000000"]
    for entry in entries:
        if entry["kind"] == "CODE_VERIFIED":
            body = (output / entry["artifact"]).read_text().splitlines()
            lines.extend(line for line in body
                         if not line.startswith("    org ") and not line.startswith("sub_"))
        else:
            lines.append(f"data_{entry['start']:06X}:")
            lines.append(f'    incbin "{entry["artifact"]}"')
    (output / "full_layout.asm").write_text("\n".join(lines) + "\n")


def first_difference(expected, actual, entries):
    limit = min(len(expected), len(actual))
    for offset in range(limit):
        if expected[offset] != actual[offset]:
            break
    else:
        if len(expected) == len(actual):
            return None
        offset = limit
    entry = next((item for item in entries if item["start"] <= offset < item["end"]), None)
    expected_byte = expected[offset] if offset < len(expected) else None
    actual_byte = actual[offset] if offset < len(actual) else None
    return {"rom_offset": offset, "expected": expected_byte, "actual": actual_byte,
            "manifest_entry": entry["manifest_index"] if entry else None,
            "artifact_type": entry["emitted_artifact_type"] if entry else None}


def metrics(entries, rom_size):
    totals = {"ASM_BYTES": 0, "HEADER_VECTOR_ASM_BYTES": 0,
              "STRUCTURED_DATA_ASM_BYTES": 0, "PADDING_ALIGNMENT_BYTES": 0,
              "STRUCTURED_DATA_BYTES": 0, "LOCAL_ROM_DERIVED_ASSET_BYTES": 0,
              "BLOB_BYTES": 0, "CONFLICT_BYTES": 0}
    for entry in entries:
        key = {"CODE_VERIFIED": "ASM_BYTES", "HEADER_VECTOR_ASM": "HEADER_VECTOR_ASM_BYTES",
               "STRUCTURED_DATA_CONFIRMED": "STRUCTURED_DATA_ASM_BYTES",
               "PADDING_ALIGNMENT_CONFIRMED": "PADDING_ALIGNMENT_BYTES",
               "DATA_KNOWN": "STRUCTURED_DATA_BYTES",
               "LOCAL_ROM_DERIVED_ASSET": "LOCAL_ROM_DERIVED_ASSET_BYTES",
               "UNKNOWN": "BLOB_BYTES",
               "CONFLICT": "CONFLICT_BYTES"}[entry["kind"]]
        totals[key] += entry["size"]
    return {**totals, "TOTAL_ROM_BYTES": rom_size,
            "percentages": {key: 100 * value / rom_size for key, value in totals.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, help="oasis_re_assemble executable")
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True, help="new ignored output directory")
    args = parser.parse_args()
    tool, assembler = tool_path(args.tool), tool_path(args.assembler)
    rom, output = Path(args.rom).resolve(), Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new to avoid stale artifacts")
    original = rom.read_bytes()
    output.mkdir(parents=True)
    corpus = output / "corpus"
    runner = Path(__file__).with_name("re_assemble_run.py")
    run([sys.executable, runner, "--tool", tool, "--assembler", assembler,
         "--rom", rom, "--output", corpus])
    corpus_manifest = json.loads((corpus / "manifest.json").read_text())
    if hashlib.sha256(original).hexdigest() != corpus_manifest["rom_sha256"]:
        raise ValueError("ROM changed since corpus validation")
    entries = build_entries(len(original), corpus_manifest["routines"])
    materialize_entries(output, entries, corpus, original)
    write_layout(output, entries)
    flags = ["-m68000", "-no-opt", "-Fbin"]
    assembled = run([assembler, *flags, "-o", output / "rebuilt.rom",
                     output / "full_layout.asm"], output, False)
    rebuilt = (output / "rebuilt.rom").read_bytes() if assembled.returncode == 0 else b""
    difference = first_difference(original, rebuilt, entries)
    split = json.loads((corpus / "result.json").read_text())
    manifest = {"schema": "oasis.full-rom-split.v1",
                "rom_sha256": hashlib.sha256(original).hexdigest(),
                "rom_size": len(original), "start": 0, "end": len(original),
                "entries": entries, "metrics": metrics(entries, len(original)),
                "quality": {"manifest_entries": len(entries), "gaps": 0, "overlaps": 0,
                            "verified_code_ranges": sum(e["kind"] == "CODE_VERIFIED" for e in entries),
                            "blob_ranges": sum(e["kind"] == "UNKNOWN" for e in entries),
                            "conflict_ranges": sum(e["kind"] == "CONFLICT" for e in entries),
                            "smallest_range": min(e["size"] for e in entries),
                            "largest_range": max(e["size"] for e in entries)},
                "full_match": difference is None and len(rebuilt) == len(original),
                "first_difference": difference,
                "regression": {"m11_9_controls": split["exact_matches"] >= 5,
                                "m11_10_corpus": split["exact_matches"] == 25,
                                "legacy_split": split["legacy_split_status"],
                                "expanded_split": split["split_status"]}}
    if manifest["full_match"]:
        manifest["hashes"] = {"crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                               "sha1": hashlib.sha1(rebuilt).hexdigest(),
                               "sha256": hashlib.sha256(rebuilt).hexdigest()}
    else:
        manifest["hashes"] = {}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Full ROM: {len(rebuilt)}/{len(original)} bytes; exact={'YES' if manifest['full_match'] else 'NO'}")
    if difference:
        print("FIRST_DIFFERENCE " + json.dumps(difference, sort_keys=True))
    print(f"Manifest: {len(entries)} entries; gaps=0; overlaps=0; ASM={manifest['metrics']['ASM_BYTES']} bytes")
    return 0 if assembled.returncode == 0 and manifest["full_match"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
