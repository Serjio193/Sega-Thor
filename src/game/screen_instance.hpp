#pragma once

#include "game/entities/screen_entity.hpp"
#include "game/genesis_graphics.hpp"
#include "game/world/byte_grid.hpp"
#include "game/world/screen_descriptor.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game {

struct ScreenInstance {
    world::ScreenId screen_id{};
    std::uint16_t setup_id{};
    std::uint32_t descriptor_address{};
    std::uint32_t primary_stream_pointer{};
    std::array<std::uint8_t, 4> gfx_resource_ids{};

    std::vector<std::uint8_t> primary_bytes{};
    std::vector<Tile8x8> tiles{};
    std::array<Palette16, 4> palettes{};
    std::vector<std::uint16_t> tilemap{};
    std::vector<entities::ScreenEntity> entities{};

    world::ByteGridView terrain{};

    static ScreenInstance load_from_rom(
        std::span<const std::uint8_t> rom,
        world::ScreenId id);

    void rebind_views() noexcept;
};

} // namespace oasis::game
