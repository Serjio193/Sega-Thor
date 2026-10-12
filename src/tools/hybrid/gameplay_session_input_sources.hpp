#pragma once

#include "tools/hybrid/gameplay_session.hpp"

#include <cstdint>

namespace oasis::hybrid::session {

enum class InputMode { Fallback, Physical, Scripted, LiveOverride };

struct Sample {
    std::uint16_t buttons{};
    InputSource source{InputSource::FallbackSchedule};
};

[[nodiscard]] Sample capture_sample(unsigned frame, InputMode mode);

} // namespace oasis::hybrid::session
