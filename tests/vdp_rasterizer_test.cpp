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
    state.registers[1] = 0x64; // Display ON, V28
    state.registers[2] = 0x30; // Plane A 0xC000
    state.registers[3] = 0x00; // Window 0x0000
    state.registers[4] = 0x07; // Plane B 0xE000
    state.registers[5] = 0x68; // SAT 0xD000
    state.registers[7] = 0x00; // Backdrop 0
    state.registers[11] = 0x00; // Full screen scroll
    state.registers[12] = 0x81; // H40 mode
    state.registers[13] = 0x37; // HScroll 0xDC00
    state.registers[16] = 0x01; // 64x32 cells

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
    state.registers[12] = 0x81; // 320x224
    state.registers[16] = 0x01; // 64x32 cells
    state.registers[7] = 0x00;  // Backdrop color index 0

    // Set CRAM: color 0 = Black, color 1 = Red, color 2 = Green, color 3 = Blue
    // Mega Drive 0x0BGR:
    // Red = 0x000E
    state.cram[2] = 0x00; state.cram[3] = 0x0E;
    // Green = 0x00E0
    state.cram[4] = 0x00; state.cram[5] = 0xE0;
    // Blue = 0x0E00
    state.cram[6] = 0x0E; state.cram[7] = 0x00;

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, &metrics);

    // With empty VRAM, all pixels should be backdrop (black)
    assert(metrics.backdrop_pixels == 320 * 224);
    assert(output[0] == 0 && output[1] == 0 && output[2] == 0);
}

void test_frame_779_exact_match() {
    oasis::genesis::VdpState state{};
    const std::string path = resolve_fixture("data/standalone-fixtures/frame_779_vdp_state.json");
    const bool loaded = state.load_from_file(path.c_str());
    assert(loaded);
    assert(state.display_width() == 320);
    assert(state.display_height() == 224);

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, &metrics);

    assert(metrics.total_pixels == 71680);
    assert(metrics.plane_a_pixels == 50205);
    assert(metrics.plane_b_pixels == 20815);
    assert(metrics.sprite_pixels == 660);
    assert(metrics.backdrop_pixels == 0);
    assert(metrics.window_pixels == 0);

    const std::string sha = oasis::core::compute_sha256(output);
    assert(sha == "fcfceb5833abef88a33c13339f2d40d38130e3ccda9b35699bd49a8d6125f690");

    const auto ref = read_binary("data/standalone-fixtures/frame_779_reference.rgb24");
    assert(ref.size() == output.size());
    assert(ref == output);
}

void test_frame_200_exact_match() {
    oasis::genesis::VdpState state{};
    const std::string path = resolve_fixture("data/standalone-fixtures/frame_200_vdp_state.json");
    const bool loaded = state.load_from_file(path.c_str());
    assert(loaded);
    assert(state.display_width() == 320);
    assert(state.display_height() == 224);

    std::vector<std::uint8_t> output(320 * 224 * 3, 0);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, output, &metrics);

    assert(metrics.total_pixels == 71680);

    const std::string sha = oasis::core::compute_sha256(output);
    assert(sha == "97287665eb8fc1e690fefeff5af4ea833b34d2384ffba2e110b5c493670d20f0");

    const auto ref = read_binary("data/standalone-fixtures/frame_200_reference.rgb24");
    assert(ref.size() == output.size());
    assert(ref == output);
}

} // namespace

int main() {
    std::cout << "[RUN] test_vdp_state_defaults\n";
    test_vdp_state_defaults();
    std::cout << "[PASS] test_vdp_state_defaults\n";

    std::cout << "[RUN] test_color_and_layer_priority\n";
    test_color_and_layer_priority();
    std::cout << "[PASS] test_color_and_layer_priority\n";

    std::cout << "[RUN] test_frame_779_exact_match\n";
    test_frame_779_exact_match();
    std::cout << "[PASS] test_frame_779_exact_match\n";

    std::cout << "[RUN] test_frame_200_exact_match\n";
    test_frame_200_exact_match();
    std::cout << "[PASS] test_frame_200_exact_match\n";

    std::cout << "All VDP rasterizer tests PASSED successfully.\n";
    return 0;
}
