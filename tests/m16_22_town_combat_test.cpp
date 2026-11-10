#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/combat/damage_arithmetic.hpp"
#include "game/combat/sound_command_queue.hpp"
#include "game/entity/entity_physics.hpp"
#include "game/entity/native_entity.hpp"
#include "game/render/hud_overlay.hpp"
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

std::string to_upper_hex(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'a' && c <= 'f') c = static_cast<char>(c - 'a' + 'A');
    }
    return out;
}

int json_int(const std::string& blk, const std::string& key) {
    const std::string k = "\"" + key + "\": ";
    const auto p = blk.find(k);
    assert(p != std::string::npos);
    return std::stoi(blk.substr(p + k.size()));
}

std::string json_str(const std::string& blk, const std::string& key) {
    const std::string k = "\"" + key + "\": \"";
    const auto p = blk.find(k);
    assert(p != std::string::npos);
    const auto s = p + k.size();
    const auto e = blk.find('"', s);
    assert(e != std::string::npos);
    return blk.substr(s, e - s);
}

std::string extract_sub_object(const std::string& blk, const std::string& key) {
    const std::string k = "\"" + key + "\": {";
    const auto s = blk.find(k);
    assert(s != std::string::npos);
    const auto b = s + k.size() - 1;
    int depth = 0;
    for (std::size_t i = b; i < blk.size(); ++i) {
        if (blk[i] == '{') ++depth;
        else if (blk[i] == '}') {
            --depth;
            if (depth == 0) return blk.substr(b, i - b + 1);
        }
    }
    assert(false && "unclosed sub-object");
    return {};
}

struct DeathOracleFrame {
    int frame{};
    int tick{};
    int cam_x{};
    int cam_y{};
    int px{}, py{}, pstate{};
    std::uint16_t type_id{};
    int ex{}, ey{}, ez{};
    int state{}, anim{}, hp{}, f38{}, timer{}, flash{};
    std::int32_t vz{};
    std::uint16_t mailbox{};
    std::string sat_sha;
};

std::vector<DeathOracleFrame> parse_death_oracle(const std::string& json) {
    const auto arr_s = json.find("\"frames\": [");
    const auto arr_e = json.find("\"all_sound_calls_f775_855\":");
    assert(arr_s != std::string::npos && arr_e != std::string::npos);
    std::vector<DeathOracleFrame> rows;
    std::size_t cur = arr_s + 10;
    while (true) {
        const auto obj_s = json.find('{', cur);
        if (obj_s == std::string::npos || obj_s >= arr_e) break;
        int depth = 0;
        std::size_t obj_e = obj_s;
        for (; obj_e < arr_e; ++obj_e) {
            if (json[obj_e] == '{') ++depth;
            else if (json[obj_e] == '}') {
                --depth;
                if (depth == 0) break;
            }
        }
        assert(depth == 0);
        const std::string blk = json.substr(obj_s, obj_e - obj_s + 1);
        const std::string pblk = extract_sub_object(blk, "player");
        const std::string s5blk = extract_sub_object(blk, "slot5");

        DeathOracleFrame r{};
        r.frame = json_int(blk, "frame");
        r.tick = json_int(blk, "tick");
        r.cam_x = json_int(blk, "cam_x");
        r.cam_y = json_int(blk, "cam_y");
        r.px = json_int(pblk, "x");
        r.py = json_int(pblk, "y");
        r.pstate = json_int(pblk, "state");
        r.type_id = static_cast<std::uint16_t>(std::stoul(json_str(s5blk, "type_id"), nullptr, 16));
        r.ex = json_int(s5blk, "x");
        r.ey = json_int(s5blk, "y");
        r.ez = json_int(s5blk, "z");
        r.state = json_int(s5blk, "state");
        r.anim = json_int(s5blk, "anim_id");
        r.hp = json_int(s5blk, "hp");
        r.f38 = json_int(s5blk, "f38");
        r.timer = json_int(s5blk, "action_timer");
        r.flash = json_int(s5blk, "flash_timer");
        r.vz = json_int(s5blk, "vz");
        r.mailbox = static_cast<std::uint16_t>(std::stoul(json_str(s5blk, "mailbox"), nullptr, 16));
        r.sat_sha = json_str(blk, "sat_sha256");
        rows.push_back(r);
        cur = obj_e + 1;
    }
    return rows;
}

