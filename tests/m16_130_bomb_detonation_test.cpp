#include "game/combat/bomb_detonation.hpp"

#include <array>
#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_blast_z_clamp() {
    // Parity with Python reference test: max(0, z - 0x00040000)
    assert(compute_blast_z(0x00060000) == 0x00020000);
    assert(compute_blast_z(0x00040000) == 0);
    assert(compute_blast_z(0x00020000) == 0);
    assert(compute_blast_z(0) == 0);
}

static void test_blast_tile_attr_mask() {
    // ROM 0x00ED62 & 0x00ED7A: andi.w #$f7ff and andi.w #$9fff -> andi.w #$97ff
    assert(compute_blast_tile_attr(0xFFFFU) == 0x97FFU);
    assert(compute_blast_tile_attr(0x0800U) == 0U);
    assert(compute_blast_tile_attr(0x2000U) == 0U);
    assert(compute_blast_tile_attr(0x4000U) == 0U);
    assert(compute_blast_tile_attr(0x1234U) == (0x1234U & 0x97FFU));
}

static void test_detonate_bomb_state_and_side_effects() {
    BombDetonationRequest req{
        .pos_x_16_16 = 0x00140000,
        .pos_y_16_16 = 0x00280000,
        .pos_z_16_16 = 0x00070000,
        .attacker_payload_aa = kAliEntityRamAddr,
        .ali_tile_attr_5a = 0x85F0U
    };

    auto res = detonate_bomb(req);
    assert(res.blast_entity.entity_type == 0x003AU);
    assert(res.blast_entity.anim_state_4 == 8U);
    assert(res.blast_entity.state_2a == 8U);
    assert(res.blast_entity.pos_x_16_16 == 0x00140000);
    assert(res.blast_entity.pos_y_16_16 == 0x00280000);
    assert(res.blast_entity.pos_z_16_16 == 0x00030000); // 0x70000 - 0x40000
    assert(res.blast_entity.timer_16 == 0U);
    assert(res.blast_entity.tile_attr_18 == (0x85F0U & 0x97FFU));
    assert((res.blast_entity.flags_36 & 0x08U) != 0U);
    assert((res.blast_entity.flags_38 & 0x08U) != 0U);
    assert((res.blast_entity.flags_3c & 0x02U) != 0U);
    assert(res.blast_entity.attacker_payload_aa == kAliEntityRamAddr);
    assert(res.blast_entity.hit_type_a6 == 2U);
    assert(res.blast_entity.damage_word_a8 == 0x8007U);

    // RAM side effects
    assert(res.detonation_flag_set);
    assert(res.ali_lock_cleared);
    assert(res.screen_shake_frames == 4U);
    assert(res.sfx_1 == 0x00B00225U);
    assert(res.sfx_2 == 0x00600382U);
}

static void test_raw_entity_slot_write() {
    std::array<std::uint8_t, 0xBC> slot{};
    slot[0x36] = 0x50U;
    slot[0x38] = 0x20U;
    slot[0x3C] = 0x00U;

    BombDetonationRequest req{
        .pos_x_16_16 = 0x00320000,
        .pos_y_16_16 = 0x00640000,
        .pos_z_16_16 = 0x00010000, // drops to 0
        .attacker_payload_aa = 0x00FF19E8U,
        .ali_tile_attr_5a = 0x1234U
    };

    bool ok = write_bomb_blast_to_entity_slot(slot, req);
    assert(ok);

    // +0x00: entity type 0x003A
    assert(slot[0x00] == 0x00 && slot[0x01] == 0x3A);

    // +0x04: anim state 8
    assert(slot[0x04] == 0x00 && slot[0x05] == 0x08);

    // +0x08: pos_x 0x00320000
    assert(slot[0x08] == 0x00 && slot[0x09] == 0x32 && slot[0x0A] == 0x00 && slot[0x0B] == 0x00);

    // +0x0C: pos_y 0x00640000
    assert(slot[0x0C] == 0x00 && slot[0x0D] == 0x64 && slot[0x0E] == 0x00 && slot[0x0F] == 0x00);

    // +0x10: pos_z 0
    assert(slot[0x10] == 0x00 && slot[0x11] == 0x00 && slot[0x12] == 0x00 && slot[0x13] == 0x00);

    // +0x16: timer 0
    assert(slot[0x16] == 0x00 && slot[0x17] == 0x00);

    // +0x2A: state 8
    assert(slot[0x2A] == 0x00 && slot[0x2B] == 0x08);

    // +0x36: bit 3 set (0x50 | 0x08 = 0x58)
    assert(slot[0x36] == 0x58U);

    // +0x38: bit 3 set (0x20 | 0x08 = 0x28)
    assert(slot[0x38] == 0x28U);

    // +0x3C: bit 1 set (0x02)
    assert(slot[0x3C] == 0x02U);

    // +0xA6: hit_type 2
    assert(slot[0xA6] == 0x00 && slot[0xA7] == 0x02);

    // +0xA8: damage_word 0x8007
    assert(slot[0xA8] == 0x80 && slot[0xA9] == 0x07);

    // +0xAA: attacker payload 0x00FF19E8
    assert(slot[0xAA] == 0x00 && slot[0xAB] == 0xFF && slot[0xAC] == 0x19 && slot[0xAD] == 0xE8);
}

int main() {
    test_blast_z_clamp();
    test_blast_tile_attr_mask();
    test_detonate_bomb_state_and_side_effects();
    test_raw_entity_slot_write();
    std::cout << "oasis_m16_130_bomb_detonation_test passed\n";
    return 0;
}
