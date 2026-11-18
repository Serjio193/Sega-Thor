#include "game/world/spirit_warp_bgm.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::world;

namespace {

void write_be16_vec(std::vector<std::uint8_t>& buf, std::size_t offset, std::uint16_t val) {
    if (offset + 1U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

void write_be32_vec(std::vector<std::uint8_t>& buf, std::size_t offset, std::uint32_t val) {
    if (offset + 3U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

} // namespace

static void test_rom_constants_and_layout() {
    assert(SpiritWarpBgmConfig::kEntryRepositionSpiritAddr == 0x00CC12U);
    assert(SpiritWarpBgmConfig::kEntryLoadCollisionTableAddr == 0x00CCA4U);
    assert(SpiritWarpBgmConfig::kEntryInitRoomEnvSyncBgmAddr == 0x00CCE6U);
    assert(SpiritWarpBgmConfig::kEntrySyncBgmTrackAddr == 0x00CD2CU);
    assert(SpiritWarpBgmConfig::kEntryLatchAllGroundSurfacesAddr == 0x00CD4AU);
    assert(SpiritWarpBgmConfig::kRangeEndAddr == 0x00CD64U);
    assert(SpiritWarpBgmConfig::kRoutineSizeBytes == 338U);

    assert(SpiritWarpBgmConfig::kCollisionAreaTableAddr == 0x04371EU);
    assert(SpiritWarpBgmConfig::kCollisionAreaCount == 32U);
    assert(SpiritWarpBgmConfig::kCollisionTickCadence == 10U);

    assert(SpiritWarpBgmConfig::kSpiritSlotAddr == 0x00FF1AA4U);
    assert(SpiritWarpBgmConfig::kPlayerAliSlotAddr == 0x00FF19E8U);
    assert(SpiritWarpBgmConfig::kCollisionTablePtrAddr == 0x00FF17B2U);
    assert(SpiritWarpBgmConfig::kCollisionTickCounterAddr == 0x00FF17B6U);
    assert(SpiritWarpBgmConfig::kCollisionTickCadenceAddr == 0x00FF17B7U);
    assert(SpiritWarpBgmConfig::kRoomSpiritForbidAddr == 0x00FF16F1U);
    assert(SpiritWarpBgmConfig::kTargetTrackAddr == 0x00FF16F4U);
    assert(SpiritWarpBgmConfig::kCurrentTrackAddr == 0x00FF16F2U);
    assert(SpiritWarpBgmConfig::kSoundDriverPort == 0x00060004U);

    assert(SpiritWarpBgmConfig::kSpiritOffsetY == -8);
    assert(SpiritWarpBgmConfig::kBgmCmdContinue == 3U);
    assert(SpiritWarpBgmConfig::kTypeUnsummonBow == 0x0016U);
    assert(SpiritWarpBgmConfig::kTypeResetState == 0x001AU);
    assert(SpiritWarpBgmConfig::kTypeDyttoEfreet == 0x001CU);
    assert(SpiritWarpBgmConfig::kTypeShade == 0x001EU);

    assert(SpiritWarpBgmConfig::kCollisionAreaTable[0] == 0x04379EU);
    assert(SpiritWarpBgmConfig::kCollisionAreaTable[1] == 0x046332U);
    assert(SpiritWarpBgmConfig::kCollisionAreaTable[14] == 0x05016AU);
    assert(SpiritWarpBgmConfig::kCollisionAreaTable[31] == 0x0437DCU);

    std::cout << "[PASS] test_rom_constants_and_layout\n";
}

static void test_spirit_repositioning_unsummon_bow() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = SpiritWarpBgmConfig::kTypeUnsummonBow; // 0x0016
    spirit.mode_byte = 0x55U;

    reposition_spirit_on_room_warp(spirit, 100, 200, 10, false);

    assert(!spirit.active);
    assert(spirit.type == 0U);
    assert(spirit.mode_byte == 0U);

    std::cout << "[PASS] test_spirit_repositioning_unsummon_bow\n";
}

static void test_spirit_repositioning_forbid_room() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = SpiritWarpBgmConfig::kTypeResetState; // 0x001A
    spirit.mode_byte = 0xAAU;

    reposition_spirit_on_room_warp(spirit, 100, 200, 10, true);

    assert(!spirit.active);
    assert(spirit.type == 0U);
    assert(spirit.mode_byte == 0U);

    std::cout << "[PASS] test_spirit_repositioning_forbid_room\n";
}

static void test_spirit_repositioning_dytto_efreet() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = SpiritWarpBgmConfig::kTypeDyttoEfreet; // 0x001C
    spirit.mode_byte = 0x7FU;
    spirit.state_synced = false;

    reposition_spirit_on_room_warp(spirit, 100, 200, 10, false);

    assert(spirit.state_synced);
    assert(spirit.mode_byte == 0U);
    assert(spirit.type == SpiritWarpBgmConfig::kTypeDyttoEfreet);

    std::cout << "[PASS] test_spirit_repositioning_dytto_efreet\n";
}

static void test_spirit_repositioning_shade() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = SpiritWarpBgmConfig::kTypeShade; // 0x001E
    spirit.mode_byte = 0x33U;

