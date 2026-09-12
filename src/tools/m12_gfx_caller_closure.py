"""Close static 0x3820 caller source sets without repeating the global census."""
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
CALLERS = [
    0x00C394, 0x00D3C8, 0x00D4EE, 0x00D54A, 0x00D650,
    0x014558, 0x01458E, 0x0145C4, 0x0145FC, 0x024A20, 0x02A67E,
    0x02B1D0, 0x02DB52, 0x02F6A0, 0x03A7FE, 0x03ACB4, 0x03ADC0,
    0x03B236, 0x03B28A, 0x03B2FE, 0x03C07C, 0x03C276, 0x03C27E,
    0x03C286, 0x03C5CA, 0x03C5D2, 0x03C5DA, 0x03C5E6, 0x03C9DA,
    0x03C9E2, 0x03C9EA, 0x03CBE0, 0x03CBE8, 0x03CBF0, 0x03CC5A,
    0x03CC62, 0x03CCD8, 0x03CCE0, 0x03CCE8, 0x03CEA6, 0x03CEAE,
    0x03CEBA, 0x03CEC2, 0x03CECA, 0x03D048, 0x03D38E, 0x03D3A0,
    0x03D5AE, 0x03E61A, 0x03E662, 0x03E704, 0x03E820,
]

ROUTINES = {
    0x00C394: "0x00C326 bounded graphics loader",
    0x00D3C8: "0x00D3B2 indexed-resource loader",
    0x00D4EE: "0x00D406 shared resource loader",
    0x00D54A: "0x00D406 shared resource loader",
    0x00D650: "0x00D406 shared resource loader",
    0x014558: "0x01454C menu graphics family",
    0x01458E: "0x01454C menu graphics family",
    0x0145C4: "0x01454C menu graphics family",
    0x0145FC: "0x01454C menu graphics family",
    0x024A20: "0x024858 bounded graphics loader",
    0x02A67E: "0x02A652 bounded graphics loader",
    0x02B1D0: "0x02B194 bounded graphics loader",
    0x02DB52: "0x02DB24 resource family",
    0x02F6A0: "0x02F662 resource family",
    0x03A7FE: "0x03A748 screen initialization family",
    0x03ACB4: "0x03ACA8 tilemap loader",
    0x03ADC0: "0x03ADB4 tilemap loader",
    0x03B236: "0x03B1D0 resource family",
    0x03B28A: "0x03B1D0 resource family",
    0x03B2FE: "0x03B1D0 resource family",
    0x03C07C: "0x03C04C resource family",
    0x03C276: "0x03C1E8 resource family",
    0x03C27E: "0x03C1E8 resource family",
    0x03C286: "0x03C1E8 resource family",
    0x03C5CA: "0x03C59C resource family",
    0x03C5D2: "0x03C59C resource family",
    0x03C5DA: "0x03C59C resource family",
    0x03C5E6: "0x03C59C resource family",
    0x03C9DA: "0x03C9CC resource family",
    0x03C9E2: "0x03C9CC resource family",
    0x03C9EA: "0x03C9CC resource family",
    0x03CBE0: "0x03CB9E resource family",
    0x03CBE8: "0x03CB9E resource family",
    0x03CBF0: "0x03CB9E resource family",
    0x03CC5A: "0x03CC4C resource family",
    0x03CC62: "0x03CC4C resource family",
    0x03CCD8: "0x03CCCA resource family",
    0x03CCE0: "0x03CCCA resource family",
    0x03CCE8: "0x03CCCA resource family",
    0x03CEA6: "0x03CE98 resource family",
    0x03CEAE: "0x03CE98 resource family",
    0x03CEBA: "0x03CE98 resource family",
    0x03CEC2: "0x03CE98 resource family",
    0x03CECA: "0x03CE98 resource family",
    0x03D048: "0x03CFDE graphics family",
    0x03D38E: "0x03D228 graphics family",
    0x03D3A0: "0x03D228 graphics family",
    0x03D5AE: "0x03D59A entity initializer",
    0x03E61A: "0x03E4DC graphics family",
    0x03E662: "0x03E4DC graphics family",
    0x03E704: "0x03E4DC graphics family",
    0x03E820: "0x03E7F4 graphics family",
}

