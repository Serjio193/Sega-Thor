#pragma once

#include <string_view>

#include "tools/hybrid/basic_block.hpp"

namespace oasis::hybrid {

unsigned parse_runner_target(std::string_view text);
void set_boundary_reason_provider(unsigned (*provider)());
BlockExitReason runner_boundary_reason() noexcept;

} // namespace oasis::hybrid
