#include "game/audio/sound_channels.hpp"

#include <algorithm>

namespace oasis::game::audio {

namespace {

inline constexpr std::array<std::uint32_t, SoundChannelConfig::kChannelCount> kChannelRamAddrs = {
    0x00FF0022U, 0x00FF00ECU, 0x00FF01B6U, 0x00FF0280U, 0x00FF034AU, 0x00FF0414U, // 6 FM (0xCA)
    0x00FF049EU, 0x00FF0528U, 0x00FF05B2U,                                       // 3 PSG (0x8A)
    0x00FF0628U, 0x00FF06F2U,                                                    // 2 SFX (0xCA)
};

inline constexpr std::array<SoundChannelKind, SoundChannelConfig::kChannelCount> kChannelKinds = {
    SoundChannelKind::Fm, SoundChannelKind::Fm, SoundChannelKind::Fm,
    SoundChannelKind::Fm, SoundChannelKind::Fm, SoundChannelKind::Fm,
    SoundChannelKind::Psg, SoundChannelKind::Psg, SoundChannelKind::Psg,
    SoundChannelKind::SfxFm, SoundChannelKind::SfxPsg,
};

} // namespace

void SoundChannelBank::reset() noexcept {
    for (std::size_t i = 0U; i < SoundChannelConfig::kChannelCount; ++i) {
        channels_[i] = SoundChannelState{
            static_cast<std::uint8_t>(i),
            kChannelKinds[i],
            kChannelRamAddrs[i],
            false,
            0U,
            0U,
            0U,
            0U,
        };
    }
    global_flags_ff0010_ = 0U;
    fade_step_ = 0U;
    fade_timer_ = 0U;
}

void SoundChannelBank::stop_all() noexcept {
    for (auto& ch : channels_) {
        ch.active = false;
        ch.fade_rate_6d = 0U;
        ch.fade_counter_6e = 0U;
        ch.flags_75 &= static_cast<std::uint8_t>(~SoundChannelConfig::kFadeBit4);
        ch.attenuation = 0x7FU;
    }
    global_flags_ff0010_ = 0U;
    fade_step_ = 0U;
    fade_timer_ = 0U;
}

void SoundChannelBank::start_fade_all(std::uint8_t rate) noexcept {
    fade_step_ = (rate > 0U) ? rate : SoundChannelConfig::kFadeInitStep;
    fade_timer_ = SoundChannelConfig::kFadeInitTimer; // 0x7E (126)
    for (auto& ch : channels_) {
        ch.fade_rate_6d = fade_step_;
        ch.fade_counter_6e = 0U;
        ch.flags_75 |= SoundChannelConfig::kFadeBit4;
    }
}

void SoundChannelBank::step_frame() noexcept {
    if (fade_timer_ == 0U) {
        return;
    }
    --fade_timer_;
    for (auto& ch : channels_) {
        if ((ch.flags_75 & SoundChannelConfig::kFadeBit4) == 0U) {
            continue;
        }
        ++ch.fade_counter_6e;
        if (ch.fade_rate_6d > 0U && ch.fade_counter_6e >= ch.fade_rate_6d) {
            ch.fade_counter_6e = 0U;
            ch.attenuation = static_cast<std::uint8_t>(std::min<int>(0x7FU, ch.attenuation + 1));
        }
        if (fade_timer_ == 0U) {
            ch.active = false;
            ch.flags_75 &= static_cast<std::uint8_t>(~SoundChannelConfig::kFadeBit4);
        }
    }
}

} // namespace oasis::game::audio
