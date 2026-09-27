"""M12 gameplay RAM/entity-candidate contracts."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
RING = ROOT / "tools" / "bizhawk-native-ring"
sys.path.insert(0, str(RING))

from live_forward_gameplay_stage import (  # noqa: E402
    build_ram_to_sat_field_chains, deduplicate_overlap_observations,
    infer_stride_from_accesses, run_gameplay_pipeline_stage, run_gameplay_stage,
    track_runtime_instances,
)
from live_forward_progress import STAGES  # noqa: E402

RECORD = struct.Struct("<QQQIIIHBBHHI")


def test_a_stride_requires_generation_witness() -> None:
    items = [{"address": 0xFF1000}, {"address": 0xFF1004}, {"address": 0xFF1008}]
    assert infer_stride_from_accesses(items)["truth_class"] == "HYPOTHESIS"
    for item in items:
        item["loop_witness"] = True
    result = infer_stride_from_accesses(items)
    assert result["stride"] == 4 and result["truth_class"] == "DERIVED_EXACT"


def test_b_slot_iteration_and_c_lifetime_reuse() -> None:
    events = [{"address": 0xFF1000, "observation": 0, "stream_sequence": 1, "kind": "WRITE"},
              {"address": 0xFF1004, "observation": 0, "stream_sequence": 2, "kind": "WRITE"},
              {"address": 0xFF1000, "observation": 3, "stream_sequence": 3, "kind": "WRITE"}]
    instances = track_runtime_instances(events, 0xFF1000, 4)
    assert [item["slot_index"] for item in instances] == [0, 0, 1]
    assert instances[0]["deactivation_reuse_witness"] == {"next_instance": 1}


def test_d_exact_ram_sat_chain_and_e_partial_fail_closed() -> None:
    writes = [{"address": 0xFF13CC, "width_bytes": 2, "pc": 0xA372}]
    transfer = [{"source_address": "0xFF13CC", "destination_start": 0xD000,
                 "length_bytes": 24, "affected_entries": [0, 1]}]
    chains = build_ram_to_sat_field_chains(writes, transfer)
    assert len(chains["exact"]) == 1
    assert chains["exact"][0]["sat_entry_field"]["field"] == "y"
    partial = build_ram_to_sat_field_chains([{"address": 0xFF13CD, "width_bytes": 1, "pc": 1}], transfer)
    assert len(partial["partial"]) == 1 and not partial["exact"]


def test_f_one_to_many_and_g_no_semantic_labels() -> None:
    writes = [{"address": 0xFF13CC, "width_bytes": 2, "pc": 0xA372}]
    chains = build_ram_to_sat_field_chains(writes, [{"source_address": "0xFF13CC",
        "destination_start": 0xD000, "length_bytes": 24, "affected_entries": [0, 1, 2]}])
    assert chains["exact"][0]["sat_entry_field"]["entries"] == [0]
    assert not any(word in json.dumps(chains).lower() for word in ("player", "enemy", "npc"))


def test_h_overlap_observation_deduplication() -> None:
    sample = {"run_id": 1, "epoch": 3, "stream_sequence": 7, "kind": "READ",
              "address": 0xFF1000, "value": 4, "width": 16}
    assert len(deduplicate_overlap_observations([sample, dict(sample)])) == 1


def _fixture(tmp_path: Path) -> tuple[dict, Path]:
    raw = tmp_path / "flow.bin"; index = tmp_path / "flow.jsonl"
    rows = [
        RECORD.pack(1, 1, 1, 0x1220, 0x1210, 0x4E75, 1 | 8, 0, 2, 0, 0, 0),
        RECORD.pack(2, 2, 2, 0x1220, 0xFF13CC, 1, 0x8000 | (2 << 11), 0, 16, 1, 0, 0),
        RECORD.pack(3, 3, 3, 0x1220, 0xFF13D0, 2, 0x8000 | (2 << 11), 0, 16, 1, 0, 0),
        RECORD.pack(4, 4, 4, 0x1220, 0xFF13D4, 3, 0x8000 | (2 << 11), 0, 16, 1, 0, 0),
    ]
    data = b"".join(rows); raw.write_bytes(data)
    segment = {"run_id": 9, "epoch": 3, "generation": 1, "worker_id": 0, "cycle": 1,
               "entry_frame": None, "record_count": len(rows), "valid": True, "ready_for_cartographer": True}
    index.write_text(json.dumps({"raw_offset": 0, "raw_length": len(data), "segment": segment}) + "\n", encoding="utf-8")
    receipt = {"runtime": {"run_id": 9}, "raw_segment_spool": {
        "raw_path": str(raw), "index_path": str(index), "segments": 1}}
    rom = tmp_path / "rom.bin"; rom.write_bytes(b"\0" * 3145728)
    return receipt, rom


def test_i_j_deterministic_receipt_and_zero_delta(tmp_path: Path) -> None:
    receipt, rom = _fixture(tmp_path)
    provenance = {"exact_dma_to_sat_chains": [{"source": "0xFF13CC", "destination_start": 0xD000,
        "length_bytes": 24, "affected_entries": [0, 1, 2], "causing_pc": "0x0027DE"}]}
    for directory in (tmp_path / "one", tmp_path / "two"):
        directory.mkdir()
        (directory / "postrun_sprite_provenance.json").write_text(json.dumps(provenance), encoding="utf-8")
    first = run_gameplay_stage(receipt, rom, tmp_path / "one", 1)
    second = run_gameplay_stage(receipt, rom, tmp_path / "two", 1)
    assert first["acceptance_ready"]
    assert first["source_owned_delta"] == 0
    left = json.loads(Path(first["receipt_path"]).read_text(encoding="utf-8"))
    right = json.loads(Path(second["receipt_path"]).read_text(encoding="utf-8"))
    assert left == right
    for path in (Path(first["analysis_path"]), Path(first["candidates_path"]),
                 Path(first["entity_sat_links_path"]), Path(first["runtime_instances_path"]),
                 Path(first["provenance_path"])):
        assert not any(word in path.read_text(encoding="utf-8").lower()
                       for word in ("player", "enemy", "npc", "boss", "projectile"))


def test_k_automatic_coordinator_invocation() -> None:
    assert STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES") == STAGES.index("SPRITE / SAT ANALYSIS") + 1
    assert STAGES.index("CONTROLLED ENTITY PROVENANCE") == STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES") + 1
    assert STAGES.index("GENERIC RECURSIVE CLOSURE") == STAGES.index("CONTROLLED ENTITY PROVENANCE") + 1
    assert STAGES.index("ASM CLOSURE") == STAGES.index("GENERIC RECURSIVE CLOSURE") + 1


def test_l_pipeline_adapter_runs_stage_and_keeps_zero_delta(tmp_path: Path) -> None:
    receipt, rom = _fixture(tmp_path)
    out = tmp_path / "pipeline"
    out.mkdir()
    (out / "postrun_sprite_provenance.json").write_text(json.dumps({"exact_dma_to_sat_chains": [
        {"source": "0xFF13CC", "destination_start": 0xD000, "length_bytes": 24,
         "affected_entries": [0, 1, 2], "causing_pc": "0x0027DE"}]}), encoding="utf-8")

    class Progress:
        def __init__(self) -> None:
            self.calls = []
        def start(self, *args, **kwargs): self.calls.append(("start", args, kwargs))
        def update(self, *args, **kwargs): self.calls.append(("update", args, kwargs))
        def finish(self, *args, **kwargs): self.calls.append(("finish", args, kwargs))

    result, stop = run_gameplay_pipeline_stage(receipt, rom, out, 1, Progress(), {})
    assert stop is None and result["source_owned_delta"] == 0
