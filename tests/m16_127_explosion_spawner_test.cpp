#include "game/combat/explosion_spawner.hpp"

#include <array>
#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_sfx_cooldown_logic() {
    std::uint16_t last_sfx = 94U;

    // Delta 6 < 7: inside cooldown, no SFX
    bool play = check_and_update_explosion_sfx_cooldown(100U, last_sfx);
    assert(!play);
    assert(last_sfx == 94U);

    // Delta 7 >= 7: cooldown expired, plays SFX and updates last_sfx
    play = check_and_update_explosion_sfx_cooldown(101U, last_sfx);
    assert(play);
    assert(last_sfx == 101U);

    // 16-bit unsigned wrap: 65535 to 2 (delta = 3 < 7)
    last_sfx = 65535U;
    play = check_and_update_explosion_sfx_cooldown(2U, last_sfx);
    assert(!play);
    assert(last_sfx == 65535U);

    // Negative signed delta (e.g. clock jump backwards or delta >= 0x8000)
    last_sfx = 32773U;
    play = check_and_update_explosion_sfx_cooldown(2U, last_sfx);
    assert(play);
    assert(last_sfx == 2U);
}

static void test_shade_phase_sync() {
    std::uint8_t flags_36 = 0x50U;
    std::uint8_t flags_38 = 0xA0U;

    // Shade active (0x001E) sets bit 1 on 0x36 and bit 2 on 0x38
    sync_shade_phase_flags(kShadeSpiritId, flags_36, flags_38);
    assert(flags_36 == 0x52U);
    assert(flags_38 == 0xA4U);

    // Other spirit (e.g. Dytto 0x0002) clears bit 1 on 0x36 and bit 2 on 0x38
    sync_shade_phase_flags(0x0002U, flags_36, flags_38);
    assert(flags_36 == 0x50U);
    assert(flags_38 == 0xA0U);
}

static void test_spawn_explosion_dispatch() {
    std::uint16_t last_sfx = 0U;

    // Slot unavailable
    auto res_fail = spawn_explosion_normal(100, 200, 10, 0x1234, 10, last_sfx, false);
    assert(!res_fail.spawned);
    assert(!res_fail.sfx_played);

    // Normal explosion spawn
    auto res_norm = spawn_explosion_normal(120, 240, 5, 0x002A, 10, last_sfx, true, kShadeSpiritId);
    assert(res_norm.spawned);
    assert(res_norm.entity.entity_type == 0x0034U);
    assert(res_norm.entity.pos_x_16_16 == (120 << 16));
    assert(res_norm.entity.pos_y_16_16 == (240 << 16));
    assert(res_norm.entity.pos_z_16_16 == (5 << 16));
    assert(res_norm.entity.base_x_16_16 == (120 << 16));
    assert(res_norm.entity.base_y_16_16 == (240 << 16));
    assert(res_norm.entity.timer_16 == 0U);
    assert(res_norm.entity.sprite_id == 0x055EU);
    assert(res_norm.entity.param_9e == 0x002AU);
    assert((res_norm.entity.flags_36 & 0x02U) != 0U);
    assert((res_norm.entity.flags_38 & 0x04U) != 0U);
    assert(res_norm.sfx_played);
    assert(res_norm.sfx_1 == 0x0090021BU);
    assert(res_norm.sfx_2 == 0x00600382U);
    assert(last_sfx == 10U);

    // Hi explosion variant (sets bit 15 of sprite ID -> 0x855E)
    // Frame 12 is within cooldown (12 - 10 = 2 < 7)
    auto res_hi = spawn_explosion_hi(150, 300, 0, 0x0055, 12, last_sfx, true);
    assert(res_hi.spawned);
    assert(res_hi.entity.sprite_id == 0x855EU);
    assert(!res_hi.sfx_played);
    assert(res_hi.sfx_1 == 0U);
    assert(res_hi.sfx_2 == 0U);
    assert(last_sfx == 10U);
}

static void test_raw_entity_slot_write() {
    std::array<std::uint8_t, 0xBC> slot{};
    ExplosionSpawnRequest req{
        .x = 80,
        .y = 160,
        .z = 20,
        .param_9e = 0x00FFU,
        .hi_variant = true
    };

    bool ok = write_explosion_to_entity_slot(slot, req, kShadeSpiritId);
    assert(ok);

    // Verify entity type at +0x00: 0x0034
    assert(slot[0x00] == 0x00 && slot[0x01] == 0x34);

    // Verify pos_x at +0x08: 80 << 16 = 0x00500000
    assert(slot[0x08] == 0x00 && slot[0x09] == 0x50 && slot[0x0A] == 0x00 && slot[0x0B] == 0x00);

    // Verify pos_y at +0x0C: 160 << 16 = 0x00A00000
    assert(slot[0x0C] == 0x00 && slot[0x0D] == 0xA0 && slot[0x0E] == 0x00 && slot[0x0F] == 0x00);

    // Verify sprite_id at +0x18: 0x855E
    assert(slot[0x18] == 0x85 && slot[0x19] == 0x5E);

    // Verify flags_36 bit 1 and flags_38 bit 2
    assert((slot[0x36] & 0x02U) != 0U);
    assert((slot[0x38] & 0x04U) != 0U);

    // Verify base coords at +0x4E and +0x52
    assert(slot[0x4E] == 0x00 && slot[0x4F] == 0x50 && slot[0x50] == 0x00 && slot[0x51] == 0x00);
    assert(slot[0x52] == 0x00 && slot[0x53] == 0xA0 && slot[0x54] == 0x00 && slot[0x55] == 0x00);

    // Verify param_9e at +0x9E: 0x00FF
    assert(slot[0x9E] == 0x00 && slot[0x9F] == 0xFF);
}

int main() {
    test_sfx_cooldown_logic();
    test_shade_phase_sync();
    test_spawn_explosion_dispatch();
    test_raw_entity_slot_write();
    std::cout << "oasis_m16_127_explosion_spawner_test passed\n";
    return 0;
}
