#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_entity_cluster_types.hpp"

namespace oasis::hybrid::entity_cluster {

[[nodiscard]] M68k9D70StepResult step_entity_cluster(
    unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

[[nodiscard]] bool is_cluster_owned_pc(unsigned pc) noexcept;

} // namespace oasis::hybrid::entity_cluster
