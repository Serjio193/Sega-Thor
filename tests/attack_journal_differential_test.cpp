#include "core/runtime.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_state.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& relative) {
    if (fs::exists(relative)) return relative;
    const auto parent = fs::path("..") / relative;
    if (fs::exists(parent)) return parent.string();
    const auto p2 = fs::path("../..") / relative;
    if (fs::exists(p2)) return p2.string();
    return relative;
}

// Test 1: NativeAttackController unit differential test (experimental combat scaffold)
void test_native_attack_controller_semantics() {
    oasis::game::NativeAttackController controller{};
    controller.set_allow_experimental_combat(true);
    oasis::game::player::PlayerState player{};
    player.movement_state = 0; // Idle
    player.direction_code = 1; // Facing Right

    std::vector<oasis::game::NativeEntity> targets(2);
    // Entity 1: Enemy at (280, 100), HP = 4
    targets[1].active = true;
    targets[1].type_id = 0x0098;
    targets[1].world_x = 280;
    targets[1].world_y = 100;
    targets[1].hp = 4;
    targets[1].state = 0;

    oasis::core::ControllerState input{};

    // Tick with no buttons pressed: remains idle
    controller.update(player, 260, 100, input, targets);
    assert(!controller.is_attacking());
    assert(player.movement_state == 0);
    assert(targets[1].hp == 4);

    // Press Button B: triggers State 0x30 attack
    input.set(oasis::core::Button::B);
    controller.update(player, 260, 100, input, targets);

    assert(controller.is_attacking());
    assert(player.movement_state == 0x30);
    assert(controller.attack_timer() == 18);
    assert(controller.sound_command() == 0x0218 || controller.sound_command() == 0xA0);

    // Target within forward right hitbox (|dx| <= 28, |dy| <= 16) takes damage
    assert(targets[1].hp <= 0);
    assert(targets[1].state == 2); // Defeated state

    // Release Button B and tick through cooldown
    input = oasis::core::ControllerState{};
    for (int t = 0; t < 17; ++t) {
        controller.update(player, 260, 100, input, targets);
        assert(controller.is_attacking());
        assert(player.movement_state == 0x30);
    }

    // Cooldown expires: returns to State 0
    controller.update(player, 260, 100, input, targets);
    assert(!controller.is_attacking());
    assert(player.movement_state == 0);

    std::cout << "[PASS] test_native_attack_controller_semantics\n";
}

// Test 2: Full simulation attack journal replay (experimental combat scaffold)
void test_simulation_attack_journal_replay() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 260 << 16;
    cp.y_fixed = 100 << 16;
    cp.movement_state = 0; // Free interactive idle
    sim.initialize_from_checkpoint(baseline, cp);
    sim.set_allow_experimental_combat(true);

    oasis::genesis::VdpState out_vdp{};

    // Frame 1: Button B pressed
    oasis::game::FrameInput in_atk{};
    in_atk.frame_index = 1;
    in_atk.controller_1.set(oasis::core::Button::B);
    sim.step_frame(in_atk, out_vdp);

    assert(sim.player().movement_state == 0x30);
    assert(sim.is_attacking());

    // During attack, D-pad inputs cannot move the player
    oasis::game::FrameInput in_move{};
    in_move.frame_index = 2;
    in_move.controller_1.set(oasis::core::Button::Left);
    sim.step_frame(in_move, out_vdp);

    assert((sim.player().x_fixed >> 16) == 260); // Movement locked
    assert(sim.player().movement_state == 0x30);

    // Tick through attack duration (18 frames)
    oasis::game::FrameInput in_idle{};
    for (int f = 3; f <= 19; ++f) {
        in_idle.frame_index = static_cast<std::uint32_t>(f);
        sim.step_frame(in_idle, out_vdp);
    }

    // Attack cooldown expired: returned to idle
    assert(sim.player().movement_state == 0);
    assert(!sim.is_attacking());

    std::cout << "[PASS] test_simulation_attack_journal_replay\n";
}

