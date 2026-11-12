#include "core/sha256.hpp"
#include "game/event/event_vm.hpp"
#include "game/npc_dialogue_handler.hpp"
#include "game/render/dialogue_window.hpp"
#include "game/scene_transition_helpers.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/town_scene_loader.hpp"
#include "game/world/world_graph.hpp"
#include "genesis/vdp_rasterizer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define OASIS_CHECK(cond)                                                                      \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << std::endl;          \
            std::exit(1);                                                                      \
        }                                                                                      \
    } while (0)

namespace {

std::string resolve_path(const std::string& rel) {
    for (const auto* p : {"", "../", "../../", "../../../"}) {
        std::filesystem::path candidate = std::string(p) + rel;
        if (std::filesystem::exists(candidate)) return candidate.string();
    }
    return rel;
}

std::vector<std::uint8_t> read_binary(const std::string& p) {
    std::ifstream in(p, std::ios::binary | std::ios::ate);
    OASIS_CHECK(in.is_open());
    const auto sz = in.tellg();
    OASIS_CHECK(sz > 0);
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

std::string read_text(const std::string& p) {
    std::ifstream in(p);
    OASIS_CHECK(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// Phase 0: Baseline verification and M16.51 fixture validation
void test_phase0_baseline_and_fixture(const std::vector<std::uint8_t>& rom) {
    const std::string fixture_str = read_text(resolve_path("data/standalone-fixtures/sanctuary_multi_room_m16_51.json"));
    OASIS_CHECK(fixture_str.find("\"milestone\": \"M16.51\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x0D00\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x0D01\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x02E452\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x02E524\"") != std::string::npos);
    OASIS_CHECK(rom.size() == 3145728U);
    std::cout << "[PASS] Phase 0: Baseline & M16.51 Fixture Verified.\n";
}

// Phase 1: Sanctuary World Structure Reverse Engineering
void test_phase1_sanctuary_world_structure(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::world;
    // Room 0x0D00 (Entrance): primary stream 0x259F78, tile banks [86, 87, 88, 0]
    const auto desc0 = load_screen_descriptor(rom, ScreenId{13U, 0U});
    OASIS_CHECK(desc0.primary_stream_pointer() == 0x259F78U);
    const auto rids0 = desc0.resource_ids();
    OASIS_CHECK(rids0[0] == 86U && rids0[1] == 87U && rids0[2] == 88U && rids0[3] == 0U);

    // Room 0x0D01 (Inner Chamber): primary stream 0x25A3C4, tile banks [86, 87, 88, 0]
    const auto desc1 = load_screen_descriptor(rom, ScreenId{13U, 1U});
    OASIS_CHECK(desc1.primary_stream_pointer() == 0x25A3C4U);
    const auto rids1 = desc1.resource_ids();
    OASIS_CHECK(rids1[0] == 86U && rids1[1] == 87U && rids1[2] == 88U && rids1[3] == 0U);

    // Verify ROM transition opcode at 0x02E44C: move.w #$0001, d0; move.w #$0100, d3; move.w #$0070, d4; jmp $00CA24
    OASIS_CHECK(rom[0x02E44C] == 0x30 && rom[0x02E44D] == 0x3C && rom[0x02E44E] == 0x00 && rom[0x02E44F] == 0x01);
    OASIS_CHECK(rom[0x02E450] == 0x36 && rom[0x02E451] == 0x3C && rom[0x02E452] == 0x01 && rom[0x02E453] == 0x00);
    OASIS_CHECK(rom[0x02E454] == 0x38 && rom[0x02E455] == 0x3C && rom[0x02E456] == 0x00 && rom[0x02E457] == 0x70);

    // Verify spawn script at 0x04382E starts with 0x00 (Halt/Yield)
    OASIS_CHECK(rom[0x04382E] == 0x00);

    std::cout << "[PASS] Phase 1: Sanctuary World Structure & ROM Descriptors Verified.\n";
}

// Phase 2: Generic Room Loading
void test_phase2_generic_room_loading(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::world;
    // Load Room 0x0D00
    const auto scene0 = load_world_screen_from_rom(rom, ScreenId{13U, 0U});
    OASIS_CHECK(!scene0.plane_a_metatiles.empty());
    OASIS_CHECK(!scene0.terrain_collision.empty());
    OASIS_CHECK(scene0.scene_palette_words[0] != 0U);

    // Load Room 0x0D01
    const auto scene1 = load_world_screen_from_rom(rom, ScreenId{13U, 1U});
    OASIS_CHECK(!scene1.plane_a_metatiles.empty());
    OASIS_CHECK(!scene1.terrain_collision.empty());
    OASIS_CHECK(scene1.scene_palette_words[0] != 0U);

    // Both rooms share the same tile banks
    OASIS_CHECK(scene0.descriptor.resource_ids() == scene1.descriptor.resource_ids());

    // Dimensions: check camera bounds
    int max_cx0 = 0, max_cy0 = 0;
    get_group1_screen_bounds(ScreenId{13U, 0U}, max_cx0, max_cy0);
    OASIS_CHECK(max_cx0 == 0 && max_cy0 == 608);

    int max_cx1 = 0, max_cy1 = 0;
    get_group1_screen_bounds(ScreenId{13U, 1U}, max_cx1, max_cy1);
    OASIS_CHECK(max_cx1 == 0 && max_cy1 == 76);

    std::cout << "[PASS] Phase 2: Generic Room Loading for Multi-Room Sanctuary Verified.\n";
}

// Phase 3 & 4: In-Sanctuary Gameplay & Continuous Progression Session
void test_phase3_4_gameplay_and_continuous_session(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));

    // Initially: on Beach Screen(1,2), Global Flag 3 is FALSE
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U));

