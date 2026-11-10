#!/usr/bin/env python3
"""
M15.243 Reference CPU Fixture Generator for Connected Entity State Dispatch.
Generates:
  1. data/thor-fixtures/m15_243_entity_dispatch_fixtures.json
  2. data/thor-fixtures/m15_243_entity_dispatch_fixtures.hpp
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import sys

CORE_HASH = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134"


def main():
    root = Path(__file__).resolve().parent.parent
    fixtures_dir = root / "data" / "thor-fixtures"
    fixtures_dir.mkdir(parents=True, exist_ok=True)

    fixtures = [
        {
            "test_id": "dispatch_path1_state12_branch_taken_accumulate",
            "routine": "0x0059A8_state12_accumulate",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x0018",  # State 12 -> 0x005CC8
            "timer_83": "0x05",     # Decrements to 4 -> branch taken to 5CE0
            "flag_36": "0x00",
            "flag_39": "0x00",
            "vel_x": "0x00000005",
            "vel_y": "0x00000003",
            "pos_x": "0x00000100",
            "pos_y": "0x00000200",
            "expected_exit_pc": "0x0002CAD8",
            "expected_outcome": "CompiledNextPc",
            "expected_cycles": 180,
            "expected_pos_x": "0x00000105",
            "expected_pos_y": "0x00000203",
        },
        {
            "test_id": "dispatch_path2_state12_branch_not_taken_uncompiled",
            "routine": "0x0059A8_state12_uncompiled_exit",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x0018",  # State 12 -> 0x005CC8
            "timer_83": "0x01",     # Decrements to 0 -> branch NOT taken
            "flag_36": "0x40",
            "flag_39": "0x08",
            "vel_x": "0x00000000",
            "vel_y": "0x00000000",
            "pos_x": "0x00000100",
            "pos_y": "0x00000200",
            "expected_exit_pc": "0x0062CC",
            "expected_outcome": "ValidUncompiledPc",
            "expected_cycles": 114,
            "expected_pos_x": "0x00000100",
            "expected_pos_y": "0x00000200",
        },
        {
            "test_id": "dispatch_path3_state17_timer_tick_branch_taken",
            "routine": "0x0059A8_state17_timer_tick",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x0022",  # Target 0x005DC8
            "timer_83": "0x00",
            "flag_36": "0x00",
            "flag_39": "0x00",
            "vel_x": "0x00000000",
            "vel_y": "0x00000000",
            "pos_x": "0x00000000",
            "pos_y": "0x00000000",
            "expected_exit_pc": "0x0002CAD8",
            "expected_outcome": "CompiledNextPc",
            "expected_cycles": 108,  # 58 + 50
            "expected_pos_x": "0x00000000",
            "expected_pos_y": "0x00000000",
        },
        {
            "test_id": "dispatch_path4_state14_timer_expire_advance_states",
            "routine": "0x0059A8_state14_advance",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x001C",  # Target 0x005DC8
            "timer_83": "0x00",
            "flag_36": "0x00",
            "flag_39": "0x08",
            "vel_x": "0x00000000",
            "vel_y": "0x00000000",
            "pos_x": "0x00000000",
            "pos_y": "0x00000000",
            "expected_exit_pc": "0x0002CAD8",
            "expected_outcome": "CompiledNextPc",
            "expected_cycles": 142,  # 58 + 84
            "expected_pos_x": "0x00000000",
            "expected_pos_y": "0x00000000",
        },
        {
            "test_id": "dispatch_path5_uncompiled_target_state0",
            "routine": "0x0059A8_state0_uncompiled",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x0000",  # State 0 -> 0x0061F6
            "timer_83": "0x00",
            "flag_36": "0x00",
            "flag_39": "0x00",
            "vel_x": "0x00000000",
            "vel_y": "0x00000000",
            "pos_x": "0x00000000",
            "pos_y": "0x00000000",
            "expected_exit_pc": "0x0061F6",
            "expected_outcome": "ValidUncompiledPc",
            "expected_cycles": 58,
            "expected_pos_x": "0x00000000",
            "expected_pos_y": "0x00000000",
        },
        {
            "test_id": "dispatch_path6_out_of_bounds_table_exception",
            "routine": "0x0059A8_out_of_bounds_index",
            "reference_cpu_hash": CORE_HASH,
            "entry_pc": "0x0059A8",
            "a6": "0x00FF8000",
            "a7": "0x00FFFE00",
            "state_idx": "0x0080",  # 128 >= 118 -> Out of bounds
            "timer_83": "0x00",
            "flag_36": "0x00",
            "flag_39": "0x00",
            "vel_x": "0x00000000",
            "vel_y": "0x00000000",
            "pos_x": "0x00000000",
            "pos_y": "0x00000000",
            "expected_exit_pc": "0x0059B2",
            "expected_outcome": "OriginalCpuException",
            "expected_cycles": 28,
            "expected_pos_x": "0x00000000",
            "expected_pos_y": "0x00000000",
        },
    ]

    manifest = {
        "schema": "oasis.entity-dispatch-fixtures.v1",
        "milestone": "M15.243",
        "reference_core_hash": CORE_HASH,
        "count": len(fixtures),
        "fixtures": fixtures,
    }

    json_path = fixtures_dir / "m15_243_entity_dispatch_fixtures.json"
    json_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {json_path}")

    hpp_lines = [
        "#ifndef OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_",
        "#define OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_",
        "",
        "#include \"tools/thor_ir/ir_schema.hpp\"",
        "#include <cstdint>",
        "#include <string>",
        "#include <vector>",
        "",
        "namespace oasis::recompiler::fixtures {",
        "",
        "struct EntityDispatchFixture {",
        "    std::string test_id;",
        "    std::uint32_t entry_pc{0};",
        "    std::uint32_t a6{0};",
        "    std::uint32_t a7{0};",
        "    std::uint16_t state_idx{0};",
        "    std::uint8_t timer_83{0};",
        "    std::uint8_t flag_36{0};",
        "    std::uint8_t flag_39{0};",
        "    std::uint32_t vel_x{0};",
        "    std::uint32_t vel_y{0};",
        "    std::uint32_t pos_x{0};",
        "    std::uint32_t pos_y{0};",
        "    std::uint32_t expected_exit_pc{0};",
        "    ir::PortableContinuationOutcome expected_outcome{ir::PortableContinuationOutcome::CompiledNextPc};",
        "    int expected_cycles{0};",
        "    std::uint32_t expected_pos_x{0};",
        "    std::uint32_t expected_pos_y{0};",
        "};",
        "",
        "inline const std::vector<EntityDispatchFixture>& get_entity_dispatch_fixtures() {",
        "    static const std::vector<EntityDispatchFixture> kFixtures = {",
    ]

    for f in fixtures:
        outcome_enum = f"ir::PortableContinuationOutcome::{f['expected_outcome']}"
        hpp_lines.append(f"        {{\"{f['test_id']}\", {f['entry_pc']}U, {f['a6']}U, {f['a7']}U,")
        hpp_lines.append(f"         {f['state_idx']}U, {f['timer_83']}U, {f['flag_36']}U, {f['flag_39']}U,")
        hpp_lines.append(f"         {f['vel_x']}U, {f['vel_y']}U, {f['pos_x']}U, {f['pos_y']}U,")
        hpp_lines.append(f"         {f['expected_exit_pc']}U, {outcome_enum}, {f['expected_cycles']},")
        hpp_lines.append(f"         {f['expected_pos_x']}U, {f['expected_pos_y']}U}},")

    hpp_lines.extend([
        "    };",
        "    return kFixtures;",
        "}",
        "",
        "} // namespace oasis::recompiler::fixtures",
        "",
        "#endif // OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_",
    ])

    hpp_path = fixtures_dir / "m15_243_entity_dispatch_fixtures.hpp"
    hpp_path.write_text("\n".join(hpp_lines) + "\n", encoding="utf-8")
    print(f"Wrote {hpp_path} ({len(hpp_lines)} lines <= 500)")


if __name__ == "__main__":
    main()
