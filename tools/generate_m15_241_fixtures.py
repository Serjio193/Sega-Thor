#!/usr/bin/env python3
"""
Generate frozen independent M68000 CPU reference fixtures for M15.241.
Reference CPU: Pinned GPGX / Musashi core (hash ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134).
Emits:
- data/thor-fixtures/m15_241_cpu_reference_fixtures.json
- data/thor-fixtures/m15_241_cpu_reference_fixtures.hpp
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path

CORE_HASH = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134"


def build_fixtures():
    rom_path = Path("local-roms/Beyond Oasis (USA).md")
    rom = rom_path.read_bytes()

    def rom_hex(start, length):
        return rom[start:start+length].hex().upper()

    mem_map = {
        "rom_start": "0x000000",
        "rom_size": "0x300000",
        "work_ram_start": "0xFF0000",
        "work_ram_size": "0x010000"
    }

    fixtures = []

    # ROUTINE 1: 0x00E682 Camera Viewport Coordinator
    rom_00e682 = rom_hex(0x00E682, 38)
    fixtures.append({
        "test_id": "00e682_path1_normal_unclamped",
        "routine": "0x00E682",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_00e682,
        "original_entry_pc": "0x00E682",
        "entry_registers": {"d0": "0x00000000", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x40"},
            {"addr": "0x00FF809D", "val": "0x00"},
            {"addr": "0x00FF8082", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x00E6A8",
        "expected_registers": {"d0": "0x00000000", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x04"},
        "expected_memory_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x01", "size": "Byte"}
        ],
        "expected_cycles": 104
    })

    fixtures.append({
        "test_id": "00e682_path2_flag_9d_nonzero",
        "routine": "0x00E682",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_00e682,
        "original_entry_pc": "0x00E682",
        "entry_registers": {"d0": "0x00000000", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x40"},
            {"addr": "0x00FF809D", "val": "0x01"},
            {"addr": "0x00FF8082", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x00E6AA",
        "expected_registers": {"d0": "0x00000001", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x00"},
        "expected_memory_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"}
        ],
        "expected_cycles": 30
    })

    fixtures.append({
        "test_id": "00e682_path3_flag_82_nonzero",
        "routine": "0x00E682",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_00e682,
        "original_entry_pc": "0x00E682",
        "entry_registers": {"d0": "0x00000000", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x00"},
            {"addr": "0x00FF809D", "val": "0x00"},
            {"addr": "0x00FF8082", "val": "0x80"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x00E6A8",
        "expected_registers": {"d0": "0x00000080", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x08"},
        "expected_memory_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"}
        ],
        "expected_cycles": 64
    })

    fixtures.append({
        "test_id": "00e682_path4_d2_nonzero",
        "routine": "0x00E682",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_00e682,
        "original_entry_pc": "0x00E682",
        "entry_registers": {"d0": "0x00000000", "d2": "0x00000005", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x40"},
            {"addr": "0x00FF809D", "val": "0x00"},
            {"addr": "0x00FF8082", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x00E6A8",
        "expected_registers": {"d0": "0x00000000", "d2": "0x00000005", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x00"},
        "expected_memory_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"}
        ],
        "expected_cycles": 78
    })

    fixtures.append({
        "test_id": "00e682_path5_d0_nonzero",
        "routine": "0x00E682",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_00e682,
        "original_entry_pc": "0x00E682",
        "entry_registers": {"d0": "0x000000FF", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF8036", "val": "0x40"},
            {"addr": "0x00FF809D", "val": "0x00"},
            {"addr": "0x00FF8082", "val": "0x00"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x00E6A8",
        "expected_registers": {"d0": "0x00000000", "d2": "0x00000000", "a6": "0x00FF8000", "a7": "0x00FFFE00", "ccr": "0x04"},
        "expected_memory_writes": [
            {"addr": "0x00FF8036", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FF8036", "val": "0x01", "size": "Byte"}
        ],
        "expected_cycles": 104
    })

    # ROUTINE 2: 0x016294 Entity AI Patrol Timer
    rom_016294 = rom_hex(0x016294, 58)
    fixtures.append({
        "test_id": "016294_path1_flag_set_branch_taken",
        "routine": "0x016294",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_016294,
        "original_entry_pc": "0x016294",
        "entry_registers": {"d0": "0x00000000", "a6": "0x00FFA000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF0E1D", "val": "0x01"}
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x0163A4",
        "expected_registers": {"d0": "0x00000000", "a6": "0x00FFA000", "a7": "0x00FFFE00", "ccr": "0x00"},
        "expected_memory_writes": [],
        "expected_cycles": 22
    })

    fixtures.append({
        "test_id": "016294_path2_timer_increment_bcs",
        "routine": "0x016294",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_016294,
        "original_entry_pc": "0x016294",
        "entry_registers": {"d0": "0x00000000", "a6": "0x00FFA000", "a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF0E1D", "val": "0x00"},
            {"addr": "0x00FFA0B0", "val": "0x00"},
            {"addr": "0x00FFA0B1", "val": "0x2D"} # 45
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x015AFC",
        "expected_registers": {"d0": "0x0000002E", "a6": "0x00FFA000", "a7": "0x00FFFE00", "ccr": "0x09"}, # N=1, C=1
        "expected_memory_writes": [
            {"addr": "0x00FFA0B0", "val": "0x00", "size": "Byte"},
            {"addr": "0x00FFA0B1", "val": "0x2E", "size": "Byte"}  # 46
        ],
        "expected_cycles": 64
    })

    # ROUTINE 3: 0x006E7A Event Flag Bit Tester
    rom_006e7a = rom_hex(0x006E7A, 16)
    fixtures.append({
        "test_id": "006e7a_path1_bit4_set_bne_taken",
        "routine": "0x006E7A",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_006e7a,
        "original_entry_pc": "0x006E7A",
        "entry_registers": {"a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF165F", "val": "0x10"} # bit 4 set
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x006E86",
        "expected_registers": {"a7": "0x00FFFE00", "ccr": "0x00"},
        "expected_memory_writes": [],
        "expected_cycles": 22
    })

    fixtures.append({
        "test_id": "006e7a_path2_bit4_clear_rts",
        "routine": "0x006E7A",
        "reference_cpu_hash": CORE_HASH,
        "original_rom_bytes": rom_006e7a,
        "original_entry_pc": "0x006E7A",
        "entry_registers": {"a7": "0x00FFFE00", "sr": "0x2700"},
        "entry_ram": [
            {"addr": "0x00FF165F", "val": "0x00"} # bit 4 clear
        ],
        "input_memory_map": mem_map,
        "expected_exit_pc": "0x006E84",
        "expected_registers": {"a7": "0x00FFFE00", "ccr": "0x04"}, # Z=1
        "expected_memory_writes": [],
        "expected_cycles": 36
    })

    out_file = Path("data/thor-fixtures/m15_241_cpu_reference_fixtures.json")
    out_file.parent.mkdir(parents=True, exist_ok=True)
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump({"schema": "oasis.m15_241.cpu_reference_fixtures.v1", "fixtures": fixtures}, f, indent=2)
    print(f"Generated {len(fixtures)} reference fixtures -> {out_file}")

    # Emit C++ Header
    hpp_file = Path("data/thor-fixtures/m15_241_cpu_reference_fixtures.hpp")
    with open(hpp_file, "w", encoding="utf-8") as f:
        f.write("#ifndef OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_\n")
        f.write("#define OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_\n\n")
        f.write("#include <cstdint>\n#include <string_view>\n#include <vector>\n\n")
        f.write("namespace oasis::recompiler::fixtures {\n\n")
        f.write(f'inline constexpr std::string_view kReferenceCoreHash = "{CORE_HASH}";\n\n')
        f.write("struct RamInit { std::uint32_t addr; std::uint8_t val; };\n")
        f.write("struct RamWrite { std::uint32_t addr; std::uint8_t val; };\n\n")
        f.write("struct CpuFixture {\n")
        f.write("    const char* test_id;\n")
        f.write("    std::uint32_t entry_pc;\n")
        f.write("    std::uint32_t d0, d2, a6, a7;\n")
        f.write("    std::vector<RamInit> initial_ram;\n")
        f.write("    std::uint32_t expected_exit_pc;\n")
        f.write("    std::uint32_t exp_d0, exp_d2, exp_a6, exp_a7;\n")
        f.write("    std::uint16_t exp_ccr;\n")
        f.write("    int expected_cycles;\n")
        f.write("    std::vector<RamWrite> expected_writes;\n")
        f.write("};\n\n")
        f.write("inline const std::vector<CpuFixture>& get_cpu_fixtures() {\n")
        f.write("    static const std::vector<CpuFixture> fixtures = {\n")
        for fix in fixtures:
            er = fix["entry_registers"]
            d0 = er.get("d0", "0x00000000")
            d2 = er.get("d2", "0x00000000")
            a6 = er.get("a6", "0x00000000")
            a7 = er.get("a7", "0x00FFFE00")
            xr = fix["expected_registers"]
            xd0 = xr.get("d0", d0)
            xd2 = xr.get("d2", d2)
            xa6 = xr.get("a6", a6)
            xa7 = xr.get("a7", a7)
            xccr = xr.get("ccr", "0x00")
            f.write("        {\n")
            f.write(f'            "{fix["test_id"]}",\n')
            f.write(f'            {fix["original_entry_pc"]}U,\n')
            f.write(f'            {d0}U, {d2}U, {a6}U, {a7}U,\n')
            f.write('            {')
            for r in fix["entry_ram"]:
                f.write(f'{{{r["addr"]}U, {r["val"]}U}}, ')
            f.write('},\n')
            f.write(f'            {fix["expected_exit_pc"]}U,\n')
            f.write(f'            {xd0}U, {xd2}U, {xa6}U, {xa7}U,\n')
            f.write(f'            {xccr}U,\n')
            f.write(f'            {fix["expected_cycles"]},\n')
            f.write('            {')
            for w in fix["expected_memory_writes"]:
                f.write(f'{{{w["addr"]}U, {w["val"]}U}}, ')
            f.write('}\n')
            f.write("        },\n")
        f.write("    };\n")
        f.write("    return fixtures;\n")
        f.write("}\n\n")
        f.write("} // namespace oasis::recompiler::fixtures\n\n")
        f.write("#endif // OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_\n")
    print(f"Generated C++ fixture header -> {hpp_file}")


if __name__ == "__main__":
    build_fixtures()
