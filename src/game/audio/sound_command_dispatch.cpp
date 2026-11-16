#include "game/audio/sound_command_dispatch.hpp"

namespace oasis::game::audio {

DecodedSoundCommand classify_sound_command(std::uint32_t cmd_d0) noexcept {
    DecodedSoundCommand out{};
    out.raw_cmd = cmd_d0;
    out.priority = static_cast<std::uint8_t>((cmd_d0 >> 16U) & 0xFFU);
    out.sub_id = static_cast<std::uint8_t>(cmd_d0 & 0xFFU);

    const std::uint16_t cmd_word = static_cast<std::uint16_t>(cmd_d0 & 0xFFFFU);
    if ((cmd_word & 0xFF00U) == 0U) {
        if (out.sub_id < SoundDriverConfig::kSystemHandlers.size()) {
            out.category = SoundCommandCategory::System;
            out.target_pc = SoundDriverConfig::kSystemHandlers[out.sub_id];
        }
        return out;
    }

    switch ((cmd_word >> 8U) & 0xFFU) {
    case 0x01U:
        out.category = SoundCommandCategory::Bgm;
        out.target_pc = SoundDriverConfig::kHandlerBgm;
        break;
    case 0x02U:
        out.category = SoundCommandCategory::SfxBank2;
        out.target_pc = SoundDriverConfig::kHandlerSfxBank2;
        break;
    case 0x03U:
        out.category = SoundCommandCategory::SfxBank3;
        out.target_pc = SoundDriverConfig::kHandlerSfxBank3;
        break;
    case 0x04U:
        out.category = SoundCommandCategory::SfxBank4;
        out.target_pc = SoundDriverConfig::kHandlerSfxBank4;
        break;
    case 0x05U:
        out.category = SoundCommandCategory::Category5;
        out.target_pc = SoundDriverConfig::kHandlerCategory5;
        break;
    default:
        out.category = SoundCommandCategory::Unknown;
        out.target_pc = 0U;
        break;
    }
    return out;
}

void SoundDriverState::reset() noexcept {
    initialized_ = false;
    tick_count_ = 0U;
    active_bgm_id_.reset();
    bgm_fading_ = false;
    history_.clear();
}

void SoundDriverState::init_or_tick() noexcept {
    initialized_ = true;
}

DecodedSoundCommand SoundDriverState::enqueue_command(std::uint32_t cmd_d0) noexcept {
    const DecodedSoundCommand decoded = classify_sound_command(cmd_d0);
    if (decoded.category == SoundCommandCategory::System && decoded.sub_id == 0x02U) {
        // 0x02 -> 0x06064C: Fade/Stop BGM
        active_bgm_id_.reset();
        bgm_fading_ = true;
    } else if (decoded.category == SoundCommandCategory::Bgm) {
        active_bgm_id_ = decoded.sub_id;
        bgm_fading_ = false;
    }
    history_.push_back(decoded);
    return decoded;
}

void SoundDriverState::hardware_tick() noexcept {
    ++tick_count_;
}

std::size_t SoundDriverState::dispatch_queue(const combat::SoundCommandQueue& queue) noexcept {
    std::size_t count = 0U;
    for (const auto& ev : queue.events()) {
        enqueue_command(ev.command_id());
        ++count;
    }
    return count;
}

} // namespace oasis::game::audio
