#include "core/sha256.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string resolve_fixture(const std::string& path) {
    if (std::filesystem::exists(path)) return path;
    const std::string parent_path = "../" + path;
    if (std::filesystem::exists(parent_path)) return parent_path;
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

void test_vdp_state_defaults() {
    oasis::genesis::VdpState state{};
    state.registers[1] = 0x64;
    state.registers[2] = 0x30;
    state.registers[3] = 0x00;
    state.registers[4] = 0x07;
    state.registers[5] = 0x68;
    state.registers[7] = 0x00;
    state.registers[11] = 0x00;
    state.registers[12] = 0x81;
    state.registers[13] = 0x37;
    state.registers[16] = 0x01;

    assert(state.is_display_enabled());
    assert(state.is_h40());
    assert(!state.is_v30());
    assert(state.display_width() == 320);
    assert(state.display_height() == 224);
    assert(state.plane_a_base() == 0xC000U);
    assert(state.plane_b_base() == 0xE000U);
    assert(state.sat_base() == 0xD000U);
    assert(state.hscroll_base() == 0xDC00U);
    assert(state.plane_width_cells() == 64);
    assert(state.plane_height_cells() == 32);
    assert(state.backdrop_cram_index() == 0);
    assert(state.hscroll_mode() == 0);
    assert(state.vscroll_mode() == 0);
}

void test_color_and_layer_priority() {
    oasis::genesis::VdpState state{};
    state.registers[1] = 0x64;
    state.registers[12] = 0x81;
    state.registers[16] = 0x01;
    state.registers[7] = 0x00;

    state.cram[2] = 0x00; state.cram[3] = 0x0E;
    state.cram[4] = 0x00; state.cram[5] = 0xE0;
    state.cram[6] = 0x0E; state.cram[7] = 0x00;

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, oasis::genesis::ColorDacMode::ModelFullRange, &metrics);

    assert(metrics.backdrop_pixels == 320 * 224);
    assert(output[0] == 0 && output[1] == 0 && output[2] == 0);
}

void test_frame_779_model_exact_match() {
    oasis::genesis::VdpState state{};
    const std::string path = resolve_fixture("data/standalone-fixtures/frame_779_vdp_state.json");
    assert(state.load_from_file(path.c_str()));

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, oasis::genesis::ColorDacMode::ModelFullRange, &metrics);

    assert(metrics.total_pixels == 71680);
    assert(metrics.plane_a_pixels == 50205);
    assert(metrics.plane_b_pixels == 20815);
    assert(metrics.sprite_pixels == 660);

    const std::string sha = oasis::core::compute_sha256(output);
    assert(sha == "fcfceb5833abef88a33c13339f2d40d38130e3ccda9b35699bd49a8d6125f690");

    const auto ref = read_binary("data/standalone-fixtures/frame_779_reference.rgb24");
    assert(ref.size() == output.size());
    assert(ref == output);
}

void test_frame_200_model_exact_match() {
    oasis::genesis::VdpState state{};
    const std::string path = resolve_fixture("data/standalone-fixtures/frame_200_vdp_state.json");
    assert(state.load_from_file(path.c_str()));

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, oasis::genesis::ColorDacMode::ModelFullRange, &metrics);

    assert(metrics.total_pixels == 71680);

    const std::string sha = oasis::core::compute_sha256(output);
    assert(sha == "97287665eb8fc1e690fefeff5af4ea833b34d2384ffba2e110b5c493670d20f0");

    const auto ref = read_binary("data/standalone-fixtures/frame_200_reference.rgb24");
    assert(ref.size() == output.size());
    assert(ref == output);
}

