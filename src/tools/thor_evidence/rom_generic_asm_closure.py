"""M14.4 exact-seed CFG closure and vasm roundtrip campaign."""

from __future__ import annotations

import argparse
import shutil
import hashlib
import json
from pathlib import Path
import subprocess

from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id
from rom_knowledge_fusion import logical_graph_hash
from rom_unknown_range_campaign import rank_unknown_ranges

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728


class CanonicalRomProvider:
    """Read-only, identity-verified file with bounded random access."""

    def __init__(self, path: Path):
        self.path = path.resolve()
        digest = hashlib.sha256()
        size = 0
        with self.path.open("rb") as source:
            while chunk := source.read(1024 * 1024):
                digest.update(chunk)
                size += len(chunk)
        if size != ROM_SIZE:
            raise ValueError("STOP_CANONICAL_ROM_SIZE_MISMATCH")
        if digest.hexdigest() != ROM_SHA256:
            raise ValueError("STOP_CANONICAL_ROM_SHA256_MISMATCH")
        self.size = size

    def read(self, offset: int, length: int) -> bytes:
        if offset < 0 or length < 0 or offset + length > self.size:
            raise ValueError("STOP_CANONICAL_ROM_READ_OUT_OF_BOUNDS")
        with self.path.open("rb") as source:
            source.seek(offset)
            data = source.read(length)
        if len(data) != length:
            raise OSError("canonical ROM bounded read was short")
        return data


def exact_cfg_seeds(store: KnowledgeStore, start: int, end: int) -> list[dict]:
    rows = store.db.execute("""
        SELECT r.start AS pc, c.object_id, c.claim_id, c.claim_type AS seed_kind,
               e.ref_id, e.source_sha256, e.fact_kind
        FROM claim c JOIN rom_object o ON o.object_id=c.object_id
        JOIN rom_range r ON r.range_id=o.range_id
        LEFT JOIN evidence_ref e ON e.subject_type='CLAIM' AND e.subject_id=c.claim_id
        WHERE ((c.claim_type='EXECUTED_FROM_ROM' AND c.status='OBSERVED_RUNTIME')
          OR (c.claim_type='GLOBAL_EVIDENCE_REFERENCE' AND c.status='DERIVED_EXACT')
          OR (c.claim_type='RECONSTRUCTION_VERIFIED' AND c.status='STATIC_VERIFIED'))
          AND o.object_type='M68K_INSTRUCTION' AND r.start>=? AND r.start<?
        ORDER BY r.start,c.claim_id,e.ref_id
    """, (start, end)).fetchall()
    grouped: dict[int, dict] = {}

    def add_seed(pc: int, *, object_id: str | None = None, claim_id: str | None = None,
                 relation_id: str | None = None, seed_kind: str,
                 evidence_ref: dict | None = None) -> None:
        seed = grouped.setdefault(pc, {"pc": pc, "object_ids": set(),
            "claim_ids": set(), "relation_ids": set(), "seed_kinds": set(),
            "evidence_refs": {}})
        if object_id:
            seed["object_ids"].add(object_id)
        if claim_id:
            seed["claim_ids"].add(claim_id)
        if relation_id:
            seed["relation_ids"].add(relation_id)
        seed["seed_kinds"].add(seed_kind)
        if evidence_ref:
            seed["evidence_refs"][evidence_ref["ref_id"]] = evidence_ref

    for row in rows:
        pc = int(row["pc"])
        ref = None
        if row["ref_id"]:
            ref = {"ref_id": str(row["ref_id"]),
                "source_sha256": str(row["source_sha256"]),
                "fact_kind": str(row["fact_kind"])}
        add_seed(pc, object_id=str(row["object_id"]), claim_id=str(row["claim_id"]),
                 seed_kind=str(row["seed_kind"]), evidence_ref=ref)

    relation_rows = store.db.execute("""
        SELECT COALESCE(target_range.start,r.target_address) AS pc,
               target.object_id AS target_object_id,r.relation_id,r.relation_type,
               e.ref_id,e.source_sha256,e.fact_kind
        FROM relation r JOIN rom_object source ON source.object_id=r.source_object_id
        LEFT JOIN rom_object target ON target.object_id=r.target_object_id
        LEFT JOIN rom_range target_range ON target_range.range_id=target.range_id
        LEFT JOIN evidence_ref e ON e.subject_type='RELATION' AND e.subject_id=r.relation_id
        WHERE source.object_type='M68K_INSTRUCTION'
          AND r.relation_type IN ('DIRECT_BRANCH_TARGET','DIRECT_CALL_TARGET',
              'CANONICAL_REFERENCE','ASM_CFG_EDGE','EXACT_CFG_EDGE')
          AND r.status IN ('DERIVED_EXACT','STATIC_VERIFIED')
          AND (target.object_type='M68K_INSTRUCTION' OR r.target_address IS NOT NULL)
          AND COALESCE(target_range.start,r.target_address)>=?
          AND COALESCE(target_range.start,r.target_address)<?
        ORDER BY pc,r.relation_id,e.ref_id
    """, (start, end)).fetchall()
    for row in relation_rows:
        ref = None
        if row["ref_id"]:
            ref = {"ref_id": str(row["ref_id"]),
                "source_sha256": str(row["source_sha256"]),
                "fact_kind": str(row["fact_kind"])}
        add_seed(int(row["pc"]), object_id=row["target_object_id"],
            relation_id=str(row["relation_id"]), seed_kind=str(row["relation_type"]),
            evidence_ref=ref)
    return [{**item, "object_ids": sorted(item["object_ids"]),
             "claim_ids": sorted(item["claim_ids"]),
             "relation_ids": sorted(item["relation_ids"]),
             "seed_kinds": sorted(item["seed_kinds"]),
             "evidence_refs": sorted(item["evidence_refs"].values(), key=lambda x: x["ref_id"])}
            for _, item in sorted(grouped.items()) if item["evidence_refs"]]


