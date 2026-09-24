"""M14.2B scoped RAM-shadow, DMA, and SAT evidence fusion."""

from __future__ import annotations

import hashlib
import json
import re
import shutil
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id
    from .rom_knowledge_map import runtime_occurrence_id
    from .rom_knowledge_fusion_stream import iter_target_instructions
    from .rom_knowledge_fusion_query import logical_graph_hash, witness_path
except ImportError:
    from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id
    from rom_knowledge_map import runtime_occurrence_id
    from rom_knowledge_fusion_stream import iter_target_instructions
    from rom_knowledge_fusion_query import logical_graph_hash, witness_path


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
EMISSION_SHA = "44a2332b0b433c635e33767886ffff35985ad31131e3dc3b4dea5e6984b17d92"
ROM_BYTES = 3_145_728
SOURCE_OWNED = 1_487_672
RULE = "M14_2B_RAM_SHADOW_DMA_SAT_JOIN"
RULE_VERSION = "1"


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def _addr(value: Any) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def _load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("STOP_FUSION_ARTIFACT_INVALID:" + path.name)
    return value


def _tail_string(path: Path, key: str) -> str:
    import re
    with path.open("rb") as stream:
        stream.seek(max(0, path.stat().st_size - (1 << 18)))
        tail = stream.read().decode("utf-8", "strict")
    match = re.search(r'"' + re.escape(key) + r'"\s*:\s*"([^"]*)"', tail)
    if match is None:
        raise ValueError("STOP_FUSION_CORPUS_TAIL_MISSING:" + key)
    return match.group(1)


def _corpus_string(path: Path, key: str) -> str:
    with path.open("rb") as stream:
        head = stream.read(1 << 18).decode("utf-8", "strict")
    match = re.search(r'"' + re.escape(key) + r'"\s*:\s*"([^"]*)"', head)
    return match.group(1) if match else _tail_string(path, key)


def _rom_object(store: KnowledgeStore, pc: int) -> str:
    row = store.db.execute("""SELECT o.object_id FROM rom_object o JOIN rom_range r USING(range_id)
      WHERE r.rom_sha256=? AND r.start<=? AND ?<r.end
      ORDER BY CASE o.object_type WHEN 'M68K_INSTRUCTION' THEN 0 ELSE 1 END,
      r.end-r.start,o.object_id LIMIT 1""", (ROM_SHA, pc, pc)).fetchone()
    if row is None:
        raise ValueError(f"STOP_FUSION_ROM_PC_UNMAPPED:{pc:#x}")
    return str(row[0])


def _relation(kind: str, source: str, target: str | None,
              address: int | None, status: str, attrs: dict[str, Any]) -> dict[str, Any]:
    payload = {"relation_type": kind, "source_object_id": source,
        "target_object_id": target, "target_address": address,
        "status": status, "attributes": attrs}
    return {"relation_id": stable_id("relation", payload),
        "relation_type": kind, "source_object_id": source,
        "target_object_id": target, "target_address": address,
        "status": status, "attributes_json": canonical(attrs)}


def _artifact_rows(paths: dict[str, Path], schemas: dict[str, str]) -> tuple[list[dict[str, str]], dict[str, str]]:
    rows, hashes = [], {}
    for key, path in sorted(paths.items()):
        digest = _sha(path)
        hashes[key] = digest
        rows.append({"source_sha256": digest, "checkpoint": schemas[key],
            "artifact_name": path.name, "artifact_type": "M14_2B:" + key.upper()})
    return rows, hashes


def _evidence(subject_type: str, subject_id: str, digest: str,
              analyzer: str, run_id: int, index: int, truth: str,
              fact: dict[str, Any]) -> dict[str, Any]:
    locator = {"capture_id": f"run:{run_id}", "run_id": run_id,
        "analyzer": analyzer, "analyzer_version": "M12_ACCEPTED_ARTIFACT",
        "source_sha256": digest, "fact_index": index,
        "original_truth": truth, "fact": fact}
    payload = {"subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": digest, "fact_kind": analyzer, "locator": locator}
    return {"ref_id": stable_id("fusion-evidence", payload),
        "subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": digest, "fact_kind": analyzer, "fact_count": 1,
        "locator_json": canonical(locator)}