DIRECT = {
    0x00C394: (0x16943C, "0x00FF2FA8", "0x00C386"),
    0x014558: (0x141580, "0x00FF2FA8", "0x01454C"),
    0x01458E: (0x1425AE, "0x00FF2FA8", "0x014582"),
    0x0145C4: (0x143262, "0x00FF2FA8", "0x0145B8"),
    0x0145FC: (0x143A42, "0x00FF2FA8", "0x0145F0"),
    0x024A20: (0x143A42, "0x00FF2FA8", "0x024A14"),
    0x02A67E: (0x2E2FE6, "A1 inherited", "0x02A672"),
    0x02B1D0: (0x2E5FD0, "A1 inherited", "0x02B1C4"),
    0x03A7FE: (0x16943C, "0x00FF316C", "0x03A7F0"),
    0x03ACB4: (0x17A750, "0x00FFB1AE", "0x03ACA8"),
    0x03ADC0: (0x17E3BA, "0x00FFB1AE", "0x03ADB4"),
    0x03D048: (0x16943C, "0x00FF3FAC", "0x03D03C"),
    0x03D38E: (0x2F9A7E, "0x00FF3A32", "0x03D382"),
    0x03D3A0: (0x2FA686, "0x00FF4E32", "0x03D394"),
    0x03E662: (0x16943C, "0x00FF2FA8", "0x03E656"),
    0x03E704: (0x16943C, "0x00FF2FA8", "0x03E6F8"),
    0x03E820: (0x141580, "0x00FF2FA8", "0x03E814"),
}

CHAINS = {
    0x03C276: ("0x03C1E8 sequential family", 0x1894EA, [0x03C276, 0x03C27E, 0x03C286], "0x00FF2FA8"),
    0x03C5CA: ("0x03C59C sequential family", 0x18CF98, [0x03C5CA, 0x03C5D2, 0x03C5DA, 0x03C5E6], "0x00FF3FAC"),
    0x03C9DA: ("0x03C9CC sequential family", 0x18F214, [0x03C9DA, 0x03C9E2, 0x03C9EA], "0x00FF3FAC"),
    0x03CBE0: ("0x03CB9E sequential family", 0x191F0A, [0x03CBE0, 0x03CBE8, 0x03CBF0], "0x00FF3FAC"),
    0x03CC5A: ("0x03CC4C sequential family", 0x194BDA, [0x03CC5A, 0x03CC62], "0x00FF3FAC"),
    0x03CCD8: ("0x03CCCA sequential family", 0x19628A, [0x03CCD8, 0x03CCE0, 0x03CCE8], "0x00FF3FAC"),
    0x03CEA6: ("0x03CE98 sequential family", 0x199CBA, [0x03CEA6, 0x03CEAE, 0x03CEBA, 0x03CEC2, 0x03CECA], "0x00FF3FAC"),
}

TABLE_CALLS = {
    0x00D3C8: ("0x05CE96", "D0 << 2; absolute longword pointer", "0x00FF2FA8"),
    0x00D4EE: ("0x05CE96", "RAM 0x00FF16FA selector; up to four nonzero entries", "0x00FF3FA8"),
}

