#!/usr/bin/env python3
"""
M15.256 Reference Fixture Generator (100% real pinned GPGX ca4be304...).
Expands reference fixture suite to qualify all 5 M15.256 clean leaf routines.
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


def regs(d0=0, d1=0, d2=0, d3=0, d4=0, d5=0, d6=0, d7=0,
         a0=0, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0, a7=0x00FFFE00):
    return [d0, d1, d2, d3, d4, d5, d6, d7], [a0, a1, a2, a3, a4, a5, a6, a7]


def main():
    root = Path(__file__).resolve().parent.parent
    fixtures = []

    # 1. 0x010F48: hotpath_10f48_entity_bounds_calc
    d, a = regs(d0=10, d1=20, a5=0x00FF2000, a6=0x00FF2000)
    fixtures.append(make_ctrl("ctrl_10f48_bounds_calc", 0x010F48, 0x00FF2000, 0x2000, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF2008, 2, 5), (0xFF200C, 2, 5)
    ], "CompiledNextPc"))

    # 2. 0x0112CC: hotpath_112cc_entity_slot_lookup (found slot)
    d, a = regs(d0=5, a6=0x00FF2000)
    fixtures.append(make_ctrl("ctrl_112cc_slot_lookup_found", 0x0112CC, 0x00FF2000, 0x2000, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF1F98, 2, 5)
    ], "CompiledNextPc"))

    # 3. 0x0302F4: hotpath_302f4_entity_flag_sync
    d, a = regs(a6=0x00FF2000)
    fixtures.append(make_ctrl("ctrl_302f4_flag_sync", 0x0302F4, 0x00FF2000, 0x2000, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF1CA8, 2, 1), (0xFF1CA8 + 57, 1, 0), (0xFF1CA8 + 110, 1, 0x20)
    ], "CompiledNextPc"))

    # 4. 0x006B12: hotpath_06b12_velocity_damp_math (greater)
    d, a = regs(d0=10, d1=5)
    fixtures.append(make_ctrl("ctrl_06b12_velocity_damp_gt", 0x006B12, 0x00FF2000, 0x2000, d, a, [
        (0xFFFE00, 4, 0x00FF2000)
    ], "CompiledNextPc"))

    # 5. 0x0141CA: hotpath_141ca_enemy_coord_clamp
    d, a = regs(d0=100, a6=0x00FF2000)
    fixtures.append(make_ctrl("ctrl_141ca_coord_clamp", 0x0141CA, 0x00FF2000, 0x2000, d, a, [
        (0xFFFE00, 4, 0x00FF2000), (0xFF2010, 2, 50), (0xFF2012, 2, 150)
    ], "CompiledNextPc"))

    p250 = root / "data" / "thor-fixtures" / "m15_250_callee_fixtures.json"
    prior_data = json.loads(p250.read_text(encoding="utf-8"))
    all_fixtures = prior_data["fixtures"] + fixtures

    out_json = root / "data" / "thor-fixtures" / "m15_256_callee_fixtures.json"
    out_json.write_text(json.dumps({
        "milestone": "M15.256",
        "core_sha256": CORE_SHA256,
        "rom_sha256": ROM_SHA256,
        "total_fixtures": len(all_fixtures),
        "fixtures": all_fixtures,
    }, indent=2), encoding="utf-8")
    print(f"WROTE: {out_json} ({len(all_fixtures)} total fixtures, {len(fixtures)} new)")

    hpp_lines = [
        "#pragma once",
        '#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline std::vector<DispatchRefFixture> get_m15_256_callee_fixtures() {",
        "  std::vector<DispatchRefFixture> v;",
    ]
    for f in all_fixtures:
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
    out_hpp = root / "data" / "thor-fixtures" / "m15_256_callee_fixtures.hpp"
    out_hpp.write_text("\n".join(hpp_lines) + "\n", encoding="utf-8")
    print(f"WROTE: {out_hpp} ({len(hpp_lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
