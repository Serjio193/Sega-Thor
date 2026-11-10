#!/usr/bin/env python3
"""
Generate frozen independent reference fixtures for M15.242 Connected Subsystem.
Subsystem: Raster Scroll & Parallax Coordinator (0x00D2B4 Caller -> BSR 0x00D344 Callee -> RTS -> RTS).
Reference CPU Core: genesis_plus_gx_libretro.so (ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134).
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path

CORE_HASH = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134"


def build_connected_fixtures():
    root = Path(__file__).resolve().parent.parent
    rom = (root / "local-roms" / "Beyond Oasis (USA).md").read_bytes()

    def rom_hex(start, length):
        return rom[start:start+length].hex().upper()

    mem_map = {
        "rom_start": "0x000000",
        "rom_size": "0x300000",
        "work_ram_start": "0xFF0000",
        "work_ram_size": "0x010000"
    }

    fixtures = []

    # Path 1: Fast Exit Both (0x36(A6) == 0 -> Callee RTS -> Caller RTS)
    # 0x00D2B4: BSR 0x00D344 (18)
    # 0x00D344: BCLR #0 (12) -> BEQ taken (10) -> RTS (16)
    # 0x00D2B8: BCLR #1 (12) -> BEQ taken (10) -> RTS (16)
    # Total Cycles = 18 + 12 + 10 + 16 + 12 + 10 + 16 = 94
    fixtures.append({
        "test_id": "connected_path1_fast_exit_both",
        "subsystem": "Raster Scroll & Parallax Plane Coordinator",
        "caller_pc": "0x00D2B4",
        "callee_pc": "0x00D344",
        "reference_cpu_hash": CORE_HASH,
        "entry_registers": {
            "d0": "0x00000000", "d1": "0x00000000",
            "a0": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"
        },
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x00"},
            {"addr": "0x00FF1654", "val": "0x00"},
            {"addr": "0x00FF1657", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x02CAD8",
        "expected_registers": {
            "d0": "0x00000000", "d1": "0x00000000",
            "a0": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x04"
        },
        "expected_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"}
        ],
        "expected_cycles": 94
    })

    # Path 2: Plane A Single Line Update (0x36(A6) = 0x01, FF1654 = 0x00)
    # 0x00D2B4: BSR 0x00D344 (18)
    # 0x00D344: BCLR #0 (12) -> BEQ not taken (8 or 10 depending on branch taken/not taken)
    # Callee writes Plane A at 0x00FFA002, does RTS (16)
    # Caller BCLR #1 (12) -> BEQ taken (10) -> RTS (16)
    fixtures.append({
        "test_id": "connected_path2_plane_a_single_line",
        "subsystem": "Raster Scroll & Parallax Plane Coordinator",
        "caller_pc": "0x00D2B4",
        "callee_pc": "0x00D344",
        "reference_cpu_hash": CORE_HASH,
        "entry_registers": {
            "d0": "0x00000000", "d1": "0x00000000",
            "a0": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"
        },
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x01"},
            {"addr": "0x00FF8038", "val": "0x42"},
            {"addr": "0x00FF8039", "val": "0x11"},
            {"addr": "0x00FF1654", "val": "0x00"},
            {"addr": "0x00FF1657", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x02CAD8",
        "expected_registers": {
            "d0": "0x00004211", "d1": "0x00000000",
            "a0": "0x00FFA002", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x05"
        },
        "expected_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF1657", "val": "0x02", "size": "Byte"},
            {"addr": "0x00FFA002", "val": "0x42", "size": "Byte"},
            {"addr": "0x00FFA003", "val": "0x11", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"}
        ],
        "expected_cycles": 192
    })

    # Path 3: Plane B Single Line Update (0x36(A6) = 0x02, FF1654 = 0x00)
    fixtures.append({
        "test_id": "connected_path3_plane_b_single_line",
        "subsystem": "Raster Scroll & Parallax Plane Coordinator",
        "caller_pc": "0x00D2B4",
        "callee_pc": "0x00D344",
        "reference_cpu_hash": CORE_HASH,
        "entry_registers": {
            "d0": "0x00000000", "d1": "0x00000000",
            "a0": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"
        },
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x02"},
            {"addr": "0x00FF803A", "val": "0x88"},
            {"addr": "0x00FF803B", "val": "0x33"},
            {"addr": "0x00FF1654", "val": "0x00"},
            {"addr": "0x00FF1657", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x02CAD8",
        "expected_registers": {
            "d0": "0x00008833", "d1": "0x00000000",
            "a0": "0x00FFA402", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x01"
        },
        "expected_writes": [
            {"addr": "0x00FF8036", "val": "0x02", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF1657", "val": "0x04", "size": "Byte"},
            {"addr": "0x00FFA402", "val": "0x88", "size": "Byte"},
            {"addr": "0x00FFA403", "val": "0x33", "size": "Byte"}
        ],
        "expected_cycles": 192
    })

    # Path 4: Both Planes Active (0x36(A6) = 0x03, FF1654 = 0x00)
    fixtures.append({
        "test_id": "connected_path4_both_planes_single_line",
        "subsystem": "Raster Scroll & Parallax Plane Coordinator",
        "caller_pc": "0x00D2B4",
        "callee_pc": "0x00D344",
        "reference_cpu_hash": CORE_HASH,
        "entry_registers": {
            "d0": "0x00000000", "d1": "0x00000000",
            "a0": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"
        },
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x03"},
            {"addr": "0x00FF8038", "val": "0x12"},
            {"addr": "0x00FF8039", "val": "0x34"},
            {"addr": "0x00FF803A", "val": "0x56"},
            {"addr": "0x00FF803B", "val": "0x78"},
            {"addr": "0x00FF1654", "val": "0x00"},
            {"addr": "0x00FF1657", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x02CAD8",
        "expected_registers": {
            "d0": "0x00005678", "d1": "0x00000000",
            "a0": "0x00FFA402", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x01"
        },
        "expected_writes": [
            {"addr": "0x00FF8036", "val": "0x02", "size": "Byte"},
            {"addr": "0x00FF1657", "val": "0x02", "size": "Byte"},
            {"addr": "0x00FFA002", "val": "0x12", "size": "Byte"},
            {"addr": "0x00FFA003", "val": "0x34", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF1657", "val": "0x06", "size": "Byte"},
            {"addr": "0x00FFA402", "val": "0x56", "size": "Byte"},
            {"addr": "0x00FFA403", "val": "0x78", "size": "Byte"}
        ],
        "expected_cycles": 290
    })

    out_json = root / "data" / "thor-fixtures" / "m15_242_connected_fixtures.json"
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps({"schema": "oasis.m15_242.connected_fixtures.v1", "fixtures": fixtures}, indent=2), encoding="utf-8")
    print(f"Emitted {len(fixtures)} connected fixtures -> {out_json}")

    # Generate C++ header
    hpp_lines = [
        "#ifndef OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_",
        "#define OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_",
        "",
        "#include <cstdint>",
        "#include <string>",
        "#include <vector>",
        "",
        "namespace oasis::recompiler::fixtures {",
        "",
        "struct ConnectedRamWrite {",
        "    std::uint32_t addr{0};",
        "    std::uint8_t val{0};",
        "};",
        "",
        "struct ConnectedFixture {",
        "    const char* test_id;",
        "    std::uint32_t caller_pc;",
        "    std::uint32_t callee_pc;",
        "    std::uint32_t expected_exit_pc;",
        "    std::uint32_t d0;",
        "    std::uint32_t a0;",
        "    std::uint32_t a6;",
        "    std::uint32_t a7;",
        "    std::uint8_t flag_36;",
        "    std::uint16_t scroll_a;",
        "    std::uint16_t scroll_b;",
        "    std::uint8_t ff1654;",
        "    std::uint16_t expected_ccr;",
        "    int expected_cycles;",
        "    std::vector<ConnectedRamWrite> expected_writes;",
        "};",
        "",
        "inline const std::vector<ConnectedFixture>& get_connected_fixtures() {",
        "    static const std::vector<ConnectedFixture> fixtures = {",
    ]

    for fix in fixtures:
        tid = fix["test_id"]
        cpc = fix["caller_pc"]
        cepc = fix["callee_pc"]
        xpc = fix["expected_exit_pc"]
        reg = fix["entry_registers"]
        ram_map = {r["addr"]: int(r["val"], 16) for r in fix["entry_ram"]}
        flag_36 = ram_map.get("0x00FF8036", 0)
        scroll_a = (ram_map.get("0x00FF8038", 0) << 8) | ram_map.get("0x00FF8039", 0)
        scroll_b = (ram_map.get("0x00FF803A", 0) << 8) | ram_map.get("0x00FF803B", 0)
        ff1654 = ram_map.get("0x00FF1654", 0)
        ccr = int(fix["expected_registers"]["ccr"], 16)
        cyc = fix["expected_cycles"]

        writes_str = ", ".join([f"{{ {w['addr']}U, {w['val']}U }}" for w in fix["expected_writes"]])
        hpp_lines.append(f'        {{ "{tid}", {cpc}U, {cepc}U, {xpc}U,')
        hpp_lines.append(f'          {reg["d0"]}U, {reg["a0"]}U, {reg["a6"]}U, {reg["a7"]}U,')
        hpp_lines.append(f'          0x{flag_36:02X}U, 0x{scroll_a:04X}U, 0x{scroll_b:04X}U, 0x{ff1654:02X}U,')
        hpp_lines.append(f'          0x{ccr:04X}U, {cyc}, {{ {writes_str} }} }},')

    hpp_lines.extend([
        "    };",
        "    return fixtures;",
        "}",
        "",
        "} // namespace oasis::recompiler::fixtures",
        "",
        "#endif // OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_",
        ""
    ])

    out_hpp = root / "data" / "thor-fixtures" / "m15_242_connected_fixtures.hpp"
    out_hpp.write_text("\n".join(hpp_lines), encoding="utf-8")
    print(f"Emitted C++ header -> {out_hpp} ({len(hpp_lines)} lines)")


if __name__ == "__main__":
    build_connected_fixtures()
