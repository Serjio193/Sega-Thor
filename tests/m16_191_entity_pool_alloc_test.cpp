#include "game/world/entity_pool_alloc.hpp"
#include "../include/game/world/entity_pool_alloc.hpp"

#include "core/rom.hpp"
#include "game/player/entity_constructor_lifecycle.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::world;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.191 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_layout() {
    static_assert(kDecompressPaletteBlockAddr == 0x00D9A4U);
    static_assert(kAllocProjectileEntitySlotAddr == 0x00D9BEU);
    static_assert(kAllocSparkEffectSlotAddr == 0x00D9F0U);
    static_assert(kFindFreeGeneralEntitySlot18Addr == 0x00DA1AU);
    static_assert(kFindFreeGeneralEntitySlot17Addr == 0x00DA2AU);
    static_assert(kAllocSpawnNpcEntitySlot18Addr == 0x00DA62U);
    static_assert(kAllocSpawnNpcEntitySlot17Addr == 0x00DA72U);
    static_assert(kSoundPlayMusicTrackAddr == 0x00DAC2U);
    static_assert(kSoundResumeMusicTrackAddr == 0x00DACEU);
    static_assert(kSoundPauseMusicTrackAddr == 0x00DAE6U);
    static_assert(kSoundStopMusicTrackAddr == 0x00DB00U);
    static_assert(kEntityAllocSoundDispatchEndAddr == 0x00DB1AU);

    static_assert(kDecompressAndDmaHelperAddr == 0x0037D2U);
    static_assert(kUniversalEntityConstructorAddr == 0x008D06U);
    static_assert(kSoundDriverEntryAddr == 0x00060004U);
    static_assert((kDecompressDmaStagingRamAddr >> 1U) == kDecompressDmaBusSource);
    static_assert(kProjectilePoolRamAddr + kProjectilePoolSlots * kProjectilePoolStride == kSparkPoolRamAddr);
    static_assert(kSparkPoolRamAddr + kSparkPoolSlots * kSparkPoolStride == kDecompressDmaStagingRamAddr);
    static_assert(kGeneralPool18RamAddr + kGeneralPoolStride == kGeneralPool17RamAddr);
    static_assert(thor::world::kSoundCmdPause == 7U);
    static_assert(thor::world::kSoundCmdStop == 0U);
}

void test_decompress_palette_block_and_ram_dma() {
    // Format A compressed stream producing 64 bytes (32 words) via 4 literal bytes + 60 repeats of 0x55
    // Extended repeat: 0x50 + count_lo (60 - 4 = 56 = 0x38), fill = 0x55; span = 2 + 5 + 3 = 10
    const std::array<std::uint8_t, 11> stream_64b{
        0x0AU, 0x00U,
        0x04U, 0x0EU, 0xEEU, 0x02U, 0x46U,
        0x50U, 0x38U, 0x55U,
        0x00U};

    std::array<std::uint8_t, kDecompressDmaStagingMaxBytes> staging{};
    std::vector<VramDmaTransferCommand> queue;
    std::uint8_t vint_sync = 0x20U;
    std::uint8_t seen_sync = 0U;

    const auto res = decompress_palette_block(
        stream_64b,
        0xBC00U,
        staging,
        queue,
        vint_sync,
        [&](std::uint8_t& flags) {
            seen_sync = flags;
            flags = static_cast<std::uint8_t>(flags & ~kVdpDmaSyncBit);
        });

    CHECK(res.ok);
    CHECK(res.output_bytes == 64U);
    CHECK(res.length_words == 32U);
    CHECK(res.dma_command == (VramDmaTransferCommand{kDecompressDmaBusSource, 0xBC00U, 32U}));
    CHECK(queue.size() == 1U);
    CHECK(queue[0] == res.dma_command);
    CHECK(seen_sync == 0x22U);
    CHECK(vint_sync == 0x20U);
    CHECK(staging[0] == 0x0EU);
    CHECK(staging[1] == 0xEEU);
    CHECK(staging[63] == 0x55U);

    // 64KB RAM window test + OOB queue pointer guard
    std::array<std::uint8_t, 0x10000> ram{};
    write_be32(ram, kVdpDmaQueuePtrRamOffset, 0x00FF10B4U);
    ram[kVdpVintSyncFlagsRamOffset] = 0x04U;
    const auto ram_res = decompress_palette_block_from_ram(stream_64b, 0xD800U, ram, false);
    CHECK(ram_res.ok);
    CHECK(ram_res.length_words == 32U);
    CHECK(read_be32(ram, 0x10B4U) == kDecompressDmaBusSource);
    CHECK(read_be16(ram, 0x10B8U) == 0xD800U);
    CHECK(read_be16(ram, 0x10BAU) == 32U);
    CHECK(read_be32(ram, kVdpDmaQueuePtrRamOffset) == 0x00FF10BCU);
    CHECK(ram[kVdpVintSyncFlagsRamOffset] == 0x06U);

    // Empty/invalid stream and OOB queue pointer fail cleanly
    CHECK(!decompress_palette_block_from_ram(std::span<const std::uint8_t>{}, 0xD800U, ram).ok);
    write_be32(ram, kVdpDmaQueuePtrRamOffset, 0x00FFFFF9U);
    CHECK(!decompress_palette_block_from_ram(stream_64b, 0xD800U, ram).ok);
    CHECK(read_be32(ram, kVdpDmaQueuePtrRamOffset) == 0x00FFFFF9U);
}

