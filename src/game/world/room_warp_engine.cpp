#include "room_warp_engine.hpp"

namespace oasis::game::world {

namespace {

inline std::uint16_t read_be16(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ram[off]) << 8) |
                                      static_cast<std::uint16_t>(ram[off + 1U]));
}

inline void write_be16(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline void write_be32(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint32_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
    ram[off + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint8_t read_u8(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return 0U;
    }
    return ram[off];
}

inline void write_u8(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint8_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return;
    }
    ram[off] = val;
}

} // namespace

CameraCoords calculate_camera_center(std::int16_t target_x, std::int16_t target_y) noexcept {
    CameraCoords coords;
    coords.cam_x = static_cast<std::int16_t>(target_x - RoomWarpEngineConfig::kCameraCenterOffsetX);
    coords.cam_y = static_cast<std::int16_t>(target_y - RoomWarpEngineConfig::kCameraCenterOffsetY);
    coords.spawn_x = target_x;
    coords.spawn_y = target_y;
    return coords;
}

bool check_cutscene_script_lock(const CutsceneLockFlags& flags) noexcept {
    return flags.cutscene_active ||
           flags.transition_busy ||
           flags.pause_menu ||
           flags.cutscene_lock_bit ||
           flags.secondary_lock ||
           flags.tertiary_lock;
}

std::uint16_t apply_room_id_overrides(std::uint16_t room_id) noexcept {
    if (room_id == RoomWarpEngineConfig::kRoomOverrideSource) {
        return RoomWarpEngineConfig::kRoomOverrideTarget;
    }
    return room_id;
}

PlayerSpawnState spawn_player_ali(const CameraCoords& coords, bool fade_transition) noexcept {
    PlayerSpawnState state;
    state.x = coords.spawn_x;
    state.y = coords.spawn_y;
    state.idle_frame = RoomWarpEngineConfig::kAliIdleFrame;
    state.visible = true;
    state.layer = RoomWarpEngineConfig::kAliTangibleLayer;

    if (fade_transition) {
        state.elevation = RoomWarpEngineConfig::kAliSkyDropElevation;
        state.mode = RoomWarpEngineConfig::kAliSkyDropMode;
        state.facing = RoomWarpEngineConfig::kAliSkyDropFacing;
        state.falling_anim_counter = 1U;
    } else {
        state.elevation = 0;
        state.mode = 0U;
        state.facing = 0U;
        state.falling_anim_counter = 0U;
    }

    return state;
}

bool check_cutscene_script_lock_raw(std::span<const std::uint8_t> ram) noexcept {
    if (ram.size() < 0x10000U) {
        return false;
    }

    CutsceneLockFlags flags;
    flags.cutscene_active = (read_u8(ram, RoomWarpEngineConfig::kCutsceneActiveRam) != 0U);
    flags.transition_busy = (read_u8(ram, RoomWarpEngineConfig::kTransitionBusyRam) != 0U);
    flags.pause_menu = (read_u8(ram, RoomWarpEngineConfig::kPauseMenuRam) != 0U);
    flags.cutscene_lock_bit = ((read_u8(ram, RoomWarpEngineConfig::kScriptPauseFlagsRam) & 0x04U) != 0U);
    flags.secondary_lock = (read_u8(ram, RoomWarpEngineConfig::kSecondaryLockRam) != 0U);
    flags.tertiary_lock = (read_u8(ram, RoomWarpEngineConfig::kTertiaryLockRam) != 0U);

    return check_cutscene_script_lock(flags);
}

CameraCoords warp_room_entry_immediate_raw(
    std::span<std::uint8_t> ram,
    std::uint16_t /*room_id*/,
    std::int16_t target_x,
    std::int16_t target_y,
    std::uint16_t mode) {
    if (ram.size() < 0x10000U) {
        return {};
    }

    write_be16(ram, RoomWarpEngineConfig::kWarpTransitionModeRam, mode);
    write_u8(ram, RoomWarpEngineConfig::kImmediateWarpFlagRam, 0xFFU);

    const CameraCoords coords = calculate_camera_center(target_x, target_y);

    // Save camera & spawn coords into $FF178E
    const std::uint32_t packed_camera = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(coords.cam_x)) << 16) |
                                        static_cast<std::uint16_t>(coords.cam_y);
    const std::uint32_t packed_spawn = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(coords.spawn_x)) << 16) |
                                       static_cast<std::uint16_t>(coords.spawn_y);
    write_be32(ram, RoomWarpEngineConfig::kCameraSpawnCoordsRam, packed_camera);
    write_be32(ram, RoomWarpEngineConfig::kCameraSpawnCoordsRam + 4U, packed_spawn);

    return coords;
}

