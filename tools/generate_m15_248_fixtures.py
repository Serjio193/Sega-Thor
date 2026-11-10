#!/usr/bin/env python3
"""
M15.248 Reference Fixture Generator (100% real pinned GPGX ca4be304...).
Expands reference fixture suite to achieve >= 120/151 internal edge coverage
across the M15.246 compiled roots, and qualifies the 15 M15.248 hotpath routines.
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


def file_sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


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

    # Load 70 baseline fixtures from M15.247
    base_file = root / "data" / "thor-fixtures" / "m15_247_callee_fixtures.json"
    fixtures = json.loads(base_file.read_text(encoding="utf-8"))["fixtures"]

    def regs(d0=0, d1=0, d2=0, d3=0, d4=0, d5=0, d6=0, d7=0,
             a0=0xFF1000, a1=0xFF1200, a2=0xFF1400, a3=0xFF1600, a4=0xFF1800, a5=0xFF1A00, a6=0xFF8000, a7=0xFFFE00):
        return [d0, d1, d2, d3, d4, d5, d6, d7], [a0, a1, a2, a3, a4, a5, a6, a7]

    print("Generating M15.248 CFG edge expansion probes...")
    # 1. 0x02EDA0 -> 0x02EDCA uncompiled callee exit (covers 0x02EDA0..0x02EDAA)
    d, a = regs()
    fixtures.append(make_ctrl("ctrl_2eda0_bsr_cascade", 0x02EDA0, 0x02EDCA, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8008, 4, 0x10000), (0xFF800C, 4, 0x20000), (0xFF8010, 4, 0x30000)
    ], "ValidUncompiledPc"))

    # 2. 0x02EE70 -> 0x02EEA0 uncompiled callee exit
    d, a = regs()
    fixtures.append(make_ctrl("ctrl_2ee70_bsr_cascade", 0x02EE70, 0x02EEA0, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8008, 4, 0x10000), (0xFF800C, 4, 0x20000), (0xFF8010, 4, 0x30000)
    ], "ValidUncompiledPc"))

    # 3. 0x02ED3E angle ladder probes for angles: 0x40, 0x3D, 0x3A, 0x37, 0x34, 0x33, 0x6C, 0x6F, 0x72, 0x75, 0x78
    angles = [0x40, 0x3D, 0x3A, 0x37, 0x34, 0x33, 0x6C, 0x6F, 0x72, 0x75, 0x78]
    for ang in angles:
        d, a = regs()
        fixtures.append(make_ctrl(f"ctrl_2ed30_ang_{ang:02X}", 0x02ED3E, 0x02EDA0, 0x2100, d, a, [
            (0xFFFE00, 4, 0x00FF2000), (0xFF1DC6, 2, ang)
        ]))

    # 4. 0x00BD40 combo ladder probes (A6 = 0xFF19E8 player and A6 != 0xFF19E8 non-player)
    d, a = regs(a6=0xFF8000)  # non-player -> 0x00BD46 taken to 0x00BD86
    fixtures.append(make_ctrl("ctrl_bd40_not_player", 0x00BD40, 0x00BD86, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)]))

    combos = [5, 15, 25, 35, 45, 75, 150, 250]
    for cmb in combos:
        d, a = regs(a6=0xFF19E8)
        fixtures.append(make_ctrl(f"ctrl_bd40_combo_{cmb}", 0x00BD40, 0x00BD86, 0x2100, d, a, [
            (0xFFFE00, 4, 0x00FF2000), (0xFF19E8 + 150, 2, cmb)
        ]))

    # 4b. 0x02EDF2 entity type branch probes
    # (0(A6) == 0x30, 4(A6) == 0x12 -> taken to 0x02EE9E)
    d, a = regs(a0=0xFF2000, a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_2edf0_type_30_12", 0x02EDF2, 0x02EE9E, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8000, 2, 0x30), (0xFF8004, 2, 0x12)
    ]))
    # (0(A6) == 0x30, 4(A6) != 0x12, 138(A6) != 0 -> fallthrough 0x02EE06, taken 0x02EE52)
    d, a = regs(a0=0xFF2000, a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_2edf0_type_30_bne_138", 0x02EDF2, 0x02EE58, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8000, 2, 0x30), (0xFF8004, 2, 0x00), (0xFF8000 + 138, 2, 1),
        (0xFF8008, 4, 0x10000), (0xFF800C, 4, 0x20000), (0xFF8010, 4, 0x30000)
    ]))

    # 5. 0x00BDE0 hitbox boundary tests
    # 5a. self_hit: A0 == A6 -> 0x00BDEA taken to 0x00BE5E
    d, a = regs(a0=0xFF8000, a6=0xFF8000)
    fixtures.append(make_ctrl("ctrl_bde0_self_hit", 0x00BDE0, 0x00BE5E, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF8000, 2, 1), (0xFF8000 + 56, 1, 0)
    ]))

    # 5b. X bound: D0 > D6 vs D0 <= D6
    d, a = regs(d0=100, d7=0, a0=0xFF2000, a6=0xFF8000)  # D0 > D6 (D5=0, D6=50)
    fixtures.append(make_ctrl("ctrl_bde0_x_out", 0x00BDE0, 0x00BE5E, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF2000, 2, 1), (0xFF2000 + 56, 1, 0),
        (0xFF2000 + 16, 2, 0), (0xFF2000 + 20, 2, 0), (0xFF2000 + 74, 2, 50)
    ]))

    d, a = regs(d0=20, d7=0, a0=0xFF2000, a6=0xFF8000)  # D0 <= D6 -> exit at 0x00BE5C
    fixtures.append(make_ctrl("ctrl_bde0_x_in", 0x00BDE0, 0x00BE5C, 0x2100, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF2000, 2, 1), (0xFF2000 + 56, 1, 0),
        (0xFF2000 + 16, 2, 0), (0xFF2000 + 20, 2, 0), (0xFF2000 + 74, 2, 50),
        (0xFF2000 + 66, 2, 10), (0xFF2000 + 8, 2, 100), (0xFF2000 + 68, 2, 10), (0xFF2000 + 12, 2, 100),
        (0xFF2000 + 130, 1, 1)
    ]))

    # 6. Probes for the 15 M15.248 Hotpath routines
    m248_roots = [
        (0x00FCB8, "fcb8_load_transform", 0x0037D2, [(0xFF1002, 4, 0x00FF1200), (0xFF1006, 2, 0x0010)], "ValidUncompiledPc"),
        (0x00D9A4, "d9a4_math_calc", 0x0037D2, [], "ValidUncompiledPc"),
        (0x002D84, "2d84_vdp_coord_prep", 0x002EE2, [], "ValidUncompiledPc"),
        (0x002DB6, "2db6_vdp_reg_config", 0xFF2000, [], "CompiledNextPc"),
        (0x002E1E, "2e1e_vdp_status_prep", 0x002F6E, [(0xFF1000, 8, 0), (0xFF8000, 4, 0)], "ValidUncompiledPc"),
        (0x002E78, "2e78_vdp_dma_queue_calc", 0xFF2000, [], "CompiledNextPc"),
        (0x003586, "3586_vdp_sprite_coord", 0x00E268, [], "ValidUncompiledPc"),
        (0x00F80E, "f80e_collision_bound_calc", 0x00F84E, [(0xFF17B2, 4, 0x00FF1000), (0xFF1000, 4, 0x00000000)], "ValidUncompiledPc"),
        (0x01A184, "1a184_rng_step", 0x01A54E, [], "ValidUncompiledPc"),
        (0x00FCCE, "fcce_vector_adjust", 0x0101EC, [(0xFF1000, 8, 0), (0xFF8000, 4, 0)], "ValidUncompiledPc"),
        (0x001112, "1112_math_lookup", 0xFF2000, [], "CompiledNextPc"),
        (0x00114A, "114a_math_interpolate", 0xFF2000, [], "CompiledNextPc"),
        (0x001E98, "1e98_tile_offset_calc", 0xFF2000, [], "CompiledNextPc"),
        (0x009BC0, "9bc0_input_buffer_poll", 0xFF2000, [], "CompiledNextPc"),
        (0x001E46, "1e46_tile_coord_decode", 0xFF2000, [(0xFF183C, 4, 0x00FF1000), (0xFF1000, 8, 0x0001000000000000)], "CompiledNextPc"),
    ]

    print("Generating M15.248 hotpath baseline probes...")
    for ep, name, xp, extra_pokes, oc in m248_roots:
        d, a = regs(d0=0x10, d1=0x20, d2=0x30, d3=0x40, a0=0xFF1000, a1=0xFF1200, a6=0xFF8000)
        pokes = [(0xFFFE00, 4, 0x00FF2000)] + extra_pokes
        fixtures.append(make_ctrl(f"ctrl_m248_{name}", ep, xp, 0x2100, d, a, pokes, oc))

    out_json = root / "data" / "thor-fixtures" / "m15_248_callee_fixtures.json"
    out_json.write_text(json.dumps({
        "milestone": "M15.248",
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
        "inline std::vector<DispatchRefFixture> get_m15_248_callee_fixtures() {",
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
    out_hpp = root / "data" / "thor-fixtures" / "m15_248_callee_fixtures.hpp"
    out_hpp.write_text("\n".join(hpp_lines) + "\n", encoding="utf-8")
    print(f"WROTE: {out_hpp} ({len(hpp_lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