void test_projectile_and_spark_allocators() {
    std::array<std::uint16_t, kProjectilePoolSlots> proj_slots{1U, 2U, 0U, 4U, 0U, 6U, 7U, 8U};
    const auto p = alloc_projectile_entity_slot(proj_slots);
    CHECK(p.has_value());
    CHECK(p->slot_index == 2U);
    CHECK(p->slot_ram_addr == (kProjectilePoolRamAddr + 2U * kProjectilePoolStride));
    CHECK(p->vram_tile_index == static_cast<std::uint16_t>(0x0694U + 18U));
    // Must remain read-only (not mutating slot 2)
    CHECK(proj_slots[2] == 0U);

    proj_slots.fill(9U);
    CHECK(!alloc_projectile_entity_slot(proj_slots).has_value());

    std::array<std::uint16_t, kSparkPoolSlots> spark_slots{10U, 20U, 30U, 40U, 50U, 0U};
    const auto s = alloc_spark_effect_slot(spark_slots);
    CHECK(s.has_value());
    CHECK(s->slot_index == 5U);
    CHECK(s->slot_ram_addr == (kSparkPoolRamAddr + 5U * kSparkPoolStride));
    spark_slots[5] = 60U;
    CHECK(!alloc_spark_effect_slot(spark_slots).has_value());

    // RAM window verification for both pools across all 8 / 6 slots
    std::array<std::uint8_t, 0x10000> ram{};
    for (std::size_t i = 0U; i < kProjectilePoolSlots; ++i) {
        const auto res = alloc_projectile_entity_slot_from_ram(ram);
        CHECK(res.has_value());
        CHECK(res->slot_index == i);
        CHECK(res->vram_tile_index == static_cast<std::uint16_t>(0x0694U + i * 9U));
        write_be16(ram, kProjectilePoolRamOffset + i * kProjectilePoolStride, 0x0001U);
    }
    CHECK(!alloc_projectile_entity_slot_from_ram(ram).has_value());

    for (std::size_t i = 0U; i < kSparkPoolSlots; ++i) {
        const auto res = alloc_spark_effect_slot_from_ram(ram);
        CHECK(res.has_value());
        CHECK(res->slot_index == i);
        write_be16(ram, kSparkPoolRamOffset + i * kSparkPoolStride, 0x0002U);
    }
    CHECK(!alloc_spark_effect_slot_from_ram(ram).has_value());
}

