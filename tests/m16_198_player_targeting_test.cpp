#include "game/physics/player_targeting.hpp"
#include "../include/game/physics/player_targeting.hpp"
#include "game/physics/aabb_force_fields.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

using namespace thor::physics;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.198 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(data[offset]) << 24U) |
        (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(data[offset + 3U]);
    return static_cast<std::int32_t>(raw);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_aliases() {
    static_assert(kPlayerTargetingStartAddr == 0x00E398U);
    static_assert(kTargetPlayerAliQuadrantAngleAddr == 0x00E398U);
    static_assert(kTargetPlayerAliQuadrantAngleEndAddr == 0x00E3E8U);
    static_assert(kTargetPlayerAliQuadrantAngleSizeBytes == 80U);

    static_assert(kTargetPlayerAliLoadMotionTableAddr == 0x00E3E8U);
    static_assert(kTargetPlayerAliLoadMotionTableEndAddr == 0x00E45EU);
    static_assert(kTargetPlayerAliLoadMotionTableSizeBytes == 118U);

    static_assert(kNormalizeTargetAimVectorDivsAddr == 0x00E45EU);
    static_assert(kNormalizeTargetAimVectorDivsEndAddr == 0x00E4D0U);
    static_assert(kNormalizeTargetAimVectorDivsSizeBytes == 114U);

    static_assert(kLaunchDirectionalProjectilePhysicsAddr == 0x00E4D0U);
    static_assert(kLaunchDirectionalProjectilePhysicsEndAddr == 0x00E502U);
    static_assert(kLaunchDirectionalProjectilePhysicsSizeBytes == 50U);

    static_assert(kPlayerTargetingEndAddr == 0x00E502U);
    static_assert(kPlayerTargetingSizeBytes == 362U);
    static_assert(kTargetPlayerAliQuadrantAngleSizeBytes +
                  kTargetPlayerAliLoadMotionTableSizeBytes +
                  kNormalizeTargetAimVectorDivsSizeBytes +
                  kLaunchDirectionalProjectilePhysicsSizeBytes == kPlayerTargetingSizeBytes);

    static_assert(kPlayerAliPosXRamAddr == 0x00FF19F0U);
    static_assert(kPlayerAliPosYRamAddr == 0x00FF19F4U);
    static_assert(kPlayerAliPosXRamOffset == 0x19F0U);
    static_assert(kPlayerAliPosYRamOffset == 0x19F4U);

    static_assert(kTargetEntityTimerOffset == 0x02U);
    static_assert(kTargetEntityPosXOffset == 0x08U);
    static_assert(kTargetEntityPosYOffset == 0x0CU);
    static_assert(kTargetEntityFacingOffset == 0x16U);
    static_assert(kTargetEntityVelXFpOffset == 0x4EU);
    static_assert(kTargetEntityVelYFpOffset == 0x52U);
    static_assert(kTargetEntityVelZFpOffset == 0x56U);
    static_assert(kTargetEntityLaunchDirOffset == 0x84U);

    static_assert(kTargetEntityMinMotionTimer == 16U);
    static_assert(kTargetMotionTableEntryBytes == 8U);
    static_assert(kTargetMotionTableEntries == 4U);
    static_assert(kLaunchSuppressMask == 0xC000U);
    static_assert(kLaunchBaseSpeedX == 0x0300U);
    static_assert(kLaunchBaseSpeedY == 0x02E0U);
    static_assert(kLaunchUpwardVelZFp == 0x00060000);

    // Alias check and coexistence with aabb_force_fields.hpp inside oasis::game::physics.
    static_assert(oasis::game::physics::kPlayerTargetingStartAddr == kPlayerTargetingStartAddr);
    static_assert(oasis::game::physics::kAabbForceFieldsStartAddr == 0x00DDA2U);
    static_assert(sizeof(oasis::game::physics::NormalizedAimVector) == sizeof(NormalizedAimVector));

    // Compile-time checks for 0x00E398 and 0x00E45E.
    constexpr TargetResult up_res = target_player_ali_quadrant_angle(100, 50, 100, 100);
    static_assert(up_res.direction == kTargetDirUp);
    static_assert(up_res.distance == 50U);

    constexpr NormalizedAimVector aim_check = normalize_target_aim_vector_divs(99, 200, 100, 100, 3U);
    static_assert(aim_check.half_span == 50U);
    static_assert(aim_check.quadrant == kTargetDirDown);
    static_assert(aim_check.vel_x_fp == -1280);
    static_assert(aim_check.vel_y_fp == 0x00020000);

    CHECK(up_res.dx == 0 && up_res.dy == -50);
    CHECK(aim_check.vel_x_fp == -1280);
}

void test_target_player_ali_quadrant_angle() {
    // 4 primary quadrants + Manhattan distance
    const auto r_up = target_player_ali_quadrant_angle(100, 40, 100, 100);
    CHECK(r_up.direction == kTargetDirUp && r_up.distance == 60U && r_up.abs_x == 0U && r_up.abs_y == 60U);

    const auto r_right = target_player_ali_quadrant_angle(170, 120, 100, 100);
    CHECK(r_right.direction == kTargetDirRight && r_right.distance == 90U);

    const auto r_down = target_player_ali_quadrant_angle(120, 190, 100, 100);
    CHECK(r_down.direction == kTargetDirDown && r_down.distance == 110U);

    const auto r_left = target_player_ali_quadrant_angle(50, 110, 100, 100);
    CHECK(r_left.direction == kTargetDirLeft && r_left.distance == 60U);

    // Exact diagonal ties (|dx| == |dy|) in 0x00E398: cmp.w d3, d4; bhi -> favors X!
    const auto tie_se = target_player_ali_quadrant_angle(120, 120, 100, 100);
    CHECK(tie_se.direction == kTargetDirRight && tie_se.distance == 40U);

    const auto tie_nw = target_player_ali_quadrant_angle(80, 80, 100, 100);
    CHECK(tie_nw.direction == kTargetDirLeft && tie_nw.distance == 40U);

    const auto zero_delta = target_player_ali_quadrant_angle(100, 100, 100, 100);
    CHECK(zero_delta.direction == kTargetDirRight && zero_delta.distance == 0U);

    // Signed 16-bit wrap / INT16_MIN delta safety
    const auto wrap_res = target_player_ali_quadrant_angle(-20000, 0, 20000, 0);
    // (uint16_t)(-20000) - 20000 = 25536 (positive 16-bit signed!)
    CHECK(wrap_res.dx == 25536);
    CHECK(wrap_res.direction == kTargetDirRight);
    CHECK(wrap_res.distance == 25536U);
}

void test_target_player_ali_and_load_motion_table() {
    // Authentic ROM 4x8 table layout from 0x01953C:
    // Up(0, -1.5), Right(+2.25, 0), Down(0, +1.5), Left(-2.25, 0)
    const std::array<std::uint8_t, 32> raw_table = {
        0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFEU, 0x80U, 0x00U, // dir 0: Up
        0x00U, 0x02U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, // dir 1: Right
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x80U, 0x00U, // dir 2: Down
        0xFFU, 0xFDU, 0xC0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, // dir 3: Left
    };
    const CardinalMotionTable table = decode_cardinal_motion_table(raw_table);
    CHECK(table[0].vel_y_fp == -0x00018000);
    CHECK(table[1].vel_x_fp == 0x00024000);
    CHECK(table[2].vel_y_fp == 0x00018000);
    CHECK(table[3].vel_x_fp == -0x00024000);

    // 1. Dominant axis distance < 16 (even though Manhattan = 10 + 10 = 20 >= 16) -> clamps to 16,
    //    AND |dx| == |dy| tie resolves to Y-dominant (Down) per cmp.w d4, d3; bls at 0x00E40C!
    TargetingEntityState ent{.pos_x = 100, .pos_y = 100};
    const auto r_tie_down = target_player_ali_and_load_motion_table(110, 110, ent, table);
    CHECK(r_tie_down.direction == kTargetDirDown);
    CHECK(r_tie_down.table_byte_offset == 16U);
    CHECK(r_tie_down.motion_timer == 16U);
    CHECK(ent.facing_16 == kTargetDirDown);
    CHECK(ent.vel_x_fp == 0);
    CHECK(ent.vel_y_fp == 0x00018000);
    CHECK(ent.timer_02 == 16U);

    // 2. |dx| == |dy| tie with dy < 0 -> Up (0), clamped to 16
    const auto r_tie_up = target_player_ali_and_load_motion_table(85, 85, ent, table);
    CHECK(r_tie_up.direction == kTargetDirUp);
    CHECK(r_tie_up.table_byte_offset == 0U);
    CHECK(r_tie_up.motion_timer == 16U); // dominant = 15 < 16 -> 16
    CHECK(ent.vel_y_fp == -0x00018000);

    // 3. Exact threshold 16 and > 16 on X-dominant Left & Right
    const auto r_left_16 = target_player_ali_and_load_motion_table(84, 90, ent, table);
    CHECK(r_left_16.direction == kTargetDirLeft);
    CHECK(r_left_16.table_byte_offset == 24U);
    CHECK(r_left_16.motion_timer == 16U);
    CHECK(ent.vel_x_fp == -0x00024000);
    CHECK(ent.vel_y_fp == 0);

    const auto r_right_45 = target_player_ali_and_load_motion_table(145, 110, ent, table);
    CHECK(r_right_45.direction == kTargetDirRight);
    CHECK(r_right_45.table_byte_offset == 8U);
    CHECK(r_right_45.motion_timer == 45U);
    CHECK(ent.vel_x_fp == 0x00024000);

    // Truncated table buffer returns zeroed table safely
    const CardinalMotionTable empty_tbl = decode_cardinal_motion_table(std::span<const std::uint8_t>{raw_table}.subspan(0, 12U));
    CHECK(empty_tbl[1].vel_x_fp == 0);
}

void test_normalize_target_aim_vector_divs() {
    // 1. half_span == 0 guards: dx/dy in {-1, 0, 1} -> (0, 0, current_facing, d4=0)
    constexpr std::array<std::int16_t, 3> tiny_deltas{-1, 0, 1};
    for (std::int16_t dx : tiny_deltas) {
        for (std::int16_t dy : tiny_deltas) {
            const auto res = normalize_target_aim_vector_divs(
                static_cast<std::int16_t>(100 + dx),
                static_cast<std::int16_t>(200 + dy),
                100, 200, kTargetDirLeft);
            CHECK(res.vel_x_fp == 0);
            CHECK(res.vel_y_fp == 0);
            CHECK(res.quadrant == kTargetDirLeft);
            CHECK(res.half_span == 0U);
        }
    }

    // 2. Signed divs.w truncation toward zero (negative small component)
    // dx = -1, dy = +100 -> half_span = 50; (-256 / 50) == -5 (NOT -6!) -> -5 << 8 = -1280
    const auto trunc_neg = normalize_target_aim_vector_divs(99, 200, 100, 100, 1U);
    CHECK(trunc_neg.half_span == 50U);
    CHECK(trunc_neg.quadrant == kTargetDirDown);
    CHECK(trunc_neg.vel_x_fp == -1280);
    CHECK(trunc_neg.vel_y_fp == 0x00020000);

    // dx = +1, dy = -100 -> half_span = 50; (+256 / 50) == +5 -> +1280
    const auto trunc_pos = normalize_target_aim_vector_divs(101, 0, 100, 100, 1U);
    CHECK(trunc_pos.half_span == 50U);
    CHECK(trunc_pos.quadrant == kTargetDirUp);
    CHECK(trunc_pos.vel_x_fp == 1280);
    CHECK(trunc_pos.vel_y_fp == -0x00020000);

    // 3. Odd max_span X-dominant Left: dx = -5, dy = +3 -> half_span = 5 >> 1 = 2
    // qx = (-1280 / 2) = -640 -> -2.5 px/frame (-0x00028000); qy = (768 / 2) = 384 -> +1.5 px/frame (0x00018000)
    const auto odd_x = normalize_target_aim_vector_divs(95, 103, 100, 100, 0U);
    CHECK(odd_x.half_span == 2U);
    CHECK(odd_x.quadrant == kTargetDirLeft);
    CHECK(odd_x.vel_x_fp == -0x00028000);
    CHECK(odd_x.vel_y_fp == 0x00018000);

    // 4. Pure Right: dx = +100, dy = 0 -> half_span = 50, vel_x = +2.0, vel_y = 0, quad = Right
    const auto pure_right = normalize_target_aim_vector_divs(200, 100, 100, 100, 0U);
    CHECK(pure_right.half_span == 50U);
    CHECK(pure_right.quadrant == kTargetDirRight);
    CHECK(pure_right.vel_x_fp == 0x00020000);
    CHECK(pure_right.vel_y_fp == 0);

    // 5. Diagonal tie |dx| == |dy| == 10 -> Y-dominant (cmp.w d5, d4; bhi)
    const auto tie_diag = normalize_target_aim_vector_divs(110, 90, 100, 100, 1U);
    CHECK(tie_diag.half_span == 5U);
    CHECK(tie_diag.quadrant == kTargetDirUp);
    CHECK(tie_diag.vel_x_fp == 0x00020000);
    CHECK(tie_diag.vel_y_fp == -0x00020000);

    // 6. INT16_MIN (-32768) extreme M68K asr.w #1 behavior:
    // abs_x = 0x8000 -> (int16_t)0x8000 >> 1 = -16384 (0xC000); (-32768 << 8) / (-16384) = +512
    const auto min_i16 = normalize_target_aim_vector_divs(-32768, 0, 0, 0, 2U);
    CHECK(min_i16.vel_x_fp == 0x00020000);
    CHECK(min_i16.vel_y_fp == 0);
    CHECK(min_i16.quadrant == kTargetDirRight);
    CHECK(min_i16.half_span == 0xC000U);
}

void test_launch_directional_projectile_physics() {
    // 1. Suppression mask bits 14/15 (d7 & 0xC000 != 0) leave entity velocities untouched
    for (std::uint16_t mask : {std::uint16_t{0x4000U}, std::uint16_t{0x8000U}, std::uint16_t{0xC000U}, std::uint16_t{0xFFFFU}}) {
        TargetingEntityState ent{.vel_x_fp = 0x1111, .vel_y_fp = 0x2222, .vel_z_fp = 0x3333, .launch_dir_84 = 1U};
        const auto res = launch_directional_projectile_physics(ent, mask);
        CHECK(!res.launched);
        CHECK(ent.vel_x_fp == 0x1111);
        CHECK(ent.vel_y_fp == 0x2222);
        CHECK(ent.vel_z_fp == 0x3333);
    }

    // 2. Active launch across all 8 directions (0x0300 -> 0x00030000, 0x02E0 -> 0x0002E000, diag * 0.75)
    struct ExpectedDir {
        std::uint16_t dir;
        std::int32_t vx;
        std::int32_t vy;
    };
    const std::array<ExpectedDir, 9> expected = {{
        {0U, 0, -0x0002E000},          // N
        {1U, 0x00030000, 0},           // E
        {2U, 0, 0x0002E000},           // S
        {3U, -0x00030000, 0},          // W
        {4U, 0x00024000, -0x00022800}, // NE (0.75 * 3.0, -0.75 * 2.875)
        {5U, 0x00024000, 0x00022800},  // SE
        {6U, -0x00024000, 0x00022800}, // SW
        {7U, -0x00024000, -0x00022800},// NW
        {13U, 0x00024000, 0x00022800}, // 13 & 7 == 5 (SE)
    }};

    for (const auto& tc : expected) {
        TargetingEntityState ent{.launch_dir_84 = tc.dir};
        const auto res = launch_directional_projectile_physics(ent, 0x3FFFU); // bits 14..15 clear
        CHECK(res.launched);
        CHECK(res.vel_x_fp == tc.vx);
        CHECK(res.vel_y_fp == tc.vy);
        CHECK(res.vel_z_fp == kLaunchUpwardVelZFp);
        CHECK(ent.vel_x_fp == tc.vx);
        CHECK(ent.vel_y_fp == tc.vy);
        CHECK(ent.vel_z_fp == kLaunchUpwardVelZFp);
    }
}

void test_ram_window_integration() {
    std::array<std::uint8_t, 0x10000> ram{};
    ram.fill(0xA5U);

    // Place Player Ali at (300, 120) in $FF19F0 / $FF19F4
    write_be16(ram, kPlayerAliPosXRamOffset, 300U);
    write_be16(ram, kPlayerAliPosYRamOffset, 120U);

    // Place target entity in slot 1 ($FF1AA4)
    constexpr std::uint32_t kEntAddr = 0x00FF1AA4U;
    constexpr std::size_t kEntOff = 0x1AA4U;
    store_targeting_entity_state_to_ram(ram, kEntAddr, TargetingEntityState{
        .timer_02 = 0U,
        .pos_x = 200,
        .pos_y = 140,
        .facing_16 = kTargetDirUp,
        .vel_x_fp = 0,
        .vel_y_fp = 0,
        .vel_z_fp = 0,
        .launch_dir_84 = 4U,
    });
    // Verify unrelated bytes inside slot (e.g. +$0A fractional X, +$20) remain 0xA5
    CHECK(ram[kEntOff + 0x0AU] == 0xA5U);
    CHECK(ram[kEntOff + 0x20U] == 0xA5U);

    // 1. 0x00E398 from RAM (via 24-bit address and 16-bit offset)
    const auto q_res = target_player_ali_quadrant_angle_from_ram(ram, kEntAddr);
    CHECK(q_res.direction == kTargetDirRight);
    CHECK(q_res.distance == 120U); // |+100| + |-20|

    // 2. 0x00E3E8 from RAM
    const CardinalMotionTable table = {{
        {0, -0x00010000},
        {0x00018000, 0},
        {0, 0x00010000},
        {-0x00018000, 0},
    }};
    const auto tbl_res = target_player_ali_and_load_motion_table_from_ram(ram, 0x1AA4U, table);
    CHECK(tbl_res.direction == kTargetDirRight);
    CHECK(tbl_res.table_byte_offset == 8U);
    CHECK(tbl_res.motion_timer == 100U);
    CHECK(read_be16(ram, kEntOff + kTargetEntityFacingOffset) == kTargetDirRight);
    CHECK(read_be16(ram, kEntOff + kTargetEntityTimerOffset) == 100U);
    CHECK(read_be32_signed(ram, kEntOff + kTargetEntityVelXFpOffset) == 0x00018000);
    CHECK(read_be32_signed(ram, kEntOff + kTargetEntityVelYFpOffset) == 0);

    // 3. 0x00E45E from RAM (zero-fallback preserves stored facing +$16 = Right)
    const auto aim_zero = normalize_target_aim_vector_from_ram(ram, kEntAddr, 200, 140);
    CHECK(aim_zero.vel_x_fp == 0 && aim_zero.vel_y_fp == 0);
    CHECK(aim_zero.quadrant == kTargetDirRight);
    CHECK(aim_zero.half_span == 0U);

    const auto aim_hit = normalize_target_aim_vector_from_ram(ram, kEntAddr, 240, 130);
    CHECK(aim_hit.half_span == 20U);
    CHECK(aim_hit.quadrant == kTargetDirRight);
    CHECK(aim_hit.vel_x_fp == 0x00020000);
    CHECK(aim_hit.vel_y_fp == -0x00008000);

    // 4. 0x00E4D0 from RAM: suppressed first (preserves +$4E..+$56), then launched (NE)
    const auto sup = launch_directional_projectile_physics_from_ram(ram, kEntAddr, 0x8000U);
    CHECK(!sup.launched);
    CHECK(read_be32_signed(ram, kEntOff + kTargetEntityVelXFpOffset) == 0x00018000);

    const auto launched = launch_directional_projectile_physics_from_ram(ram, kEntAddr, 0x0000U);
    CHECK(launched.launched);
    const TargetingEntityState after = load_targeting_entity_state_from_ram(ram, kEntAddr);
    CHECK(after.vel_x_fp == 0x00024000);
    CHECK(after.vel_y_fp == -0x00022800);
    CHECK(after.vel_z_fp == 0x00060000);
    CHECK(ram[kEntOff + 0x0AU] == 0xA5U);
    CHECK(ram[kEntOff + 0x20U] == 0xA5U);

    // 5. Out-of-bounds / short RAM buffer guards
    std::array<std::uint8_t, 64> short_ram{};
    CHECK(load_targeting_entity_state_from_ram(short_ram, kEntAddr) == TargetingEntityState{});
    CHECK(target_player_ali_quadrant_angle_from_ram(ram, 0x00FE0000U) == TargetResult{});
    CHECK(target_player_ali_and_load_motion_table_from_ram(ram, 0xFFF0U, table) == MotionTableLoadResult{});
    CHECK(normalize_target_aim_vector_from_ram(ram, 0xFFF0U, 0, 0) == NormalizedAimVector{});
    CHECK(!launch_directional_projectile_physics_from_ram(ram, 0xFFF0U, 0U).launched);
}

} // namespace

int main() {
    test_constants_and_aliases();
    test_target_player_ali_quadrant_angle();
    test_target_player_ali_and_load_motion_table();
    test_normalize_target_aim_vector_divs();
    test_launch_directional_projectile_physics();
    test_ram_window_integration();

    std::cout << "[PASS] All M16.198 player targeting tests succeeded!\n";
    return 0;
}
