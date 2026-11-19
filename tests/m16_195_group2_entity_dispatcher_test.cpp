#include "game/world/group2_entity_dispatcher.hpp"
#include "../include/game/world/group2_entity_dispatcher.hpp"

#include "core/rom.hpp"

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
        std::cerr << "[FAIL] M16.195 check failed at line " << line << ": " << expr << '\n';
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

void test_constants_and_table_mappings() {
    static_assert(kMasterGroup2EntityDispatcherAddr == 0x00DEECU);
    static_assert(kGroup2ActionTableAddr == 0x00DF54U);
    static_assert(kGroup2DummyHandlerAddr == 0x00E0B8U);
    static_assert(kMasterGroup2EntityDispatcherEndAddr == 0x00E0BAU);
    static_assert(kGroup2DispatcherTotalBytes == 462U);
    static_assert(kGroup2DispatcherCodeBytes + kGroup2ActionTableBytes + 2U == kGroup2DispatcherTotalBytes);

    static_assert(kGroup2PauseGateRamAddr == 0x00FF17B8U);
    static_assert(kGroup2PoolRamAddr == 0x00FF1AA4U);
    static_assert(kGroup2SlotCount == 20U);
    static_assert(kGroup2SlotStride == 0xBCU);
    static_assert(kGroup2PoolRamOffset + kGroup2SlotCount * kGroup2SlotStride == 0x2954U);

    static_assert(kMinGroup2Type == 0x0016U);
    static_assert(kMaxGroup2Type == 0x00C6U);
    static_assert(kGroup2TableEntryCount == 89U);
    static_assert(kGroup2ActionHandlerAddrs.size() == 89U);

    // Type index mapping and skip gates (<= 0, < 0x16, odd, > 0xC6)
    static_assert(!group2_type_to_table_index(0).has_value());
    static_assert(!group2_type_to_table_index(-2).has_value());
    static_assert(!group2_type_to_table_index(0x0014).has_value());
    static_assert(!group2_type_to_table_index(0x0017).has_value());
    static_assert(!group2_type_to_table_index(0x00C8).has_value());
    static_assert(group2_type_to_table_index(0x0016) == 0U);
    static_assert(group2_type_to_table_index(0x002E) == 12U);
    static_assert(group2_type_to_table_index(0x00C6) == 88U);

    // Key ROM handler addresses
    static_assert(lookup_group2_action_handler(0x0016) == kGroup2HandlerBow);
    static_assert(lookup_group2_action_handler(0x0018) == kGroup2HandlerDitte);
    static_assert(lookup_group2_action_handler(0x001A) == kGroup2HandlerEfreet);
    static_assert(lookup_group2_action_handler(0x001C) == kGroup2HandlerShadow);
    static_assert(lookup_group2_action_handler(0x001E) == kGroup2HandlerSpiritInteract);
    static_assert(lookup_group2_action_handler(0x002A) == kGroup2DummyHandlerAddr);
    static_assert(lookup_group2_action_handler(0x002E) == kGroup2HandlerGeneralNpc);
    static_assert(lookup_group2_action_handler(0x00C6) == 0x016CBEU);
}

void test_isometric_footprint_projection() {
    // 0x00DF20..0x00DF49: eff_w = d1, s1 = (d2 + 1) >> 1, eff_h = s1 + (s1 >> 1)
    static_assert(project_isometric_footprint(16U, 8U, false) == IsometricFootprint{16, 6});
    static_assert(project_isometric_footprint(16U, 9U, false) == IsometricFootprint{16, 7});
    static_assert(project_isometric_footprint(16U, 10U, false) == IsometricFootprint{16, 7});
    static_assert(project_isometric_footprint(24U, 16U, false) == IsometricFootprint{24, 12});
    static_assert(project_isometric_footprint(16U, 8U, true) == IsometricFootprint{8, 12});
    static_assert(project_isometric_footprint(0U, 0U, false) == IsometricFootprint{0, 0});
    static_assert(project_isometric_footprint(255U, 255U, false) == IsometricFootprint{255, 192});

    Group2Entity ent{};
    ent.raw_extent_x = 20U;
    ent.raw_extent_y = 12U;
    ent.flags_17 = 0x00U;
    const auto fp_normal = apply_post_tick_footprint(ent);
    CHECK((fp_normal == IsometricFootprint{20, 9}));
    CHECK(ent.effective_half_w == 20);
    CHECK(ent.effective_half_h == 9);

    ent.flags_17 = 0x03U; // bit 0 set -> swap X/Y extents
    const auto fp_swapped = apply_post_tick_footprint(ent);
    CHECK((fp_swapped == IsometricFootprint{12, 15}));
    CHECK(ent.effective_half_w == 12);
    CHECK(ent.effective_half_h == 15);
}

