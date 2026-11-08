#pragma once

#include "tools/hybrid/m68k_entity_cluster_context.hpp"
#include "tools/hybrid/m68k_entity_cluster_types.hpp"

namespace oasis::hybrid::entity_cluster {

[[nodiscard]] M68k9D70StepResult step_cluster_target10(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer,
    bool execute_child_inline = false) noexcept;

} // namespace oasis::hybrid::entity_cluster
