#include "core/sha256.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " --fixture <vdp_state.json> [options]\n"
              << "Options:\n"
              << "  --dac <model|gpgx>               Color DAC formula (default: model)\n"
              << "  --output <file.ppm|file.rgb24>   Save native rendered frame\n"
              << "  --compare <reference.rgb24>      Compare against reference framebuffer\n"
              << "  --diff <diff.ppm>                Save visual diff on mismatch\n";
}

bool write_ppm(const std::string& path, int width, int height, const std::vector<std::uint8_t>& rgb) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out << "P6\n" << width << " " << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    return true;
}

bool write_raw(const std::string& path, const std::vector<std::uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return true;
}

std::vector<std::uint8_t> read_binary_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return {};
    const auto size = in.tellg();
    if (size <= 0) return {};
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string fixture_path;
    std::string output_path;
    std::string compare_path;
    std::string diff_path;
    oasis::genesis::ColorDacMode dac_mode = oasis::genesis::ColorDacMode::ModelFullRange;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--fixture" && i + 1 < argc) {
            fixture_path = argv[++i];
        } else if (arg == "--dac" && i + 1 < argc) {
            std::string_view mode_str = argv[++i];
            if (mode_str == "gpgx" || mode_str == "genesis_plus_gx") {
                dac_mode = oasis::genesis::ColorDacMode::GenesisPlusGx;
            } else {
                dac_mode = oasis::genesis::ColorDacMode::ModelFullRange;
            }
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--compare" && i + 1 < argc) {
            compare_path = argv[++i];
        } else if (arg == "--diff" && i + 1 < argc) {
            diff_path = argv[++i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (fixture_path.empty()) {
        std::cerr << "Error: --fixture argument is required\n";
        print_usage(argv[0]);
        return 1;
    }

    oasis::genesis::VdpState state{};
    if (!state.load_from_file(fixture_path.c_str())) {
        std::cerr << "Error: failed to load VDP fixture: " << fixture_path << "\n";
        return 2;
    }

    const int width = state.display_width();
    const int height = state.display_height();
    std::vector<std::uint8_t> native_rgb(static_cast<std::size_t>(width * height * 3));

    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgb24(state, native_rgb, dac_mode, &metrics);

    const std::string native_sha256 = oasis::core::compute_sha256(native_rgb);

    std::cout << "NATIVE_WIDTH=" << width << "\n"
              << "NATIVE_HEIGHT=" << height << "\n"
              << "NATIVE_SHA256=" << native_sha256 << "\n"
              << "BACKDROP_PIXELS=" << metrics.backdrop_pixels << "\n"
              << "PLANE_B_PIXELS=" << metrics.plane_b_pixels << "\n"
              << "PLANE_A_PIXELS=" << metrics.plane_a_pixels << "\n"
              << "SPRITE_PIXELS=" << metrics.sprite_pixels << "\n"
              << "WINDOW_PIXELS=" << metrics.window_pixels << "\n"
              << "TOTAL_PIXELS=" << metrics.total_pixels << "\n";

    if (!output_path.empty()) {
        if (output_path.ends_with(".ppm")) {
            write_ppm(output_path, width, height, native_rgb);
        } else {
            write_raw(output_path, native_rgb);
        }
        std::cout << "WROTE_OUTPUT=" << output_path << "\n";
    }

    if (!compare_path.empty()) {
        const auto ref_data = read_binary_file(compare_path);
        if (ref_data.empty()) {
            std::cerr << "Error: failed to read reference file: " << compare_path << "\n";
            return 3;
        }

        const std::string ref_sha256 = oasis::core::compute_sha256(ref_data);
        const int ref_pixels = static_cast<int>(ref_data.size() / 3);
        const int total_pixels = width * height;

        int matched = 0;
        int diff = 0;
        int first_diff_x = -1;
        int first_diff_y = -1;
        int max_channel_err = 0;

        std::vector<std::uint8_t> diff_img;
        if (!diff_path.empty()) {
            diff_img.assign(native_rgb.size(), 0);
        }

        for (int i = 0; i < total_pixels && i < ref_pixels; ++i) {
            const int px = i % width;
            const int py = i / width;
            const int nr = native_rgb[i * 3];
            const int ng = native_rgb[i * 3 + 1];
            const int nb = native_rgb[i * 3 + 2];
            const int rr = ref_data[i * 3];
            const int rg = ref_data[i * 3 + 1];
            const int rb = ref_data[i * 3 + 2];

            const int dr = std::abs(nr - rr);
            const int dg = std::abs(ng - rg);
            const int db = std::abs(nb - rb);
            const int merr = std::max({dr, dg, db});
            if (merr > max_channel_err) max_channel_err = merr;

            if (dr == 0 && dg == 0 && db == 0) {
                matched++;
            } else {
                if (diff == 0) {
                    first_diff_x = px;
                    first_diff_y = py;
                }
                diff++;
                if (!diff_path.empty()) {
                    diff_img[i * 3] = 255;
                    diff_img[i * 3 + 1] = 0;
                    diff_img[i * 3 + 2] = 0;
                }
            }
        }

        std::cout << "REFERENCE_WIDTH=" << width << "\n"
                  << "REFERENCE_HEIGHT=" << height << "\n"
                  << "REFERENCE_SHA256=" << ref_sha256 << "\n"
                  << "TOTAL_COMPARED_PIXELS=" << total_pixels << "\n"
                  << "MATCHED_PIXELS=" << matched << "\n"
                  << "DIFFERENT_PIXELS=" << diff << "\n";

        if (diff > 0) {
            std::cout << "FIRST_DIFFERENT_XY=(" << first_diff_x << "," << first_diff_y << ")\n"
                      << "MAX_COLOR_CHANNEL_ERROR=" << max_channel_err << "\n"
                      << "STATUS=FAIL\n";
            if (!diff_path.empty()) {
                write_ppm(diff_path, width, height, diff_img);
                std::cout << "WROTE_DIFF=" << diff_path << "\n";
            }
            return 4;
        } else {
            std::cout << "FIRST_DIFFERENT_XY=NONE\n"
                      << "MAX_COLOR_CHANNEL_ERROR=0\n"
                      << "STATUS=PASS\n";
        }
    }

    return 0;
}