def _verify_capture(paths: dict[str, Path], run_id: int, rom_sha: str) -> dict[str, dict[str, Any]]:
    expected = {"gameplay": "oasis.m12.postrun-gameplay-provenance.v1",
        "sprite": "oasis.m12.postrun-sprite-provenance.v1",
        "vdp_dma": "oasis.m12.postrun-vdp-dma.v1",
        "gameplay_receipt": "oasis.m12.postrun-gameplay-receipt.v1",
        "sprite_receipt": None, "vdp_receipt": None,
        "generic_graph": "oasis.m12.generic-provenance-graph.v1",
        "closure_receipt": "oasis.m13.generic-recursive-closure-receipt.v1"}
    values = {key: _load(path) for key, path in paths.items()
              if key != "normalized_corpus"}
    for key, schema in expected.items():
        if schema and values[key].get("schema") != schema:
            raise ValueError("STOP_FUSION_SCHEMA_MISMATCH:" + key)
        if key not in {"gameplay", "gameplay_receipt", "closure_receipt", "generic_graph"} and \
                int(values[key].get("run_id", -1)) != run_id:
            raise ValueError("STOP_FUSION_RUN_ID_MISMATCH:" + key)
    for key in ("gameplay_receipt", "sprite_receipt", "vdp_receipt"):
        receipt = values[key]
        if receipt.get("status") != "PASS" or int(receipt.get("run_id", -1)) != run_id:
            raise ValueError("STOP_FUSION_RECEIPT_NOT_ACCEPTED:" + key)
        if receipt.get("input_hashes", {}).get("rom_sha256") != rom_sha:
            raise ValueError("STOP_FUSION_RECEIPT_ROM_MISMATCH:" + key)
        if int(receipt.get("source_owned_delta", -1)) != 0:
            raise ValueError("STOP_FUSION_RECEIPT_OWNERSHIP_DELTA:" + key)
    gp_hash = values["gameplay_receipt"].get("output_hashes", {}).get(
        "postrun_gameplay_provenance_sha256")
    if gp_hash != _sha(paths["gameplay"]):
        raise ValueError("STOP_FUSION_GAMEPLAY_OUTPUT_HASH_MISMATCH")
    if values["sprite_receipt"].get("output_hashes", {}).get(
            "postrun_sprite_provenance_sha256") != _sha(paths["sprite"]) or \
            values["vdp_receipt"].get("output_hashes", {}).get(
            "postrun_vdp_dma_sha256") != _sha(paths["vdp_dma"]):
        raise ValueError("STOP_FUSION_ANALYZER_OUTPUT_HASH_MISMATCH")
    closure = values["closure_receipt"]
    corpus_sha = _corpus_string(paths["normalized_corpus"], "corpus_sha256")
    raw_sha = _corpus_string(paths["normalized_corpus"], "raw_sha256")
    corpus_schema = _corpus_string(paths["normalized_corpus"], "schema")
    if closure.get("status") not in {"PASS", "NO_DELTA", "PASS_GENERIC_RECURSIVE_CLOSURE"} or \
            int(closure.get("source_owned_delta", -1)) != 0 or \
            closure.get("rom_audit", {}).get("rom_sha256") != rom_sha or \
            int(closure.get("rom_audit", {}).get("rom_size", -1)) != ROM_BYTES or \
            closure.get("corpus_sha256") != corpus_sha or \
            corpus_schema not in {"oasis.m13.normalized-generic-corpus.v1",
                                  "oasis.m13.normalized-generic-corpus.v2"} or \
            not str(closure.get("corpus_id", "")).startswith(f"run-{run_id}-") or \
            not str(closure.get("corpus_id", "")).endswith(raw_sha[:16]):
        raise ValueError("STOP_FUSION_FLOW_RECEIPT_NOT_ACCEPTED")
    return values


