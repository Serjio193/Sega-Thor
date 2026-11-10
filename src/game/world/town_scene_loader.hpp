#pragma once

#include "game/entity/native_entity.hpp"
#include "game/world/screen_descriptor.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::world {

inline constexpr std::uint16_t kBeachMapId = 0x003DU;
inline constexpr std::uint16_t kTownMapId = 0x005AU;
inline constexpr std::int32_t kBeachNorthExitThresholdY = 77;

struct TownStreamChunk {
    std::uint32_t rom_start{};
    std::size_t compressed_size{};
    std::size_t decoded_size{};
    std::string decoded_sha256{};
    std::vector<std::uint8_t> decoded_bytes{};
};

struct TownTransitionState {
    std::uint16_t map_id{kTownMapId};
    std::int32_t player_x{160};
    std::int32_t player_y{488};
    int initial_cam_x{0};
    int initial_cam_y{376};
    int settled_cam_y{288};
    int max_camera_x{0};
    int max_camera_y{288};
    int initial_vscroll_a{136};
};

struct LoadedTownScene {
    ScreenDescriptorRaw descriptor{};
    std::array<TownStreamChunk, 7> stream_chunks{};
    std::vector<NativeEntity> entities{};
    TownTransitionState transition{};
};

// Returns true when Beach (MAP_ID=0x003D) player world position triggers the north exit to Town.
[[nodiscard]] constexpr bool is_beach_to_town_exit_triggered(std::int32_t /*player_x*/,
                                                             std::int32_t player_y) noexcept {
    return player_y <= kBeachNorthExitThresholdY;
}

// Decodes all 7 Town Screen(1,3) 0x003820 compressed streams and entity spawn list directly
// from authentic ROM bytes via oasis::game::decompress_graphics().
[[nodiscard]] LoadedTownScene load_town_scene_from_rom(std::span<const std::uint8_t> rom);

// Uploads decoded Town 4bpp pattern chunks cleanly into VDP VRAM without prerecorded snapshots.
void upload_town_tiles_to_vdp(const LoadedTownScene& scene, genesis::VdpState& vdp);

} // namespace oasis::game::world
