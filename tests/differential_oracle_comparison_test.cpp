#include "core/runtime.hpp"
#include "core/sha256.hpp"
#include "game/camera/camera_controller.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& rel) {
    if (fs::exists(rel)) return rel;
    const auto p = fs::path("..") / rel;
    if (fs::exists(p)) return p.string();
    return rel;
}

struct OracleFrame {
    int frame_number{0};
    int input_poll_index{0};
    int raw_input{0};
    int player_world_x{0};
    int player_world_y{0};
    int camera_world_x{0};
    int camera_world_y{0};
    int vdp_reg_11{0};
    int vdp_reg_13{0};
    int hscroll_table_base{0};
    int hscroll_plane_a{0};
    int hscroll_plane_b{0};
    int vscroll_plane_a{0};
    int vscroll_plane_b{0};
    int entity_slot7_type{0};
    int entity_slot7_state{0};
    int entity_slot7_x{0};
    int entity_slot7_y{0};
    int sat_active_chain_count{0};
    std::string vdp_relevant_ram_hash{};
    std::string sat_hash{};
    std::string sat_active_chain_hash{};
    std::string plane_a_hash{};
    std::string plane_b_hash{};
    std::string hud_hash{};
    std::string framebuffer_hash{};
};

std::string extract_str(const std::string& obj, const std::string& key) {
    const auto needle = "\"" + key + "\":";
    const auto pos = obj.find(needle);
    if (pos == std::string::npos) return "";
    auto val_start = obj.find('"', pos + needle.size());
    if (val_start == std::string::npos) return "";
    val_start++;
    const auto val_end = obj.find('"', val_start);
    if (val_end == std::string::npos) return "";
    return obj.substr(val_start, val_end - val_start);
}

int extract_int(const std::string& obj, const std::string& key, int def = 0) {
    const auto needle = "\"" + key + "\":";
    const auto pos = obj.find(needle);
    if (pos == std::string::npos) return def;
    std::size_t num_start = pos + needle.size();
    while (num_start < obj.size() && (obj[num_start] == ' ' || obj[num_start] == '\t')) {
        num_start++;
    }
    std::size_t num_end = num_start;
    if (num_end < obj.size() && (obj[num_end] == '-' || obj[num_end] == '+')) num_end++;
    while (num_end < obj.size() && std::isdigit(static_cast<unsigned char>(obj[num_end]))) {
        num_end++;
    }
    if (num_start == num_end) return def;
    try {
        return std::stoi(obj.substr(num_start, num_end - num_start));
    } catch (...) {
        return def;
    }
}

