#include "game/world/entity_stun_lookup.hpp"
#include "../include/game/world/entity_stun_lookup.hpp"
#include "game/world/entity_pool_alloc.hpp"
#include "game/world/group2_entity_dispatcher.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

using namespace thor::world;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.201 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
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

void test_constants_and_cross_headers() {
    static_assert(kEntityStunLookupStartAddr == 0x00F1B2U);
    static_assert(kPool2KnockbackStunBroadcastAddr == 0x00F1B2U);
    static_assert(kPool2KnockbackStunBroadcastEndAddr == 0x00F234U);
    static_assert(kPool2KnockbackStunBroadcastSizeBytes == 130U);
    static_assert(kEntityFindByTypeIdGlobalAddr == 0x00F234U);
    static_assert(kEntityFindByTypeIdGlobalEndAddr == 0x00F258U);
    static_assert(kEntityFindByTypeIdGlobalSizeBytes == 36U);
    static_assert(kEntityStunLookupEndAddr == 0x00F258U);
    static_assert(kEntityStunLookupSizeBytes == 166U);
    static_assert(kEntityStunLookupEndAddr - kEntityStunLookupStartAddr ==
                  kEntityStunLookupSizeBytes);
    static_assert(kPool2KnockbackStunBroadcastSizeBytes +
                  kEntityFindByTypeIdGlobalSizeBytes == kEntityStunLookupSizeBytes);

    static_assert(kStunBroadcastPool2RamAddr == kGeneralPool17RamAddr);
    static_assert(kStunBroadcastPool2SlotCount == kGeneralPool17Slots);
    static_assert(kGlobalTypeLookupPool1RamAddr == kPlayerEntityRamAddr);
    static_assert(kGlobalTypeLookupPool1SlotCount == 21U);
    static_assert(kStunEntitySlotStrideBytes == kGeneralPoolStride);
    static_assert(kStunPoolMemoryEndOffset == 0x2954U);
    static_assert(kStunScriptEventPtrRamAddr == 0x00FF17B2U);
    static_assert(kStunScriptEventPtrValue == 0x0004382EU);
    static_assert(kStunKnockbackDamageWord == 0xCFFFU);
    static_assert(kStunHitTimerLatchWord == 0xFFFFU);

    // Cross-namespace alias & co-inclusion with entity_pool_alloc / group2_entity_dispatcher
    static_assert(oasis::game::world::kEntityStunLookupEndAddr == kEntityStunLookupEndAddr);
    static_assert(sizeof(oasis::game::world::StunBroadcastEntity) == sizeof(StunBroadcastEntity));
    static_assert(kGroup2SlotStride == kStunEntitySlotStrideBytes);
}

void test_caller_occupied_abort_and_oob_guards() {
    std::array<StunBroadcastEntity, kStunBroadcastPool2SlotCount> pool{};
    pool[0].type = 0x0022; // Caller slot occupied -> bne.b $f22a abort
    pool[1].type = 0x0024;
    pool[1].facing_16 = 0x0001U;

    std::uint32_t script_ptr = 0x11223344U;
    const auto res_pos = broadcast_pool2_knockback_stun(pool, 0U, script_ptr);
    CHECK(!res_pos.carry_set);
    CHECK(res_pos.script_event_ptr_ff17b2 == 0U);
    CHECK(res_pos.affected_slots.empty());
    CHECK(script_ptr == 0x11223344U);
    CHECK(pool[1].damage_86 == 0U && pool[1].launch_dir_84 == 0U);

    // Negative caller status also fails tst.w $0(a5); bne.b $f22a
    pool[0].type = -1;
    CHECK(!broadcast_pool2_knockback_stun(pool, 0U, script_ptr).carry_set);
    CHECK(script_ptr == 0x11223344U);

    // Out-of-bounds caller index guard
    CHECK(!broadcast_pool2_knockback_stun(pool, 17U, script_ptr).carry_set);
    CHECK(!broadcast_pool2_knockback_stun(
              std::span<StunBroadcastEntity>(pool).first(4U), 4U, script_ptr)
               .carry_set);
}

