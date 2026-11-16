#include "game/event/event_vm.hpp"
#include "game/npc_dialogue_handler.hpp"
#include "game/render/dialogue_window.hpp"
#include "game/spirits/dytto_controller.hpp"
#include "game/spirits/spirit_awakening.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/world_graph.hpp"

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

void test_rom_dispatcher_and_flags(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::spirits;
    OASIS_CHECK(rom.size() == 3145728U);

    // 0x018CAA: Spirit Awakening Dispatcher room checks
    // 0x018CB0: cmp.w #$0106, d0 (Water Shrine -> Dytto 0x018CDE)
    OASIS_CHECK(rom[0x018CB0] == 0x0C && rom[0x018CB1] == 0x40 && rom[0x018CB2] == 0x01 && rom[0x018CB3] == 0x06);
    // 0x018CB8: cmp.w #$0206, d0 (Fire Shrine -> Efreet 0x018D4E)
    OASIS_CHECK(rom[0x018CB8] == 0x0C && rom[0x018CB9] == 0x40 && rom[0x018CBA] == 0x02 && rom[0x018CBB] == 0x06);
    // 0x018CC0: cmp.w #$0805, d0 (Shadow Shrine -> Shade 0x018DAA)
    OASIS_CHECK(rom[0x018CC0] == 0x0C && rom[0x018CC1] == 0x40 && rom[0x018CC2] == 0x08 && rom[0x018CC3] == 0x05);
    // 0x018CC8: cmp.w #$0A10, d0 (Plant Shrine -> Bow 0x018E12)
    OASIS_CHECK(rom[0x018CC8] == 0x0C && rom[0x018CC9] == 0x40 && rom[0x018CCA] == 0x0A && rom[0x018CCB] == 0x10);
    // 0x018CD0: cmp.w #$090B, d0 (Seal Chamber -> Agito 0x018E7A)
    OASIS_CHECK(rom[0x018CD0] == 0x0C && rom[0x018CD1] == 0x40 && rom[0x018CD2] == 0x09 && rom[0x018CD3] == 0x0B);

    // 0x018D30: bclr #2, ($00FF0E1D) -> Water Shrine Area Flag
    OASIS_CHECK(rom[0x018D30] == 0x08 && rom[0x018D31] == 0xB9 && rom[0x018D32] == 0x00 && rom[0x018D33] == 0x02 &&
                rom[0x018D34] == 0x00 && rom[0x018D35] == 0xFF && rom[0x018D36] == 0x0E && rom[0x018D37] == 0x1D);
    // 0x018D38: bset #3, ($00FF0DBC) -> Global Flag 3 (Dytto acquisition)
    OASIS_CHECK(rom[0x018D38] == 0x08 && rom[0x018D39] == 0xF9 && rom[0x018D3A] == 0x00 && rom[0x018D3B] == 0x03 &&
                rom[0x018D3C] == 0x00 && rom[0x018D3D] == 0xFF && rom[0x018D3E] == 0x0D && rom[0x018D3F] == 0xBC);
    // 0x018D40: bset #0, ($00FF0D7A) -> Active Spirit Dytto bit 0
    OASIS_CHECK(rom[0x018D40] == 0x08 && rom[0x018D41] == 0xF9 && rom[0x018D42] == 0x00 && rom[0x018D43] == 0x00 &&
                rom[0x018D44] == 0x00 && rom[0x018D45] == 0xFF && rom[0x018D46] == 0x0D && rom[0x018D47] == 0x7A);

    // 0x014668: move.w #$0018, $00(a6) -> Dytto spawn entity type 0x0018
    OASIS_CHECK(rom[0x014668] == 0x3D && rom[0x014669] == 0x7C && rom[0x01466A] == 0x00 && rom[0x01466B] == 0x18);

    const auto dytto = dispatch_spirit_awakening(0x0106U);
    OASIS_CHECK(dytto.has_value());
    OASIS_CHECK(dytto->handler_address == 0x018CDEU);
    OASIS_CHECK(dytto->spawn_helper_address == 0x014668U);
    OASIS_CHECK(dytto->message_id == 0U);
    OASIS_CHECK(dytto->global_flag_idx == 3U);
    OASIS_CHECK(dytto->active_spirit_bit == 0U);
    OASIS_CHECK(dytto->area_flag_idx == 10U);
    OASIS_CHECK(dytto->elevation_delta_px == 16);

    OASIS_CHECK(dispatch_spirit_awakening(0x0206U)->message_id == 9U);
    OASIS_CHECK(dispatch_spirit_awakening(0x0805U)->message_id == 12U);
    OASIS_CHECK(dispatch_spirit_awakening(0x0A10U)->message_id == 13U);
    OASIS_CHECK(dispatch_spirit_awakening(0x090BU)->message_id == 56U);
    OASIS_CHECK(!dispatch_spirit_awakening(0xFFFFU).has_value());

    std::cout << "[PASS] Phase 1: Spirit Awakening Dispatcher (0x018CAA) & ROM Evidence Verified.\n";
}

