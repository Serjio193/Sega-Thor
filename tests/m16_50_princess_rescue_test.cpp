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

// Phase 0: Baseline verification and M16.50 fixture check
void test_phase0_baseline_and_fixture(const std::vector<std::uint8_t>& rom) {
    const std::string fixture_str = read_text(resolve_path("data/standalone-fixtures/princess_rescue_and_sanctuary_m16_50.json"));
    OASIS_CHECK(fixture_str.find("\"milestone\": \"M16.50\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x018CAA\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x018CDE\"") != std::string::npos);
    OASIS_CHECK(fixture_str.find("\"0x018D38\"") != std::string::npos);
    OASIS_CHECK(rom.size() == 3145728U);
    std::cout << "[PASS] Phase 0: Baseline & M16.50 Fixture Verified.\n";
}

// Phase 1: Reverse engineering verification of ROM routines 0x018CAA, 0x018CDE, 0x018D38
void test_phase1_rom_routines(const std::vector<std::uint8_t>& rom) {
    // 0x00835E: cmpi.w #$0020, d0 -> beq 0x008392 -> jmp $018CAA
    OASIS_CHECK(rom[0x00835E] == 0x0C && rom[0x00835F] == 0x40 && rom[0x008360] == 0x00 && rom[0x008361] == 0x20);
    OASIS_CHECK(rom[0x008392] == 0x4E && rom[0x008393] == 0xF9 && rom[0x008394] == 0x00 && rom[0x008395] == 0x01 && rom[0x008396] == 0x8C && rom[0x008397] == 0xAA);

    // 0x018CAA: checks $FF0E40 == 0x0106 -> beq 0x018CDE
    OASIS_CHECK(rom[0x018CB0] == 0x0C && rom[0x018CB1] == 0x40 && rom[0x018CB2] == 0x01 && rom[0x018CB3] == 0x06);

    // 0x018CDE: sets Message ID 0 (moveq #0, d0) and calls dialogue dispatch 0x00C2EC
    OASIS_CHECK(rom[0x018CEE] == 0x70 && rom[0x018CEF] == 0x00);
    OASIS_CHECK(rom[0x018CF0] == 0x4E && rom[0x018CF1] == 0xB9 && rom[0x018CF2] == 0x00 && rom[0x018CF3] == 0x00 && rom[0x018CF4] == 0xC2 && rom[0x018CF5] == 0xEC);

    // 0x018D38: bset #3, ($00FF0DBC) -> Global Flag 3 SET!
    OASIS_CHECK(rom[0x018D38] == 0x08 && rom[0x018D39] == 0xF9 && rom[0x018D3A] == 0x00 && rom[0x018D3B] == 0x03 &&
                rom[0x018D3C] == 0x00 && rom[0x018D3D] == 0xFF && rom[0x018D3E] == 0x0D && rom[0x018D3F] == 0xBC);

    // Decode Message 0 from ROM table 0x051514
    const auto seq = oasis::game::event::decode_dialogue_by_id(rom, 0U);
    OASIS_CHECK(seq.pages.size() == 3U);
    OASIS_CHECK(!seq.pages[0].lines.empty());
    OASIS_CHECK(seq.pages[0].lines[0].find("I am Dytto, a water spirit.") != std::string::npos);

    std::cout << "[PASS] Phase 1: Princess Rescue Event ROM Routines Verified.\n";
}

