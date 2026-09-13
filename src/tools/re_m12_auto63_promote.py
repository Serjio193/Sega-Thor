"""Promote only the exact AUTO63-proven six-byte stream interval."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import zlib


ROOT = Path(__file__).resolve().parent


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


AUTO = load_module("re_auto_promote", ROOT / "re_auto_promote.py")
STREAM = load_module("re_m12_record_stream_promote", ROOT / "re_m12_record_stream_promote.py")
ROM_SHA256 = STREAM.ROM_SHA256
CLASSIFICATION = "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM"


def promote(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    evidence_path = Path(args.evidence).resolve()
    evidence = json.loads(evidence_path.read_text())
    candidate = evidence.get("promotion_candidate", {})
    if candidate.get("status") != "ELIGIBLE":
        raise ValueError("AUTO63 evidence is not promotion-eligible")
    if candidate.get("classification") != CLASSIFICATION:
        raise ValueError("unexpected AUTO63 classification")
    start, end = int(candidate["start"]), int(candidate["end"])
    if end <= start or (end - start) % 6 or (end - start) != 108:
        raise ValueError("AUTO63 candidate is not exactly 18 six-byte records")

    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    if baseline.get("rom_sha256") != ROM_SHA256:
        raise ValueError("baseline manifest ROM identity mismatch")
    entries = STREAM.renumber(copy.deepcopy(baseline["entries"]))
    promoted = STREAM.split_unknown(
        entries, start, end, CLASSIFICATION,
        "AUTO63 focused BizHawk capture proves A6+0x1A -> A0, the AF20 ROM "
        "consumer, D3=17, and the repeated six-byte address enumeration",
        source="M12_AUTO63_AF22_focused_register_slice")
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    mapped = AUTO.materialize(materialized, promoted, rom,
                              AUTO.source_map(promoted, baseline_path))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, mapped)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(mapped, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(STREAM.source_owned(mapped, len(rom)))
    manifest["transaction"] = "M12-AUTO63 AF22 focused register-slice promotion"
    manifest["evidence"] = {
        "report": str(evidence_path),
        "report_sha256": hashlib.sha256(evidence_path.read_bytes()).hexdigest(),
        "candidate": {"start": start, "end": end, "classification": CLASSIFICATION},
    }
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto63-promotion.v1",
        "baseline_manifest": str(baseline_path),
        "promoted_range": {"start": start, "end": end, "bytes": end - start,
                           "classification": CLASSIFICATION},
        "metrics": manifest["metrics"],
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
    parser.add_argument("--evidence", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    promote(parser.parse_args())


if __name__ == "__main__":
    main()
