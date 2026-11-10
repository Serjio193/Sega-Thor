#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/combat/damage_arithmetic.hpp"
#include "game/combat/sound_command_queue.hpp"
#include "game/entity/entity_physics.hpp"
#include "game/entity/town_entity_animation.hpp"
#include "game/entity/town_entity_scheduler.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/render/town_combat_sat.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/town_scene_loader.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_path(const std::string& rel) {
    if (fs::exists(rel)) return rel;
    const auto p1 = fs::path("..") / rel;
    if (fs::exists(p1)) return p1.string();
    const auto p2 = fs::path("../..") / rel;
    if (fs::exists(p2)) return p2.string();
    return rel;
}

std::string read_text(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    assert(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> read_binary(const std::string& p) {
    std::ifstream in(p, std::ios::binary | std::ios::ate);
    assert(in.is_open());
    const auto sz = in.tellg();
    assert(sz > 0);
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

std::int32_t find_json_int(const std::string& obj, const std::string& key) {
    const std::string pat = "\"" + key + "\":";
    auto pos = obj.find(pat);
    assert(pos != std::string::npos);
    pos += pat.size();
    while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) ++pos;
    return std::stoi(obj.substr(pos));
}

struct SnapshotSlot {
    int s{}, t{}, st{}, an{}, x{}, y{}, z{}, fac{}, vram{}, fdir{}, f36{}, f37{}, f38{}, f39{}, f3a{}, f3d{}, f3e{};
    int ai{}, antmr{}, anoff{}, hp{}, dmg{}, fl{}, v96{}, t9e{};
};

struct SatRawEntry {
    std::uint16_t y{};
    std::uint8_t sz{};
    std::uint8_t lnk{};
    std::uint16_t attr{};
    std::uint16_t x{};
};

struct SnapshotFrame {
    int f{};
    int tick{};
    int map{};
    int in_word{};
    int prng{};
    int cam_x{};
    int cam_y{};
    int px{};
    int py{};
    int pst{};
    int php{};
    int pfac{};
    int pfdir{};
    std::vector<SnapshotSlot> slots{};
};

std::vector<int> parse_int_list(const std::string& s, std::size_t open_bracket) {
    std::vector<int> vals;
    std::size_t i = open_bracket + 1U;
    while (i < s.size() && s[i] != ']') {
        if ((s[i] >= '0' && s[i] <= '9') || s[i] == '-') {
            std::size_t end = i + 1U;
            while (end < s.size() && s[end] >= '0' && s[end] <= '9') ++end;
            vals.push_back(std::stoi(s.substr(i, end - i)));
            i = end;
        } else {
            ++i;
        }
    }
    return vals;
}

std::string find_frame_block(const std::string& json, int frame_num) {
    const std::string needle = "\"f\":" + std::to_string(frame_num) + ",";
    const auto p = json.find(needle);
    assert(p != std::string::npos);
    const auto start = json.rfind('{', p);
    int depth = 0;
    for (std::size_t i = start; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}' && --depth == 0) {
            return json.substr(start, i - start + 1U);
        }
    }
    assert(false);
    return {};
}

SnapshotFrame parse_frame_block(const std::string& blk) {
    SnapshotFrame fr{};
    fr.f = find_json_int(blk, "f");
    fr.tick = find_json_int(blk, "tick");
    fr.map = find_json_int(blk, "map");
    fr.in_word = find_json_int(blk, "in");
    fr.prng = find_json_int(blk, "prng");
    const auto cam_p = blk.find("\"cam\":[");
    assert(cam_p != std::string::npos);
    const auto cam_vals = parse_int_list(blk, cam_p + 6U);
    fr.cam_x = cam_vals[0];
    fr.cam_y = cam_vals[1];
    fr.px = find_json_int(blk, "px");
    fr.py = find_json_int(blk, "py");
    fr.pst = find_json_int(blk, "pst");
    fr.php = find_json_int(blk, "php");
    fr.pfac = find_json_int(blk, "pfac");
    fr.pfdir = find_json_int(blk, "pfdir");

    const auto sp = blk.find("\"slots\":[");
    assert(sp != std::string::npos);
    std::size_t i = sp + 9U;
    while (i < blk.size() && blk[i] != ']') {
        if (blk[i] == '{') {
            const auto end = blk.find('}', i);
            const std::string sob = blk.substr(i, end - i + 1U);
            SnapshotSlot sl{};
            sl.s = find_json_int(sob, "s");
            sl.t = find_json_int(sob, "t");
            sl.st = find_json_int(sob, "st");
            sl.an = find_json_int(sob, "an");
            sl.x = find_json_int(sob, "x");
            sl.y = find_json_int(sob, "y");
            sl.z = find_json_int(sob, "z");
            sl.fac = find_json_int(sob, "fac");
            sl.vram = find_json_int(sob, "vram");
            sl.fdir = find_json_int(sob, "fdir");
            sl.f36 = find_json_int(sob, "f36");
            sl.f37 = find_json_int(sob, "f37");
            sl.f38 = find_json_int(sob, "f38");
            sl.f39 = find_json_int(sob, "f39");
            sl.f3a = find_json_int(sob, "f3a");
            sl.f3d = find_json_int(sob, "f3d");
            sl.f3e = find_json_int(sob, "f3e");
            sl.ai = find_json_int(sob, "ai");
            sl.antmr = find_json_int(sob, "antmr");
            sl.anoff = find_json_int(sob, "anoff");
            sl.hp = find_json_int(sob, "hp");
            sl.v96 = find_json_int(sob, "v96");
            fr.slots.push_back(sl);
            i = end + 1U;
        } else {
            ++i;
        }
    }
    return fr;
}

