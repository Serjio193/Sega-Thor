#!/usr/bin/env python3
"""
Deterministic Verification Suite for M15.259 ROM Room Descriptor, Spawn VM,
and Multi-Scene Map Transition Semantic Extraction.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from __future__ import annotations
import json
import pathlib
import sys


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    json_path = repo_root / "data" / "thor-evidence" / "rom_room_transition_semantics_m15_259.json"
    assert json_path.exists(), f"Missing extracted semantics JSON: {json_path}"

    data = json.loads(json_path.read_text(encoding="utf-8"))
    assert data["milestone"] == "M15.259"
    assert data["rom_sha256"] == "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

    sm = data["summary_metrics"]
    assert sm["core_routines_analyzed"] == 15
    assert sm["room_handler_functions_analyzed"] == 558
    assert sm["total_rom_functions_analyzed"] == 573
    assert sm["areas_extracted"] == 21
    assert sm["room_slots_extracted"] == 188
    assert sm["unique_room_descriptors_extracted"] == 186
    assert sm["state_transitions_extracted"] == 448
    assert sm["tables_extracted"] == 25
    assert sm["vm_opcodes_extracted"] == 33
    assert sm["shared_tile_banks_extracted"] == 65
    assert sm["d406_coverage_ratio"] == "186/186"

    # Verify all 186 room descriptors have valid handler PCs and call 0x00D406
    by_room: dict[str, dict] = {}
    for rd in data["room_descriptors"]:
        assert rd["calls_00d406"] is True, f"Descriptor {rd['descriptor_addr']} missing 0x00D406 call"
        for rid in rd["room_ids"]:
            by_room[rid] = rd
    assert len(by_room) == 188

    # Verify canonical Area 1 multi-scene chain 0x0100 -> 0x0101 -> 0x0102 -> 0x0103 -> 0x0104 -> 0x0105
    r0100 = by_room["0x0100"]
    assert r0100["descriptor_addr"] == "0x02E57E"
    assert r0100["map_stream_rom_addr"] == "0x211F7A"
    assert any(t["target_room_effective"] == "0x0101" and t["trigger_detail"].get("threshold_px") == 108 for t in r0100["transitions"])

    r0101 = by_room["0x0101"]
    assert r0101["descriptor_addr"] == "0x02E612"
    assert r0101["map_stream_rom_addr"] == "0x212684"
    assert any(t["target_room_effective"] == "0x0102" and t["trigger_detail"].get("threshold_px") == 108 for t in r0101["transitions"])

    r0102 = by_room["0x0102"]
    assert r0102["descriptor_addr"] == "0x02E748"
    assert r0102["map_stream_rom_addr"] == "0x212CBA"
    assert any(t["target_room_effective"] == "0x0103" and t["trigger_detail"].get("threshold_px") == 76 for t in r0102["transitions"])
    assert sum(1 for t in r0102["transitions"] if t["trigger_type"] == "AABB_00C1CE") == 4

    r0103 = by_room["0x0103"]
    assert r0103["descriptor_addr"] == "0x02E88A"
    assert r0103["map_stream_rom_addr"] == "0x213850"
    assert r0103["spawn_script"]["script_addr"] == "0x0464EE"
    assert len(r0103["spawn_script"]["preload_dma_streams"]) == 2
    assert any(t["target_room_effective"] == "0x0104" and t["trigger_detail"].get("threshold_px") == 108 for t in r0103["transitions"])
    assert any(t["target_room_effective"] == "0x0102" and t["trigger_detail"].get("threshold_px") == 500 for t in r0103["transitions"])

    r0104 = by_room["0x0104"]
    assert r0104["descriptor_addr"] == "0x02E972"
    assert r0104["map_stream_rom_addr"] == "0x213FE8"
    assert r0104["layout_flags_16"] == "0x0101"
    assert any(t["target_room_effective"] == "0x0105" and t["trigger_detail"].get("threshold_px") == 108 for t in r0104["transitions"])
    assert any(t["target_room_effective"] == "0x0103" and t["trigger_detail"].get("threshold_px") == 500 for t in r0104["transitions"])

    r0105 = by_room["0x0105"]
    assert r0105["descriptor_addr"] == "0x02EB3E"
    assert r0105["map_stream_rom_addr"] == "0x214752"
    assert any(t["target_room_effective"] == "0x0107" and t["trigger_detail"].get("threshold_px") == 108 for t in r0105["transitions"])
    assert any(t["target_room_effective"] == "0x0104" and t["trigger_detail"].get("threshold_px") == 308 for t in r0105["transitions"])

    # Verify all 448 transitions have resolved target rooms and spawn expressions
    for rd in data["room_descriptors"]:
        for tr in rd["transitions"]:
            assert tr["target_room_raw"] != "UNKNOWN"
            assert tr["spawn_x_expr"] is not None
            assert tr["spawn_y_expr"] is not None

    # Verify all 6 GPGX validation scenarios match expected transition counts with 0 errors
    gpgx = data["gpgx_validation_scenarios"]
    assert len(gpgx) == 6
    for scen, sdata in gpgx.items():
        assert sdata["transition_count_exact_match"] is True, f"Transition mismatch in {scen}"
        assert sdata["rom_fallback_errors"] == 0, f"Fallback error in {scen}"

    print("ROM_ROOM_TRANSITION_SEMANTICS_TEST: ALL ASSERTIONS PASSED (573 functions, 188 rooms, 448 transitions, 6 GPGX scenarios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
