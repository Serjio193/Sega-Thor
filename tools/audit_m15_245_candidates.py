#!/usr/bin/env python3
"""
M15.245 Phase 1 & Phase 2: Real Gameplay Hotspot Profiling & Target Boundary/Ownership Audit.
Combines pinned GPGX execution profiles across Scenarios C (1,800f), A (1,592f), B (600f)
with static M68000 CFG/decoder boundary analysis and active 196-helper ownership checks.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter
from tools.thor_recompiler.decoder import decode_at

PROMPT_TARGETS = [0x006516, 0x0061F6, 0x0062E4, 0x006338, 0x007112]


def load_production_pcs(root: Path) -> dict[int, str]:
    active = set(StatusAdapter().active_set)
    owned: dict[int, str] = {}
    for p in sorted((root / "src" / "tools" / "hybrid").glob("m68k_*.cpp")):
        m = re.match(r"m68k_([0-9a-fA-F]{6})", p.name)
        if not m or int(m.group(1), 16) not in active:
            continue
        txt = p.read_text(encoding="utf-8", errors="ignore")
        for c in re.findall(r"case\s+0x([0-9a-fA-F]+)U?\s*:", txt):
            owned[int(c, 16)] = p.name
    return owned


def load_m15_244_pcs(root: Path) -> set[int]:
    ir = json.loads((root / "src" / "tools" / "thor_ir" / "entity_dispatch_subsystem_ir.json").read_text(encoding="utf-8"))
    pcs: set[int] = set()
    for blk in ir["blocks"]:
        for ins in blk["instructions"]:
            pcs.add(int(ins["pc"], 16))
    return pcs


def audit_target_boundaries(rom: bytes, entry_pc: int, prod_owned: dict[int, str]) -> dict:
    """BFS static boundary/decoder/ownership trace strictly from decoded instructions."""
    q = [entry_pc]
    seen: dict[int, object] = {}
    unsup: dict[int, tuple[str, str]] = {}
    calls: set[int] = set()
    tails: list[tuple[str, int]] = []
    returns: list[int] = []
    mmio_pcs: list[int] = []
    block_starts: set[int] = {entry_pc}

    while q and len(seen) < 220:
        pc = q.pop(0)
        while pc not in seen and pc not in unsup and len(seen) < 220:
            if pc in prod_owned and pc != entry_pc:
                tails.append(("FALL_INTO_PROD", pc))
                break
            try:
                ins = decode_at(rom, pc)
            except Exception as ex:
                w0 = struct.unpack_from(">H", rom, pc)[0]
                unsup[pc] = (f"0x{w0:04X}", str(ex))
                break
            seen[pc] = ins
            nxt = pc + len(ins.raw_words) * 2
            for op in ins.ops:
                if op.kind == "abs_l" and op.val >= 0xC00000:
                    mmio_pcs.append(pc)
            if ins.mnemonic in ("RTS", "RTE"):
                returns.append(pc)
                break
            if ins.mnemonic == "JMP":
                if ins.ops and ins.ops[0].kind == "abs_l":
                    tails.append(("JMP", ins.ops[0].val))
                else:
                    tails.append(("JMP_IND", pc))
                break
            if ins.mnemonic == "BRA":
                tgt = ins.branch_target
                if tgt is not None and entry_pc <= tgt < entry_pc + 0x240 and tgt not in prod_owned:
                    block_starts.add(tgt)
                    q.append(tgt)
                else:
                    tails.append(("BRA", tgt or 0))
                break
            if ins.mnemonic in ("BSR", "JSR"):
                if ins.mnemonic == "BSR" and ins.branch_target is not None:
                    calls.add(ins.branch_target)
                elif ins.ops and ins.ops[0].kind == "abs_l":
                    calls.add(ins.ops[0].val)
                else:
                    calls.add(0xFFFFFFFF)
                block_starts.add(nxt)
                pc = nxt
            elif ins.branch_target is not None:
                tgt = ins.branch_target
                block_starts.add(tgt)
                block_starts.add(nxt)
                if entry_pc - 0x40 <= tgt < entry_pc + 0x240 and tgt not in prod_owned:
                    q.append(tgt)
                else:
                    tails.append((ins.mnemonic, tgt))
                pc = nxt
            else:
                pc = nxt

    prod_overlap_pcs = sorted(p for p in seen if p in prod_owned)
    max_end = max((p + len(seen[p].raw_words) * 2 for p in seen), default=entry_pc)
    return {
        "entry_pc": f"0x{entry_pc:06X}",
        "min_pc": f"0x{min(seen, default=entry_pc):06X}",
        "max_end_pc": f"0x{max_end:06X}",
        "decoded_instructions": len(seen),
        "estimated_blocks": len(block_starts),
        "callees": [f"0x{c:06X}" for c in sorted(calls)],
        "tail_exits": [[k, f"0x{v:06X}"] for k, v in sorted(set(tails))],
        "return_pcs": [f"0x{r:06X}" for r in sorted(returns)],
        "mmio_access_pcs": [f"0x{m:06X}" for m in mmio_pcs],
        "production_overlap_count": len(prod_overlap_pcs),
        "production_overlap_sample": [f"0x{p:06X}({prod_owned[p]})" for p in prod_overlap_pcs[:6]],
        "unsupported_opcodes": {f"0x{k:06X}": {"word": v[0], "error": v[1]} for k, v in sorted(unsup.items())},
        "clean_boundary": len(unsup) == 0 and len(prod_overlap_pcs) == 0,
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom = (root / "local-roms" / "Beyond Oasis (USA).md").read_bytes()
    prod_owned = load_production_pcs(root)
    m244_pcs = load_m15_244_pcs(root)

    prof_c = json.loads((root / "build" / "m15_245" / "profile_c.json").read_text(encoding="utf-8"))
    prof_a = json.loads((root / "build" / "m15_245" / "profile_a.json").read_text(encoding="utf-8"))
    prof_b = json.loads((root / "build" / "m15_245" / "profile_b.json").read_text(encoding="utf-8"))

    all_obs_pcs = sorted(
        {int(k, 16) for k in prof_c["table_handlers"]}
        | {int(k, 16) for k in prof_a["table_handlers"]}
        | {int(k, 16) for k in prof_b["table_handlers"]}
        | set(PROMPT_TARGETS)
    )

    candidates = []
    for pc in all_obs_pcs:
        hx = f"0x{pc:x}"
        hc = prof_c["table_handlers"].get(hx, {})
        ha = prof_a["table_handlers"].get(hx, {})
        hb = prof_b["table_handlers"].get(hx, {})
        ent_c, ent_a, ent_b = hc.get("entries", 0), ha.get("entries", 0), hb.get("entries", 0)
        dyn_c, dyn_a, dyn_b = hc.get("dynamic_steps", 0), ha.get("dynamic_steps", 0), hb.get("dynamic_steps", 0)
        binfo = audit_target_boundaries(rom, pc, prod_owned)
        in_m244 = pc in m244_pcs
        is_prod = binfo["production_overlap_count"] > 0
        status = "PRODUCTION_OVERLAP" if is_prod else ("ALREADY_M15_244" if in_m244 else "UNOWNED_CANDIDATE")
        candidates.append({
            "target_pc": f"0x{pc:06X}",
            "prompt_candidate": pc in PROMPT_TARGETS,
            "ownership_status": status,
            "entries_c": ent_c, "entries_a": ent_a, "entries_b": ent_b,
            "entries_total": ent_c + ent_a + ent_b,
            "dynamic_steps_c": dyn_c, "dynamic_steps_a": dyn_a, "dynamic_steps_b": dyn_b,
            "dynamic_steps_total": dyn_c + dyn_a + dyn_b,
            "unique_pcs_c": hc.get("unique_pc_count", 0),
            "ram_writes_c": hc.get("ram_writes", 0),
            "guest_cycles_c": hc.get("guest_cycles", 0),
            "entity_w0_c": hc.get("entity_w0", {}),
            "callers_c": hc.get("callers", {}),
            "return_pcs_c": hc.get("return_pcs", {}),
            "boundary_audit": binfo,
        })

    candidates.sort(key=lambda r: (r["ownership_status"] != "UNOWNED_CANDIDATE", -r["dynamic_steps_total"]))

    manifest = {
        "schema": "oasis.m15_245.hotspot-manifest.v1",
        "milestone": "M15.245",
        "scenarios": {
            "scenario_c": {k: prof_c[k] for k in ("frames", "total_original_steps", "idle_steps", "non_idle_steps", "caller_005670", "dispatcher_0059a8", "timer_00e682")},
            "scenario_a": {k: prof_a[k] for k in ("frames", "total_original_steps", "idle_steps", "non_idle_steps", "caller_005670", "dispatcher_0059a8", "timer_00e682")},
            "scenario_b": {k: prof_b[k] for k in ("frames", "total_original_steps", "idle_steps", "non_idle_steps", "caller_005670", "dispatcher_0059a8", "timer_00e682")},
        },
        "candidates": candidates,
    }
    out_p = root / "data" / "thor-evidence" / "entity_handler_hotspot_manifest_m15_245.json"
    out_p.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print(f"Wrote {out_p} ({len(candidates)} handlers audited)")
    for c in candidates:
        b = c["boundary_audit"]
        print(f"{c['target_pc']} [{c['ownership_status']:<18}] ent={c['entries_total']:>3} dyn(C/A/B)={c['dynamic_steps_c']:>5}/{c['dynamic_steps_a']:>5}/{c['dynamic_steps_b']:>5} ok={b['decoded_instructions']:>3} ov={b['production_overlap_count']:>2} calls={len(b['callees'])} unsup={list(b['unsupported_opcodes'].values())[:3]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
