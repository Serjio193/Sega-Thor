#include "game/entity/native_entity.hpp"
#include "game/spirits/dytto_abilities.hpp"
#include "game/spirits/dytto_controller.hpp"
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

bool has_sat_tile(const oasis::game::render::SatPipelineContract& sat, std::uint16_t tile_idx) {
    for (const auto& e : sat.active_chain()) {
        if ((e.attr & 0x07FFU) == tile_idx) return true;
    }
    return false;
}

void test_autonomous_dispatch_0x00() {
    using oasis::game::spirits::DyttoSpiritController;
    DyttoSpiritController dytto;
    dytto.init_spawn(160, 120, 0, 2U, false);

    // prng <= 0x2000 -> State 0x06 (MagicAttack1), Action 0x02, SFX 0x800251
    dytto.set_state(DyttoSpiritController::State::IdleDispatch);
    dytto.update_frame(160, 120, {}, 0x2000U, false);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::MagicAttack1);
    OASIS_CHECK(dytto.action() == 0x0002U);
    OASIS_CHECK(dytto.last_sound_cmd() == 0x00800251U);

    // 0x2000 < prng <= 0x6000 -> State 0x10 (SpecialSpread), Action 0x12
    dytto.set_state(DyttoSpiritController::State::IdleDispatch);
    dytto.update_frame(160, 120, {}, 0x4000U, false);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::SpecialSpread);
    OASIS_CHECK(dytto.action() == 0x0012U);

    // 0x6000 < prng <= 0x9000 -> State 0x16 (ReturnToAli), Action 0x18
    dytto.set_state(DyttoSpiritController::State::IdleDispatch);
    dytto.update_frame(160, 120, {}, 0x8000U, false);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::ReturnToAli);
    OASIS_CHECK(dytto.action() == 0x0018U);
    OASIS_CHECK(dytto.last_sound_cmd() == 0x00800251U);

    // prng > 0x9000 or cutscene_locked -> State 0x04 wander (timer 10)
    dytto.set_state(DyttoSpiritController::State::IdleDispatch);
    dytto.update_frame(160, 120, {}, 0xA000U, false);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::FollowPlayer);
    OASIS_CHECK(dytto.wander_timer() == 10U);

    dytto.set_state(DyttoSpiritController::State::IdleDispatch);
    dytto.update_frame(160, 120, {}, 0x1000U, true);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::FollowPlayer);
    OASIS_CHECK(dytto.wander_timer() == 10U);

    std::cout << "[PASS] Phase 1: Autonomous Dispatch (State 0x00, 0x2000/0x6000/0x9000) Verified.\n";
}

