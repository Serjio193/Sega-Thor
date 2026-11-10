#!/usr/bin/env python3
"""
M15.246 Phase 7 Reference Fixture Generator (100% real pinned GPGX ca4be304...).
Combines 18 AUTHENTIC_GAMEPLAY_ENTRY traces from longrun_3600f (frames 0..3599 across all 7 roots)
with 12 CONTROLLED_CPU_ENTRY probes covering MOVEM, MULS, DIVU, DIVS (normal/overflow/div0),
shifts (ASL/LSL/LSR), EXT/EXG/NEG/SWAP/SF/ST/ADDA/CMPA/LEA disp_ax, JSR (A2), and cross-module exits.
Strictly <= 220 lines per AGENTS.md Rule 3.
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
    lr_oracle = json.loads((root / "build" / "m15_246" / "oracle_lr.json").read_text(encoding="utf-8"))
    ck_lr = file_sha(root / "build" / "m15_246" / "longrun_3600f" / "initial.state")
    inp_lr = file_sha(root / "build" / "m15_246" / "longrun_3600f" / "input.polls")

    fixtures = []
    for idx, cap in enumerate(lr_oracle["authentic_captures"]):
        ep = cap["entry_pc"].lower()
        xp = cap["exit_pc"].lower()
        fixtures.append({
            "test_id": f"auth_lr_{idx:02d}_f{cap['frame']}_{ep}_{xp}",
            "classification": "AUTHENTIC_GAMEPLAY_ENTRY",
            "scenario": "longrun_3600f",
            "frame": cap["frame"],
            "rom_sha256": ROM_SHA256,
            "core_sha256": CORE_SHA256,
            "checkpoint_sha256": ck_lr,
            "input_journal_sha256": inp_lr,
            "entry_pc": cap["entry_pc"],
            "entry_d": cap["entry_d"],
            "entry_a": cap["entry_a"],
            "entry_sr": cap["entry_sr"],
            "ram_init": [{"addr": addr, "val": val} for addr, val in cap["ram_pre"]],
            "expected_exit_pc": cap["exit_pc"],
            "expected_outcome": cap["continuation_type"],
            "expected_d": cap["exit_d"],
            "expected_a": cap["exit_a"],
            "expected_sr": cap["exit_sr"],
            "expected_cycles": cap["cycles"],
            "allow_refresh_jitter": True,
            "pc_trace": cap["pc_trace"],
            "writes": cap["writes"],
        })

    def regs(d0=0, d1=0, d2=0, d3=0, a0=0xFF1000, a1=0xFF1200, a2=0xFF1400, a6=0xFF8000, a7=0xFFFE00):
        return [d0, d1, d2, d3, 0, 0, 0, 0], [a0, a1, a2, 0xFF1000, 0xFF1000, 0xFF1000, a6, a7]

    # 12 Controlled CPU probes directly measured on pinned GPGX
    d, a = regs(d0=0x1000); fixtures.append(make_ctrl("ctrl_e6ba_isqrt_4096_rts", 0x00E6BA, 0xFF2000, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)]))
    d, a = regs(d0=0); fixtures.append(make_ctrl("ctrl_e6ba_isqrt_zero_early_rts", 0x00E6BA, 0xFF2000, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)]))
    d, a = regs(d0=0); fixtures.append(make_ctrl("ctrl_e6dc_divu_zero_trap_vec5", 0x00E6DC, 0x00E6DE, 0x2100, d, a, []))
    d, a = regs(d0=0x10, d1=0x20, d2=0x110, d3=0x120); fixtures.append(make_ctrl("ctrl_f0d2_dist2d_bsr_isqrt_rts", 0x00F0D2, 0xFF2000, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)]))
    d, a = regs(d0=0x7FFF0000); fixtures.append(make_ctrl("ctrl_2ee58_divs_overflow_ccr", 0x02EE58, 0x02EE5C, 0x2100, d, a, []))
    d, a = regs(d0=0xFFFFFB00, d1=0x00000500, d2=0x00000000); fixtures.append(make_ctrl("ctrl_2ee58_divs_extl_asll7_block", 0x02EE58, 0x02EE70, 0x2100, d, a, []))
    d, a = regs(); fixtures.append(make_ctrl("ctrl_2ed20_movem_to_prod_2edca", 0x02ED20, 0x02EDCA, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000), (0xFF8008, 4, 0x10000), (0xFF800C, 4, 0x20000), (0xFF8010, 4, 0x30000), (0xFF8070, 2, 0), (0xFF8072, 2, 0)], "ValidUncompiledPc"))
    d, a = regs(); fixtures.append(make_ctrl("ctrl_6dd2_call_prod_85e2", 0x006DD2, 0x0085E2, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000), (0xFF802C, 2, 1)], "ValidUncompiledPc"))
    d, a = regs(d0=0); fixtures.append(make_ctrl("ctrl_6de2_resume_to_m245_6e4a", 0x006DE2, 0x006E4A, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000), (0xFF1983, 1, 1)], "ValidUncompiledPc"))
    d, a = regs(); fixtures.append(make_ctrl("ctrl_6e0a_st_lea_bsr_m245_6e7a", 0x006E0A, 0x006E7A, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)], "ValidUncompiledPc"))
    d, a = regs(a2=0x00FF2020); fixtures.append(make_ctrl("ctrl_be5a_jsr_a2_indirect_exit", 0x00BE5A, 0x00FF2020, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000)], "ValidUncompiledPc"))
    d, a = regs(); fixtures.append(make_ctrl("ctrl_15ae2_cmpal_to_16294", 0x015AE2, 0x016294, 0x2100, d, a, [(0xFFFE00, 4, 0x00FF2000), (0xFF8000, 4, 0x10)], "ValidUncompiledPc"))

    out_json = root / "data" / "thor-fixtures" / "m15_246_callee_fixtures.json"
    out_json.write_text(json.dumps({
        "milestone": "M15.246",
        "core_sha256": CORE_SHA256,
        "rom_sha256": ROM_SHA256,
        "fixtures": fixtures,
    }, indent=2), encoding="utf-8")

    hpp_lines = [
        "#pragma once",
        '#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline std::vector<DispatchRefFixture> get_m15_246_callee_fixtures() {",
        "  std::vector<DispatchRefFixture> v;",
    ]
    for f in fixtures:
        ed = ",".join(f"0x{int(x,16):08X}U" for x in f["entry_d"])
        ea = ",".join(f"0x{int(x,16):08X}U" for x in f["entry_a"])
        xd = ",".join(f"0x{int(x,16):08X}U" for x in f["expected_d"])
        xa = ",".join(f"0x{int(x,16):08X}U" for x in f["expected_a"])
        ram = ",".join(f"{{0x{int(r['addr'],16):06X}U,{r['val']}U}}" for r in f["ram_init"])
        jit = "true" if f["allow_refresh_jitter"] else "false"
        oc = f"PortableContinuationOutcome::{f['expected_outcome']}"
        epc, xpc = int(f["entry_pc"], 16), int(f["expected_exit_pc"], 16)
        esr, xsr = int(f["entry_sr"], 16), int(f["expected_sr"], 16)
        hpp_lines.append(
            f'  v.push_back({{"{f["test_id"]}","{f["classification"]}",0x{epc:06X}U,0x{xpc:06X}U,0x{esr:04X}U,0x{xsr:04X}U,'
            f'{{{ed}}},{{{ea}}},{{{xd}}},{{{xa}}},{oc},{f["expected_cycles"]}ULL,{jit},{{{ram}}}}});'
        )
    hpp_lines.extend(["  return v;", "}", "} // namespace oasis::recompiler::ir"])
    out_hpp = root / "data" / "thor-fixtures" / "m15_246_callee_fixtures.hpp"
    out_hpp.write_text("\n".join(hpp_lines) + "\n", encoding="utf-8")

    auth_n = sum(1 for f in fixtures if f["classification"] == "AUTHENTIC_GAMEPLAY_ENTRY")
    ctrl_n = sum(1 for f in fixtures if f["classification"] == "CONTROLLED_CPU_ENTRY")
    print(f"Generated {len(fixtures)} M15.246 GPGX fixtures (AUTHENTIC={auth_n}, CONTROLLED={ctrl_n}), header lines={len(hpp_lines)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