    reposition_spirit_on_room_warp(spirit, 150, 250, 42, false);

    assert(spirit.projected);
    assert(spirit.mode_byte == 0U);
    assert(spirit.x() == 150);
    assert(spirit.y() == 250);
    assert(spirit.elevation == 42);

    std::cout << "[PASS] test_spirit_repositioning_shade\n";
}

static void test_spirit_repositioning_reset_state_type() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = SpiritWarpBgmConfig::kTypeResetState; // 0x001A
    spirit.mode = 4U;
    spirit.facing = 2U;
    spirit.counter_9e = 9U;
    spirit.counter_a2 = 7U;
    spirit.subpixel_vel = 0x0200U;
    spirit.mode_byte = 0x11U;

    reposition_spirit_on_room_warp(spirit, 120, 300, 0, false);

    assert(spirit.x() == 120);
    assert(spirit.y() == 292); // 300 - 8
    assert(spirit.mode == 0U);
    assert(spirit.facing == 0U);
    assert(spirit.counter_9e == 0U);
    assert(spirit.counter_a2 == 0U);
    assert(spirit.subpixel_vel == 0U);
    assert(spirit.mode_byte == 0U);

    std::cout << "[PASS] test_spirit_repositioning_reset_state_type\n";
}

static void test_spirit_repositioning_generic_type() {
    SpiritState spirit;
    spirit.active = true;
    spirit.type = 0x0042U;
    spirit.mode = 5U;
    spirit.facing = 3U;
    spirit.mode_byte = 0xFFU;

    reposition_spirit_on_room_warp(spirit, 80, 160, 0, false);

    assert(spirit.x() == 80);
    assert(spirit.y() == 152); // 160 - 8
    assert(spirit.mode == 5U);
    assert(spirit.facing == 3U);
    assert(spirit.mode_byte == 0U);

    std::cout << "[PASS] test_spirit_repositioning_generic_type\n";
}

static void test_collision_table_resolution() {
    std::vector<std::uint8_t> mock_rom(0x55000, 0x00);

    constexpr std::uint32_t kAreaBase = SpiritWarpBgmConfig::kCollisionAreaTable[0]; // 0x04379E
    constexpr std::uint8_t kSceneId = 3U;
    constexpr std::size_t kSceneEntry = kAreaBase + kSceneId * 2U;
    constexpr std::int16_t kOffset = 0x0034;

    write_be16_vec(mock_rom, kSceneEntry, static_cast<std::uint16_t>(kOffset));

    const std::uint16_t packed_id = (0U << 8) | kSceneId;
    const std::uint32_t resolved = resolve_collision_table_pointer(mock_rom, packed_id);

    assert(resolved == static_cast<std::uint32_t>(kSceneEntry + kOffset));

    const auto table_res = load_room_collision_table(mock_rom, packed_id);
    assert(table_res.collision_ptr == resolved);
    assert(table_res.cadence_timer == 10U);
    assert(table_res.busy_flag_set);

    std::cout << "[PASS] test_collision_table_resolution\n";
}

static void test_bgm_synchronization() {
    // Current == Target: sends command 3 (continue)
    const auto continue_res = synchronize_bgm_track(5U, 5U);
    assert(continue_res.sound_cmd == 3U);
    assert(continue_res.current_bgm == 5U);
    assert(!continue_res.track_changed);

    // Current != Target: sends target track and updates current
    const auto change_res = synchronize_bgm_track(5U, 14U);
    assert(change_res.sound_cmd == 14U);
    assert(change_res.current_bgm == 14U);
    assert(change_res.track_changed);

    std::cout << "[PASS] test_bgm_synchronization\n";
}

