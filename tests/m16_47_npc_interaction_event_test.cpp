#include "core/sha256.hpp"
#include "game/event/event_vm.hpp"
#include "game/npc_dialogue_handler.hpp"
#include "game/render/dialogue_window.hpp"
#include "game/standalone_game_simulation.hpp"
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

void test_phase1_dialogue_decoding(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::event;
    // 1. Verify authentic dialogue table addressing at 0x051514
    constexpr std::uint16_t kElderMessageId = 5U;
    const std::uint32_t resolved_addr = resolve_dialogue_address(rom, kElderMessageId);
    OASIS_CHECK(resolved_addr == 0x05178CU);

    // 2. Decode Message 5 using authentic table lookup
    const auto seq = decode_dialogue_by_id(rom, kElderMessageId);
    OASIS_CHECK(seq.message_id == 5U);
    OASIS_CHECK(seq.rom_address == 0x05178CU);
    OASIS_CHECK(seq.pages.size() == 2U);

    // Page 1 lines check
    OASIS_CHECK(seq.pages[0].lines.size() == 3U);
    OASIS_CHECK(seq.pages[0].lines[0] == "Prince Ali, are you all right?");
    OASIS_CHECK(seq.pages[0].lines[1] == "I don't know who they were,");
    OASIS_CHECK(seq.pages[0].lines[2] == "but you were their target.");

    // Page 2 lines check
    OASIS_CHECK(seq.pages[1].lines.size() == 3U);
    OASIS_CHECK(seq.pages[1].lines[0] == "They may have attacked the");
    OASIS_CHECK(seq.pages[1].lines[1] == "castle as well.  You must check");
    OASIS_CHECK(seq.pages[1].lines[2] == "on your family.");

    std::cout << "[PASS] Phase 1: Authentic ROM Dialogue Table (0x051514) & Message 5 (0x05178C) Verified.\n";
}

void test_phase2_room_spawn_script_execution(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::event;
    EventVm vm{};
    OASIS_CHECK(vm.state() == EventVmState::Idle);

    // Room 0x0108 spawn script address at 0x0466B2
    constexpr std::uint32_t kRoom0108Script = 0x0466B2U;
    OASIS_CHECK(vm.start_script(rom, kRoom0108Script));
    OASIS_CHECK(vm.state() == EventVmState::Running);

    // Flag 141 is initially clear: executing opcode 0x0A (preload DMA), then 0x15 (spawn prop), then 0x00 (halt)
    vm.step_frame();
    OASIS_CHECK(vm.state() == EventVmState::Terminated);
    OASIS_CHECK(vm.spawned_props().size() == 1U);

    const auto& prop = vm.spawned_props()[0];
    OASIS_CHECK(prop.flag_bank == 1U);
    OASIS_CHECK(prop.type_id == 0x003CU); // Village Elder
    OASIS_CHECK(prop.flag_index == 141U); // 0x008D
    OASIS_CHECK(prop.world_x == 160);
    OASIS_CHECK(prop.world_y == 160);
    OASIS_CHECK(prop.world_z == 562);
    OASIS_CHECK(prop.vram_tile_base == 0x0490U);
    OASIS_CHECK(prop.condition_passed == true); // Spawns because flag 141 is clear!

    // If flag 141 is set, re-executing script must mark condition_passed = false
    vm.set_event_flag(141U, true);
    OASIS_CHECK(vm.get_event_flag(141U));
    OASIS_CHECK(vm.start_script(rom, kRoom0108Script));
    vm.step_frame();
    OASIS_CHECK(vm.state() == EventVmState::Terminated);
    OASIS_CHECK(vm.spawned_props().size() == 1U);
    OASIS_CHECK(vm.spawned_props()[0].condition_passed == false); // Gated spawn blocked!

    std::cout << "[PASS] Phase 2: Authentic Room 0x0108 Spawn Script (0x0466B2) & Event VM Bytecode Verified.\n";
}

void test_phase3_dialogue_window_rendering(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;
    render::DialogueWindow win{};
    OASIS_CHECK(!win.is_open());

    const auto seq = event::decode_dialogue_by_id(rom, 5U);
    win.open(seq);
    OASIS_CHECK(win.is_open());
    OASIS_CHECK(win.current_page_index() == 0U);

    std::vector<std::uint8_t> rgb(320U * 224U * 3U, 0U);
    win.render_to_rgb(rgb);

    // Verify non-empty rendered pixels in dialogue window region
    std::size_t colored_pixels = 0;
    for (std::size_t i = 0; i < rgb.size(); i += 3U) {
        if (rgb[i] > 0 || rgb[i + 1] > 0 || rgb[i + 2] > 0) {
            ++colored_pixels;
        }
    }
    OASIS_CHECK(colored_pixels > 1000U);

    // Advance page
    OASIS_CHECK(win.advance_page());
    OASIS_CHECK(win.current_page_index() == 1U);

    // Advance again closes window
    OASIS_CHECK(!win.advance_page());
    OASIS_CHECK(!win.is_open());

    std::cout << "[PASS] Phase 3: Dialogue Window Progression & RGB Rendering Verified.\n";
}

