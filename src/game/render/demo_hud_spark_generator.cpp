#include "game/render/demo_hud_spark_generator.hpp"

#include <algorithm>

namespace oasis::game::render {

namespace {

[[nodiscard]] inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0;
    }
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

[[nodiscard]] inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 1 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_u32_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 3 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        buf[offset + 2] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 3] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

} // namespace

DemoOverlayResult process_demo_overlay(
    std::uint16_t status_flags,
    std::uint8_t current_sync,
    std::uint16_t current_sprites) noexcept {
    DemoOverlayResult result{};

    // Bit 7 set: demo overlay is suppressed (0x00A7EE)
    if ((status_flags & 0x0080U) != 0U) {
        return result;
    }

    if ((status_flags & 0x0040U) != 0U) {
        // Mode 3: 3 sprites
        result.sprite_count = 3U;
        if (current_sync != 1U) {
            result.dma = DemoDmaCommand{
                .source_addr = DemoHudSparkConfig::kMode3DmaSrc,
                .vram_dest = DemoHudSparkConfig::kMode3DmaVram,
                .word_count = DemoHudSparkConfig::kMode3DmaWords,
            };
        }

        const std::int16_t xs[3] = {
            DemoHudSparkConfig::kMode3X0,
            DemoHudSparkConfig::kMode3X1,
            DemoHudSparkConfig::kMode3X2,
        };

        for (std::size_t i = 0U; i < 3U; ++i) {
            if (current_sprites + i < DemoHudSparkConfig::kMaxSatSprites) {
                DemoSatSprite sp{};
                sp.y = DemoHudSparkConfig::kMode3OverlayY;
                sp.size = 0x0FU; // 4x4
                sp.link = static_cast<std::uint8_t>(current_sprites + i + 1U);
                sp.attr = 0x879CU + static_cast<std::uint16_t>(i * 16U);
                sp.x = xs[i];
                result.sprites.push_back(sp);
            }
        }
    } else {
        // Mode 2: 2 sprites
        result.sprite_count = 2U;
        if (current_sync != 0xFFU) {
            result.dma = DemoDmaCommand{
                .source_addr = DemoHudSparkConfig::kMode2DmaSrc,
                .vram_dest = DemoHudSparkConfig::kMode2DmaVram,
                .word_count = DemoHudSparkConfig::kMode2DmaWords,
            };
        }

        const std::int16_t xs[2] = {
            DemoHudSparkConfig::kMode2X0,
            DemoHudSparkConfig::kMode2X1,
        };

        for (std::size_t i = 0U; i < 2U; ++i) {
            if (current_sprites + i < DemoHudSparkConfig::kMaxSatSprites) {
                DemoSatSprite sp{};
                sp.y = DemoHudSparkConfig::kMode2OverlayY;
                sp.size = 0x0FU; // 4x4
                sp.link = static_cast<std::uint8_t>(current_sprites + i + 1U);
                sp.attr = 0x879CU + static_cast<std::uint16_t>(i * 16U);
                sp.x = xs[i];
                result.sprites.push_back(sp);
            }
        }
    }

    return result;
}