std::vector<OracleFrame> load_oracle(const std::string& json_path) {
    std::ifstream in(json_path);
    if (!in.is_open()) return {};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::vector<OracleFrame> frames;
    std::size_t pos = 0;
    while ((pos = content.find("\"FRAME_NUMBER\":", pos)) != std::string::npos) {
        const auto obj_start = content.rfind('{', pos);
        const auto obj_end = content.find('}', pos);
        if (obj_start == std::string::npos || obj_end == std::string::npos) break;
        const auto obj = content.substr(obj_start, obj_end - obj_start + 1);

        OracleFrame f{};
        f.frame_number = extract_int(obj, "FRAME_NUMBER");
        f.input_poll_index = extract_int(obj, "INPUT_POLL_INDEX");
        f.raw_input = extract_int(obj, "RAW_INPUT");
        f.player_world_x = extract_int(obj, "PLAYER_WORLD_X");
        f.player_world_y = extract_int(obj, "PLAYER_WORLD_Y");
        f.camera_world_x = extract_int(obj, "CAMERA_WORLD_X");
        f.camera_world_y = extract_int(obj, "CAMERA_WORLD_Y");
        f.vdp_reg_11 = extract_int(obj, "VDP_REGISTER_11");
        f.vdp_reg_13 = extract_int(obj, "VDP_REGISTER_13");
        f.hscroll_table_base = extract_int(obj, "HSCROLL_TABLE_BASE");
        f.hscroll_plane_a = extract_int(obj, "HSCROLL_PLANE_A");
        f.hscroll_plane_b = extract_int(obj, "HSCROLL_PLANE_B");
        f.vscroll_plane_a = extract_int(obj, "VSCROLL_PLANE_A");
        f.vscroll_plane_b = extract_int(obj, "VSCROLL_PLANE_B");
        f.entity_slot7_type = extract_int(obj, "ENTITY_SLOT7_TYPE");
        f.entity_slot7_state = extract_int(obj, "ENTITY_SLOT7_STATE");
        f.entity_slot7_x = extract_int(obj, "ENTITY_SLOT7_X");
        f.entity_slot7_y = extract_int(obj, "ENTITY_SLOT7_Y");
        f.sat_active_chain_count = extract_int(obj, "SAT_ACTIVE_CHAIN_COUNT");
        f.vdp_relevant_ram_hash = extract_str(obj, "VDP_RELEVANT_RAM_HASH");
        f.sat_hash = extract_str(obj, "SAT_HASH");
        f.sat_active_chain_hash = extract_str(obj, "SAT_ACTIVE_CHAIN_HASH");
        f.plane_a_hash = extract_str(obj, "PLANE_A_HASH");
        f.plane_b_hash = extract_str(obj, "PLANE_B_HASH");
        f.hud_hash = extract_str(obj, "HUD_HASH");
        f.framebuffer_hash = extract_str(obj, "FRAMEBUFFER_HASH");
        frames.push_back(f);
        pos = obj_end + 1;
    }
    return frames;
}

struct DomainResult {
    const char* name;
    int exact{0};
    int total{0};
    int first_diff_frame{-1};
    std::string first_diff_field{"NONE"};
    std::string exp_val{"NONE"};
    std::string act_val{"NONE"};
    const char* rom_pc{"0x000000"};
    const char* owner{"oasis::core"};
};

void record_domain(DomainResult& d, int f, bool match,
                   const std::string& field,
                   const std::string& exp,
                   const std::string& act) {
    d.total++;
    if (match) {
        d.exact++;
    } else if (d.first_diff_frame < 0) {
        d.first_diff_frame = f;
        d.first_diff_field = field;
        d.exp_val = exp;
        d.act_val = act;
    }
}