void test_structured_pool_and_function_table_dispatch() {
    std::array<Group2Entity, kGroup2SlotCount> pool{};

    // Slot 0: Spirit Bow (0x16)
    pool[0] = {0x0016, 0x00U, 0, 0, 12U, 12U};
    // Slot 1: Inactive (0) -> skipped
    pool[1] = {0, 0x00U, 99, 99, 10U, 10U};
    // Slot 2: Negative type (-4) -> skipped
    pool[2] = {-4, 0x00U, 77, 77, 10U, 10U};
    // Slot 3: Sub-0x16 type (0x14) -> skipped
    pool[3] = {0x0014, 0x00U, 55, 55, 10U, 10U};
    // Slot 4: Unmapped dummy type (0x2A -> 0x00E0B8)
    pool[4] = {0x002A, 0x01U, 0, 0, 16U, 8U};
    // Slot 19: General NPC (0x2E -> 0x00E25C), last of 20 slots
    pool[19] = {0x002E, 0x00U, 0, 0, 24U, 16U};

    // 1. Pause gate active -> entire sweep skipped
    Group2TableFnArray fn_table{};
    fn_table[0] = [](Group2Entity& e) { e.raw_extent_x = 99U; };
    fn_table[12] = [](Group2Entity& e) { e.type = 0; }; // Simulates self-despawn during tick

    const auto paused_res = dispatch_group2_entities(pool, true, fn_table);
    CHECK(paused_res.paused);
    CHECK(paused_res.dispatched_count == 0U);
    CHECK(pool[0].raw_extent_x == 12U);
    CHECK(pool[0].effective_half_w == 0);

    // 2. Unpaused dispatch via fn_table
    const auto res = dispatch_group2_entities(pool, false, fn_table);
    CHECK(!res.paused);
    CHECK(res.dispatched_count == 3U);
    CHECK(res.dummy_count == 1U);
    CHECK(res.skipped_count == 17U);
    CHECK(res.events.size() == 3U);

    // Slot 0 handler updated raw_extent_x to 99 before post-tick footprint ran
    CHECK(res.events[0].slot_index == 0U);
    CHECK(res.events[0].handler_rom_addr == kGroup2HandlerBow);
    CHECK(pool[0].effective_half_w == 99);
    CHECK(pool[0].effective_half_h == 9);

    // Skipped slots 1, 2, 3 must retain original half-widths untouched
    CHECK(pool[1].effective_half_w == 99);
    CHECK(pool[2].effective_half_w == 77);
    CHECK(pool[3].effective_half_w == 55);

    // Dummy slot 4 still gets post-tick swapped footprint (8, 12)
    CHECK(res.events[1].is_dummy_handler);
    CHECK(res.events[1].handler_rom_addr == kGroup2DummyHandlerAddr);
    CHECK(pool[4].effective_half_w == 8);
    CHECK(pool[4].effective_half_h == 12);

    // Slot 19 self-set type = 0 during tick, yet 0x00DF20..0x00DF46 STILL ran post-tick
    CHECK(res.events[2].slot_index == 19U);
    CHECK(res.events[2].entity_type == 0x002E);
    CHECK(res.events[2].handler_rom_addr == kGroup2HandlerGeneralNpc);
    CHECK(pool[19].type == 0);
    CHECK(pool[19].effective_half_w == 24);
    CHECK(pool[19].effective_half_h == 12);
}