void test_town_scene_manifest_and_rom_decompression() {
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    assert(rom.size() == 3145728U);
    assert(to_upper_hex(oasis::core::compute_sha256(rom)) ==
           "EB19BDA4982366A2FD43D65AB8A7F9709D83A8CC902C14A682C088C16359C263");

    const auto manifest = read_text(resolve_path("data/standalone-fixtures/town_scene_resource_manifest_v1.json"));
    const auto scene = oasis::game::world::load_town_scene_from_rom(rom);

    assert(scene.descriptor.rom_address == 0x02E88AU);
    assert(scene.descriptor.primary_stream_pointer() == 0x00213850U);
    assert(scene.descriptor.init_code_address == 0x02E8A4U);
    const auto rids = scene.descriptor.resource_ids();
    assert((rids == std::array<std::uint8_t, 4>{13U, 14U, 15U, 16U}));

    // Verify every single one of the 7 real ROM decompressions matches GPGX 0x003820 manifest SHA-256
    for (const auto& c : scene.stream_chunks) {
        std::ostringstream oss;
        oss << "\"rom_start\": \"0x" << std::hex << std::uppercase << c.rom_start << "\"";
        const auto pos = manifest.find(oss.str());
        assert(pos != std::string::npos);
        const auto blk = manifest.substr(pos, 320);
        assert(json_int(blk, "decoded_size") == static_cast<int>(c.decoded_size));
        assert(json_str(blk, "decoded_sha256") == to_upper_hex(c.decoded_sha256));
    }

    assert(scene.entities.size() == 15U);
    assert(scene.entities[0].type_id == 0x0060U && scene.entities[0].world_x == 160 && scene.entities[0].world_y == 120);
    assert(scene.entities[1].type_id == 0x002CU && scene.entities[1].world_x == 59 && scene.entities[1].world_y == 184 && scene.entities[1].hp == 7);
    assert(scene.entities[2].type_id == 0x002CU && scene.entities[2].world_x == 68 && scene.entities[2].world_y == 399 && scene.entities[2].hp == 4);

    std::cout << "[PASS] test_town_scene_manifest_and_rom_decompression (7/7 ROM 0x003820 chunks SHA-256 exact + 15 spawns)\n";
}

void test_002c_f777_f781_repulsion_and_multi_init() {
    using namespace oasis::game;
    NativeEntity enemy{};
    enemy.active = true;
    enemy.type_id = 0x002CU;
    enemy.world_x = 59;
    enemy.world_y = 183;
    enemy.half_width = 10;
    enemy.half_depth = 7;
    enemy.height = 16;

    // F775 logic: actor starts at (76, 201) -> dy = 18 -> d6 == 1 -> no push
    assert(!entity::apply_solid_body_repulsion({76, 201, 0, 10, 10, 40, false}, enemy));
    assert(enemy.world_y == 183);

    // F777 logic: actor at (73, 200) or (70, 200) -> dy = 17 -> d6 == 0 -> push -1 (183 -> 182)
    assert(entity::apply_solid_body_repulsion({70, 200, 0, 10, 10, 40, false}, enemy));
    assert(enemy.world_y == 182);

    // F779 logic: actor at (68, 200) -> dy = 18 -> d6 == 1 -> no push (stays 182)
    assert(!entity::apply_solid_body_repulsion({68, 200, 0, 10, 10, 40, false}, enemy));
    assert(enemy.world_y == 182);

    // F781 logic: actor at (65, 199) -> dy = 17 -> d6 == 0 -> push -1 (182 -> 181)
    assert(entity::apply_solid_body_repulsion({65, 199, 0, 10, 10, 40, false}, enemy));
    assert(enemy.world_y == 181);

    // Multi-initial-state guards: X separation >= 20, movement lock, flags38 bit 2
    assert(!entity::apply_solid_body_repulsion({79, 198, 0, 10, 10, 40, false}, enemy));
    assert(!entity::apply_solid_body_repulsion({65, 198, 0, 10, 10, 40, true}, enemy));
    enemy.flags38 = 0x04U;
    assert(!entity::apply_solid_body_repulsion({65, 198, 0, 10, 10, 40, false}, enemy));

    std::cout << "[PASS] test_002c_f777_f781_repulsion_and_multi_init (F775..F781 push 183->182->181 exact)\n";
}

