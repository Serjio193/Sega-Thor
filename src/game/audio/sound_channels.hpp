#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::audio {

enum class SoundChannelKind : std::uint8_t {
    Fm = 0,
    Psg,
    SfxFm,
    SfxPsg,
};

struct SoundChannelConfig {
    static constexpr std::uint32_t kStopAllAddr = 0x000604E6U;
    static constexpr std::uint32_t kFadeAllAddr = 0x00060516U;
    static constexpr std::size_t kChannelCount = 11U;
    static constexpr std::size_t kFmStrideBytes = 0xCAU;  // 202
    static constexpr std::size_t kPsgStrideBytes = 0x8AU; // 138
    static constexpr std::uint8_t kFadeBit4 = 0x10U;      // +0x75 bit 4
    static constexpr std::uint8_t kFadeInitStep = 2U;
    static constexpr std::uint8_t kFadeInitTimer = 0x7EU; // 126
};

struct SoundChannelState {
    std::uint8_t index{0U};
    SoundChannelKind kind{SoundChannelKind::Fm};
    std::uint32_t ram_addr{0U};
    bool active{false};
    std::uint8_t fade_rate_6d{0U};    // +0x6D
    std::uint8_t fade_counter_6e{0U}; // +0x6E
    std::uint8_t flags_75{0U};        // +0x75 (bit 4 = fading)
    std::uint8_t attenuation{0U};     // 0 = full volume, higher = quieter
};

// ROM 0x0604E6..0x06064B: 11-channel FM/PSG/SFX bank ($FF0022..$FF07BB).
class SoundChannelBank {
public:
    SoundChannelBank() noexcept { reset(); }

    void reset() noexcept;

    // ROM 0x0604E6 (System Cmd 0x00): Mutes all 11 channels and clears global flags ($FF0010..$FF0014).
    void stop_all() noexcept;

    // ROM 0x060516 (System Cmd 0x07): Sets step 2, timer 0x7E, and bit 4 on +0x75 across all 11 channels.
    void start_fade_all(std::uint8_t rate = SoundChannelConfig::kFadeInitStep) noexcept;

    // Advances fade attenuation across all fading channels; mutes channels when timer expires.
    void step_frame() noexcept;

    [[nodiscard]] const std::array<SoundChannelState, SoundChannelConfig::kChannelCount>& channels() const noexcept {
        return channels_;
    }
    [[nodiscard]] std::array<SoundChannelState, SoundChannelConfig::kChannelCount>& channels() noexcept {
        return channels_;
    }
    [[nodiscard]] std::uint16_t global_flags_ff0010() const noexcept { return global_flags_ff0010_; }
    [[nodiscard]] std::uint8_t fade_step() const noexcept { return fade_step_; }
    [[nodiscard]] std::uint8_t fade_timer() const noexcept { return fade_timer_; }
    [[nodiscard]] bool is_fading() const noexcept { return fade_timer_ > 0U; }

private:
    std::array<SoundChannelState, SoundChannelConfig::kChannelCount> channels_{};
    std::uint16_t global_flags_ff0010_{0U}; // $FF0010..$FF0014
    std::uint8_t fade_step_{0U};
    std::uint8_t fade_timer_{0U};
};

} // namespace oasis::game::audio
