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

std::optional<std::uint32_t> lookup_bgm_sequence_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint8_t track_id) noexcept {
    const std::size_t entry_offset =
        static_cast<std::size_t>(SoundDriverConfig::kBgmOffsetTableRom) +
        static_cast<std::size_t>(track_id) * 4U;
    if (entry_offset + 4U > rom.size()) {
        return std::nullopt;
    }
    const std::uint32_t rel =
        (static_cast<std::uint32_t>(rom[entry_offset]) << 24U) |
        (static_cast<std::uint32_t>(rom[entry_offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(rom[entry_offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(rom[entry_offset + 3U]);
    if (rel == 0U) {
        return std::nullopt;
    }
    const std::uint64_t seq_addr =
        static_cast<std::uint64_t>(SoundDriverConfig::kBgmOffsetTableRom) + rel;
    if (seq_addr >= rom.size()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(seq_addr);
}

void Z80BusHandshakeState::reset() noexcept {
    *this = Z80BusHandshakeState{};
}

void Z80BusHandshakeState::begin_stop_and_load(std::uint32_t bgm_rom_addr) noexcept {
    bus_requested = true;
    bus_req_word = SoundDriverConfig::kZ80BusRequestWord; // 0x0100 -> $00A11100
    z80_cmd_byte = SoundDriverConfig::kZ80StopByte;       // 0x83 -> $00A00003
    awaiting_ack = true;
    pending_bgm_rom_addr = bgm_rom_addr;
}

void Z80BusHandshakeState::release_bus() noexcept {
    bus_requested = false;
    bus_req_word = 0U;
}

void Z80BusHandshakeState::acknowledge_z80_ready() noexcept {
    z80_cmd_byte = 0U; // $00A00003 == 0
    awaiting_ack = false;
    ++completed_handshakes;
}

void SoundDriverState::reset() noexcept {
    initialized_ = false;
    tick_count_ = 0U;
    active_bgm_id_.reset();
    last_bgm_rom_addr_.reset();
    z80_handshake_.reset();
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
        last_bgm_rom_addr_.reset();
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

std::size_t SoundDriverState::dispatch_queue(
    const combat::SoundCommandQueue& queue,
    std::span<const std::uint8_t> rom) noexcept {
    std::size_t count = 0U;
    for (const auto& ev : queue.events()) {
        const DecodedSoundCommand decoded = enqueue_command(ev.command_id());
        if (decoded.category == SoundCommandCategory::Bgm && !rom.empty()) {
            const auto resolved = lookup_bgm_sequence_rom_addr(rom, decoded.sub_id);
            last_bgm_rom_addr_ = resolved;
            if (resolved.has_value()) {
                z80_handshake_.begin_stop_and_load(*resolved);
            }
        }
        ++count;
    }
    return count;
}

} // namespace oasis::game::audio
