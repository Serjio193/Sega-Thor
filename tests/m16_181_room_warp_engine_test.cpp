#include "game/world/room_warp_engine.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::world;

static void test_constants_and_addresses() {
    static_assert(RoomWarpEngineConfig::kEntryCheckCutsceneLockAddr == 0x00C9B2U);
    static_assert(RoomWarpEngineConfig::kEntryWarpImmediateAddr == 0x00C9ECU);
    static_assert(RoomWarpEngineConfig::kEntryWarpFadeAddr == 0x00CA10U);
    static_assert(RoomWarpEngineConfig::kEntryRoomTransitionExecAddr == 0x00CA3EU);
    static_assert(RoomWarpEngineConfig::kEntrySpawnPlayerAliAddr == 0x00CB64U);
    static_assert(RoomWarpEngineConfig::kRangeEndAddr == 0x00CC12U);
    static_assert(RoomWarpEngineConfig::kRoutineSizeBytes == 608U);

    static_assert(RoomWarpEngineConfig::kCameraCenterOffsetX == 160);
    static_assert(RoomWarpEngineConfig::kCameraCenterOffsetY == 112);

    static_assert(RoomWarpEngineConfig::kRoomOverrideSource == 0x0004U);
    static_assert(RoomWarpEngineConfig::kRoomOverrideTarget == 0x0611U);

    static_assert(RoomWarpEngineConfig::kAliIdleFrame == 0x003FU);
    static_assert(RoomWarpEngineConfig::kAliTangibleLayer == 0x10U);
    static_assert(RoomWarpEngineConfig::kAliCacheInvalidTag == 0xFFFFU);
    static_assert(RoomWarpEngineConfig::kAliSkyDropElevation == 240);
    static_assert(RoomWarpEngineConfig::kAliSkyDropMode == 6U);
    static_assert(RoomWarpEngineConfig::kAliSkyDropFacing == 6U);

    static_assert(RoomWarpEngineConfig::kPlayerAliSlotRam == 0x00FF19E8U);
    static_assert(RoomWarpEngineConfig::kCutsceneActiveRam == 0x00FF17B9U);
    static_assert(RoomWarpEngineConfig::kTransitionBusyRam == 0x00FF1987U);
    static_assert(RoomWarpEngineConfig::kPauseMenuRam == 0x00FF1985U);
    static_assert(RoomWarpEngineConfig::kScriptPauseFlagsRam == 0x00FF17B8U);
    static_assert(RoomWarpEngineConfig::kSecondaryLockRam == 0x00FF1983U);
    static_assert(RoomWarpEngineConfig::kTertiaryLockRam == 0x00FF1982U);
    static_assert(RoomWarpEngineConfig::kWarpTransitionModeRam == 0x00FF19FEU);
    static_assert(RoomWarpEngineConfig::kImmediateWarpFlagRam == 0x00FF17BEU);
    static_assert(RoomWarpEngineConfig::kFadeFlagRam == 0x00FF17BCU);
    static_assert(RoomWarpEngineConfig::kBlackoutFlagRam == 0x00FF17BBU);
    static_assert(RoomWarpEngineConfig::kRoomLoadedFlagRam == 0x00FF17BAU);
    static_assert(RoomWarpEngineConfig::kCameraSpawnCoordsRam == 0x00FF178EU);
    static_assert(RoomWarpEngineConfig::kPlayerSpawnCacheRam == 0x00FF1972U);
    static_assert(RoomWarpEngineConfig::kSpawnLockFlagRam == 0x00FF1984U);

    std::cout << "[PASS] test_constants_and_addresses\n";
}

static void test_camera_centering_math() {
    // Normal screen coordinate
    const CameraCoords c1 = calculate_camera_center(200, 150);
    assert(c1.cam_x == 40);
    assert(c1.cam_y == 38);
    assert(c1.spawn_x == 200);
    assert(c1.spawn_y == 150);

    // Exact center
    const CameraCoords c2 = calculate_camera_center(160, 112);
    assert(c2.cam_x == 0);
    assert(c2.cam_y == 0);
    assert(c2.spawn_x == 160);
    assert(c2.spawn_y == 112);

    // Near top-left (negative camera coordinates)
    const CameraCoords c3 = calculate_camera_center(50, 40);
    assert(c3.cam_x == -110);
    assert(c3.cam_y == -72);

    std::cout << "[PASS] test_camera_centering_math\n";
}

