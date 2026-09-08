#pragma once

#include "tools/hybrid/basic_block.hpp"

#include <span>

namespace oasis::hybrid::generated {

// Stable declarations retained for the existing provenance tests; new callers
// should use blocks() so the generated registry remains data-driven.
void execute_0x002D66(BasicBlockApi&);
void execute_0x0604BC(BasicBlockApi&);
void execute_0x061032(BasicBlockApi&);
void execute_0x03A85E(BasicBlockApi&);
void execute_0x03A8BA(BasicBlockApi&);
void execute_0x03A88C(BasicBlockApi&);

[[nodiscard]] std::span<const GeneratedBlockSpec> blocks();

} // namespace oasis::hybrid::generated
