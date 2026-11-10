#!/usr/bin/env python3
"""
M15.249 Reference Fixture Generator (100% real pinned GPGX ca4be304...).
Expands reference fixture suite to qualify M15.246 unhit CFG edges,
and qualifies all 7 M15.249 hotpath routines.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import subprocess

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CORE_SHA256 = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134"
ORACLE_BIN = "/mnt/c/Github/Sega-Thor/build/m15_246/gpgx_oracle"
CORE_WSL = "/mnt/c/Github/Sega-Thor/build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so"
ROM_WSL = "/mnt/c/Github/Sega-Thor/local-roms/Beyond Oasis (USA).md"


def run_probe(entry_pc: int, stop_pc: int, sr: int, d: list[int], a: list[int], pokes: list[tuple[int, int, int]]) -> dict:
    cmd = ["wsl", ORACLE_BIN, "probe-case", CORE_WSL, ROM_WSL, f"{entry_pc:06X}", f"{stop_pc:06X}", f"{sr:04X}"]
    cmd.extend(f"{x:08X}" for x in d)
    cmd.extend(f"{x:08X}" for x in a)
    cmd.extend(f"{addr:06X}:{w}={val:X}" for addr, w, val in pokes)
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60, check=True)
    return json.loads(proc.stdout.strip().splitlines()[-1])


def make_ctrl(tid: str, entry_pc: int, stop_pc: int, sr: int, ed: list[int], ea: list[int],
              pokes: list[tuple[int, int, int]], outcome_override: str | None = None) -> dict:
    p = run_probe(entry_pc, stop_pc, sr, ed, ea, pokes)
    ram_init = []
    for addr, w, val in pokes:
        for b in range(w):
            ram_init.append({"addr": f"0x{addr + b:06X}", "val": (val >> ((w - 1 - b) * 8)) & 0xFF})
    return {
        "test_id": tid,
        "classification": "CONTROLLED_CPU_ENTRY",
        "scenario": "isolated_gpgx_probe",
        "frame": 0,
        "rom_sha256": ROM_SHA256,
        "core_sha256": CORE_SHA256,
        "checkpoint_sha256": "",
        "input_journal_sha256": "",
        "entry_pc": f"0x{entry_pc:06X}",
        "entry_d": [f"0x{x:08X}" for x in ed],
        "entry_a": [f"0x{x:08X}" for x in ea],
        "entry_sr": f"0x{sr:04X}",
        "ram_init": ram_init,
        "expected_exit_pc": p["exit_pc"],
        "expected_outcome": outcome_override or p["outcome"],
        "expected_d": p["exit_d"],
        "expected_a": p["exit_a"],
        "expected_sr": p["exit_sr"],
        "expected_cycles": p["cycles"],
        "allow_refresh_jitter": False,
        "pc_trace": p["pc_trace"],
        "writes": p.get("writes", []),
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent

    # Load 112 baseline fixtures from M15.248
    base_file = root / "data" / "thor-fixtures" / "m15_248_callee_fixtures.json"
    fixtures = json.loads(base_file.read_text(encoding="utf-8"))["fixtures"]

    def regs(d0=0, d1=0, d2=0, d3=0, d4=0, d5=0, d6=0, d7=0,
             a0=0xFF1000, a1=0xFF1200, a2=0xFF1400, a3=0xFF1600, a4=0xFF1800, a5=0xFF1A00, a6=0xFF8000, a7=0xFFFE00):
        return [d0, d1, d2, d3, d4, d5, d6, d7], [a0, a1, a2, a3, a4, a5, a6, a7]

    print("Generating M15.249 targeted M15.246 CFG edge expansion probes...")
    # 1. 0x00BE20..0x00BE5C AABB bounding box collision branches
    aabb_base = [(0xFFFE00, 4, 0x00FF2000), (0xFF2044, 2, 10), (0xFF200C, 2, 50), (0xFF2014, 2, 0), (0xFF1940, 4, 0x0000BE5C)]

    # 1a. D1 > D6 -> taken at 0xBE24
    d, a = regs(d1=100, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_d1_gt_d6", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 0)]))

    # 1b. D1 <= D6, D3 < D5 -> fallthrough 0xBE24, taken 0xBE2A
    d, a = regs(d1=40, d3=20, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_d3_lt_d5", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 0)]))

    # 1c. D1 <= D6, D3 >= D5, D2 > D6 -> fallthrough to 0xBE42, taken
    d, a = regs(d1=40, d2=100, d3=60, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_d2_gt_d6", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 0)]))

    # 1d. D1 <= D6, D3 >= D5, D2 <= D6, D4 < D5 -> fallthrough to 0xBE48, taken
    d, a = regs(d1=40, d2=40, d3=60, d4=20, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_d4_lt_d5", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 0)]))

    # 1e. D1 <= D6, D3 >= D5, D2 <= D6, D4 >= D5, t130 != 0 -> fallthrough to 0xBE50, taken
    d, a = regs(d1=40, d2=40, d3=60, d4=60, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_t130_ne", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 1)]))

    # 1f. Full collision hit -> fallthrough to 0xBE54 uncompiled call
    d, a = regs(d1=40, d2=40, d3=60, d4=60, d5=50, d6=50, a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_be20_hit_jsr", 0x00BE22, 0x00BE5E, 0x2100, d, a, aabb_base + [(0xFF2082, 1, 0)]))

    # 2. 0x015C44 -> 0x02ED20 unconditional tail jump
    d, a = regs()
    fixtures.append(make_ctrl("ctrl_15c44_tail_jump", 0x015C40, 0x02ED20, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)]))

    print("Generating M15.249 hotpath probes...")
    # 3. 0x0254EA probes (Water Shrine enemy controller)
    # 3a. 102(A6) == 0 -> taken 0x0254EE -> 0x0256B8 uncompiled exit
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_254ea_st0_uncompiled", 0x0254EA, 0x0256B8, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 0)
    ], "ValidUncompiledPc"))

    # 3b. 102(A6) == 1 -> state 1 lookup table to 0x0255D8
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_254ea_st1_table", 0x0254EA, 0x0255D8, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 1), (0xFF8096, 2, 0)
    ], "ValidUncompiledPc"))

    # 3c. 102(A6) == 2 -> state 2 full flow to RTS at 0x0255D6
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_254ea_st2_rts", 0x0254EA, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 2), (0xFF8096, 2, 0)
    ], "CompiledNextPc"))

    # 3d. 102(A6) == 3 -> state 3 full flow to RTS at 0x0255D6
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_254ea_st3_rts", 0x0254EA, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 3), (0xFF8096, 2, 0)
    ], "CompiledNextPc"))

    # 3e. 102(A6) == 4 -> state 4 full flow to RTS at 0x0255D6
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_254ea_st4_rts", 0x0254EA, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 4), (0xFF8096, 2, 0)
    ], "CompiledNextPc"))

    # 4. 0x00EA04 (enemy substate check)
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_ea04_substate_rts", 0x00EA04, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF802A, 2, 10)
    ], "CompiledNextPc"))

    # 5. 0x016D5C (enemy cooldown init)
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_16d5c_cooldown_rts", 0x016D5C, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 0), (0xFF80A0, 2, 0x20)
    ], "CompiledNextPc"))

    # 6. 0x0170E0 (enemy target lock)
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_170e0_lock_rts", 0x0170E0, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 0), (0xFF80A0, 2, 0x10), (0xFF808C, 2, 0x5)
    ], "CompiledNextPc"))

    # 7. 0x0171C4 (enemy target track)
    d, a = regs(a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_171c4_track_rts", 0x0171C4, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8066, 1, 0), (0xFF80A0, 2, 0x10), (0xFF808C, 2, 0x5), (0xFF8008, 2, 0x1)
    ], "CompiledNextPc"))

    # 8. 0x00E0B8 (dummy RTS)
    d, a = regs()
    fixtures.append(make_ctrl("ctrl_e0b8_dummy_rts", 0x00E0B8, 0x00FF2000, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000)
    ], "CompiledNextPc"))

    # 9. 0x00F96A (movement state logic)
    d, a = regs(a0=0xFF2000)
    fixtures.append(make_ctrl("ctrl_f96a_movement", 0x00F96A, 0x00FA82, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF2002, 1, 3), (0xFF2003, 1, 0), (0xFF2004, 2, 0)
    ], "ValidUncompiledPc"))

    out_json = root / "data" / "thor-fixtures" / "m15_249_callee_fixtures.json"
    out_json.write_text(json.dumps({
        "milestone": "M15.249",
        "core_sha256": CORE_SHA256,
        "rom_sha256": ROM_SHA256,
        "total_fixtures": len(fixtures),
        "fixtures": fixtures,
    }, indent=2), encoding="utf-8")
    print(f"WROTE: {out_json} ({len(fixtures)} fixtures)")

    # Emit C++ fixture header
    hpp_lines = [
        "#pragma once",
        '#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline std::vector<DispatchRefFixture> get_m15_249_callee_fixtures() {",
        "  std::vector<DispatchRefFixture> v;",
    ]
    for f in fixtures:
        ed = ",".join(f"0x{int(x,16):08X}U" for x in f["entry_d"])
        ea = ",".join(f"0x{int(x,16):08X}U" for x in f["entry_a"])
        xd = ",".join(f"0x{int(x,16):08X}U" for x in f["expected_d"])
        xa = ",".join(f"0x{int(x,16):08X}U" for x in f["expected_a"])
        ram = ",".join(f"{{0x{int(r['addr'],16):06X}U,{r['val']}U}}" for r in f["ram_init"])
        jit = "true" if f.get("allow_refresh_jitter", False) else "false"
        oc = f"PortableContinuationOutcome::{f['expected_outcome']}"
        epc, xpc = int(f["entry_pc"], 16), int(f["expected_exit_pc"], 16)
        esr, xsr = int(f["entry_sr"], 16), int(f["expected_sr"], 16)
        hpp_lines.append(
            f'  v.push_back({{"{f["test_id"]}","{f["classification"]}",0x{epc:06X}U,0x{xpc:06X}U,0x{esr:04X}U,0x{xsr:04X}U,'
            f'{{{ed}}},{{{ea}}},{{{xd}}},{{{xa}}},{oc},{f["expected_cycles"]}ULL,{jit},{{{ram}}}}});'
        )
    hpp_lines.extend(["  return v;", "}", "} // namespace oasis::recompiler::ir"])
    out_hpp = root / "data" / "thor-fixtures" / "m15_249_callee_fixtures.hpp"
    out_hpp.write_text("\n".join(hpp_lines) + "\n", encoding="utf-8")
    print(f"WROTE: {out_hpp} ({len(hpp_lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