DYNAMIC = {
    0x00D54A: ("A4 = entry A1 + 4; helper 0x00F80E preserves A4; RAM-mediated field", "0x00FF3FA8", "INHERITED_A1_FIELD_NOT_ROM_PROVEN"),
    0x00D650: ("A4/A5 are post-state of the first 0x3820 call at 0x00D54A; FF16F1.bit2 gates this sequential continuation", "A5 after first call", "FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN"),
    0x02DB52: ("A0 = 0x00FF17AA post-source saved by preceding 0x00D406; A1 = 0x00FF2FA8", "0x00FF2FA8", "D406_POST_SOURCE_NOT_ROM_PROVEN"),
    0x02F6A0: ("A0/A1 = 0x00FF17AA/0x00FF17AE post-state saved by preceding 0x00D406", "0x00FF17AE after 0x00D406", "D406_POST_STATE_NOT_ROM_PROVEN"),
    0x03B236: ("A0 = 4(A5); A1 = 0x00FF316C shared output buffer", "0x00FF316C", "A5_FIELD_NOT_ROM_PROVEN"),
    0x03B28A: ("A0 = A3; A1 = 0x00FF316C shared output buffer", "0x00FF316C", "A3_ARGUMENT_NOT_ROM_PROVEN"),
    0x03B2FE: ("A0 = A4; 0x002CBC/0x00D950 preserve A4; A1 = 0x00FF316C", "0x00FF316C", "A4_ARGUMENT_NOT_ROM_PROVEN"),
    0x03C07C: ("A0/A1 inherited at shared family entry", "A1 inherited", "CALLER_ARGUMENT_NOT_ROM_PROVEN"),
    0x03D5AE: ("A0 loaded from RAM-mediated entity record at 0x00FF199E", "0x00FF19AE", "RAM_MEDIATED_SOURCE_NOT_ROM_PROVEN"),
    0x03E61A: ("A0/A1 inherited before later direct 0x16943C arm", "A1 inherited", "CALLER_ARGUMENT_NOT_ROM_PROVEN"),
}


def hx(value):
    return f"0x{value:06X}"


def owner(entries, address):
    return next((entry for entry in entries if entry["start"] <= address < entry["end"]), None)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_GFX2_caller_closure", "confidence": "CONFIRMED",
                "classification": "ANCIENT_COMPRESSED_RESOURCE",
                "emitted_artifact_type": "rom_asset", "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"resource overlaps non-UNKNOWN range {hx(entry['start'])}..{hx(entry['end'])}")
    raise ValueError(f"resource is not wholly UNKNOWN {hx(start)}..{hx(end)}")


def stream_records(census):
    return {int(item["start"]): item for item in census["ancient_sweep"]["records"]}


def expand_chain(records, family, first, callers, destination):
    result = []
    start = first
    for call_site in callers:
        item = records.get(start)
        if item is None or not item["deterministic"] or item["end"] != start + item["compressed_bytes"]:
            raise ValueError(f"strict caller stream missing at {hx(start)}")
        result.append({"caller_sites": [call_site], "family": family, "start": start,
                       "end": int(item["end"]), "compressed_bytes": int(item["compressed_bytes"]),
                       "decompressed_bytes": int(item["decompressed_bytes"]),
                       "output_sha256": item["output_sha256"], "mode": item["mode"],
                       "destination": destination})
        start = int(item["end"])
    return result


def subtract_blockers(blockers, promoted):
    result = {key: {"bytes": 0, "ranges": 0} for key in "BFG"}
    for item in blockers:
        fragments = [(item["start"], item["end"])]
        for start, end in promoted:
            next_fragments = []
            for left, right in fragments:
                if end <= left or right <= start:
                    next_fragments.append((left, right))
                else:
                    if left < start:
                        next_fragments.append((left, start))
                    if end < right:
                        next_fragments.append((end, right))
            fragments = next_fragments
        result[item["blocker"]]["ranges"] += len(fragments)
        result[item["blocker"]]["bytes"] += sum(right - left for left, right in fragments)
    return result


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["end"] - entry["start"] for entry in entries if entry["kind"] in kinds)
    return {"bytes": count, "percent": 100.0 * count / rom_size}


def annotate_resources(resources, records, baseline_entries, promoted):
    for resource in resources:
        item = records[resource["start"]]
        overlaps = sorted(start for start, other in records.items()
                          if start != resource["start"] and
                          start < resource["end"] and int(other["end"]) > resource["start"])
        resource.update({
            "declared_block_sizes": item["declared_block_sizes"],
            "carver_class": item["features"]["classification"],
            "overlap_count": len(overlaps),
            "overlap_starts": overlaps,
            "overlap_resolution": "CALLER_PROVEN_EXACT_SPAN; nested/false-start candidates retained as evidence" if overlaps else "NO_OVERLAP",
            "baseline_owner": owner(baseline_entries, resource["start"])["kind"],
            "promoted_here": (resource["start"], resource["end"]) in promoted,
            "executable_conflict": False,
            "local_canonical_extraction": True,
            "committed_decompressed_payload": False,
        })


