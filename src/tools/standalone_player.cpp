#include "core/sha256.hpp"
#include "game/standalone_game_loop.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"
#include "platform/window.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct PlayerOptions {
    std::string sequence_dir{"data/standalone-fixtures/sequence"};
    int fps{60};
    int scale{3};
    bool headless{false};
    int max_frames{0};
    oasis::genesis::ColorDacMode dac_mode{oasis::genesis::ColorDacMode::GenesisPlusGx};
};

struct SequenceEntry {
    int frame_number{0};
    std::string json_path;
    std::string rgb24_path;
};

std::vector<SequenceEntry> discover_sequence(const std::string& dir_path) {
    std::vector<SequenceEntry> entries;
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return entries;
    }

    const std::regex frame_re(R"(frame_(\d+)\.json)", std::regex::icase);
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (!entry.is_regular_file()) continue;
        const std::string filename = entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, frame_re)) {
            const int num = std::stoi(match[1].str());
            std::string rgb_path = (entry.path().parent_path() / ("frame_" + match[1].str() + ".rgb24")).string();
            entries.push_back({num, entry.path().string(), rgb_path});
        }
    }

    std::sort(entries.begin(), entries.end(), [](const SequenceEntry& a, const SequenceEntry& b) {
        return a.frame_number < b.frame_number;
    });
    return entries;
}

#ifdef _WIN32
bool is_key_pressed(int vk, bool& prev_state) noexcept {
    const bool current = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool triggered = current && !prev_state;
    prev_state = current;
    return triggered;
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    PlayerOptions opt{};

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--sequence-dir" && i + 1 < argc) {
            opt.sequence_dir = argv[++i];
        } else if (arg == "--fps" && i + 1 < argc) {
            opt.fps = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--scale" && i + 1 < argc) {
            opt.scale = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--headless") {
            opt.headless = true;
        } else if (arg == "--max-frames" && i + 1 < argc) {
            opt.max_frames = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--dac" && i + 1 < argc) {
            std::string_view mode = argv[++i];
            if (mode == "model") {
                opt.dac_mode = oasis::genesis::ColorDacMode::ModelFullRange;
            } else {
                opt.dac_mode = oasis::genesis::ColorDacMode::GenesisPlusGx;
            }
        }
    }

    const auto sequence_entries = discover_sequence(opt.sequence_dir);
    if (sequence_entries.empty()) {
        std::cerr << "Error: no valid sequence frames found in " << opt.sequence_dir << "\n";
        return 1;
    }

    std::cout << "Loaded " << sequence_entries.size() << " sequence frames from " << opt.sequence_dir << "\n";

    std::vector<oasis::genesis::VdpState> vdp_frames;
    vdp_frames.reserve(sequence_entries.size());
    for (const auto& entry : sequence_entries) {
        oasis::genesis::VdpState state{};
        if (!state.load_from_file(entry.json_path.c_str())) {
            std::cerr << "Error: failed to parse frame: " << entry.json_path << "\n";
            return 2;
        }
        vdp_frames.push_back(std::move(state));
    }

    oasis::game::FrameSequenceSource sequence(std::move(vdp_frames));

    if (opt.headless) {
        std::cout << "Running in HEADLESS verification mode...\n";
        std::vector<std::uint32_t> rgba(320 * 224);
        int processed = 0;
        for (std::size_t i = 0; i < sequence.frame_count(); ++i) {
            sequence.seek(i);
            const auto* vdp = sequence.current_vdp();
            oasis::genesis::RasterMetrics metrics{};
            oasis::genesis::render_vdp_frame_rgba32(*vdp, rgba, opt.dac_mode, &metrics);
            processed++;
            if (opt.max_frames > 0 && processed >= opt.max_frames) break;
        }
        std::cout << "HEADLESS_FRAMES_RENDERED=" << processed << "\nSTATUS=PASS\n";
        return 0;
    }

    oasis::platform::NativeWindow window(320, 224, static_cast<std::uint32_t>(opt.scale));
    if (!window.open("Oasis Standalone Player")) {
        std::cerr << "Error: failed to open host platform window\n";
        return 3;
    }

    std::vector<std::uint32_t> rgba(320 * 224);
    bool paused = false;
    int rendered_frames = 0;

#ifdef _WIN32
    bool space_down = false;
    bool right_down = false;
    bool left_down = false;
    bool r_down = false;
    bool esc_down = false;
#endif

    const auto frame_duration = std::chrono::microseconds(1000000 / opt.fps);
    auto next_frame_time = std::chrono::steady_clock::now();

    while (window.process_events() && window.is_open()) {
#ifdef _WIN32
        if (is_key_pressed(VK_ESCAPE, esc_down)) {
            break;
        }
        if (is_key_pressed(VK_SPACE, space_down)) {
            paused = !paused;
        }
        if (is_key_pressed(VK_RIGHT, right_down) || is_key_pressed('N', right_down)) {
            sequence.advance();
        }
        if (is_key_pressed(VK_LEFT, left_down) || is_key_pressed('P', left_down)) {
            sequence.rewind();
        }
        if (is_key_pressed('R', r_down)) {
            sequence.reset();
        }
#endif

        const std::size_t cur_idx = sequence.current_index();
        const int frame_num = sequence_entries[cur_idx].frame_number;
        const auto* vdp = sequence.current_vdp();

        oasis::genesis::RasterMetrics metrics{};
        oasis::genesis::render_vdp_frame_rgba32(*vdp, rgba, opt.dac_mode, &metrics);
        window.present(rgba);

        std::string title = "Beyond Oasis Native Player - Frame " + std::to_string(frame_num) +
                            " [" + std::to_string(cur_idx + 1) + "/" +
                            std::to_string(sequence.frame_count()) + "]" +
                            (paused ? " (PAUSED)" : " (PLAYING @ " + std::to_string(opt.fps) + " FPS)");
        window.set_title(title);

        rendered_frames++;
        if (opt.max_frames > 0 && rendered_frames >= opt.max_frames) {
            break;
        }

        if (!paused) {
            sequence.advance();
        }

        next_frame_time += frame_duration;
        const auto now = std::chrono::steady_clock::now();
        if (next_frame_time > now) {
            std::this_thread::sleep_for(next_frame_time - now);
        } else {
            next_frame_time = now;
        }
    }

    std::cout << "Playback terminated cleanly. Frames rendered: " << rendered_frames << "\n";
    return 0;
}