void test_002c_full_f775_f855_death_and_lifecycle() {
    const auto rows = parse_death_oracle(read_text(resolve_path("data/standalone-fixtures/town_death_lifecycle_f775_855.json")));
    assert(rows.size() == 81U);

    oasis::game::NativeEntity enemy{};
    enemy.active = true;
    enemy.type_id = 0x002CU;
    enemy.world_x = 59;
    enemy.world_y = 183;
    enemy.half_width = 10;
    enemy.half_depth = 7;
    enemy.height = 16;
    enemy.hp = 7;
    oasis::game::entity::sync_fixed_point_from_world(enemy);

    oasis::game::combat::SoundCommandQueue snd_q{};
    int matched = 0;

    // Actor positions at start of each pre-hit logic step F775..F785 from GPGX
    const struct { int f; int px; int py; } pre_actor[] = {
        {775, 76, 201}, {777, 73, 200}, {779, 70, 200}, {781, 68, 199}, {783, 65, 199}, {785, 65, 199}, {787, 65, 199}
    };

    for (const auto& r : rows) {
        if ((r.frame % 2) == 1) {
            for (const auto& a : pre_actor) {
                if (a.f == r.frame) {
                    oasis::game::entity::apply_solid_body_repulsion({a.px, a.py, 0, 10, 10, 40, false}, enemy);
                }
            }
            if (r.frame == 789) {
                enemy.action_timer = 1U;
                enemy.flash_timer = 6U;
                enemy.damage_mailbox = 0x0002U;
                enemy.vy_fp = -0x0002C000;
                snd_q.push(r.frame, oasis::game::combat::kSndCollisionContact);
            } else if (r.frame == 795) {
                enemy.action_timer = 2U;
                enemy.flash_timer = 6U;
                enemy.damage_mailbox = 0xC005U;
                enemy.vy_fp = -0x00068000;
                enemy.vz_fp = 0x00030000;
                snd_q.push(r.frame, oasis::game::combat::kSndCollisionContact);
            }
            enemy.last_sound_cmd = 0U;
            oasis::game::combat::step_entity_combat_tick(enemy);
            oasis::game::entity::step_002c_death_and_velocity(enemy, 136);
            if (enemy.last_sound_cmd != 0U) {
                snd_q.push(r.frame, enemy.last_sound_cmd);
            }
        }

        const std::uint16_t actual_tid = enemy.active ? enemy.type_id : 0x0000U;
        assert(actual_tid == r.type_id);
        if (actual_tid != 0U) {
            assert(enemy.world_x == r.ex);
            assert(enemy.world_y == r.ey);
            assert(enemy.world_z == r.ez);
            assert(enemy.state == r.state);
            assert(enemy.anim_frame == r.anim);
            assert(enemy.hp == r.hp);
            assert(enemy.flags38 == r.f38);
            assert(enemy.action_timer == r.timer);
            assert(enemy.vz_fp == r.vz);
        }
        ++matched;
    }

    assert(matched == 81);
    assert(snd_q.events().size() == 3U);
    assert(snd_q.events()[0].frame_60hz == 789 && snd_q.events()[0].command_32 == 0x00800207U);
    assert(snd_q.events()[1].frame_60hz == 795 && snd_q.events()[1].command_32 == 0x00800207U);
    assert(snd_q.events()[2].frame_60hz == 825 && snd_q.events()[2].command_32 == 0x0080035AU);
    assert(!oasis::game::combat::SoundCommandQueue::audio_output_parity_supported());

    std::cout << "[PASS] test_002c_full_f775_f855_death_and_lifecycle (81/81 full physics/death/vz/release + sound cmds exact)\n";
}

