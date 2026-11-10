#ifndef OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_
#define OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_

#include "tools/thor_ir/ir_schema.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace oasis::recompiler::fixtures {

struct EntityDispatchFixture {
    std::string test_id;
    std::uint32_t entry_pc{0};
    std::uint32_t a6{0};
    std::uint32_t a7{0};
    std::uint16_t state_idx{0};
    std::uint8_t timer_83{0};
    std::uint8_t flag_36{0};
    std::uint8_t flag_39{0};
    std::uint32_t vel_x{0};
    std::uint32_t vel_y{0};
    std::uint32_t pos_x{0};
    std::uint32_t pos_y{0};
    std::uint32_t expected_exit_pc{0};
    ir::PortableContinuationOutcome expected_outcome{ir::PortableContinuationOutcome::CompiledNextPc};
    int expected_cycles{0};
    std::uint32_t expected_pos_x{0};
    std::uint32_t expected_pos_y{0};
};

inline const std::vector<EntityDispatchFixture>& get_entity_dispatch_fixtures() {
    static const std::vector<EntityDispatchFixture> kFixtures = {
        {"dispatch_path1_state12_branch_taken_accumulate", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x0018U, 0x05U, 0x00U, 0x00U,
         0x00000005U, 0x00000003U, 0x00000100U, 0x00000200U,
         0x0002CAD8U, ir::PortableContinuationOutcome::CompiledNextPc, 180,
         0x00000105U, 0x00000203U},
        {"dispatch_path2_state12_branch_not_taken_uncompiled", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x0018U, 0x01U, 0x40U, 0x08U,
         0x00000000U, 0x00000000U, 0x00000100U, 0x00000200U,
         0x0062CCU, ir::PortableContinuationOutcome::ValidUncompiledPc, 114,
         0x00000100U, 0x00000200U},
        {"dispatch_path3_state17_timer_tick_branch_taken", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x0022U, 0x00U, 0x00U, 0x00U,
         0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
         0x0002CAD8U, ir::PortableContinuationOutcome::CompiledNextPc, 108,
         0x00000000U, 0x00000000U},
        {"dispatch_path4_state14_timer_expire_advance_states", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x001CU, 0x00U, 0x00U, 0x08U,
         0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
         0x0002CAD8U, ir::PortableContinuationOutcome::CompiledNextPc, 142,
         0x00000000U, 0x00000000U},
        {"dispatch_path5_uncompiled_target_state0", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x0000U, 0x00U, 0x00U, 0x00U,
         0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
         0x0061F6U, ir::PortableContinuationOutcome::ValidUncompiledPc, 58,
         0x00000000U, 0x00000000U},
        {"dispatch_path6_out_of_bounds_table_exception", 0x0059A8U, 0x00FF8000U, 0x00FFFE00U,
         0x0080U, 0x00U, 0x00U, 0x00U,
         0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
         0x0059B2U, ir::PortableContinuationOutcome::OriginalCpuException, 28,
         0x00000000U, 0x00000000U},
    };
    return kFixtures;
}

} // namespace oasis::recompiler::fixtures

#endif // OASIS_DATA_THOR_FIXTURES_M15_243_ENTITY_DISPATCH_FIXTURES_HPP_
