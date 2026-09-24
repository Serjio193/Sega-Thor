"""Canonical read API backed by a verified MASTER V2 shadow container."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import sqlite3
import sys
from typing import Any, Iterator

from master_v2_shadow import ROM_SHA, ROM_SIZE, SCHEMA, read_master_v2_section

MASTER_SCHEMA = {
    "map_conflict": "conflict_id TEXT PRIMARY KEY,object_type TEXT NOT NULL,object_id TEXT NOT NULL,claims TEXT NOT NULL,lineage TEXT NOT NULL",
    "map_edge": "edge_id TEXT PRIMARY KEY,source_id TEXT NOT NULL,target_id TEXT NOT NULL,relation TEXT NOT NULL,scope TEXT NOT NULL,status TEXT NOT NULL,body TEXT NOT NULL,lineage TEXT NOT NULL",
    "map_frontier": "frontier_id TEXT PRIMARY KEY,anchor TEXT NOT NULL,role TEXT NOT NULL,reason TEXT NOT NULL,target TEXT NOT NULL,status TEXT NOT NULL,body TEXT NOT NULL,lineage TEXT NOT NULL",
    "map_import": "import_ref TEXT PRIMARY KEY,source_sha256 TEXT NOT NULL,body TEXT NOT NULL",
    "map_meta": "key TEXT PRIMARY KEY,value TEXT NOT NULL",
    "map_node": "node_id TEXT PRIMARY KEY,kind TEXT NOT NULL,node_key TEXT NOT NULL,scope TEXT NOT NULL,status TEXT NOT NULL,body TEXT NOT NULL,lineage TEXT NOT NULL",
}
KNOWLEDGE_SCHEMA = {
    "claim": "claim_id TEXT PRIMARY KEY,object_id TEXT NOT NULL,claim_type TEXT NOT NULL,value_json TEXT NOT NULL,status TEXT NOT NULL",
    "conflict": "conflict_id TEXT PRIMARY KEY,start INTEGER NOT NULL,end INTEGER NOT NULL,conflict_type TEXT NOT NULL,detail_json TEXT NOT NULL",
    "emission": "start INTEGER NOT NULL,end INTEGER NOT NULL,emission_type TEXT NOT NULL,classification TEXT NOT NULL,source_kind TEXT NOT NULL,source_owned INTEGER NOT NULL,artifact_type TEXT NOT NULL,artifact TEXT NOT NULL,PRIMARY KEY(start,end)",
    "evidence_ref": "ref_id TEXT PRIMARY KEY,subject_type TEXT NOT NULL,subject_id TEXT NOT NULL,source_sha256 TEXT NOT NULL,fact_kind TEXT NOT NULL,fact_count INTEGER NOT NULL,locator_json TEXT NOT NULL",
    "map_import": "import_key TEXT PRIMARY KEY,input_hash TEXT NOT NULL",
    "map_meta": "key TEXT PRIMARY KEY,value TEXT NOT NULL",
    "relation": "relation_id TEXT PRIMARY KEY,relation_type TEXT NOT NULL,source_object_id TEXT NOT NULL,target_object_id TEXT,target_address INTEGER,status TEXT NOT NULL,attributes_json TEXT NOT NULL",
    "rom_object": "object_id TEXT PRIMARY KEY,range_id TEXT NOT NULL,object_type TEXT NOT NULL,attributes_json TEXT NOT NULL",
    "rom_range": "range_id TEXT PRIMARY KEY,rom_sha256 TEXT NOT NULL,start INTEGER NOT NULL,end INTEGER NOT NULL",
    "source_artifact": "source_sha256 TEXT PRIMARY KEY,checkpoint TEXT NOT NULL,artifact_name TEXT NOT NULL,artifact_type TEXT NOT NULL",
    "derivation": "derivation_id TEXT PRIMARY KEY,rule_id TEXT NOT NULL,rule_version TEXT NOT NULL,implementation_hash TEXT NOT NULL,parameters_hash TEXT NOT NULL,output_type TEXT NOT NULL,output_id TEXT NOT NULL,result_json TEXT NOT NULL,assumptions_json TEXT NOT NULL",
    "derivation_input": "derivation_id TEXT NOT NULL,ordinal INTEGER NOT NULL,subject_type TEXT NOT NULL,subject_id TEXT NOT NULL,role TEXT NOT NULL,PRIMARY KEY(derivation_id,ordinal)",
    "map_proposal": "proposal_id TEXT PRIMARY KEY,base_generation TEXT NOT NULL,base_map_hash TEXT NOT NULL,graph_hash TEXT NOT NULL,validator_version TEXT NOT NULL,status TEXT NOT NULL,proposal_set_hash TEXT NOT NULL",
    "map_proposal_operation": "proposal_id TEXT NOT NULL,ordinal INTEGER NOT NULL,operation_json TEXT NOT NULL,PRIMARY KEY(proposal_id,ordinal)",
}
COLUMNS = {
    "map_conflict": ("conflict_id", "object_type", "object_id", "claims", "lineage"),
    "map_edge": ("edge_id", "source_id", "target_id", "relation", "scope", "status", "body", "lineage"),
    "map_frontier": ("frontier_id", "anchor", "role", "reason", "target", "status", "body", "lineage"),
    "map_import": ("import_ref", "source_sha256", "body"),
    "map_meta": ("key", "value"),
    "map_node": ("node_id", "kind", "node_key", "scope", "status", "body", "lineage"),
    "claim": ("claim_id", "object_id", "claim_type", "value_json", "status"),
    "conflict": ("conflict_id", "start", "end", "conflict_type", "detail_json"),
    "emission": ("start", "end", "emission_type", "classification", "source_kind", "source_owned", "artifact_type", "artifact"),
    "evidence_ref": ("ref_id", "subject_type", "subject_id", "source_sha256", "fact_kind", "fact_count", "locator_json"),
    "relation": ("relation_id", "relation_type", "source_object_id", "target_object_id", "target_address", "status", "attributes_json"),
    "rom_object": ("object_id", "range_id", "object_type", "attributes_json"),
    "rom_range": ("range_id", "rom_sha256", "start", "end"),
    "source_artifact": ("source_sha256", "checkpoint", "artifact_name", "artifact_type"),
}
KNOWLEDGE_COLUMNS = dict(COLUMNS, map_import=("import_key", "input_hash"),
    derivation=("derivation_id", "rule_id", "rule_version", "implementation_hash", "parameters_hash", "output_type", "output_id", "result_json", "assumptions_json"),
    derivation_input=("derivation_id", "ordinal", "subject_type", "subject_id", "role"),
    map_proposal=("proposal_id", "base_generation", "base_map_hash", "graph_hash", "validator_version", "status", "proposal_set_hash"),
    map_proposal_operation=("proposal_id", "ordinal", "operation_json"))


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _payload(view_path: Path, section: str) -> dict[str, Any]:
    return json.loads(read_master_v2_section(view_path, section))


class MasterCanonicalView:
    """Single canonical access layer for map, ownership and emission facts."""

    def __init__(self, path: Path):
        self.path = Path(path).resolve()
        self._meta = json.loads(read_master_v2_section(self.path, "meta"))
        if self._meta.get("schema") != SCHEMA or self.rom_identity != ROM_SHA:
            raise ValueError("STOP_MASTER_V2_CANONICAL_ROM_IDENTITY")
        if int(self._meta.get("rom", {}).get("size", 0)) != ROM_SIZE:
            raise ValueError("STOP_MASTER_V2_CANONICAL_ROM_SIZE")
        self._sections: dict[str, dict[str, Any]] = {}

    @property
    def rom_identity(self) -> str:
        return str(self._meta.get("rom", {}).get("sha256", ""))

    @property
    def generation_id(self) -> str:
        return str(self._meta.get("generation_id", ""))

    @property
    def parent_generation(self) -> str:
        return str(self._meta.get("parent_generation", ""))

    @property
    def parent_master_sha256(self) -> str:
        return str(self._meta.get("parent_master_sha256", ""))

    @property
    def absorbed_run_ids(self) -> tuple[int, ...]:
        return tuple(int(item) for item in self._meta.get("absorbed_run_ids", ()))

    def verify_legacy_shadow(self, canonical_root: Path) -> None:
        """Check legacy pointer identity without reading legacy canonical content."""
        pointer = json.loads((Path(canonical_root) / "current.json").read_text(encoding="utf-8"))
        if pointer.get("master_sha256") != self._meta.get("canonical_master_sha256") or \
                pointer.get("knowledge_sha256") != self._meta.get("canonical_knowledge_sha256"):
            raise ValueError("STOP_MASTER_V2_LEGACY_CANONICAL_SHADOW_MISMATCH")

    def _tables(self, section: str) -> dict[str, list[list[Any]]]:
        if section not in self._sections:
            self._sections[section] = _payload(self.path, section)["tables"]
        return self._sections[section]

    def rows(self, table: str) -> Iterator[dict[str, Any]]:
        section = "canonical_map_master" if table.startswith("map_") and table in MASTER_SCHEMA else "canonical_knowledge"
        columns = (KNOWLEDGE_COLUMNS if section == "canonical_knowledge" else COLUMNS)[table]
        for values in self._tables(section).get(table, []):
            yield dict(zip(columns, values))

    @property
    def objects(self) -> Iterator[dict[str, Any]]:
        return self.rows("rom_object")

    @property
    def intervals(self) -> Iterator[dict[str, Any]]:
        return self.rows("rom_range")

    @property
    def instruction_map(self) -> Iterator[dict[str, Any]]:
        for item in self.objects:
            if item.get("object_type") == "M68K_INSTRUCTION":
                yield item

    @property
    def relations(self) -> Iterator[dict[str, Any]]:
        return self.rows("relation")

    @property
    def executed_instruction_facts(self) -> Iterator[dict[str, Any]]:
        return (row for row in self.rows("claim") if row["claim_type"] == "EXECUTED_FROM_ROM")

    @property
    def executed_next(self) -> Iterator[dict[str, Any]]:
        return (row for row in self.relations if row["relation_type"] == "EXECUTED_NEXT")

    @property
    def observed_next_pc(self) -> Iterator[dict[str, Any]]:
        return (row for row in self.relations if row["relation_type"] == "OBSERVED_NEXT_PC")

    @property
    def emission_partition(self) -> Iterator[dict[str, Any]]:
        return self.rows("emission")

    @property
    def source_owned_bytes(self) -> int:
        return sum(int(row["end"]) - int(row["start"]) for row in self.emission_partition
                   if int(row["source_owned"]) == 1)

    @property
    def ownership(self) -> Iterator[dict[str, Any]]:
        return self.emission_partition

    @property
    def canonical_hashes(self) -> dict[str, str]:
        return {key: str(self._meta[key]) for key in
                ("canonical_master_sha256", "canonical_knowledge_sha256")}

    @property
    def lineage(self) -> dict[str, Any]:
        return {"generation_id": self.generation_id, "parent_generation": self.parent_generation,
                "parent_master_sha256": self.parent_master_sha256,
                "absorbed_run_ids": list(self.absorbed_run_ids)}

    def materialize(self, root: Path, rom_path: Path | None = None) -> Path:
        """Materialize a stage scratch generation from V2, never from legacy canonical files."""
        root = Path(root).resolve(); generation = root / "generations" / self.generation_id
        generation.mkdir(parents=True, exist_ok=False)
        paths = {"master": generation / "master.sqlite", "knowledge": generation / "knowledge.sqlite"}
        self._write_db(paths["master"], MASTER_SCHEMA, "canonical_map_master")
        self._write_db(paths["knowledge"], KNOWLEDGE_SCHEMA, "canonical_knowledge")
        evidence_tools = Path(__file__).resolve().parents[2] / "src" / "tools" / "thor_evidence"
        if str(evidence_tools) not in sys.path:
            sys.path.insert(0, str(evidence_tools))
        from rom_knowledge_map import KnowledgeStore
        knowledge = KnowledgeStore(paths["knowledge"], ROM_SHA, ROM_SIZE, read_only=True)
        try:
            logical_hashes = knowledge.hashes()
        finally:
            knowledge.close()
        expected_hashes = self._meta.get("canonical_logical_hashes", {})
        if expected_hashes and logical_hashes != expected_hashes:
            raise ValueError("STOP_MASTER_V2_CANONICAL_LOGICAL_HASH_MISMATCH")
        baseline = None
        if rom_path is not None:
            baseline = self._write_baseline_materialized(generation, Path(rom_path).read_bytes())
        metadata = {"schema": "oasis.m12.postrun-canonical-generation.v1",
                    "generation_id": self.generation_id, "rom_sha256": ROM_SHA,
                    "rom_size": ROM_SIZE, "parent_generation_id": self.parent_generation or None,
                    "logical_hashes": logical_hashes}
        if baseline is not None:
            metadata["stage7"] = {"materialized": str(baseline)}
        (generation / "generation-metadata.json").write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        pointer = {"schema": "oasis.m12.master-v2-materialized.current.v1",
                   "generation_dir": f"generations/{self.generation_id}",
                   "master_sha256": _sha(paths["master"]), "knowledge_sha256": _sha(paths["knowledge"]),
                   "generation_id": self.generation_id,
                   "parent_generation_id": self.parent_generation or None,
                   "logical_hashes": logical_hashes,
                   "rom_sha256": ROM_SHA, "source": str(self.path)}
        (root / "current.json").write_text(json.dumps(pointer, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return generation

    def _write_baseline_materialized(self, generation: Path, rom: bytes) -> Path:
        if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
            raise ValueError("STOP_MASTER_V2_CANONICAL_ROM_SOURCE")
        baseline = generation / "stage7-baseline"
        (baseline / "code").mkdir(parents=True)
        (baseline / "blobs").mkdir()
        entries = []
        for index, row in enumerate(self.emission_partition):
            start, end = int(row["start"]), int(row["end"])
            artifact_type = str(row["artifact_type"])
            artifact = (f"code/sub_{start:06X}.asm" if artifact_type == "asm"
                        else f"blobs/{start:06X}_{end:06X}.bin")
            entry = {"start": start, "end": end, "size": end - start,
                     "kind": "CODE_VERIFIED" if artifact_type == "asm" else "UNKNOWN",
                     "source": "MASTER_V2_CANONICAL", "confidence": "ROM_HASH_VERIFIED",
                     "emitted_artifact_type": "asm" if artifact_type == "asm" else "blob",
                     "artifact": artifact, "manifest_index": index}
            destination = baseline / artifact
            destination.parent.mkdir(parents=True, exist_ok=True)
            if artifact_type == "asm":
                lines = ["; MASTER V2 canonical baseline", "    dc.b " + ",".join(
                    f"${value:02X}" for value in rom[start:min(end, start + 16)])]
                for offset in range(start + 16, end, 16):
                    lines.append("    dc.b " + ",".join(
                        f"${value:02X}" for value in rom[offset:min(end, offset + 16)]))
                destination.write_text("\n".join(lines) + "\n", encoding="utf-8")
            else:
                destination.write_bytes(rom[start:end])
            entries.append(entry)
        manifest = {"schema": "oasis.full-rom-split.v1", "rom_sha256": ROM_SHA,
                    "rom_size": ROM_SIZE, "entries": entries, "full_match": True,
                    "metrics": {"SOURCE_OWNED_BYTES": self.source_owned_bytes},
                    "source": "M12_MASTER_V2_CANONICAL_BASELINE"}
        (baseline / "manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return baseline

    def _write_db(self, path: Path, schemas: dict[str, str], section: str) -> None:
        db = sqlite3.connect(path)
        try:
            for table, schema in schemas.items():
                db.execute(f"CREATE TABLE {table}({schema})")
                columns = (KNOWLEDGE_COLUMNS if section == "canonical_knowledge" else COLUMNS)[table]
                for row in self._tables(section).get(table, []):
                    db.execute(f"INSERT INTO {table} VALUES ({','.join('?' for _ in columns)})", row)
            if section == "canonical_knowledge":
                db.execute("INSERT OR REPLACE INTO map_meta VALUES ('schema','oasis.m14.canonical-rom-knowledge.v2')")
                db.execute("INSERT OR REPLACE INTO map_meta VALUES ('rom_sha256',?)", (ROM_SHA,))
                db.execute("INSERT OR REPLACE INTO map_meta VALUES ('rom_size',?)", (str(ROM_SIZE),))
                db.execute("INSERT OR REPLACE INTO map_meta VALUES ('generation_id',?)",
                           (self.generation_id,))
                db.execute("INSERT OR REPLACE INTO map_meta VALUES ('parent_generation_id',?)",
                           (self.parent_generation,))
            db.commit()
            if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
                raise ValueError("STOP_MASTER_V2_MATERIALIZED_INTEGRITY")
        finally:
            db.close()


def resolve_master_canonical_view(project_root: Path) -> MasterCanonicalView | None:
    """Resolve the accepted R2 MASTER V2 pointer, verifying its legacy shadow."""
    project_root = Path(project_root).resolve()
    configured = os.environ.get("THOR_MASTER_V2_PATH")
    shadow_root = project_root / "build" / "thor-evidence" / "master-v2-shadow-r2"
    pointer_path = shadow_root / "shadow-current.json"
    if configured:
        candidates = [Path(configured)]
    elif pointer_path.is_file():
        pointer = json.loads(pointer_path.read_text(encoding="utf-8"))
        candidates = [shadow_root / pointer["path"]]
    else:
        candidates = []
    legacy_root = project_root / "build" / "thor-evidence" / \
        "archivist-knowledge-pipeline-2g-canonical-verified-20260918"
    for candidate in candidates:
        if not candidate.is_file():
            continue
        try:
            view = MasterCanonicalView(candidate)
            if not configured:
                view.verify_legacy_shadow(legacy_root)
            return view
        except (KeyError, OSError, ValueError, json.JSONDecodeError):
            continue
    return None


__all__ = ["MasterCanonicalView", "resolve_master_canonical_view"]