void test_dytto_steering_primitives() {
    using oasis::game::spirits::DyttoSpiritController;

    // binary_atan2 (0x00E6F0)
    OASIS_CHECK(DyttoSpiritController::binary_atan2(10, 0) == 0U);
    OASIS_CHECK(DyttoSpiritController::binary_atan2(0, 10) == 64U);
    OASIS_CHECK(DyttoSpiritController::binary_atan2(-10, 0) == 128U);
    OASIS_CHECK(DyttoSpiritController::binary_atan2(0, -10) == 192U);

    // Angular step 8 with 8-bit wrap-around
    OASIS_CHECK(DyttoSpiritController::step_angle_toward(0U, 20U, 8U) == 8U);
    OASIS_CHECK(DyttoSpiritController::step_angle_toward(8U, 20U, 8U) == 16U);
    OASIS_CHECK(DyttoSpiritController::step_angle_toward(16U, 20U, 8U) == 20U);
    OASIS_CHECK(DyttoSpiritController::step_angle_toward(4U, 250U, 8U) == 252U);
    OASIS_CHECK(DyttoSpiritController::step_angle_toward(252U, 4U, 8U) == 4U);

    // ASR.L #7 damping
    OASIS_CHECK(DyttoSpiritController::damp_velocity_asr7(0x00010000) == 512);
    OASIS_CHECK(DyttoSpiritController::damp_velocity_asr7(-0x00010000) == -512);

    std::cout << "[PASS] Phase 2: Dytto Steering Primitives (atan2, step=8, ASR.L #7) Verified.\n";
}

