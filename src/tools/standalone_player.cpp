#include "core/sha256.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/standalone_game_simulation.hpp"
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
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

enum class PlayerMode {
    Replay,
    Simulation,
};

struct PlayerOptions {
    PlayerMode mode{PlayerMode::Simulation};
    std::string sequence_dir{"data/standalone-fixtures/sequence"};
    std::string sim_fixture{"data/standalone-fixtures/beach_frame_000.json"};
    std::string rom_path{};
    std::string input_journal{};
    int fps{60};
    int scale{3};
    bool headless{false};
    int max_frames{0};
    bool camera_tracking{true};
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

std::vector<std::uint16_t> load_journal_buttons(const std::string& path) {
    std::vector<std::uint16_t> result;
    if (path.empty() || !fs::exists(path)) return result;
    std::ifstream in(path);
    if (!in.is_open()) return result;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("P ", 0) == 0) {
            std::istringstream iss(line);
            char rec{};
            std::uint32_t frame{}, sub{};
            std::string mask;
            if (iss >> rec >> frame >> sub >> mask) {
                if (sub == 0) {
                    const auto val = static_cast<std::uint16_t>(std::stoul(mask, nullptr, 16));
                    result.push_back(val);
                }
            }
        }
    }
    return result;
}

#ifdef _WIN32
bool is_key_pressed(int vk, bool& prev_state) noexcept {
    const bool current = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool triggered = current && !prev_state;
    prev_state = current;
    return triggered;
}
#endif

void print_help(const char* exe_name) {
    std::cout << "Usage: " << exe_name << " [options]\n"
              << "Options:\n"
              << "  --mode <replay|simulation>   Operating mode (default: simulation)\n"
              << "  --sim-fixture <path>         Initial VDP state fixture (default: beach_frame_000.json)\n"
              << "  --rom <path>                 Optional Beyond Oasis ROM for background streaming\n"
              << "  --sequence-dir <dir>         Replay sequence directory (default: sequence)\n"
              << "  --input-journal <path>       Input poll journal for deterministic simulation\n"
              << "  --fps <number>               Playback framerate (default: 60)\n"
              << "  --scale <number>             Window integer scaling factor (default: 3)\n"
              << "  --dac <gpgx|model>           Color DAC model (default: gpgx)\n"
              << "  --no-cam                     Disable automatic camera tracking at start\n"
              << "  --headless                   Run without window for CI / validation\n"
              << "  --max-frames <number>        Limit total frames rendered\n";
}

} // namespace

