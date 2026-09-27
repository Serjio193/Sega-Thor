"""Contract tests for M12 controlled-entity provenance."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_controlled_entity_stage import (  # noqa: E402
    analyze_controlled_entity,
    audit_partial_chains,
    build_entity_sat_chain,
    build_input_entity_chains,
    evaluate_lifetime,
    evaluate_player_label,
    recover_entity_fields,
    select_entity_candidate,
)
from live_forward_progress import STAGES  # noqa: E402
import live_forward_controlled_entity_stage as stage  # noqa: E402


def _candidate(primary: bool = False) -> dict:
    return {"candidate_id": "ram-001000-pc-000100", "base": "0x001000", "end": "0x001008",
            "stride": 8, "stride_truth": "DERIVED_EXACT", "slot_index": 0, "frames_observed": [1, 2],
            "write_count": 1, "primary_avatar_witness": primary}


def _link() -> dict:
    return {"candidate_id": "ram-001000-pc-000100", "sat_entries": [0],
            "hardware_sprite_pieces": [{"sat_entry": 0, "frame": 1}], "truth_class": "EXACT"}


def _events() -> list[dict]:
    return [
        {"run_id": 1, "epoch": 1, "stream_sequence": 1, "instruction_sequence": 1,
         "control_flow_sequence": 7, "pc": 0x200, "address": 0xA10003, "value": 0xEF,
         "kind": "READ", "width_bytes": 1, "event_id": "1:1:1:1"},
        {"run_id": 1, "epoch": 1, "stream_sequence": 2, "instruction_sequence": 2,
         "control_flow_sequence": 7, "pc": 0x204, "address": 0xFF165C, "value": 4,
         "kind": "WRITE", "width_bytes": 1, "event_id": "1:1:2:2"},
        {"run_id": 1, "epoch": 1, "stream_sequence": 3, "instruction_sequence": 3,
         "control_flow_sequence": 7, "pc": 0x208, "address": 0xFF165C, "value": 4,
         "kind": "READ", "width_bytes": 1, "event_id": "1:1:3:3"},
        {"run_id": 1, "epoch": 1, "stream_sequence": 4, "instruction_sequence": 4,
         "control_flow_sequence": 7, "pc": 0x20C, "address": 0x001000, "value": 12,
         "kind": "WRITE", "width_bytes": 2, "event_id": "1:1:4:4"},
    ]


def _gameplay() -> dict:
    return {"ram_to_sat": {"exact": [{"ram_source_field": {"address": "0x001000", "width_bytes": 2},
        "sat_shadow_write": {"address": "0x001000", "producer_pc": "0x000300"},
        "sat_entry_field": {"entries": [0], "field": "x"},
        "transfer": {"source_address": "0x001000", "destination_start": "0x00D000",
                      "length_bytes": 8, "causing_pc": "0x000400"}}]}}


def test_input_event_identity_and_exact_consumer() -> None:
    chains = build_input_entity_chains(_events(), _candidate())
    assert len(chains["exact"]) == 1
    chain = chains["exact"][0]
    assert chain["input_event_id"] == "1:1:1:1"
    assert chain["consumer_pc"] == "0x000208"
    assert chain["entity_write_pc"] == "0x00020C"


def test_entity_field_update_and_ram_sat_continuation() -> None:
    fields = recover_entity_fields(_candidate(), _gameplay(), _events())
    assert fields[0]["truth_class"] == "EXACT"
    assert fields[0]["write_pcs"] == ["0x00020C"]
    chains = build_input_entity_chains(_events(), _candidate())
    sat = build_entity_sat_chain(_candidate(), chains, _gameplay(), [_link()])
    assert sat["truth_class"] == "EXACT"
    assert sat["chains"][0]["dma_event_identity"]["destination_start"] == "0x00D000"
    assert sat["chains"][0]["hardware_sprite_piece_proven"] is True


def test_slot_reuse_is_fail_closed() -> None:
    instances = [{"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                  "observation_intervals": [1, 2], "deactivation_reuse_witness": None},
                 {"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                  "observation_intervals": [4, 5], "deactivation_reuse_witness": None}]
    lifetime = evaluate_lifetime(_candidate(), instances)
    assert lifetime["slot_reuse_observed"] is True
    assert lifetime["identity_continuity"] == "UNRESOLVED"


def test_partial_audit_classifies_missing_edges_and_boundary() -> None:
    events = [dict(item, segment=1) for item in _events()[:2]]
    partial = [{"input_event_id": "1:1:1:1", "representation_event_id": "1:1:2:2"}]
    instances = [{"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                  "observation_intervals": [1, 1], "runtime_instance_id": "i0"},
                 {"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                  "observation_intervals": [3, 3], "runtime_instance_id": "i1"}]
    audit = audit_partial_chains(partial, events, _candidate(), instances,
                                 {"frame_boundary_count": 0})
    assert audit["partial_count"] == 1
    assert audit["blocker_counts"] == {"A": 0, "B": 1, "C": 1, "D": 0, "E": 1, "F": 1, "G": 0}
    assert audit["strongest_chain_contract"]["ENTITY_WRITE_PC"] is None
    assert audit["coverage_failure"] == "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT"


def test_player_label_gate_requires_primary_witness() -> None:
    chains = build_input_entity_chains(_events(), _candidate())
    sat = build_entity_sat_chain(_candidate(), chains, _gameplay(), [_link()])
    lifetime = evaluate_lifetime(_candidate(), [{"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                                                 "observation_intervals": [1, 2], "frame_intervals": [1, 2]}])
    assert evaluate_player_label(_candidate(), chains, sat, lifetime)["player_label_proven"] == "NO"
    assert evaluate_player_label(_candidate(True), chains, sat, lifetime)["player_label_proven"] == "YES"


def test_deterministic_analysis_and_zero_source_owned() -> None:
    kwargs = dict(candidates=[_candidate()], exact_links=[_link()], gameplay=_gameplay(),
                  instances=[{"candidate_id": _candidate()["candidate_id"], "slot_index": 0,
                              "observation_intervals": [1, 2], "frame_intervals": [1, 2]}],
                  events=_events(), run_id=1)
    first = analyze_controlled_entity(**kwargs)
    second = analyze_controlled_entity(**kwargs)
    assert json.dumps(first, sort_keys=True) == json.dumps(second, sort_keys=True)
    assert first["source_owned_delta"] == 0
    assert first["acceptance_ready"] is True
    assert first["entity_role"]["entity_role"] == "CONTROLLED_ENTITY"


def test_automatic_stage_order() -> None:
    assert STAGES.index("CONTROLLED ENTITY PROVENANCE") == STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES") + 1
    assert STAGES.index("GENERIC RECURSIVE CLOSURE") == STAGES.index("CONTROLLED ENTITY PROVENANCE") + 1
    assert STAGES.index("ASM CLOSURE") == STAGES.index("GENERIC RECURSIVE CLOSURE") + 1


def test_automatic_coordinator_invocation(monkeypatch) -> None:
    calls = []

    def fake_run(*args, **kwargs):
        calls.append((args, kwargs))
        return {"acceptance_ready": True, "status": "PASS"}

    monkeypatch.setattr(stage, "run_controlled_entity_stage", fake_run)
    progress = SimpleNamespace(start=lambda *args, **kwargs: None,
                               finish=lambda *args, **kwargs: None)
    result, stop = stage.run_controlled_entity_pipeline_stage({}, Path("rom"), Path("out"), 1,
                                                               progress, {"analysis_path": "out/analysis.json"})
    assert stop is None and result["status"] == "PASS" and calls


def _run_pipeline_with_semantic_gap(monkeypatch, tmp_path: Path, gap_stage: str,
                                    cleanup_unabsorbed: bool = False):
    import live_forward_complete_pipeline as pipeline
    import semantic_stage_scheduler as scheduler
    from live_forward_progress import ProgressPublisher

    receipt = tmp_path / "receipt.json"
    receipt.write_text(json.dumps({"runtime": {"run_id": 3},
                                   "raw_segment_spool": {"segments": 1}}), encoding="utf-8")
    decoder = tmp_path / "decoder.exe"
    range_tool = tmp_path / "range.exe"
    decoder.write_bytes(b"")
    range_tool.write_bytes(b"")
    status = tmp_path / "status.json"
    generation = tmp_path / "generation"
    generation.mkdir()
    (generation / "knowledge.sqlite").write_bytes(b"map")
    monkeypatch.setattr(pipeline, "_stage5", lambda *args, **kwargs: {
        "generation_dir": str(generation), "state": "PASS", "replay_status": "PASS",
        "session_sha256": "x", "pointer": {}, "source_owned_delta": 0})
    monkeypatch.setattr(pipeline, "run_audio_stage", lambda *args, **kwargs: {"status": "PASS"})
    monkeypatch.setattr(pipeline, "run_vdp_stage", lambda *args, **kwargs: {"status": "PASS"})
    monkeypatch.setattr(pipeline, "run_sprite_stage", lambda *args, **kwargs: {"status": "PASS"})
    gameplay_reason = ("STOP_GAMEPLAY_ACCEPTANCE_CONTRACT_UNPROVEN"
                       if gap_stage == "gameplay" else None)
    controlled_reason = ("STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT"
                         if gap_stage == "controlled" else None)

    def gameplay(*args, **kwargs):
        args[4].start("GAMEPLAY RAM / ENTITY CANDIDATES")
        args[4].finish("UNRESOLVED" if gameplay_reason else "PASS",
                       detail=gameplay_reason or "accepted")
        return {"status": "STOP" if gameplay_reason else "PASS", "acceptance_ready": not gameplay_reason,
                "analysis_path": str(tmp_path / "analysis.json")}, gameplay_reason

    monkeypatch.setattr(scheduler, "run_gameplay_pipeline_stage", gameplay)
    calls: list[str] = []

    def controlled(*args, **kwargs):
        calls.append("controlled")
        args[4].start("CONTROLLED ENTITY PROVENANCE")
        args[4].finish("UNRESOLVED" if controlled_reason else "PASS",
                       detail=controlled_reason or "accepted")
        return {"status": "STOP" if controlled_reason else "PASS",
                "acceptance_ready": not controlled_reason}, controlled_reason

    monkeypatch.setattr(scheduler, "run_controlled_entity_pipeline_stage", controlled)
    def generic(receipt_value, analysis_dir, rom_path, segments, progress, result):
        calls.append("generic")
        progress.start("GENERIC RECURSIVE CLOSURE")
        progress.finish("NO_DELTA")
        result["stages"]["GENERIC RECURSIVE CLOSURE"] = {"state": "NO_DELTA"}
        return {"state": "NO_DELTA"}, None

    monkeypatch.setattr(scheduler, "run_generic_closure_handoff", generic)
    def stage7(*args, **kwargs):
        calls.append("asm")
        args[-1].start("ASM CLOSURE")
        args[-1].finish("NO_DELTA")
        return {"state": "NO_DELTA", "materialized": str(tmp_path / "materialized")}

    def stage8(*args, **kwargs):
        calls.append("rom-audit")
        args[-1].start("FULL ROM AUDIT")
        args[-1].finish("PASS")
        return {"status": "PASS", "bytes": pipeline.ROM_SIZE}

    def cleanup(*args, **kwargs):
        calls.append("cleanup")
        if cleanup_unabsorbed:
            from live_forward_absorption_cleanup import CleanupStop, STOP_NOT_ABSORBED
            raise CleanupStop(STOP_NOT_ABSORBED, "run absent from rolling-master lineage")
        return {"state": "PASS", "status": "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1",
                "files_deleted": 0}

    monkeypatch.setattr(pipeline, "run_stage7", stage7)
    monkeypatch.setattr(pipeline, "run_stage8", stage8)
    monkeypatch.setattr(pipeline, "reclaim_absorbed_run", cleanup)
    monkeypatch.setattr(pipeline, "reclaim_in_memory_run", cleanup)
    publisher = ProgressPublisher(status)
    for completed in ("FINALIZING RUN", "AUDITING FLOW", "MERGING MASTER", "COMPACTING EVIDENCE"):
        publisher.rows[completed]["state"] = "PASS"
    result = pipeline.run_remaining(receipt, {}, tmp_path, tmp_path / "analysis", tmp_path / "rom.md",
                                    publisher, decoder=decoder, range_tool=range_tool,
                                    stage6_result={"status": "NO_DELTA", "output_generation_or_same": "x"})
    assert publisher.partial is False
    from live_forward_progress import ACCEPTED_TERMINAL, NONFATAL_TERMINAL
    assert all(row["state"] in ACCEPTED_TERMINAL | NONFATAL_TERMINAL
               for row in publisher.rows.values()), {
        name: row["state"] for name, row in publisher.rows.items()
        if row["state"] not in ACCEPTED_TERMINAL | NONFATAL_TERMINAL}
    publisher.complete(report=result)
    return result, calls, json.loads(status.read_text(encoding="utf-8"))


def test_controlled_entity_unresolved_continues_through_cleanup(monkeypatch, tmp_path: Path) -> None:
    result, calls, status = _run_pipeline_with_semantic_gap(monkeypatch, tmp_path, "controlled")
    controlled = result["stages"]["CONTROLLED ENTITY PROVENANCE"]
    assert controlled["stop_reason"] == "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT"
    assert controlled["stage_result"] == "NONFATAL_UNRESOLVED"
    assert controlled["pipeline_effect"] == "CONTINUE_NONFATAL"
    assert calls == ["controlled", "generic", "asm", "rom-audit", "cleanup"]
    assert result["pipeline_state"] == "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
    assert status["overall_state"] == "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE", (
        [(name, row["state"]) for name, row in status["stages"].items()
         if row["state"] not in {"PASS", "NO_DELTA", "SKIPPED_NOT_APPLICABLE", "UNRESOLVED"}],
        status["pipeline_state"])
    assert status["stages"]["CONTROLLED ENTITY PROVENANCE"]["state"] == "UNRESOLVED"


def test_gameplay_acceptance_unresolved_reaches_generic_closure(monkeypatch, tmp_path: Path) -> None:
    result, calls, _ = _run_pipeline_with_semantic_gap(monkeypatch, tmp_path, "gameplay")
    gameplay = result["stages"]["GAMEPLAY RAM / ENTITY CANDIDATES"]
    assert gameplay["stop_reason"] == "STOP_GAMEPLAY_ACCEPTANCE_CONTRACT_UNPROVEN"
    assert gameplay["stage_result"] == "NONFATAL_UNRESOLVED"
    assert "generic" in calls
    assert calls[-3:] == ["asm", "rom-audit", "cleanup"]


def test_unabsorbed_cleanup_retains_raw_and_completes_with_unresolved(monkeypatch, tmp_path: Path) -> None:
    result, calls, status = _run_pipeline_with_semantic_gap(
        monkeypatch, tmp_path, "controlled", cleanup_unabsorbed=True)
    cleanup = result["stages"]["CLEANUP"]
    assert calls[-1] == "cleanup"
    assert cleanup["status"] == "RAW_RETAINED"
    assert cleanup["files_deleted"] == 0
    assert cleanup["reason"].startswith("STOP_CLEANUP_NOT_ABSORBED:")
    assert result["pipeline_state"] == "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
    assert status["stages"]["CLEANUP"]["state"] == "UNRESOLVED"


def test_missing_exact_candidate_is_unresolved_not_pipeline_error(monkeypatch, tmp_path: Path) -> None:
    import live_forward_controlled_entity_stage as controlled
    from live_forward_progress import ProgressPublisher

    monkeypatch.setattr(controlled, "run_controlled_entity_stage",
                        lambda *args, **kwargs: (_ for _ in ()).throw(
                            ValueError("STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING")))
    progress = ProgressPublisher(tmp_path / "status.json")
    result, stop = controlled.run_controlled_entity_pipeline_stage(
        {"runtime": {"run_id": 1}}, tmp_path / "rom.md", tmp_path, 1, progress,
        {"analysis_path": str(tmp_path / "postrun_gameplay_ram_analysis.json")})

    assert stop == "STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING"
    assert result["stage_result"] == "NONFATAL_UNRESOLVED"
    assert result["pipeline_effect"] == "CONTINUE_NONFATAL"
    assert result["state"] == "UNRESOLVED"
    assert result["status"] != "PASS"
    assert progress.rows["CONTROLLED ENTITY PROVENANCE"]["state"] == "UNRESOLVED"


def test_fatal_generic_integrity_stop_blocks_downstream(monkeypatch, tmp_path: Path) -> None:
    import live_forward_complete_pipeline as pipeline
    import semantic_stage_scheduler as scheduler
    from live_forward_progress import ProgressPublisher

    receipt = tmp_path / "receipt.json"
    receipt.write_text(json.dumps({"runtime": {"run_id": 3}, "raw_segment_spool": {"segments": 1}}),
                       encoding="utf-8")
    decoder, range_tool = tmp_path / "decoder.exe", tmp_path / "range.exe"
    decoder.write_bytes(b"")
    range_tool.write_bytes(b"")
    generation = tmp_path / "generation"
    generation.mkdir()
    (generation / "knowledge.sqlite").write_bytes(b"map")
    monkeypatch.setattr(pipeline, "_stage5", lambda *args, **kwargs: {
        "generation_dir": str(generation), "state": "PASS", "replay_status": "PASS",
        "session_sha256": "x", "pointer": {}, "source_owned_delta": 0})
    monkeypatch.setattr(pipeline, "run_audio_stage", lambda *args, **kwargs: {"status": "PASS"})
    monkeypatch.setattr(pipeline, "run_vdp_stage", lambda *args, **kwargs: {"status": "PASS"})
    monkeypatch.setattr(pipeline, "run_sprite_stage", lambda *args, **kwargs: {"status": "PASS"})
    monkeypatch.setattr(scheduler, "run_gameplay_pipeline_stage", lambda *args, **kwargs: ({
        "status": "PASS", "acceptance_ready": True}, None))
    monkeypatch.setattr(scheduler, "run_controlled_entity_pipeline_stage", lambda *args, **kwargs: ({
        "status": "PASS", "acceptance_ready": True}, None))
    downstream: list[str] = []
    def fatal_generic(*args, **kwargs):
        args[4].start("GENERIC RECURSIVE CLOSURE")
        args[4].finish("STOP", detail="STOP_GENERIC_CLOSURE_ROM_IDENTITY_MISMATCH")
        return {"state": "STOP"}, "STOP_GENERIC_CLOSURE_ROM_IDENTITY_MISMATCH"
    monkeypatch.setattr(scheduler, "run_generic_closure_handoff", fatal_generic)
    monkeypatch.setattr(pipeline, "run_stage7", lambda *args, **kwargs: downstream.append("asm"))
    result = pipeline.run_remaining(receipt, {}, tmp_path, tmp_path / "analysis", tmp_path / "rom.md",
        ProgressPublisher(tmp_path / "status.json"), decoder=decoder, range_tool=range_tool,
        stage6_result={"status": "NO_DELTA", "output_generation_or_same": "x"})
    assert result["stop"] == "STOP_GENERIC_CLOSURE_ROM_IDENTITY_MISMATCH"
    assert downstream == []
    assert "ASM CLOSURE" not in result["stages"]
