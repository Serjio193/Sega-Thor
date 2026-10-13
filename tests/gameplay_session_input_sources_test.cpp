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
    const auto c_prefix = capture_sample(2299, InputMode::ReplayC);
    const auto a_prefix = capture_sample(2299, InputMode::Fallback);
    if (c_prefix.buttons != a_prefix.buttons || c_prefix.source != InputSource::FallbackSchedule) return 4;
    const auto c_diverge = capture_sample(2300, InputMode::ReplayC);
    if (c_diverge.buttons != ((1U << RETRO_DEVICE_ID_JOYPAD_RIGHT) |
        (1U << RETRO_DEVICE_ID_JOYPAD_B)) ||
        c_diverge.source != InputSource::InjectedScripted) return 5;
    const auto c_action = capture_sample(2372, InputMode::ReplayC);
    if (c_action.buttons != ((1U << RETRO_DEVICE_ID_JOYPAD_UP) |
        (1U << RETRO_DEVICE_ID_JOYPAD_A))) return 6;
    if (oasis::hybrid::session::parse_input_mode("replay-c") != InputMode::ReplayC) return 7;
    return 0;
}