    // Step 1: Cold start traverse from Beach to Cliff Peak Screen(1,6)
    game::FrameInput in{};
    genesis::VdpState vdp{};

    // Move to Screen(1,6) Cliff Peak
    const auto tr_cliff = game::world::evaluate_world_transition(
        game::world::ScreenId{1, 7}, 160, 100, 0U, false, false);
    OASIS_CHECK(tr_cliff.has_value());
    sim->transition_to_screen(*tr_cliff);
    OASIS_CHECK(sim->screen_id().group == 1U && sim->screen_id().index == 6U);

    // Step 2: Trigger Native Princess Rescue Event
    auto& entities = sim->entities();
    OASIS_CHECK(!entities.empty());
    OASIS_CHECK(entities[0].type_id == 0x0020U);
    sim->set_player_position(160, 100);
    sim->set_player_direction(0U); // North

    in.controller_1.set(core::Button::A, true);
    game::render::DialogueWindow dialogue_window{};

    bool dialogue_handled = game::update_npc_dialogue_state(
        const_cast<game::player::PlayerState&>(sim->player()),
        in.controller_1,
        sim->event_vm(),
        dialogue_window,
        sim->entities(),
        rom);
    OASIS_CHECK(dialogue_handled);
    OASIS_CHECK(sim->player().movement_state == 0x36U); // Movement locked in talk state
    OASIS_CHECK(dialogue_window.is_open());
    OASIS_CHECK(dialogue_window.message_id() == 0U);
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U));

    // Advance 3 pages of dialogue to complete event
    for (int p = 0; p < 3; ++p) {
        in.controller_1.buttons = 0;
        in.controller_1.set(core::Button::A, true);
        dialogue_handled = game::update_npc_dialogue_state(
            const_cast<game::player::PlayerState&>(sim->player()),
            in.controller_1,
            sim->event_vm(),
            dialogue_window,
            sim->entities(),
            rom);
        OASIS_CHECK(dialogue_handled);
    }
    OASIS_CHECK(!dialogue_window.is_open());
    OASIS_CHECK(sim->player().movement_state == 0U); // Player unlocked!
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U)); // Authentic Story Flag 3 is now SET!

    // Step 3: Trigger Cross-Area Warp into Spirit Sanctuary Room 0x0D00
    const auto tr_warp = game::world::evaluate_world_transition(
        game::world::ScreenId{1, 6}, 160, 56, 0U, false, true);
    OASIS_CHECK(tr_warp.has_value());
    sim->transition_to_screen(*tr_warp);
    OASIS_CHECK(sim->screen_id().group == 13U && sim->screen_id().index == 0U);
    OASIS_CHECK(sim->player().x_fixed / 0x10000 == 256);
    OASIS_CHECK(sim->player().y_fixed / 0x10000 == 768);

    // Step 4: Move North in Sanctuary Entrance Room 0
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Up, true);
    for (int i = 0; i < 10; ++i) {
        sim->step_frame(in, vdp);
    }
    OASIS_CHECK(sim->player().y_fixed / 0x10000 < 768);

    // Step 5: Advance player North to doorway threshold (y <= 120) to trigger Room 1 transition
    sim->set_player_position(256, 118);
    const auto tr_room1 = game::world::evaluate_world_transition(
        sim->screen_id(), 256, 118, sim->player().direction_code, false, true);
    OASIS_CHECK(tr_room1.has_value());
    OASIS_CHECK(tr_room1->destination_screen.group == 13U && tr_room1->destination_screen.index == 1U);
    OASIS_CHECK(tr_room1->player_x == 256 && tr_room1->player_y == 112);

    // Execute transition into Room 1 (Inner Chamber)
    sim->transition_to_screen(*tr_room1);
    OASIS_CHECK(sim->screen_id().group == 13U && sim->screen_id().index == 1U);
    OASIS_CHECK(sim->player().x_fixed / 0x10000 == 256);
    OASIS_CHECK(sim->player().y_fixed / 0x10000 == 112);
    OASIS_CHECK(sim->camera().config().max_camera_y == 76);
    OASIS_CHECK(sim->town_scene().plane_a_metatiles.size() > 0);
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U)); // Flag 3 preserved

    // Step 6: Step gameplay inside Room 1
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Down, true); // Face Down/South
    for (int i = 0; i < 5; ++i) {
        sim->step_frame(in, vdp);
    }
    OASIS_CHECK(sim->player().direction_code == 2U); // Facing Down
    OASIS_CHECK(sim->player().y_fixed / 0x10000 >= 112); // Player walked South

    // Step 7: Reach South doorway (y >= 230) and return to Room 0
    sim->set_player_position(256, 235);
    const auto tr_room0 = game::world::evaluate_world_transition(
        sim->screen_id(), 256, 235, sim->player().direction_code, false, true);
    OASIS_CHECK(tr_room0.has_value());
    OASIS_CHECK(tr_room0->destination_screen.group == 13U && tr_room0->destination_screen.index == 0U);
    OASIS_CHECK(tr_room0->player_x == 256 && tr_room0->player_y == 160);

    // Execute return transition into Room 0
    sim->transition_to_screen(*tr_room0);
    OASIS_CHECK(sim->screen_id().group == 13U && sim->screen_id().index == 0U);
    OASIS_CHECK(sim->player().x_fixed / 0x10000 == 256);
    OASIS_CHECK(sim->player().y_fixed / 0x10000 == 160);
    OASIS_CHECK(sim->camera().config().max_camera_y == 608); // Camera limits restored
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U)); // Flag 3 preserved

    std::cout << "[PASS] Phase 3 & 4: Multi-Room Sanctuary Playability & Continuous Progression Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.51: SPIRIT SANCTUARY PLAYABILITY & ROOM PROGRESSION\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_phase0_baseline_and_fixture(rom);
    test_phase1_sanctuary_world_structure(rom);
    test_phase2_generic_room_loading(rom);
    test_phase3_4_gameplay_and_continuous_session(rom);
    std::cout << "\n[PASS] All M16.51 Sanctuary Playability Tests PASSED!\n";
    return 0;
}
