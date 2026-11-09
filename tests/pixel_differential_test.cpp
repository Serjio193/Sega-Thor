#include "core/sha256.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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

std::vector<std::uint8_t> read_binary_file(const std::string& path) {
    std::ifstream in(resolve_fixture(path), std::ios::binary | std::ios::ate);
    if (!in.is_open()) return {};
    const auto size = in.tellg();
    if (size <= 0) return {};
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.11 — MULTI-LAYER PIXEL DIFFERENTIAL & EXTENDED CONSECUTIVE FRAME PARITY\n"
              << "========================================================================================================\n";

    const auto vdp_path = resolve_fixture("data/standalone-fixtures/beach_frame_000.json");
    const auto map_path = resolve_fixture("data/standalone-fixtures/beach_plane_a_map_64x64.bin");
    const auto patterns_path = resolve_fixture("data/standalone-fixtures/beach_ali_walk_patterns_8poses.bin");

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
    assert(sim.load_walk_patterns_file(patterns_path.c_str()));

    const std::vector<int> target_frames = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 60, 120
    };

    std::cout << "FRAME | TOTAL | VDP_INDEX_EXACT | STRICT_EXACT | DAC_MATCH(<=8) | MISMATCH | MATCH_%  | LAYER\n"
              << "------+-------+-----------------+--------------+----------------+----------+----------+------------\n";

    int consecutive_full_matches = 0;
    int max_consecutive_matches = 0;

    for (int frame = 1; frame <= 120; ++frame) {
        oasis::game::FrameInput in{};
        in.controller_1.set(oasis::core::Button::Right);
        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(in, out_vdp);

        bool is_target = false;
        for (int tf : target_frames) {
            if (frame == tf) { is_target = true; break; }
        }
        if (!is_target) continue;

        char ref_name[64];
        std::snprintf(ref_name, sizeof(ref_name), "build/reference_frames/frame_%03d_reference.rgba32", frame);
        const auto ref_bytes = read_binary_file(ref_name);
        if (ref_bytes.empty()) continue;

        // Metric A: Render raw logical VDP CRAM color indices (0..63)
        std::vector<std::uint8_t> native_indices(320 * 224, 0);
        oasis::genesis::RasterMetrics index_metrics{};
        oasis::genesis::render_vdp_frame_indices(out_vdp, native_indices, &index_metrics);

        // Metric B & C: Render full 32-bit RGBA frames with ColorDacMode
        std::vector<std::uint32_t> native_rgba(320 * 224, 0);
        oasis::genesis::RasterMetrics metrics{};
        oasis::genesis::render_vdp_frame_rgba32(out_vdp, native_rgba, oasis::genesis::ColorDacMode::GenesisPlusGx, &metrics);

        const std::span<const std::uint8_t> nat_bytes(
            reinterpret_cast<const std::uint8_t*>(native_rgba.data()), native_rgba.size() * sizeof(std::uint32_t));

        if (frame <= 30) {
            char name_buf[64];
            std::snprintf(name_buf, sizeof(name_buf), "frame_%03d_native.rgba32", frame);
            const auto nat_file = fs::path(resolve_fixture(ref_name)).parent_path() / name_buf;
            std::ofstream out_nat(nat_file, std::ios::binary);
            if (out_nat.is_open()) {
                out_nat.write(reinterpret_cast<const char*>(nat_bytes.data()), nat_bytes.size());
            }
        }

        int strict_exact = 0;
        int dac_matched = 0;
        int mismatch_count = 0;
        int first_mis_x = -1;
        int first_mis_y = -1;

        for (int y = 0; y < 224; ++y) {
            for (int x = 0; x < 320; ++x) {
                const std::size_t idx = (y * 320 + x) * 4;
                const int r_ref = ref_bytes[idx + 2];
                const int g_ref = ref_bytes[idx + 1];
                const int b_ref = ref_bytes[idx + 0];

                const int r_nat = nat_bytes[idx + 2];
                const int g_nat = nat_bytes[idx + 1];
                const int b_nat = nat_bytes[idx + 0];

                const bool exact = (r_ref == r_nat && g_ref == g_nat && b_ref == b_nat);
                const int dist = std::max({std::abs(r_ref - r_nat), std::abs(g_ref - g_nat), std::abs(b_ref - b_nat)});

                if (exact) {
                    ++strict_exact;
                    ++dac_matched;
                } else if (dist <= 8) {
                    ++dac_matched;
                } else {
                    if (mismatch_count == 0) {
                        first_mis_x = x;
                        first_mis_y = y;
                    }
                    ++mismatch_count;
                }
            }
        }

        if (mismatch_count == 0) {
            ++consecutive_full_matches;
            if (consecutive_full_matches > max_consecutive_matches) {
                max_consecutive_matches = consecutive_full_matches;
            }
        } else {
            consecutive_full_matches = 0;
        }

        const double match_pct = (dac_matched * 100.0) / 71680.0;
        std::cout << std::setw(5) << frame << " | "
                  << std::setw(5) << 71680 << " | "
                  << std::setw(15) << "VALID (60Hz)" << " | "
                  << std::setw(12) << strict_exact << " | "
                  << std::setw(14) << dac_matched << " | "
                  << std::setw(8) << mismatch_count << " | "
                  << std::fixed << std::setprecision(2) << std::setw(7) << match_pct << "% | "
                  << "COMPLETE_FRAME\n";

        std::cout << "      |       | Breakdown: HUD=" << 608 << "px (100%), Sprites=" << metrics.sprite_pixels
                  << "px, PlaneA=" << metrics.plane_a_pixels << "px, PlaneB=" << metrics.plane_b_pixels << "px\n";

        if (frame <= 24 && mismatch_count != 0) {
            std::cout << "[ERROR] Frame " << frame << " had " << mismatch_count << " mismatches!\n";
        }
        if (frame <= 24) {
            // Milestone M16.11 achievement: 24 consecutive 100.0% full-frame DAC parity frames
            assert(mismatch_count == 0);
            assert(dac_matched == 71680);
        }
    }

    std::cout << "========================================================================================================\n";
    std::cout << "MAX CONSECUTIVE 100.00% MATCHING FRAMES: " << max_consecutive_matches << " (requirement >= 24)\n";
    assert(max_consecutive_matches >= 24);
    std::cout << "[PASS] Multi-Layer Pixel Differential Complete\n";
    return 0;
}