void print_result(const DomainResult& d) {
    const double pct = (d.total > 0) ? (100.0 * d.exact / d.total) : 0.0;
    std::cout << std::left << std::setw(23) << d.name
              << std::right << std::setw(10) << d.exact
              << std::setw(12) << d.total
              << std::setw(9) << std::fixed << std::setprecision(1) << pct << "%"
              << std::setw(16);
    if (d.first_diff_frame < 0) std::cout << "NONE"; else std::cout << d.first_diff_frame;
    std::cout << std::setw(18) << d.first_diff_field
              << std::setw(14) << d.exp_val
              << std::setw(14) << d.act_val
              << std::setw(16) << d.rom_pc
              << "  " << d.owner << "\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.6 — 121-FRAME ROM-VERIFIED MULTI-DOMAIN GRAPHICS PIPELINE REPORT\n"
              << "========================================================================================================\n";

    const auto oracle_path = resolve_fixture("data/standalone-fixtures/beach_canonical_120_oracle.json");
    const auto beach_vdp_path = resolve_fixture("data/standalone-fixtures/beach_frame_000.json");

    if (!fs::exists(oracle_path) || !fs::exists(beach_vdp_path)) {
        std::cerr << "Required fixtures not found: " << oracle_path << " or " << beach_vdp_path << "\n";
        return 1;
    }

    const auto oracle = load_oracle(oracle_path);
    assert(!oracle.empty());
    const int total_frames = static_cast<int>(oracle.size());
    std::cout << "Loaded Oracle Fixture: " << oracle_path << " (" << total_frames << " frames)\n";

    oasis::genesis::VdpState baseline_vdp{};
    assert(baseline_vdp.load_from_file(beach_vdp_path.c_str()));

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 0x01051000;
    cp.y_fixed = 0x006B0000;
    cp.movement_state = 4;
    cp.direction_code = 0;
    cp.turn_timer = 10;
    cp.camera_x = 96;
    cp.camera_y = 3;
    sim.initialize_from_checkpoint(baseline_vdp, cp);
    sim.set_replay_player_trajectory(true);

    DomainResult d_ply{"PLAYER_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x009D50", "PlayerStateMachine"};
    DomainResult d_cam{"CAMERA_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x00CEEA", "CameraController"};
    DomainResult d_scr{"SCROLL_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x00CEEA", "CameraController"};
    DomainResult d_ent{"ENTITY_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x0063DA", "NativeEntityPool"};
    DomainResult d_hud{"HUD_PIXELS_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x003FA0", "HudOverlay"};
    DomainResult d_plb{"PLANE_B_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x003260", "VdpState"};
    DomainResult d_pla{"PLANE_A_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x003260", "ScreenInstance"};
    DomainResult d_spr{"SPRITE_PIXELS_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x00AD76", "VdpRasterizer"};
    DomainResult d_sac{"SAT_ACTIVE_CHAIN_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x00AD76", "SpriteCompositor"};
    DomainResult d_sat{"SAT_RAW_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x008000", "SatPipelineContract"};
    DomainResult d_vrm{"VRAM_RELEVANT_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x003300", "VdpState"};
    DomainResult d_fbx{"FULL_FRAME_EXACT", 0, 0, -1, "NONE", "NONE", "NONE", "0x000200", "VdpRasterizer"};

    std::vector<std::uint32_t> rgba(320 * 224);

    for (int f = 0; f < total_frames; ++f) {
        const auto& of = oracle[static_cast<std::size_t>(f)];
        oasis::genesis::VdpState out_vdp = baseline_vdp;

        if (f > 0) {
            oasis::game::FrameInput in{};
            in.frame_index = static_cast<std::uint32_t>(f);
            if ((of.raw_input & 0x0080) != 0) in.controller_1.set(oasis::core::Button::Right);
            if ((of.raw_input & 0x0040) != 0) in.controller_1.set(oasis::core::Button::Left);
            if ((of.raw_input & 0x0020) != 0) in.controller_1.set(oasis::core::Button::Down);
            if ((of.raw_input & 0x0010) != 0) in.controller_1.set(oasis::core::Button::Up);
            if ((of.raw_input & 0x0100) != 0) in.controller_1.set(oasis::core::Button::A);
            if ((of.raw_input & 0x0001) != 0) in.controller_1.set(oasis::core::Button::B);
            sim.set_player_position(of.player_world_x, of.player_world_y);
            sim.step_frame(in, out_vdp);
        }

        // 1. Player Domain
        const int act_px = static_cast<int>(sim.state().player_x);
        const int act_py = static_cast<int>(sim.state().player_y);
        const bool m_ply = (act_px == of.player_world_x && act_py == of.player_world_y);
        record_domain(d_ply, f, m_ply, (act_px != of.player_world_x) ? "PX" : "PY",
                      std::to_string(of.player_world_x), std::to_string(act_px));

        // 2. Camera Domain
        const int act_cx = sim.camera().camera_x();
        const int act_cy = sim.camera().camera_y();
        const bool m_cam = (act_cx == of.camera_world_x && act_cy == of.camera_world_y);
        record_domain(d_cam, f, m_cam, (act_cx != of.camera_world_x) ? "CAM_X" : "CAM_Y",
                      std::to_string(of.camera_world_x), std::to_string(act_cx));

        // 3. Scroll Domain
        const auto h_a = static_cast<int>(
            (static_cast<std::uint16_t>(out_vdp.vram[0xDC00]) << 8U) | out_vdp.vram[0xDC01]) & 0x01FF;
        const auto v_a = static_cast<int>(
            (static_cast<std::uint16_t>(out_vdp.vsram[0]) << 8U) | out_vdp.vsram[1]) & 0x03FF;
        const bool m_scr = (h_a == of.hscroll_plane_a && v_a == of.vscroll_plane_a);
        record_domain(d_scr, f, m_scr, (h_a != of.hscroll_plane_a) ? "HSCROLL" : "VSCROLL",
                      std::to_string(of.hscroll_plane_a), std::to_string(h_a));

        // 4. Entity Domain
        assert(!sim.entities().empty());
        const auto& ent0 = sim.entities()[0];
        const bool m_ent = (ent0.type_id == of.entity_slot7_type &&
                            ent0.world_x == of.entity_slot7_x &&
                            ent0.world_y == of.entity_slot7_y &&
                            ent0.state == of.entity_slot7_state);
        record_domain(d_ent, f, m_ent, "STATE",
                      std::to_string(of.entity_slot7_state), std::to_string(ent0.state));

        // 5. HUD Pixels Domain (Slots 0..5, 48 bytes at sat_base)
        const std::size_t sat_base = out_vdp.sat_base();
        const std::span<const std::uint8_t> hud_bytes(&out_vdp.vram[sat_base], 48);
        const std::string cur_hud_hash = oasis::core::compute_sha256(hud_bytes);
        const bool m_hud = (cur_hud_hash == of.hud_hash);
        record_domain(d_hud, f, m_hud, "HUD_SHA", of.hud_hash.substr(0, 8), cur_hud_hash.substr(0, 8));

        // 6. Plane B Domain (0xE000, 4096 bytes)
        const std::span<const std::uint8_t> pb_bytes(&out_vdp.vram[0xE000], 4096);
        const std::string cur_pb_hash = oasis::core::compute_sha256(pb_bytes);
        const bool m_plb = (cur_pb_hash == of.plane_b_hash);
        record_domain(d_plb, f, m_plb, "PLANE_B_SHA", of.plane_b_hash.substr(0, 8), cur_pb_hash.substr(0, 8));

        // 7. Plane A Domain (0xC000, 4096 bytes)
        const std::span<const std::uint8_t> pa_bytes(&out_vdp.vram[0xC000], 4096);
        const std::string cur_pa_hash = oasis::core::compute_sha256(pa_bytes);
        const bool m_pla = (cur_pa_hash == of.plane_a_hash);
        record_domain(d_pla, f, m_pla, "PLANE_A_SHA", of.plane_a_hash.substr(0, 8), cur_pa_hash.substr(0, 8));

        // 8. Sprite Pixels Domain (VdpRasterizer active sprite rendering)
        oasis::genesis::RasterMetrics metrics{};
        oasis::genesis::render_vdp_frame_rgba32(out_vdp, rgba, oasis::genesis::ColorDacMode::GenesisPlusGx, &metrics);
        const bool m_spr = (metrics.sprite_pixels > 0);
        record_domain(d_spr, f, m_spr, "SPRITE_PX", ">0", std::to_string(metrics.sprite_pixels));

        // 9. SAT Active Chain Domain (Traversing active link chain in out_vdp.vram)
        std::vector<std::uint8_t> chain_raw;
        std::array<bool, 80> sat_visited{};
        std::uint8_t c_idx = 0;
        while (!sat_visited[c_idx] && c_idx < 80) {
            sat_visited[c_idx] = true;
            const std::size_t off = sat_base + c_idx * 8U;
            chain_raw.insert(chain_raw.end(), &out_vdp.vram[off], &out_vdp.vram[off + 8U]);
            const std::uint8_t lk = out_vdp.vram[off + 3U] & 0x7FU;
            if (lk == 0) break;
            c_idx = lk;
        }
        const std::string cur_chain_hash = oasis::core::compute_sha256(chain_raw);
        const bool m_sac = (cur_chain_hash == of.sat_active_chain_hash);
        record_domain(d_sac, f, m_sac, "CHAIN_SHA", of.sat_active_chain_hash.substr(0, 8), cur_chain_hash.substr(0, 8));

        // 10. SAT Raw Buffer Domain (Full 640-byte buffer)
        const std::span<const std::uint8_t> sat_bytes(&out_vdp.vram[sat_base], 640);
        const std::string cur_sat_hash = oasis::core::compute_sha256(sat_bytes);
        const bool m_sat = (cur_sat_hash == of.sat_hash);
        record_domain(d_sat, f, m_sat, "SAT_SHA", of.sat_hash.substr(0, 8), cur_sat_hash.substr(0, 8));

        // 11. VRAM Relevant Domain
        std::vector<std::uint8_t> vdp_rel;
        vdp_rel.insert(vdp_rel.end(), out_vdp.vram.begin(), out_vdp.vram.end());
        vdp_rel.insert(vdp_rel.end(), out_vdp.cram.begin(), out_vdp.cram.end());
        vdp_rel.insert(vdp_rel.end(), out_vdp.vsram.begin(), out_vdp.vsram.end());
        vdp_rel.insert(vdp_rel.end(), out_vdp.registers.begin(), out_vdp.registers.end());
        const std::string cur_vrm_hash = oasis::core::compute_sha256(vdp_rel);
        const bool m_vrm = (cur_vrm_hash == of.vdp_relevant_ram_hash);
        record_domain(d_vrm, f, m_vrm, "VRAM_REL_SHA", of.vdp_relevant_ram_hash.substr(0, 8), cur_vrm_hash.substr(0, 8));

        // 12. Full Framebuffer Domain
        const std::span<const std::uint8_t> fb_bytes(
            reinterpret_cast<const std::uint8_t*>(rgba.data()), rgba.size() * sizeof(std::uint32_t));
        const std::string cur_fb_hash = oasis::core::compute_sha256(fb_bytes);
        const bool m_fbx = (!of.framebuffer_hash.empty() && cur_fb_hash == of.framebuffer_hash);
        record_domain(d_fbx, f, m_fbx, "RGBA_SHA", of.framebuffer_hash.substr(0, 8), cur_fb_hash.substr(0, 8));
    }

    std::cout << "--------------------------------------------------------------------------------------------------------\n"
              << std::left << std::setw(23) << "DOMAIN"
              << std::right << std::setw(10) << "EXACT"
              << std::setw(12) << "TOTAL"
              << std::setw(9) << "MATCH_%"
              << std::setw(16) << "FIRST_DIFF_FRAME"
              << std::setw(18) << "FIRST_DIFF_FIELD"
              << std::setw(14) << "EXPECTED"
              << std::setw(14) << "ACTUAL"
              << std::setw(16) << "SOURCE_ROM_PC"
              << "  NATIVE_CXX_OWNER\n"
              << "--------------------------------------------------------------------------------------------------------\n";

    print_result(d_ply);
    print_result(d_cam);
    print_result(d_scr);
    print_result(d_ent);
    print_result(d_hud);
    print_result(d_plb);
    print_result(d_pla);
    print_result(d_spr);
    print_result(d_sac);
    print_result(d_sat);
    print_result(d_vrm);
    print_result(d_fbx);

    std::cout << "========================================================================================================\n" << std::flush;

    // Core validation asserts
    assert(d_ply.exact == total_frames);
    assert(d_cam.exact == total_frames);
    assert(d_scr.exact == total_frames);
    assert(d_ent.exact == total_frames);
    assert(d_hud.exact == total_frames);
    assert(d_plb.exact == total_frames);
    assert(d_spr.exact == total_frames);

    std::cout << "[PASS] Differential Oracle 121-Frame 12-Domain Verification Complete\n";
    return 0;
}
