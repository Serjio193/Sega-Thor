"""Admit exact static entry proofs into an M14 canonical knowledge generation."""

from __future__ import annotations

import hashlib
import argparse
import json
import shutil
import sqlite3
import subprocess
import sys
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parents[1]
EVIDENCE = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
if str(EVIDENCE) not in sys.path:
    sys.path.insert(0, str(EVIDENCE))

import re_m12_auto_promote as stage7
from rom_knowledge_map import canonical, sha256_bytes, stable_id

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
SOURCE_OWNED = 1_487_672
RULE_ID = "M14_7_EXACT_STATIC_ENTRY_PROOF"
RULE_VERSION = "1"


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def _instruction(db: sqlite3.Connection, pc: int, rom: bytes) -> dict[str, Any]:
    row = db.execute("""SELECT o.object_id,o.attributes_json,r.start,r.end
        FROM rom_object o JOIN rom_range r USING(range_id)
        WHERE o.object_type='M68K_INSTRUCTION' AND r.start=? ORDER BY r.end""",
        (pc,)).fetchall()
    if len(row) != 1:
        raise ValueError("STOP_STATIC_XREF_INSTRUCTION_OBJECT_NOT_UNIQUE")
    item = row[0]
    attrs = json.loads(item[1])
    raw = rom[int(item[2]):int(item[3])]
    if int(item[2]) != pc or attrs.get("bytes_sha256") != sha256_bytes(raw) or \
            attrs.get("length") != len(raw) or attrs.get("opcode") != int.from_bytes(raw[:2], "big"):
        raise ValueError("STOP_STATIC_XREF_INSTRUCTION_BYTES_MISMATCH")
    return {"object_id": item[0], "start": int(item[2]), "end": int(item[3]),
            "attributes": attrs, "bytes": raw}


def _logical_snapshot(db: sqlite3.Connection) -> tuple[Any, ...]:
    tables = ("rom_range", "rom_object", "claim", "relation", "source_artifact",
        "evidence_ref", "derivation", "derivation_input", "emission")
    return tuple((table, tuple(tuple(row) for row in db.execute(
        f"SELECT * FROM {table} ORDER BY 1,2"))) for table in tables)


def _audit_emissions(db: sqlite3.Connection) -> dict[str, int]:
    rows = db.execute("SELECT start,end,emission_type,classification,source_owned FROM emission ORDER BY start,end").fetchall()
    cursor = 0
    for row in rows:
        if int(row[0]) != cursor or int(row[1]) <= int(row[0]):
            raise ValueError("STOP_M14_7_EMISSION_PARTITION_GAP_OR_OVERLAP")
        cursor = int(row[1])
    if cursor != ROM_SIZE:
        raise ValueError("STOP_M14_7_EMISSION_PARTITION_SIZE_MISMATCH")
    owned = sum(int(row[1])-int(row[0]) for row in rows if int(row[4]) == 1)
    unknown = [row for row in rows if row[2] == "INCBIN" and row[3] == "UNKNOWN"]
    return {"map_ranges": len(rows), "gaps": 0, "overlaps": 0,
        "source_owned": owned, "unknown_ranges": len(unknown),
        "unknown_bytes": sum(int(row[1])-int(row[0]) for row in unknown)}


def _emission_rows(db: sqlite3.Connection) -> tuple[tuple, ...]:
    return tuple(tuple(row) for row in db.execute("SELECT * FROM emission ORDER BY start,end"))


def _component_records(path: Path) -> list[dict[str, Any]]:
    rows = json.loads(path.read_text(encoding="utf-8"))["components"]
    if len(rows) != 5 or sum(int(row["size"]) for row in rows) != 260:
        raise ValueError("STOP_M14_7_COMPONENT_SET_MISMATCH")
    return rows


