#include "game/player/bounding_box_pushable_latch.hpp"

#include <algorithm>

namespace oasis::game::player {

namespace {

inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        std::uint16_t u = (static_cast<std::uint16_t>(buf[offset]) << 8U) |
                          static_cast<std::uint16_t>(buf[offset + 1U]);
        return static_cast<std::int16_t>(u);
    }
    return 0;
}

inline void write_i16_be(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        auto u = static_cast<std::uint16_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

} // namespace

TileBoxSampleResult sample_bounding_tile_box(
    std::int16_t center_x,
    std::int16_t center_y,
    std::int16_t radius,
    const PointLayerTilemapDescriptor& layer) noexcept {
    TileBoxSampleResult res{0U, 0xFFU};
    if (layer.base == nullptr || layer.base_size == 0U) {
        return res;
    }

    auto start_col = static_cast<std::int16_t>((center_x - radius) >> 3);
    auto end_col = static_cast<std::int16_t>((center_x + radius) >> 3);
    auto start_row = static_cast<std::int16_t>((center_y - radius) >> 3);
    auto end_row = static_cast<std::int16_t>((center_y + radius) >> 3);

    for (auto r = start_row; r <= end_row; ++r) {
        auto row_offset = static_cast<std::int32_t>(r << layer.row_shift);
        for (auto c = start_col; c <= end_col; ++c) {
            auto offset = row_offset + c;
            if (offset >= 0 && static_cast<std::size_t>(offset) < layer.base_size) {
                auto b = layer.base[offset];
                res.or_accum |= b;
                res.and_accum &= b;
            }
        }
    }
    return res;
}

TileBoxSampleResult sample_entity_bounding_tile_box_dispatch(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2) noexcept {
    const auto& layer = ((layer_select_ff16f0 & 0x80U) != 0U) ? layer2 : layer1;
    auto x = read_i16_be(slot, BoundingBoxPushableLatchConfig::kXPosOffset);
    auto y = read_i16_be(slot, BoundingBoxPushableLatchConfig::kYPosOffset);
    auto rad = read_i16_be(slot, BoundingBoxPushableLatchConfig::kBoundingRadiusOffset);
    return sample_bounding_tile_box(x, y, rad, layer);
}

std::uint8_t read_point_tile_layer(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2) noexcept {
    const auto& layer = ((layer_select_ff16f0 & 0x80U) != 0U) ? layer2 : layer1;
    if (layer.base == nullptr || layer.base_size == 0U) {
        return 0U;
    }
    auto x = read_i16_be(slot, BoundingBoxPushableLatchConfig::kXPosOffset);
    auto y = read_i16_be(slot, BoundingBoxPushableLatchConfig::kYPosOffset);
    auto col = static_cast<std::int16_t>(x >> 3);
    auto row = static_cast<std::int16_t>(y >> 3);
    auto offset = static_cast<std::int32_t>((row << layer.row_shift) + col);
    if (offset >= 0 && static_cast<std::size_t>(offset) < layer.base_size) {
        return layer.base[offset];
    }
    return 0U;
}

bool scan_pushable_block_solid_collision(
    std::int16_t entity_x,
    std::int16_t entity_y,
    std::int16_t entity_rad_x,
    std::int16_t entity_rad_y,
    std::span<const PushableBlockDesc> block_pool) noexcept {
    auto count = std::min(block_pool.size(), BoundingBoxPushableLatchConfig::kPushableBlockSlotCount);

    for (std::size_t i = 0; i < count; ++i) {
        const auto& block = block_pool[i];
        if (block.type != BoundingBoxPushableLatchConfig::kPushableBlockType) {
            continue;
        }

        auto block_min_x = static_cast<std::int16_t>(block.x - block.rad_x);
        auto block_max_x = static_cast<std::int16_t>(block.x + block.rad_x);
        auto ent_min_x = static_cast<std::int16_t>(entity_x - entity_rad_x);
        auto ent_max_x = static_cast<std::int16_t>(entity_x + entity_rad_x);

        if (ent_min_x > block_max_x || ent_max_x < block_min_x) {
            continue;
        }

        auto block_min_y = static_cast<std::int16_t>(block.y - block.rad_y);
        auto block_max_y = static_cast<std::int16_t>(block.y + block.rad_y);
        auto ent_min_y = static_cast<std::int16_t>(entity_y - entity_rad_y);
        auto ent_max_y = static_cast<std::int16_t>(entity_y + entity_rad_y);

        if (ent_min_y > block_max_y || ent_max_y < block_min_y) {
            continue;
        }

        return true;
    }

    return false;
}