void test_pool2_filter_chain_facing_and_launch_physics() {
    std::array<StunBroadcastEntity, kStunBroadcastPool2SlotCount> pool{};

    // Caller slot 0 is free (type == 0)
    // Slot 1: eligible, facing 1 (East) -> reversed launch_dir_84 = 3 (West), timer_8c = 0 -> 0xFFFF
    pool[1] = StunBroadcastEntity{
        .type = 0x0022, .facing_16 = 0x0001U, .flags_38 = 0U, .flags_3a = 0U,
        .hitstun_82 = 0U, .hit_timer_8c = 0U};
    // Slot 2: type <= 0 -> skipped
    pool[2] = StunBroadcastEntity{.type = -5, .facing_16 = 0x0000U};
    // Slot 3: intangible (+$38 bit 3) -> skipped
    pool[3] = StunBroadcastEntity{.type = 0x0024, .flags_38 = kStunIntangibleBit3Mask};
    // Slot 4: invulnerable (+$3A bit 0) -> skipped
    pool[4] = StunBroadcastEntity{.type = 0x0026, .flags_3a = kStunInvulnerableBit0Mask};
    // Slot 5: already in hitstun (+$82 != 0) -> skipped
    pool[5] = StunBroadcastEntity{.type = 0x0028, .hitstun_82 = 0x04U};
    // Slot 6: eligible, facing 4 (NE) + existing non-zero hit_timer_8c (preserved!)
    pool[6] = StunBroadcastEntity{
        .type = 0x0030, .facing_16 = 0x0104U, .hit_timer_8c = 0x0020U};
    // Slot 16 (last slot): eligible, facing 0 (North) -> reversed launch_dir_84 = 2 (South)
    pool[16] = StunBroadcastEntity{.type = 0x003AU, .facing_16 = 0x0000U};

    std::uint32_t script_ptr = 0U;
    const auto res = broadcast_pool2_knockback_stun(pool, 0U, script_ptr);
    CHECK(res.carry_set);
    CHECK(res.script_event_ptr_ff17b2 == kStunScriptEventPtrValue);
    CHECK(script_ptr == kStunScriptEventPtrValue);
    CHECK((res.affected_slots == std::vector<std::size_t>{1U, 6U, 16U}));

    // Slot 1: facing_16 (+$16) MUST remain unchanged! launch_dir_84 = 3 (West)
    CHECK(pool[1].facing_16 == 0x0001U);
    CHECK(pool[1].launch_dir_84 == 0x0003U);
    CHECK(pool[1].vel_x_fp == -0x00030000);
    CHECK(pool[1].vel_y_fp == 0);
    CHECK(pool[1].vel_z_fp == kStunLaunchUpwardVelZFp);
    CHECK(pool[1].damage_86 == kStunKnockbackDamageWord);
    CHECK(pool[1].hit_timer_8c == kStunHitTimerLatchWord);

    // Slot 6: bchg.b #1 toggles bit 1 in low byte (0x0104 ^ 2 = 0x0106 -> SW), preserves timer 0x20
    CHECK(pool[6].facing_16 == 0x0104U);
    CHECK(pool[6].launch_dir_84 == 0x0106U);
    CHECK(pool[6].vel_x_fp == -0x00024000); // -0x030000 * 0.75
    CHECK(pool[6].vel_y_fp == 0x00022800);  // +0x02E000 * 0.75
    CHECK(pool[6].vel_z_fp == kStunLaunchUpwardVelZFp);
    CHECK(pool[6].damage_86 == kStunKnockbackDamageWord);
    CHECK(pool[6].hit_timer_8c == 0x0020U);

    // Slot 16: facing 0 (N) -> launch_dir_84 = 2 (S)
    CHECK(pool[16].facing_16 == 0x0000U);
    CHECK(pool[16].launch_dir_84 == 0x0002U);
    CHECK(pool[16].vel_x_fp == 0 && pool[16].vel_y_fp == 0x0002E000);
    CHECK(pool[16].vel_z_fp == kStunLaunchUpwardVelZFp);

    // Skipped slots untouched
    for (const std::size_t idx : {2U, 3U, 4U, 5U}) {
        CHECK(pool[idx].damage_86 == 0U && pool[idx].launch_dir_84 == 0U);
        CHECK(pool[idx].vel_x_fp == 0 && pool[idx].vel_y_fp == 0 && pool[idx].vel_z_fp == 0);
    }

    // Custom 0x00E4D0 callback hook receives exact d7 = 0; empty pool still sets $FF17B2
    std::array<StunBroadcastEntity, kStunBroadcastPool2SlotCount> empty_pool{};
    script_ptr = 0U;
    std::size_t cb_calls = 0U;
    const auto empty_res = broadcast_pool2_knockback_stun(
        empty_pool, 3U, script_ptr,
        [&](std::size_t, StunBroadcastEntity&, std::uint16_t d7) {
            CHECK(d7 == 0U);
            ++cb_calls;
        });
    CHECK(empty_res.carry_set && empty_res.affected_slots.empty());
    CHECK(script_ptr == kStunScriptEventPtrValue && cb_calls == 0U);
}