def _run_scanner(executable: Path, rom_path: Path, source_spans: Path,
                 component_spans: Path, output_dir: Path) -> tuple[list[dict], dict]:
    candidates_path = output_dir / "static-xrefs.tsv"
    summary_path = output_dir / "static-xrefs.json"
    result = subprocess.run([str(executable), str(rom_path), str(source_spans),
        str(component_spans), str(candidates_path), str(summary_path)],
        capture_output=True, text=True, check=False)
    if result.returncode:
        raise ValueError(f"STOP_STATIC_XREF_SCAN_FAILED:{result.stderr.strip()}")
    lines = candidates_path.read_text(encoding="utf-8").splitlines()
    header = lines[0].split("\t")
    candidates = [dict(zip(header, line.split("\t"))) for line in lines[1:]]
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    if summary["rom_sha256"] != ROM_SHA or summary["rom_size"] != ROM_SIZE:
        raise ValueError("STOP_STATIC_XREF_ROM_IDENTITY_MISMATCH")
    if any(row["stop_reason"] != "COMPLETE" for row in summary["ranges"]):
        raise ValueError("STOP_STATIC_XREF_SOURCE_RANGE_INCOMPLETE")
    return candidates, summary


def _admit(db: sqlite3.Connection, component: dict, candidate: dict,
           source: dict, target: dict, source_sha: str, scanner_sha: str) -> dict:
    if candidate["kind"] != "VERIFIED_FALLTHROUGH" or candidate["target_is_entry"] != "true":
        raise ValueError("STOP_STATIC_ENTRY_CANDIDATE_KIND_NOT_ADMISSIBLE")
    pc, target_pc = int(candidate["caller_pc"]), int(candidate["target_pc"])
    if source["end"] != target_pc or target["start"] != target_pc:
        raise ValueError("STOP_STATIC_ENTRY_TARGET_BOUNDARY_MISMATCH")
    proof = {"canonical_rom_sha256": ROM_SHA, "caller_object_id": source["object_id"],
        "caller_pc": pc, "caller_end": source["end"], "caller_bytes_sha256": sha256_bytes(source["bytes"]),
        "target_object_id": target["object_id"], "target_pc": target_pc,
        "component_end": int(component["end"]), "fallthrough_semantics": "instruction_returns_or_continues",
        "decoder": "existing M68K decoder; exact instruction boundaries", "decoder_source_sha256": scanner_sha,
        "source_span": [int(candidate["source_start"]), int(candidate["source_end"])],
        "truth": "STATIC_VERIFIED"}
    relation_id = stable_id("relation", {"relation_type": "STATIC_ENTRY_FALLTHROUGH",
        "source_object_id": source["object_id"], "target_object_id": target["object_id"],
        "target_address": target_pc, "status": "STATIC_VERIFIED", "attributes": proof})
    claim_value = {"proof_relation_id": relation_id, "proof": proof}
    claim_id = stable_id("claim", {"object_id": target["object_id"],
        "claim_type": "STATIC_VERIFIED_ENTRY", "value": claim_value,
        "status": "STATIC_VERIFIED"})
    artifact = db.execute("SELECT checkpoint,artifact_name,artifact_type FROM source_artifact WHERE source_sha256=?",
                          (ROM_SHA,)).fetchone()
    if artifact is None:
        db.execute("INSERT INTO source_artifact VALUES (?,?,?,?)",
            (ROM_SHA, "M14-7", "Beyond Oasis (USA).md", "CANONICAL_ROM_BYTES"))
    elif artifact[2] != "ROM_IDENTITY":
        raise ValueError("STOP_STATIC_ENTRY_ROM_SOURCE_ARTIFACT_CONFLICT")
    db.execute("INSERT OR IGNORE INTO relation VALUES (?,?,?,?,?,?,?)",
        (relation_id, "STATIC_ENTRY_FALLTHROUGH", source["object_id"],
         target["object_id"], target_pc, "STATIC_VERIFIED", canonical(proof)))
    db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (claim_id,
        target["object_id"], "STATIC_VERIFIED_ENTRY", canonical(claim_value), "STATIC_VERIFIED"))
    evidence_ids=[]
    for subject_type, subject_id, kind, locator in (
        ("RELATION", relation_id, "CANONICAL_STATIC_FALLTHROUGH", proof),
        ("CLAIM", claim_id, "STATIC_ENTRY_PROOF", proof)):
        ref_id = stable_id("evidence", {"subject_type": subject_type,
            "subject_id": subject_id, "source_sha256": source_sha, "fact_kind": kind,
            "fact_count": 1, "locator": locator})
        db.execute("INSERT OR IGNORE INTO evidence_ref VALUES (?,?,?,?,?,?,?)",
            (ref_id, subject_type, subject_id, source_sha, kind, 1, canonical(locator)))
        evidence_ids.append(ref_id)
    inputs = [("ROM_OBJECT", source["object_id"], "EXACT_PREDECESSOR_INSTRUCTION"),
        ("ROM_OBJECT", target["object_id"], "EXACT_ENTRY_OBJECT"),
        ("EMISSION", f"{candidate['source_start']}:{candidate['source_end']}", "VERIFIED_SOURCE_EMISSION"),
        ("SOURCE_ARTIFACT", ROM_SHA, "CANONICAL_ROM_BYTES")]
    derivation_ids=[]
    for output_type, output_id, result in (("RELATION", relation_id, proof),
                                           ("CLAIM", claim_id, claim_value)):
        payload = {"rule_id": RULE_ID, "rule_version": RULE_VERSION,
            "implementation_hash": scanner_sha, "parameters_hash": sha256_bytes(canonical(proof).encode()),
            "inputs": sorted([{"subject_type": a, "subject_id": b, "role": c} for a,b,c in inputs], key=canonical),
            "output_type": output_type, "output_id": output_id,
            "result": result, "assumptions": []}
        derivation_id = stable_id("derivation", payload)
        db.execute("INSERT OR IGNORE INTO derivation VALUES (?,?,?,?,?,?,?,?,?)",
            (derivation_id, RULE_ID, RULE_VERSION, scanner_sha, payload["parameters_hash"],
             output_type.lower(), output_id, canonical(result), "[]"))
        derivation_ids.append(derivation_id)
        for ordinal, item in enumerate(payload["inputs"]):
            db.execute("INSERT OR IGNORE INTO derivation_input VALUES (?,?,?,?,?)",
                (derivation_id, ordinal, item["subject_type"], item["subject_id"], item["role"]))
    return {"relation_id": relation_id, "claim_id": claim_id,
            "caller_range": [source["start"], source["end"]],
            "opcode": source["attributes"]["opcode"],
            "instruction_bytes_sha256": sha256_bytes(source["bytes"]),
            "mnemonic": candidate.get("mnemonic"), "flow": candidate.get("flow"),
            "call_type": "CALL_RETURN_FALLTHROUGH" if candidate.get("flow") == "direct_call" else "SEQUENTIAL_FALLTHROUGH",
            "caller_truth": "STATIC_VERIFIED_SOURCE_OWNED_ASM",
            "evidence_refs": evidence_ids, "derivation_ids": derivation_ids, **proof}