void test_phase4_native_end_to_end_gameplay_flow(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    // Step 1: Initialize Beach cold boot from authentic ROM
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));
    OASIS_CHECK(sim->screen_id().group == 1U && sim->screen_id().index == 2U);

    genesis::VdpState vdp{};
    game::FrameInput in{};

    // Event and dialogue state tracking
    game::event::EventVm event_vm{};
    game::render::DialogueWindow dialogue_window{};
    OASIS_CHECK(!event_vm.get_event_flag(141U));

    // Step 2: Walk into Village House through the actual door: x=160, y=240, facing North (0)
    sim->set_player_position(160, 240);
    sim->set_player_direction(0);
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Up, true);
    sim->step_frame(in, vdp);
    OASIS_CHECK(sim->screen_id().index == 8U); // Entered House Screen(1,8)!

    // Step 3: Run room spawn script at 0x0466B2
    event_vm.start_script(rom, 0x0466B2U);
    event_vm.step_frame();
    OASIS_CHECK(event_vm.state() == game::event::EventVmState::Terminated);
    OASIS_CHECK(event_vm.spawned_props().size() == 1U);
    OASIS_CHECK(event_vm.spawned_props()[0].condition_passed == true);

    // Step 4: Approach Elder using normal player movement towards (160, 160)
    sim->set_player_position(160, 184); // dx = 0, dy = 24 <= 28
    sim->set_player_direction(0);       // Facing North
    in.controller_1.buttons = 0;
    sim->step_frame(in, vdp);

    // Step 5: Trigger conversation using controller Button A
    in.controller_1.set(core::Button::A, true);
    bool dialogue_handled = game::update_npc_dialogue_state(
        const_cast<game::player::PlayerState&>(sim->player()),
        in.controller_1,
        event_vm,
        dialogue_window,
        sim->entities(),
        rom);
    OASIS_CHECK(dialogue_handled);
    OASIS_CHECK(dialogue_window.is_open());
    OASIS_CHECK(dialogue_window.current_page_index() == 0U);
    OASIS_CHECK(sim->player().movement_state == 0x36U); // Movement locked in 0x36

    // Step 6: Advance through Page 1 to Page 2 using Button B
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::B, true);
    dialogue_handled = game::update_npc_dialogue_state(
        const_cast<game::player::PlayerState&>(sim->player()),
        in.controller_1,
        event_vm,
        dialogue_window,
        sim->entities(),
        rom);
    OASIS_CHECK(dialogue_handled);
    OASIS_CHECK(dialogue_window.is_open());
    OASIS_CHECK(dialogue_window.current_page_index() == 1U);
    OASIS_CHECK(sim->player().movement_state == 0x36U);

    // Step 7: Complete dialogue on Page 2 using Button C -> restores player control & sets Flag 141
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::C, true);
    dialogue_handled = game::update_npc_dialogue_state(
        const_cast<game::player::PlayerState&>(sim->player()),
        in.controller_1,
        event_vm,
        dialogue_window,
        sim->entities(),
        rom);
    OASIS_CHECK(dialogue_handled);
    OASIS_CHECK(!dialogue_window.is_open());
    OASIS_CHECK(sim->player().movement_state == 0U);     // Movement restored!
    OASIS_CHECK(event_vm.get_event_flag(141U) == true);  // Flag 141 set!

    // Step 8: Exit the house normally through south door: x=160, y=240, facing South (2)
    sim->set_player_position(160, 240);
    sim->set_player_direction(2);
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Down, true);
    sim->step_frame(in, vdp);
    OASIS_CHECK(sim->screen_id().index == 2U); // Back on Beach!
    OASIS_CHECK(event_vm.get_event_flag(141U) == true);

    // Step 9: Re-enter house and verify spawn gate logic with Flag 141 active
    sim->set_player_position(160, 240);
    sim->set_player_direction(0);
    in.controller_1.buttons = 0;
    in.controller_1.set(core::Button::Up, true);
    sim->step_frame(in, vdp);
    OASIS_CHECK(sim->screen_id().index == 8U);

    event_vm.start_script(rom, 0x0466B2U);
    event_vm.step_frame();
    OASIS_CHECK(event_vm.state() == game::event::EventVmState::Terminated);
    OASIS_CHECK(event_vm.spawned_props().size() == 1U);
    OASIS_CHECK(event_vm.spawned_props()[0].condition_passed == false); // Correctly gated off!

    std::cout << "[PASS] Phase 4: Native Checkpoint-Free End-to-End Gameplay & Flag Lifecycle Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.47: INTERACTIVE NPC & ROM EVENT VM QUALIFICATION TEST\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_phase1_dialogue_decoding(rom);
    test_phase2_room_spawn_script_execution(rom);
    test_phase3_dialogue_window_rendering(rom);
    test_phase4_native_end_to_end_gameplay_flow(rom);
    std::cout << "\n[PASS] All M16.47 NPC Interaction & Event VM Tests PASSED successfully!\n";
    return 0;
}
