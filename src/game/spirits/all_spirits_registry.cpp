#include "game/spirits/all_spirits_registry.hpp"

#include "game/graphics_decompress.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace oasis::game::spirits {

std::optional<SpiritResourceSpec> get_spirit_spec(std::uint8_t spirit_index) noexcept {
    if (spirit_index >= kAllSpiritSpecs.size()) {
        return std::nullopt;
    }
    return kAllSpiritSpecs[spirit_index];
}

std::optional<SpiritResourceSpec> get_spirit_spec_by_type(std::uint16_t type_id) noexcept {
    for (const auto& s : kAllSpiritSpecs) {
        if (s.type_id == type_id) {
            return s;
        }
    }
    return std::nullopt;
}

std::size_t upload_spirit_tiles_to_vram(
    std::span<const std::uint8_t> rom,
    std::uint8_t spirit_index,
    genesis::VdpState& vdp) noexcept {
    const auto spec = get_spirit_spec(spirit_index);
    if (!spec.has_value() || rom.empty() ||
        spec->nemesis_rom_addr >= rom.size() ||
        spec->vram_byte_addr >= vdp.vram.size()) {
        return 0U;
    }

    // ROM 0x014522: decompresses via 0x003820 into $FF2FA8, then DMAs to target VRAM address.
    std::array<std::uint8_t, 8192> decomp_buf{};
    try {
        const auto res = decompress_graphics(rom.subspan(spec->nemesis_rom_addr), decomp_buf);
        const std::size_t max_bytes = vdp.vram.size() - static_cast<std::size_t>(spec->vram_byte_addr);
        const std::size_t copy_bytes = std::min(res.output_size, max_bytes);
        if (copy_bytes == 0U) {
            return 0U;
        }
        std::memcpy(vdp.vram.data() + spec->vram_byte_addr, decomp_buf.data(), copy_bytes);
        return copy_bytes;
    } catch (...) {
        return 0U;
    }
}

void ActiveSpiritController::reset() noexcept {
    active_ = false;
    spirit_index_ = 0U;
    type_id_ = 0U;
    vram_tile_ = 0U;
    max_sp_ = 100U;
    current_sp_ = 100U;
    sp_drain_rate_ = 10U;
    world_x_ = 0;
    world_y_ = 0;
    world_z_ = 0;
    vram_uploaded_bytes_ = 0U;
    sp_drain_timer_ = 0U;
    despawned_ = false;
    last_despawn_handler_pc_ = 0U;
    dytto_.reset();
}

bool ActiveSpiritController::summon_spirit_by_index(
    std::uint8_t spirit_index,
    std::int32_t spawn_x,
    std::int32_t spawn_y,
    std::int32_t spawn_z,
    std::span<const std::uint8_t> rom,
    genesis::VdpState* vdp) noexcept {
    const auto spec = get_spirit_spec(spirit_index);
    if (!spec.has_value()) {
        return false;
    }

    active_ = true;
    spirit_index_ = spec->spirit_index;
    type_id_ = spec->type_id;
    vram_tile_ = spec->vram_tile;
    max_sp_ = spec->max_sp;
    current_sp_ = spec->max_sp;
    sp_drain_rate_ = spec->sp_drain_rate;
    world_x_ = spawn_x;
    world_y_ = spawn_y;
    world_z_ = spawn_z;
    vram_uploaded_bytes_ = 0U;
    sp_drain_timer_ = spec->sp_drain_interval_frames;
    despawned_ = false;
    last_despawn_handler_pc_ = 0U;

    if (spirit_index == 0U) {
        dytto_.init_spawn(spawn_x, spawn_y, spawn_z, 2U, false);
    } else {
        dytto_.reset();
    }

    if (!rom.empty() && vdp != nullptr) {
        vram_uploaded_bytes_ = upload_spirit_tiles_to_vram(rom, spirit_index, *vdp);
        if (spirit_index == 0U && vram_uploaded_bytes_ > 0U) {
            dytto_.clear_dma_requested();
        }
    }

    return true;
}

bool ActiveSpiritController::step_sp_drain(std::uint16_t& player_sp) noexcept {
    if (!active_) {
        return false;
    }
    const auto spec = get_spirit_spec(spirit_index_);
    if (!spec.has_value()) {
        return false;
    }

    if (sp_drain_timer_ > 0U) {
        --sp_drain_timer_;
    }
    if (sp_drain_timer_ == 0U) {
        sp_drain_timer_ = spec->sp_drain_interval_frames;
        if (player_sp > 0U) {
            --player_sp;
        }
        if (player_sp == 0U) {
            active_ = false;
            despawned_ = true;
            last_despawn_handler_pc_ = spec->despawn_handler_pc;
            dytto_.reset();
            return true;
        }
    }
    return false;
}

} // namespace oasis::game::spirits
