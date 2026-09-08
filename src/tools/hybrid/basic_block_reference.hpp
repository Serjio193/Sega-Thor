#pragma once

#include "tools/hybrid/basic_block.hpp"

namespace oasis::hybrid {

[[nodiscard]] BasicBlockRegistry::Prediction predict_generated_block(
    const BasicBlockApi& source, const GeneratedBlockSpec& block,
    const BasicBlockRegistry::Prediction& entry, unsigned entry_pc,
    unsigned instruction_limit = 0);

} // namespace oasis::hybrid