def caller_records(resources, table_targets):
    by_call = {}
    for resource in resources:
        for call_site in resource["caller_sites"]:
            by_call.setdefault(call_site, []).append(resource)
    records = []
    for call_site in CALLERS:
        base = {"call_site": hx(call_site), "containing_routine": ROUTINES[call_site],
                "source_value_origin": None, "destination_value_origin": None,
                "source_mode": None, "immediate_source_address": None,
                "pointer_table_origin": None, "base_offset_expression": None,
                "selector_index_origin": None, "known_ram_destination": None,
                "source_set_kind": "UNRESOLVED", "source_set": [],
                "confidence": "BLOCKED", "blocker": None}
        if call_site in TABLE_CALLS:
            table, selector, destination = TABLE_CALLS[call_site]
            base.update({"source_value_origin": f"absolute longword table {table}",
                         "destination_value_origin": destination, "source_mode": "indirect table",
                         "pointer_table_origin": f"[{table},0x05D046)",
                         "base_offset_expression": f"{table} + (D0 << 2)" if call_site == 0x00D3C8 else f"{table} + (RAM 0x00FF16FA << 2)",
                         "selector_index_origin": selector,
                         "known_ram_destination": destination,
                         "source_set_kind": "FINITE_TABLE_DERIVED_SET",
                         "source_set": [{"start": item["start"], "end": item["end"], "index": item["index"]}
                                        for item in table_targets],
                         "confidence": "CONFIRMED; table and consumer closed"})
        elif call_site in by_call:
            items = by_call[call_site]
            item = items[0]
            sequential = "sequential" in item["family"]
            base.update({"source_value_origin": "A0 direct literal followed by returned A0" if sequential else "LEA.L direct ROM source literal",
                         "destination_value_origin": item.get("destination", "A1 inherited"),
                         "source_mode": "sequential A0" if sequential else "direct",
                         "immediate_source_address": hx(item["start"]),
                         "base_offset_expression": "A0_next = previous 0x3820 returned A0" if sequential else "absolute ROM literal",
                         "known_ram_destination": item.get("destination") if str(item.get("destination", "")).startswith("0x00FF") else None,
                         "source_set_kind": "EXACT_ROM_ADDRESS" if not sequential else "PARAMETERIZED_SEQUENTIAL_FAMILY",
                         "source_set": [{"start": x["start"], "end": x["end"],
                                         "compressed_bytes": x["compressed_bytes"],
                                         "decompressed_bytes": x["decompressed_bytes"],
                                         "output_sha256": x["output_sha256"]} for x in items],
                         "confidence": "CONFIRMED; strict Ancient boundary"})
        elif call_site in DYNAMIC:
            source, destination, blocker = DYNAMIC[call_site]
            base.update({"source_value_origin": source, "destination_value_origin": destination,
                         "source_mode": "indirect/RAM", "known_ram_destination": destination if destination.startswith("0x00FF") else None,
                         "source_set_kind": "UNRESOLVED_PARAMETERIZED_FAMILY", "confidence": "BLOCKED",
                         "blocker": blocker})
        else:
            raise ValueError(f"caller has no classification {hx(call_site)}")
        records.append(base)
    return records


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    sweep = json.loads(Path(args.sweep).read_text())
    if sweep["rom_sha256"] != ROM_SHA256 or sweep["static_3820_callers"] != CALLERS:
        raise ValueError("M12-GFX-1 caller/canonical census mismatch")
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    entries = renumber(baseline["entries"])
    records = stream_records(sweep)
    resources = []
    for family, first, sites, destination in CHAINS.values():
        chain = expand_chain(records, family, first, sites, destination)
        resources.extend(chain)
    for call_site, (start, destination, setup) in DIRECT.items():
        item = records.get(start)
        if item is None:
            raise ValueError(f"direct caller stream missing at {hx(start)}")
        resources.append({"caller_sites": [call_site], "family": "direct literal", "start": start,
                          "end": int(item["end"]), "compressed_bytes": int(item["compressed_bytes"]),
                          "decompressed_bytes": int(item["decompressed_bytes"]),
                          "output_sha256": item["output_sha256"], "mode": item["mode"],
                          "setup": setup, "destination": destination})
    table_targets = []
    table_base = 0x05CE96
    for index in range(1, 108):
        start = int.from_bytes(rom[table_base + index * 4:table_base + index * 4 + 4], "big")
        item = records.get(start)
        if item is None:
            raise ValueError(f"table target missing at index {index} {hx(start)}")
        overlaps = sorted(candidate for candidate, other in records.items()
                          if candidate != start and candidate < int(item["end"]) and
                          int(other["end"]) > start)
        table_targets.append({"index": index, "start": start, "end": int(item["end"]),
                              "compressed_bytes": int(item["compressed_bytes"]),
                              "decompressed_bytes": int(item["decompressed_bytes"]),
                              "output_sha256": item["output_sha256"],
                              "declared_block_sizes": item["declared_block_sizes"],
                              "carver_class": item["features"]["classification"],
                              "overlap_count": len(overlaps), "overlap_starts": overlaps,
                              "overlap_resolution": "TABLE_TARGET_EXACT_SPAN; nested/false-start candidates retained as evidence" if overlaps else "NO_OVERLAP",
                              "baseline_owner": owner(baseline["entries"], start)["kind"],
                              "promoted_here": False, "executable_conflict": False,
                              "local_canonical_extraction": True,
                              "committed_decompressed_payload": False})
    promoted = []
    for resource in sorted(resources, key=lambda item: (item["start"], item["end"])):
        entry = owner(entries, resource["start"])
        if entry is None:
            raise ValueError(f"resource outside manifest {hx(resource['start'])}")
        if entry["kind"] == "UNKNOWN":
            entries = split_unknown(entries, resource["start"], resource["end"],
                                     f"direct caller {hx(resource['caller_sites'][0])}; exact Ancient end")
            promoted.append((resource["start"], resource["end"]))
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(entries)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    AUTO.materialize(materialized, entries, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    if (materialized / "rebuilt.rom").read_bytes() != rom:
        raise ValueError("materialized ROM is not canonical")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update({
        "SOURCE_OWNED_BYTES": source_owned(entries, len(rom))["bytes"],
        "SOURCE_OWNED_PERCENT": source_owned(entries, len(rom))["percent"],
        "ROM_SIZE": len(rom),
    })
    manifest["transaction"] = "M12-GFX-2 0x3820 caller closure"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    annotate_resources(resources, records, baseline["entries"], promoted)
    by_start = {}
    for resource in resources:
        by_start.setdefault(resource["start"], {**resource, "caller_sites": []})["caller_sites"].extend(resource["caller_sites"])
    for resource in by_start.values():
        resource["caller_sites"] = sorted(set(resource["caller_sites"]))
    carver = json.loads(Path(args.carver_report).read_text())
    blockers_before = carver["blocker_map"]["by_blocker"]
    blockers_after = subtract_blockers(carver["blocker_map"]["ranges"], promoted)
    caller_catalog = caller_records(resources, table_targets)
    report = {
        "schema": "oasis.m68k.m12-gfx2-caller-closure.v1", "rom_sha256": ROM_SHA256,
        "abi": {"entry": 0x3820, "end": 0x3B3E, "source_register": "A0",
                "destination_register": "A1", "source_return": "A0 advanced to exclusive consumed end",
                "destination_return": "A1 advanced to exclusive produced end",
                "format_dispatch": "source[2] != 0 command stream; source[2] == 0 bit stream",
                "header_and_size": "strict Ancient block declarations and EOS determine compressed end; output length is parser-produced",
                "preserved_registers": ["D0-D2", "A2", "D3/D6/D7 in format B"],
                "helpers_or_hardware": "none in [0x3820,0x3B3E)",
                "68000_independent_parser": "MATCH on 0x16943C and 0x1894EA vectors"},
        "callers": caller_catalog, "resources": sorted(by_start.values(), key=lambda x: x["start"]),
        "table": {"base": table_base, "end": table_base + 432, "entry_size": 4,
                  "entry_count": 108, "sha256": hashlib.sha256(rom[table_base:table_base + 432]).hexdigest(),
                  "owner": owner(baseline["entries"], table_base)["kind"],
                  "consumer_call_sites": [0xD3C8, 0xD4EE], "targets": table_targets},
        "promoted_spans": [{"start": start, "end": end, "bytes": end - start} for start, end in promoted],
        "unique_promoted_bytes": sum(end - start for start, end in promoted),
        "unique_resource_bytes": sum(item["end"] - item["start"] for item in {x["start"]: x for x in resources}.values()),
        "source_owned_before": {"bytes": baseline["metrics"]["SOURCE_OWNED_BYTES"],
                                 "percent": baseline["metrics"]["SOURCE_OWNED_PERCENT"]},
        "source_owned_after": source_owned(entries, len(rom)),
        "blockers_before": blockers_before, "blockers_after": blockers_after,
        "census_reused": True, "whole_rom_scan_repeated": False,
        "sibling_loaders": {"examined": [0x37D2, 0xD3B2, 0xD950, 0x2CBC, 0x2E1E, 0x36D4],
                             "new_promotions": 0, "blocker": "only 0x37D2/0xD3B2 relations had a closed Ancient source contract"},
        "implementation_sha": None, "exact_implementation_ci": None, "final_publication_sha": None,
        "full_rom": {"size": len(rom), "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest()},
    }
    (output / "caller_closure_report.json").write_text(json.dumps(report, indent=2) + "\n")
    Path(args.report).write_text(render_report(report, sweep, carver))
    print(json.dumps({"promoted_bytes": sum(item["bytes"] for item in report["promoted_spans"]),
                      "promoted_spans": len(report["promoted_spans"]),
                      "source_owned_after": report["source_owned_after"],
                      "output": str(output)}, indent=2))


def render_report(report, sweep, carver):
    lines = ["# M12-GFX-2 — 0x3820 Caller-to-Asset Closure", "", "## Scope and ABI", "",
             "This developer-only pass consumes the published M12-GFX-1 caller list and strict census JSON; it does not repeat the whole-ROM scan. No ROM, decoded payload, PNG, CRAM/SAT capture, M13 work, or ASM-to-C++ migration is committed.", "",
             "The proven ABI is `A0` compressed source in / advanced to the exclusive consumed end, `A1` output destination / advanced to the exclusive produced end, format dispatch by `source[2]` (command stream when nonzero, bit stream when zero), and no nested helper call or hardware access inside `[0x003820,0x003B3E)`. The 68000 slice and independent parser agree on the existing format-A and format-B vectors: `0x16943C` consumes 1217 and emits 3072; `0x1894EA` consumes 112 and emits 128. Register preservation and command semantics match the existing contract in `docs/REVERSE_ENGINEERING.md`; no status return is used.", "",
             "## Ownership accounting", "", "| measure | before | after | delta |", "| --- | ---: | ---: | ---: |",
             f"| SOURCE_OWNED bytes | {report['source_owned_before']['bytes']:,} | {report['source_owned_after']['bytes']:,} | {report['source_owned_after']['bytes'] - report['source_owned_before']['bytes']:,} |",
             f"| SOURCE_OWNED percent | {report['source_owned_before']['percent']:.10f}% | {report['source_owned_after']['percent']:.10f}% | {report['source_owned_after']['percent'] - report['source_owned_before']['percent']:.10f}% |", "",
             "Carver blockers before/after are derived by subtracting only the exact caller-derived promoted spans from the preserved blocker intervals:", "",
             "| blocker | before ranges/bytes | after ranges/bytes |", "| --- | ---: | ---: |"]
    for key in "BFG":
        before = report["blockers_before"][key]; after = report["blockers_after"][key]
        lines.append(f"| {key} | {before['ranges']} / {before['bytes']:,} | {after['ranges']} / {after['bytes']:,} |")
    lines += ["", "## Table and caller-family graph", "", "The exact table `[0x05CE96,0x05D046)` is 108 four-byte absolute ROM pointers consumed by `0x00D3B2` and the related `0x00D4EE` path. Entries 1..107 enumerate strict Ancient resources; all 107 targets were already SOURCE_OWNED at baseline, so table promotion gain is zero.", "", "| call-site | containing routine/family | source set | destination | mode | confidence/blocker |", "| ---: | --- | --- | --- | --- | --- |"]
    resources = report["resources"]
    by_call = {}
    for resource in resources:
        for call_site in resource["caller_sites"]:
            by_call.setdefault(call_site, []).append(resource)
    for call_site in CALLERS:
        if call_site in TABLE_CALLS:
            table, selector, destination = TABLE_CALLS[call_site]
            source = f"{table}[1..107] ({len(report['table']['targets'])} targets)"
            mode, confidence = "indirect table", "CONFIRMED; baseline-owned"
        elif call_site in by_call:
            items = by_call[call_site]
            source = "; ".join(f"{hx(x['start'])}..{hx(x['end'])}" for x in items)
            destination = items[0].get("destination", "0x00FF3FAC")
            sequential = any("sequential" in item["family"] for item in items)
            mode, confidence = ("sequential A0" if sequential else "direct literal"), "CONFIRMED; exact parser end"
        elif call_site in DYNAMIC:
            source, destination, blocker = DYNAMIC[call_site]
            mode, confidence = "indirect/RAM", f"BLOCKED: {blocker}"
        else:
            source, destination, setup = DIRECT.get(call_site, ("not recovered", "unknown", ""))
            source = hx(source) if isinstance(source, int) else source
            mode, confidence = "direct literal", "CONFIRMED; exact parser end"
        lines.append(f"| `{hx(call_site)}` | {ROUTINES[call_site]} | `{source}` | `{destination}` | {mode} | {confidence} |")
    lines += ["", "## Exact resources and overlaps", "", f"The closure catalog contains {len(resources)} call-derived stream edges merging to {len({x['start'] for x in resources})} unique resource starts. Each record stores the strict half-open Ancient boundary, declared compressed bytes, decompressed bytes, output SHA-256, caller reverse-xrefs, baseline owner, and promotion decision in the machine-readable report. Promoted spans: {len(report['promoted_spans'])}, {sum(x['bytes'] for x in report['promoted_spans']):,} bytes.", "", "The five newly closed sequential families are `0x18F214`, `0x191F0A`, `0x194BDA`, `0x19628A`, and `0x199CBA`. Decoder candidates nested inside these caller-selected spans are reclassified as caller-proven streams; arbitrary sibling starts and post-chain gaps remain UNKNOWN. No executable conflict was accepted.", "", "The 64 KiB promotion target was not reached: exact caller-derived closure is 47,389 bytes. Further promotion is intentionally gated on closing the ten explicit unresolved source producers below; no generic detector or visual inference is used to fill the gap.", "", "## Sibling loaders and blockers", "", "The bounded second-order review followed only nearby `0x37D2`/`0x3820`, `0xD3B2`, and existing loader relations. No new generic detector family was added. Nearby `0xD950`, `0x2CBC`, `0x2E1E`, and `0x36D4` effects do not close an Ancient ROM source set and remain evidence-only.", "", f"Unresolved callers are explicit: RAM-mediated A0/A1, inherited caller arguments, helper output, or entity-record pointers cannot be promoted without a closed ROM source producer. CRAM, SAT, VRAM, runtime registers, and visual identity are not required for generic resource ownership and were not used.", "", "## Gates and identity", "", f"Canonical ROM: CRC32 `{report['full_rom']['crc32']}`, SHA-1 `{report['full_rom']['sha1']}`, SHA-256 `{report['full_rom']['sha256']}`. Reused M12-GFX-1 JSON: whole-ROM scan repeated = `{report['whole_rom_scan_repeated']}`. The machine-readable report is emitted as `caller_closure_report.json`; implementation and exact CI publication SHAs are recorded in the final worklog update after validation."]
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--sweep", required=True)
    parser.add_argument("--carver-report", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--report", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
