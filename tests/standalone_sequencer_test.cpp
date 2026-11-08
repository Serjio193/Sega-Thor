#include "core/sha256.hpp"
#include "game/standalone_game_loop.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& path) {
    if (fs::exists(path)) return path;
    const std::string parent_path = "../" + path;
    if (fs::exists(parent_path)) return parent_path;
    return path;
}

std::vector<std::uint8_t> read_binary(const std::string& path) {
    std::ifstream in(resolve_fixture(path), std::ios::binary | std::ios::ate);
    if (!in.is_open()) return {};
    const auto size = in.tellg();
    if (size <= 0) return {};
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

void test_sequence_chronology_and_exact_pixels() {
    const std::string seq_dir = resolve_fixture("data/standalone-fixtures/sequence");
    assert(fs::exists(seq_dir));

    std::vector<oasis::genesis::VdpState> states;
    states.reserve(17);

    for (int frame_idx = 770; frame_idx <= 786; ++frame_idx) {
        const std::string json_file = seq_dir + "/frame_" + std::to_string(frame_idx) + ".json";
        const std::string rgb_file = seq_dir + "/frame_" + std::to_string(frame_idx) + ".rgb24";

        oasis::genesis::VdpState state{};
        assert(state.load_from_file(json_file.c_str()));
        states.push_back(state);

        std::vector<std::uint8_t> rendered(320 * 224 * 3);
        oasis::genesis::render_vdp_frame_rgb24(state, rendered, oasis::genesis::ColorDacMode::GenesisPlusGx);

        const auto ref_data = read_binary(rgb_file);
        assert(!ref_data.empty());
        assert(ref_data.size() == rendered.size());
        assert(ref_data == rendered);
    }

    assert(states.size() == 17);
}

void test_sprite_animation_transition() {
    const std::string seq_dir = resolve_fixture("data/standalone-fixtures/sequence");

    oasis::genesis::VdpState state780{};
    oasis::genesis::VdpState state781{};
    assert(state780.load_from_file((seq_dir + "/frame_780.json").c_str()));
    assert(state781.load_from_file((seq_dir + "/frame_781.json").c_str()));

    std::vector<std::uint8_t> rgb780(320 * 224 * 3);
    std::vector<std::uint8_t> rgb781(320 * 224 * 3);
    oasis::genesis::RasterMetrics m780{};
    oasis::genesis::RasterMetrics m781{};

    oasis::genesis::render_vdp_frame_rgb24(state780, rgb780, oasis::genesis::ColorDacMode::GenesisPlusGx, &m780);
    oasis::genesis::render_vdp_frame_rgb24(state781, rgb781, oasis::genesis::ColorDacMode::GenesisPlusGx, &m781);

    const std::string sha780 = oasis::core::compute_sha256(rgb780);
    const std::string sha781 = oasis::core::compute_sha256(rgb781);

    assert(sha780 == "9c2c1e57e68104859714e1f24d6cfe0b941aa0b79e30200ebd52ab2efe9a29d2");
    assert(sha781 == "2ec3a6d3c0274e885a97dda06c53eb2e4d02aeaed87ff502d7435a8efc3fe7a9");
    assert(sha780 != sha781);

    // Verify difference is detected and localized
    int diff_pixels = 0;
    for (std::size_t i = 0; i < 320 * 224; ++i) {
        if (rgb780[i * 3] != rgb781[i * 3] ||
            rgb780[i * 3 + 1] != rgb781[i * 3 + 1] ||
            rgb780[i * 3 + 2] != rgb781[i * 3 + 2]) {
            diff_pixels++;
        }
    }
    assert(diff_pixels > 0);
    assert(diff_pixels < 320 * 224); // localized animation difference, not a corrupted blank frame
}

void test_sequencer_controls_state_machine() {
    std::vector<oasis::genesis::VdpState> frames(5);
    oasis::game::FrameSequenceSource seq(std::move(frames));

    assert(seq.frame_count() == 5);
    assert(seq.current_index() == 0);

    // Step forward
    seq.advance();
    assert(seq.current_index() == 1);
    seq.advance();
    assert(seq.current_index() == 2);

    // Step backward
    seq.rewind();
    assert(seq.current_index() == 1);

    // Reset
    seq.reset();
    assert(seq.current_index() == 0);

    // Wrap-around backward
    seq.rewind();
    assert(seq.current_index() == 4);

    // Wrap-around forward
    seq.advance();
    assert(seq.current_index() == 0);

    // Direct seek
    seq.seek(3);
    assert(seq.current_index() == 3);
}

void test_malformed_fixture_handling() {
    oasis::genesis::VdpState state{};
    // Non-existent path fails gracefully
    assert(!state.load_from_file("non_existent_path_to_fixture_12345.json"));

    // Empty state should report safe dimensions and no crashes
    assert(state.display_width() == 256 || state.display_width() == 320);
    assert(state.display_height() == 224 || state.display_height() == 240);
}

} // namespace

int main() {
    std::cout << "[RUN] test_sequence_chronology_and_exact_pixels\n";
    test_sequence_chronology_and_exact_pixels();
    std::cout << "[PASS] test_sequence_chronology_and_exact_pixels\n";

    std::cout << "[RUN] test_sprite_animation_transition\n";
    test_sprite_animation_transition();
    std::cout << "[PASS] test_sprite_animation_transition\n";

    std::cout << "[RUN] test_sequencer_controls_state_machine\n";
    test_sequencer_controls_state_machine();
    std::cout << "[PASS] test_sequencer_controls_state_machine\n";

    std::cout << "[RUN] test_malformed_fixture_handling\n";
    test_malformed_fixture_handling();
    std::cout << "[PASS] test_malformed_fixture_handling\n";

    std::cout << "All standalone sequencer tests PASSED successfully.\n";
    return 0;
}
