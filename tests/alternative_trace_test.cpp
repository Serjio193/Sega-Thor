#include "core/sha256.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& path) {
    if (fs::exists(path)) return path;
    const std::string p1 = "../" + path;
    if (fs::exists(p1)) return p1;
    const std::string p2 = "../../" + path;
    if (fs::exists(p2)) return p2;
    return path;
}

struct AltFrame {
    int frame_num{0};
    int raw_input{0};
    int player_x{0};
    int player_y{0};
    int player_dir{0};
    int player_state{0};
    int cam_x{0};
    int cam_y{0};
    int h_a{0};
    int h_b{0};
    std::string plane_a_hash{};
    std::string plane_b_hash{};
};

std::string extract_str(const std::string& line, const std::string& key) {
    const std::string token = "\"" + key + "\": \"";
    const auto pos = line.find(token);
    if (pos == std::string::npos) return "";
    const auto end_pos = line.find('"', pos + token.size());
    if (end_pos == std::string::npos) return "";
    return line.substr(pos + token.size(), end_pos - (pos + token.size()));
}

int extract_int(const std::string& line, const std::string& key) {
    const std::string token = "\"" + key + "\": ";
    const auto pos = line.find(token);
    if (pos == std::string::npos) return 0;
    const auto end_pos = line.find_first_of(",}\n\r", pos + token.size());
    if (end_pos == std::string::npos) return 0;
    return std::stoi(line.substr(pos + token.size(), end_pos - (pos + token.size())));
}

std::vector<AltFrame> load_alt_oracle(const std::string& path) {
    std::vector<AltFrame> res;
    std::ifstream in(resolve_fixture(path));
    if (!in.is_open()) return res;

    std::string line;
    std::string obj;
    bool in_obj = false;

    while (std::getline(in, line)) {
        if (line.find('{') != std::string::npos) {
            in_obj = true;
            obj = line + "\n";
        } else if (in_obj) {
            obj += line + "\n";
            if (line.find('}') != std::string::npos) {
                in_obj = false;
                if (obj.find("\"FRAME_NUMBER\"") != std::string::npos) {
                    AltFrame f{};
                    f.frame_num = extract_int(obj, "FRAME_NUMBER");
                    f.raw_input = extract_int(obj, "RAW_INPUT");
                    f.player_x = extract_int(obj, "PLAYER_WORLD_X");
                    f.player_y = extract_int(obj, "PLAYER_WORLD_Y");
                    f.player_dir = extract_int(obj, "PLAYER_DIRECTION");
                    f.player_state = extract_int(obj, "PLAYER_MOVEMENT_STATE");
                    f.cam_x = extract_int(obj, "CAMERA_WORLD_X");
                    f.cam_y = extract_int(obj, "CAMERA_WORLD_Y");
                    f.h_a = extract_int(obj, "HSCROLL_PLANE_A");
                    f.h_b = extract_int(obj, "HSCROLL_PLANE_B");
                    f.plane_a_hash = extract_str(obj, "PLANE_A_HASH");
                    f.plane_b_hash = extract_str(obj, "PLANE_B_HASH");
                    res.push_back(f);
                }
            }
        }
    }
    return res;
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.8 — ALTERNATIVE INPUT SEQUENCE GENERALIZATION TEST (60 FRAMES)\n"
              << "========================================================================================================\n";

    const auto oracle_path = resolve_fixture("data/standalone-fixtures/beach_alternative_trace_oracle.json");
    const auto vdp_path = resolve_fixture("data/standalone-fixtures/beach_frame_000.json");
    const auto map_path = resolve_fixture("data/standalone-fixtures/beach_plane_a_map_64x64.bin");

    assert(fs::exists(oracle_path));
    assert(fs::exists(vdp_path));
    assert(fs::exists(map_path));

    const auto oracle = load_alt_oracle(oracle_path);
    assert(!oracle.empty());
    std::cout << "Loaded Alternative Oracle: " << oracle_path << " (" << oracle.size() << " frames)\n";

    oasis::genesis::VdpState baseline_vdp{};
    assert(baseline_vdp.load_from_file(vdp_path.c_str()));

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
    assert(sim.load_screen_map_file(map_path.c_str()));
    sim.set_oracle_mode(false); // interactive responsive mode

    int cam_matches = 0;
    int scroll_matches = 0;
    int plane_b_matches = 0;

    for (std::size_t i = 1; i < oracle.size(); ++i) {
        const auto& of = oracle[i];
        oasis::game::FrameInput in{};
        // Map raw input
        if (of.raw_input & 0x40) in.controller_1.set(oasis::core::Button::Left);
        if (of.raw_input & 0x10) in.controller_1.set(oasis::core::Button::Up);
        if (of.raw_input & 0x20) in.controller_1.set(oasis::core::Button::Down);
        if (of.raw_input & 0x80) in.controller_1.set(oasis::core::Button::Right);

        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(in, out_vdp);

        if (sim.camera().camera_x() == of.cam_x && sim.camera().camera_y() == of.cam_y) {
            ++cam_matches;
        }

        const int h_a = sim.camera().state().plane_a_hscroll;
        const int h_b = sim.camera().state().plane_b_hscroll;
        if (h_a == of.h_a && h_b == of.h_b) {
            ++scroll_matches;
        }

        const std::span<const std::uint8_t> plb_bytes(&out_vdp.vram[out_vdp.plane_b_base()], 4096);
        const std::string plb_hash = oasis::core::compute_sha256(plb_bytes);
        if (plb_hash == of.plane_b_hash) {
            ++plane_b_matches;
        }
    }

    const int total = static_cast<int>(oracle.size()) - 1;
    std::cout << "ALTERNATIVE TRACE RESULTS (" << total << " FRAMES):\n"
              << "  PLANE_B_EXACT:  " << plane_b_matches << " / " << total << " (" << (plane_b_matches * 100 / total) << "%)\n"
              << "  CAMERA_MATCH:   " << cam_matches << " / " << total << " (" << (cam_matches * 100 / total) << "%)\n"
              << "  SCROLL_MATCH:   " << scroll_matches << " / " << total << " (" << (scroll_matches * 100 / total) << "%)\n";

    assert(plane_b_matches == total);
    std::cout << "[PASS] Alternative Trace Generalization Test Succeeded\n";
    return 0;
}
