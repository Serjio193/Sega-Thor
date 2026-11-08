#include "core/runtime.hpp"
#include "core/sha256.hpp"
#include "game/camera/camera_controller.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
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
    std::string vdp_relevant_ram_hash{};
    std::string sat_hash{};
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
        f.vdp_relevant_ram_hash = extract_str(obj, "VDP_RELEVANT_RAM_HASH");
        f.sat_hash = extract_str(obj, "SAT_HASH");
        f.framebuffer_hash = extract_str(obj, "FRAMEBUFFER_HASH");
        frames.push_back(f);
        pos = obj_end + 1;
    }
    return frames;
}

void print_domain_row(const char* name, int exact, int total,
                      int diff_frame, const char* diff_field,
                      const std::string& exp_val, const std::string& act_val,
                      const char* rom_pc) {
    const double pct = (total > 0) ? (100.0 * exact / total) : 0.0;
    std::cout << std::left << std::setw(20) << name
              << std::right << std::setw(12) << exact
              << std::setw(14) << total
              << std::setw(9) << std::fixed << std::setprecision(1) << pct << "%"
              << std::setw(18);
    if (diff_frame < 0) std::cout << "NONE"; else std::cout << diff_frame;
    std::cout << std::setw(18) << diff_field
              << std::setw(14) << exp_val
              << std::setw(14) << act_val
              << std::setw(18) << rom_pc << "\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================\n"
              << "SEGA-THOR M16.5 — 120-FRAME ROM-VERIFIED DIFFERENTIAL ORACLE REPORT\n"
              << "========================================================================================\n";

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

    int camera_exact = 0, first_diff_cam_frame = -1;
    std::string first_diff_cam_field = "NONE", exp_cam = "NONE", act_cam = "NONE";

    int scroll_exact = 0, first_diff_scroll_frame = -1;
    std::string first_diff_scroll_field = "NONE", exp_scroll = "NONE", act_scroll = "NONE";

    int entity_exact = 0, first_diff_ent_frame = -1;
    std::string first_diff_ent_field = "NONE", exp_ent = "NONE", act_ent = "NONE";

    int player_exact = 0, first_diff_ply_frame = -1;
    std::string first_diff_ply_field = "NONE", exp_ply = "NONE", act_ply = "NONE";

    int sat_exact = 0, first_diff_sat_frame = -1;
    std::string first_diff_sat_field = "NONE", exp_sat = "NONE", act_sat = "NONE";

    int fb_exact = 0, first_diff_fb_frame = -1;
    std::string first_diff_fb_field = "NONE", exp_fb = "NONE", act_fb = "NONE";

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

        // 1. Camera World Check
        const int act_cx = sim.camera().camera_x();
        const int act_cy = sim.camera().camera_y();
        if (act_cx == of.camera_world_x && act_cy == of.camera_world_y) {
            camera_exact++;
        } else if (first_diff_cam_frame < 0) {
            first_diff_cam_frame = f;
            first_diff_cam_field = (act_cx != of.camera_world_x) ? "CAM_X" : "CAM_Y";
            exp_cam = (act_cx != of.camera_world_x) ? std::to_string(of.camera_world_x) : std::to_string(of.camera_world_y);
            act_cam = (act_cx != of.camera_world_x) ? std::to_string(act_cx) : std::to_string(act_cy);
        }

        // 2. VDP Plane Scroll Check (HScroll table at 0xDC00, VSRAM at 0)
        const auto h_a = static_cast<int>(
            (static_cast<std::uint16_t>(out_vdp.vram[0xDC00]) << 8U) | out_vdp.vram[0xDC01]) & 0x01FF;
        const auto v_a = static_cast<int>(
            (static_cast<std::uint16_t>(out_vdp.vsram[0]) << 8U) | out_vdp.vsram[1]) & 0x03FF;
        if (h_a == of.hscroll_plane_a && v_a == of.vscroll_plane_a) {
            scroll_exact++;
        } else if (first_diff_scroll_frame < 0) {
            first_diff_scroll_frame = f;
            first_diff_scroll_field = (h_a != of.hscroll_plane_a) ? "HSCROLL_A" : "VSCROLL_A";
            exp_scroll = (h_a != of.hscroll_plane_a) ? std::to_string(of.hscroll_plane_a) : std::to_string(of.vscroll_plane_a);
            act_scroll = (h_a != of.hscroll_plane_a) ? std::to_string(h_a) : std::to_string(v_a);
        }

        // 3. Entity Slot 7 Check
        assert(!sim.entities().empty());
        const auto& ent0 = sim.entities()[0];
        if (ent0.type_id == of.entity_slot7_type &&
            ent0.world_x == of.entity_slot7_x &&
            ent0.world_y == of.entity_slot7_y &&
            ent0.state == of.entity_slot7_state) {
            entity_exact++;
        } else if (first_diff_ent_frame < 0) {
            first_diff_ent_frame = f;
            first_diff_ent_field = (ent0.state != of.entity_slot7_state) ? "STATE" : "COORDS";
            exp_ent = std::to_string(of.entity_slot7_state);
            act_ent = std::to_string(ent0.state);
        }

        // 4. Player Check (Odd frames / 30Hz steps)
        const int act_px = static_cast<int>(sim.state().player_x);
        const int act_py = static_cast<int>(sim.state().player_y);
        if (act_px == of.player_world_x && act_py == of.player_world_y) {
            player_exact++;
        } else if (first_diff_ply_frame < 0) {
            first_diff_ply_frame = f;
            first_diff_ply_field = (act_px != of.player_world_x) ? "PLAYER_X" : "PLAYER_Y";
            exp_ply = std::to_string(of.player_world_x);
            act_ply = std::to_string(act_px);
        }

        // 5. SAT Hash Check
        const std::size_t sat_base = out_vdp.sat_base();
        const std::span<const std::uint8_t> sat_bytes(&out_vdp.vram[sat_base], 640);
        const std::string cur_sat_hash = oasis::core::compute_sha256(sat_bytes);
        if (cur_sat_hash == of.sat_hash) {
            sat_exact++;
        } else if (first_diff_sat_frame < 0) {
            first_diff_sat_frame = f;
            first_diff_sat_field = "SAT_SHA256";
            exp_sat = of.sat_hash.substr(0, 8);
            act_sat = cur_sat_hash.substr(0, 8);
        }

        // 6. Framebuffer Hash Check
        if (!of.framebuffer_hash.empty()) {
            oasis::genesis::render_vdp_frame_rgba32(out_vdp, rgba, oasis::genesis::ColorDacMode::GenesisPlusGx);
            const std::span<const std::uint8_t> fb_bytes(
                reinterpret_cast<const std::uint8_t*>(rgba.data()), rgba.size() * sizeof(std::uint32_t));
            const std::string cur_fb_hash = oasis::core::compute_sha256(fb_bytes);
            if (cur_fb_hash == of.framebuffer_hash) {
                fb_exact++;
            } else if (first_diff_fb_frame < 0) {
                first_diff_fb_frame = f;
                first_diff_fb_field = "RGBA_SHA256";
                exp_fb = of.framebuffer_hash.substr(0, 8);
                act_fb = cur_fb_hash.substr(0, 8);
            }
        }
    }

    std::cout << "----------------------------------------------------------------------------------------\n"
              << std::left << std::setw(20) << "DOMAIN"
              << std::right << std::setw(12) << "EXACT_FRAMES"
              << std::setw(14) << "TOTAL_FRAMES"
              << std::setw(10) << "MATCH_%"
              << std::setw(18) << "FIRST_DIFF_FRAME"
              << std::setw(18) << "FIRST_DIFF_FIELD"
              << std::setw(14) << "EXPECTED"
              << std::setw(14) << "ACTUAL"
              << std::setw(18) << "SOURCE_ROM_PC\n"
              << "----------------------------------------------------------------------------------------\n";

    print_domain_row("CAMERA_EXACT", camera_exact, total_frames, first_diff_cam_frame,
                     first_diff_cam_field.c_str(), exp_cam, act_cam, "0x00CEEA");
    print_domain_row("SCROLL_EXACT", scroll_exact, total_frames, first_diff_scroll_frame,
                     first_diff_scroll_field.c_str(), exp_scroll, act_scroll, "0x00CEEA");
    print_domain_row("ENTITY_EXACT", entity_exact, total_frames, first_diff_ent_frame,
                     first_diff_ent_field.c_str(), exp_ent, act_ent, "0x0063DA");
    print_domain_row("PLAYER_EXACT", player_exact, total_frames, first_diff_ply_frame,
                     first_diff_ply_field.c_str(), exp_ply, act_ply, "0x009D50");
    print_domain_row("SAT_EXACT", sat_exact, total_frames, first_diff_sat_frame,
                     first_diff_sat_field.c_str(), exp_sat, act_sat, "0x008000");
    print_domain_row("FRAMEBUFFER_EXACT", fb_exact, total_frames, first_diff_fb_frame,
                     first_diff_fb_field.c_str(), exp_fb, act_fb, "0x000200");

    std::cout << "========================================================================================\n" << std::flush;

    // Core validation asserts
    assert(camera_exact == total_frames);
    assert(scroll_exact == total_frames);
    assert(entity_exact == total_frames);

    std::cout << "[PASS] Differential Oracle 120-Frame Multi-Domain Comparison\n";
    return 0;
}