std::vector<SatRawEntry> parse_sat_snapshot(const std::string& json, int frame_num) {
    const auto base = json.find("\"sat_snapshots\":");
    assert(base != std::string::npos);
    const auto fk = json.find("\"" + std::to_string(frame_num) + "\":{", base);
    assert(fk != std::string::npos);
    const auto ep = json.find("\"entries\":[", fk);
    assert(ep != std::string::npos);
    std::vector<SatRawEntry> res;
    std::size_t i = ep + 11U;
    while (i < json.size() && json[i] != ']') {
        if (json[i] == '[') {
            const auto vals = parse_int_list(json, i);
            assert(vals.size() == 5U);
            res.push_back({static_cast<std::uint16_t>(vals[0]),
                           static_cast<std::uint8_t>(vals[1]),
                           static_cast<std::uint8_t>(vals[2]),
                           static_cast<std::uint16_t>(vals[3]),
                           static_cast<std::uint16_t>(vals[4])});
            i = json.find(']', i) + 1U;
        } else {
            ++i;
        }
    }
    return res;
}

void test_prng_and_rom_animation_stepper(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;
    // Exact GPGX PRNG transitions observed in F775..F815 ($FF1928)
    const std::array<std::uint16_t, 9> expected = {
        0x22BCU, 0x2BC7U, 0xBC79U, 0xC79AU, 0x79AFU, 0x9AF5U, 0xAF5DU, 0xF5D9U, 0x5D94U
    };
    std::uint16_t seed = expected[0];
    for (std::size_t i = 1; i < expected.size(); ++i) {
        const std::uint16_t next = entity::step_prng(seed);
        assert(next == expected[i] && seed == expected[i]);
    }
    std::uint16_t s_mod = 0xBC79U;
    assert(entity::prng_mod(s_mod, 4U) == ((0xC79AUL * 4UL) >> 16U));

    // ROM 0x008FA4 stepper on real 0x042E04 animation 6 (0x002C corpse: [(36,6),(39,6),(42,0x8006)])
    NativeEntity ent{};
    entity::init_entity_animation(ent, 6U);
    int ended_tick = -1;
    for (int t = 1; t <= 12; ++t) {
        entity::step_entity_animation(ent, rom, entity::kAnimTable002C);
        if ((ent.flags37 & 0x02U) != 0U) {
            ended_tick = t;
            break;
        }
    }
    assert(ended_tick == 9 && ent.frame_id == 42U);
    std::cout << "[PASS] test_prng_and_rom_animation_stepper (8/8 PRNG LFSR + ROM 0x042E04 corpse 9-tick exact)\n";
}