CameraCoords warp_room_entry_fade_raw(
    std::span<std::uint8_t> ram,
    std::uint16_t /*room_id*/,
    std::int16_t target_x,
    std::int16_t target_y) {
    if (ram.size() < 0x10000U) {
        return {};
    }

    write_u8(ram, RoomWarpEngineConfig::kFadeFlagRam, 0xFFU);
    write_u8(ram, RoomWarpEngineConfig::kBlackoutFlagRam, 0xFFU);
    write_be16(ram, RoomWarpEngineConfig::kWarpTransitionModeRam, 2U);

    const CameraCoords coords = calculate_camera_center(target_x, target_y);

    const std::uint32_t packed_camera = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(coords.cam_x)) << 16) |
                                        static_cast<std::uint16_t>(coords.cam_y);
    const std::uint32_t packed_spawn = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(coords.spawn_x)) << 16) |
                                       static_cast<std::uint16_t>(coords.spawn_y);
    write_be32(ram, RoomWarpEngineConfig::kCameraSpawnCoordsRam, packed_camera);
    write_be32(ram, RoomWarpEngineConfig::kCameraSpawnCoordsRam + 4U, packed_spawn);

    write_u8(ram, RoomWarpEngineConfig::kImmediateWarpFlagRam, 0x00U);

    return coords;
}

PlayerSpawnState spawn_player_ali_in_room_raw(
    std::span<std::uint8_t> ram,
    std::int16_t spawn_x,
    std::int16_t spawn_y) {
    if (ram.size() < 0x10000U) {
        return {};
    }

    const std::uint32_t slot = RoomWarpEngineConfig::kPlayerAliSlotRam;

    // Fixed 16.16 coordinates: integer pixel position in high word
    write_be32(ram, slot + 0x08U, static_cast<std::uint32_t>(static_cast<std::uint16_t>(spawn_x)) << 16);
    write_be32(ram, slot + 0x0CU, static_cast<std::uint32_t>(static_cast<std::uint16_t>(spawn_y)) << 16);
    write_be32(ram, slot + 0x10U, 0U); // elevation

    // Clear velocities
    write_be32(ram, slot + 0x72U, 0U);
    write_be32(ram, slot + 0x76U, 0U);
    write_be32(ram, slot + 0x7AU, 0U);

    // Default status
    write_be16(ram, slot + 0x04U, 0U); // mode
    write_be16(ram, slot + 0x2AU, 0U); // facing
    write_be16(ram, slot + 0x2CU, RoomWarpEngineConfig::kAliCacheInvalidTag);
    write_be16(ram, slot + 0x34U, RoomWarpEngineConfig::kAliCacheInvalidTag);
    write_be16(ram, slot + 0x5CU, RoomWarpEngineConfig::kAliCacheInvalidTag);
    write_be16(ram, slot + 0x32U, RoomWarpEngineConfig::kAliIdleFrame);
    write_u8(ram, slot + 0x36U, 1U); // visible
    write_u8(ram, slot + 0x37U, 0U);
    write_u8(ram, slot + 0x38U, RoomWarpEngineConfig::kAliTangibleLayer);
    write_u8(ram, slot + 0x39U, 0U);
    write_be16(ram, slot + 0x86U, 0U);

    const bool fade_transition = (read_u8(ram, RoomWarpEngineConfig::kFadeFlagRam) != 0U ||
                                  read_u8(ram, RoomWarpEngineConfig::kBlackoutFlagRam) != 0U);

    if (fade_transition) {
        write_be32(ram, slot + 0x4EU, 0U);
        write_be32(ram, slot + 0x52U, 0U);
        write_be32(ram, slot + 0x56U, 1U);
        write_be16(ram, slot + 0x10U, static_cast<std::uint16_t>(RoomWarpEngineConfig::kAliSkyDropElevation));
        write_be16(ram, slot + 0x04U, RoomWarpEngineConfig::kAliSkyDropMode);
        write_be16(ram, slot + 0x2AU, RoomWarpEngineConfig::kAliSkyDropFacing);
    }

    // Cache spawn position at $FF1972
    write_be16(ram, RoomWarpEngineConfig::kPlayerSpawnCacheRam, static_cast<std::uint16_t>(spawn_x));
    write_be16(ram, RoomWarpEngineConfig::kPlayerSpawnCacheRam + 2U, static_cast<std::uint16_t>(spawn_y));

    // Clear spawn and tertiary locks
    write_u8(ram, RoomWarpEngineConfig::kSpawnLockFlagRam, 0U);
    write_u8(ram, RoomWarpEngineConfig::kTertiaryLockRam, 0U);

    CameraCoords coords;
    coords.spawn_x = spawn_x;
    coords.spawn_y = spawn_y;
    return spawn_player_ali(coords, fade_transition);
}

} // namespace oasis::game::world