def candidate_ranges(store: KnowledgeStore) -> list[dict]:
    return [row for row in rank_unknown_ranges(store)
            if row["exact_blocker"] == "NO_CLOSED_CFG_OR_ROM_BYTE_ROUNDTRIP"
            and row["executed_pc_count"] > 0]


def canonical_partition_audit(store: KnowledgeStore) -> dict:
    cursor = 0
    gaps = 0
    overlaps = 0
    total = 0
    for row in store.db.execute("SELECT start,end FROM emission ORDER BY start,end"):
        start, end = int(row["start"]), int(row["end"])
        if start < cursor:
            overlaps += 1
        elif start > cursor:
            gaps += 1
        cursor = max(cursor, end)
        total += end - start
    if cursor != ROM_SIZE:
        gaps += 1
    metrics = store.metrics()
    return {"ranges": int(store.db.execute("SELECT COUNT(*) FROM emission").fetchone()[0]),
        "rom_bytes": total, "gaps": gaps, "overlaps": overlaps,
        "source_owned_bytes": metrics["source_owned_bytes"],
        "unknown_ranges": int(store.db.execute("SELECT COUNT(*) FROM emission WHERE "
            "source_kind IN ('UNKNOWN','UNKNOWN_DATA','UNKNOWN_WITH_EVIDENCE')").fetchone()[0]),
        "unknown_bytes": metrics["unknown_bytes"]}


def write_tsv(path: Path, rows: list[str]) -> None:
    path.write_text("\n".join(rows) + "\n", encoding="ascii")


