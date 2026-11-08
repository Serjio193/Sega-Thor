#pragma once

#include "tools/hybrid/m68k_entity_cluster_context.hpp"
#include "tools/hybrid/m68k_entity_cluster_types.hpp"

namespace oasis::hybrid::entity_cluster {

[[nodiscard]] M68k9D70StepResult step_cluster_target11(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept;

[[nodiscard]] M68k9D70StepResult step_cluster_target12(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept;

} // namespace oasis::hybrid::entity_cluster