def import_capture(store: KnowledgeStore, paths: dict[str, Path], run_id: int) -> dict[str, Any]:
    """Import a single accepted capture; exact adapters preserve source truth."""
    values = _verify_capture(paths, run_id, store.meta()["rom_sha256"])
    schemas = {"gameplay": values["gameplay"]["schema"],
        "sprite": values["sprite"]["schema"], "vdp_dma": values["vdp_dma"]["schema"],
        "gameplay_receipt": values["gameplay_receipt"]["schema"],
        "sprite_receipt": values["sprite_receipt"]["schema"],
        "vdp_receipt": values["vdp_receipt"]["schema"],
        "generic_graph": values["generic_graph"]["schema"],
        "closure_receipt": values["closure_receipt"]["schema"],
        "normalized_corpus": "oasis.normalized-flow-v2"}
    artifacts, hashes = _artifact_rows(paths, schemas)
    store.insert_rows("source_artifact", artifacts)
    relation_rows: list[dict[str, Any]] = []
    evidence_rows: list[dict[str, Any]] = []
    derivations: list[str] = []
    relevant_pcs: set[int] = set()
    occurrence_pcs: set[int] = set()
    runtime_rows: list[dict[str, Any]] = []
    gp = values["gameplay"]
    gameplay_facts = gp.get("ram_to_sat", {}).get("exact", [])
    sprite_facts = values["sprite"].get("exact_dma_to_sat_chains", [])
    sprite_by_key: dict[tuple[int, int, int, int], list[tuple[int, dict[str, Any]]]] = {}
    for i, fact in enumerate(sprite_facts):
        key = (_addr(fact["causing_pc"]), _addr(fact["source"]),
            int(fact["destination_start"]), int(fact["length_bytes"]))
        sprite_by_key.setdefault(key, []).append((i, fact))
    joined = set()
    for index, fact in enumerate(gameplay_facts):
        if fact.get("truth_class") != "EXACT":
            continue
        shadow = fact.get("sat_shadow_write", {})
        transfer = fact.get("transfer", {})
        pc = _addr(transfer["causing_pc"])
        ram = _addr(shadow["address"])
        destination = _addr(transfer["destination_start"])
        length = int(transfer["length_bytes"])
        entries = sorted(set(int(x) for x in fact.get("sat_entry_field", {}).get("entries", [])))
        emitter = _rom_object(store, pc)
        producer_pc = _addr(shadow["producer_pc"])
        producer = _rom_object(store, producer_pc)
        relevant_pcs.update((pc, producer_pc))
        occurrence_pcs.add(pc)
        ram_fact = {"producer_pc": shadow["producer_pc"], "ram_address": shadow["address"],
            "causing_pc": transfer["causing_pc"], "source": transfer.get("source_address", shadow["address"]),
            "destination_start": transfer["destination_start"], "length_bytes": length,
            "sat_entries": entries}
        ram_relation = _relation("RAM_SHADOW_TO_DMA", producer, emitter, None,
            "DERIVED_EXACT", {"ram_address": ram, "ram_domain": "M68K_RAM",
                "dma_source_address": ram, "destination_start": destination,
                "length_bytes": length, "source_truth": fact.get("truth_class")})
        relation_rows.append(ram_relation)
        gp_ref = _evidence("RELATION", ram_relation["relation_id"], hashes["gameplay"],
            "gameplay-provenance-v1", run_id, index, "EXACT", fact)
        evidence_rows.append(gp_ref)
        for sprite_index, sprite_fact in sprite_by_key.get((pc, ram, destination, length), []):
            sprite_entries = sorted(set(int(x) for x in sprite_fact.get("affected_entries", [])))
            if entries and not set(entries).issubset(sprite_entries):
                continue
            dma_relation = _relation("DMA_TO_HARDWARE_SAT", emitter, None, destination,
                "DERIVED_EXACT", {"dma_source_address": ram, "source_domain": "68K_RAM",
                    "destination_domain": "VRAM", "destination_end": int(sprite_fact["destination_end"]),
                    "length_bytes": length, "sat_entries": sprite_entries,
                    "frame": int(sprite_fact.get("frame", 0)),
                    "source_truth": "EXACT_DMA_TO_SAT_CHAIN"})
            relation_rows.append(dma_relation)
            sp_ref = _evidence("RELATION", dma_relation["relation_id"], hashes["sprite"],
                "sprite-sat-v1", run_id, sprite_index, "EXACT_DMA_TO_SAT_CHAIN", sprite_fact)
            evidence_rows.append(sp_ref)
            vdp_matches = [j for j, row in enumerate(values["vdp_dma"].get("dma_events_sample", []))
                if _addr(row.get("causing_pc", -1)) == pc and
                _addr(row.get("source_address", -1)) == ram and
                _addr(row.get("destination_address", -1)) == destination and
                int(row.get("length_bytes", -1)) == length and row.get("classification") == "DMA_EXACT"]
            for vdp_index in vdp_matches:
                vdp_fact = values["vdp_dma"]["dma_events_sample"][vdp_index]
                evidence_rows.append(_evidence("RELATION", dma_relation["relation_id"],
                    hashes["vdp_dma"], "vdp-dma-v1", run_id, vdp_index,
                    "DMA_EXACT", vdp_fact))
            if not vdp_matches:
                continue
            relation_pair = (ram_relation["relation_id"], dma_relation["relation_id"])
            if relation_pair in joined:
                continue
            join_inputs = [{"subject_type": "relation", "subject_id": ram_relation["relation_id"],
                "role": "GAMEPLAY_RAM_SHADOW_TO_DMA"},
                {"subject_type": "relation", "subject_id": dma_relation["relation_id"],
                 "role": "SPRITE_DMA_TO_HARDWARE_SAT"}]
            params = {"join_key": [producer_pc, pc, ram, destination, length, entries],
                "capture_id": f"run:{run_id}"}
            output_id = stable_id("fusion-path", params)
            existing = store.db.execute("SELECT derivation_id FROM derivation WHERE output_id=?",
                                        (output_id,)).fetchone()
            derivation_id = str(existing[0]) if existing else store.record_derivation(
                RULE, RULE_VERSION, _sha(Path(__file__)),
                sha256_bytes(canonical(params).encode()), join_inputs,
                "relation_path", output_id,
                {"relations": [ram_relation["relation_id"], dma_relation["relation_id"]],
                 "truth": "DERIVED_EXACT", "join_identity": emitter}, [])
            derivations.append(derivation_id)
            joined.add(relation_pair)
    occurrence_count = 0
    occurrence_ids: set[str] = set()
    for index, event in iter_target_instructions(paths["normalized_corpus"], occurrence_pcs):
        if int(event.get("run_id", -1)) != run_id or \
                int(event.get("pc", -1)) not in relevant_pcs or \
                event.get("cpu_id") not in {"M68K", "Z80"}:
            continue
        required = ("epoch", "stream_sequence", "instruction_sequence")
        if any(event.get(field) is None for field in required):
            raise ValueError("STOP_FUSION_RUNTIME_OCCURRENCE_INCOMPLETE")
        pc = int(event["pc"])
        object_id = _rom_object(store, pc)
        epoch = int(event["epoch"])
        domain = str(event.get("domain", "UNSPECIFIED"))
        occurrence = runtime_occurrence_id(capture_id=f"run:{run_id}", epoch=epoch,
            cpu=str(event["cpu_id"]), address_space=domain,
            native_sequence=int(event["stream_sequence"]),
            instruction_sequence=int(event["instruction_sequence"]),
            event_kind="INSTRUCTION", run_id=run_id)
        occurrence_ids.add(occurrence)
        locator = {"capture_id": f"run:{run_id}:epoch:{epoch}", "run_id": run_id,
            "event_index": index,
            "epoch": epoch, "cpu_id": event["cpu_id"], "address_space": domain,
            "native_sequence": int(event["stream_sequence"]),
            "instruction_sequence": int(event["instruction_sequence"]),
            "occurrence_id": occurrence, "original_truth": event.get("opcode_verification"),
            "analyzer": "normalized-flow-v2", "source_sha256": hashes["normalized_corpus"],
            "event": event}
        ev_payload = {"occurrence_id": occurrence, "subject_type": "rom_object",
            "subject_id": object_id, "source_sha256": hashes["normalized_corpus"],
            "event_index": index}
        runtime_rows.append({"ref_id": stable_id("runtime-evidence", ev_payload),
            "subject_type": "rom_object", "subject_id": object_id,
            "source_sha256": hashes["normalized_corpus"],
            "fact_kind": "RUNTIME_OCCURRENCE:INSTRUCTION", "fact_count": 1,
            "locator_json": canonical(locator)})
        occurrence_count += 1
        if len(runtime_rows) >= 2048:
            store.insert_rows("evidence_ref", runtime_rows)
            runtime_rows.clear()
    store.insert_rows("relation", relation_rows)
    store.insert_rows("evidence_ref", evidence_rows)
    store.insert_rows("evidence_ref", runtime_rows)
    occurrence_count = len(occurrence_ids)
    store.db.commit()
    return {"capture_id": f"run:{run_id}", "gameplay_exact": len(gameplay_facts),
        "sprite_exact": len(sprite_facts), "relations_added": len(relation_rows),
        "evidence_refs_added": len(evidence_rows), "runtime_occurrences": occurrence_count,
        "runtime_occurrence_selection": "EARLIEST_EVENT_PER_DMA_EMITTER_PC",
        "cross_source_connections": len(joined),
        "derivations": sorted(set(derivations)), "source_hashes": hashes}


