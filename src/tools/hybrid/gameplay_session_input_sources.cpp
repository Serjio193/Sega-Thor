#include "tools/hybrid/gameplay_session_input_sources.hpp"

#include <libretro.h>

#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace oasis::hybrid::session {
namespace {
Sample fallback_sample(unsigned frame) {
    if (frame < 1200) {
        if (frame % 120 == 0) return {1U << RETRO_DEVICE_ID_JOYPAD_START, InputSource::FallbackSchedule};
        if (frame % 60 == 0) return {1U << RETRO_DEVICE_ID_JOYPAD_A, InputSource::FallbackSchedule};
        if (frame % 60 == 30) return {1U << RETRO_DEVICE_ID_JOYPAD_X, InputSource::FallbackSchedule};
        return {};
    }
    const auto phase = (frame / 30) % 8;
    std::uint16_t mask{};
    if (phase == 0) mask = 1U << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (phase == 1) mask = (1U << RETRO_DEVICE_ID_JOYPAD_RIGHT) |
        (frame % 8 < 2 ? 1U << RETRO_DEVICE_ID_JOYPAD_B : 0);
    if (phase == 2) mask = 1U << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (phase == 3) mask = 1U << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (phase == 4) mask = (1U << RETRO_DEVICE_ID_JOYPAD_LEFT) |
        (frame % 8 < 2 ? 1U << RETRO_DEVICE_ID_JOYPAD_B : 0);
    if (phase == 5) mask = 1U << RETRO_DEVICE_ID_JOYPAD_UP;
    if (phase == 6) mask = (1U << RETRO_DEVICE_ID_JOYPAD_RIGHT) |
        (frame % 15 == 0 ? 1U << RETRO_DEVICE_ID_JOYPAD_X : 0);
    if (phase == 7 && frame % 20 == 0) mask = 1U << RETRO_DEVICE_ID_JOYPAD_A;
    return {mask, InputSource::FallbackSchedule};
}

Sample scripted_sample(unsigned frame) {
    if (frame == 0) return {1U << RETRO_DEVICE_ID_JOYPAD_START, InputSource::InjectedScripted};
    if (frame >= 120 && frame < 180) return {1U << RETRO_DEVICE_ID_JOYPAD_RIGHT, InputSource::InjectedScripted};
    if (frame >= 180 && frame < 240) return {1U << RETRO_DEVICE_ID_JOYPAD_DOWN, InputSource::InjectedScripted};
    if (frame >= 240 && frame < 300) return {1U << RETRO_DEVICE_ID_JOYPAD_LEFT, InputSource::InjectedScripted};
    if (frame >= 300 && frame < 360) return {1U << RETRO_DEVICE_ID_JOYPAD_UP, InputSource::InjectedScripted};
    if (frame >= 600 && frame < 603) return {1U << RETRO_DEVICE_ID_JOYPAD_A, InputSource::InjectedScripted};
    return {0, InputSource::InjectedScripted};
}

Sample replay_c_sample(unsigned frame) {
    if (frame < 2300U) return fallback_sample(frame);
    const auto offset = frame - 2300U;
    const auto phase = (offset / 72U) % 4U;
    const auto phase_frame = offset % 72U;
    std::uint16_t mask{};
    switch (phase) {
    case 0:
        mask = 1U << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        if (phase_frame < 24U) mask |= 1U << RETRO_DEVICE_ID_JOYPAD_B;
        break;
    case 1:
        mask = 1U << RETRO_DEVICE_ID_JOYPAD_UP;
        if (phase_frame < 24U) mask |= 1U << RETRO_DEVICE_ID_JOYPAD_A;
        break;
    case 2:
        mask = 1U << RETRO_DEVICE_ID_JOYPAD_LEFT;
        if (phase_frame < 24U) mask |= 1U << RETRO_DEVICE_ID_JOYPAD_X;
        break;
    case 3:
        mask = 1U << RETRO_DEVICE_ID_JOYPAD_DOWN;
        if (phase_frame < 24U) mask |= 1U << RETRO_DEVICE_ID_JOYPAD_Y;
        break;
    }
    return {mask, InputSource::InjectedScripted};
}

#ifdef _WIN32
struct Pad { unsigned short buttons; unsigned char left_trigger, right_trigger; short lx, ly, rx, ry; };
struct PadState { unsigned packet; Pad pad; };
using XInputGetState = unsigned long (WINAPI*)(unsigned long, PadState*);
std::uint16_t physical_sample() {
    static HMODULE dll = LoadLibraryA("xinput1_4.dll");
    static auto get_state = dll ? reinterpret_cast<XInputGetState>(GetProcAddress(dll, "XInputGetState")) : nullptr;
    if (!get_state) throw std::runtime_error("physical controller requested but XInput is unavailable");
    PadState state{};
    if (get_state(0, &state) != 0) throw std::runtime_error("physical controller disconnected");
    const auto b = state.pad.buttons;
    std::uint16_t mask{};
    const auto set = [&](unsigned bit, unsigned id) { if (b & bit) mask |= static_cast<std::uint16_t>(1U << id); };
    set(0x0001, RETRO_DEVICE_ID_JOYPAD_UP); set(0x0002, RETRO_DEVICE_ID_JOYPAD_DOWN);
    set(0x0004, RETRO_DEVICE_ID_JOYPAD_LEFT); set(0x0008, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    set(0x0010, RETRO_DEVICE_ID_JOYPAD_START); set(0x0020, RETRO_DEVICE_ID_JOYPAD_SELECT);
    set(0x1000, RETRO_DEVICE_ID_JOYPAD_A); set(0x2000, RETRO_DEVICE_ID_JOYPAD_B);
    set(0x4000, RETRO_DEVICE_ID_JOYPAD_X); set(0x8000, RETRO_DEVICE_ID_JOYPAD_Y);
    return mask;
}
#endif
} // namespace

Sample capture_sample(unsigned frame, InputMode mode) {
#ifdef _WIN32
    if (mode == InputMode::Physical) return {physical_sample(), InputSource::PhysicalOverride};
    if (mode == InputMode::LiveOverride) {
        const auto buttons = physical_sample();
        if (buttons) return {buttons, InputSource::PhysicalOverride};
        return fallback_sample(frame);
    }
#else
    if (mode == InputMode::Physical || mode == InputMode::LiveOverride)
        throw std::runtime_error("physical input capture is currently Windows-only");
#endif
    if (mode == InputMode::Scripted) return scripted_sample(frame);
    if (mode == InputMode::ReplayC) return replay_c_sample(frame);
    return fallback_sample(frame);
}

InputMode parse_input_mode(std::string_view mode) {
    if (mode == "fallback") return InputMode::Fallback;
    if (mode == "physical") return InputMode::Physical;
    if (mode == "scripted") return InputMode::Scripted;
    if (mode == "replay-c") return InputMode::ReplayC;
    if (mode == "live") return InputMode::LiveOverride;
    throw std::runtime_error("input mode must be fallback, physical, scripted, replay-c, or live");
}

} // namespace oasis::hybrid::session