SparkEmissionResult process_spark_particle(
    SparkParticleState& spark,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept {
    SparkEmissionResult res{};

    // Decrement lifetime (0x00A8F4)
    --spark.lifetime;
    if (spark.lifetime <= 0) {
        spark.type = 0;
        res.expired = true;
        return res;
    }

    // Hardware sprite limit check (0x00A8FE)
    if (current_sprites >= DemoHudSparkConfig::kMaxSatSprites) {
        return res;
    }

    // Screen coordinate calculation (0x00A950..0x00A980)
    const std::int16_t screen_y = static_cast<std::int16_t>(
        (spark.world_y - cam_y) + DemoHudSparkConfig::kScreenCoordBias);
    const std::int16_t screen_x = static_cast<std::int16_t>(
        (spark.world_x - cam_x) + DemoHudSparkConfig::kScreenCoordBias);

    // Screen bounds clipping against [0..240] and [0..336] relative to margin
    const std::int16_t adj_y = static_cast<std::int16_t>(
        screen_y - DemoHudSparkConfig::kScreenClipMargin);
    if (adj_y < 0 || adj_y > DemoHudSparkConfig::kMaxScreenClippedY) {
        return res;
    }

    const std::int16_t adj_x = static_cast<std::int16_t>(
        screen_x - DemoHudSparkConfig::kScreenClipMargin);
    if (adj_x < 0 || adj_x > DemoHudSparkConfig::kMaxScreenClippedX) {
        return res;
    }

    res.emitted = true;
    res.screen_x = screen_x;
    res.screen_y = screen_y;
    return res;
}

std::vector<SparkEmissionResult> step_spark_pool(
    std::span<SparkParticleState> sparks,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t& current_sprites,
    std::uint16_t& current_sat_offset) {
    std::vector<SparkEmissionResult> results;
    results.reserve(sparks.size());

    for (auto& spark : sparks) {
        if (!spark.is_active()) {
            SparkEmissionResult inactive_res{};
            inactive_res.expired = true;
            results.push_back(inactive_res);
            continue;
        }

        auto res = process_spark_particle(spark, cam_x, cam_y, current_sprites);
        if (res.emitted) {
            current_sprites += 1U;
            current_sat_offset = static_cast<std::uint16_t>(current_sat_offset + 8U);
        }
        results.push_back(res);
    }

    return results;
}

DemoHudSparkPassResult process_demo_hud_spark_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    DemoHudSparkPassResult result{};

    // Read current state
    const std::size_t status_pos = ram_offset(DemoHudSparkConfig::kDemoStatusRam);
    const std::uint16_t status_flags = (status_pos < ram_buffer.size()) ?
                                       read_u16_be(ram_buffer, status_pos) : 0U;

    const std::size_t sync_pos = ram_offset(DemoHudSparkConfig::kDemoSyncRam);
    const std::uint8_t sync_val = (sync_pos < ram_buffer.size()) ?
                                  ram_buffer[sync_pos] : 0U;

    const std::size_t link_pos = ram_offset(DemoHudSparkConfig::kSatLinkCountRam);
    std::uint16_t current_sprites = (link_pos < ram_buffer.size()) ?
                                    read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(DemoHudSparkConfig::kSatByteOffsetRam);
    std::uint16_t current_sat_offset = (sat_off_pos < ram_buffer.size()) ?
                                       read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Process demo overlay
    result.demo_overlay = process_demo_overlay(status_flags, sync_val, current_sprites);

    // Queue DMA if generated
    if (result.demo_overlay.dma.has_value()) {
        const auto& dma = *result.demo_overlay.dma;
        const std::size_t dma_q_pos = ram_offset(DemoHudSparkConfig::kDmaQueueRam);
        if (dma_q_pos + 8 <= ram_buffer.size()) {
            write_u32_be(ram_buffer, dma_q_pos, dma.source_addr);
            write_u16_be(ram_buffer, dma_q_pos + 4, dma.vram_dest);
            write_u16_be(ram_buffer, dma_q_pos + 6, dma.word_count);
        }
    }

    // Emit demo overlay sprites to SAT buffer
    const std::size_t sat_base_pos = ram_offset(DemoHudSparkConfig::kVdpSatBufferRam);
    for (const auto& sp : result.demo_overlay.sprites) {
        const std::size_t entry_pos = sat_base_pos + current_sat_offset;
        if (entry_pos + 8 <= ram_buffer.size()) {
            write_u16_be(ram_buffer, entry_pos, static_cast<std::uint16_t>(sp.y));
            ram_buffer[entry_pos + 2] = sp.size;
            ram_buffer[entry_pos + 3] = sp.link;
            write_u16_be(ram_buffer, entry_pos + 4, sp.attr);
            write_u16_be(ram_buffer, entry_pos + 6, static_cast<std::uint16_t>(sp.x));
            current_sprites += 1U;
            current_sat_offset = static_cast<std::uint16_t>(current_sat_offset + 8U);
        }
    }

    // Determine camera scroll plane from bit 7 of $FF16F0
    const std::size_t cam_sel_pos = ram_offset(DemoHudSparkConfig::kCameraSelectRam);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   DemoHudSparkConfig::kCameraPlane2Ram :
                                   DemoHudSparkConfig::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    // Read special pool sparks from $FF19C8 (4 slots of 8 bytes)
    std::vector<SparkParticleState> sparks(DemoHudSparkConfig::kSpecialPoolCount);
    for (std::size_t i = 0U; i < DemoHudSparkConfig::kSpecialPoolCount; ++i) {
        const std::size_t sp_pos = ram_offset(DemoHudSparkConfig::kSpecialPoolRam +
                                              static_cast<std::uint32_t>(i * DemoHudSparkConfig::kSpecialPoolStride));
        if (sp_pos + 8 <= ram_buffer.size()) {
            sparks[i].type = read_i16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkTypeOffset);
            sparks[i].world_y = read_i16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkYOffset);
            sparks[i].world_x = read_i16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkXOffset);
            sparks[i].lifetime = read_i16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkLifetimeOffset);
        }
    }

    // Step sparks and emit to SAT buffer
    result.spark_results = step_spark_pool(sparks, cam_x, cam_y, current_sprites, current_sat_offset);

    // Write back updated spark states to RAM
    for (std::size_t i = 0U; i < DemoHudSparkConfig::kSpecialPoolCount; ++i) {
        const std::size_t sp_pos = ram_offset(DemoHudSparkConfig::kSpecialPoolRam +
                                              static_cast<std::uint32_t>(i * DemoHudSparkConfig::kSpecialPoolStride));
        if (sp_pos + 8 <= ram_buffer.size()) {
            write_u16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkTypeOffset,
                         static_cast<std::uint16_t>(sparks[i].type));
            write_u16_be(ram_buffer, sp_pos + DemoHudSparkConfig::kSparkLifetimeOffset,
                         static_cast<std::uint16_t>(sparks[i].lifetime));
        }
    }

    // Write back updated SAT link count and byte offset
    if (link_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, link_pos, current_sprites);
    }
    if (sat_off_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, sat_off_pos, current_sat_offset);
    }

    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;
    return result;
}

} // namespace oasis::game::render