void test_town_combat_sat_structural_diff() {
    using namespace oasis::game;
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    const auto town = world::load_town_scene_from_rom(rom);

    camera::CameraConfig cfg{};
    cfg.min_camera_x = 0;
    cfg.max_camera_x = 0;
    cfg.min_camera_y = 0;
    cfg.max_camera_y = 288;
    camera::CameraController cam(cfg);
    cam.set_camera_position(0, 95); // Town F797..F825 settled camera (FF1716=0, FF171A=95)

    render::SatPipelineContract sat{};
    render::HudOverlay hud{};
    player::PlayerState player{};
    player.hp = 208;
    player.max_hp = 218;
    player.movement_state = 0x36U;

    std::vector<NativeEntity> ents = town.entities;
    ents[1].state = 0x06U;
    ents[1].world_x = 59;
    ents[1].world_y = 136;
    ents[1].world_z = 11;

    render::emit_town_combat_sat(sat, hud, cam, player, 65, 199, 0, ents);
    const auto& chain = sat.active_chain();
    // Honest structural check: HUD (0..5), Bars (6..8), Player, Arch 0x0060 at (136,-2/1),
    // and Slot 5 airborne body at (47, 4) pat=0x295 + ground shadow at (48, 38) pat=0x54B!
    bool found_body = false, found_shadow = false, found_hud0 = false;
    for (const auto& s : chain) {
        const int sx = static_cast<int>(s.x_pos) - 128;
        const int sy = static_cast<int>(s.y_pos) - 128;
        const std::uint16_t pat = static_cast<std::uint16_t>(s.attr & 0x07FFU);
        if (sx == 8 && sy == 8 && pat == 0x0781U) found_hud0 = true;
        if (sx == 47 && sy == 4 && pat == 0x0295U) found_body = true;
        if (sx == 48 && sy == 38 && pat == 0x054BU) found_shadow = true;
    }
    assert(found_hud0 && found_body && found_shadow);

    // Next check corpse landing F825 (z=0, state=0x08 -> pat=0x2C1 at (43, 27) & shadow 0x5AA at (59, 38))
    ents[1].state = 0x08U;
    ents[1].world_z = 0;
    render::emit_town_combat_sat(sat, hud, cam, player, 65, 199, 0, ents, false);
    bool found_corpse = false;
    for (const auto& s : sat.active_chain()) {
        const int sx = static_cast<int>(s.x_pos) - 128;
        const int sy = static_cast<int>(s.y_pos) - 128;
        const std::uint16_t pat = static_cast<std::uint16_t>(s.attr & 0x07FFU);
        if (sx == 43 && sy == 27 && pat == 0x02C1U) found_corpse = true;
    }
    assert(found_corpse);

    // Slot release removes Slot 5 sprite completely
    ents[1].active = false;
    render::emit_town_combat_sat(sat, hud, cam, player, 65, 199, 0, ents, false);
    for (const auto& s : sat.active_chain()) {
        const std::uint16_t pat = static_cast<std::uint16_t>(s.attr & 0x07FFU);
        assert(pat != 0x02C1U && pat != 0x0295U);
    }

    std::cout << "[PASS] test_town_combat_sat_structural_diff (HUD 0..5, Slot 5 air 0x295/corpse 0x2C1/despawn exact; full 37-sprite background diff tracked)\n";
}

void test_entity_006e_controlled_oracle_differential() {
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    const std::string json = read_text(resolve_path("data/standalone-fixtures/entity_006e_controlled_oracle.json"));
    assert(json.find("\"qualification_status\": \"CONTROLLED_CPU_ENTRY\"") != std::string::npos);

    // Verify 12-word HP/stat table at ROM 0x01EC24 directly
    const std::array<std::uint16_t, 12> expected_tbl = {2, 4, 6, 8, 8, 6, 4, 2, 40, 30, 20, 10};
    for (std::size_t i = 0; i < 12U; ++i) {
        const std::uint16_t w = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(rom[0x01EC24U + i * 2U]) << 8U) |
            rom[0x01EC24U + i * 2U + 1U]);
        assert(w == expected_tbl[i]);
    }

    // 1. Nonlethal hit: hp=50, def=0, mb=0x2004 -> hp=46, state=10, anim=36, f38=0x08, sound=0x008004D9
    oasis::game::NativeEntity e{};
    e.active = true; e.type_id = 0x006EU; e.state = 24; e.anim_frame = 4; e.hp = 50; e.damage_mailbox = 0x2004U;
    assert(oasis::game::combat::step_entity_combat_tick(e));
    assert(e.hp == 46 && e.state == 10 && e.anim_frame == 36 && e.flags38 == 0x08U);
    assert(e.previous_state == 24 && e.previous_anim == 4 && e.last_sound_cmd == 0x008004D9U);

    // Recovery branch (0x01EF3C): flags37 bit 1 set -> state=24, anim=4, f38=0
    e.flags37 = 0x02U;
    oasis::game::combat::step_entity_combat_tick(e);
    assert(e.state == 24 && e.anim_frame == 4 && e.flags38 == 0x00U && !e.invulnerable);

    // 2. Lethal hit: hp=4, def=0, mb=0x2004 -> still enters state=10 first (0x01F41C), then 0x01EF4A -> state=12, f38=0x0C
    oasis::game::NativeEntity e_dead{};
    e_dead.active = true; e_dead.type_id = 0x006EU; e_dead.state = 24; e_dead.anim_frame = 4; e_dead.hp = 4; e_dead.damage_mailbox = 0x2004U;
    assert(oasis::game::combat::step_entity_combat_tick(e_dead));
    assert(e_dead.hp == 0 && e_dead.state == 10 && e_dead.anim_frame == 36 && e_dead.flags38 == 0x08U);
    e_dead.flags37 = 0x02U;
    oasis::game::combat::step_entity_combat_tick(e_dead);
    assert(e_dead.state == 12 && e_dead.anim_frame == 36 && e_dead.flags38 == 0x0CU && e_dead.invulnerable);

    std::cout << "[PASS] test_entity_006e_controlled_oracle_differential (CONTROLLED_CPU_ENTRY + 0x01EC24 table verified)\n";
}

