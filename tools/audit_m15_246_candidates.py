#!/usr/bin/env python3
"""
M15.246 Phase 4 & 5 Hotspot Reprofiling & Static CFG Boundary Audit.
Audits longrun_3600f + Scenarios C/A/B profiles, issues explicit verdicts on
0x0085E2, 0x00BCBA, 0x007A28, 0x0032E8, 0x003820, and verifies the connected
M15.246 unowned callee subgraph has zero production/M15.244/M15.245 overlap.
Strictly <= 300 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import sys
from typing import Dict, Set

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.thor_recompiler.cfg_reconstructor import reconstruct_cfg
from tools.thor_recompiler.decoder_ext_m15_246 import decode_m15_246_at

M15_246_ENTRY_ROOTS = {
    0x00F0D2: "callee_f0d2_dist2d",
    0x00E6BA: "callee_e6ba_isqrt",
    0x006DD2: "state_3_entity_action",
    0x00BCBA: "callee_bcba_hitbox_scan",
    0x00BCD8: "callee_bcd8_hitbox_alt",
    0x015AE2: "callee_15ae2_entity_phys",
    0x02ED20: "callee_2ed20_viewport_proj",
}

M15_246_WINDOWS = [
    (0x00E6BA, 0x00E6F0),
    (0x00F0D2, 0x00F0EC),
    (0x006DD2, 0x006E28),
    (0x00BCBA, 0x00BE98),
    (0x015AE2, 0x015C4A),
    (0x02ED20, 0x02EEA0),
]

EXPLICIT_EXTRA_STOPS = {0x015C4A, 0x016306, 0x01634A, 0x0163A4, 0x00BE98}


def load_m15_245_pcs(root: Path) -> Set[int]:
    m245 = json.loads((root / "data" / "thor-fixtures" / "m15_245_ir_manifest.json").read_text(encoding="utf-8"))
    return {
        int(ins["pc"], 16) if isinstance(ins["pc"], str) else int(ins["pc"])
        for blk in m245["blocks"]
        for ins in blk["instructions"]
    }


def load_pc_counts(prof: dict) -> Dict[int, int]:
    counts: Dict[int, int] = {}
    for row in prof.get("top_non_idle_pcs", []):
        counts[int(row["pc"], 16)] = int(row["count"])
    for k, v in prof.get("entity_region_pc_counts", {}).items():
        counts[int(k, 16)] = int(v)
    return counts


def classify_owner(pc: int, prod_owned: Dict[int, str], m244: Set[int], m245: Set[int], m246: Set[int]) -> str:
    if pc in prod_owned:
        return f"PRODUCTION_HELPER({prod_owned[pc]})"
    if pc in m244:
        return "M15_244_PORTABLE"
    if pc in m245:
        return "M15_245_PORTABLE"
    if pc in m246:
        return "M15_246_PORTABLE"
    return "ORIGINAL_CPU"


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom = (root / "local-roms" / "Beyond Oasis (USA).md").read_bytes()
    prod_owned = load_production_pcs(root)
    m244 = load_m15_244_pcs(root)
    m245 = load_m15_245_pcs(root)

    stop_pcs = set(prod_owned.keys()) | m244 | m245 | EXPLICIT_EXTRA_STOPS
    cfg = reconstruct_cfg(
        rom,
        M15_246_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_246_ENTRY_ROOTS.keys()),
        compiled_windows=M15_246_WINDOWS,
        stop_pcs=stop_pcs,
        decode_fn=decode_m15_246_at,
    )
    dmap = cfg["decoded_map"]
    m246_pcs = set(dmap.keys())
    assert not (m246_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (m246_pcs & m244), "M15.244 overlap!"
    assert not (m246_pcs & m245), "M15.245 overlap!"

    prof_c = json.loads((root / "build" / "m15_245" / "profile_c.json").read_text(encoding="utf-8"))
    prof_a = json.loads((root / "build" / "m15_245" / "profile_a.json").read_text(encoding="utf-8"))
    prof_b = json.loads((root / "build" / "m15_245" / "profile_b.json").read_text(encoding="utf-8"))
    prof_lr = json.loads((root / "build" / "m15_246" / "longrun_3600f_profile.json").read_text(encoding="utf-8"))

    pc_c = load_pc_counts(prof_c)
    pc_a = load_pc_counts(prof_a)
    pc_b = load_pc_counts(prof_b)

    candidate_specs = [
        (0x00F0D2, "SELECTED_UNOWNED_CALLEE", [(0x00F0D2, 0x00F0EC)], "2D distance helper calling 0x00E6BA; 0 overlap; hot math ROI."),
        (0x00E6BA, "SELECTED_UNOWNED_CALLEE", [(0x00E6BA, 0x00E6F0)], "32-bit integer square root called by 0x00F0D2."),
        (0x006DD2, "SELECTED_UNOWNED_HANDLER", [(0x006DD2, 0x006E28)], "Entity state index 3 (offset 0x06); bridges M15.246 -> M15.245 & 0x0085E2."),
        (0x00BCBA, "SELECTED_UNOWNED_CALLEE", [(0x00BCBA, 0x00BE98)], "Hitbox/entity collision scan called by M15.244/245; prompt target; includes 0x00BCD8."),
        (0x00BCD8, "SELECTED_UNOWNED_CALLEE", [(0x00BCBA, 0x00BE98)], "Secondary entrypoint into 0x00BCBA hitbox scan called by M15.245."),
        (0x015AE2, "SELECTED_UNOWNED_CALLEE", [(0x015AE2, 0x015C4A)], "Entity ground physics/displacement helper entering viewport tail."),
        (0x02ED20, "SELECTED_UNOWNED_CALLEE", [(0x02ED20, 0x02EEA0)], "Viewport/screen projection routine calling prod 0x02EDCA and 0x02EEA0."),
        (0x0085E2, "REJECTED_PRODUCTION_OWNED", [], f"Already owned by active production helper {prod_owned.get(0x0085E2, 'UNKNOWN')}."),
        (0x007A28, "DEFERRED_LOW_ROI", [], "Entity state idx 29: 20 C / 20 3600f entries (~227 C steps), 28 uncompiled/complex ops."),
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
        win_pcs = [p for p in m246_pcs if any(s <= p < e for s, e in win_list)]
        audit_entries.append({
            "pc": f"0x{pc:06X}",
            "status": status,
            "current_owner": classify_owner(pc, prod_owned, m244, m245, m246_pcs),
            "rationale": rationale,
            "compiled_static_insns": len(win_pcs),
            "observed_entity_region_c_steps": sum(pc_c.get(p, 0) for p in win_pcs) if win_pcs else pc_c.get(pc, 0),
            "observed_entity_region_a_steps": sum(pc_a.get(p, 0) for p in win_pcs) if win_pcs else pc_a.get(pc, 0),
            "observed_entity_region_b_steps": sum(pc_b.get(p, 0) for p in win_pcs) if win_pcs else pc_b.get(pc, 0),
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
                "guest_cycles": lr.get("guest_cycles", 0),
                "ram_writes": lr.get("ram_writes", 0),
                "nonlocal_returns": lr.get("nonlocal_returns", 0),
                "callers": lr.get("callers", {}),
                "return_pcs": lr.get("return_pcs", {}),
                "call_depths": lr.get("call_depths", {}),
                "entity_w0": lr.get("entity_w0", {}),
                "entity_states": lr.get("entity_states", {}),
            },
        })

    boundary_exits = []
    for edge in cfg["edges"]:
        if edge["kind"] in ("UNCOMPILED_EXIT", "CALL_UNCOMPILED"):
            to_pc = edge["to_pc"]
            if to_pc == "INDIRECT":
                owner = "INDIRECT_VIA_A2(RETURNS_TO_NEXT_PC)"
            else:
                owner = classify_owner(int(to_pc, 16), prod_owned, m244, m245, m246_pcs)
            boundary_exits.append({**edge, "target_owner": owner})

    manifest = {
        "schema": "oasis.m15_246.callee-hotspot-manifest.v1",
        "milestone": "M15.246",
        "selected_roots": {f"0x{k:06X}": v for k, v in sorted(M15_246_ENTRY_ROOTS.items())},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_246_WINDOWS],
        "cfg_summary": {
            "compiled_instructions": len(m246_pcs),
            "basic_blocks": len(cfg["basic_blocks"]),
            "edges": len(cfg["edges"]),
            "edge_counts": cfg["edge_summary"],
            "production_overlap_count": len(m246_pcs & set(prod_owned.keys())),
            "m15_244_overlap_count": len(m246_pcs & m244),
            "m15_245_overlap_count": len(m246_pcs & m245),
        },
        "exception_paths": [
            {"pc": f"0x{ins.pc:06X}", "op": ins.mnemonic, "vector": 5, "note": "Zero-divide trap Vector 5 (0x0014)"}
            for ins in dmap.values() if ins.mnemonic in ("DIVU", "DIVS")
        ],
        "candidates_and_verdicts": audit_entries,
        "boundary_exits": boundary_exits,
    }
    out_p = root / "data" / "thor-evidence" / "callee_hotspot_manifest_m15_246.json"
    out_p.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out_p}: {len(m246_pcs)} insns, {len(cfg['basic_blocks'])} blocks, {len(cfg['edges'])} edges, 0 overlaps")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