void test_real_emulator_oracle_parity() {
    // Frame 779 with GenesisPlusGx DAC mode matches genuine emulator display pixels
    oasis::genesis::VdpState state779{};
    assert(state779.load_from_file(resolve_fixture("data/standalone-fixtures/frame_779_vdp_state.json").c_str()));
    std::vector<std::uint8_t> out779(320 * 224 * 3, 0);
    oasis::genesis::render_vdp_frame_rgb24(state779, out779, oasis::genesis::ColorDacMode::GenesisPlusGx);
    const std::string sha779 = oasis::core::compute_sha256(out779);
    assert(sha779 == "9c2c1e57e68104859714e1f24d6cfe0b941aa0b79e30200ebd52ab2efe9a29d2");

    const auto ref779 = read_binary("data/standalone-fixtures/frame_779_real_emulator.rgb24");
    assert(ref779.size() == out779.size());
    assert(ref779 == out779);

    // Frame 200 with GenesisPlusGx DAC mode matches genuine emulator display pixels
    oasis::genesis::VdpState state200{};
    assert(state200.load_from_file(resolve_fixture("data/standalone-fixtures/frame_200_vdp_state.json").c_str()));
    std::vector<std::uint8_t> out200(320 * 224 * 3, 0);
    oasis::genesis::render_vdp_frame_rgb24(state200, out200, oasis::genesis::ColorDacMode::GenesisPlusGx);
    const std::string sha200 = oasis::core::compute_sha256(out200);
    assert(sha200 == "9efaa3202d894bba02c53eafe11567af663f4c1d993ff62762518af3abaa0756");

    const auto ref200 = read_binary("data/standalone-fixtures/frame_200_real_emulator.rgb24");
    assert(ref200.size() == out200.size());
    assert(ref200 == out200);
}

void test_determinism_and_rgba32() {
    oasis::genesis::VdpState state{};
    assert(state.load_from_file(resolve_fixture("data/standalone-fixtures/frame_779_vdp_state.json").c_str()));

    // Determinism: rendering the exact same state twice produces identical output
    std::vector<std::uint8_t> run1(320 * 224 * 3, 0);
    std::vector<std::uint8_t> run2(320 * 224 * 3, 0);
    oasis::genesis::render_vdp_frame_rgb24(state, run1, oasis::genesis::ColorDacMode::GenesisPlusGx);
    oasis::genesis::render_vdp_frame_rgb24(state, run2, oasis::genesis::ColorDacMode::GenesisPlusGx);
    assert(run1 == run2);

    // RGBA32 format consistency
    std::vector<std::uint32_t> rgba(320 * 224, 0);
    oasis::genesis::render_vdp_frame_rgba32(state, rgba, oasis::genesis::ColorDacMode::GenesisPlusGx);
    for (std::size_t i = 0; i < rgba.size(); ++i) {
        const std::uint8_t r = run1[i * 3];
        const std::uint8_t g = run1[i * 3 + 1];
        const std::uint8_t b = run1[i * 3 + 2];
        const std::uint32_t expected = 0xFF000000U | (static_cast<std::uint32_t>(r) << 16U) |
                                       (static_cast<std::uint32_t>(g) << 8U) | static_cast<std::uint32_t>(b);
        assert(rgba[i] == expected);
    }
}

} // namespace

int main() {
    std::cout << "[RUN] test_vdp_state_defaults\n";
    test_vdp_state_defaults();
    std::cout << "[PASS] test_vdp_state_defaults\n";

    std::cout << "[RUN] test_color_and_layer_priority\n";
    test_color_and_layer_priority();
    std::cout << "[PASS] test_color_and_layer_priority\n";

    std::cout << "[RUN] test_frame_779_model_exact_match\n";
    test_frame_779_model_exact_match();
    std::cout << "[PASS] test_frame_779_model_exact_match\n";

    std::cout << "[RUN] test_frame_200_model_exact_match\n";
    test_frame_200_model_exact_match();
    std::cout << "[PASS] test_frame_200_model_exact_match\n";

    std::cout << "[RUN] test_real_emulator_oracle_parity\n";
    test_real_emulator_oracle_parity();
    std::cout << "[PASS] test_real_emulator_oracle_parity\n";

    std::cout << "[RUN] test_determinism_and_rgba32\n";
    test_determinism_and_rgba32();
    std::cout << "[PASS] test_determinism_and_rgba32\n";

    std::cout << "All VDP rasterizer tests PASSED successfully.\n";
    return 0;
}