def run_one(*, provider: CanonicalRomProvider, store: KnowledgeStore,
            candidate: dict, tool: Path, assembler: Path, output: Path,
            known_ranges: list[tuple[int, int]]) -> dict:
    start, end = int(candidate["rom_start"]), int(candidate["rom_end"])
    seeds = exact_cfg_seeds(store, start, end)
    work = output / f"{start:06X}-{end:06X}"
    work.mkdir(parents=True, exist_ok=False)
    write_tsv(work / "seeds.tsv", [f"{seed['pc']}\t" + ",".join(
        seed["claim_ids"] + seed["relation_ids"]) for seed in seeds])
    write_tsv(work / "known-code.tsv", [str(address) for address in known_ranges])
    completed = subprocess.run([str(tool), str(provider.path), hex(start), hex(end),
        str(work / "seeds.tsv"), str(work / "known-code.tsv"), str(work / "decode")],
        check=False, capture_output=True, text=True)
    if completed.returncode not in (0, 1):
        raise RuntimeError(completed.stdout + completed.stderr)
    cfg = json.loads((work / "decode" / "cfg.json").read_text(encoding="utf-8"))
    roundtrips = []
    for extent in cfg["extents"]:
        source = work / "decode" / extent["asm"]
        binary = source.with_suffix(".bin")
        assembled = subprocess.run([str(assembler), "-m68000", "-no-opt", "-Fbin",
            "-o", str(binary), str(source)], check=False, capture_output=True, text=True)
        result = {"start": extent["start"], "end": extent["end"],
                  "status": "ASSEMBLER_ERROR" if assembled.returncode else "ROUNDTRIP_MISMATCH"}
        if assembled.returncode == 0:
            actual = binary.read_bytes()
            expected = provider.read(extent["start"], extent["size"])
            mismatch = next((i for i, pair in enumerate(zip(expected, actual))
                             if pair[0] != pair[1]), min(len(expected), len(actual)))
            if actual == expected:
                result.update(status="EXACT", size=len(actual), sha256=sha256_bytes(actual),
                              first_mismatch=None)
            else:
                result.update(size=len(actual), first_mismatch=extent["start"] + mismatch)
        roundtrips.append(result)
    seed_payload = [{**seed, "pc": seed["pc"]} for seed in seeds]
    (work / "seeds.json").write_text(canonical(seed_payload) + "\n", encoding="utf-8")
    closed = cfg["closed_cfg"] and all(item["status"] == "EXACT" for item in roundtrips)
    blockers = list(cfg["blockers"])
    for item in roundtrips:
        if item["status"] == "ASSEMBLER_ERROR":
            blockers.append("ASSEMBLER_ERROR")
        elif item["status"] == "ROUNDTRIP_MISMATCH":
            blockers.append("ROUNDTRIP_MISMATCH")
    exact_proofs = []
    if closed:
        for extent, roundtrip in zip(cfg["extents"], roundtrips):
            decoded_extent = json.loads((work / "decode" / extent["slice"]).read_text(encoding="utf-8"))
            references = [{"source_pc": instruction["address"],
                "target_pc": instruction["branch_target"],
                "instruction_operation": instruction["operation"]}
                for instruction in decoded_extent["instructions"]
                if instruction["branch_target"] is not None]
            exact_proofs.append({"start": extent["start"], "end": extent["end"],
                "cfg_sha256": sha256_bytes(canonical(cfg).encode()),
                "roundtrip_sha256": roundtrip["sha256"],
                "references": references,
                "proof_refs": sorted({ref["ref_id"] for seed in seeds
                    for ref in seed["evidence_refs"]})})
    return {"start": start, "end": end, "size": end-start,
        "seed_count": len(seeds), "seeds_sha256": sha256_bytes(canonical(seed_payload).encode()),
        "seed_provenance": seed_payload,
        "instruction_count": cfg["instruction_count"], "decoded_bytes": cfg["decoded_bytes"],
        "connected_components": cfg["connected_components"],
        "closed_cfg": cfg["closed_cfg"], "roundtrips": roundtrips,
        "closed_and_roundtrip_exact": closed, "blockers": sorted(set(blockers)),
        "cfg_sha256": sha256_bytes(canonical(cfg).encode()),
        "exact_extent_proofs": exact_proofs}