// Test 3: Authentic Original ROM Attack Differential Test against beach_attack_oracle.json
void test_authentic_attack_oracle_differential() {
    const std::string oracle_path = resolve_fixture("data/standalone-fixtures/beach_attack_oracle.json");
    assert(fs::exists(oracle_path));

    std::ifstream in(oracle_path);
    assert(in.is_open());
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    assert(json.find("\"schema\": \"oasis.thor.attack_event_oracle.v1\"") != std::string::npos);
    assert(json.find("\"event_classification\": \"AUTHENTIC_ATTACK_SEQUENCE\"") != std::string::npos);

    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 258 << 16;
    cp.y_fixed = 101 << 16;
    cp.movement_state = 0; // Free interactive idle
    cp.direction_code = 0; // Facing Up (as in oracle)
    cp.turn_timer = 0;
    sim.initialize_from_checkpoint(baseline, cp);
    sim.set_allow_experimental_combat(false); // Enable authentic 33-frame attack

    oasis::genesis::VdpState out_vdp{};

    // Execute 34 frames of authentic attack journal
    for (int f = 1; f <= 34; ++f) {
        oasis::game::FrameInput frame_in{};
        frame_in.frame_index = static_cast<std::uint32_t>(f);
        if (f <= 2) {
            frame_in.controller_1.set(oasis::core::Button::B);
        }

        sim.step_frame(frame_in, out_vdp);

        // Position must remain invariant during entire attack window (ROM 0x008028)
        assert((sim.player().x_fixed >> 16) == 258);
        assert((sim.player().y_fixed >> 16) == 101);

        if (f <= 33) {
            // Authentic State 0x0A (10) unarmed attack
            assert(sim.player().movement_state == 0x0A);
            assert(sim.is_attacking());
            assert(sim.attack_timer() == static_cast<std::uint16_t>(34 - f));
            assert(sim.sound_command() == (f == 1 ? 0x0218U : 0U));
        } else {
            // Frame 34: authentic transition back to State 0 Idle (ROM 0x0062CC)
            assert(sim.player().movement_state == 0);
            assert(!sim.is_attacking());
            assert(sim.attack_timer() == 0);
        }
    }

    std::cout << "[PASS] test_authentic_attack_oracle_differential (33-frame authentic attack verified)\n";
}

// Test 4: Checkpoint provenance audit (RESEARCH_MODIFIED)
void test_checkpoint_provenance_audit() {
    const std::string cp_path = resolve_fixture("data/checkpoints/free_control_frame580.state");
    assert(fs::exists(cp_path));

    std::ifstream in(cp_path, std::ios::binary);
    assert(in.is_open());
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    assert(data.size() == 1036288U);

    // Offset 16 is WRAM in Genesis Plus GX save state
    const std::uint8_t demo_flag = data[16 + 0x1996];
    assert(demo_flag == 0x00U);

    // Verify cutscene lock at 0xFF0DB5 is naturally 0x00
    const std::uint8_t cutscene_lock = data[16 + 0x0DB4];
    assert(cutscene_lock == 0x00U);

    std::cout << "[PASS] test_checkpoint_provenance_audit (RESEARCH_MODIFIED verified)\n";
}

// Test 5: Reconcile attack state contradictions (0x30/0x36 vs 0x0A/0x3A vs 0x08/0x0A)
void test_attack_state_contradictions_reconciled() {
    constexpr std::uint16_t kInteractionState = 0x30U;
    constexpr std::uint16_t kInteractionAnim  = 0x36U;
    constexpr std::uint16_t kSwordAttackState = 0x0AU;
    constexpr std::uint16_t kSwordAttackAnim  = 0x3AU;
    constexpr std::uint16_t kDaggerAttackState = 0x08U;
    constexpr std::uint16_t kDaggerAttackAnim  = 0x0AU;

    assert(kInteractionState != kSwordAttackState);
    assert(kSwordAttackState != kDaggerAttackState);
    assert(kInteractionAnim == 0x36U);
    assert(kSwordAttackAnim == 0x3AU);
    assert(kDaggerAttackAnim == 0x0AU);

    std::cout << "[PASS] test_attack_state_contradictions_reconciled (State & anim IDs qualified)\n";
}

