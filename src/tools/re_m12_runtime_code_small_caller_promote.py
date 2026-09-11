"""Promote the AUTO43 caller-backed exact 68000 routine."""
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
START, END = 0x0083F8, 0x00846C
CALLER = 0x006230


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= START and END <= entry["end"]:
            replacement = [{**entry, "end": START}] if entry["start"] < START else []
            replacement.append({
                "start": START, "end": END, "kind": "CODE_VERIFIED",
                "source": "M12_AUTO43_runtime_small_caller_exact_routine",
                "confidence": "CONFIRMED",
                "classification": "CALLER_BACKED_RUNTIME_EXACT_ASM_ROUNDTRIP",
                "emitted_artifact_type": "asm",
                "ownership_reason": (
                    "exact BSR caller, complete runtime-observed decoded range, "
                    "RTS boundaries, and canonical-ROM-equal ASM"),
            })
            if END < entry["end"]:
                replacement.append({**entry, "start": END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if START < entry["end"] and entry["start"] < END:
            raise ValueError("routine overlaps non-UNKNOWN ownership")
    raise ValueError("routine is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_evidence(root, rom, runtime):
    root = Path(root)
    asm, binary = root / f"sub_{START:06X}.asm", root / f"sub_{START:06X}.bin"
    detail = root / f"{START:06X}_{END:06X}.json"
    if not all(path.exists() for path in (asm, binary, detail)):
        raise ValueError("missing AUTO43 exact routine evidence")
    if binary.read_bytes() != rom[START:END]:
        raise ValueError("AUTO43 binary differs from canonical ROM")
    if json.loads(detail.read_text()) != {"start": START, "end": END,
                                          "terminator": "RTS", "roundtrip": "EXACT"}:
        raise ValueError("AUTO43 decoder/round-trip contract changed")
    observed = {int(address, 16) for address in runtime["executed_addresses"]}
    if START not in observed or rom[CALLER:CALLER + 2] != bytes.fromhex("6100"):
        raise ValueError("AUTO43 caller/entry contract changed")
    facts = [item for item in runtime["execution_facts"]
             if START <= int(item["address"], 16) < END]
    if len(facts) != 33 or any(item.get("decoder_status") != "DECODED" for item in facts):
        raise ValueError("AUTO43 runtime facts are not the complete decoded routine")
    return {"start": START, "end": END, "bytes": END - START, "caller": CALLER,
            "runtime_observed_instruction_starts": len(facts),
            "asm_sha256": hashlib.sha256(asm.read_bytes()).hexdigest(),
            "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}


def run(args):
    rom_path = Path(args.rom).resolve(); rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    evidence = validate_evidence(Path(args.evidence_root).resolve(), rom,
                                 json.loads(Path(args.runtime_evidence).read_text()))
    baseline_path = Path(args.manifest).resolve(); baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]))
    sources = {i: baseline_path.parent / e["artifact"] for i, e in enumerate(current)
               if e.get("emitted_artifact_type") == "asm" and e.get("artifact")}
    sources[next(i for i, e in enumerate(current) if e["start"] == START)] = (
        Path(args.evidence_root).resolve() / f"sub_{START:06X}.asm")
    materialized = output / "materialized"; entries = AUTO.materialize(materialized, current, rom, sources)
    rebuilt = baseline_path.parent / "rebuilt.rom"
    if hashlib.sha256(rebuilt.read_bytes()).hexdigest() != ROM_SHA256:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom)); manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO43 runtime small caller-backed exact routine"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto43-runtime-code.v1", "routine": evidence,
              "metrics": manifest["metrics"], "full_rom": {"size": len(rebuilt),
              "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
              "sha1": hashlib.sha1(rebuilt).hexdigest(), "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "manifest", "evidence-root", "runtime-evidence", "output"):
        parser.add_argument(f"--{name}", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