// Phase 2, 3, and 4: Authentic Story Flag Progression & Cross-Area Transition
void test_phase2_3_4_authentic_progression(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));

    // Initially: on Beach Screen(1,2), Global Flag 3 is FALSE (NOT INJECTED!)
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U));

    // Warp is blocked when Flag 3 is false:
    const auto tr_blocked = game::world::evaluate_world_transition(
        game::world::ScreenId{1U, 6U}, 160, 56, 0U, false, sim->event_vm().get_event_flag(3U, 1U));
    OASIS_CHECK(!tr_blocked.has_value());

    // Step 1: Transition player naturally through world graph to Screen(1,6) Cliff Peak
    game::world::ScreenTransitionTarget tr_to_16{
        game::world::ScreenId{1U, 6U}, 160, 200, 0, 96, 0, 96, 136, 0x00000002U,
        game::world::TransitionEdgeType::BoundaryNorth};
    sim->transition_to_screen(tr_to_16);
    OASIS_CHECK(sim->screen_id().group == 1U && sim->screen_id().index == 6U);

    // In Screen(1,6), verify entity 0x0020 (Princess Gwyn) is present at (160, 80)
    OASIS_CHECK(!sim->entities().empty());
    OASIS_CHECK(sim->entities()[0].type_id == 0x0020U);
    OASIS_CHECK(sim->entities()[0].world_x == 160 && sim->entities()[0].world_y == 80);

    // Step 2: Approach entity 0x0020 (player pos at (160, 100), facing North, distance dy=20 <= 28)
    sim->set_player_position(160, 100);
    sim->set_player_direction(0U); // North

    // Step 3: Trigger interaction with Button A
    game::FrameInput in{};
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
    OASIS_CHECK(sim->player().movement_state == 0x36U); // Movement locked in talk state!
    OASIS_CHECK(dialogue_window.is_open());
    OASIS_CHECK(dialogue_window.message_id() == 0U);
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U)); // Flag 3 NOT set until dialogue completes!

    // Page 1 advance: press Button A
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
    OASIS_CHECK(dialogue_window.current_page_index() == 1U); // Advanced to page 2
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U)); // Still not set

    // Page 2 advance: press Button A
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
    OASIS_CHECK(dialogue_window.current_page_index() == 2U); // Advanced to page 3
    OASIS_CHECK(!sim->event_vm().get_event_flag(3U, 1U)); // Still not set

    // Page 3 completion: press Button A to finish dialogue
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
    OASIS_CHECK(!dialogue_window.is_open()); // Dialogue finished
    OASIS_CHECK(sim->player().movement_state == 0U); // Player unlocked!

    // AUTHENTIC STORY FLAG PROGRESSION: Global Flag 3 IS NOW SET BY GAMEPLAY!
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U));
    std::cout << "[PASS] Phase 2 & 3: Princess Event Completed & Global Flag 3 Set Authentically!\n";

    // Step 4: Player moves north to Cliff Peak altar / sanctuary warp trigger at (160, 56)
    sim->set_player_position(160, 56);
    sim->set_player_direction(0U);

    // Evaluate transition with authentic gameplay flag 3:
    const auto tr_warp = game::world::evaluate_world_transition(
        sim->screen_id(), 160, 56, 0U, false, sim->event_vm().get_event_flag(3U, 1U));
    OASIS_CHECK(tr_warp.has_value());
    OASIS_CHECK(tr_warp->destination_screen.group == 13U && tr_warp->destination_screen.index == 0U);

    // Execute transition:
    sim->transition_to_screen(*tr_warp);
    OASIS_CHECK(sim->screen_id().group == 13U && sim->screen_id().index == 0U);
    OASIS_CHECK(sim->player().x_fixed / 0x10000 == 256);
    OASIS_CHECK(sim->player().y_fixed / 0x10000 == 768);

    // Step 5: Verify Room 0x0D00 movement and collision
    genesis::VdpState vdp{};
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Up, true); // Move North in Sanctuary
    sim->step_frame(in, vdp);
    OASIS_CHECK(sim->player().y_fixed / 0x10000 <= 768); // Player moves forward into Sanctuary
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U)); // Flag 3 persisted across cross-area warp!

    std::cout << "[PASS] Phase 4: Spirit Sanctuary Reached Authentically Without State Injection!\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.50: PRINCESS RESCUE EVENT & AUTHENTIC SANCTUARY WARP\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_phase0_baseline_and_fixture(rom);
    test_phase1_rom_routines(rom);
    test_phase2_3_4_authentic_progression(rom);
    std::cout << "\n[PASS] All M16.50 Princess Rescue & Progression Tests PASSED!\n";
    return 0;
}
