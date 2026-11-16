#include "game/render/dytto_sat_renderer.hpp"

#include "game/graphics_decompress.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace oasis::game::render {

std::size_t upload_dytto_tiles_to_vram(
    std::span<const std::uint8_t> rom,
    genesis::VdpState& vdp) noexcept {
    constexpr std::uint32_t kRomSrc = spirits::DyttoEntityConfig::kRomGfxSource;   // 0x00141580
    constexpr std::uint32_t kVramDst = spirits::DyttoEntityConfig::kVramByteOffset; // 0x9A00 (tile 0x04D0)

    if (rom.empty() || kRomSrc >= rom.size() || kVramDst >= vdp.vram.size()) {
        return 0U;
    }

    // ROM 0x01454C decompresses 0x00141580 into RAM $FF2FA8 via 0x003820 (2464 bytes / 77 tiles).
    std::array<std::uint8_t, 4096> decomp_buf{};
    try {
        const auto res = decompress_graphics(rom.subspan(kRomSrc), decomp_buf);
        const std::size_t max_bytes = vdp.vram.size() - static_cast<std::size_t>(kVramDst);
        const std::size_t copy_bytes = std::min(res.output_size, max_bytes);
        if (copy_bytes == 0U) {
            return 0U;
        }
        std::memcpy(vdp.vram.data() + kVramDst, decomp_buf.data(), copy_bytes);
        return copy_bytes;
    } catch (...) {
        return 0U;
    }
}

void emit_dytto_sat(
    SatPipelineContract& sat,
    const camera::CameraController& camera,
    const spirits::DyttoSpiritController& dytto) noexcept {
    emit_dytto_sat_at(sat, camera.display_camera_x(), camera.display_camera_y(), dytto);
}

void emit_dytto_sat_at(
    SatPipelineContract& sat,
    int cam_x,
    int cam_y,
    const spirits::DyttoSpiritController& dytto) noexcept {
    if (!dytto.is_active()) {
        return;
    }

    const int sx = dytto.world_x() - cam_x;
    const int sy = dytto.world_y() - dytto.world_z() - cam_y;
    const int ground_y = dytto.world_y() - cam_y;

    // Base VRAM tile 0x04D0 (byte offset 0x9A00) uploaded by 0x01454C.
    constexpr std::uint16_t kBaseTile = spirits::DyttoEntityConfig::kVramTileBase; // 0x04D0
    // In 0..255 binary angle space, angles (64..192) face left (-X).
    const bool h_flip = (dytto.current_angle() > 64U && dytto.current_angle() < 192U);

    if (dytto.state() == spirits::DyttoSpiritController::State::AppearCutscene) {
        const std::uint16_t t = dytto.appear_timer();
        // 240f..299f: visual palette flash / pulse effect (0x014F60: 0x00F0).
        const std::uint8_t palette =
            (t >= spirits::DyttoEntityConfig::kTimelinePalettePulseFrame &&
             t < spirits::DyttoEntityConfig::kTimelineDialogueCompleteFrame &&
             ((t / 4U) % 2U) != 0U)
                ? 2U
                : 1U;

        // Main Dytto body sprite (3x4 cells = 24x32 px) centered horizontally.
        sat.emit_piece(sx - 12, sy - 24, 3, 4, kBaseTile, palette, true, h_flip, false);

        // 180f..239f: appearance burst particle sprite (0x014F60: 0x00B4).
        if (t >= spirits::DyttoEntityConfig::kTimelineBurstSfxFrame &&
            t < spirits::DyttoEntityConfig::kTimelinePalettePulseFrame) {
            sat.emit_piece(sx - 8, sy - 28, 2, 2, static_cast<std::uint16_t>(kBaseTile + 12U), 2U, true, false, false);
        }

        // Ground shadow beneath elevated altar spawn.
        sat.emit_shadow(sx - 8, ground_y - 4, 0x054BU, 0U);
        return;
    }

    // Active gameplay / combat states (0x00..0x18): 3x4 sprite + shadow + active 0x0024 projectiles.
    sat.emit_piece(sx - 12, sy - 24, 3, 4, kBaseTile, 1U, true, h_flip, false);
    sat.emit_shadow(sx - 8, ground_y - 4, 0x054BU, 0U);

    for (const auto& proj : dytto.projectiles()) {
        if (!proj.active) continue;
        const int px = proj.world_x - cam_x;
        const int py = proj.world_y - proj.world_z - cam_y;
        sat.emit_piece(px - 8, py - 8, 2, 2, static_cast<std::uint16_t>(kBaseTile + 16U), 2U, true, false, false);
    }
}

} // namespace oasis::game::render
