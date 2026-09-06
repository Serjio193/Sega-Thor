"""Developer-only transactional promotion of safe UNKNOWN code ranges."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zlib
from collections import Counter


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_full_split_run", HERE / "re_full_split_run.py")
FULL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FULL)

if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from re_auto_promote_helpers import (acceptance_windows, classify_error, clean_detail,
    forms_from_asm, forms_from_json, instruction_family,
    legacy_mismatch_analysis, reject_clusters, reject_form, resolve_artifact,
    finalize_rejection)


def run(command, cwd=None, check=False):
    result = subprocess.run([str(value) for value in command], cwd=cwd,
                            text=True, capture_output=True, check=False)
    if check and result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


def parse_int(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def parse_range(value):
    if ".." not in value:
        return None
    start, end = value.split("..", 1)
    return parse_int(start), parse_int(end)


def overlap(left, right):
    return left[0] < right[1] and right[0] < left[1]


def candidate_score(item, size):
    structural = {"STRONG_STATIC": 40, "MODERATE_STATIC": 30,
                  "WEAK_STATIC": 15}.get(item["structural_classification"], 0)
    complexity = {"LEAF": 20, "SHALLOW": 12}.get(item["complexity"], 0)
    callers = min(int(item["direct_caller_count"]), 4) * 3
    target = 10 if item["known_static_target"] else 0
    beta = 5 if item["existing_beta_support"] else 0
    small = max(0, 18 - size // 8)
    return structural + complexity + callers + target + beta + small


def discover_candidates(mass, ghidra, manifest, limit=25, excluded_addresses=None):
    excluded_addresses = excluded_addresses or set()
    functions = {}
    for function in ghidra.get("functions", []):
        bounds = parse_range(function.get("range", ""))
        if bounds:
            functions[parse_int(function["entry"])] = bounds
    existing = [(entry["start"], entry["end"]) for entry in manifest["entries"]
                if entry["kind"] == "CODE_VERIFIED"]
    unknown = [(entry["start"], entry["end"]) for entry in manifest["entries"]
               if entry["kind"] == "UNKNOWN"]
    eligible = []
    for item in mass.get("candidates", []):
        address = parse_int(item["entry"])
        if address in excluded_addresses:
            continue
        bounds = functions.get(address)
        if not bounds or bounds[0] & 1 or bounds[0] >= bounds[1]:
            continue
        if any(overlap(bounds, region) for region in existing):
            continue
        if not any(bounds[0] >= region[0] and bounds[1] <= region[1] for region in unknown):
            continue
        safe = (item["decode_ok"] and item["first_instruction_supported"] and
                item["boundary_status"] == "BOUNDARY_AGREES" and
                item["unsupported_opcode_count"] == 0 and
                item["unsupported_addressing_count"] == 0 and
                not item["unresolved_indirect_flow"] and not item["decode_conflict"] and
                not item["known_data_overlap"] and not item["confirmed_code_overlap"] and
                not item["other_ghidra_overlap"] and item["complexity"] in ("LEAF", "SHALLOW"))
        if not safe:
            continue
        size = bounds[1] - bounds[0]
        reasons = ["structural=" + item["structural_classification"],
                   "complexity=" + item["complexity"], "decoder_supported",
                   "boundary_agrees", "no_conflict"]
        eligible.append({"address": address, "start": bounds[0], "end": bounds[1],
                         "size": size, "score": candidate_score(item, size),
                         "classification": "AUTO_ELIGIBLE", "reasons": reasons,
                         "evidence": item})
    eligible.sort(key=lambda item: (-item["score"], item["size"], item["address"]))
    return eligible[:limit], len(eligible)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def promote(entries, candidate):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN" or not (entry["start"] <= candidate["start"] and
                                               candidate["end"] <= entry["end"]):
            continue
        replacement = []
        if entry["start"] < candidate["start"]:
            replacement.append({"start": entry["start"], "end": candidate["start"],
                                "kind": "UNKNOWN", "source": "canonical_local_rom",
                                "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        replacement.append({"start": candidate["start"], "end": candidate["end"],
                            "kind": "CODE_VERIFIED", "source": "automated_blob_promotion",
                            "confidence": "STRUCTURAL_EXACT_BYTE_ROUND_TRIP",
                            "emitted_artifact_type": "asm",
                            "promotion_score": candidate["score"]})
        if candidate["end"] < entry["end"]:
            replacement.append({"start": candidate["end"], "end": entry["end"],
                                "kind": "UNKNOWN", "source": "canonical_local_rom",
                                "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        return renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError("candidate is outside an UNKNOWN range")


def materialize(root, entries, rom, code_sources):
    if root.exists():
        shutil.rmtree(root)
    (root / "code").mkdir(parents=True)
    (root / "blobs").mkdir()
    sources = {}
    for index, entry in enumerate(entries):
        if entry["kind"] == "CODE_VERIFIED":
            source = code_sources[index]
            filename = f"sub_{entry['start']:06X}.asm"
            destination = root / "code" / filename
            shutil.copyfile(source, destination)
            entry["artifact"] = f"code/{filename}"
            sources[index] = destination
        else:
            filename = f"{entry['start']:06X}_{entry['end']:06X}.bin"
            (root / "blobs" / filename).write_bytes(rom[entry["start"]:entry["end"]])
            entry["artifact"] = f"blobs/{filename}"
    lines = ["; Generated transactional full-ROM promotion layout.", "    org $000000"]
    for entry in entries:
        if entry["kind"] == "CODE_VERIFIED":
            lines.extend(line for line in (root / entry["artifact"]).read_text().splitlines()
                         if not line.startswith("    org ") and not line.startswith("sub_"))
        else:
            lines.extend([f"data_{entry['start']:06X}:",
                          f'    incbin "{entry["artifact"]}"'])
    (root / "full_layout.asm").write_text("\n".join(lines) + "\n")
    return entries


def manifest_for(entries, rom_size):
    entries = renumber(entries)
    metrics = FULL.metrics(entries, rom_size)
    return {"schema": "oasis.full-rom-split.v1", "start": 0, "end": rom_size,
            "rom_size": rom_size, "entries": entries, "metrics": metrics,
            "quality": {"manifest_entries": len(entries), "gaps": 0, "overlaps": 0,
                        "verified_code_ranges": sum(e["kind"] == "CODE_VERIFIED" for e in entries),
                        "blob_ranges": sum(e["kind"] == "UNKNOWN" for e in entries),
                        "conflict_ranges": 0,
                        "smallest_range": min(e["size"] for e in entries),
                        "largest_range": max(e["size"] for e in entries)}}


def verify_full(root, rom, assembler, entries):
    assembled = run([assembler, "-m68000", "-no-opt", "-Fbin", "-o",
                     root / "rebuilt.rom", root / "full_layout.asm"], root)
    if assembled.returncode:
        return False, None, "ASSEMBLER_ERROR", assembled.stdout + assembled.stderr
    rebuilt = (root / "rebuilt.rom").read_bytes()
    difference = FULL.first_difference(rom, rebuilt, entries)
    if difference or len(rebuilt) != len(rom):
        return False, difference, "FULL_ROM_MISMATCH", "byte comparison failed"
    return True, None, "", ""


def summary(manifest):
    return {"ASM_BYTES": manifest["metrics"]["ASM_BYTES"],
            "STRUCTURED_DATA_BYTES": manifest["metrics"]["STRUCTURED_DATA_BYTES"],
            "BLOB_BYTES": manifest["metrics"]["BLOB_BYTES"],
            "CODE_VERIFIED_ENTRIES": manifest["quality"]["verified_code_ranges"],
            "UNKNOWN_ENTRIES": manifest["quality"]["blob_ranges"]}



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, help="existing oasis_re_assemble")
    parser.add_argument("--range-tool", required=True, help="oasis_re_assemble_range")
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True, help="post-M11.12 manifest.json")
    parser.add_argument("--mass-report", required=True)
    parser.add_argument("--ghidra-map", required=True)
    parser.add_argument("--output", required=True, help="new ignored output directory")
    parser.add_argument("--max-candidates", type=int, default=100)
    parser.add_argument("--prior-report", help="previous promotion report to exclude")
    args = parser.parse_args()
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    mass = json.loads(Path(args.mass_report).read_text())
    ghidra = json.loads(Path(args.ghidra_map).read_text())
    prior = None
    excluded_addresses = set()
    retried_addresses = set()
    if args.prior_report:
        prior = json.loads(Path(args.prior_report).read_text())
        for item in prior.get("attempts", []):
            address = parse_int(item["address"])
            if item.get("reason") == "SLICE_MISMATCH":
                retried_addresses.add(address)
            else:
                excluded_addresses.add(address)
    candidates, eligible_count = discover_candidates(
        mass, ghidra, baseline, args.max_candidates,
        excluded_addresses | retried_addresses)
    if retried_addresses:
        retry_candidates, _ = discover_candidates(
            mass, ghidra, baseline, len(mass.get("candidates", [])),
            set(excluded_addresses))
        candidates = sorted(candidates + [item for item in retry_candidates
                                          if item["address"] in retried_addresses],
                           key=lambda item: (-item["score"], item["size"], item["address"]))[:args.max_candidates]
    output.mkdir(parents=True)
    regression = output / "regression"
    run([sys.executable, HERE / "re_assemble_run.py", "--tool", args.tool,
         "--assembler", args.assembler, "--rom", rom_path, "--output", regression], check=True)
    regression_result = json.loads((regression / "result.json").read_text())
    current = renumber(copy.deepcopy(baseline["entries"]))
    before = summary(baseline)
    baseline_forms = set()
    for entry in baseline["entries"]:
        if entry["kind"] == "CODE_VERIFIED":
            source = resolve_artifact(baseline_path, entry["artifact"])
            baseline_forms.update(forms_from_asm(source))
    code_sources = {}
    for index, entry in enumerate(current):
        if entry["kind"] == "CODE_VERIFIED":
            code_sources[index] = resolve_artifact(baseline_path, entry["artifact"])
    attempts = []
    accepted = []
    accepted_families = set()
    staging = output / "staging"
    staging.mkdir()
    for attempt_index, candidate in enumerate(candidates, 1):
        trial = output / f"trial_{attempt_index:02d}"
        asm = staging / f"candidate_{attempt_index:02d}.asm"
        data = staging / f"candidate_{attempt_index:02d}.json"
        trial.mkdir(parents=True)
        record = {"address": f"0x{candidate['address']:06X}",
                  "range": [candidate["start"], candidate["end"]],
                  "score": candidate["score"], "classification": candidate["classification"],
                  "slice_match": False, "full_rom_match": False, "accepted": False,
                  "reason": ""}
        if not any(entry["kind"] == "UNKNOWN" and entry["start"] <= candidate["start"] and
                   candidate["end"] <= entry["end"] for entry in current):
            record["reason"] = "CODE_DATA_CONFLICT"
            record["detail"] = "candidate no longer lies inside an UNKNOWN range"
            attempts.append(finalize_rejection(record, rom))
            shutil.rmtree(trial)
            continue
        emitted = run([args.range_tool, rom_path, hex(candidate["start"]),
                       hex(candidate["end"]), asm, data])
        if emitted.returncode:
            record["reason"] = classify_error(emitted.stdout + emitted.stderr)
            record["detail"] = clean_detail(emitted.stdout + emitted.stderr, output).strip()[:400]
            attempts.append(finalize_rejection(record, rom))
            shutil.rmtree(trial)
            continue
        binary = trial / "candidate.bin"
        assembled = run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode:
            record["reason"] = classify_error(assembled.stdout + assembled.stderr)
            record["detail"] = clean_detail(assembled.stdout + assembled.stderr, output).strip()[:400]
            attempts.append(finalize_rejection(record, rom))
            shutil.rmtree(trial)
            continue
        rebuilt_slice = binary.read_bytes()
        expected = rom[candidate["start"]:candidate["end"]]
        record["slice_match"] = rebuilt_slice == expected
        if not record["slice_match"]:
            record["reason"] = "SLICE_MISMATCH"
            if rebuilt_slice != expected:
                difference = FULL.first_difference(
                    expected, rebuilt_slice,
                    [{"manifest_index": 0, "start": candidate["start"],
                      "end": candidate["end"], "emitted_artifact_type": "asm"}])
                if difference:
                    difference["rom_offset"] += candidate["start"]
                record["first_difference"] = difference
            attempts.append(finalize_rejection(record, rom, data))
            shutil.rmtree(trial)
            continue
        try:
            new_entries = promote(current, candidate)
        except ValueError as error:
            record["reason"] = "BOUNDARY_UNCERTAIN"
            record["detail"] = str(error)
            attempts.append(finalize_rejection(record, rom, data))
            shutil.rmtree(trial)
            continue
        new_sources = {}
        for index, entry in enumerate(new_entries):
            if entry["kind"] == "CODE_VERIFIED":
                if entry["start"] == candidate["start"]:
                    new_sources[index] = asm
                else:
                    old = next((old_index for old_index, old_entry in enumerate(current)
                                if old_entry["start"] == entry["start"] and
                                old_entry["kind"] == "CODE_VERIFIED"), None)
                    if old is None:
                        raise ValueError("accepted source mapping lost")
                    new_sources[index] = code_sources[old]
        trial_entries = materialize(trial, new_entries, rom, new_sources)
        matched, difference, reason, detail = verify_full(trial, rom, args.assembler, trial_entries)
        record["full_rom_match"] = matched
        if not matched:
            record["reason"] = reason
            record["detail"] = detail[:400]
            if difference:
                record["first_difference"] = difference
            attempts.append(finalize_rejection(record, rom, data))
            shutil.rmtree(trial)
            continue
        accepted_code = output / "accepted_code"
        accepted_code.mkdir(exist_ok=True)
        stable = accepted_code / f"sub_{candidate['start']:06X}.asm"
        shutil.copyfile(asm, stable)
        for index, entry in enumerate(new_entries):
            if entry["kind"] == "CODE_VERIFIED" and entry["start"] == candidate["start"]:
                new_sources[index] = stable
        current, code_sources = new_entries, new_sources
        record["accepted"] = True
        record["reason"] = "FULL_MATCH"
        instructions = json.loads(data.read_text())["instructions"]
        record["instructions"] = len(instructions)
        record["bytes"] = candidate["size"]
        record["instruction_forms"] = forms_from_json(data)
        accepted_families.update(instruction_family(item) for item in instructions)
        attempts.append(record)
        accepted.append(record)
        shutil.rmtree(trial)
    final_root = output / "final"
    current = materialize(final_root, current, rom, code_sources)
    final_manifest = manifest_for(current, len(rom))
    matched, difference, reason, detail = verify_full(final_root, rom, args.assembler, current)
    final_manifest.update({"rom_sha256": hashlib.sha256(rom).hexdigest(),
                           "full_match": matched, "first_difference": difference})
    if matched:
        rebuilt = (final_root / "rebuilt.rom").read_bytes()
        final_manifest["hashes"] = {"crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                                     "sha256": hashlib.sha256(rebuilt).hexdigest()}
    else:
        final_manifest["hashes"] = {}
    after = summary(final_manifest)
    windows = acceptance_windows(attempts)
    rates = [item["acceptance_percent"] for item in windows]
    if rates and rates[-1] < 50.0:
        trend = "SATURATED"
    elif len(rates) >= 2 and rates[-1] + 15.0 < rates[0]:
        trend = "DECLINING"
    else:
        trend = "STABLE"
    unsupported = Counter(reject_form(item) for item in attempts
                          if item.get("reason") == "UNSUPPORTED_FORM")
    report = {"schema": "oasis.m68k.re-auto-promote.v1",
              "rom_sha256": final_manifest["rom_sha256"],
              "evidence": {"mass_schema": mass.get("schema"), "ghidra_schema": ghidra.get("schema"),
                           "baseline_manifest": "manifest.json"},
              "deterministic": True, "manual_candidate_addresses": False,
              "transactional": True,
              "discovered_candidates": len(mass.get("candidates", [])),
              "eligible_candidates": eligible_count, "new_candidates": len(candidates) - len(retried_addresses),
              "maximum_attempts": args.max_candidates, "attempted": len(attempts),
              "accepted": len(accepted), "rejected": len(attempts) - len(accepted),
              "before": before, "after": after,
              "delta": {key: after[key] - before[key] for key in before},
              "attempts": attempts, "regression": regression_result,
              "full_match": matched, "hashes": final_manifest["hashes"],
              "handwritten_overrides": 0, "systemic_fixes": [],
              "prior_report": args.prior_report or "",
              "retried_addresses": [f"0x{address:06X}" for address in sorted(retried_addresses)],
              "legacy_slice_mismatch_analysis": legacy_mismatch_analysis(prior, attempts),
              "acceptance_percent": (100.0 * len(accepted) / len(attempts)) if attempts else 0.0,
              "acceptance_windows": windows, "acceptance_trend": trend,
              "reject_classes": dict(Counter(item["reason"] for item in attempts
                                              if not item["accepted"])),
              "accepted_instruction_forms": sorted({form for item in accepted
                                                     for form in item.get("instruction_forms", [])}),
              "newly_supported_forms": sorted(accepted_families - baseline_forms),
              "remaining_unsupported_forms": sorted({reject_form(item) for item in attempts
                                                      if item["reason"] in
                                                      ("UNSUPPORTED_FORM", "ASSEMBLER_SYNTAX")}),
              "unsupported_form_inventory": [
                  {"form": form, "count": count,
                   "examples": [item["address"] for item in attempts
                                if item.get("reason") == "UNSUPPORTED_FORM" and
                                reject_form(item) == form][:3],
                   "fixed": False}
                  for form, count in unsupported.most_common()],
              "reject_clusters": reject_clusters(attempts),
              "top_reject_forms": [{"form": form, "count": count} for form, count in
                                    Counter(reject_form(item) for item in attempts
                                            if not item["accepted"]).most_common(10)]}
    (output / "manifest.json").write_text(json.dumps(final_manifest, indent=2) + "\n")
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Candidates: discovered={report['discovered_candidates']} eligible={eligible_count} "
          f"attempted={len(attempts)} accepted={len(accepted)}")
    print(f"Coverage: ASM {before['ASM_BYTES']} -> {after['ASM_BYTES']} bytes; "
          f"BLOB {before['BLOB_BYTES']} -> {after['BLOB_BYTES']} bytes")
    print(f"Full ROM: {len(rom)} bytes; exact={'YES' if matched else 'NO'}")
    return 0 if matched and len(attempts) > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
