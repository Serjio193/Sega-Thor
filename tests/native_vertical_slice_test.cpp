#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "core/runtime.hpp"
#include "game/controlled_screen.hpp"
#include "game/render/framebuffer.hpp"
#include "game/world/byte_grid.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace {

using oasis::core::Button, oasis::core::ControllerState, oasis::core::InputSnapshot;
using oasis::game::player::Direction, oasis::game::player::PlayerMovementConfig, oasis::game::player::PlayerState, oasis::game::player::try_move;
using oasis::game::render::SoftwareFramebuffer;
using oasis::game::screen::ControlledScreen, oasis::game::screen::ScreenState;
using oasis::game::world::ByteGridView;

ControllerState controller_for(std::uint8_t nibble) {
    ControllerState result{}; result.buttons = nibble & 0x0FU; return result;
}

std::uint8_t replay_input(std::size_t frame) {
    constexpr std::array<std::uint8_t, 16> inputs{
        0x0, 0x1, 0x2, 0x4, 0x8, 0x5, 0x6, 0x9,
        0xA, 0x3, 0x7, 0xB, 0xD, 0xE, 0xF, 0x0,
    };
    return inputs[frame % inputs.size()];
}

std::vector<ScreenState> run_replay(std::size_t render_interval,
                                    std::string* framebuffer_hash) {
    ControlledScreen screen;
    oasis::core::RuntimeLoop runtime(screen);
    SoftwareFramebuffer framebuffer;
    std::vector<ScreenState> states;
    states.reserve(600);
    for (std::size_t frame = 0; frame < 600; ++frame) {
        runtime.step(InputSnapshot{controller_for(replay_input(frame)), {}});
        states.push_back(screen.state());
        if (frame % render_interval == 0) screen.render(framebuffer.pixels());
    }
    screen.render(framebuffer.pixels());
    if (framebuffer_hash != nullptr) {
        std::vector<std::uint8_t> canonical_bytes;
        canonical_bytes.reserve(framebuffer.pixels().size() * 4U);
        for (const auto pixel : framebuffer.pixels()) {
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 24U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 16U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 8U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel));
        }
        *framebuffer_hash = oasis::calculate_sha256(canonical_bytes);
    }
    assert(runtime.frame_index() == 600);
    return states;
}

} // namespace