def persist_proposals(database: Path, output: Path, results: list[dict],
                      graph_hash: str, map_hash: str, emission_hash: str,
                      generation_id: str) -> dict:
    operations = []
    for result in results:
        for proof in result["exact_extent_proofs"]:
            common = {"base_generation": generation_id, "base_map_hash": map_hash,
                "base_emission_hash": emission_hash, "graph_hash": graph_hash,
                "cfg_proof": proof["cfg_sha256"],
                "roundtrip_proof": proof["roundtrip_sha256"],
                "proof_refs": proof["proof_refs"],
                "analysis_window": [result["start"], result["end"]]}
            operations.extend([
                {"operation": "SPLIT_RANGE", "start": proof["start"],
                 "end": proof["end"], **common},
                {"operation": "ADD_BOUNDARY", "address": proof["start"], **common},
                {"operation": "ADD_BOUNDARY", "address": proof["end"], **common},
                {"operation": "CLASSIFY_RANGE", "start": proof["start"],
                 "end": proof["end"], "emission_type": "ASM", **common}])
            operations.extend({"operation": "ADD_REFERENCE", **reference,
                "reference_kind": "M68K_CONTROL_FLOW", **common}
                for reference in proof["references"])
    if not operations:
        return {"proposal_id": None, "operation_count": 0,
                "database": None, "proposal_hash": None}
    child_path = output / "proposal-child.sqlite"
    shutil.copy2(database, child_path)
    child = KnowledgeStore(child_path, ROM_SHA256, ROM_SIZE)
    try:
        proposal_id = stable_id("m14-4-proposal", {"generation": generation_id,
            "map_hash": map_hash, "graph_hash": graph_hash, "operations": operations})
        proposal_hash = child.create_map_proposal(proposal_id, generation_id,
            map_hash, graph_hash, "M14.4_CLOSED_CFG_ROUNDTRIP_V1", operations)
        child.db.commit()
        child.validate_map_proposal_parent(proposal_id, generation_id)
        if child.hashes()["map_hash"] != map_hash or child.hashes()["emission_hash"] != emission_hash:
            raise ValueError("STOP_PROPOSAL_CHANGED_CANONICAL_MAP")
        return {"proposal_id": proposal_id, "operation_count": len(operations),
            "database": child_path.name, "proposal_hash": proposal_hash}
    finally:
        child.db.close()