void test_dytto_awakening_and_follow(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));

    // Transition to Screen(1,6) Water Shrine (0x0106)
    game::world::ScreenTransitionTarget tr_to_16{
        game::world::ScreenId{1U, 6U}, 160, 200, 0, 96, 0, 96, 136, 0x00000002U,
        game::world::TransitionEdgeType::BoundaryNorth};
    sim->transition_to_screen(tr_to_16);
    OASIS_CHECK(sim->screen_id().group == 1U && sim->screen_id().index == 6U);

    sim->set_player_position(160, 100);
    sim->set_player_direction(0U);

    game::FrameInput in{};
    game::render::DialogueWindow dialogue_window{};
    const std::uint16_t room_id = static_cast<std::uint16_t>(
        (sim->screen_id().group << 8U) | sim->screen_id().index);

    // Open and advance all 3 pages of Message 0
    for (int p = 0; p < 4; ++p) {
        in.controller_1.buttons = 0;
        in.controller_1.set(core::Button::A, true);
        const bool handled = game::update_npc_dialogue_state(
            const_cast<game::player::PlayerState&>(sim->player()),
            in.controller_1,
            sim->event_vm(),
            dialogue_window,
            sim->entities(),
            rom,
            room_id,
            &sim->dytto_controller());
        OASIS_CHECK(handled);
    }

    // Verify flags set by 0x018CDE
    OASIS_CHECK(sim->event_vm().get_event_flag(3U, 1U));   // Global Flag 3 ($FF0DBC:3)
    OASIS_CHECK(sim->event_vm().get_event_flag(10U, 2U));  // Water Shrine Area Flag ($FF0E1D:2)
    OASIS_CHECK(sim->event_vm().has_active_spirit(0U));    // Active Spirit Dytto ($FF0D7A:0)

    // Verify Dytto spawn (0x014668)
    const auto& dytto = sim->dytto_controller();
    OASIS_CHECK(dytto.is_active());
    OASIS_CHECK(dytto.type_id() == 0x0018U);
    OASIS_CHECK(dytto.collision_flags() == 0x0002U);
    OASIS_CHECK(dytto.vram_pattern() == 0x04F8U);
    OASIS_CHECK(dytto.vram_tile_base() == 0x04D0U);
    OASIS_CHECK(dytto.rom_gfx_source() == 0x00141580U);
    OASIS_CHECK(dytto.max_sp() == 100U);
    OASIS_CHECK(dytto.current_sp() == 100U);
    OASIS_CHECK(dytto.sp_drain_rate() == 10U);
    OASIS_CHECK(dytto.sp_drain_timer() == 10U);
    OASIS_CHECK(dytto.world_x() == 160 && dytto.world_y() == 80 && dytto.world_z() == 16);
    OASIS_CHECK(dytto.state() == game::spirits::DyttoSpiritController::State::AppearCutscene);
    OASIS_CHECK(dytto.action() == 0x0022U);

    // Step through State 0x1A 420-frame timeline (0x014F60)
    genesis::VdpState vdp{};
    in.controller_1.buttons = 0;
    for (int f = 1; f <= 420; ++f) {
        sim->step_frame(in, vdp);
        if (f == 179) OASIS_CHECK(!dytto.awakening_events().burst_sfx_triggered);
        if (f == 180) OASIS_CHECK(dytto.awakening_events().burst_sfx_triggered);
        if (f == 239) OASIS_CHECK(!dytto.awakening_events().palette_pulse_triggered);
        if (f == 240) OASIS_CHECK(dytto.awakening_events().palette_pulse_triggered);
        if (f == 299) OASIS_CHECK(!dytto.awakening_events().dialogue_complete_handoff);
        if (f == 300) OASIS_CHECK(dytto.awakening_events().dialogue_complete_handoff);
        if (f == 359) OASIS_CHECK(!dytto.awakening_events().camera_unlocked);
        if (f == 360) OASIS_CHECK(dytto.awakening_events().camera_unlocked);
        if (f == 419) {
            OASIS_CHECK(!dytto.awakening_events().cutscene_completed);
            OASIS_CHECK(dytto.state() == game::spirits::DyttoSpiritController::State::AppearCutscene);
        }
    }
    OASIS_CHECK(dytto.awakening_events().cutscene_completed);
    OASIS_CHECK(dytto.state() == game::spirits::DyttoSpiritController::State::FollowPlayer);

    // Move Ali and verify Dytto follows toward (player_x, player_y - 16)
    sim->set_player_position(220, 140);
    for (int f = 0; f < 120; ++f) {
        sim->step_frame(in, vdp);
    }
    const int dx = std::abs(dytto.world_x() - 220);
    const int dy = std::abs(dytto.world_y() - (140 - 16));
    OASIS_CHECK(dx <= 2 && dy <= 2);

    std::cout << "[PASS] Phase 3: Dytto Awakening Timeline (0x1A -> 0x04) & Follow Steering Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.52: SPIRIT AWAKENING DISPATCHER & DYTTO INTEGRATION\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_rom_dispatcher_and_flags(rom);
    test_dytto_steering_primitives();
    test_dytto_awakening_and_follow(rom);
    std::cout << "\n[PASS] All M16.52 Spirit Awakening & Dytto Tests PASSED!\n";
    return 0;
}