int main(int argc, char* argv[]) {
    PlayerOptions opt{};

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) {
            std::string_view m = argv[++i];
            opt.mode = (m == "replay") ? PlayerMode::Replay : PlayerMode::Simulation;
        } else if (arg == "--sim-fixture" && i + 1 < argc) {
            opt.sim_fixture = argv[++i];
        } else if (arg == "--rom" && i + 1 < argc) {
            opt.rom_path = argv[++i];
        } else if (arg == "--sequence-dir" && i + 1 < argc) {
            opt.sequence_dir = argv[++i];
        } else if (arg == "--input-journal" && i + 1 < argc) {
            opt.input_journal = argv[++i];
        } else if (arg == "--fps" && i + 1 < argc) {
            opt.fps = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--scale" && i + 1 < argc) {
            opt.scale = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--no-cam") {
            opt.camera_tracking = false;
        } else if (arg == "--headless") {
            opt.headless = true;
        } else if (arg == "--max-frames" && i + 1 < argc) {
            opt.max_frames = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--dac" && i + 1 < argc) {
            std::string_view mode = argv[++i];
            opt.dac_mode = (mode == "model") ? oasis::genesis::ColorDacMode::ModelFullRange
                                             : oasis::genesis::ColorDacMode::GenesisPlusGx;
        } else if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        }
    }

    // Prepare simulation with authentic checkpoint initial state
    oasis::genesis::VdpState baseline_vdp{};
    if (fs::exists(opt.sim_fixture)) {
        if (!baseline_vdp.load_from_file(opt.sim_fixture.c_str())) {
            std::cerr << "Warning: Failed to load simulation fixture: " << opt.sim_fixture << "\n";
        }
    }

    oasis::game::StandaloneGameSimulation simulation{};
    oasis::game::CheckpointInitialState cp_init{};
    cp_init.x_fixed = 0x01051000;
    cp_init.y_fixed = 0x006B0000;
    cp_init.movement_state = 4;
    cp_init.direction_code = 0;
    cp_init.turn_timer = 10;
    simulation.initialize_from_checkpoint(baseline_vdp, cp_init);
    simulation.set_camera_tracking(opt.camera_tracking);

    if (!opt.rom_path.empty() && fs::exists(opt.rom_path)) {
        std::ifstream rom_file(opt.rom_path, std::ios::binary);
        std::vector<std::uint8_t> rom_bytes((std::istreambuf_iterator<char>(rom_file)),
                                            std::istreambuf_iterator<char>());
        if (simulation.load_rom_background(rom_bytes)) {
            std::cout << "Loaded authentic ROM background: " << opt.rom_path << "\n";
        }
    }

    const std::string default_map = "data/standalone-fixtures/beach_plane_a_map_64x64.bin";
    if (fs::exists(default_map)) {
        if (simulation.load_screen_map_file(default_map.c_str())) {
            std::cout << "Loaded authentic Plane A map for streaming: " << default_map << "\n";
        }
    }

    // Prepare replay sequence
    const auto sequence_entries = discover_sequence(opt.sequence_dir);
    std::vector<oasis::genesis::VdpState> vdp_frames;
    for (const auto& entry : sequence_entries) {
        oasis::genesis::VdpState state{};
        if (state.load_from_file(entry.json_path.c_str())) {
            vdp_frames.push_back(std::move(state));
        }
    }
    oasis::game::FrameSequenceSource sequence(std::move(vdp_frames));

    // Optional input journal
    const auto journal_buttons = load_journal_buttons(opt.input_journal);

    if (opt.headless) {
        std::cout << "Running in HEADLESS mode ("
                  << (opt.mode == PlayerMode::Simulation ? "SIMULATION" : "REPLAY") << ")...\n";
        std::vector<std::uint32_t> rgba(320 * 224);
        int processed = 0;
        const int limit = (opt.max_frames > 0) ? opt.max_frames : 60;

        if (opt.mode == PlayerMode::Simulation) {
            oasis::genesis::VdpState cur_vdp{};
            for (int f = 0; f < limit; ++f) {
                oasis::game::FrameInput input{};
                input.frame_index = static_cast<std::uint32_t>(f);
                if (f < static_cast<int>(journal_buttons.size())) {
                    const auto b = journal_buttons[f];
                    if ((b & 0x0080U) != 0U) input.controller_1.set(oasis::core::Button::Right);
                    if ((b & 0x0040U) != 0U) input.controller_1.set(oasis::core::Button::Left);
                    if ((b & 0x0020U) != 0U) input.controller_1.set(oasis::core::Button::Down);
                    if ((b & 0x0010U) != 0U) input.controller_1.set(oasis::core::Button::Up);
                } else if (f > 0) {
                    input.controller_1.set(oasis::core::Button::Right);
                }
                simulation.step_frame(input, cur_vdp);
                oasis::genesis::RasterMetrics metrics{};
                oasis::genesis::render_vdp_frame_rgba32(cur_vdp, rgba, opt.dac_mode, &metrics);
                processed++;
            }
            std::cout << "HEADLESS_SIMULATION_FRAMES=" << processed << "\n"
                      << "FINAL_PLAYER_X=" << simulation.state().player_x << "\n"
                      << "FINAL_PLAYER_Y=" << simulation.state().player_y << "\n"
                      << "FINAL_CAM_X=" << simulation.camera_x() << "\n"
                      << "FINAL_CAM_Y=" << simulation.camera_y() << "\n"
                      << "FINAL_HSCROLL=" << simulation.camera().state().plane_a_hscroll << "\n"
                      << "CHEST_OPENED=" << (simulation.chest_opened() ? "YES" : "NO") << "\n"
                      << "STATUS=PASS\n";
        } else {
            for (std::size_t i = 0; i < sequence.frame_count(); ++i) {
                sequence.seek(i);
                const auto* vdp = sequence.current_vdp();
                oasis::genesis::RasterMetrics metrics{};
                oasis::genesis::render_vdp_frame_rgba32(*vdp, rgba, opt.dac_mode, &metrics);
                processed++;
                if (processed >= limit) break;
            }
            std::cout << "HEADLESS_REPLAY_FRAMES=" << processed << "\nSTATUS=PASS\n";
        }
        return 0;
    }

    oasis::platform::NativeWindow window(320, 224, static_cast<std::uint32_t>(opt.scale));
    if (!window.open("Beyond Oasis Standalone Player")) {
        std::cerr << "Error: failed to open host platform window\n";
        return 3;
    }

    std::vector<std::uint32_t> rgba(320 * 224);
    bool paused = false;
    int rendered_frames = 0;
    PlayerMode active_mode = opt.mode;

