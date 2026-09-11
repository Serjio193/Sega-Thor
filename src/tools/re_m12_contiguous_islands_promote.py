"""Promote exact direct-caller-to-RTS static islands from a bounded audit."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SLICES = (
    (0x0004C6, 0x0005F8), (0x0031AC, 0x0031FC), (0x003464, 0x0034BE),
    (0x003CA0, 0x003CD2), (0x00400C, 0x004026), (0x004538, 0x00455E),
    (0x004800, 0x00482A), (0x004A1A, 0x004A88), (0x006718, 0x006738),
    (0x007FCC, 0x00803C), (0x010440, 0x010464), (0x01A1B6, 0x01A1D6),
    (0x01A4CA, 0x01A52E), (0x01B580, 0x01B5CA), (0x0629EE, 0x062A0C),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "CODE_VERIFIED",
                "source": "M12_AUTO60_contiguous_island_audit",
                "confidence": "CONFIRMED",
                "classification": "STATIC_DIRECT_CALLER_RTS_ISLAND",
                "trust_level": "STATIC_SUPPORTED",
                "emitted_artifact_type": "asm",
                "ownership_reason": (
                    "range-tool and vasm independently round-trip the exact slice; "
                    "the static census has a direct caller target and an explicit RTS"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"island overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"island is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET", "DATA_KNOWN"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_audit(audit_root, rom):
    report = json.loads((audit_root / "audit.json").read_text())
    by_range = {(item["start"], item["end"]): item for item in report}
    evidence = []
    for start, end in SLICES:
        item = by_range.get((start, end))
        if not item or item.get("status") != "EXACT":
            raise ValueError(f"missing EXACT audit evidence for 0x{start:06X}")
        asm = audit_root / f"{start:06X}_{end:06X}.asm"
        binary = audit_root / f"{start:06X}_{end:06X}.bin"
        if not asm.exists() or not binary.exists() or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"audit artifact mismatch at 0x{start:06X}")
        evidence.append({"start": start, "end": end, "bytes": end - start,
                         "callers": item["callers"],
                         "asm_sha256": hashlib.sha256(asm.read_bytes()).hexdigest(),
                         "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()})
    return evidence


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    audit_root = Path(args.audit_root).resolve()
    evidence = validate_audit(audit_root, rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end in SLICES:
        current = split_unknown(current, start, end)
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    for index, entry in enumerate(current):
        if entry.get("emitted_artifact_type") == "asm" and entry["start"] in dict(SLICES):
            sources[index] = audit_root / f"{entry['start']:06X}_{entry['end']:06X}.asm"
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO60 contiguous static islands"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto60-contiguous-islands.v1", "evidence": evidence,
              "promoted_bytes": sum(end - start for start, end in SLICES),
              "metrics": manifest["metrics"],
              "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--audit-root", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
