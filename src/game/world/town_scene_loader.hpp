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
inline constexpr std::uint16_t kTownNorthMapId = 0x005BU; // Screen(1,4)
inline constexpr std::int32_t kBeachNorthExitThresholdY = 76; // ROM 0x02E784: cmpi.w #$4c, d0; bls
inline constexpr std::int32_t kTownNorthExitThresholdY = 108; // ROM 0x02E920: cmpi.w #$6c, d0; bls
inline constexpr std::int32_t kTownSouthExitThresholdY = 500; // ROM 0x02E926: cmpi.w #$1f4, d0; bcc

struct ExitLockFlags {
    std::uint8_t lock_ff17b9{0};
    std::uint8_t lock_ff1987{0};
    std::uint8_t lock_ff1985{0};
    std::uint8_t flags_ff17b8{0};
    std::uint8_t lock_ff1983{0};
    std::uint8_t lock_ff1982{0};

    [[nodiscard]] constexpr bool any_locked() const noexcept {
        return lock_ff17b9 != 0U || lock_ff1987 != 0U || lock_ff1985 != 0U ||
               (flags_ff17b8 & 0x04U) != 0U || lock_ff1983 != 0U || lock_ff1982 != 0U;
    }
};

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
    std::uint32_t music_stop_cmd{0x00000002U}; // ROM 0x00CA90..0x00CA92
};

struct LoadedTownScene {
    ScreenDescriptorRaw descriptor{};
    std::array<TownStreamChunk, 7> stream_chunks{};
    std::vector<NativeEntity> entities{}; // Slots 4..18 (Archway + 14 0x002C guards)
    std::vector<std::uint16_t> plane_a_metatiles{}; // 64x64 from 0x213850+4
    std::vector<std::uint8_t> terrain_collision{};  // 64x64 from 0x213850+8196
    std::array<std::uint16_t, 32> scene_palette_words{}; // 32 words from 0x213850+12292 ($FF179E)
    TownTransitionState transition{};
};

// Returns true when Beach (Screen(1,2)) player world position triggers the north exit to Town.
[[nodiscard]] constexpr bool is_beach_to_town_exit_triggered(
    std::int32_t /*player_x*/, std::int32_t player_y,
    const ExitLockFlags& locks = {}) noexcept {
    return !locks.any_locked() && player_y <= kBeachNorthExitThresholdY;
}

// Returns true when Town (Screen(1,3)) player world position triggers north exit to Screen(1,4).
[[nodiscard]] constexpr bool is_town_to_north_exit_triggered(
    std::int32_t player_y, const ExitLockFlags& locks = {}) noexcept {
    return !locks.any_locked() && player_y <= kTownNorthExitThresholdY;
}

// Decodes all 7 Town Screen(1,3) 0x003820 compressed streams, 64x64 layout/collision/palette,
// and all 15 entity spawns directly from authentic ROM bytes.
[[nodiscard]] LoadedTownScene load_town_scene_from_rom(std::span<const std::uint8_t> rom);

// Uploads decoded Town 4bpp pattern chunks cleanly into VDP VRAM and CRAM without prerecorded snapshots.
void upload_town_tiles_to_vdp(const LoadedTownScene& scene, genesis::VdpState& vdp);

} // namespace oasis::game::world