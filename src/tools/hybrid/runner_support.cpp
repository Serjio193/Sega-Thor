#include "tools/hybrid/runner_support.hpp"

#include <stdexcept>
#include <string>

namespace oasis::hybrid {

namespace {
unsigned (*boundary_reason_provider)(){};
}

unsigned parse_runner_target(std::string_view text) {
    std::size_t consumed = 0;
    const auto value = std::stoul(std::string(text), &consumed, 0);
    if (consumed != text.size() || value > 0xFFFFFF) throw std::runtime_error("invalid target");
    return value;
}

void set_boundary_reason_provider(unsigned (*provider)()) { boundary_reason_provider = provider; }

BlockExitReason runner_boundary_reason() noexcept {
    if (!boundary_reason_provider) return BlockExitReason::FALLBACK;
    const auto value = boundary_reason_provider();
    return value <= static_cast<unsigned>(BlockExitReason::FALLBACK) ?
        static_cast<BlockExitReason>(value) : BlockExitReason::FALLBACK;
}

} // namespace oasis::hybrid
