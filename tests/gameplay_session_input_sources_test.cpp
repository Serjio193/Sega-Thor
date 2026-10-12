#include "tools/hybrid/gameplay_session_input_sources.hpp"

#include <libretro.h>

int main() {
    using oasis::hybrid::session::InputMode;
    using oasis::hybrid::session::InputSource;
    using oasis::hybrid::session::capture_sample;
    const auto boot = capture_sample(0, InputMode::Fallback);
    if (boot.buttons != (1U << RETRO_DEVICE_ID_JOYPAD_START) ||
        boot.source != InputSource::FallbackSchedule) return 1;
    const auto scripted = capture_sample(120, InputMode::Scripted);
    if (scripted.buttons != (1U << RETRO_DEVICE_ID_JOYPAD_RIGHT) ||
        scripted.source != InputSource::InjectedScripted) return 2;
    const auto neutral = capture_sample(1300, InputMode::Fallback);
    if (neutral.source != InputSource::FallbackSchedule) return 3;
    return 0;
}