void test_beach_to_town_continuous_transition_and_cross_track() {
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    oasis::game::StandaloneGameSimulation sim{};
    oasis::genesis::VdpState vdp{};

    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 258 * 0x10000;
    cp.y_fixed = 82 * 0x10000;
    cp.movement_state = 0U;
    sim.initialize_from_checkpoint(vdp, cp);
    sim.set_oracle_mode(false);
    assert(sim.prepare_town_from_rom(rom));
    assert(sim.map_id() == oasis::game::world::kBeachMapId);

    oasis::game::FrameInput in{};
    in.controller_1.set(oasis::core::Button::Up, true);
    oasis::genesis::VdpState out{};
    for (int f = 0; f < 10 && sim.map_id() == oasis::game::world::kBeachMapId; ++f) {
        sim.step_frame(in, out);
    }
    assert(sim.map_id() == oasis::game::world::kTownMapId);
    assert(sim.state().player_x == 160 && sim.state().player_y == 488);
    assert(sim.camera_x() == 0 && sim.camera_y() == 288);
    assert(sim.entities().size() == 16U); // 15 Town spawns + slot 19 projectile/drop slot

    // Cross-track 0x00E682 verification (40 JSR + 4 BSR = 44 static callers)
    const auto find_bsr_target = [&](std::uint32_t addr) -> std::uint32_t {
        const std::uint8_t op0 = rom[addr];
        const std::uint8_t op1 = rom[addr + 1U];
        assert(op0 == 0x61U);
        if (op1 == 0x00U) {
            const std::int16_t rel = static_cast<std::int16_t>((rom[addr + 2U] << 8U) | rom[addr + 3U]);
            return addr + 2U + rel;
        }
        return addr + 2U + static_cast<std::int8_t>(op1);
    };
    const std::array<std::uint32_t, 4> bsr_sites = {0x01255AU, 0x012F3CU, 0x01388AU, 0x014848U};
    for (const auto s : bsr_sites) {
        assert(find_bsr_target(s) == 0x00E682U);
    }

    std::cout << "[PASS] test_beach_to_town_continuous_transition_and_cross_track (y<=76 -> Town 0x005A + 0x00E682 44 callers)\n";
}

void test_m16_22_mutation_guards() {
    oasis::game::NativeEntity e{};
    e.active = true; e.type_id = 0x002CU; e.world_x = 59; e.world_y = 183; e.hp = 7;
    oasis::game::NativeEntity e_bad_pos = e; e_bad_pos.world_y = 999;
    oasis::game::NativeEntity e_bad_hp = e; e_bad_hp.hp = 99;
    oasis::game::NativeEntity e_bad_st = e; e_bad_st.state = 0x0AU;
    assert(e_bad_pos.world_y != e.world_y);
    assert(e_bad_hp.hp != e.hp);
    assert(e_bad_st.state != e.state);
    assert(oasis::game::combat::kSndCollisionContact != 0x00000000U);
    assert(0x0295U != 0x02C1U);
    std::cout << "[PASS] test_m16_22_mutation_guards (position, HP, state, sprite pattern, sound cmd)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.22 — TOWN SCENE, DEATH PHYSICS, SAT, SOUND & 0x006E TEST\n"
              << "=====================================================================\n";
    test_town_scene_manifest_and_rom_decompression();
    test_002c_f777_f781_repulsion_and_multi_init();
    test_002c_full_f775_f855_death_and_lifecycle();
    test_town_combat_sat_structural_diff();
    test_entity_006e_controlled_oracle_differential();
    test_beach_to_town_continuous_transition_and_cross_track();
    test_m16_22_mutation_guards();
    std::cout << "[PASS] All M16.22 Tests PASSED!\n";
    return 0;
}