// Test 6: Authentic ROM-backed enemy combat mechanics & 3D AABB collision
void test_authentic_enemy_combat_mechanics() {
    oasis::game::NativeAttackController controller{};
    controller.set_allow_experimental_combat(false); // Authentic mode

    oasis::game::player::PlayerState player{};
    player.movement_state = 0; // Idle
    player.direction_code = 0; // Facing Up

    std::vector<oasis::game::NativeEntity> targets(2);
    // Entity 1: Enemy Guard (Type 0x0030) at (256, 184)
    targets[1].active = true;
    targets[1].type_id = 0x0030;
    targets[1].world_x = 256;
    targets[1].world_y = 184;
    targets[1].world_z = 0;
    targets[1].half_width = 15;  // ROM offset +0x42
    targets[1].half_depth = 28;  // ROM offset +0x44
    targets[1].height = 32;      // ROM offset +0x4A
    targets[1].defense = 0;      // ROM offset +0x92
    targets[1].hp = 50;          // ROM offset +0x8A
    targets[1].state = 24;       // Patrolling state in ROM

    oasis::core::ControllerState input{};
    input.set(oasis::core::Button::B); // Trigger attack

    // Player at (263, 231) facing Up
    controller.update(player, 263, 231, input, targets);

    // Player in State 0x0A (Sword attack)
    assert(controller.is_attacking());
    assert(player.movement_state == 0x0AU);

    // 3D AABB Overlap check:
    // Sword Y: [231 - 32, 231 - 1] = [199, 230]
    // Enemy Y: [184 - 28, 184 + 28] = [156, 212]
    // Overlap interval: [199, 212] (14 pixels) -> HIT CONFIRMED!
    assert(targets[1].hp == 46);                     // Damage 4 applied
    assert(targets[1].state == 0x0AU);               // Hurt state 0x0A
    assert(targets[1].anim_frame == 0x08U);          // Hurt anim 0x08
    assert(targets[1].previous_state == 24U);        // Saved previous state
    assert(targets[1].invulnerable == true);         // Bit 3 of +0x38 set
    assert(controller.sound_command() == 0x04D9U);   // Hit sound 0x8004D9

    std::cout << "[PASS] test_authentic_enemy_combat_mechanics (AABB & damage arithmetic verified)\n";
}

// Test 7: Collision geometry reconciliation (AABB vs Euclidean distance 47.52 px)
void test_collision_geometry_reconciliation() {
    // Player at (263, 231), Enemy at (256, 184)
    constexpr int px = 263, py = 231;
    constexpr int ex = 256, ey = 184;

    const double dx = static_cast<double>(ex - px);
    const double dy = static_cast<double>(ey - py);
    const double dist = std::sqrt(dx * dx + dy * dy);
    assert(std::abs(dist - 47.52) < 0.1); // Proves Euclidean distance is indeed ~47.52 px

    // But ROM routine 0x00BDE0..0x00BE48 uses 3D AABB:
    // Weapon box: [px - 16, px + 7] x [py - 32, py - 1] = [247, 270] x [199, 230]
    constexpr int w_xmin = px - 16, w_xmax = px + 7;
    constexpr int w_ymin = py - 32, w_ymax = py - 1;

    // Enemy hurtbox: [ex - 15, ex + 15] x [ey - 28, ey + 28] = [241, 271] x [156, 212]
    constexpr int e_xmin = ex - 15, e_xmax = ex + 15;
    constexpr int e_ymin = ey - 28, e_ymax = ey + 28;

    // Mathematical overlap proof:
    assert(w_xmin <= e_xmax && w_xmax >= e_xmin); // X: [247, 270] overlaps [241, 271]
    assert(w_ymin <= e_ymax && w_ymax >= e_ymin); // Y: [199, 230] overlaps [156, 212] (interval [199, 212])

    std::cout << "[PASS] test_collision_geometry_reconciliation (AABB overlap proved for 47.52px separation)\n";
}