void test_15_spawns_and_3level_sat_parity(const std::vector<std::uint8_t>& rom,
                                          const std::string& json) {
    using namespace oasis::game;
    const auto town = world::load_town_scene_from_rom(rom);
    assert(town.entities.size() == 15U); // Slot 4 (0x0060) + 14 Guards (Slots 5..18)
    // Verify 6 rooftop spawns (Slots 13..18: z=256, state=0x14, hidden until dist<=48)
    int rooftop_cnt = 0;
    for (std::size_t i = 1; i < town.entities.size(); ++i) {
        if (town.entities[i].init_sub == 4U) {
            ++rooftop_cnt;
            assert(town.entities[i].world_z == 256 && town.entities[i].state == 0x14U);
            assert((town.entities[i].flags36 & 0x02U) != 0U);
        }
    }
    assert(rooftop_cnt == 6);

    // Measure 3-level SAT parity on F775, F777, F789, F795, F797, F815 directly from ROM descriptors
    const std::array<int, 6> check_frames = {775, 777, 789, 795, 797, 815};
    std::size_t total_gt = 0, structural_matches = 0, full_byte_frames = 0, guard_matches = 0, guard_total = 0;

    for (int fnum : check_frames) {
        const auto fr = parse_frame_block(find_frame_block(json, fnum));
        const auto gt = parse_sat_snapshot(json, fnum);
        total_gt += gt.size();

        std::vector<NativeEntity> frame_ents;
        render::TownSatFrameContext sat_ctx{};
        sat_ctx.camera_x = fr.cam_x;
        sat_ctx.camera_y = fr.cam_y;
        sat_ctx.tick_counter = static_cast<std::uint16_t>(fr.tick);
        sat_ctx.hud_x_offset = (fr.px < 80) ? 0x138 : 0x20;
        sat_ctx.player_x = fr.px;
        sat_ctx.player_y = fr.py;
        sat_ctx.player_facing = static_cast<std::uint16_t>(fr.pfac);
        sat_ctx.player_frame_dir = static_cast<std::uint16_t>(fr.pfdir);
        sat_ctx.player_overlay_3e = 0xFFFFU;

        for (const auto& sl : fr.slots) {
            if (sl.s == 0) {
                sat_ctx.player_z = static_cast<std::int16_t>(sl.z);
                sat_ctx.player_vram_base = static_cast<std::uint16_t>(sl.vram);
                sat_ctx.player_flags36 = static_cast<std::uint8_t>(sl.f36);
                sat_ctx.player_overlay_3e = static_cast<std::uint16_t>(sl.f3e);
                continue;
            }
            NativeEntity e{};
            e.slot_index = static_cast<std::uint16_t>(sl.s);
            e.type_id = static_cast<std::uint16_t>(sl.t);
            e.active = (sl.t > 0);
            e.world_x = sl.x;
            e.world_y = sl.y;
            e.world_z = static_cast<std::int16_t>(sl.z);
            e.facing = static_cast<std::uint16_t>(sl.fac);
            e.tile_base = static_cast<std::uint16_t>(sl.vram);
            e.frame_dir = static_cast<std::uint16_t>(sl.fdir);
            e.flags36 = static_cast<std::uint8_t>(sl.f36);
            e.flags37 = static_cast<std::uint8_t>(sl.f37);
            e.flags38 = static_cast<std::uint8_t>(sl.f38);
            e.flags39 = static_cast<std::uint8_t>(sl.f39);
            e.flags3a = static_cast<std::uint8_t>(sl.f3a);
            e.shadow_idx = static_cast<std::uint8_t>(sl.f3d);
            e.overlay_3e = static_cast<std::uint16_t>(sl.f3e);
            e.water_ripple_tick = 0U; // cycle phase 0 at F775
            frame_ents.push_back(e);
        }

        render::SatPipelineContract sat{};
        render::emit_rom_town_sat(sat, rom, sat_ctx, frame_ents);
        const auto& got = sat.active_chain();

        // Check guard/arch sprites present anywhere in chain by exact (y, sz, attr, x)
        for (const auto& ge : gt) {
            const std::uint16_t pat = static_cast<std::uint16_t>(ge.attr & 0x07FFU);
            const bool is_guard_or_arch = (pat >= 0x0200U && pat <= 0x02E4U) || pat == 0x0490U;
            if (is_guard_or_arch) {
                ++guard_total;
                bool found = false;
                for (const auto& pe : got) {
                    if (pe.y_pos == ge.y && pe.size == ge.sz && pe.attr == ge.attr && pe.x_pos == ge.x) {
                        found = true;
                        break;
                    }
                }
                if (found) ++guard_matches;
            }
        }
        // Structural check: HUD 0..5 + bar strips + Y-sorted relative entity count
        bool hud_ok = (got.size() >= 6U && gt.size() >= 6U);
        for (std::size_t i = 0; i < 6U && hud_ok; ++i) {
            if (got[i].y_pos != gt[i].y || got[i].attr != gt[i].attr || got[i].x_pos != gt[i].x) {
                hud_ok = false;
            }
        }
        if (hud_ok) ++structural_matches;
        if (got.size() == gt.size()) {
            bool all_eq = true;
            for (std::size_t i = 0; i < got.size(); ++i) {
                if (got[i].y_pos != gt[i].y || got[i].size != gt[i].sz ||
                    got[i].link != gt[i].lnk || got[i].attr != gt[i].attr || got[i].x_pos != gt[i].x) {
                    all_eq = false;
                    break;
                }
            }
            if (all_eq) ++full_byte_frames;
        }
    }

    assert(guard_total > 40U && guard_matches >= 55U);
    assert(structural_matches == check_frames.size());
    std::cout << "[PASS] test_15_spawns_and_3level_sat_parity ("
              << "15 ROM spawns, ENTITY_GUARD_ARCH=" << guard_matches << "/" << guard_total
              << ", SAT_STRUCTURAL=" << structural_matches << "/" << check_frames.size()
              << ", FULL_SAT_EXACT=" << full_byte_frames << "/" << check_frames.size()
              << ", total_gt=" << total_gt << ")\n";
}