def campaign(database: Path, rom_path: Path, tool: Path, assembler: Path,
             output: Path) -> dict:
    provider = CanonicalRomProvider(rom_path)
    store = KnowledgeStore(database, ROM_SHA256, ROM_SIZE, read_only=True)
    try:
        rankings = candidate_ranges(store)
        if len(rankings) != 27:
            raise ValueError(f"STOP_EXPECTED_27_CODE_BLOCKED_RANGES_GOT_{len(rankings)}")
        known_ranges = [int(r[0]) for r in store.db.execute("""
            SELECT r.start FROM rom_object o JOIN rom_range r USING(range_id)
            JOIN emission e ON e.emission_type='ASM' AND r.start>=e.start AND r.end<=e.end
            WHERE o.object_type='M68K_INSTRUCTION' ORDER BY r.start
        """)]
        output.mkdir(parents=True, exist_ok=False)
        primary = next(row for row in rankings if (row["rom_start"], row["rom_end"]) ==
                       (0x061588, 0x061CD2))
        ordered = [primary] + [row for row in rankings if row is not primary]
        results = [run_one(provider=provider, store=store, candidate=item, tool=tool,
                           assembler=assembler, output=output, known_ranges=known_ranges)
                   for item in ordered]
        graph_hash = logical_graph_hash(store)
        map_hash = store.hashes()["map_hash"]
        emission_hash = store.hashes()["emission_hash"]
        generation_id = store.meta()["generation_id"]
        proposal = persist_proposals(database, output, results, graph_hash,
                                     map_hash, emission_hash, generation_id)
        exact_extents = [proof for result in results for proof in result["exact_extent_proofs"]]
        proposed_references = sum(len(proof["references"]) for proof in exact_extents)
        unknown_rows = store.db.execute("SELECT COUNT(*),COALESCE(SUM(end-start),0) FROM emission "
            "WHERE source_kind IN ('UNKNOWN','UNKNOWN_DATA','UNKNOWN_WITH_EVIDENCE')").fetchone()
        partition = canonical_partition_audit(store)
        return {"schema": "oasis.m14.4.generic-asm-closure.v1",
            "rom_sha256": ROM_SHA256, "rom_size": ROM_SIZE,
            "generation_id": generation_id,
            "map_hash": map_hash,
            "emission_hash": emission_hash,
            "graph_hash": graph_hash,
            "canonical_partition": partition,
            "ranges_attempted": len(results), "primary": results[0], "ranges": results,
            "ranges_with_progress": sum(item["decoded_bytes"] > 0 for item in results),
            "closed_cfg_components": sum(item["connected_components"] for item in results
                                          if item["closed_cfg"]),
            "unresolved_components": sum(item["connected_components"] for item in results
                                          if not item["closed_cfg"]),
            "roundtrip_exact_components": sum(item["connected_components"] for item in results
                                               if item["closed_and_roundtrip_exact"]),
            "proposed_asm_ranges": len(exact_extents),
            "proposed_asm_bytes": sum(item["end"] - item["start"] for item in exact_extents),
            "proposed_boundaries": 2 * len(exact_extents),
            "proposed_references": proposed_references, "proposed_symbols": 0,
            "new_asm_ranges": 0, "new_asm_bytes": 0, "new_boundaries": 0,
            "unknown_ranges_before": int(unknown_rows[0]),
            "unknown_ranges_after": int(unknown_rows[0]),
            "unknown_bytes_before": int(unknown_rows[1]),
            "unknown_bytes_after": int(unknown_rows[1]),
            "new_source_owned_bytes": 0,
            "map_proposal": proposal,
            "source_owned_before": store.metrics()["source_owned_bytes"],
            "source_owned_after": store.metrics()["source_owned_bytes"],
            "source_owned_delta": 0}
    finally:
        store.db.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--tool", required=True, type=Path)
    parser.add_argument("--assembler", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--verify-replay", action="store_true",
        help="rerun the full campaign in a sibling output directory and require identical receipts")
    args = parser.parse_args()
    result = campaign(args.database, args.rom, args.tool, args.assembler, args.output)
    if args.verify_replay:
        replay_output = args.output.with_name(args.output.name + "-replay")
        replay = campaign(args.database, args.rom, args.tool, args.assembler, replay_output)
        if replay != result:
            raise ValueError("STOP_NONDETERMINISTIC_M14_4_REPLAY")
        partition = result["canonical_partition"]
        if (partition["ranges"] != 2489 or partition["rom_bytes"] != ROM_SIZE or
                partition["gaps"] or partition["overlaps"] or
                partition["source_owned_bytes"] != 1_487_672 or
                result["source_owned_delta"] != 0 or result["ranges_attempted"] != 27 or
                result["roundtrip_exact_components"] != result["proposed_asm_ranges"] or
                result["map_proposal"]["operation_count"] !=
                    4 * result["proposed_asm_ranges"] + result["proposed_references"]):
            raise ValueError("STOP_M14_4_ACCEPTANCE_INVARIANT")
        result["deterministic_replay"] = "PASS"
        result["acceptance_status"] = "PASS_CANONICAL_ROM_GENERIC_ASM_CLOSURE_V1"
    else:
        result["deterministic_replay"] = "NOT_RUN"
        result["acceptance_status"] = "REPLAY_REQUIRED_FOR_PASS"
    payload = canonical(result) + "\n"
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(payload, encoding="utf-8")
    print(payload, end="")


if __name__ == "__main__":
    main()