static void test_cutscene_script_locks() {
    CutsceneLockFlags flags;
    assert(!check_cutscene_script_lock(flags));

    flags.cutscene_active = true;
    assert(check_cutscene_script_lock(flags));
    flags.cutscene_active = false;

    flags.transition_busy = true;
    assert(check_cutscene_script_lock(flags));
    flags.transition_busy = false;

    flags.pause_menu = true;
    assert(check_cutscene_script_lock(flags));
    flags.pause_menu = false;

    flags.cutscene_lock_bit = true;
    assert(check_cutscene_script_lock(flags));
    flags.cutscene_lock_bit = false;

    flags.secondary_lock = true;
    assert(check_cutscene_script_lock(flags));
    flags.secondary_lock = false;

    flags.tertiary_lock = true;
    assert(check_cutscene_script_lock(flags));
    flags.tertiary_lock = false;

    assert(!check_cutscene_script_lock(flags));

    std::cout << "[PASS] test_cutscene_script_locks\n";
}

static void test_room_id_overrides() {
    assert(apply_room_id_overrides(0x0004U) == 0x0611U);
    assert(apply_room_id_overrides(0x0000U) == 0x0000U);
    assert(apply_room_id_overrides(0x0100U) == 0x0100U);
    assert(apply_room_id_overrides(0x0611U) == 0x0611U);

    std::cout << "[PASS] test_room_id_overrides\n";
}

static void test_spawn_player_ali() {
    const CameraCoords coords = calculate_camera_center(300, 250);

    // Normal spawn
    const PlayerSpawnState s_norm = spawn_player_ali(coords, false);
    assert(s_norm.x == 300);
    assert(s_norm.y == 250);
    assert(s_norm.elevation == 0);
    assert(s_norm.mode == 0U);
    assert(s_norm.facing == 0U);
    assert(s_norm.idle_frame == 0x003FU);
    assert(s_norm.visible);
    assert(s_norm.layer == 0x10U);
    assert(s_norm.falling_anim_counter == 0U);

    // Sky drop spawn (fade transition active)
    const PlayerSpawnState s_fade = spawn_player_ali(coords, true);
    assert(s_fade.x == 300);
    assert(s_fade.y == 250);
    assert(s_fade.elevation == 240);
    assert(s_fade.mode == 6U);
    assert(s_fade.facing == 6U);
    assert(s_fade.idle_frame == 0x003FU);
    assert(s_fade.visible);
    assert(s_fade.layer == 0x10U);
    assert(s_fade.falling_anim_counter == 1U);

    std::cout << "[PASS] test_spawn_player_ali\n";
}