int main() {
    ControlledScreen screen;
    const auto fixture = screen.fixture_cells();
    assert(fixture.size() == ControlledScreen::kGridWidth * ControlledScreen::kGridHeight);
    assert(fixture[6U * ControlledScreen::kGridWidth + 20U] == 0x05U);
    assert(fixture[18U * ControlledScreen::kGridWidth + 20U] == 0x05U);
    assert(fixture[10U * ControlledScreen::kGridWidth + 29U] == 0x05U);

    const auto config = screen.movement_config();
    assert(config.footprint_radius != 0);
    assert(config.cardinal_x_speed == 0x36000);
    assert(config.diagonal_x_speed == 0x2A000);
    assert(config.diagonal_y_speed == 0x25800);

    constexpr std::array<Direction, 16> directions{
        Direction::none, Direction::up, Direction::down, Direction::none,
        Direction::left, Direction::up_left, Direction::down_left, Direction::left,
        Direction::right, Direction::up_right, Direction::down_right, Direction::right,
        Direction::none, Direction::up, Direction::down, Direction::none,
    };
    for (std::uint8_t nibble = 0; nibble < directions.size(); ++nibble) {
        assert(oasis::game::player::direction_from_input(nibble) == directions[nibble]);
    }

    std::array<std::uint8_t, 32U * 28U> terrain_bytes{};
    terrain_bytes.fill(0x02U);
    terrain_bytes[18U * 32U + 20U] = 0x05U;
    const ByteGridView terrain(terrain_bytes, 32, 28, 32, 5);
    PlayerMovementConfig collision_config = config;
    collision_config.footprint_radius = 0;

    PlayerState free_player{48 * 0x10000, 48 * 0x10000, 2};
    assert(try_move(free_player, controller_for(0x8), terrain, collision_config).moved && free_player.x_fixed == 48 * 0x10000 + 0x36000);
    PlayerState wall_player{19 * 8 * 0x10000 + 7 * 0x10000, 17 * 8 * 0x10000 + 7 * 0x10000, 2};
    assert(!try_move(wall_player, controller_for(0xA), terrain, collision_config).moved);
    PlayerState boundary_player{0, 0, 2};
    assert(!try_move(boundary_player, controller_for(0x5), terrain, collision_config).moved && boundary_player.x_fixed == 0);
    PlayerState footprint_player{19 * 8 * 0x10000, 18 * 8 * 0x10000, 2}; collision_config.footprint_radius = 6;
    assert(!try_move(footprint_player, controller_for(0x8), terrain, collision_config).moved);

    std::string first_hash, second_hash, third_hash;
    const auto first = run_replay(1, &first_hash);
    const auto second = run_replay(7, &second_hash);
    const auto third = run_replay(31, &third_hash);
    assert(first == second && second == third && first_hash == second_hash && second_hash == third_hash);
    assert(first_hash == "3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da");
    assert(first.back().frame_index == 599 && (first.back().player.movement_state == 0 || first.back().player.movement_state == 2));

    const std::filesystem::path local_rom = "local-roms/Beyond Oasis (USA).md";
    if (std::filesystem::exists(local_rom)) {
        const auto rom = oasis::Rom::load(local_rom);
        ControlledScreen rom_screen(rom.bytes());
        assert(rom_screen.is_rom_backed());
        assert(rom_screen.screen_setup_id() == 0x0100);
        assert(rom_screen.descriptor_address() == 0x02E57E);
        assert(rom_screen.primary_resource_address() == 0x00211F7A);
        const auto res_ids = rom_screen.gfx_resource_ids();
        assert((res_ids == std::array<std::uint8_t, 4>{13, 14, 15, 16}));
        assert(rom_screen.state().player.x_fixed == 160 * 0x10000);
        assert(rom_screen.state().player.y_fixed == 160 * 0x10000);

        // Entity validation for Screen A
        assert(rom_screen.entity_count() == 2);
        const auto& ents_a = rom_screen.entities();
        assert(ents_a.size() == 2);
        assert(ents_a[0].type_id() == 0x002A);
        assert(ents_a[0].world_x() == 192 && ents_a[0].world_y() == 160);
        assert(ents_a[0].descriptor().gfx_descriptor_pointer == 0x167DAC);
        assert(!ents_a[0].pieces().empty());
        assert(ents_a[0].active());

        // Initial animation state on spawn
        const auto& anim_init = ents_a[0].anim_state();
        assert(anim_init.script_address == 0x00041052 && anim_init.cursor == 0 && anim_init.current_frame_index == 0);
        assert(anim_init.current_frame_pointer == 0x167DB2 && anim_init.ticks_remaining == 4 && anim_init.loop_count == 0 && !anim_init.halted);

        oasis::core::RuntimeLoop loop(rom_screen);
        SoftwareFramebuffer fb;

        // Step ticks to verify animation transitions across frames 0 -> 1 -> 2 -> loop 0
        loop.step(InputSnapshot{});
        assert(ents_a[0].anim_state().ticks_remaining == 3 && ents_a[0].anim_state().current_frame_index == 0);
        loop.step(InputSnapshot{}); loop.step(InputSnapshot{}); loop.step(InputSnapshot{});
        assert(ents_a[0].anim_state().cursor == 4 && ents_a[0].anim_state().current_frame_index == 1 && ents_a[0].anim_state().current_frame_pointer == 0x167DE4);
        for (int i = 0; i < 4; ++i) loop.step(InputSnapshot{});
        assert(ents_a[0].anim_state().cursor == 8 && ents_a[0].anim_state().current_frame_index == 2 && ents_a[0].anim_state().loop_pending);
        for (int i = 0; i < 4; ++i) loop.step(InputSnapshot{});
        assert(ents_a[0].anim_state().cursor == 0 && ents_a[0].anim_state().current_frame_index == 0 && ents_a[0].anim_state().loop_count == 1);

        // Player movement & collision checks
        loop.step(InputSnapshot{controller_for(0x2), {}});
        assert(rom_screen.state().player.y_fixed > 160 * 0x10000);
        assert(rom_screen.state().movement.moved && !rom_screen.state().movement.blocked);

        loop.step(InputSnapshot{controller_for(0x4), {}});
        assert(rom_screen.state().movement.blocked);

        assert(rom_screen.debug_overlay_visible());
        rom_screen.toggle_debug_overlay();
        assert(!rom_screen.debug_overlay_visible());
        rom_screen.toggle_debug_overlay();

        // 1. SCREEN_A_RENDERED = YES with real entity pixels
        SoftwareFramebuffer fb_a;
        rom_screen.render(fb_a.pixels());
        assert(fb_a.pixels()[0] != 0);

        // Verify entity transforms with camera:
        const auto cam_x_initial = rom_screen.camera_x();
        const auto cam_y_initial = rom_screen.camera_y();
        const auto ent_screen_x_initial = ents_a[0].world_x() - cam_x_initial;
        const auto ent_screen_y_initial = ents_a[0].world_y() - cam_y_initial;
        assert(ent_screen_x_initial >= 0 && ent_screen_x_initial < 320);
        assert(ent_screen_y_initial >= 0 && ent_screen_y_initial < 224);

        // 2. TRANSITION_OCCURRED = YES
        assert(rom_screen.transition_count() == 0);
        assert(rom_screen.current_screen_setup_id() == 0x0100);
        const bool transition_ok = rom_screen.trigger_screen_transition({1, 1}, false);
        assert(transition_ok);
        assert(rom_screen.transition_count() == 1);
        assert(rom_screen.previous_screen_setup_id() == 0x0100);
        assert(rom_screen.current_screen_setup_id() == 0x0101);
        assert(rom_screen.descriptor_address() == 0x02E612);
        assert(rom_screen.primary_resource_address() == 0x00212684);

        // Entity validation on Screen B: old entities retired, Screen B entities active with fresh animation
        assert(rom_screen.entity_count() == 3);
        const auto& ents_b = rom_screen.entities();
        assert(ents_b.size() == 3);
        assert(ents_b[0].world_x() == 224 && ents_b[0].world_y() == 144 && ents_b[1].world_x() == 160 && ents_b[1].world_y() == 200);
        assert(ents_b[0].type_id() == 0x002A && ents_b[1].type_id() == 0x002A);
        assert(ents_b[2].type_id() == 0x004C && ents_b[2].is_flame());
        for (std::size_t i = 0; i < 2; ++i) {
            const auto& eb = ents_b[i];
            assert(!(eb.world_x() == 192 && eb.world_y() == 160) && eb.anim_state().cursor == 0 && eb.anim_state().loop_count == 0);
            assert(eb.anim_state().current_frame_index == 0 && eb.anim_state().current_frame_pointer == 0x167DB2 && !eb.anim_state().halted);
        }

        // 3. SCREEN_B_RENDERED = YES
        SoftwareFramebuffer fb_b;
        rom_screen.render(fb_b.pixels());
        assert(fb_b.pixels()[0] != 0);

        // 4. SCREEN_B_COLLISION_ACTIVE = YES & regression proving old grid is not active
        const auto raw_cell_b = rom_screen.terrain().sample_world(204, 164);
        assert(rom_screen.previous_screen() != nullptr);
        const auto raw_cell_a = rom_screen.previous_screen()->terrain.sample_world(204, 164);
        assert(raw_cell_b.has_value() && *raw_cell_b == 0x76 && raw_cell_a.has_value() && *raw_cell_a == 0xE1);

        // 5. PLAYER_CONTINUES_CONTROL = YES
        const auto y_before = rom_screen.state().player.y_fixed;
        loop.step(InputSnapshot{controller_for(0x2), {}}); // Down
        assert(rom_screen.state().player.y_fixed > y_before && rom_screen.state().movement.moved && !rom_screen.state().movement.blocked);
        assert(rom_screen.entities()[0].active());

        // 6. PROCESS_RESTART_REQUIRED = NO: transition back to Screen A
        assert(rom_screen.trigger_screen_transition({1, 0}, false) && rom_screen.transition_count() == 2);
        assert(rom_screen.previous_screen_setup_id() == 0x0101 && rom_screen.current_screen_setup_id() == 0x0100);
        assert(rom_screen.descriptor_address() == 0x02E57E && rom_screen.primary_resource_address() == 0x00211F7A);

        // Entity reload on return: Screen A entity restored with fresh animation
        assert(rom_screen.entity_count() == 2 && rom_screen.entities()[0].world_x() == 192 && rom_screen.entities()[0].world_y() == 160);
        assert(rom_screen.entities()[0].anim_state().cursor == 0 && rom_screen.entities()[0].anim_state().current_frame_index == 0);

        const auto y_return_before = rom_screen.state().player.y_fixed;
        loop.step(InputSnapshot{controller_for(0x1), {}}); // Up
        assert(rom_screen.state().player.y_fixed < y_return_before);
        assert(rom_screen.state().movement.moved && !rom_screen.state().movement.blocked);
        loop.step(InputSnapshot{controller_for(0x2), {}}); // Down back to y=160
        assert(rom_screen.state().player.y_fixed == y_return_before);

        // 7. Fail-closed script decoder regression
        auto bad_entity = oasis::game::entities::ScreenEntity::spawn_from_rom(
            rom.bytes(), 0x002A, 100, 100);
        bad_entity.set_anim_script(0x00167E48); // Compressed tile data, invalid sequence header
        bad_entity.update(rom.bytes());
        assert(bad_entity.anim_state().halted);

        // 8. Hitbox derivation and 4-side static chest blocking
        const auto hb = rom_screen.entities()[0].hitbox();
        assert(hb.left == 180 && hb.right == 204 && hb.top == 150 && hb.bottom == 170);

        // Direct hitbox arithmetic validation across all 4 sides and diagonals
        assert(oasis::game::entities::player_hitbox(171, 160).overlaps(hb) && !oasis::game::entities::player_hitbox(169, 160).overlaps(hb));
        assert(oasis::game::entities::player_hitbox(213, 160).overlaps(hb) && !oasis::game::entities::player_hitbox(215, 160).overlaps(hb));
        assert(oasis::game::entities::player_hitbox(192, 141).overlaps(hb) && !oasis::game::entities::player_hitbox(192, 139).overlaps(hb));
        assert(oasis::game::entities::player_hitbox(192, 179).overlaps(hb) && !oasis::game::entities::player_hitbox(192, 181).overlaps(hb));
        assert(oasis::game::entities::player_hitbox(171, 141).overlaps(hb) && !oasis::game::entities::player_hitbox(169, 139).overlaps(hb));

        // Walk Right towards chest until blocked by chest hitbox (left approach)
        for (int i = 0; i < 10; ++i) loop.step(InputSnapshot{controller_for(0x8), {}}); // Right
        assert(rom_screen.state().movement.blocked && rom_screen.state().player.x_fixed / 0x10000 <= 170 && rom_screen.overlap_detected());

        // 9. Interaction candidate acquisition
        // Facing chest (Right) in range -> target acquired and READY
        assert(rom_screen.interaction_target_type() == 0x002A && rom_screen.interaction_state_str() == "READY");

        // Face away from chest (Left) -> target cleared and IDLE
        loop.step(InputSnapshot{controller_for(0x4), {}}); // Left
        assert(rom_screen.interaction_target_type() == 0 && rom_screen.interaction_state_str() == "IDLE");

        // 10. Action button triggers authentic chest opening animation
        ControllerState act = controller_for(0x8); act.set(Button::A, true);
        loop.step(InputSnapshot{act, {}});
        assert(rom_screen.interaction_state_str() == "TRIGGERED (OPENING)");
        for (int i = 0; i < 8; ++i) loop.step(InputSnapshot{});
        assert(rom_screen.entities()[0].anim_state().current_frame_index == 2 && rom_screen.entities()[0].anim_state().halted && rom_screen.interaction_state_str() == "OPENED");
        for (int i = 0; i < 10; ++i) loop.step(InputSnapshot{});
        assert(rom_screen.entities()[0].anim_state().current_frame_index == 2 && rom_screen.interaction_state_str() == "OPENED");

        // 11. Screen transition cleanly clears collision and interaction targets
        assert(rom_screen.trigger_screen_transition({1, 1}, false));
        assert(!rom_screen.overlap_detected() && rom_screen.interaction_target_type() == 0 && rom_screen.interaction_state_str() == "IDLE");
        assert(rom_screen.entities()[0].hitbox().left == 224 - 12 && rom_screen.entities()[1].hitbox().left == 160 - 12);

        // 12. Recompiled Moving Actor State Machine (Type 0x0048)
        assert(rom_screen.trigger_screen_transition({1, 0}, false));
        assert(rom_screen.entity_count() == 2);
        const auto desc48 = oasis::game::entities::load_entity_descriptor_from_rom(rom.bytes(), 0x0048);
        assert(desc48.type_id == 0x0048 && desc48.dim_x == 8 && desc48.dim_y == 8);
        assert(desc48.gfx_descriptor_pointer == 0x165E12 && desc48.anim_descriptor_pointer == 0x040F4A && desc48.update_callback_pointer == 0x058C80);

        const auto& act0 = rom_screen.entities()[1];
        assert(act0.type_id() == 0x0048 && act0.is_actor() && act0.actor().state == 1 && act0.actor().direction == 2);
        assert(act0.actor().anchor_x == 160 && act0.actor().anchor_y == 120 && act0.world_x() == 160 && act0.world_y() == 120);
        assert(act0.actor().anim_selector == 0 && act0.pieces().size() == 2);

        // Standalone actor unit tests
        auto actor_unit = oasis::game::entities::ScreenEntity::spawn_from_rom(rom.bytes(), 0x0048, 160, 120, 0, 2);
        assert(actor_unit.actor().state == 1);
        actor_unit.update_actor_state_machine(nullptr, 100, 100);
        assert(actor_unit.actor().state == 1 && actor_unit.world_y() == 120);
        actor_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(actor_unit.actor().state == 2 && actor_unit.actor().timer == 30 && actor_unit.actor().anim_selector == 2 && actor_unit.pieces().size() == 2);
        actor_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(actor_unit.actor().state == 2 && actor_unit.actor().timer == 29 && actor_unit.world_y() == 121);
        while (actor_unit.actor().state == 2) actor_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(actor_unit.actor().state == 3 && actor_unit.actor().direction == 0 && actor_unit.actor().anim_selector == 4);
        while (actor_unit.actor().state == 3) actor_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(actor_unit.actor().state == 1 && actor_unit.world_x() == 160 && actor_unit.world_y() == 120 && actor_unit.actor().direction == 2);
        actor_unit.actor().state = 99;
        actor_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(actor_unit.actor().halted);

        // Terrain collision blocking causes immediate turn
        auto blocked_unit = oasis::game::entities::ScreenEntity::spawn_from_rom(rom.bytes(), 0x0048, 160, 120, 0, 2);
        blocked_unit.update_actor_state_machine(nullptr, 160, 160);
        assert(blocked_unit.actor().state == 2);
        std::array<std::uint8_t, 64 * 64> wall_bytes{}; wall_bytes.fill(0x05U);
        oasis::game::world::ByteGridView wall_grid(wall_bytes, 64, 64, 64, 6);
        blocked_unit.update_actor_state_machine(&wall_grid, 160, 160);
        assert(blocked_unit.actor().state == 3 && blocked_unit.actor().direction == 0);

        // 13. Player <-> Moving Enemy Contact, Damage Gate, Knockback, Invulnerability
        oasis::game::player::PlayerState p_unit{};
        p_unit.hp = 100;
        oasis::game::player::apply_enemy_contact_damage(p_unit, 0x0048, 2 /* Down */);
        assert(p_unit.hp == 90 && p_unit.damage_gate_open && p_unit.hurt_timer == 6 && p_unit.invuln_timer == 30);
        assert(p_unit.movement_state == 24 && p_unit.knockback_y_fixed == 0x30000);
        oasis::game::player::apply_enemy_contact_damage(p_unit, 0x0048, 2);
        assert(p_unit.hp == 90 && !p_unit.damage_gate_open);
        for (int f = 0; f < 6; ++f) assert(oasis::game::player::update_player_hurt_knockback(p_unit, nullptr));
        assert(!p_unit.is_hurt() && p_unit.movement_state == 0 && p_unit.knockback_y_fixed == 0);

        // Integrated screen test: Type 0x0048 charges and hits player on screen {1, 0}
        assert(rom_screen.trigger_screen_transition({1, 0}, false));
        assert(rom_screen.state().player.hp == 100 && !rom_screen.state().player.is_hurt() && !rom_screen.state().player.damage_gate_open);
        while (rom_screen.state().player.hp == 100) loop.step(InputSnapshot{});
        assert(rom_screen.state().player.hp == 90 && rom_screen.state().player.movement_state == 24);
        assert(rom_screen.state().player.hurt_timer == 6 && rom_screen.state().player.invuln_timer == 30);
        assert(rom_screen.state().player.contact_entity == 0x0048 && rom_screen.state().player.damage_gate_open && rom_screen.state().player.knockback_y_fixed == 0x30000);
        loop.step(InputSnapshot{});
        assert(rom_screen.state().player.hp == 90 && !rom_screen.state().player.damage_gate_open);
        while (rom_screen.state().player.is_hurt()) loop.step(InputSnapshot{});
        assert(rom_screen.state().player.movement_state == 0 && rom_screen.state().player.knockback_y_fixed == 0);
        assert(rom_screen.trigger_screen_transition({1, 1}, false));
        assert(rom_screen.state().player.contact_entity == 0 && !rom_screen.state().player.damage_gate_open);
        assert(rom_screen.state().player.hurt_timer == 0 && rom_screen.state().player.invuln_timer == 0);

        // 14. Player Authentic Melee Attack & Enemy Damage/Death (11 Test Cases)
        // 1. Attack input start: Button::B triggers player attack, sets State 10, locks movement
        oasis::game::player::PlayerState p_atk{};
        p_atk.direction_code = 1; // Facing Right
        assert(oasis::game::player::start_player_attack(p_atk));
        assert(p_atk.attack_state == 10 && p_atk.attack_timer == 0 && !p_atk.attack_active && p_atk.attack_direction == 1);
        assert(p_atk.is_attacking() && !p_atk.attack_hit_latched);

        for (int f = 0; f < 4; ++f) {
            assert(!p_atk.attack_active && !oasis::game::player::player_attack_hitbox(p_atk).has_value());
            oasis::game::player::update_player_attack(p_atk);
        }
        assert(p_atk.attack_active && p_atk.attack_timer == 4 && oasis::game::player::player_attack_hitbox(p_atk).has_value());
        auto enemy_miss = oasis::game::entities::ScreenEntity::spawn_from_rom(rom.bytes(), 0x0048, 100, 100, 0, 2);
        assert(!oasis::game::player::player_attack_hitbox(p_atk)->overlaps(enemy_miss.hitbox()));

        p_atk.x_fixed = 100 * 0x10000; p_atk.y_fixed = 100 * 0x10000;
        p_atk.attack_direction = 0; const auto hb_u = *oasis::game::player::player_attack_hitbox(p_atk);
        p_atk.attack_direction = 1; const auto hb_r = *oasis::game::player::player_attack_hitbox(p_atk);
        p_atk.attack_direction = 2; const auto hb_d = *oasis::game::player::player_attack_hitbox(p_atk);
        p_atk.attack_direction = 3; const auto hb_l = *oasis::game::player::player_attack_hitbox(p_atk);
        assert(hb_u.top == 60 && hb_r.right == 147 && hb_d.bottom == 147 && hb_l.left == 53);

        auto enemy_test = oasis::game::entities::ScreenEntity::spawn_from_rom(rom.bytes(), 0x0048, 160, 120, 0, 2);
        assert(enemy_test.actor().hp == 20 && !enemy_test.actor().is_hurt() && enemy_test.apply_damage(10, 0));
        p_atk.attack_hit_latched = true; p_atk.last_hit_damage = 10;
        oasis::game::player::update_player_attack(p_atk);
        assert(p_atk.attack_active && p_atk.attack_hit_latched && !enemy_test.apply_damage(10, 0) && enemy_test.actor().hp == 10);
        assert(enemy_test.actor().state == 4 && enemy_test.actor().is_hurt() && enemy_test.actor().timer == 20);
        for (int f = 0; f < 20; ++f) enemy_test.update_actor_state_machine(nullptr, 160, 160);
        assert(enemy_test.actor().state == 1 && !enemy_test.actor().is_hurt() && enemy_test.actor().hp == 10);
        while (p_atk.is_attacking()) oasis::game::player::update_player_attack(p_atk);
        assert(p_atk.attack_state == 0 && !p_atk.attack_active);
        assert(enemy_test.apply_damage(10, 0) && enemy_test.actor().hp == 0 && enemy_test.actor().state == 2 && enemy_test.actor().is_dead() && !enemy_test.active());

        // Integrated screen combat on {1, 0}: Button::B triggers melee swing damaging enemy
        assert(rom_screen.trigger_screen_transition({1, 0}, false) && rom_screen.entity_count() == 2);
        auto& screen_enemy = const_cast<oasis::game::entities::ScreenEntity&>(rom_screen.entities()[1]);
        assert(screen_enemy.type_id() == 0x0048 && screen_enemy.actor().hp == 20);
        auto& p_state = const_cast<oasis::game::player::PlayerState&>(rom_screen.state().player);
        p_state.x_fixed = 160 * 0x10000; p_state.y_fixed = 140 * 0x10000; p_state.direction_code = 0;
        ControllerState b_btn{}; b_btn.set(Button::B, true);
        loop.step(InputSnapshot{b_btn, {}});
        assert(rom_screen.state().player.is_attacking() && rom_screen.state().player.attack_state == 10);
        for (int f = 1; f < 4; ++f) loop.step(InputSnapshot{});
        assert(screen_enemy.actor().hp == 20);
        loop.step(InputSnapshot{});
        assert(screen_enemy.actor().hp == 10 && screen_enemy.actor().state == 4 && screen_enemy.actor().is_hurt());
        assert(rom_screen.state().player.attack_hit_latched && rom_screen.state().player.last_hit_damage == 10);

        // 15. Enemy Death -> Loot Drop -> Player Pickup Pipeline
        auto enemy_drop_unit = oasis::game::entities::ScreenEntity::spawn_from_rom(rom.bytes(), 0x0048, 160, 120, 0, 2);
        assert(enemy_drop_unit.apply_damage(10, 0) && !enemy_drop_unit.drop_pending());
        assert(enemy_drop_unit.apply_damage(10, 0) && enemy_drop_unit.actor().is_dead() && enemy_drop_unit.drop_pending());
        assert(enemy_drop_unit.drop_item_id() == 0x001E && enemy_drop_unit.drop_count() == 1);
        auto drop_item = oasis::game::entities::ScreenEntity::spawn_drop_from_rom(
            rom.bytes(), enemy_drop_unit.drop_item_id(), enemy_drop_unit.world_x(),
            enemy_drop_unit.world_y(), enemy_drop_unit.world_z(), enemy_drop_unit.drop_count());
        assert(drop_item.world_x() == 160 && drop_item.world_y() == 120 && drop_item.world_z() == 1);
        assert(drop_item.active() && drop_item.is_item() && drop_item.type_id() == 0x0008 && drop_item.actor().state == 2);
        assert(!drop_item.pieces().empty() && drop_item.pieces().front().tile_index == 1268);

        oasis::game::player::PlayerState p_drop{}; p_drop.hp = 75; p_drop.max_hp = 100;
        const auto distant_hb = oasis::game::entities::player_hitbox(100, 100);
        assert(!distant_hb.overlaps(drop_item.hitbox()) && p_drop.hp == 75 && p_drop.item_count == 0 && drop_item.active());
        const auto overlap_hb = oasis::game::entities::player_hitbox(160, 120);
        assert(overlap_hb.overlaps(drop_item.hitbox()));
        oasis::game::player::apply_item_pickup(p_drop, drop_item.item_id(), drop_item.item_count());
        drop_item.collect();
        assert(p_drop.hp == 100 && p_drop.last_pickup_type == 0x001E && p_drop.last_pickup_heal == 25);
        assert(p_drop.item_count == 1 && !drop_item.active());
        oasis::game::player::PlayerState p_repeat = p_drop;
        if (drop_item.active() && overlap_hb.overlaps(drop_item.hitbox())) {
            oasis::game::player::apply_item_pickup(p_repeat, drop_item.item_id(), drop_item.item_count());
        }
        assert(p_repeat.hp == 100 && p_repeat.item_count == 1 && !drop_item.active());
        assert(rom_screen.trigger_screen_transition({1, 0}, false) && rom_screen.trigger_screen_transition({1, 1}, false));
        for (const auto& e : rom_screen.entities()) assert(!e.is_item());

        // 16. Spirit Summoning & 17. Dytto Active Command Pipeline on ControlledScreen
        ControllerState a_btn{}; a_btn.set(Button::A, true);
        loop.step(InputSnapshot{a_btn, {}}); // Button A summon triggered
        assert(rom_screen.spirit().active && rom_screen.spirit().type_id == 0x0018);
        assert(rom_screen.spirit().slot == 0 && rom_screen.spirit().state == 26);
        for (int f = 0; f < 25; ++f) loop.step(InputSnapshot{});
        assert(rom_screen.spirit().state == 2); // Follow state
        loop.step(InputSnapshot{a_btn, {}}); // Tap press
        loop.step(InputSnapshot{}); // Tap release
        for (int f = 0; f < 7; ++f) loop.step(InputSnapshot{}); // Double-tap window expires -> Command 1
        assert(rom_screen.spirit().state == 14 || rom_screen.spirit().projectile_active);
        for (int f = 0; f < 30; ++f) loop.step(InputSnapshot{});
        assert(rom_screen.trigger_screen_transition({1, 0}, false));
        assert(rom_screen.spirit().active); // Spirit persists across screen transition!
        assert(rom_screen.spirit().world_x == static_cast<std::int32_t>(rom_screen.state().player.x_fixed / 0x10000));

        // 18. Dytto Water Bubble -> Flame Environment Interaction on ControlledScreen
        assert(rom_screen.trigger_screen_transition({1, 1}, false));
        assert(rom_screen.entities().size() == 3 && rom_screen.entities()[2].is_flame());
        assert(rom_screen.entities()[2].active() && rom_screen.entities()[2].flame_substate() == 0);
        auto& pl_mut = const_cast<oasis::game::player::PlayerState&>(rom_screen.state().player);
        pl_mut.x_fixed = 170 * 0x10000; pl_mut.y_fixed = 160 * 0x10000; pl_mut.direction_code = 1; pl_mut.sp = 50;
        assert(oasis::game::spirits::try_execute_dytto_command(rom_screen.spirit(), 1, pl_mut, rom.bytes()));
        for (int f = 0; f < 30; ++f) loop.step(InputSnapshot{});
        assert(rom_screen.entities()[2].flame_substate() == 0xFF);
        for (int f = 0; f < 20; ++f) loop.step(InputSnapshot{});
        assert(!rom_screen.entities()[2].active() && rom_screen.is_persistent_flag_set(1));
        assert(rom_screen.trigger_screen_transition({1, 0}, false) && rom_screen.trigger_screen_transition({1, 1}, false));
        assert(!rom_screen.entities()[2].active()); // Persistence holds across reload

        // 19. Secondary Weapon Selection + Use (Bow) on ControlledScreen
        assert(rom_screen.trigger_screen_transition({1, 0}, false));
        assert(rom_screen.entities().size() >= 2 && rom_screen.entities()[1].is_actor());
        auto& target_enemy = const_cast<oasis::game::entities::ScreenEntity&>(rom_screen.entities()[1]);
        assert(target_enemy.actor().hp == 20);

        ControllerState y_btn{}; y_btn.set(Button::Y, true);
        loop.step(InputSnapshot{y_btn, {}}); // Button::Y cycles to Bow slot 1
        assert(rom_screen.state().player.secondary_slot == 1);
        assert(rom_screen.state().player.secondary_weapon == 0x06); // Bow
        assert(rom_screen.state().player.secondary_ammo == 10);

        auto& p_sec = const_cast<oasis::game::player::PlayerState&>(rom_screen.state().player);
        p_sec.x_fixed = 160 * 0x10000; p_sec.y_fixed = 145 * 0x10000; p_sec.direction_code = 0; // Facing Up towards enemy
        ControllerState c_btn{}; c_btn.set(Button::C, true);
        loop.step(InputSnapshot{c_btn, {}}); // Button::C starts secondary attack
        assert(rom_screen.state().player.is_secondary_attacking());
        assert(rom_screen.state().player.secondary_attack_state == 14);

        for (int f = 1; f < 12; ++f) loop.step(InputSnapshot{});
        assert(rom_screen.arrow().active && rom_screen.arrow().type_id == 0x0034);
        assert(rom_screen.state().player.secondary_ammo == 9);

        loop.step(InputSnapshot{});
        assert(target_enemy.actor().hp == 14 && target_enemy.actor().is_hurt());
        assert(!rom_screen.arrow().active && rom_screen.state().player.last_weapon_hit == 0x0048);

        for (int f = 13; f <= 16; ++f) loop.step(InputSnapshot{});
        assert(!rom_screen.state().player.is_secondary_attacking());
        assert(rom_screen.state().player.secondary_attack_state == 0);
    }

    return 0;
}