void test_global_type_lookup() {
    std::array<std::int16_t, 25> types{};
    types.fill(0x0010);
    types[0] = 0x0002;  // Player Ali at $FF19E8
    types[12] = 0x003A; // Colossus Boss
    types[18] = 0x0072; // First duplicate
    types[20] = 0x0072; // Last slot in 21-slot sweep
    types[21] = 0x0099; // Beyond 21 slots (must NOT be found)

    const auto hit0 = find_entity_by_type_id_global(types, 0x0002);
    CHECK(hit0.found && hit0.slot_index == 0U && hit0.slot_ram_addr == 0x00FF19E8U);

    const auto hit12 = find_entity_by_type_id_global(types, 0x003A);
    CHECK(hit12.found && hit12.slot_index == 12U &&
          hit12.slot_ram_addr == 0x00FF19E8U + 12U * 0xBCU);

    const auto dup = find_entity_by_type_id_global(types, 0x0072);
    CHECK(dup.found && dup.slot_index == 18U);

    types[18] = 0x0010;
    const auto last = find_entity_by_type_id_global(types, 0x0072);
    CHECK(last.found && last.slot_index == 20U &&
          last.slot_ram_addr == 0x00FF2898U);

    CHECK(!find_entity_by_type_id_global(types, 0x0099).found);
    CHECK(!find_entity_by_type_id_global(types, 0x0000).found);

    // Matching type 0 works just like cmp.w $0(a0), d0
    types[4] = 0;
    const auto free_hit = find_entity_by_type_id_global(types, 0);
    CHECK(free_hit.found && free_hit.slot_index == 4U &&
          free_hit.slot_ram_addr == kStunBroadcastPool2RamAddr);
}

void test_ram_window_roundtrips_and_optional_rom() {
    std::array<std::uint8_t, 0x10000> ram{};

    // Populate slot 0 ($FF19E8) + Pool 2 slot 2 ($FF1CD8 + 2 * 0xBC)
    write_be16(ram, kGlobalTypeLookupPool1RamOffset, 0x0002U);
    StunBroadcastEntity target{};
    target.type = 0x0072;
    target.facing_16 = 0x0002U; // South -> flipped to 0 (North)
    target.hit_timer_8c = 0U;
    store_stun_entity_to_ram(ram, kStunBroadcastPool2RamAddr + 2U * 0xBCU, target);
    ram[kStunBroadcastPool2RamOffset + 2U * 0xBCU + 0x10U] = 0xAAU; // Unrelated byte sentinel

    // Global lookup across 21 slots finds Pool 2 slot 2 at Pool 1 index 6 (4 + 2)
    const auto gres = find_entity_by_type_id_global_from_ram(ram, 0x0072);
    CHECK(gres.found && gres.slot_index == 6U &&
          gres.slot_ram_addr == kStunBroadcastPool2RamAddr + 2U * 0xBCU);

    // Broadcast with caller_slot_index = 0 (free)
    const auto bres = broadcast_pool2_knockback_stun_from_ram(ram, 0U);
    CHECK(bres.carry_set);
    CHECK(bres.script_event_ptr_ff17b2 == kStunScriptEventPtrValue);
    CHECK((bres.affected_slots == std::vector<std::size_t>{2U}));
    CHECK(read_be32(ram, kStunScriptEventPtrRamOffset) == kStunScriptEventPtrValue);

    const auto loaded = load_stun_entity_from_ram(ram, gres.slot_ram_addr);
    CHECK(loaded.facing_16 == 0x0002U); // +$16 preserved
    CHECK(loaded.launch_dir_84 == 0x0000U);
    CHECK(loaded.vel_x_fp == 0 && loaded.vel_y_fp == -0x0002E000);
    CHECK(loaded.vel_z_fp == kStunLaunchUpwardVelZFp);
    CHECK(loaded.damage_86 == kStunKnockbackDamageWord);
    CHECK(loaded.hit_timer_8c == kStunHitTimerLatchWord);
    CHECK(ram[kStunBroadcastPool2RamOffset + 2U * 0xBCU + 0x10U] == 0xAAU);

    // Undersized RAM guards
    std::array<std::uint8_t, 64> tiny{};
    CHECK(!broadcast_pool2_knockback_stun_from_ram(tiny, 0U).carry_set);
    CHECK(!find_entity_by_type_id_global_from_ram(tiny, 0x0072).found);

    // Optional local ROM check (CI-safe: skips silently if commercial ROM is absent)
    const std::array<std::filesystem::path, 3> candidates{
        "local-roms/Beyond Oasis (USA).md",
        "../local-roms/Beyond Oasis (USA).md",
        "../Sega-Thor/local-roms/Beyond Oasis (USA).md",
    };
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(candidate, ec)) {
            continue;
        }
        std::ifstream in(candidate, std::ios::binary);
        if (!in) {
            break;
        }
        std::vector<std::uint8_t> rom(0x010000U);
        in.read(reinterpret_cast<char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
        if (static_cast<std::size_t>(in.gcount()) == rom.size()) {
            CHECK(read_be32(rom, 0x00F1B2U) == 0x48E7C106U); // movem.l d0-d1/d7/a5-a6, -(a7)
            CHECK(read_be32(rom, 0x00F21CU) == kStunScriptEventPtrValue);
            CHECK(read_be16(rom, 0x00F256U) == 0x4E75U); // rts
        }
        break;
    }
}

} // namespace

int main() {
    test_constants_and_cross_headers();
    test_caller_occupied_abort_and_oob_guards();
    test_pool2_filter_chain_facing_and_launch_physics();
    test_global_type_lookup();
    test_ram_window_roundtrips_and_optional_rom();

    std::cout << "[PASS] All M16.201 entity stun & global lookup tests succeeded!\n";
    return 0;
}