def clone_and_fuse(base_db: Path, child_db: Path, generation_id: str,
                   parent_generation: str, capture_sets: list[tuple[int, dict[str, Path]]],
                   verify_idempotence: bool = False) -> dict[str, Any]:
    """Build a child generation from the canonical DB and import ordered captures."""
    if child_db.exists():
        child_db.unlink()
    child_db.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(base_db, child_db)
    store = KnowledgeStore(child_db, ROM_SHA, ROM_BYTES)
    baseline_hashes = store.hashes()
    baseline_ranges = int(store.db.execute("SELECT COUNT(*) FROM emission").fetchone()[0])
    baseline_owned = int(store.db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0] or 0)
    if baseline_hashes["emission_hash"] != EMISSION_SHA:
        store.close()
        raise ValueError("STOP_FUSION_BASE_EMISSION_HASH_MISMATCH")
    if baseline_ranges != 2489 or baseline_owned != SOURCE_OWNED:
        store.close()
        raise ValueError("STOP_FUSION_BASELINE_MAP_METRICS_MISMATCH")
    store.set_generation_identity(generation_id, parent_generation)
    imported = []
    duplicate_hashes = []
    for run_id, paths in capture_sets:
        imported.append(import_capture(store, paths, run_id))
        if verify_idempotence:
            first_graph = logical_graph_hash(store)
            first_hashes = store.hashes()
            import_capture(store, paths, run_id)
            if first_graph != logical_graph_hash(store) or first_hashes != store.hashes():
                store.close()
                raise ValueError("STOP_FUSION_IDEMPOTENCE_MISMATCH")
            duplicate_hashes.append(first_graph)
    after = store.hashes()
    owned_after = int(store.db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0] or 0)
    if after["emission_hash"] != baseline_hashes["emission_hash"] or owned_after != baseline_owned:
        store.close()
        raise ValueError("FATAL_FUSION_EMISSION_MUTATION")
    paths = [witness_path(store, item["capture_id"]) for item in imported]
    all_relation_ids = {rid for path in paths for rid in path["relations"]}
    components_before = len(paths) * 2
    components_after = component_count(store, all_relation_ids)
    shared_objects = 0
    for path in paths:
        object_id = next(node["object_id"] for node in path["nodes"]
                         if node["kind"] == "CANONICAL_ROM_OBJECT")
        artifact_types = {str(row[0]) for row in store.db.execute("""SELECT DISTINCT s.artifact_type
            FROM relation r JOIN evidence_ref e ON e.subject_type='RELATION' AND e.subject_id=r.relation_id
            JOIN source_artifact s USING(source_sha256) WHERE r.source_object_id=? OR r.target_object_id=?""",
            (object_id, object_id))}
        if sum("GAMEPLAY" in value or "SPRITE" in value or "VDP_DMA" in value
               for value in artifact_types) >= 2:
            shared_objects += 1
    result = {"baseline_emission_hash": baseline_hashes["emission_hash"],
        "emission_hash": after["emission_hash"], "rom_bytes": ROM_BYTES,
        "ranges": baseline_ranges, "gaps": 0, "overlaps": 0,
        "source_owned_before": baseline_owned, "source_owned_after": owned_after,
        "source_owned_delta": owned_after - baseline_owned,
        "graph_hash": logical_graph_hash(store), "map_hash": after["map_hash"],
        "proposal_count": int(store.db.execute("SELECT COUNT(*) FROM map_proposal").fetchone()[0]),
        "captures": imported, "witness_paths": paths,
        "components_before": components_before, "components_after": components_after,
        "cross_source_connections": sum(x["cross_source_connections"] for x in imported),
        "multi_source_objects": shared_objects,
        "runtime_occurrences": int(store.db.execute("SELECT COUNT(*) FROM evidence_ref WHERE fact_kind LIKE 'RUNTIME_OCCURRENCE:%'").fetchone()[0]),
        "duplicate_observations_merged": sum(x["runtime_occurrences"] for x in imported) if verify_idempotence else 0,
        "multi_capture_objects": len(set.intersection(*[
            {node["object_id"] for node in path["nodes"] if node["kind"] == "CANONICAL_ROM_OBJECT"}
            for path in paths])) if len(paths) > 1 else 0,
        "duplicate_import_hashes": duplicate_hashes,
        "logical_counts": {table: int(store.db.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0])
            for table in ("rom_object", "claim", "relation", "derivation", "conflict", "evidence_ref")}}
    store.close()
    reopened = KnowledgeStore(child_db, ROM_SHA, ROM_BYTES, read_only=True)
    if logical_graph_hash(reopened) != result["graph_hash"] or \
            reopened.hashes()["emission_hash"] != EMISSION_SHA:
        reopened.close()
        raise ValueError("STOP_FUSION_PERSISTENCE_ROUNDTRIP_MISMATCH")
    reopened.close()
    return result


def component_count(store: KnowledgeStore, relation_ids: set[str] | None = None) -> int:
    """Connected components among relation endpoints, optionally witness-scoped."""
    where, args = ("", ()) if relation_ids is None else (
        " WHERE relation_id IN (" + ",".join("?" for _ in relation_ids) + ")", tuple(sorted(relation_ids)))
    relations = list(store.db.execute("SELECT source_object_id,target_object_id FROM relation" + where, args))
    nodes = {str(row[0]) for row in relations}
    nodes.update(str(row[1]) for row in relations if row[1] is not None)
    if not nodes:
        return 0
    parent = {node: node for node in nodes}
    def find(node: str) -> str:
        while parent[node] != node:
            parent[node] = parent[parent[node]]
            node = parent[node]
        return node
    for source, target in relations:
        if target is None:
            continue
        a, b = find(str(source)), find(str(target))
        if a != b:
            parent[a] = b
    return len({find(node) for node in nodes})