void test_callback_and_ram_window_dispatch() {
    // Callback overload on dynamic slice
    std::array<Group2Entity, 4> small_pool{};
    small_pool[0] = {0x0018, 0x00U, 0, 0, 10U, 14U}; // Ditte
    small_pool[2] = {0x003E, 0x00U, 0, 0, 8U, 8U};   // Dummy 0x00E0B8
    std::vector<std::uint32_t> called_addrs;
    const auto cb_res = dispatch_group2_entities(
        small_pool,
        false,
        [&](std::size_t, Group2Entity&, std::uint32_t addr) { called_addrs.push_back(addr); });
    CHECK(cb_res.dispatched_count == 2U);
    CHECK(cb_res.dummy_count == 1U);
    CHECK((called_addrs == std::vector<std::uint32_t>{kGroup2HandlerDitte, kGroup2DummyHandlerAddr}));
    CHECK(small_pool[0].effective_half_h == 10); // (14+1)>>1=7, 7+(7>>1)=10

    // 64KB RAM window test: $FF17B8 pause gate + 20 slots at $FF1AA4
    std::array<std::uint8_t, 0x10000> ram{};
    const auto slot0_off = kGroup2PoolRamOffset + (0U * kGroup2SlotStride);
    const auto slot1_off = kGroup2PoolRamOffset + (1U * kGroup2SlotStride);
    const auto slot19_off = kGroup2PoolRamOffset + (19U * kGroup2SlotStride);

    write_be16(ram, slot0_off + 0x00U, 0x001AU); // Efreet (0x01254C)
    ram[slot0_off + 0x48U] = 14U;
    ram[slot0_off + 0x49U] = 10U;

    write_be16(ram, slot1_off + 0x00U, 0x0010U); // < 0x16 -> skipped
    write_be16(ram, slot1_off + 0x42U, 0x1234U);

    write_be16(ram, slot19_off + 0x00U, 0x002EU); // NPC
    ram[slot19_off + 0x17U] = 0x01U;              // swap X/Y extents
    ram[slot19_off + 0x48U] = 16U;
    ram[slot19_off + 0x49U] = 8U;

    // Paused when bit 1 of $FF17B8 is set (0x02)
    ram[kGroup2PauseGateRamOffset] = kGroup2PauseGateBit;
    const auto ram_paused = dispatch_group2_entities_from_ram(ram);
    CHECK(ram_paused.paused);
    CHECK(read_be16(ram, slot0_off + 0x42U) == 0U);

    // Unpaused (unrelated bit 0 set on $FF17B8 does NOT pause Group 2)
    ram[kGroup2PauseGateRamOffset] = 0x01U;
    std::vector<std::size_t> ram_slots;
    const auto ram_res = dispatch_group2_entities_from_ram(
        ram,
        [&](std::size_t idx, std::span<std::uint8_t, kGroup2SlotStride> bytes, std::uint32_t addr) {
            ram_slots.push_back(idx);
            if (idx == 0U) {
                CHECK(addr == kGroup2HandlerEfreet);
                bytes[0x48U] = 18U;
            }
        });

    CHECK(!ram_res.paused);
    CHECK(ram_res.dispatched_count == 2U);
    CHECK((ram_slots == std::vector<std::size_t>{0U, 19U}));
    CHECK(read_be16(ram, slot0_off + 0x42U) == 18U);
    CHECK(read_be16(ram, slot0_off + 0x44U) == 7U); // (10+1)>>1=5 -> 5+2=7
    CHECK(read_be16(ram, slot1_off + 0x42U) == 0x1234U); // Skipped
    CHECK(read_be16(ram, slot19_off + 0x42U) == 8U); // Swapped
    CHECK(read_be16(ram, slot19_off + 0x44U) == 12U); // (16+1)>>1=8 -> 8+4=12

    // Synthetic ROM lookup verification
    std::vector<std::uint8_t> synth_rom(kGroup2ActionTableAddr + kGroup2ActionTableBytes, 0U);
    write_be32(synth_rom, kGroup2ActionTableAddr + 12U * 4U, 0x000ABCDEU);
    CHECK(lookup_group2_action_handler_in_rom(synth_rom, 0x002E) == 0x000ABCDEU);
    CHECK(!lookup_group2_action_handler_in_rom(synth_rom, 0x0014).has_value());
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
    for (std::size_t i = 0U; i < kGroup2TableEntryCount; ++i) {
        const auto type_id = static_cast<std::int16_t>(kMinGroup2Type + i * 2U);
        const auto rom_handler = lookup_group2_action_handler_in_rom(rom.bytes(), type_id);
        CHECK(rom_handler.has_value());
        CHECK(*rom_handler == kGroup2ActionHandlerAddrs[i]);
    }
    // Confirm 0x00E0B8 is `rts` (0x4E75)
    CHECK(read_be16(rom.bytes(), kGroup2DummyHandlerAddr) == 0x4E75U);
}

} // namespace

int main() {
    test_constants_and_table_mappings();
    test_isometric_footprint_projection();
    test_structured_pool_and_function_table_dispatch();
    test_callback_and_ram_window_dispatch();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.195 Group 2 entity dispatcher tests succeeded!\n";
    return 0;
}