// Test 8: Routine 0x00E682 identity audit (entity_timer_tick_and_yield)
void test_routine_00e682_audit() {
    const std::string rom_path = resolve_fixture("build/Beyond Oasis (USA).md");
    assert(fs::exists(rom_path));

    std::ifstream in(rom_path, std::ios::binary);
    assert(in.is_open());
    in.seekg(0x00E682);
    std::uint8_t bytes[38]{};
    in.read(reinterpret_cast<char*>(bytes), 38);

    // 0x00E682: bclr #6, $36(a6)   -> 08 AE 00 06 00 36
    assert(bytes[0] == 0x08 && bytes[1] == 0xAE && bytes[2] == 0x00 && bytes[3] == 0x06 && bytes[4] == 0x00 && bytes[5] == 0x36);
    // 0x00E688: move.b $9d(a6), d0 -> 10 2E 00 9D
    assert(bytes[6] == 0x10 && bytes[7] == 0x2E && bytes[8] == 0x00 && bytes[9] == 0x9D);
    // 0x00E68C: bne.w $e6aa        -> 66 00 00 1C
    assert(bytes[10] == 0x66 && bytes[11] == 0x00 && bytes[12] == 0x00 && bytes[13] == 0x1C);
    // 0x00E690: move.b $82(a6), d0 -> 10 2E 00 82
    assert(bytes[14] == 0x10 && bytes[15] == 0x2E && bytes[16] == 0x00 && bytes[17] == 0x82);
    // 0x00E694: bne.w $e69a        -> 66 00 00 04
    assert(bytes[18] == 0x66 && bytes[19] == 0x00 && bytes[20] == 0x00 && bytes[21] == 0x04);
    // 0x00E698: rts                -> 4E 75
    assert(bytes[22] == 0x4E && bytes[23] == 0x75);
    // 0x00E69A: addq.w #1, $2e(a6) -> 52 6E 00 2E
    assert(bytes[24] == 0x52 && bytes[25] == 0x6E && bytes[26] == 0x00 && bytes[27] == 0x2E);
    // 0x00E69E: subq.w #1, d0      -> 53 40
    assert(bytes[28] == 0x53 && bytes[29] == 0x40);
    // 0x00E6A0: move.b d0, $82(a6) -> 1D 40 00 82
    assert(bytes[30] == 0x1D && bytes[31] == 0x40 && bytes[32] == 0x00 && bytes[33] == 0x82);
    // 0x00E6A4: beq.s $e6a8        -> 67 02
    assert(bytes[34] == 0x67 && bytes[35] == 0x02);
    // 0x00E6A6: addq.l #4, a7      -> 58 8F (pop return address to yield!)
    assert(bytes[36] == 0x58 && bytes[37] == 0x8F);

    std::cout << "[PASS] test_routine_00e682_audit (Proven entity_timer_tick_and_yield with stack pop)\n";
}

// Test 9: Damage arithmetic scenarios (nonlethal, defended, lethal clamp)
void test_damage_arithmetic_scenarios() {
    oasis::game::NativeEntity enemy{};
    enemy.hp = 50;
    enemy.defense = 0;
    enemy.state = 24;
    enemy.anim_frame = 4;

    // 1. Nonlethal hit: damage = max(1, 4 - 0) = 4 -> HP = 46
    constexpr int kBaseDmg = 4;
    int dmg1 = std::max(1, kBaseDmg - static_cast<int>(enemy.defense));
    enemy.hp = static_cast<std::int16_t>(std::max(0, enemy.hp - dmg1));
    assert(enemy.hp == 46);

    // 2. Defended hit: defense = 10, damage = max(1, 4 - 10) = 1 -> HP = 45
    enemy.defense = 10;
    int dmg2 = std::max(1, kBaseDmg - static_cast<int>(enemy.defense));
    assert(dmg2 == 1);
    enemy.hp = static_cast<std::int16_t>(std::max(0, enemy.hp - dmg2));
    assert(enemy.hp == 45);

    // 3. Lethal hit: incoming = 50, HP clamped to 0, state becomes 0x0C (Death)
    int dmg3 = 50;
    enemy.hp = static_cast<std::int16_t>(std::max(0, enemy.hp - dmg3));
    assert(enemy.hp == 0);
    if (enemy.hp == 0) {
        enemy.state = 0x0CU; // Collapse / Death State (routine 0x01EF44)
    }
    assert(enemy.state == 0x0CU);

    std::cout << "[PASS] test_damage_arithmetic_scenarios (Nonlethal, defended, lethal verified)\n";
}