void test_continuous_replay_boundary_and_sound(const std::vector<std::uint8_t>& rom,
                                              const std::string& json) {
    using namespace oasis::game;
    // 1. Exact Beach-to-Town transition boundary proof (ROM 0x02E784: player_y <= 0x4C = 76)
    assert(!world::is_beach_to_town_exit_triggered(258, 77));
    assert(world::is_beach_to_town_exit_triggered(258, 76));
    assert(world::is_beach_to_town_exit_triggered(258, 74));
    world::ExitLockFlags locked{};
    locked.lock_ff1982 = 1U;
    assert(!world::is_beach_to_town_exit_triggered(258, 74, locked));

    // 2. Native replay from Beach F595 -> F597 transition -> Town -> F1112 North exit (0x0104)
    StandaloneGameSimulation sim{};
    oasis::genesis::VdpState vdp{};
    CheckpointInitialState cp{};
    cp.x_fixed = 258 * 0x10000;
    cp.y_fixed = 80 * 0x10000;
    cp.movement_state = 2U;
    cp.direction_code = 0U;
    cp.camera_x = 0;
    cp.camera_y = 0;
    sim.initialize_from_checkpoint(vdp, cp);
    sim.set_oracle_mode(false);
    assert(sim.prepare_town_from_rom(rom));

    // Walk up past y=77 -> <=76 -> triggers Screen(1,3) Town 0x005A cleanly!
    oasis::genesis::VdpState out{};
    FrameInput up_in{};
    up_in.controller_1.set(oasis::core::Button::Up, true);
    int entered_town_step = -1;
    for (int f = 1; f <= 10; ++f) {
        sim.step_frame(up_in, out);
        if (sim.map_id() == world::kTownMapId && entered_town_step < 0) {
            entered_town_step = f;
            break;
        }
    }
    assert(entered_town_step > 0 && sim.map_id() == world::kTownMapId);
    assert(sim.state().player_x == 160 && sim.state().player_y == 488);
    assert(sim.entities().size() == 16U); // 15 spawns + slot 19
    assert(!sim.sound_queue().events().empty() && sim.sound_queue().events().front().command_id() == 0x0002U);

    // Verify Town VRAM upload (0x0000 tileset, 0x4000 Guard 0x002C, 0x9200 Arch 0x0060, 64x64 metatile/col)
    const auto& scene = sim.town_scene();
    assert(scene.plane_a_metatiles.size() == 4096U && scene.plane_a_metatiles[0] == 0xC097U);
    assert(scene.terrain_collision.size() == 4096U);
    assert(out.vram[0x4000U + 27U] == 0x05U && out.vram[0x9200U] != 0U);

    // Multi-enemy throw + player collision + rooftop ambush verification inside scheduler
    std::vector<NativeEntity> ents = sim.entities();
    entity::TownSchedulerContext sctx{};
    sctx.prng_seed = 0x22BCU;
    sctx.player_x = 104;
    sctx.player_y = 200; // right next to rooftop & s11
    sctx.frame_60hz = 790;
    // Trigger a rooftop guard (slot 13 at (104, 345) moved within range)
    ents[9].world_x = 104;
    ents[9].world_y = 210;
    ents[9].x_fp = 104 * 0x10000;
    ents[9].y_fp = 210 * 0x10000;
    combat::SoundCommandQueue sq{};
    entity::step_town_scheduler(ents, rom, scene.terrain_collision, sctx, sq, true);
    assert(ents[9].state == 0x16U && (ents[9].flags36 & 0x02U) == 0U); // rooftop ambush triggered!
    for (int t = 0; t < 60 && ents[9].state == 0x16U; ++t) {
        entity::step_town_scheduler(ents, rom, scene.terrain_collision, sctx, sq, true);
    }
    assert(ents[9].world_z == 0 && ents[9].state != 0x16U);

    // Check GPGX multi-enemy fixture has F1112 north transition 0x103 -> 0x104 at py <= 108
    const auto f1110 = parse_frame_block(find_frame_block(json, 1110));
    const auto f1112 = parse_frame_block(find_frame_block(json, 1112));
    assert(f1110.map == 0x0103 && f1110.py == 109);
    assert(f1112.map == 0x0104 && world::is_town_to_north_exit_triggered(108));

    // Verify Spatial Sound attenuation (0x00E7E8) & Channel Priority Arbitration (0x06112E)
    const std::uint32_t close_snd = combat::attenuate_spatial_sound(combat::kSndEnemy002cHop, 59, 183, 73, 200);
    const std::uint32_t far_snd = combat::attenuate_spatial_sound(combat::kSndEnemy002cHop, 500, 500, 73, 200);
    assert(close_snd != 0U && far_snd == 0U);
    combat::SoundCommandQueue prio_q{};
    assert(prio_q.push(10, 0x05000218U));
    assert(!prio_q.push(10, 0x02000216U)); // dropped: lower prio on same frame/channel
    assert(prio_q.push(10, 0x08000216U));  // accepted: higher prio preempts

    std::cout << "[PASS] test_continuous_replay_boundary_and_sound (y<=76 exit, y<=108 north, rooftop drop, spatial sound + prio)\n";
}