bool scan_pushable_block_solid_collision_raw(
    std::span<const std::uint8_t> entity_slot,
    std::span<const std::uint8_t> block_pool_bytes) noexcept {
    auto ent_x = read_i16_be(entity_slot, BoundingBoxPushableLatchConfig::kXPosOffset);
    auto ent_y = read_i16_be(entity_slot, BoundingBoxPushableLatchConfig::kYPosOffset);
    auto ent_rad_x = read_i16_be(entity_slot, BoundingBoxPushableLatchConfig::kRadXOffset);
    auto ent_rad_y = read_i16_be(entity_slot, BoundingBoxPushableLatchConfig::kRadYOffset);

    for (std::size_t i = 0; i < BoundingBoxPushableLatchConfig::kPushableBlockSlotCount; ++i) {
        auto offset = i * BoundingBoxPushableLatchConfig::kPushableBlockSlotStride;
        if (offset + BoundingBoxPushableLatchConfig::kPushableBlockSlotStride > block_pool_bytes.size()) {
            break;
        }

        auto slot_span = block_pool_bytes.subspan(offset, BoundingBoxPushableLatchConfig::kPushableBlockSlotStride);
        auto block_type = static_cast<std::uint16_t>(read_i16_be(slot_span, BoundingBoxPushableLatchConfig::kTypeOffset));
        if (block_type != BoundingBoxPushableLatchConfig::kPushableBlockType) {
            continue;
        }

        auto bx = read_i16_be(slot_span, BoundingBoxPushableLatchConfig::kXPosOffset);
        auto by = read_i16_be(slot_span, BoundingBoxPushableLatchConfig::kYPosOffset);
        auto brad_x = read_i16_be(slot_span, BoundingBoxPushableLatchConfig::kRadXOffset);
        auto brad_y = read_i16_be(slot_span, BoundingBoxPushableLatchConfig::kRadYOffset);

        auto block_min_x = static_cast<std::int16_t>(bx - brad_x);
        auto block_max_x = static_cast<std::int16_t>(bx + brad_x);
        auto ent_min_x = static_cast<std::int16_t>(ent_x - ent_rad_x);
        auto ent_max_x = static_cast<std::int16_t>(ent_x + ent_rad_x);

        if (ent_min_x > block_max_x || ent_max_x < block_min_x) {
            continue;
        }

        auto block_min_y = static_cast<std::int16_t>(by - brad_y);
        auto block_max_y = static_cast<std::int16_t>(by + brad_y);
        auto ent_min_y = static_cast<std::int16_t>(ent_y - ent_rad_y);
        auto ent_max_y = static_cast<std::int16_t>(ent_y + ent_rad_y);

        if (ent_min_y > block_max_y || ent_max_y < block_min_y) {
            continue;
        }

        return true;
    }

    return false;
}

void latch_entity_ground_surface(
    std::span<std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2,
    std::span<const std::int16_t> elevation_table) noexcept {
    if (slot.size() < 0x72U) {
        return;
    }

    auto res = sample_entity_bounding_tile_box_dispatch(slot, layer_select_ff16f0, layer1, layer2);

    slot[BoundingBoxPushableLatchConfig::kOrAccumOffset] = res.or_accum;
    slot[BoundingBoxPushableLatchConfig::kAndAccumOffset] = res.and_accum;

    auto attr = static_cast<std::size_t>(res.and_accum & 0x0FU);
    slot[BoundingBoxPushableLatchConfig::kSurfaceTypeOffset] =
        BoundingBoxPushableLatchConfig::kTerrainSurfaceLut[attr];

    auto elev = probe_entity_elevation(slot, layer_select_ff16f0, layer1, layer2, elevation_table);
    write_i16_be(slot, BoundingBoxPushableLatchConfig::kGroundHeightOffset, elev);
}

} // namespace oasis::game::player
