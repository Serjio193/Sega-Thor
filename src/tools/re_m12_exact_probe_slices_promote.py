"""Promote exact runtime-correlated 68000 probe slices."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SLICES = (
    (0x008F12, 0x008F22, "m12-6-probe-a"),
    (0x008F22, 0x008F3A, "m12-6-probe-a"),
    (0x008F3A, 0x008F72, "m12-6-probe-a"),
    (0x0091F2, 0x009234, "m12-6-probe-a"),
    (0x0092B0, 0x0092F4, "m12-6-probe-a"),
    (0x0092F4, 0x009330, "m12-6-probe-a"),
    (0x03B1D0, 0x03B358, "m12-auto-probe-all-b"),
    (0x060090, 0x060286, "m12-auto-probe-all-b"),
    (0x061232, 0x061328, "m12-auto-probe-all-b"),
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
                "source": "M12_AUTO16_exact_runtime_probe_slices",
                "confidence": "CONFIRMED",
                "classification": "RUNTIME_CORRELATED_EXACT_PROBE_SLICE",
                "emitted_artifact_type": "asm",
                "ownership_reason": (
                    "canonical-ROM-equal decoded ASM and binary from a bounded "
                    "probe slice; its entry PC is present in the canonical "
                    "runtime execution evidence"),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"slice overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"slice is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def evidence_path(root, start, end, directory):
    return Path(root) / directory / f"{start:06X}_{end:06X}"


def validate_evidence(root, rom, runtime):
    observed = {int(str(address), 16) for address in runtime["executed_addresses"]}
    result = []
    for start, end, directory in SLICES:
        stem = evidence_path(root, start, end, directory)
        asm = stem.with_suffix(".asm")
        binary = stem.with_suffix(".bin")
        detail = stem.with_suffix(".json")
        if not asm.exists() or not binary.exists() or not detail.exists():
            raise ValueError(f"missing exact probe evidence for 0x{start:06X}")
        if binary.read_bytes() != rom[start:end]:
            raise ValueError(f"probe binary differs from canonical ROM at 0x{start:06X}")
        decoded = json.loads(detail.read_text())
        if decoded.get("start") != start or decoded.get("end") != end:
            raise ValueError(f"probe decoder bounds changed at 0x{start:06X}")
        observed_count = sum(start <= address < end for address in observed)
        if not observed_count:
            raise ValueError(f"probe slice was not runtime-observed at 0x{start:06X}")
        result.append({"start": start, "end": end, "bytes": end - start,
                       "evidence_directory": directory,
                       "runtime_observed_instruction_starts": observed_count,
                       "asm_sha256": hashlib.sha256(asm.read_bytes()).hexdigest(),
                       "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()})
    return result


def verify_inherited(materialized, baseline_root, baseline_rebuilt, rom, entries,
                     evidence, evidence_root):
    rebuilt = Path(baseline_rebuilt).read_bytes()
    if len(rebuilt) != len(rom) or hashlib.sha256(rebuilt).hexdigest() != ROM_SHA256:
        raise ValueError("verified baseline rebuilt ROM is not canonical")
    source_by_range = {(item["start"], item["end"]): item for item in evidence}
    for entry in entries:
        artifact = materialized / entry["artifact"]
        bounds = (entry["start"], entry["end"])
        if bounds in source_by_range:
            source = source_by_range[bounds]
            path = evidence_path(evidence_root, *bounds,
                                 source["evidence_directory"]).with_suffix(".asm")
            if artifact.read_bytes() != path.read_bytes():
                raise ValueError("materialized probe ASM differs from evidence")
        elif entry.get("emitted_artifact_type") == "asm":
            previous = baseline_root / "code" / f"sub_{entry['start']:06X}.asm"
            if not previous.exists() or artifact.read_bytes() != previous.read_bytes():
                raise ValueError("inherited ASM artifact changed")
        elif artifact.read_bytes() != rom[entry["start"]:entry["end"]]:
            raise ValueError("materialized ROM artifact differs from canonical slice")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    return {"mode": "INHERITED_BASELINE_FULL_ROM",
            "baseline_rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()}


def run(args):
    evidence_root = Path(args.evidence_root).resolve()
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    runtime = json.loads(Path(args.runtime_evidence).read_text())
    evidence = validate_evidence(evidence_root, rom, runtime)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end, _ in SLICES:
        current = split_unknown(current, start, end)
    sources = {entry["start"]: baseline_path.parent / entry["artifact"]
               for entry in baseline["entries"]
               if entry.get("emitted_artifact_type") == "asm"}
    for start, end, directory in SLICES:
        sources[start] = evidence_path(evidence_root, start, end,
                                        directory).with_suffix(".asm")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    code_sources = {index: sources[entry["start"]]
                    for index, entry in enumerate(current)
                    if entry.get("emitted_artifact_type") == "asm"}
    entries = AUTO.materialize(materialized, current, rom, code_sources)
    verification = verify_inherited(
        materialized, baseline_path.parent, args.baseline_rebuilt, rom,
        entries, evidence, evidence_root)
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO16 exact runtime-correlated probe slices"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto16-exact-probe-slices.v1",
        "slices": evidence, "metrics": manifest["metrics"],
        "promoted_code_bytes": sum(item["bytes"] for item in evidence),
        "verification": verification,
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--baseline-rebuilt", required=True)
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--runtime-evidence", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