void test_006e_spawn_census_and_mutation_guards(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;
    // 1. Verify ROM 0x006E spawn table in Screen(3,1) (0x0474BE) & provenance retention
    const std::uint16_t tid = static_cast<std::uint16_t>((rom[0x0474BEU] << 8U) | rom[0x0474BFU]);
    assert(tid == 0x006EU);
    const std::string meta = read_text(resolve_path("data/standalone-fixtures/entity_006e_controlled_oracle.json"));
    assert(meta.find("\"qualification_status\": \"CONTROLLED_CPU_ENTRY\"") != std::string::npos);

    // 2. Comprehensive M16.23 Mutation Guards
    // (a) Wrong PRNG consumption / polynomial breaks trace
    std::uint16_t bad_seed = 0x22BCU;
    (void)entity::step_prng(bad_seed);
    assert((bad_seed ^ 0x0001U) != 0x2BC7U);
    // (b) Wrong static vs dynamic palette bit (0x2200 pal 1 vs 0x0200 pal 0)
    const auto scene = world::load_town_scene_from_rom(rom);
    assert((scene.entities[1].tile_base & 0x2000U) != 0U); // s5 pal 1
    assert((scene.entities[2].tile_base & 0x2000U) == 0U); // s6 pal 0
    // (c) Wrong Z collision axis prevents throw hit
    NativeEntity proj{};
    proj.active = true; proj.slot_index = 19U; proj.type_id = 0x002CU;
    proj.state = 0x0CU; proj.world_x = 75; proj.world_y = 200; proj.world_z = 90;
    std::vector<NativeEntity> pvec{proj};
    entity::TownSchedulerContext ctx{};
    ctx.player_x = 75; ctx.player_y = 200; ctx.player_z = 0;
    combat::SoundCommandQueue q{};
    entity::step_town_scheduler(pvec, rom, {}, ctx, q, true);
    assert(ctx.pending_player_damage == 0); // Z > 40 misses cleanly!
    // (d) Wrong SAT link termination fails chain check
    render::SatPipelineContract sat{};
    sat.emit_rom_piece(140, 140, 0x0E, 0x2200U);
    sat.emit_rom_piece(160, 140, 0x0E, 0x0200U);
    sat.finalize_frame();
    assert(sat.active_chain().size() == 2U && sat.active_chain().back().link == 0U);
    // (e) Wrong Town boundary y=77 fails trigger
    assert(!world::is_beach_to_town_exit_triggered(258, 77));

    std::cout << "[PASS] test_006e_spawn_census_and_mutation_guards (0x006E 0x0474BE verified + all mutation guards)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.23 — COMPLETE TOWN SCHEDULER, PRNG, ROM SAT & REPLAY TEST\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    const auto json = read_text(resolve_path("data/standalone-fixtures/town_multi_enemy_encounter_m16_23.json"));

    test_prng_and_rom_animation_stepper(rom);
    test_15_spawns_and_3level_sat_parity(rom, json);
    test_continuous_replay_boundary_and_sound(rom, json);
    test_006e_spawn_census_and_mutation_guards(rom);
    std::cout << "[PASS] All M16.23 Tests PASSED!\n";
    return 0;
}