#!/usr/bin/env python3
"""
M15.248 Phase 1 & 2: Multi-Scenario Hotspot Profiling & ROI Candidate Ranking.
Aggregates unowned original CPU execution across 6 authentic gameplay scenarios:
Scenario C (1800f), A (1592f), B (600f), Longrun (3600f), Longrun Alt (1800f), Longrun Combat (1800f).
Emits:
- data/thor-evidence/m15_248_hotspot_profile_manifest.json
- data/thor-evidence/ranked_callee_candidates_m15_248.json
Strictly <= 300 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.audit_m15_246_candidates import load_m15_245_pcs
from tools.audit_m15_247_candidates import load_m15_246_pcs

CANDIDATE_DATA = [
    (0x00FCB8, "callee_fcb8_subroutine", 1, 1.0, 1.0, "Selected unowned connected math/call subroutine; 399,018 dynamic steps across all 6 scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x00D9A4, "callee_d9a4_subroutine", 1, 1.0, 1.0, "Selected unowned connected math/call subroutine; 398,923 dynamic steps across all 6 scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x002D84, "callee_2d84_coord_helper", 2, 1.0, 1.0, "Selected unowned coordinate transform helper; 716,831 dynamic steps across all 6 scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x003586, "callee_3586_loop", 2, 1.0, 1.0, "Selected high-frequency loop; 640,632 dynamic steps in 3600f longrun.", "SELECTED_UNOWNED_CALLEE"),
    (0x00C8AA, "tilemap_prep_helper", 8, 1.0, 1.0, "Selected tilemap prep helper; 1,957,114 dynamic steps across C, A, LR, Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x00D406, "graphics_line_engine", 12, 1.0, 1.0, "Selected graphics line rendering subroutine; 1,955,412 dynamic steps across C, A, LR, Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x002EE2, "callee_2ee2_bounds_check", 4, 1.0, 1.0, "Selected coordinate bounds check helper; 553,096 dynamic steps across all 6 scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x00F80E, "callee_f80e_query_helper", 5, 1.0, 1.0, "Selected entity parameter query helper; 399,571 dynamic steps across all 6 scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x00A4C8, "movement_velocity_clamp", 4, 1.0, 1.0, "Selected movement velocity clamp; 255,524 dynamic steps across A and Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x002F6E, "screen_coord_offset_helper", 2, 1.0, 1.0, "Selected screen coordinate offset helper; 130,534 dynamic steps in longrun.", "SELECTED_UNOWNED_CALLEE"),
    (0x002E1E, "screen_viewport_math_helper", 3, 1.0, 1.0, "Selected screen viewport math helper; 137,248 dynamic steps in longrun.", "SELECTED_UNOWNED_CALLEE"),
    (0x01A184, "state_town_entity_logic", 3, 1.0, 1.0, "Selected town entity state logic called via 0x00DF1A; 67,008 dynamic steps (842 entries) in A and Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x060004, "sound_fx_audio_dispatch", 3, 1.0, 1.0, "Selected sound FX trigger dispatched from entity handlers; 66,420 dynamic steps (392 entries) across all scenarios.", "SELECTED_UNOWNED_CALLEE"),
    (0x00D7C0, "tilemap_coord_scan_helper", 8, 1.0, 1.0, "Selected tilemap coordinate scan helper; 173,540 dynamic steps across C, A, LR, Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x017908, "directional_jump_table_router", 2, 1.0, 1.0, "Selected directional state router; 18,040 dynamic steps (198 entries) in A and Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x00FCCE, "scenario_c_subroutine_fcce", 4, 1.0, 1.0, "Selected subroutine in Scenario C and Combat; 30,080 dynamic steps.", "SELECTED_UNOWNED_CALLEE"),
    (0x00F96A, "movement_state_logic", 6, 1.0, 1.0, "Selected movement state logic; 19,884 dynamic steps (198 entries) in A and Alt.", "SELECTED_UNOWNED_CALLEE"),
    (0x060008, "sound_driver_bank_selector", 5, 1.0, 1.0, "Sound driver bank selector; 20,268 dynamic steps in longrun.", "SELECTED_UNOWNED_CALLEE"),
    (0x008778, "combat_entity_coord_transform", 3, 1.0, 1.0, "Combat entity coordinate transform; 836 dynamic steps in B and Combat.", "SELECTED_UNOWNED_CALLEE"),
    (0x0083F8, "combat_entity_physics_helper", 4, 1.0, 1.0, "Combat entity physics/hitbox helper; 998 dynamic steps in B and Combat.", "SELECTED_UNOWNED_CALLEE"),
    (0x003820, "nemesis_lz_decompressor", 15, 10.0, 10.0, "Explicitly quarantined graphics decompressor per AGENTS.md.", "QUARANTINED_DECOMPRESSOR"),
    (0x0032E8, "vblank_spin_wait_loop", 1, 10.0, 10.0, "4-insn VBlank spin-wait loop in 0x00307A; non-gameplay wait.", "DEFERRED_BUSY_WAIT"),
    (0x002FDA, "palette_interpolation_math", 4, 5.0, 10.0, "Palette interpolation math helper; causes master cycle drift during VDP DMA (Phase 8 forensics).", "QUARANTINED_DMA_PALETTE"),
    (0x00302E, "palette_fade_loop", 3, 5.0, 10.0, "Palette fade loop; causes master cycle drift during VDP DMA (Phase 8 forensics).", "QUARANTINED_DMA_PALETTE"),
]


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    prod_owned = load_production_pcs(root)
    m244 = load_m15_244_pcs(root)
    m245 = load_m15_245_pcs(root)
    m246 = load_m15_246_pcs(root)
    m247 = json.loads((root / "data/thor-fixtures/m15_247_ir_manifest.json").read_text(encoding="utf-8"))
    m247_pcs = {int(ins["pc"], 16) if isinstance(ins["pc"], str) else int(ins["pc"])
                for blk in m247["blocks"] for ins in blk["instructions"]}

    profiles = {
        "scenario_c": json.loads((root / "build/m15_248/profile_c_profile.json").read_text(encoding="utf-8")),
        "scenario_a": json.loads((root / "build/m15_248/profile_a_profile.json").read_text(encoding="utf-8")),
        "scenario_b": json.loads((root / "build/m15_248/profile_b_profile.json").read_text(encoding="utf-8")),
        "longrun_3600f": json.loads((root / "build/m15_248/profile_lr_profile.json").read_text(encoding="utf-8")),
        "longrun_alt_1800f": json.loads((root / "build/m15_248/profile_alt_profile.json").read_text(encoding="utf-8")),
        "longrun_combat_1800f": json.loads((root / "build/m15_248/profile_combat_profile.json").read_text(encoding="utf-8")),
    }

    hotspot_records = []
    ranked_candidates = []

    for pc, name, cfg_comp, mmio_risk, cyc_risk, rationale, verdict in CANDIDATE_DATA:
        pc_hex = f"0x{pc:06X}"
        total_steps = 0
        total_entries = 0
        scenarios_hit = []
        frames_exercised = 0
        callers = set()
        callees = set()
        unique_pcs = 0

        for sc_name, prof in profiles.items():
            c_data = prof.get("top_callees", {}).get(f"0x{pc:x}", {})
            if not c_data:
                c_data = prof.get("top_callees", {}).get(f"0x{pc:06x}", {})
            if c_data:
                e = c_data.get("entries", 0)
                st = c_data.get("dynamic_steps", 0)
                if e or st:
                    total_entries += e
                    total_steps += st
                    scenarios_hit.append(sc_name)
                    frames_exercised += prof["frames"]
                    callers.update(c_data.get("callers", {}).keys())
                    unique_pcs = max(unique_pcs, c_data.get("unique_pcs", 0))

        if pc in prod_owned:
            owner_status = f"PRODUCTION_HELPER({prod_owned[pc]})"
        elif pc in m244:
            owner_status = "M15_244_PORTABLE"
        elif pc in m245:
            owner_status = "M15_245_PORTABLE"
        elif pc in m246:
            owner_status = "M15_246_PORTABLE"
        elif pc in m247_pcs:
            owner_status = "M15_247_PORTABLE"
        else:
            owner_status = "ORIGINAL_CPU"

        expected_displacement = total_steps if verdict.startswith("SELECTED") else 0
        risk_penalty = mmio_risk * cyc_risk
        roi = round(total_steps / (cfg_comp * risk_penalty), 2) if (cfg_comp * risk_penalty) > 0 else 0.0

        rec = {
            "ROOT_PC": pc_hex,
            "ROUTINE_NAME": name,
            "ENTRY_COUNT": total_entries,
            "DYNAMIC_INSTRUCTION_COUNT": total_steps,
            "UNIQUE_ROM_PC_COUNT": unique_pcs,
            "CALLER_PC": sorted(list(callers))[:5],
            "CALLEE_PC": sorted(list(callees)),
            "SCENARIOS_EXERCISED": scenarios_hit,
            "FRAMES_EXERCISED": frames_exercised,
            "OWNER_STATUS": owner_status,
            "CFG_COMPLEXITY": cfg_comp,
            "MMIO_RISK": "LOW" if mmio_risk == 1.0 else ("MEDIUM" if mmio_risk < 10.0 else "HIGH"),
            "CYCLE_RISK": "LOW" if cyc_risk == 1.0 else ("MEDIUM" if cyc_risk < 10.0 else "HIGH"),
            "EXPECTED_PORTABLE_DISPLACEMENT": expected_displacement,
            "ROI": roi,
            "VERDICT": verdict,
            "RATIONALE": rationale,
        }
        hotspot_records.append(rec)
        ranked_candidates.append(rec)

    ranked_candidates.sort(key=lambda x: (x["VERDICT"].startswith("SELECTED"), x["ROI"]), reverse=True)

    manifest = {
        "schema": "oasis.m15_248.hotspot-profile-manifest.v1",
        "milestone": "M15.248",
        "scenarios_audited": list(profiles.keys()),
        "total_audited_frames": sum(p["frames"] for p in profiles.values()),
        "candidates": hotspot_records,
    }
    m_path = root / "data/thor-evidence/m15_248_hotspot_profile_manifest.json"
    m_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {m_path} ({len(hotspot_records)} candidate records)")

    ranked_manifest = {
        "schema": "oasis.m15_248.ranked-callee-candidates.v1",
        "milestone": "M15.248",
        "ranking_metric": "ROI = (total dynamic instruction executions across all scenarios) / (CFG complexity * risk penalty)",
        "top_20_candidates": ranked_candidates[:20],
        "all_evaluated_candidates": ranked_candidates,
    }
    r_path = root / "data/thor-evidence/ranked_callee_candidates_m15_248.json"
    r_path.write_text(json.dumps(ranked_manifest, indent=2), encoding="utf-8")
    print(f"Wrote {r_path} (Top 20 candidates ranked by ROI)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