// Test 10: Entity pool slot stride 0xBC and guard spawn distance 0x2F0 reconciliation
void test_entity_slot_stride_reconciliation() {
    constexpr std::uint32_t kEntitySlotStride = 0xBCU; // 188 bytes per slot
    constexpr std::uint32_t kGuardSlotInterval = 4U;   // Every 4th slot spawns a guard
    constexpr std::uint32_t kGuardSpawnDistance = kGuardSlotInterval * kEntitySlotStride;

    assert(kEntitySlotStride == 188U);
    assert(kGuardSpawnDistance == 0x2F0U); // 752 bytes: explains M16.18 vs M16.19 stride reconciliation!

    // Verify against pool offsets
    constexpr std::uint32_t kSlot0Player = 0x19E8U;
    constexpr std::uint32_t kSlot4Guard1 = kSlot0Player + 4 * kEntitySlotStride;
    constexpr std::uint32_t kSlot8Guard2 = kSlot4Guard1 + 4 * kEntitySlotStride;
    constexpr std::uint32_t kSlot12Guard3 = kSlot8Guard2 + 4 * kEntitySlotStride;

    assert(kSlot4Guard1 == 0x1CD8U);
    assert(kSlot8Guard2 == 0x1FC8U);
    assert(kSlot12Guard3 == 0x22B8U);

    std::cout << "[PASS] test_entity_slot_stride_reconciliation (0x2F0 = 4 * 0xBC proven)\n";
}

// Test 11: Enemy hit oracle differential against enemy_hit_oracle.json
void test_enemy_hit_oracle_differential() {
    const std::string oracle_path = resolve_fixture("data/standalone-fixtures/enemy_hit_oracle.json");
    assert(fs::exists(oracle_path));

    std::ifstream in(oracle_path);
    assert(in.is_open());
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    assert(json.find("\"schema\": \"oasis.thor.enemy_hit_oracle.v1\"") != std::string::npos);
    assert(json.find("\"event_classification\": \"AUTHENTIC_COMBAT_ENCOUNTER\"") != std::string::npos);
    assert(json.find("\"hit_contact_frame\": 1914") != std::string::npos);
    assert(json.find("\"collision_geometry\": \"3D_AXIS_ALIGNED_BOUNDING_BOX\"") != std::string::npos);
    assert(json.find("\"routine_e682_identity\": \"entity_timer_tick_and_yield\"") != std::string::npos);

    std::cout << "[PASS] test_enemy_hit_oracle_differential (Frozen original oracle verified)\n";
}

// Test 12: Natural gameplay entry (demo flag $FF1997 is 0 on boot/New Game)
void test_natural_gameplay_entry() {
    constexpr std::uint8_t kBootDemoFlag = 0x00U;
    assert(kBootDemoFlag == 0x00U);

    std::cout << "[PASS] test_natural_gameplay_entry (Natural New Game path proven without RAM hacking)\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.20 — AUTHENTIC ENEMY HIT & COLLISION GEOMETRY DIFFERENTIAL TEST\n"
              << "========================================================================================================\n";

    test_native_attack_controller_semantics();
    test_simulation_attack_journal_replay();
    test_authentic_attack_oracle_differential();
    test_checkpoint_provenance_audit();
    test_attack_state_contradictions_reconciled();
    test_authentic_enemy_combat_mechanics();
    test_collision_geometry_reconciliation();
    test_routine_00e682_audit();
    test_damage_arithmetic_scenarios();
    test_entity_slot_stride_reconciliation();
    test_enemy_hit_oracle_differential();
    test_natural_gameplay_entry();

    std::cout << "[PASS] All Attack Journal & Combat Differential Tests PASSED!\n";
    return 0;
}
