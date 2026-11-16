#include "game/npc_dialogue_handler.hpp"
#include "game/render/dialogue_window.hpp"
#include "game/render/dytto_sat_renderer.hpp"
#include "game/spirits/dytto_controller.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/world_graph.hpp"
#include "genesis/vdp_rasterizer.hpp"

#include <algorithm>
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

bool has_sat_tile(const oasis::game::render::SatPipelineContract& sat, std::uint16_t tile_idx) {
    for (const auto& e : sat.active_chain()) {
        if ((e.attr & 0x07FFU) == tile_idx) return true;
    }
    return false;
}

std::optional<oasis::game::render::MegaDriveSatEntry> find_sat_tile(
    const oasis::game::render::SatPipelineContract& sat, std::uint16_t tile_idx) {
    for (const auto& e : sat.active_chain()) {
        if ((e.attr & 0x07FFU) == tile_idx) return e;
    }
    return std::nullopt;
}

void test_vram_upload_from_rom(const std::vector<std::uint8_t>& rom) {
    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0xAAU);

    const std::size_t uploaded = oasis::game::render::upload_dytto_tiles_to_vram(rom, vdp);
    OASIS_CHECK(uploaded == 2464U); // 77 tiles * 32 bytes from 0x00141580

    // Bytes before 0x9A00 and after 0x9A00 + 2464 must remain untouched.
    OASIS_CHECK(vdp.vram[0x99FFU] == 0xAAU);
    OASIS_CHECK(vdp.vram[0x9A00U + 2464U] == 0xAAU);

    // Uploaded span must contain non-sentinel tile data.
    bool any_non_aa = false;
    for (std::size_t i = 0; i < uploaded; ++i) {
        if (vdp.vram[0x9A00U + i] != 0xAAU) {
            any_non_aa = true;
            break;
        }
    }
    OASIS_CHECK(any_non_aa);
    std::cout << "[PASS] Phase 1: Dytto Nemesis Decompression (0x00141580 -> 2464B @ VRAM 0x9A00) Verified.\n";
}

void test_simulation_vram_and_sat_rendering(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));

    // Transition to Screen(1,6) Water Shrine (0x0106)
    game::world::ScreenTransitionTarget tr_to_16{
        game::world::ScreenId{1U, 6U}, 160, 200, 0, 0, 0, 288, 136, 0x00000002U,
        game::world::TransitionEdgeType::BoundaryNorth};
    sim->transition_to_screen(tr_to_16);
    sim->set_player_position(160, 100);
    sim->set_player_direction(0U);

    game::FrameInput in{};
    game::render::DialogueWindow dialogue_window{};
    for (int p = 0; p < 4; ++p) {
        in.controller_1.buttons = 0;
        in.controller_1.set(core::Button::A, true);
        OASIS_CHECK(game::update_npc_dialogue_state(
            const_cast<game::player::PlayerState&>(sim->player()),
            in.controller_1,
            sim->event_vm(),
            dialogue_window,
            sim->entities(),
            rom,
            0x0106U,
            &sim->dytto_controller()));
    }
    OASIS_CHECK(sim->dytto_controller().is_active());
    OASIS_CHECK(sim->dytto_controller().dma_requested());

    // Step 2 frames (60Hz -> 30Hz SAT update cadence) so VRAM 0x9A00 and State 0x1A SAT sprite are committed.
    genesis::VdpState vdp{};
    in.controller_1.buttons = 0;
    sim->step_frame(in, vdp);
    sim->step_frame(in, vdp);
    OASIS_CHECK(!sim->dytto_controller().dma_requested());
    OASIS_CHECK(has_sat_tile(sim->sat_pipeline(), 0x04D0U));

    // Step to frame 181 (odd SAT update frame with burst sparkle at 0x04DC) and frame 245 (palette pulse).
    for (int f = 3; f <= 181; ++f) {
        sim->step_frame(in, vdp);
    }
    OASIS_CHECK(has_sat_tile(sim->sat_pipeline(), 0x04D0U));
    OASIS_CHECK(has_sat_tile(sim->sat_pipeline(), 0x04DCU));

    for (int f = 182; f <= 245; ++f) {
        sim->step_frame(in, vdp);
    }
    const auto pulse_entry = find_sat_tile(sim->sat_pipeline(), 0x04D0U);
    OASIS_CHECK(pulse_entry.has_value());
    OASIS_CHECK(((pulse_entry->attr >> 13U) & 0x03U) == 2U);

    // Finish State 0x1A -> State 0x04 (FollowPlayer).
    for (int f = 246; f <= 421; ++f) {
        sim->step_frame(in, vdp);
    }
    OASIS_CHECK(sim->dytto_controller().state() == game::spirits::DyttoSpiritController::State::FollowPlayer);

    // Move player left so Dytto steers left (angle ~128 -> h_flip bit 0x0800 set).
    sim->set_player_position(80, 100);
    for (int f = 0; f < 30; ++f) {
        sim->step_frame(in, vdp);
    }
    const auto follow_entry = find_sat_tile(sim->sat_pipeline(), 0x04D0U);
    OASIS_CHECK(follow_entry.has_value());
    OASIS_CHECK((follow_entry->attr & 0x0800U) != 0U);

    // Verify VDP rasterizer produces valid 320x224 RGB24 output with sprite pixels rendered.
    std::vector<std::uint8_t> rgb(320U * 224U * 3U, 0U);
    genesis::RasterMetrics metrics{};
    genesis::render_vdp_frame_rgb24(vdp, rgb, genesis::ColorDacMode::ModelFullRange, &metrics);
    OASIS_CHECK(metrics.total_pixels == 320 * 224);
    OASIS_CHECK(metrics.sprite_pixels > 0);

    std::cout << "[PASS] Phase 2: Dytto VRAM 0x9A00 Upload, State 0x1A/0x04 SAT Emission & Rasterization Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.53: DYTTO VRAM 0x9A00 DMA & SAT RENDERING INTEGRATION\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_vram_upload_from_rom(rom);
    test_simulation_vram_and_sat_rendering(rom);
    std::cout << "\n[PASS] All M16.53 Dytto VRAM & SAT Rendering Tests PASSED!\n";
    return 0;
}