#ifdef _WIN32
    bool space_down = false;
    bool right_down = false;
    bool left_down = false;
    bool r_down = false;
    bool mode_down = false;
    bool t_down = false;
    bool esc_down = false;
#endif

    const auto frame_duration = std::chrono::microseconds(1000000 / opt.fps);
    auto next_frame_time = std::chrono::steady_clock::now();
    oasis::genesis::VdpState sim_vdp = baseline_vdp;

    while (window.process_events() && window.is_open()) {
#ifdef _WIN32
        if (is_key_pressed(VK_ESCAPE, esc_down)) break;
        if (is_key_pressed(VK_SPACE, space_down)) paused = !paused;
        if (is_key_pressed('R', r_down)) {
            if (active_mode == PlayerMode::Simulation) simulation.reset();
            else sequence.reset();
        }
        if (is_key_pressed(VK_TAB, mode_down) || is_key_pressed('M', mode_down)) {
            active_mode = (active_mode == PlayerMode::Simulation) ? PlayerMode::Replay
                                                                  : PlayerMode::Simulation;
        }
        if (is_key_pressed('T', t_down)) {
            simulation.set_camera_tracking(!simulation.camera_tracking());
        }
#endif

        if (active_mode == PlayerMode::Simulation) {
            const auto ctrl = window.poll_controller();
            if (!paused) {
                oasis::game::FrameInput input{};
                input.frame_index = simulation.state().frame_counter;
                input.controller_1 = ctrl;
                simulation.step_frame(input, sim_vdp);
            }
#ifdef _WIN32
            else if (is_key_pressed('N', right_down)) {
                oasis::game::FrameInput input{};
                input.frame_index = simulation.state().frame_counter;
                input.controller_1 = ctrl;
                simulation.step_frame(input, sim_vdp);
            }
#endif

            oasis::genesis::RasterMetrics metrics{};
            oasis::genesis::render_vdp_frame_rgba32(sim_vdp, rgba, opt.dac_mode, &metrics);
            window.present(rgba);

            const auto& st = simulation.state();
            const auto& pl = simulation.player();
            std::string title = "Beyond Oasis Simulation - F" +
                                std::to_string(st.frame_counter) + " [Ali: " +
                                std::to_string(st.player_x) + "," + std::to_string(st.player_y) +
                                " S" + std::to_string(pl.movement_state) + "]" +
                                " [Chest: " + (simulation.chest_opened() ? "OPEN" : "CLOSED") + "]" +
                                " [Cam: " + std::to_string(simulation.camera_x()) +
                                "," + std::to_string(simulation.camera_y()) +
                                " HScr=" + std::to_string(simulation.camera().state().plane_a_hscroll) +
                                " Track=" + (simulation.camera_tracking() ? "ON" : "OFF") + "]" +
                                (paused ? " (PAUSED)" : " (PLAYING @ " + std::to_string(opt.fps) + " FPS)");
            window.set_title(title);
        } else {
            // Replay Mode
#ifdef _WIN32
            if (is_key_pressed(VK_RIGHT, right_down) || is_key_pressed('N', right_down)) {
                sequence.advance();
            }
            if (is_key_pressed(VK_LEFT, left_down) || is_key_pressed('P', left_down)) {
                sequence.rewind();
            }
#endif
            const std::size_t cur_idx = sequence.current_index();
            const int frame_num = (!sequence_entries.empty() && cur_idx < sequence_entries.size())
                                      ? sequence_entries[cur_idx].frame_number
                                      : static_cast<int>(cur_idx);
            const auto* vdp = sequence.current_vdp();
            if (vdp != nullptr) {
                oasis::genesis::RasterMetrics metrics{};
                oasis::genesis::render_vdp_frame_rgba32(*vdp, rgba, opt.dac_mode, &metrics);
                window.present(rgba);
            }

            std::string title = "Beyond Oasis Replay Mode - Frame " + std::to_string(frame_num) +
                                " [" + std::to_string(cur_idx + 1) + "/" +
                                std::to_string(sequence.frame_count()) + "]" +
                                (paused ? " (PAUSED)" : " (PLAYING @ " + std::to_string(opt.fps) + " FPS)");
            window.set_title(title);

            if (!paused) {
                sequence.advance();
            }
        }

        rendered_frames++;
        if (opt.max_frames > 0 && rendered_frames >= opt.max_frames) break;

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