static void test_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U);

    auto read_be16 = [&](std::uint32_t addr) -> std::uint16_t {
        const std::size_t off = addr & 0xFFFFU;
        return static_cast<std::uint16_t>((ram[off] << 8) | ram[off + 1U]);
    };

    auto read_be32 = [&](std::uint32_t addr) -> std::uint32_t {
        const std::size_t off = addr & 0xFFFFU;
        return (static_cast<std::uint32_t>(ram[off]) << 24) |
               (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
               (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
               static_cast<std::uint32_t>(ram[off + 3U]);
    };

    // Test raw cutscene lock checks
    assert(!check_cutscene_script_lock_raw(ram));

    ram[RoomWarpEngineConfig::kScriptPauseFlagsRam & 0xFFFFU] = 0x04U; // bit 2 set
    assert(check_cutscene_script_lock_raw(ram));
    ram[RoomWarpEngineConfig::kScriptPauseFlagsRam & 0xFFFFU] = 0x00U;

    ram[RoomWarpEngineConfig::kCutsceneActiveRam & 0xFFFFU] = 1U;
    assert(check_cutscene_script_lock_raw(ram));
    ram[RoomWarpEngineConfig::kCutsceneActiveRam & 0xFFFFU] = 0U;

    // Test immediate warp raw
    const CameraCoords c_imm = warp_room_entry_immediate_raw(ram, 0x0100U, 320, 224, 1U);
    assert(c_imm.cam_x == 160);
    assert(c_imm.cam_y == 112);
    assert(read_be16(RoomWarpEngineConfig::kWarpTransitionModeRam) == 1U);
    assert(ram[RoomWarpEngineConfig::kImmediateWarpFlagRam & 0xFFFFU] == 0xFFU);
    // Packed camera at $FF178E: (160 << 16) | 112
    assert(read_be32(RoomWarpEngineConfig::kCameraSpawnCoordsRam) == 0x00A00070U);
    // Packed spawn at $FF178E+4: (320 << 16) | 224
    assert(read_be32(RoomWarpEngineConfig::kCameraSpawnCoordsRam + 4U) == 0x014000E0U);

    // Test fade warp raw
    const CameraCoords c_fade = warp_room_entry_fade_raw(ram, 0x0200U, 400, 300);
    assert(c_fade.cam_x == 240);
    assert(c_fade.cam_y == 188);
    assert(read_be16(RoomWarpEngineConfig::kWarpTransitionModeRam) == 2U);
    assert(ram[RoomWarpEngineConfig::kFadeFlagRam & 0xFFFFU] == 0xFFU);
    assert(ram[RoomWarpEngineConfig::kBlackoutFlagRam & 0xFFFFU] == 0xFFU);
    assert(ram[RoomWarpEngineConfig::kImmediateWarpFlagRam & 0xFFFFU] == 0x00U);

    // Test player spawn in room raw (fade active -> sky drop!)
    ram[RoomWarpEngineConfig::kSpawnLockFlagRam & 0xFFFFU] = 0x01U;
    ram[RoomWarpEngineConfig::kTertiaryLockRam & 0xFFFFU] = 0x01U;

    const PlayerSpawnState ps = spawn_player_ali_in_room_raw(ram, 400, 300);
    assert(ps.x == 400);
    assert(ps.y == 300);
    assert(ps.elevation == 240);
    assert(ps.mode == 6U);
    assert(ps.facing == 6U);
    assert(ps.falling_anim_counter == 1U);

    // Check RAM entity fields at $FF19E8
    const std::uint32_t slot = RoomWarpEngineConfig::kPlayerAliSlotRam;
    assert(read_be32(slot + 0x08U) == (static_cast<std::uint32_t>(400) << 16));
    assert(read_be32(slot + 0x0CU) == (static_cast<std::uint32_t>(300) << 16));
    assert(read_be16(slot + 0x10U) == 240U);
    assert(read_be16(slot + 0x04U) == 6U);
    assert(read_be16(slot + 0x2AU) == 6U);
    assert(read_be16(slot + 0x2CU) == 0xFFFFU);
    assert(read_be16(slot + 0x34U) == 0xFFFFU);
    assert(read_be16(slot + 0x5CU) == 0xFFFFU);
    assert(read_be16(slot + 0x32U) == 0x003FU);
    assert(ram[(slot + 0x36U) & 0xFFFFU] == 1U);
    assert(ram[(slot + 0x38U) & 0xFFFFU] == 0x10U);
    assert(read_be32(slot + 0x56U) == 1U);

    // Check spawn cache at $FF1972
    assert(read_be16(RoomWarpEngineConfig::kPlayerSpawnCacheRam) == 400U);
    assert(read_be16(RoomWarpEngineConfig::kPlayerSpawnCacheRam + 2U) == 300U);

    // Check locks cleared
    assert(ram[RoomWarpEngineConfig::kSpawnLockFlagRam & 0xFFFFU] == 0U);
    assert(ram[RoomWarpEngineConfig::kTertiaryLockRam & 0xFFFFU] == 0U);

    std::cout << "[PASS] test_ram_integration\n";
}

static void test_thor_warp_namespace_aliases() {
    const thor::warp::CameraCoords c = thor::warp::calculate_camera_center(200, 150);
    assert(c.cam_x == 40);
    assert(c.cam_y == 38);

    assert(thor::warp::apply_room_id_overrides(4) == 0x0611U);

    const thor::warp::PlayerSpawnState ps = thor::warp::spawn_player_ali(c, false);
    assert(ps.elevation == 0);
    assert(ps.mode == 0U);

    std::cout << "[PASS] test_thor_warp_namespace_aliases\n";
}

int main() {
    std::cout << "Starting M16.181 Room Warp Engine tests...\n";
    test_constants_and_addresses();
    test_camera_centering_math();
    test_cutscene_script_locks();
    test_room_id_overrides();
    test_spawn_player_ali();
    test_ram_integration();
    test_thor_warp_namespace_aliases();
    std::cout << "All M16.181 Room Warp Engine tests PASS!\n";
    return 0;
}