def run(*, parent_db: Path, components_json: Path, rom_path: Path,
        scanner_executable: Path, source_spans: Path, component_spans: Path,
        output_dir: Path, replay_dir: Path) -> dict:
    components = _component_records(components_json)
    output_dir.mkdir(parents=True, exist_ok=True)
    replay_dir.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or sha256_bytes(rom) != ROM_SHA:
        raise ValueError("STOP_M14_7_CANONICAL_ROM_MISMATCH")
    scanner_sha = _sha(scanner_executable)
    candidates, scan_summary = _run_scanner(scanner_executable, rom_path,
        source_spans, component_spans, output_dir)
    replay_candidates, replay_summary = _run_scanner(scanner_executable, rom_path,
        source_spans, component_spans, replay_dir)
    if candidates != replay_candidates or scan_summary != replay_summary:
        raise ValueError("STOP_M14_7_STATIC_XREF_REPLAY_MISMATCH")
    base = sqlite3.connect(parent_db); base.row_factory = sqlite3.Row
    meta = dict(base.execute("SELECT key,value FROM map_meta"))
    if meta.get("rom_sha256") != ROM_SHA or meta.get("generation_id") != "gen-m14-6-dcb2596abccf9612":
        raise ValueError("STOP_M14_7_CANONICAL_PARENT_MISMATCH")
    emission_audit = _audit_emissions(base)
    accepted_emissions = _emission_rows(base)
    ownership = emission_audit["source_owned"]
    if ownership != SOURCE_OWNED or emission_audit["map_ranges"] != 2490 or \
            emission_audit["unknown_ranges"] != 764 or emission_audit["unknown_bytes"] != 1657796:
        raise ValueError("STOP_M14_7_BASELINE_ACCOUNTING_MISMATCH")
    before=[]; after=[]; admitted=[]
    for idx, target_component in enumerate(components):
        target_pc=int(target_component["start"])
        entry=_instruction(base,target_pc,rom)
        incoming=base.execute("SELECT relation_type,status,source_object_id FROM relation WHERE target_object_id=? ORDER BY relation_type,relation_id",(entry["object_id"],)).fetchall()
        outgoing=base.execute("SELECT relation_type,status,target_object_id,target_address FROM relation WHERE source_object_id=? ORDER BY relation_type,relation_id",(entry["object_id"],)).fetchall()
        claims=base.execute("SELECT claim_type,status FROM claim WHERE object_id=? ORDER BY claim_type,claim_id",(entry["object_id"],)).fetchall()
        count=base.execute("SELECT COUNT(*) FROM relation WHERE target_object_id=? AND relation_type IN ('DIRECT_CALL_TARGET','STATIC_CALLER') AND status='STATIC_VERIFIED'",(entry["object_id"],)).fetchone()[0]
        before.append(bool(stage7.selected([{"start":target_pc,"end":target_component["end"],"exact":True,"called_by":int(count)}],{"records":[]})[0]))
        after.append(before[-1])
        target_component["_entry"] = entry
        target_component["_incoming"] = [dict(r) for r in incoming]
        target_component["_outgoing"] = [dict(r) for r in outgoing]
        target_component["_claims"] = [dict(r) for r in claims]
    scanner_candidates=[]
    for row in candidates:
        component=next(c for c in components if int(row["component_start"])==int(c["start"]))
        pc=int(row["caller_pc"]); src=_instruction(base,pc,rom)
        if src["end"] != int(row["caller_end"]): raise ValueError("STOP_STATIC_XREF_CALLER_EXTENT_MISMATCH")
        if not (int(row["source_start"]) <= pc < int(row["source_end"])): raise ValueError("STOP_STATIC_XREF_CALLER_OUTSIDE_VERIFIED_ASM")
        exact_entry=int(row["target_pc"])==int(component["start"]) and row["target_is_entry"]=="true"
        target=component["_entry"] if exact_entry else None
        item={"component":component,"row":row,"source":src,"target":target,"exact_entry":exact_entry}
        scanner_candidates.append(item)
    # Each replay writes into a separate child copied from the accepted parent.
    child_paths=[output_dir/"m14-7-generation.sqlite", replay_dir/"m14-7-generation.sqlite"]
    for path in child_paths:
        shutil.copy2(parent_db,path)
    scanner_candidates.sort(key=lambda item:(int(item["row"]["component_start"]),int(item["row"]["caller_pc"]),item["row"]["kind"]))
    for child_index, path in enumerate(child_paths):
        db=sqlite3.connect(path); db.execute("PRAGMA foreign_keys=ON")
        for item in scanner_candidates:
            if item["exact_entry"] and item["row"]["kind"]=="VERIFIED_FALLTHROUGH":
                proof = _admit(db,item["component"],item["row"],item["source"],item["target"],ROM_SHA,scanner_sha)
                if child_index == 0:
                    admitted.append(proof)
        digest_payload = []
        for table in ("rom_range", "rom_object", "claim", "relation", "source_artifact",
                      "evidence_ref", "derivation", "derivation_input", "emission"):
            digest_payload.append((table, [list(row) for row in db.execute(
                f"SELECT * FROM {table} ORDER BY 1,2")]))
        graph_digest = sha256_bytes(canonical(digest_payload).encode("utf-8"))
        child_generation = "gen-m14-7-" + graph_digest[:16]
        db.execute("INSERT OR REPLACE INTO map_meta VALUES ('parent_generation_id',?)", (meta["generation_id"],))
        db.execute("INSERT OR REPLACE INTO map_meta VALUES ('generation_id',?)", (child_generation,))
        db.commit(); db.close()
    if _logical_snapshot(sqlite3.connect(child_paths[0])) != _logical_snapshot(sqlite3.connect(child_paths[1])):
        raise ValueError("STOP_M14_7_GRAPH_ADMISSION_REPLAY_MISMATCH")
    # Reopen the admitted child and replay the unchanged Stage7 selector.
    child=sqlite3.connect(child_paths[0]); child.row_factory=sqlite3.Row
    for i,component in enumerate(components):
        entry=component["_entry"]
        count=child.execute("SELECT COUNT(*) FROM relation WHERE target_object_id=? AND relation_type IN ('DIRECT_CALL_TARGET','STATIC_CALLER') AND status='STATIC_VERIFIED'",(entry["object_id"],)).fetchone()[0]
        after[i]=bool(stage7.selected([{"start":int(component["start"]),"end":int(component["end"]),"exact":True,"called_by":int(count)}],{"records":[]})[0])
    source_owned_after=int(child.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0])
    if _emission_rows(child) != accepted_emissions:
        raise ValueError("STOP_M14_7_CANONICAL_EMISSION_CHANGED")
    report={"schema":"oasis.m14.7.static-entry-proof.v1","status":"PASS_STATIC_ENTRY_STAGE7_READINESS_V1",
        "rom_sha256":ROM_SHA,"rom_size":ROM_SIZE,"parent_generation":meta["generation_id"],
        "generation_id":child_generation,
        "components_audited":len(components),"bytes_audited":sum(int(c["size"]) for c in components),
        "decoded_instructions":scan_summary["decoded_instruction_count"],"decoded_bytes":scan_summary["decoded_byte_count"],
        "static_xref_candidates":len(candidates),"static_xrefs_accepted":len(admitted),
        "new_static_verified_entries":len(admitted),"new_static_verified_callers":0,
        "static_verified_components":len({x["target_pc"] for x in admitted}),
        "static_verified_bytes":sum(int(c["size"]) for c in components if int(c["start"]) in {x["target_pc"] for x in admitted}),
        "stage7_selector_before":before,"stage7_selector_after":after,
        "stage7_eligible_components":sum(after),"stage7_eligible_bytes":sum(int(c["size"]) for c,ok in zip(components,after) if ok),
        "manifest_regenerated":False,"promoted_components":0,"promoted_bytes":0,
        "source_owned_before":ownership,"source_owned_after":source_owned_after,"source_owned_delta":source_owned_after-ownership,
        "map_ranges":emission_audit["map_ranges"],"map_gaps":emission_audit["gaps"],
        "map_overlaps":emission_audit["overlaps"],"unknown_ranges":emission_audit["unknown_ranges"],
        "unknown_bytes":emission_audit["unknown_bytes"],
        "pointer_refs":"NOT_SCANNED_NO_TYPED_TABLE_ENTRY_OBJECTS",
        "scan":scan_summary,"candidates":candidates,"accepted":admitted,
        "components":[{"start":int(c["start"]),"end":int(c["end"]),"size":int(c["size"]),
            "cfg_sha256":c["cfg_sha256"],"roundtrip_sha256":c["roundtrip_sha256"],
            "incoming_references":c["_incoming"],"outgoing_references":c["_outgoing"],
            "existing_claims":c["_claims"],
            "runtime_entry_observations":sum(1 for r in c["_incoming"] if r["status"]=="OBSERVED_RUNTIME"),
            "static_entry_before":any(claim["claim_type"]=="STATIC_VERIFIED_ENTRY" and claim["status"]=="STATIC_VERIFIED" for claim in c["_claims"]),
            "static_entry_after":int(c["start"]) in {x["target_pc"] for x in admitted},
            "selector_before":before[i],"selector_after":after[i],
            "blocker":None if after[i] else "NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR"} for i,c in enumerate(components)],
        "deterministic_replay":True,"logical_graph_replay_identical":True}
    if report["bytes_audited"] != 260 or report["source_owned_delta"] != 0 or any(after):
        if report["source_owned_delta"] != 0: raise ValueError("STOP_M14_7_SOURCE_OWNED_CHANGED")
    (output_dir/"report.json").write_text(canonical(report)+"\n",encoding="utf-8",newline="\n")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("parent-db", "components-json", "rom", "scanner", "source-spans",
                 "component-spans", "output-dir", "replay-dir"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    report = run(parent_db=args.parent_db, components_json=args.components_json,
        rom_path=args.rom, scanner_executable=args.scanner,
        source_spans=args.source_spans, component_spans=args.component_spans,
        output_dir=args.output_dir, replay_dir=args.replay_dir)
    print(f"{report['status']} entries={report['new_static_verified_entries']} "
          f"stage7_eligible={report['stage7_eligible_components']} "
          f"source_owned={report['source_owned_after']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