void test_general_pool_find_and_spawn_npc() {
    std::array<std::uint16_t, kGeneralPool18Slots> slots18{1U, 2U, 0U};
    std::uint8_t hud_timer = 0x5AU;
    const std::uint32_t expected_slot2 = kGeneralPool18RamAddr + 2U * kGeneralPoolStride;

    // 18-slot finder clears $FF184F when $FF1850 == slot_ram_addr
    const auto f18 = find_free_general_entity_slot_18(slots18, expected_slot2, hud_timer);
    CHECK(f18.has_value());
    CHECK(f18->slot_index == 2U);
    CHECK(f18->slot_ram_addr == expected_slot2);
    CHECK(f18->player_rel_offset == (expected_slot2 - kPlayerEntityRamAddr));
    CHECK(f18->cleared_enemy_hud_timer);
    CHECK(hud_timer == 0U);
    CHECK(slots18[2] == 0U);

    // 17-slot finder preserves $FF184F when $FF1850 doesn't match
    std::array<std::uint16_t, kGeneralPool17Slots> slots17{1U, 0U};
    hud_timer = 0x33U;
    const auto f17 = find_free_general_entity_slot_17(slots17, kGeneralPool17RamAddr, hud_timer);
    CHECK(f17.has_value());
    CHECK(f17->slot_index == 1U);
    CHECK(!f17->cleared_enemy_hud_timer);
    CHECK(hud_timer == 0x33U);

    // Full 17-slot pool returns nullopt
    slots17.fill(7U);
    CHECK(!find_free_general_entity_slot_17(slots17, 0U, hud_timer).has_value());

    // Spawn NPC (0x00DA62 / 0x00DA72) with synthetic ROM descriptor table at 0x03F2FA
    std::vector<std::uint8_t> synth_rom(0x040000U, 0U);
    const std::size_t desc_off =
        oasis::game::player::EntityLifecycleConfig::kDescriptorTableRomAddr +
        (static_cast<std::size_t>(kSpawnNpcEntityType - 2U) * 16U);
    write_be16(synth_rom, desc_off + 0x00U, 0x0456U); // sprite_id -> +0x4A
    synth_rom[desc_off + 0x02U] = 0x14U;             // radius -> +0x42/+0x44/+0x46
    synth_rom[desc_off + 0x06U] = 0x20U;             // height -> +0x4C
    write_be32(synth_rom, desc_off + 0x0CU, 0x00012340U); // fn_ptr_1a -> +0x1A
    write_be32(synth_rom, desc_off + 0x1CU, 0x00056780U); // fn_ptr_5e -> +0x5E

    std::array<std::uint8_t, 0x10000> ram{};
    write_be16(ram, kGeneralPool18RamOffset, 0x0001U); // slot 0 busy ($FF1C1C)
    // Pre-fill slot 1 ($FF1CD8 == slot 0 of 17-pool) with garbage to confirm constructor clears +0x02..+0xBB
    std::fill_n(ram.begin() + kGeneralPool17RamOffset + 2U, kGeneralPoolStride - 2U, std::uint8_t{0xAAU});
    write_be32(ram, kEnemyHudTargetPtrRamOffset, kGeneralPool17RamAddr);
    ram[kEnemyHudTimerRamOffset] = 0x77U;

    const auto npc_res = alloc_spawn_npc_entity_slot_18_from_ram(ram, synth_rom);
    CHECK(npc_res.has_value());
    CHECK(npc_res->slot_index == 1U);
    CHECK(npc_res->slot_ram_addr == kGeneralPool17RamAddr);
    CHECK(npc_res->player_rel_offset == (kGeneralPool17RamAddr - kPlayerEntityRamAddr));
    CHECK(npc_res->cleared_enemy_hud_timer);
    CHECK(ram[kEnemyHudTimerRamOffset] == 0U);

    // Check initialized NPC fields at $FF1CD8: +0x00=0x002E, +0x02=0x003C, constructor fields
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x00U) == kSpawnNpcEntityType);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x02U) == kSpawnNpcInitialTimer);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x04U) == 0U);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x2CU) == 0xFFFFU);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x34U) == 0xFFFFU);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x5CU) == 0xFFFFU);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x4AU) == 0x0456U);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x46U) == 0x0014U);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + 0x4CU) == 0x0020U);
    CHECK(read_be32(ram, kGeneralPool17RamOffset + 0x1AU) == 0x00012340U);
    CHECK(read_be32(ram, kGeneralPool17RamOffset + 0x5EU) == 0x00056780U);

    // Next call via 17-pool spawns at $FF1CD8 + 0xBC (index 1 of 17-pool) without synth ROM
    const auto npc_17 = alloc_spawn_npc_entity_slot_17_from_ram(ram);
    CHECK(npc_17.has_value());
    CHECK(npc_17->slot_index == 1U);
    CHECK(npc_17->slot_ram_addr == (kGeneralPool17RamAddr + kGeneralPoolStride));
    CHECK(read_be16(ram, kGeneralPool17RamOffset + kGeneralPoolStride + 0x00U) == kSpawnNpcEntityType);
    CHECK(read_be16(ram, kGeneralPool17RamOffset + kGeneralPoolStride + 0x02U) == kSpawnNpcInitialTimer);

    // Structured array pool API
    std::array<std::array<std::uint8_t, kGeneralPoolStride>, kGeneralPool17Slots> pool17{};
    hud_timer = 0x10U;
    const auto sp17 = alloc_spawn_npc_entity_slot_17(pool17, kGeneralPool17RamAddr, hud_timer, synth_rom);
    CHECK(sp17.has_value());
    CHECK(sp17->slot_index == 0U);
    CHECK(sp17->cleared_enemy_hud_timer);
    CHECK(hud_timer == 0U);
    CHECK(read_be16(pool17[0], 0x00U) == kSpawnNpcEntityType);
    CHECK(read_be16(pool17[0], 0x02U) == kSpawnNpcInitialTimer);
}