static void test_group2_entity_ground_latch_sweep() {
    std::vector<std::uint8_t> mock_ram(0x10000, 0x00);

    constexpr std::uint32_t kBase = SpiritWarpBgmConfig::kPlayerAliSlotAddr; // 0xFF19E8
    constexpr std::size_t kStride = SpiritWarpBgmConfig::kEntitySlotStride;  // 188 bytes

    // Slot 0 (Ali) active
    write_be16_vec(mock_ram, kBase & 0xFFFFU, 1U);
    // Slot 2 active
    write_be16_vec(mock_ram, (kBase + 2 * kStride) & 0xFFFFU, 5U);
    // Slot 5 active
    write_be16_vec(mock_ram, (kBase + 5 * kStride) & 0xFFFFU, 12U);
    // Slot 7 inactive (0)
    write_be16_vec(mock_ram, (kBase + 7 * kStride) & 0xFFFFU, 0U);
    // Slot 9 dead/negative (-1)
    write_be16_vec(mock_ram, (kBase + 9 * kStride) & 0xFFFFU, 0xFFFFU);

    const std::size_t count = count_active_group2_entities(mock_ram);
    assert(count == 3U);

    std::cout << "[PASS] test_group2_entity_ground_latch_sweep\n";
}

static void test_raw_memory_window_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00);
    std::vector<std::uint8_t> rom(0x55000, 0x00);

    // Setup Ali at $FF19E8
    constexpr std::uint32_t kAli = SpiritWarpBgmConfig::kPlayerAliSlotAddr & 0xFFFFU;
    write_be32_vec(ram, kAli + 0x08U, 100U << 16);
    write_be32_vec(ram, kAli + 0x0CU, 200U << 16);
    write_be16_vec(ram, kAli + 0x14U, 15U);

    // Setup Spirit at $FF1AA4
    constexpr std::uint32_t kSpirit = SpiritWarpBgmConfig::kSpiritSlotAddr & 0xFFFFU;
    write_be16_vec(ram, kSpirit + 0x00U, SpiritWarpBgmConfig::kTypeResetState); // 0x001A
    write_be16_vec(ram, kSpirit + 0x04U, 2U);
    ram[kSpirit + 0x39U] = 0xAAU;

    // Reposition spirit in RAM
    reposition_spirit_raw(ram, false);

    assert((static_cast<std::int32_t>(
               (static_cast<std::uint32_t>(ram[kSpirit + 0x08U]) << 24) |
               (static_cast<std::uint32_t>(ram[kSpirit + 0x09U]) << 16)) >> 16) == 100);
    assert((static_cast<std::int32_t>(
               (static_cast<std::uint32_t>(ram[kSpirit + 0x0CU]) << 24) |
               (static_cast<std::uint32_t>(ram[kSpirit + 0x0DU]) << 16)) >> 16) == 192); // 200 - 8
    assert(ram[kSpirit + 0x39U] == 0U);

    // Setup BGM in RAM
    write_be16_vec(ram, SpiritWarpBgmConfig::kCurrentTrackAddr & 0xFFFFU, 3U);
    write_be16_vec(ram, SpiritWarpBgmConfig::kTargetTrackAddr & 0xFFFFU, 8U);

    const auto bgm_res = init_room_environment_and_sync_bgm_raw(ram, rom, 0x0001U);
    assert(bgm_res.sound_cmd == 8U);
    assert(bgm_res.current_bgm == 8U);
    assert(bgm_res.track_changed);

    assert((ram[SpiritWarpBgmConfig::kVdpBusyFlagAddr & 0xFFFFU] & 0x20U) != 0U);
    assert(ram[SpiritWarpBgmConfig::kCollisionTickCadenceAddr & 0xFFFFU] == 10U);
    assert(ram[SpiritWarpBgmConfig::kStaleEnemyHudTrackerAddr & 0xFFFFU] == 0U);

    std::cout << "[PASS] test_raw_memory_window_integration\n";
}

static void test_cleanroom_namespace_aliases() {
    thor::environment::SpiritState spirit;
    spirit.active = true;
    spirit.type = thor::environment::SpiritWarpBgmConfig::kTypeUnsummonBow;
    thor::environment::reposition_spirit_on_room_warp(spirit, 50, 60, 0, false);
    assert(!spirit.active);

    const auto bgm = thor::world::synchronize_bgm_track(4U, 4U);
    assert(bgm.sound_cmd == 3U);

    std::cout << "[PASS] test_cleanroom_namespace_aliases\n";
}

int main() {
    std::cout << "Starting M16.182 Spirit Warp & BGM Synchronizer tests...\n";
    test_rom_constants_and_layout();
    test_spirit_repositioning_unsummon_bow();
    test_spirit_repositioning_forbid_room();
    test_spirit_repositioning_dytto_efreet();
    test_spirit_repositioning_shade();
    test_spirit_repositioning_reset_state_type();
    test_spirit_repositioning_generic_type();
    test_collision_table_resolution();
    test_bgm_synchronization();
    test_group2_entity_ground_latch_sweep();
    test_raw_memory_window_integration();
    test_cleanroom_namespace_aliases();
    std::cout << "All M16.182 Spirit Warp & BGM Synchronizer tests PASS!\n";
    return 0;
}
