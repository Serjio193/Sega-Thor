#pragma once

#include "tools/hybrid/m68k_entity_cluster_context.hpp"
#include "tools/hybrid/m68k_entity_cluster_types.hpp"

#include <optional>

namespace oasis::hybrid::entity_cluster {

struct JumpTableResolution {
    unsigned d0_index{};
    unsigned table_address{};
    std::int16_t offset_word{};
    unsigned target_pc{};
    bool is_valid{};
};

[[nodiscard]] JumpTableResolution calculate_jump_target(
    const GuestExecutionContext& ctx, std::uint16_t d0_val) noexcept;

[[nodiscard]] M68k9D70StepResult step_cluster_dispatcher(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept;

} // namespace oasis::hybrid::entity_cluster