void test_sound_driver_dispatchers() {
    std::vector<std::uint16_t> driver_cmds;
    const auto driver_cb = [&](std::uint16_t cmd) { driver_cmds.push_back(cmd); };

    std::int16_t active_track = -1;

    // 1. Resume when $FF16F2 < 0 -> no-op
    const auto res_skip = sound_resume_music_track(active_track, driver_cb);
    CHECK(!res_skip.driver_called);
    CHECK(driver_cmds.empty());

    // 2. Play track 0x0106 -> stores 0x0106 in $FF16F2 and calls 0x060004
    const auto res_play = sound_play_music_track(active_track, 0x0106U, driver_cb);
    CHECK(res_play.driver_called);
    CHECK(res_play.dispatched_cmd == 0x0106U);
    CHECK(active_track == 0x0106);

    // 3. Resume when $FF16F2 >= 0 -> replays 0x0106
    const auto res_resume = sound_resume_music_track(active_track, driver_cb);
    CHECK(res_resume.driver_called);
    CHECK(res_resume.dispatched_cmd == 0x0106U);

    // 4. Pause -> sets $FF16F2 = -1, dispatches cmd 7
    const auto res_pause = sound_pause_music_track(active_track, driver_cb);
    CHECK(res_pause.driver_called);
    CHECK(res_pause.dispatched_cmd == kSoundCmdPause);
    CHECK(active_track == kSoundTrackStopped);

    // 5. Stop -> sets $FF16F2 = -1, dispatches cmd 0
    active_track = 0x0102;
    const auto res_stop = sound_stop_music_track(active_track, driver_cb);
    CHECK(res_stop.driver_called);
    CHECK(res_stop.dispatched_cmd == kSoundCmdStop);
    CHECK(active_track == kSoundTrackStopped);

    CHECK((driver_cmds == std::vector<std::uint16_t>{0x0106U, 0x0106U, 7U, 0U}));

    // 64KB RAM window sound dispatchers
    std::array<std::uint8_t, 0x10000> ram{};
    driver_cmds.clear();
    CHECK(sound_play_music_track_from_ram(ram, 0x010BU, driver_cb).driver_called);
    CHECK(read_be16(ram, kActiveMusicTrackRamOffset) == 0x010BU);
    CHECK(sound_resume_music_track_from_ram(ram, driver_cb).dispatched_cmd == 0x010BU);
    CHECK(sound_pause_music_track_from_ram(ram, driver_cb).dispatched_cmd == 7U);
    CHECK(read_be16(ram, kActiveMusicTrackRamOffset) == 0xFFFFU);
    CHECK(!sound_resume_music_track_from_ram(ram, driver_cb).driver_called);
    CHECK(sound_stop_music_track_from_ram(ram, driver_cb).dispatched_cmd == 0U);
    CHECK(read_be16(ram, kActiveMusicTrackRamOffset) == 0xFFFFU);
    CHECK((driver_cmds == std::vector<std::uint16_t>{0x010BU, 0x010BU, 7U, 0U}));
}

void test_real_rom_if_available() {
    const std::array<std::filesystem::path, 3> candidates{
        "local-roms/Beyond Oasis (USA).md",
        "../local-roms/Beyond Oasis (USA).md",
        "../Sega-Thor/local-roms/Beyond Oasis (USA).md",
    };

    std::filesystem::path rom_path;
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, ec)) {
            rom_path = candidate;
            break;
        }
        ec.clear();
    }
    if (rom_path.empty()) {
        return; // Optional check: CI runs without commercial ROMs
    }

    const auto rom = oasis::Rom::load(rom_path);
    std::array<std::uint8_t, 0x10000> ram{};
    const auto spawned = alloc_spawn_npc_entity_slot_18_from_ram(ram, rom.bytes());
    CHECK(spawned.has_value());
    CHECK(spawned->slot_index == 0U);
    CHECK(read_be16(ram, kGeneralPool18RamOffset + 0x00U) == kSpawnNpcEntityType);
    CHECK(read_be16(ram, kGeneralPool18RamOffset + 0x02U) == kSpawnNpcInitialTimer);
    CHECK(read_be16(ram, kGeneralPool18RamOffset + 0x2CU) == 0xFFFFU);
    CHECK(read_be16(ram, kGeneralPool18RamOffset + 0x5CU) == 0xFFFFU);
}

} // namespace

int main() {
    test_constants_and_layout();
    test_decompress_palette_block_and_ram_dma();
    test_projectile_and_spark_allocators();
    test_general_pool_find_and_spawn_npc();
    test_sound_driver_dispatchers();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.191 entity pool allocator & sound dispatch tests succeeded!\n";
    return 0;
}
