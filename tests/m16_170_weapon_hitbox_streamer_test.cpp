#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "game/combat/weapon_hitbox_streamer.hpp"

using namespace oasis::game::combat;

namespace {

void write_be16(std::vector<std::uint8_t>& mem, std::size_t offset, std::uint16_t val) {
    if (offset + 2 <= mem.size()) {
        mem[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
        mem[offset + 1] = static_cast<std::uint8_t>(val & 0xFF);
    }
}

void write_be16_signed(std::vector<std::uint8_t>& mem, std::size_t offset, std::int16_t val) {
    write_be16(mem, offset, static_cast<std::uint16_t>(val));
}

constexpr std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

// Builds a mock 36-byte 3-direction weapon frame stream (Down, Right, Up).
std::vector<std::uint8_t> create_mock_weapon_stream() {
    std::vector<std::uint8_t> stream(36, 0);
    // Dir 0 (Down, offset 0): z[0, 20], x[-8, 8], y[0, 24]
    write_be16_signed(stream, 0, 0);
    write_be16_signed(stream, 2, 20);
    write_be16_signed(stream, 4, -8);
    write_be16_signed(stream, 6, 0);
    write_be16_signed(stream, 8, 8);
    write_be16_signed(stream, 10, 24);

    // Dir 1 (Right, offset 12): z[0, 20], x[0, 30], y[-10, 10]
    write_be16_signed(stream, 12, 0);
    write_be16_signed(stream, 14, 20);
    write_be16_signed(stream, 16, 0);
    write_be16_signed(stream, 18, -10);
    write_be16_signed(stream, 20, 30);
    write_be16_signed(stream, 22, 10);

    // Dir 2 (Up, offset 24): z[0, 20], x[-8, 8], y[-24, 0]
    write_be16_signed(stream, 24, 0);
    write_be16_signed(stream, 26, 20);
    write_be16_signed(stream, 28, -8);
    write_be16_signed(stream, 30, -24);
    write_be16_signed(stream, 32, 8);
    write_be16_signed(stream, 34, 0);

    return stream;
}

void test_constants() {
    static_assert(WeaponHitboxConfig::kPlayerEntryAddr == 0x00BAACU);
    static_assert(WeaponHitboxConfig::kEnemiesEntryAddr == 0x00BAB8U);
    static_assert(WeaponHitboxConfig::kPrimaryExitRtsAddr == 0x00BBA6U);
    static_assert(WeaponHitboxConfig::kUnprotectedEntryAddr == 0x00BBA8U);
    static_assert(WeaponHitboxConfig::kSecondaryExitRtsAddr == 0x00BC8CU);
    static_assert(WeaponHitboxConfig::kWeaponHitboxRangeEnd == 0x00BC8EU);

    static_assert(WeaponHitboxConfig::kGroup2PoolRam == 0x00FF19E8U);
    static_assert(WeaponHitboxConfig::kEnemyPoolRam == 0x00FF1CD8U);
    static_assert(WeaponHitboxConfig::kPlayerSlotCount == 1U);
    static_assert(WeaponHitboxConfig::kEnemySlotCount == 17U);
    static_assert(WeaponHitboxConfig::kAllGroup2SlotCount == 21U);
    static_assert(WeaponHitboxConfig::kSlotStride == 188U);
    static_assert(WeaponHitboxConfig::kHitboxEntryBytes == 12U);

    std::cout << "[PASS] test_constants\n";
}

void test_unpack_and_mirroring() {
    const std::vector<std::uint8_t> stream = create_mock_weapon_stream();

    // Down (dir 0)
    const DirectionalHitbox down = resolve_directional_hitbox(0, stream);
    assert(!down.mirrored);
    assert(down.stream_offset == 0);
    assert(down.local_box.z_min == 0 && down.local_box.z_max == 20);
    assert(down.local_box.x_min == -8 && down.local_box.x_max == 8);
    assert(down.local_box.y_min == 0 && down.local_box.y_max == 24);

    // Right (dir 1)
    const DirectionalHitbox right = resolve_directional_hitbox(1, stream);
    assert(!right.mirrored);
    assert(right.stream_offset == 12);
    assert(right.local_box.x_min == 0 && right.local_box.x_max == 30);

    // Left (dir 3 -> mirrors dir 1)
    const DirectionalHitbox left = resolve_directional_hitbox(3, stream);
    assert(left.mirrored);
    assert(left.stream_offset == 12);
    // Mirrored: x_min = -30, x_max = 0
    assert(left.local_box.x_min == -30 && left.local_box.x_max == 0);
    assert(left.local_box.y_min == -10 && left.local_box.y_max == 10);
    assert(left.local_box.z_min == 0 && left.local_box.z_max == 20);

    // Up (dir 2)
    const DirectionalHitbox up = resolve_directional_hitbox(2, stream);
    assert(!up.mirrored);
    assert(up.stream_offset == 24);
    assert(up.local_box.y_min == -24 && up.local_box.y_max == 0);

    std::cout << "[PASS] test_unpack_and_mirroring\n";
}

void test_world_projection() {
    const RawHitboxRecord local{
        .z_min = 5,
        .z_max = 25,
        .x_min = -10,
        .y_min = -8,
        .x_max = 30,
        .y_max = 12
    };

    const ProjectedHitbox proj = project_to_world(local, 200, 300, 15, 10);
    // z_base = 15 + 10 = 25 -> z_min = 30, z_max = 50
    assert(proj.z_min == 30);
    assert(proj.z_max == 50);
    // x_min = -10 + 200 = 190, x_max = 30 + 200 = 230
    assert(proj.x_min == 190);
    assert(proj.x_max == 230);
    // y_base = 300 + 10 = 310 -> y_min = 302, y_max = 322
    assert(proj.y_min == 302);
    assert(proj.y_max == 322);

    std::cout << "[PASS] test_world_projection\n";
}

void test_target_filters() {
    TargetEntityState target{
        .address = 0x00FF1CD8U,
        .type = 1,
        .flags_38 = 0,
        .flags_3a = 0
    };

    // Valid target
    assert(is_target_eligible(target, 0x00FF19E8U, false));

    // Inactive target (type <= 0)
    target.type = 0;
    assert(!is_target_eligible(target, 0x00FF19E8U, false));
    target.type = -1;
    assert(!is_target_eligible(target, 0x00FF19E8U, false));
    target.type = 1;

    // Self target (address == attacker_addr)
    assert(!is_target_eligible(target, 0x00FF1CD8U, false));

    // Invulnerable (bit 3 on flags_38)
    target.flags_38 = 0x08U;
    assert(!is_target_eligible(target, 0x00FF19E8U, false));
    target.flags_38 = 0x00U;

    // Protected (bit 0 on flags_3a)
    target.flags_3a = 0x01U;
    // Primary scan: filtered out
    assert(!is_target_eligible(target, 0x00FF19E8U, false));
    // Unprotected scan: eligible
    assert(is_target_eligible(target, 0x00FF19E8U, true));

    std::cout << "[PASS] test_target_filters\n";
}

void test_aabb_overlap_evaluation() {
    const ProjectedHitbox box{
        .z_min = 10,
        .z_max = 30,
        .x_min = 100,
        .y_min = 150,
        .x_max = 140,
        .y_max = 180
    };

    TargetEntityState target{
        .address = 0x00FF1CD8U,
        .type = 1,
        .world_x = 120,
        .world_y = 160,
        .elev_z = 10,
        .ground_h = 5,
        .rad_x = 15,
        .rad_y = 12,
        .height_z = 20
    };
    // target tz_base = 15, tz_top = 35. Overlaps box z[10, 30].
    // target tx[105, 135]. Overlaps box x[100, 140].
    // target ty_adj = 165, ty[153, 177]. Overlaps box y[150, 180].
    assert(test_aabb_overlap(box, target));

    // Z disjoint (above)
    target.elev_z = 40; // tz_base = 45 > box.z_max = 30
    assert(!test_aabb_overlap(box, target));
    target.elev_z = 10;

    // X disjoint (to the right)
    target.world_x = 200; // tx_min = 185 > box.x_max = 140
    assert(!test_aabb_overlap(box, target));
    target.world_x = 120;

    // Y disjoint (to the bottom)
    target.world_y = 250; // ty_min = 243 > box.y_max = 180
    assert(!test_aabb_overlap(box, target));

    std::cout << "[PASS] test_aabb_overlap_evaluation\n";
}

void test_dual_pool_scans() {
    const std::vector<std::uint8_t> stream = create_mock_weapon_stream();

    // Attacker facing Right (dir 1) at world (100, 150), z=0, ground_h=0
    const AttackerEntityState attacker{
        .address = 0x00FF19E8U, // Player
        .world_x = 100,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .facing = 1 // Right
    };
    // Box for Right: z[0, 20], x[100, 130], y[140, 160]

    // Enemy 0: out of range (x=200)
    // Enemy 1: in range (x=115, y=150, z=0, height=20, rad=10)
    // Enemy 2: in range but protected
    std::vector<TargetEntityState> enemies(3);
    enemies[0] = TargetEntityState{
        .address = 0x00FF1CD8U,
        .type = 2,
        .world_x = 200,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .rad_x = 10,
        .rad_y = 10,
        .height_z = 20
    };
    enemies[1] = TargetEntityState{
        .address = 0x00FF1D94U,
        .type = 2,
        .world_x = 115,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .rad_x = 10,
        .rad_y = 10,
        .height_z = 20
    };
    enemies[2] = TargetEntityState{
        .address = 0x00FF1E50U,
        .type = 2,
        .world_x = 110,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .flags_3a = 0x01U, // Protected
        .rad_x = 10,
        .rad_y = 10,
        .height_z = 20
    };

    // Enemy scan: should hit enemies[1]
    const WeaponHitResult res1 = scan_enemy_weapon_hitbox(attacker, stream, enemies);
    assert(res1.carry);
    assert(res1.target_addr == 0x00FF1D94U);
    assert(res1.slot_index == 1);

    // Protected enemy test: make enemy 1 inactive, test enemy 2
    enemies[1].type = 0; // Inactive
    const WeaponHitResult res_prot_blocked = scan_enemy_weapon_hitbox(attacker, stream, enemies);
    assert(!res_prot_blocked.carry);

    // Unprotected scan (0x00BBA8): should pierce enemy 2's protection
    const WeaponHitResult res_prot_hit = scan_enemy_weapon_hitbox_unprotected(attacker, stream, enemies);
    assert(res_prot_hit.carry);
    assert(res_prot_hit.target_addr == 0x00FF1E50U);
    assert(res_prot_hit.slot_index == 2);

    // Player scan test: enemy attacks player
    const AttackerEntityState enemy_attacker{
        .address = 0x00FF1CD8U,
        .world_x = 115,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .facing = 3 // Left (mirrors Right, box x[-30, 0] -> world x[85, 115])
    };
    const TargetEntityState player{
        .address = 0x00FF19E8U,
        .type = 1,
        .world_x = 100,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .rad_x = 8,
        .rad_y = 8,
        .height_z = 24
    };
    const WeaponHitResult res_player = scan_player_weapon_hitbox(enemy_attacker, stream, player);
    assert(res_player.carry);
    assert(res_player.target_addr == 0x00FF19E8U);

    std::cout << "[PASS] test_dual_pool_scans\n";
}

void test_raw_ram_window_simulation() {
    std::vector<std::uint8_t> ram(0x10000, 0); // 64KB RAM window
    const std::vector<std::uint8_t> stream = create_mock_weapon_stream();

    // Populate Attacker at $FF1CD8 (Enemy slot 0)
    const std::size_t atk_off = to_ram_offset(0x00FF1CD8U);
    write_be16_signed(ram, atk_off + WeaponHitboxConfig::kWorldXOffset, 120);
    write_be16_signed(ram, atk_off + WeaponHitboxConfig::kWorldYOffset, 150);
    write_be16_signed(ram, atk_off + WeaponHitboxConfig::kElevZOffset, 0);
    write_be16_signed(ram, atk_off + WeaponHitboxConfig::kGroundHOffset, 0);
    write_be16(ram, atk_off + WeaponHitboxConfig::kFacingOffset, 3); // Left

    // Populate Player at $FF19E8 (Group 2 slot 0)
    const std::size_t ply_off = to_ram_offset(0x00FF19E8U);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kTypeOffset, 1); // Active
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kWorldXOffset, 100);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kWorldYOffset, 150);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kElevZOffset, 0);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kGroundHOffset, 0);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kRadiusXOffset, 8);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kRadiusYOffset, 8);
    write_be16_signed(ram, ply_off + WeaponHitboxConfig::kHeightZOffset, 24);

    // Scan player from enemy
    const WeaponHitResult res_raw = scan_player_weapon_hitbox_raw(ram, 0x00FF1CD8U, stream);
    assert(res_raw.carry);
    assert(res_raw.target_addr == 0x00FF19E8U);

    std::cout << "[PASS] test_raw_ram_window_simulation\n";
}

} // namespace

int main() {
    test_constants();
    test_unpack_and_mirroring();
    test_world_projection();
    test_target_filters();
    test_aabb_overlap_evaluation();
    test_dual_pool_scans();
    test_raw_ram_window_simulation();

    std::cout << "\nAll M16.170 Weapon Hitbox Streamer tests PASSED!\n";
    return 0;
}
