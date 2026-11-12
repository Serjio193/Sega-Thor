#include "core/sha256.hpp"
#include "game/entity/town_entity_scheduler.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/town_scene_loader.hpp"
#include "genesis/vdp_rasterizer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
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

std::string read_text(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    OASIS_CHECK(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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

std::vector<std::uint8_t> unhex(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t i = 0; i + 1U < hex.size(); i += 2U) {
        const char buf[3] = {hex[i], hex[i + 1U], '\0'};
        bytes.push_back(static_cast<std::uint8_t>(std::strtoul(buf, nullptr, 16)));
    }
    return bytes;
}

struct ParsedSnapshot {
    int frame{0};
    std::vector<std::uint8_t> vram{}, cram{}, vsram{}, regs{};
};

std::vector<ParsedSnapshot> parse_snapshots(const std::string& json_str) {
    std::vector<ParsedSnapshot> snaps;
    std::size_t pos = 0;
    while ((pos = json_str.find("\"f\":", pos)) != std::string::npos) {
        ParsedSnapshot s{};
        pos += 4U;
        while (pos < json_str.size() && (json_str[pos] == ' ' || json_str[pos] == ':')) ++pos;
        s.frame = std::stoi(json_str.substr(pos, 10U));
        const auto rpos = json_str.find("\"regs\":", pos);
        const auto rbracket = json_str.find("[", rpos);
        const auto rend = json_str.find("]", rbracket);
        std::stringstream ss(json_str.substr(rbracket + 1U, rend - (rbracket + 1U)));
        std::string item;
        while (std::getline(ss, item, ',')) {
            while (!item.empty() && item.front() == ' ') item.erase(0, 1);
            s.regs.push_back(static_cast<std::uint8_t>(std::stoul(item)));
        }
        auto get_hex = [&](const std::string& key) -> std::vector<std::uint8_t> {
            const auto kp = json_str.find("\"" + key + "\":", pos);
            const auto q1 = json_str.find("\"", kp + key.size() + 3U);
            const auto q2 = json_str.find("\"", q1 + 1U);
            return unhex(json_str.substr(q1 + 1U, q2 - (q1 + 1U)));
        };
        s.cram = get_hex("cram");
        s.vsram = get_hex("vsram");
        s.vram = get_hex("vram");
        snaps.push_back(std::move(s));
        pos = json_str.find("}", rend);
        if (pos != std::string::npos) ++pos;
    }
    return snaps;
}

oasis::genesis::VdpState to_vdp_state(const ParsedSnapshot& snap) {
    oasis::genesis::VdpState s{};
    std::copy(snap.vram.begin(), snap.vram.end(), s.vram.begin());
    std::copy(snap.cram.begin(), snap.cram.end(), s.cram.begin());
    std::copy(snap.vsram.begin(), snap.vsram.end(), s.vsram.begin());
    std::copy(snap.regs.begin(), snap.regs.end(), s.registers.begin());
    return s;
}

oasis::game::TownCheckpointInitialState build_cp(const std::vector<std::uint8_t>& cp_bin) {
    auto r8 = [&](std::size_t off) -> std::uint8_t { return cp_bin[0x10U + (off ^ 1U)]; };
    auto r16 = [&](std::size_t off) -> std::uint16_t { return static_cast<std::uint16_t>((r8(off) << 8U) | r8(off + 1U)); };
    auto rs16 = [&](std::size_t off) -> std::int16_t { return static_cast<std::int16_t>(r16(off)); };
    auto r32 = [&](std::size_t off) -> std::uint32_t { return (static_cast<std::uint32_t>(r16(off)) << 16U) | r16(off + 2U); };
    auto rs32 = [&](std::size_t off) -> std::int32_t { return static_cast<std::int32_t>(r32(off)); };

    oasis::game::TownCheckpointInitialState cp{};
    cp.x_fixed = 4808704; cp.y_fixed = 13107200;
    cp.movement_state = 4U; cp.direction_code = 0U;
    cp.camera_x = 0; cp.camera_y = 121;
    cp.tick_counter = 2391U; cp.frame_counter = 775U;
    cp.prng_seed = 0x22BCU; cp.turn_timer = 67U;
    cp.intent_y_fixed = -294912; cp.initial_walk_tick = 1;

    for (std::uint16_t s = 4U; s <= 19U; ++s) {
        const std::size_t b = 0x19E8U + static_cast<std::size_t>(s) * 0xBCU;
        oasis::game::NativeEntity ent{};
        ent.slot_index = s; ent.type_id = r16(b + 0x00U); ent.active = (ent.type_id != 0U);
        ent.ai_timer = r16(b + 0x02U); ent.state = static_cast<std::uint8_t>(r16(b + 0x04U));
        ent.x_fp = rs32(b + 0x08U); ent.y_fp = rs32(b + 0x0CU); ent.z_fp = rs32(b + 0x10U);
        ent.world_x = rs16(b + 0x08U); ent.world_y = rs16(b + 0x0CU); ent.world_z = rs16(b + 0x10U);
        ent.facing = r16(b + 0x16U); ent.tile_base = r16(b + 0x18U);
        ent.anim_frame = static_cast<std::uint8_t>(r16(b + 0x2AU));
        ent.previous_anim_id = r16(b + 0x2CU);
        ent.anim_step = r16(b + 0x2EU);
        ent.anim_key_off = r16(b + 0x30U);
        ent.frame_id = r16(b + 0x32U);
        ent.frame_dir = r16(b + 0x34U);
        ent.flags36 = r8(b + 0x36U); ent.flags37 = r8(b + 0x37U);
        ent.flags38 = r8(b + 0x38U); ent.flags39 = r8(b + 0x39U);
        ent.flags3a = r8(b + 0x3AU); ent.shadow_idx = r8(b + 0x3DU);
        ent.overlay_3e = r16(b + 0x3EU);
        ent.half_width = rs16(b + 0x42U); ent.half_depth = rs16(b + 0x44U);
        ent.base_half_w = r8(b + 0x48U); ent.base_half_d = r8(b + 0x49U);
        ent.height = rs16(b + 0x4AU);
        ent.action_timer = r8(b + 0x82U); ent.flash_timer = r8(b + 0x83U);
        ent.max_hp = rs16(b + 0x88U); ent.hp = rs16(b + 0x8AU);
        ent.defense = rs16(b + 0x92U); ent.variant_96 = r16(b + 0x96U);
        ent.water_ripple_tick = r8(b + 0x99U);
        const std::int16_t raw_9e = rs16(b + 0x9EU);
        ent.linked_slot_9e = (raw_9e > 0 && (raw_9e % 0xBC) == 0 && (raw_9e / 0xBC) < 20)
            ? static_cast<std::int16_t>(raw_9e / 0xBC) : std::int16_t{-1};
        ent.prng_mask_a0 = r16(b + 0xA0U);
        ent.invulnerable = (ent.flags38 & 0x08U) != 0U;
        cp.entities.push_back(ent);
    }
    return cp;
}

std::size_t count_non_black_pixels(const oasis::genesis::VdpState& vdp) {
    std::vector<std::uint8_t> rgb(320U * 224U * 3U, 0U);
    oasis::genesis::render_vdp_frame_rgb24(vdp, rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    std::size_t non_black = 0;
    for (std::size_t i = 0; i + 2U < rgb.size(); i += 3U) {
        if (rgb[i] != 0U || rgb[i + 1U] != 0U || rgb[i + 2U] != 0U) ++non_black;
    }
    return non_black;
}

oasis::game::FrameInput make_canonical_input(int f) {
    oasis::game::FrameInput in{};
    in.frame_index = static_cast<std::uint32_t>(f);
    if (f < 782) {
        in.controller_1.set(oasis::core::Button::Up, true);
        in.controller_1.set(oasis::core::Button::Left, true);
    } else if (f <= 784) {
        in.controller_1.set(oasis::core::Button::B, true);
        in.controller_1.set(oasis::core::Button::Up, true);
        in.controller_1.set(oasis::core::Button::Left, true);
    }
    return in;
}

// ----------------------------------------------------------------------------
// Phase 1 & 2: Native-Only End-to-End Playability Session (1,800 Frames / 30s)
// ----------------------------------------------------------------------------
void test_native_only_end_to_end_playability(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    game::StandaloneGameSimulation sim{};
    OASIS_CHECK(sim.initialize_native_from_rom(rom, game::world::kBeachMapId));
    OASIS_CHECK(sim.is_native_rom_session());
    OASIS_CHECK(!sim.oracle_mode());
    OASIS_CHECK(sim.map_id() == game::world::kBeachMapId);
    OASIS_CHECK(sim.current_vdp().is_display_enabled());

    // Render initial cold-boot Beach frame (zero GPGX checkpoint or VDP snapshot used)
    OASIS_CHECK(count_non_black_pixels(sim.current_vdp()) > 40000U);

    genesis::VdpState vdp{};
    std::size_t frames_simulated = 0;
    std::size_t player_move_steps = 0;
    std::size_t combat_actions = 0;
    std::size_t guard_reactions = 0;
    std::size_t rooftop_ambushes = 0;
    std::size_t guard_kills = 0;
    bool chest_opened = false;
    bool beach_slash_executed = false;
    bool visited_town = false;
    bool visited_town_north = false;
    bool returned_to_town = false;
    bool returned_to_beach = false;

    std::uint16_t prev_px = sim.state().player_x;
    std::uint16_t prev_py = sim.state().player_y;

    for (int f = 1; f <= 1800; ++f) {
        game::FrameInput in{};
        if (sim.map_id() == game::world::kBeachMapId && !visited_town) {
            if (f <= 11) {
                in.controller_1.set(core::Button::Up, true);
            } else if (f == 12) {
                in.controller_1.set(core::Button::A, true);
                in.controller_1.set(core::Button::B, true);
                ++combat_actions;
                beach_slash_executed = true;
            } else if (f >= 50) {
                in.controller_1.set(core::Button::Up, true);
            }
        } else if (sim.map_id() == game::world::kTownMapId && !visited_town_north) {
            visited_town = true;
            const int py = sim.state().player_y;
            const int px = sim.state().player_x;
            // Find the nearest active ground/landed guard to engage and defeat before advancing north
            const game::NativeEntity* target = nullptr;
            int best_dist = 999999;
            if (f < 480) {
                for (const auto& e : sim.entities()) {
                    if (e.active && e.type_id == 0x002CU && e.world_z == 0 && e.state != 0x08U && e.hp > 0) {
                        const int d = std::abs(e.world_x - px) + std::abs(e.world_y - py);
                        if (d < best_dist) { best_dist = d; target = &e; }
                    }
                }
            }
            if (target != nullptr) {
                const int goal_x = target->world_x;
                const int goal_y = target->world_y + 14;
                if (px < goal_x - 4) in.controller_1.set(core::Button::Right, true);
                else if (px > goal_x + 4) in.controller_1.set(core::Button::Left, true);
                else if (py > goal_y + 4) in.controller_1.set(core::Button::Up, true);
                else if (py < goal_y - 10) in.controller_1.set(core::Button::Down, true);
                else {
                    in.controller_1.set(core::Button::Up, true);
                    if ((f & 3) < 2) { in.controller_1.set(core::Button::B, true); ++combat_actions; }
                }
            } else {
                if (px < 105) in.controller_1.set(core::Button::Right, true);
                else if (px > 120) in.controller_1.set(core::Button::Left, true);
                in.controller_1.set(core::Button::Up, true);
            }
        } else if (sim.map_id() == game::world::kTownNorthMapId) {
            visited_town_north = true;
            if (sim.state().player_y > 420 && f < 950) {
                in.controller_1.set(core::Button::Up, true);
                if ((f % 16) == 0) { in.controller_1.set(core::Button::B, true); ++combat_actions; }
            } else {
                in.controller_1.set(core::Button::Down, true);
            }
        } else if (sim.map_id() == game::world::kTownMapId && visited_town_north) {
            returned_to_town = true;
            if (sim.state().player_x < 195) in.controller_1.set(core::Button::Right, true);
            in.controller_1.set(core::Button::Down, true);
        } else if (sim.map_id() == game::world::kBeachMapId && visited_town) {
            returned_to_beach = true;
            if ((f % 40) < 20) in.controller_1.set(core::Button::Down, true);
            else in.controller_1.set(core::Button::Right, true);
            if ((f % 45) == 0) { in.controller_1.set(core::Button::B, true); ++combat_actions; }
        }

        sim.step_frame(in, vdp);
        ++frames_simulated;

        if (sim.state().player_x != prev_px || sim.state().player_y != prev_py) {
            ++player_move_steps;
            prev_px = sim.state().player_x;
            prev_py = sim.state().player_y;
        }
        if (sim.chest_opened()) chest_opened = true;
        if (sim.map_id() == game::world::kTownMapId && !returned_to_town) {
            std::size_t alive_guards = 0;
            for (const auto& e : sim.entities()) {
                if (e.type_id == 0x002CU && e.active && e.state != 0x08U && e.hp > 0) {
                    ++alive_guards;
                    if (e.state == 0x02U || e.state == 0x04U || e.state == 0x06U || e.state == 0x0AU || e.state == 0x0CU) {
                        ++guard_reactions;
                    }
                    if (e.state == 0x16U || e.state == 0x18U) ++rooftop_ambushes;
                }
            }
            if (alive_guards < 14U) guard_kills = std::max(guard_kills, 14U - alive_guards);
        }
    }

    OASIS_CHECK(count_non_black_pixels(vdp) > 40000U);
    OASIS_CHECK(frames_simulated == 1800U);
    OASIS_CHECK(chest_opened && beach_slash_executed);
    OASIS_CHECK(visited_town && visited_town_north && returned_to_town && returned_to_beach);
    OASIS_CHECK(sim.transition_count() == 4U);
    OASIS_CHECK(player_move_steps > 400U);
    OASIS_CHECK(combat_actions >= 8U);
    OASIS_CHECK(guard_reactions > 50U);
    OASIS_CHECK(rooftop_ambushes > 0U);
    OASIS_CHECK(guard_kills >= 1U);

    std::cout << "[PASS] Phase 1 & 2: Native-Only Playability Session Verified ("
              << "Frames=" << frames_simulated << " [30.0s], MoveSteps=" << player_move_steps
              << ", CombatActions=" << combat_actions << ", GuardReactions=" << guard_reactions
              << ", RooftopAmbushes=" << rooftop_ambushes << ", GuardKills=" << guard_kills
              << ", Transitions=" << sim.transition_count() << " [0x003D->0x005A->0x005B->0x005A->0x003D])\n";
}

// ----------------------------------------------------------------------------
// Phase 5, 6 & 7: Systemic Graphics Differential & Holdout Regression Audit
// ----------------------------------------------------------------------------
void test_systemic_graphics_and_holdout_regression(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    const auto cp_bin = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));

    auto diff_px_count = [](const ParsedSnapshot& orig_snap, const genesis::VdpState& nat) -> std::size_t {
        const auto orig = to_vdp_state(orig_snap);
        std::vector<std::uint8_t> o_rgb(320U * 224U * 3U, 0U);
        std::vector<std::uint8_t> n_rgb(320U * 224U * 3U, 0U);
        genesis::render_vdp_frame_rgb24(orig, o_rgb, genesis::ColorDacMode::GenesisPlusGx);
        genesis::render_vdp_frame_rgb24(nat, n_rgb, genesis::ColorDacMode::GenesisPlusGx);
        std::size_t diff_px = 0;
        for (std::size_t p = 0; p < 320U * 224U; ++p) {
            if (o_rgb[p * 3U] != n_rgb[p * 3U] ||
                o_rgb[p * 3U + 1U] != n_rgb[p * 3U + 1U] ||
                o_rgb[p * 3U + 2U] != n_rgb[p * 3U + 2U]) ++diff_px;
        }
        return diff_px;
    };

    // 1. Verify 21-Frame Continuous Bit-For-Bit Exact Prefix (F775..F795)
    {
        const auto snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_canonical_continuous_f775_f795.json")));
        OASIS_CHECK(snaps.size() == 21U);
        auto cp = build_cp(cp_bin);
        cp.attract_banner_enabled = true;
        auto sim = std::make_unique<game::StandaloneGameSimulation>();
        OASIS_CHECK(sim->initialize_town_checkpoint(rom, cp, to_vdp_state(snaps.front())));
        sim->set_oracle_mode(false);
        genesis::VdpState out_vdp{};
        for (std::size_t idx = 0; idx < snaps.size(); ++idx) {
            const auto& snap = snaps[idx];
            if (snap.frame > 775) sim->step_frame(make_canonical_input(snap.frame), out_vdp);
            else out_vdp = sim->current_vdp();
            OASIS_CHECK(diff_px_count(snap, out_vdp) == 0U);
            for (std::size_t a = 0; a < 65536U; ++a) OASIS_CHECK(snap.vram[a] == out_vdp.vram[a]);
        }
    }

    // 2. Verify Canonical 7-Snapshot Sum (148,820) & Holdout 7-Snapshot Improvement (21,383 -> 21,110)
    const auto c_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json")));
    const auto h_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json")));

    std::size_t c_sum = 0;
    {
        auto c_cp = build_cp(cp_bin);
        c_cp.attract_banner_enabled = true;
        auto c_sim = std::make_unique<game::StandaloneGameSimulation>();
        OASIS_CHECK(c_sim->initialize_town_checkpoint(rom, c_cp, to_vdp_state(c_snaps[0])));
        c_sim->set_oracle_mode(false);
        genesis::VdpState out_vdp{};
        std::size_t c_next = 0;
        for (int f = 775; f <= 1115; ++f) {
            if (f > 775) c_sim->step_frame(make_canonical_input(f), out_vdp);
            else out_vdp = c_sim->current_vdp();
            if (c_next < c_snaps.size() && f == c_snaps[c_next].frame) {
                c_sum += diff_px_count(c_snaps[c_next], out_vdp);
                ++c_next;
            }
        }
        OASIS_CHECK(c_next == 7U && c_sum == 148820U);
    }

    std::vector<std::size_t> hold_px{};
    std::size_t h_sum = 0;
    {
        auto h_cp = build_cp(cp_bin);
        h_cp.attract_banner_enabled = false;
        auto h_sim = std::make_unique<game::StandaloneGameSimulation>();
        OASIS_CHECK(h_sim->initialize_town_checkpoint(rom, h_cp, to_vdp_state(h_snaps[0])));
        h_sim->set_oracle_mode(false);
        genesis::VdpState out_vdp{};
        std::size_t h_next = 0;
        for (int f = 775; f <= 1125; ++f) {
            if (f > 775) {
                game::FrameInput in{};
                in.frame_index = static_cast<std::uint32_t>(f);
                if (f >= 776 && f <= 818) in.controller_1.set(core::Button::Right, true);
                if (f >= 819 && f <= 822) { in.controller_1.set(core::Button::B, true); in.controller_1.set(core::Button::Right, true); }
                if (f >= 855 && f <= 858) { in.controller_1.set(core::Button::B, true); in.controller_1.set(core::Button::Right, true); }
                if (f >= 890 && f <= 935) { in.controller_1.set(core::Button::Right, true); in.controller_1.set(core::Button::Down, true); }
                if (f >= 936 && f <= 939) { in.controller_1.set(core::Button::B, true); in.controller_1.set(core::Button::Right, true); }
                if (f >= 975 && f <= 978) { in.controller_1.set(core::Button::B, true); in.controller_1.set(core::Button::Right, true); }
                h_sim->step_frame(in, out_vdp);
            } else {
                out_vdp = h_sim->current_vdp();
            }
            if (h_next < h_snaps.size() && f == h_snaps[h_next].frame) {
                const std::size_t px = diff_px_count(h_snaps[h_next], out_vdp);
                hold_px.push_back(px);
                h_sum += px;
                ++h_next;
            }
        }
        OASIS_CHECK(h_next == 7U);
    }

    // Holdout per-snapshot exact verification (F775, F809, F825, F833, F893, F941, F1125):
    OASIS_CHECK(hold_px[0] == 0U);
    OASIS_CHECK(hold_px[1] == 3649U); // Improved from 3,708 (M16.42/M16.43) & 3,678 (M16.41) via ROM 0x025B0E mid-air z=40 explosion!
    OASIS_CHECK(hold_px[2] == 2576U); // Improved from 2,597 (M16.41/M16.42) via ROM 0x02EE2C hitstun jitter (flash_timer & 1)!
    OASIS_CHECK(hold_px[3] == 2592U); // Improved from 2,806 (M16.43) & 2,702 (M16.41) via ROM 0x058012 per-frame weapon AABB hit sparks!
    OASIS_CHECK(hold_px[4] == 3948U);
    OASIS_CHECK(hold_px[5] == 4259U);
    OASIS_CHECK(hold_px[6] == 4086U);
    OASIS_CHECK(h_sum == 21110U);    // Net -273 px vs M16.43 (21,383) and -160 px vs M16.41 (21,270)!

    // 3. Mutation Guards: ROM 0x025B0E mid-air explosion & ROM 0x058012 weapon AABB hit sparks
    {
        game::NativeEntity bomb{};
        bomb.active = true;
        bomb.type_id = 0x002CU;
        bomb.state = 0x0CU;
        bomb.x_fp = 200 << 16; bomb.y_fp = 228 << 16; bomb.z_fp = 40 << 16;
        bomb.world_x = 200; bomb.world_y = 228; bomb.world_z = 40;
        std::vector<game::NativeEntity> ents = {bomb};
        game::entity::TownSchedulerContext ctx{};
        ctx.player_x = 200;
        ctx.player_y = 228;
        ctx.player_z = 0; // Grounded player -> ROM 0x025B0A tst.l $10(a0); beq 0x025AB4 preserves bomb z_fp!
        game::combat::SoundCommandQueue sq{};
        std::vector<std::uint8_t> tc(64U * 64U, 0U);
        game::entity::step_town_scheduler(ents, rom, tc, ctx, sq, true);
        OASIS_CHECK(ents[0].state == 0x0EU && (ents[0].z_fp >> 16) == 40);

        game::NativeAttackController atk;
        atk.reset();
        game::player::PlayerState pl{};
        pl.direction_code = 0U; // Facing Up -> must NOT use hardcoded (63, 167) spark coords!
        std::vector<game::NativeEntity> guards(1);
        guards[0].active = true;
        guards[0].type_id = 0x002CU;
        guards[0].state = 0x04U;
        guards[0].world_x = 200; guards[0].world_y = 212;
        guards[0].half_width = 12; guards[0].half_depth = 12; guards[0].height = 36;
        guards[0].hp = 40; guards[0].defense = 0x008A;
        std::array<game::render::TownEffect, 4> effects{};
        core::ControllerState ctrl{};
        ctrl.set(core::Button::B, true);
        ctrl.set(core::Button::Up, true);
        for (int s = 0; s < 7; ++s) {
            atk.update_town_30hz(pl, 200, 228, ctrl, guards, effects);
        }
        bool spark_spawned = false;
        for (const auto& ef : effects) {
            if (ef.active) {
                spark_spawned = true;
                OASIS_CHECK(!(ef.world_x == 63 && ef.world_y == 167));
            }
        }
        OASIS_CHECK(spark_spawned && game::world::is_south_exit_triggered(500));
    }

    std::cout << "[PASS] Phase 5, 6 & 7: 21-Frame Exact Prefix (F775..F795) + Holdout Audit Verified ("
              << "CanonSum=" << c_sum << ", HoldSum=" << h_sum
              << " [F809=" << hold_px[1] << ", F825=" << hold_px[2] << ", F833=" << hold_px[3] << "])\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.44: STANDALONE PLAYABILITY & HOLDOUT REGRESSION AUDIT\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_native_only_end_to_end_playability(rom);
    test_systemic_graphics_and_holdout_regression(rom);
    std::cout << "\n[PASS] All M16.44 Tests PASSED successfully!\n";
    return 0;
}