void test_spread_burst_damage_buff_knockback() {
    using namespace oasis::game;
    using oasis::game::spirits::DyttoSpiritController;
    DyttoSpiritController dytto;
    dytto.init_spawn(100, 100, 0, 2U, false);

    // 3-Way Spread (State 0x10 -> 0x015214)
    dytto.set_state(DyttoSpiritController::State::SpecialSpread);
    dytto.update_frame(100, 100, {});
    OASIS_CHECK(dytto.projectiles().size() == 3U);
    OASIS_CHECK(dytto.projectiles()[0].type_id == 0x0024U);
    OASIS_CHECK(dytto.projectiles()[0].state == 0x0012U);
    OASIS_CHECK(dytto.projectiles()[0].action == 0x0014U);
    OASIS_CHECK(dytto.projectiles()[0].hitbox_radius == 5);
    OASIS_CHECK(dytto.last_sound_cmd() == 0x0080034FU);

    // Ground enemy hit -> 0x2008 Water damage + 0x700345 SFX
    std::vector<NativeEntity> enemies(1);
    enemies[0].active = true;
    enemies[0].type_id = 0x002CU;
    enemies[0].world_x = 109;
    enemies[0].world_y = 100;
    enemies[0].world_z = 0;
    for (int f = 0; f < 10; ++f) {
        dytto.update_frame(100, 100, enemies);
    }
    OASIS_CHECK(enemies[0].damage_mailbox == 0x2008U);
    OASIS_CHECK(dytto.last_sound_cmd() == 0x00700345U);

    // Airborne enemy hit -> 0xA008 Water damage
    dytto.init_spawn(100, 100, 16, 2U, false);
    dytto.set_state(DyttoSpiritController::State::SpecialSpread);
    dytto.update_frame(100, 100, {});
    enemies[0].damage_mailbox = 0U;
    enemies[0].world_z = 16;
    for (int f = 0; f < 10; ++f) {
        dytto.update_frame(100, 100, enemies);
    }
    OASIS_CHECK(enemies[0].damage_mailbox == 0xA008U);

    // Magic Burst (State 0x06 -> 0x08 -> 0x0A -> 0x015136)
    dytto.init_spawn(100, 100, 0, 2U, false);
    dytto.set_state(DyttoSpiritController::State::MagicAttack1);
    dytto.update_frame(100, 100, {}); // -> MagicAttack2
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::MagicAttack2);
    for (int f = 0; f < 3; ++f) {
        dytto.update_frame(100, 100, {});
    }
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::MagicRecovery);
    OASIS_CHECK(dytto.projectiles().size() == 1U);
    OASIS_CHECK(dytto.projectiles()[0].state == 0x000CU);
    OASIS_CHECK(dytto.projectiles()[0].action == 0x0006U);
    OASIS_CHECK(dytto.projectiles()[0].hitbox_radius == 8);
    OASIS_CHECK(dytto.projectiles()[0].vz_fp == -0x00050000);
    dytto.update_frame(100, 100, {}); // -> FollowPlayer (wander_timer 10)
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::FollowPlayer);

    // Ali Contact Buff (State 0x16 -> 0x18)
    dytto.init_spawn(130, 100, 0, 2U, false);
    dytto.set_state(DyttoSpiritController::State::ReturnToAli);
    for (int f = 0; f < 25; ++f) {
        dytto.update_frame(100, 100, {});
        if (dytto.ali_buff_triggered()) break;
    }
    OASIS_CHECK(dytto.ali_buff_triggered());
    OASIS_CHECK(dytto.last_callback_addr() == 0x00014C04U);

    // Knockback Recovery (State 0x0E)
    dytto.trigger_knockback(0x00010000, -0x00010000, 0x00014000);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::KnockbackRecovery);
    for (int f = 0; f < 10; ++f) {
        dytto.update_frame(100, 100, {});
    }
    OASIS_CHECK(dytto.kb_vx_fp() == 0 && dytto.kb_vy_fp() == 0 && dytto.kb_vz_fp() == 0);
    OASIS_CHECK(dytto.state() == DyttoSpiritController::State::FollowPlayer);

    std::cout << "[PASS] Phase 2: 3-Way Spread, Magic Burst, Water Damage (0x2008/0xA008), Buff & Knockback Verified.\n";
}

void test_town_sat_rendering(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim->initialize_native_from_rom(rom, game::world::kBeachMapId));
    sim->transition_to_town(true);
    OASIS_CHECK(sim->map_id() == game::world::kTownMapId);

    sim->spawn_dytto_awakening(160, 460, 0);
    OASIS_CHECK(sim->dytto_controller().is_active());

    game::FrameInput in{};
    genesis::VdpState vdp{};
    // Step 2 frames so Town 30Hz SAT update (tick_counter % 2 == 0) executes
    sim->step_frame(in, vdp);
    sim->step_frame(in, vdp);
    OASIS_CHECK(has_sat_tile(sim->sat_pipeline(), 0x04D0U));

    std::cout << "[PASS] Phase 3: Town-Mode SAT Rendering of Dytto (0x04D0) Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.54: DYTTO ABILITIES & TOWN SAT RENDERING\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_autonomous_dispatch_0x00();
    test_spread_burst_damage_buff_knockback();
    test_town_sat_rendering(rom);
    std::cout << "\n[PASS] All M16.54 Dytto Abilities & Town SAT Tests PASSED!\n";
    return 0;
}
