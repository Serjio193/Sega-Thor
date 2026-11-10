#!/usr/bin/env python3
"""
M15.247 Phase 6 Hotspot Reprofiling & Static CFG Boundary Audit.
Audits unowned hotspots across Scenario C, A, B and longrun_3600f,
ranks candidate roots, issues verdicts on 0x0085E2, 0x003820, 0x0032E8, 0x007A28,
and verifies zero overlap with 196 active production helpers and M15.244..246.
Strictly <= 300 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import sys
from typing import Dict, List, Set, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.audit_m15_246_candidates import load_m15_245_pcs
from tools.thor_recompiler.cfg_reconstructor import reconstruct_cfg
from tools.thor_recompiler.decoder_ext_m15_246 import decode_m15_246_at

M15_247_ENTRY_ROOTS = {
    0x005DE4: "state_15_entity_action",
    0x00792A: "state_57_entity_action",
    0x006C3A: "state_9_entity_action",
    0x006844: "state_25_entity_action",
    0x006908: "state_26_entity_action",
    0x00739A: "state_41_entity_action",
    0x0074FC: "state_42_entity_action",
    0x0060B2: "state_54_entity_action",
    0x0079FC: "state_10_entity_action",
    0x0079EE: "state_29_entity_action",
    0x006D58: "state_37_entity_action",
    0x005F8A: "state_48_entity_action",
    0x006F82: "state_49_entity_action",
    0x006FB6: "state_50_entity_action",
    0x006770: "state_55_entity_action",
    0x0066C4: "state_58_entity_action",
    0x005CD8: "state_16_entity_action",
}

M15_247_WINDOWS = [
    (0x005DE4, 0x005E60),
    (0x006C3A, 0x006C46),
    (0x00792A, 0x007972),
    (0x006844, 0x006864),
    (0x006908, 0x006924),
    (0x00739A, 0x0073D0),
    (0x0074FC, 0x007510),
    (0x0060B2, 0x0060CE),
    (0x0079FC, 0x007A10),
    (0x0079EE, 0x0079F8),
    (0x006D58, 0x006D64),
    (0x005F8A, 0x005F92),
    (0x006F82, 0x006F8A),
    (0x006FB6, 0x006FC2),
    (0x006770, 0x006776),
    (0x0066C4, 0x0066CA),
    (0x005CD8, 0x005CE0),
]

EXPLICIT_EXTRA_STOPS_247 = {
    0x005F14, 0x0062CC, 0x008910, 0x003064,
    0x00E2D4, 0x00BCC4, 0x00BCEA, 0x0060004
}


def load_m15_246_pcs(root: Path) -> Set[int]:
    m246 = json.loads((root / "src/tools/thor_ir/m15_246_callee_ir.json").read_text(encoding="utf-8"))
    return {
        int(ins["pc"], 16) if isinstance(ins["pc"], str) else int(ins["pc"])
        for blk in m246["blocks"]
        for ins in blk["instructions"]
    }


def load_pc_counts(prof: dict) -> Dict[int, int]:
    counts: Dict[int, int] = {}
    for row in prof.get("top_non_idle_pcs", []):
        counts[int(row["pc"], 16)] = int(row["count"])
    for k, v in prof.get("entity_region_pc_counts", {}).items():
        counts[int(k, 16)] = int(v)
    return counts


def classify_owner(pc: int, prod_owned: Dict[int, str], m244: Set[int], m245: Set[int], m246: Set[int], m247: Set[int]) -> str:
    if pc in prod_owned:
        return f"PRODUCTION_HELPER({prod_owned[pc]})"
    if pc in m244:
        return "M15_244_PORTABLE"
    if pc in m245:
        return "M15_245_PORTABLE"
    if pc in m246:
        return "M15_246_PORTABLE"
    if pc in m247:
        return "M15_247_PORTABLE"
    return "ORIGINAL_CPU"


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom = (root / "local-roms/Beyond Oasis (USA).md").read_bytes()
    prod_owned = load_production_pcs(root)
    m244 = load_m15_244_pcs(root)
    m245 = load_m15_245_pcs(root)
    m246 = load_m15_246_pcs(root)

    stop_pcs = set(prod_owned.keys()) | m244 | m245 | m246 | EXPLICIT_EXTRA_STOPS_247
    cfg = reconstruct_cfg(
        rom,
        M15_247_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_247_ENTRY_ROOTS.keys()),
        compiled_windows=M15_247_WINDOWS,
        stop_pcs=stop_pcs,
        decode_fn=decode_m15_246_at,
    )
    dmap = cfg["decoded_map"]
    m247_pcs = set(dmap.keys())
    assert not (m247_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (m247_pcs & m244), "M15.244 overlap!"
    assert not (m247_pcs & m245), "M15.245 overlap!"
    assert not (m247_pcs & m246), "M15.246 overlap!"

    prof_c = json.loads((root / "build/m15_245/profile_c.json").read_text(encoding="utf-8"))
    prof_a = json.loads((root / "build/m15_245/profile_a.json").read_text(encoding="utf-8"))
    prof_b = json.loads((root / "build/m15_245/profile_b.json").read_text(encoding="utf-8"))
    prof_lr = json.loads((root / "build/m15_246/longrun_3600f_profile.json").read_text(encoding="utf-8"))

    candidate_specs = [
        (0x005DE4, "SELECTED_UNOWNED_HANDLER", [(0x005DE4, 0x005E60)], "Entity state index 15 (offset 0x1E); 30 entries in authentic C+A+B."),
        (0x00792A, "SELECTED_UNOWNED_HANDLER", [(0x00792A, 0x007972)], "Entity state index 57 (offset 0x72); 24 entries in authentic C+A+B."),
        (0x006C3A, "SELECTED_UNOWNED_HANDLER", [(0x006C3A, 0x006C46)], "Entity state index 9 (offset 0x12); direct bridge to M15.245 0x0062CC."),
        (0x002FDA, "SELECTED_UNOWNED_CALLEE", [(0x002FDA, 0x00302E)], "Palette interpolation math helper; 32 longrun entries, 43,168 dynamic steps."),
        (0x00302E, "SELECTED_UNOWNED_CALLEE", [(0x00302E, 0x003064)], "Palette fade loop; 66 longrun entries, 46,728 dynamic steps; bridges to prod 0x003064."),
        (0x0085E2, "REJECTED_PRODUCTION_OWNED", [], f"Already owned by active production helper {prod_owned.get(0x0085E2, 'UNKNOWN')}."),
        (0x007A28, "DEFERRED_LOW_ROI", [], "Entity state idx 29: 20 C entries, low dynamic cycle yield compared to math/callees."),
        (0x0032E8, "DEFERRED_BUSY_WAIT", [], "4-insn VBlank spin-wait loop in 0x00307A; not gameplay logic."),
        (0x003820, "QUARANTINED_DECOMPRESSOR", [], "Nemesis/LZ decompressor explicitly quarantined by AGENTS.md."),
    ]

    lr_top = prof_lr.get("top_callees", {})
    audit_entries = []
    for pc, status, win_list, rationale in candidate_specs:
        lr_key = f"0x{pc:x}"
        lr = lr_top.get(lr_key, {})
        th_lr = prof_lr.get("table_handlers", {}).get(lr_key, {})
        th_c = prof_c.get("table_handlers", {}).get(lr_key, {})
        th_a = prof_a.get("table_handlers", {}).get(lr_key, {})
        th_b = prof_b.get("table_handlers", {}).get(lr_key, {})
        win_pcs = [p for p in m247_pcs if any(s <= p < e for s, e in win_list)]
        audit_entries.append({
            "pc": f"0x{pc:06X}",
            "status": status,
            "current_owner": classify_owner(pc, prod_owned, m244, m245, m246, m247_pcs),
            "rationale": rationale,
            "compiled_static_insns": len(win_pcs),
            "table_handler_entries": {
                "scenario_c": th_c.get("entries", 0),
                "scenario_a": th_a.get("entries", 0),
                "scenario_b": th_b.get("entries", 0),
                "longrun_3600f": th_lr.get("entries", 0),
            },
            "longrun_3600f": {
                "entries": lr.get("entries", 0),
                "dynamic_steps": lr.get("dynamic_steps", 0),
                "unique_pcs": lr.get("unique_pcs", 0),
            },
        })

    boundary_exits = []
    for edge in cfg["edges"]:
        if edge["kind"] in ("UNCOMPILED_EXIT", "CALL_UNCOMPILED"):
            to_pc = edge["to_pc"]
            if to_pc == "INDIRECT":
                owner = "INDIRECT_VIA_A2(RETURNS_TO_NEXT_PC)"
            else:
                owner = classify_owner(int(to_pc, 16), prod_owned, m244, m245, m246, m247_pcs)
            boundary_exits.append({**edge, "target_owner": owner})

    manifest = {
        "schema": "oasis.m15_247.ranked-callee-candidates.v1",
        "milestone": "M15.247",
        "selected_roots": {f"0x{k:06X}": v for k, v in sorted(M15_247_ENTRY_ROOTS.items())},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_247_WINDOWS],
        "cfg_summary": {
            "compiled_instructions": len(m247_pcs),
            "basic_blocks": len(cfg["basic_blocks"]),
            "edges": len(cfg["edges"]),
            "edge_counts": cfg["edge_summary"],
            "production_overlap_count": len(m247_pcs & set(prod_owned.keys())),
            "m15_244_overlap_count": len(m247_pcs & m244),
            "m15_245_overlap_count": len(m247_pcs & m245),
            "m15_246_overlap_count": len(m247_pcs & m246),
        },
        "candidates_and_verdicts": audit_entries,
        "boundary_exits": boundary_exits,
    }
    out_p = root / "data/thor-evidence/ranked_callee_candidates_m15_247.json"
    out_p.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out_p}: {len(m247_pcs)} insns, {len(cfg['basic_blocks'])} blocks, {len(cfg['edges'])} edges, 0 overlaps")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
