#pragma once

#include "tools/hybrid/gameplay_session.hpp"

#include <cstdint>
#include <string_view>

namespace oasis::hybrid::session {

enum class InputMode { Fallback, Physical, Scripted, ReplayC, LiveOverride };

struct Sample {
    std::uint16_t buttons{};
    InputSource source{InputSource::FallbackSchedule};
};

[[nodiscard]] Sample capture_sample(unsigned frame, InputMode mode);
[[nodiscard]] InputMode parse_input_mode(std::string_view mode);

} // namespace oasis::hybrid::session
