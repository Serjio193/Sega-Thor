#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "tools/hybrid/gameplay_session_identity.hpp"
#include "tools/hybrid/gameplay_session_input_sources.hpp"
#include "tools/hybrid/gameplay_session_landmarks.hpp"
#include "tools/hybrid/gameplay_session_receipts.hpp"
#include "tools/hybrid/gameplay_session_call_capture.hpp"
#include "tools/hybrid/m68k_gameplay_step_set.hpp"
#include "tools/hybrid/gameplay_session_replay_diagnostic.hpp"
#include "tools/hybrid/gameplay_session_replay_options.hpp"
#include "tools/hybrid/gameplay_session_step_trace.hpp"
#include "tools/hybrid/replay_equivalence_trace.hpp"
#include "tools/hybrid/checkpoint_evidence.hpp"
#include "tools/hybrid/grid_query_native_model.hpp"
#include "tools/hybrid/m68k_call_census.hpp"
#include "tools/hybrid/native_island_runtime.hpp"
#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include <libretro.h>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {
using namespace oasis::hybrid::session;
constexpr char canonical_sha[] = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
constexpr unsigned execute_event = 1, read_event = 2, write_event = 4;
constexpr unsigned source_pc = 0xE376, target_pc = 0x9C40, return_pc = 0xE37A;
constexpr unsigned rts_pc = 0x9C7A, finish_pc = 0xE396, landmark_period = 60;
using RegisterWrite = void (*)(unsigned, unsigned);
using RegisterRead = oasis::hybrid::session::RegisterRead;
using WriteMemory = int (*)(unsigned, int, unsigned);
using Install = void (*)(unsigned (*)(unsigned, int, unsigned, unsigned));
using StopCore = void (*)();
using Cpu = oasis::hybrid::session::CpuSnapshot;
using Event = oasis::hybrid::session::CallMemoryEvent;
oasis::hybrid::session::RegisterRead read_register{};
RegisterWrite set_register{};
oasis::hybrid::session::Peek peek_memory{};
WriteMemory write_memory{};
StopCore stop_core{};
oasis::hybrid::session::SchedulerRead read_scheduler{};
oasis::hybrid::session::CopyObservableMemory copy_observable_memory{};
InputJournal* journal{};
std::ofstream* call_output{};
unsigned frame_number{}, poll_index{}, frame_poll_count{}, frame_exec_count{}, previous_pc{};
oasis::hybrid::call_census::M68kCallCensus replay_call_census;
std::uint64_t pc_fingerprint{14695981039346656037ULL};
std::map<unsigned, unsigned> watched_hits, frame_watched_hits;
std::vector<unsigned> watch_pcs{source_pc, target_pc, 0x94A2, 0x8EF8, 0xB730, 0x16A46,
                               0x9D70, 0x9D86, 0x9D90, 0x9148};
std::string hook_error;
oasis::hybrid::native_island::Counters island_counters;
bool native_island_selected{};
bool diagnostic_replay{};
bool diagnostic_native_consumed{};
bool restore_initial_state{};
std::string native_helper_selected;
unsigned island_first_frame{}, island_last_frame{};
std::filesystem::path island_metrics_path;
std::unique_ptr<oasis::hybrid::session::ReplayEquivalenceTrace> equivalence_trace_owner;
oasis::hybrid::session::ReplayEquivalenceTrace* equivalence_trace{};
std::uint64_t total_m68k_transfers{};
bool native_step_observer{};

bool island_write_memory(unsigned address, int width, unsigned value) {
    return write_memory && write_memory(address, width, value) != 0;
}

void write_island_metrics(const std::string& status, const std::string& reason) {
    oasis::hybrid::session::write_native_island_metrics(island_metrics_path.string(), status, reason,
        total_m68k_transfers, island_counters, island_first_frame, island_last_frame);
}
oasis::hybrid::session::CallCapture call;

unsigned bridge_hook(unsigned type, int width, unsigned address, unsigned value);
void observe_step_instruction(unsigned pc, unsigned opcode) {
    native_step_observer = true;
    bridge_hook(execute_event, 0, pc, opcode);
    native_step_observer = false;
    if (equivalence_trace && (pc == 0x9D70U || pc == 0x9D4EU || pc == 0x9DB4U))
        equivalence_trace->mark_native_route(pc);
}
void fail_9d70_instruction_step(unsigned pc) {
    hook_error = "9D70 instruction-step contract fault at " + std::to_string(pc);
    if (stop_core) stop_core();
}
} // namespace
namespace {
std::vector<std::uint8_t> call_ram_before;
std::string call_json() { return oasis::hybrid::session::serialize_call_capture(call, peek_memory, call_ram_before); }

unsigned bridge_hook(unsigned type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    try {
        if (equivalence_trace && ((type & execute_event) != 0U || equivalence_trace->capturing_external_window())) {
            const auto cpu = read_cpu_snapshot(read_register);
            std::string memory_hash_json = "null";
            std::vector<std::uint8_t> memory_snapshot;
            std::span<const std::uint8_t> z80_ram_snapshot;
            if (equivalence_trace->wants_memory_snapshot(type, address, previous_pc)) {
                memory_snapshot.resize(oasis::hybrid::session::observable_memory_snapshot_size);
                if (!copy_observable_memory(memory_snapshot.data(), static_cast<unsigned>(memory_snapshot.size())))
                    throw std::runtime_error("observable memory snapshot failed");
                constexpr std::array<std::pair<const char*, unsigned>, 7> regions{{
                    {"work_ram", 0x10000U}, {"z80_ram", 0x2000U}, {"vram", 0x10000U},
                    {"cram", 0x80U}, {"vsram", 0x80U}, {"vdp_regs", 0x20U}, {"sat", 0x400U}}};
                std::ostringstream hashes;
                hashes << '{';
                std::size_t offset{};
                for (std::size_t i = 0; i < regions.size(); ++i) {
                    const auto [name, length] = regions[i];
                    const auto data = std::span<const std::uint8_t>(memory_snapshot.data() + offset, length);
                    hashes << (i ? "," : "") << '\"' << name << "\":\"" << oasis::calculate_sha256(data) << '\"';
                    offset += length;
                }
                hashes << '}';
                memory_hash_json = hashes.str();
                z80_ram_snapshot = std::span<const std::uint8_t>(memory_snapshot.data() + 0x10000U, 0x2000U);
            }
            equivalence_trace->observe(type, width, address, value, previous_pc, frame_number + 1U,
                cpu, oasis::hybrid::session::read_scheduler_snapshot(read_scheduler), memory_hash_json,
                z80_ram_snapshot);
        }
        replay_call_census.on_hook(type, width, address, value, previous_pc,
                                   frame_number + 1U, read_register, peek_memory);
        if ((type & execute_event) != 0) {
            oasis::hybrid::session::observe_step_instruction_trace(*call_output, frame_number + 1U,
                address, previous_pc, read_register, read_scheduler, native_step_observer);
            if (previous_pc && oasis::hybrid::session::is_call_transfer_instruction(previous_pc, peek_memory))
                ++total_m68k_transfers;
            if (address == 0x9D70U && native_helper_selected != "9d70-step" &&
                !native_helper_selected.starts_with("step:")) {
                if (!island_first_frame) island_first_frame = frame_number + 1U;
                island_last_frame = frame_number + 1U;
                const auto state = read_cpu_snapshot(read_register);
                const auto stack_return = read_be(peek_memory, state.a[7] & 0xFFFFFFU, 4) & 0xFFFFFFU;
                const oasis::hybrid::native_island::CpuState native_state{
                    state.d, state.a, state.pc, static_cast<std::uint16_t>(state.sr)};
                const oasis::hybrid::native_island::Access access{
                    peek_memory, island_write_memory, set_register};
                const auto route_result = oasis::hybrid::native_island::route_9d70(
                    native_island_selected, native_helper_selected == "9d70" &&
                        (!diagnostic_replay || !diagnostic_native_consumed),
                    true, address, previous_pc, read_be(peek_memory, previous_pc, 4),
                    stack_return, "M68K_009D70_CALLER_009144_V1", native_state,
                    access, island_counters);
                if (route_result == oasis::hybrid::native_island::Result::FatalAbort) {
                    hook_error = "native route rollback failed; emulator stopped";
                    if (stop_core) stop_core();
                }
                if (route_result == oasis::hybrid::native_island::Result::NativeRouted) {
                    if (equivalence_trace) equivalence_trace->mark_native_route(address);
                    diagnostic_native_consumed = true;
                    *call_output << "{\"kind\":\"NATIVE_ROUTE_STATE\",\"frame\":" << frame_number + 1
                        << ",\"source_pc\":" << previous_pc << ",\"stack_return\":" << stack_return
                        << ",\"pre\":" << oasis::hybrid::session::serialize_cpu_snapshot(state) << ",\"post\":" << oasis::hybrid::session::serialize_cpu_snapshot(read_cpu_snapshot(read_register)) << "}\n";
                }
            }
            ++frame_exec_count;
            pc_fingerprint = oasis::hybrid::session::append_pc_fingerprint(pc_fingerprint, address);
            bool watched = false;
            for (const auto pc : watch_pcs) if (pc == address) {
                ++watched_hits[pc]; ++frame_watched_hits[pc];
                watched = true;
            }
            if (watched) {
                const auto state = read_cpu_snapshot(read_register);
                *call_output << "{\"kind\":\"PC_WATCH\",\"frame\":" << frame_number + 1
                    << ",\"pc\":" << address << ",\"previous_pc\":" << previous_pc
                    << ",\"transfer_opcode\":" << read_be(peek_memory, previous_pc, 2)
                    << ",\"entry\":" << oasis::hybrid::session::serialize_cpu_snapshot(state) << "}\n";
            }
            if (address == source_pc) call.source = read_cpu_snapshot(read_register);
            if (address == target_pc && previous_pc == source_pc) {
                call.active = true; call.entry = read_cpu_snapshot(read_register);
                call.source_opcode = read_be(peek_memory, source_pc, 4); call.entry_opcode = read_be(peek_memory, target_pc, 2);
                call.stack_return = read_be(peek_memory, call.entry.a[7] & 0xFFFFFFU, 4) & 0xFFFFFFU;
                if (call.stack_return != return_pc) throw std::runtime_error("watched caller return address mismatch");
                call.native_grid = oasis::hybrid::grid_query::predict(
                    call.entry.d, call.entry.a, peek_memory);
                call.native_d5_match.reset();
                call_ram_before = read_work_ram(peek_memory); call.events.clear(); call.pcs.clear();
                call.saw_rts = call.saw_return = call.saw_continuation = false;
            } else if (call.active) {
                call.pcs.push_back(address);
                if (address == rts_pc) {
                    call.rts = read_cpu_snapshot(read_register); call.saw_rts = true;
                    if (call.native_grid)
                        call.native_d5_match = call.native_grid->d5 == call.rts.d[5];
                }
                if (address == return_pc) { call.returned = read_cpu_snapshot(read_register); call.saw_return = true; }
                if (address == finish_pc) {
                    call.continuation = read_cpu_snapshot(read_register);
                    call.saw_continuation = true;
                    if (!call.saw_rts || !call.saw_return)
                        throw std::runtime_error("watched full call missed RTS or caller return");
                    *call_output << call_json() << '\n';
                    call.active = false;
                }
            }
            previous_pc = address;
        } else if (call.active && (type & (read_event | write_event)) != 0) {
            if (call.events.size() >= 100000) throw std::runtime_error("target call event cap exceeded");
            call.events.push_back({type, read_register(16) & 0xFFFFFFU, address,
                                   static_cast<unsigned>(width), value});
        }
    } catch (const std::exception& error) { hook_error = error.what(); }
    return value;
}
} // namespace
namespace {
std::string output_dir;
unsigned bytes_per_pixel = 2;
void* callback_session{};
unsigned current_mask{};
InputMode active_input_mode{InputMode::Fallback};
bool callback_failed{};
std::string input_error;
unsigned retro_frame{};

void video(const void*, unsigned, unsigned, std::size_t) {}
void audio(std::int16_t, std::int16_t) {}
std::size_t audio_batch(const std::int16_t*, std::size_t count) { return count; }
bool environment(unsigned command, void* data) {
    if (command == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || command == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        *static_cast<const char**>(data) = output_dir.c_str(); return true;
    }
    if (command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        bytes_per_pixel = *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2; return true;
    }
    if (command == RETRO_ENVIRONMENT_GET_CAN_DUPE) { *static_cast<bool*>(data) = true; return true; }
    return false;
}
void input_poll() {
    try {
        auto& j = *static_cast<InputJournal*>(callback_session);
        if (poll_index && frame_poll_count == poll_index) j.end_poll();
        const auto sample = j.mode() == InputJournal::Mode::Replay ? Sample{} : capture_sample(retro_frame, active_input_mode);
        current_mask = j.begin_poll(retro_frame, poll_index, sample.buttons, sample.source);
        ++poll_index; ++frame_poll_count;
    } catch (const std::exception& error) { callback_failed = true; input_error = error.what(); }
}
std::int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    try {
        const auto value = port == 0 && device == RETRO_DEVICE_JOYPAD && id < 16 &&
            (current_mask & (1U << id)) ? 1 : 0;
        return journal->record_query({
            static_cast<std::uint8_t>(port), static_cast<std::uint8_t>(device),
            static_cast<std::uint16_t>(index), static_cast<std::uint16_t>(id)},
            static_cast<std::int16_t>(value));
    } catch (const std::exception& error) { callback_failed = true; input_error = error.what(); return 0; }
}
}
int main(int argc, char** argv) {
    if (argc < 5 || argc > 8) {
        std::cerr << "usage: oasis_gameplay_session <capture|replay> <GPGX core> <canonical ROM> <session-dir> [frames] [input-mode|--native-island=9d70-step] [--restore-initial-state]\n";
        return 2;
    }
    try {
        const std::string mode = argv[1];
        if (mode != "capture" && mode != "replay") throw std::runtime_error("invalid session mode");
        const bool replay = mode == "replay";
        if (replay && argc >= 6) {
            const auto options = oasis::hybrid::session::parse_replay_options(
                argc, argv, oasis::hybrid::native_island::supported_helper);
            restore_initial_state = options.restore_initial_state;
            diagnostic_replay = options.diagnostic;
            native_helper_selected = options.native_helper;
            native_island_selected = options.native_island;
        }
        if (replay && argc > 8) throw std::runtime_error("too many replay options");
        if (!replay && argc >= 6 && std::string(argv[5]).starts_with("--native-island="))
            throw std::runtime_error("native island is replay-only");
        const auto core_path = std::filesystem::absolute(argv[2]);
        const auto rom_path = std::filesystem::absolute(argv[3]);
        const auto session_path = std::filesystem::absolute(argv[4]);
        std::filesystem::create_directories(session_path);
        const auto manifest_path = session_path / "session.manifest";
        const auto polls_path = session_path / "input.polls";
        const auto frames_path = session_path / "landmarks.jsonl";
        const auto calls_path = session_path / "calls.jsonl";
        auto manifest = replay ? read_manifest(manifest_path) : std::map<std::string, std::string>{};
        const auto frames = replay ? static_cast<unsigned>(std::stoul(manifest.at("frames"))) :
            argc >= 6 && std::string(argv[5]).find_first_not_of("0123456789") == std::string::npos ?
            static_cast<unsigned>(std::stoul(argv[5])) : 3600U;
        std::string input_mode = replay ? manifest.at("input_mode") : "fallback";
        if (!replay && argc == 6 && std::string(argv[5]).find_first_not_of("0123456789") != std::string::npos)
            input_mode = argv[5];
        if (!replay && argc == 7) {
            if (std::string(argv[5]).find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("frame count must precede input mode");
            input_mode = argv[6];
        }
        if (!replay && input_mode != "fallback" && input_mode != "physical" &&
            input_mode != "scripted" && input_mode != "live")
            throw std::runtime_error("input mode must be fallback, physical, scripted, or live");
        if (!frames || frames > 1000000) throw std::runtime_error("invalid frame count");
        const auto rom = oasis::Rom::load(rom_path.string());
        replay_call_census.set_rom(rom.bytes().data(), rom.bytes().size());
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.fingerprint.sha256 != canonical_sha) throw std::runtime_error("canonical ROM identity mismatch");
        const auto executable = executable_path();
        const auto config = config_identity(frames, input_mode);
        const Manifest fixed{
            {"rom_sha256", identity.fingerprint.sha256}, {"rom_size", std::to_string(identity.fingerprint.size)},
            {"core_path", core_path.string()}, {"core_sha256", file_sha256(core_path)},
            {"runner_path", executable}, {"runner_sha256", file_sha256(executable)},
            {"config", config}, {"config_sha256", oasis::calculate_sha256({reinterpret_cast<const std::uint8_t*>(config.data()), config.size()})},
            {"frames", std::to_string(frames)}, {"input_mode", input_mode},
            {"reset_method", "retro_load_game_cold_start"}, {"landmark_period", std::to_string(landmark_period)},
            {"target_watch", "0xE376,0x9C40,0x94A2,0x8EF8,0xB730,0x16A46"},
            {"call_watch", "0xE376:0x9C40:0xE37A:0xE396;RTS=0x9C7A"},
            {"call_census", "M68K_DIRECT_BSR_JSR_ABS_JSR_AN_V1"},
            {"cross_cpu_landmark", "UNAVAILABLE_M68K_HOOK_ONLY"},
        };
        if (replay) for (const auto& [key, value] : fixed)
            if (manifest.at(key) != value) throw std::runtime_error("session identity mismatch: " + key);
        if (replay && file_sha256(polls_path) != manifest.at("input_sha256"))
            throw std::runtime_error("input log hash mismatch");
        const auto run_path = session_path / (replay ? "replay-run" : "capture-run");
        if (std::filesystem::exists(run_path)) throw std::runtime_error("run output directory already exists");
        std::filesystem::create_directories(run_path);
        if (diagnostic_replay) {
            equivalence_trace_owner = std::make_unique<oasis::hybrid::session::ReplayEquivalenceTrace>(run_path.string());
            equivalence_trace = equivalence_trace_owner.get();
        }
        island_metrics_path = run_path / "native-island.metrics.json";
        const auto calls_run_path = run_path / "calls.jsonl";
        oasis::hybrid::session::ReplayLandmarkDiagnostic landmark_diagnostic(
            (run_path / "observed-landmarks.jsonl").string());
        const auto census_path = replay ? run_path / "call-census.json" : session_path / "call-census.json";
        const auto full_calls_path = replay ? run_path / "auto-full-calls.jsonl" : session_path / "auto-full-calls.jsonl";
        output_dir = run_path.string();
        auto input = replay ? InputJournal::load(polls_path) : InputJournal(InputJournal::Mode::Capture);
        journal = &input; callback_session = &input;
        active_input_mode = input_mode == "physical" ? InputMode::Physical :
            input_mode == "scripted" ? InputMode::Scripted :
            input_mode == "live" ? InputMode::LiveOverride : InputMode::Fallback;
        std::ofstream calls(calls_run_path, std::ios::trunc);
        calls.exceptions(std::ios::failbit | std::ios::badbit); call_output = &calls;
        std::ifstream expected_frames;
        std::ofstream actual_frames;
        if (replay) expected_frames.open(frames_path);
        else actual_frames.open(frames_path, std::ios::trunc);
        if ((!replay && !actual_frames) || (replay && !expected_frames)) throw std::runtime_error("landmark file open failed");
        const auto abi = "GPGX_CAPTURE_BRIDGE_ABI=1";
        #ifdef _WIN32
        _putenv_s("GPGX_HYBRID_ONLY", abi);
        #else
        setenv("GPGX_HYBRID_ONLY", "1", 1);
        #endif
        const auto core_path_text = core_path.string();
        oasis::hybrid::ExternalLibrary core(core_path_text.c_str());
        if (core.get<unsigned(*)()>("retro_9c40_capture_abi")() != 1) throw std::runtime_error("HOOK_CPU bridge ABI mismatch");
        read_register = core.get<RegisterRead>("retro_9c40_capture_register");
        set_register = core.get<RegisterWrite>("retro_9c40_capture_set_register");
        peek_memory = core.get<Peek>("retro_9c40_capture_peek");
        write_memory = core.get<WriteMemory>("retro_9c40_capture_write");
        stop_core = core.get<StopCore>("retro_9c40_capture_stop");
        read_scheduler = core.get<oasis::hybrid::session::SchedulerRead>("retro_9c40_capture_scheduler");
        copy_observable_memory = core.get<oasis::hybrid::session::CopyObservableMemory>("retro_9c40_capture_copy_observable");
        core.get<Install>("retro_9c40_capture_install")(bridge_hook);
        if (native_helper_selected == "9d70-step" || native_helper_selected.starts_with("step:"))
            oasis::hybrid::install_m68k_gameplay_step_set(core,
                native_helper_selected == "9d70-step" ? "9d70" : native_helper_selected.substr(5),
                observe_step_instruction, fail_9d70_instruction_step);
        core.get<void(*)(decltype(&environment))>("retro_set_environment")(environment);
        core.get<void(*)(decltype(&video))>("retro_set_video_refresh")(video);
        core.get<void(*)(decltype(&audio))>("retro_set_audio_sample")(audio);
        core.get<void(*)(decltype(&audio_batch))>("retro_set_audio_sample_batch")(audio_batch);
        core.get<void(*)(decltype(&input_poll))>("retro_set_input_poll")(input_poll);
        core.get<void(*)(decltype(&input_state))>("retro_set_input_state")(input_state);
        core.get<void(*)()>("retro_init")();
        const auto rom_path_text = rom_path.string();
        const retro_game_info game{rom_path_text.c_str(), rom.bytes().data(), rom.size(), nullptr};
        if (!core.get<bool(*)(const retro_game_info*)>("retro_load_game")(&game)) throw std::runtime_error("core rejected canonical ROM");
        const auto state_size = core.get<std::size_t(*)()>("retro_serialize_size");
        const auto serialize = core.get<bool(*)(void*, std::size_t)>("retro_serialize");
        const auto initial = serialize_core_state(state_size, serialize);
        auto initial_sha = oasis::calculate_sha256(initial);
        if (replay && restore_initial_state) {
            std::ifstream snapshot(session_path / "initial.state", std::ios::binary);
            if (!snapshot) throw std::runtime_error("captured initial-state file missing");
            std::vector<std::uint8_t> saved((std::istreambuf_iterator<char>(snapshot)), {});
            const auto snapshot_sha = oasis::calculate_sha256(saved);
            if (saved.size() != state_size() || snapshot_sha != manifest.at("initial_state_sha256") ||
                !core.get<bool(*)(const void*, std::size_t)>("retro_unserialize")(saved.data(), saved.size()))
                throw std::runtime_error("captured initial-state restore failed closed");
            initial_sha = oasis::calculate_sha256(serialize_core_state(state_size, serialize));
            if (manifest.contains("replay_restored_initial_sha256") &&
                initial_sha != manifest.at("replay_restored_initial_sha256"))
                throw std::runtime_error("restored initial-state identity mismatch");
            std::ofstream receipt(run_path / "replay-start-state.json");
            receipt << "{\"mode\":\"captured_snapshot_restore\",\"cold_start_sha256\":\""
                << oasis::calculate_sha256(initial) << "\",\"restored_sha256\":\""
                << initial_sha << "\",\"snapshot_sha256\":\"" << snapshot_sha << "\"}\n";
        }
        if (replay && !restore_initial_state && initial_sha != manifest.at("initial_state_sha256")) {
            throw std::runtime_error("initial serialized state mismatch: expected " +
                manifest.at("initial_state_sha256") + " actual " + initial_sha);
        }
        if (!replay) {
            std::ofstream initial_file(session_path / "initial.state", std::ios::binary);
            initial_file.write(reinterpret_cast<const char*>(initial.data()), static_cast<std::streamsize>(initial.size()));
            manifest.insert(fixed.begin(), fixed.end());
            manifest["initial_state_sha256"] = initial_sha;
            manifest["initial_state_size"] = std::to_string(initial.size());
        }
        auto run = core.get<void(*)()>("retro_run");
        for (frame_number = 0; frame_number < frames && hook_error.empty() && !callback_failed; ++frame_number) {
            retro_frame = frame_number; poll_index = frame_poll_count = frame_exec_count = 0;
            pc_fingerprint = 14695981039346656037ULL; frame_watched_hits.clear();
            run();
            if (equivalence_trace) equivalence_trace->observe_frame(frame_number + 1,
                read_register, read_scheduler, copy_observable_memory);
            input.end_frame(frame_number, frame_poll_count);
            std::vector<std::uint8_t> state, ram;
            const bool checkpoint = (frame_number + 1) % landmark_period == 0 || frame_number + 1 == frames;
            if (checkpoint) { state = serialize_core_state(state_size, serialize); ram = read_work_ram(peek_memory); }
            if (checkpoint) oasis::hybrid::maybe_record_checkpoint(run_path, frame_number + 1, state,
                core.get<bool(*)(const void*, std::size_t)>("retro_unserialize"), serialize);
            FrameLandmark landmark{frame_number + 1, frame_poll_count, frame_exec_count,
                pc_fingerprint, read_register(16), checkpoint ? oasis::hybrid::checkpoint_identity_hash(state) : "",
                checkpoint ? oasis::calculate_sha256(ram) : "", 0, 0, 0, &frame_watched_hits};
            if (checkpoint) {
                landmark.grid_selector = read_be(peek_memory, 0xFF16F0, 2);
                landmark.object_state = read_be(peek_memory, 0xFF1706, 4);
                landmark.grid_descriptor = read_be(peek_memory, 0xFF1766, 4);
            }
            const auto line = serialize_frame_landmark(landmark);
            if (replay) {
                const bool equal = landmark_diagnostic.observe(expected_frames, line, frame_number + 1);
                if (!equal && !diagnostic_replay)
                    throw std::runtime_error("landmark divergence at frame " + std::to_string(frame_number + 1));
            }
            else actual_frames << line << '\n';
        }
        if (!hook_error.empty()) throw std::runtime_error(hook_error);
        if (callback_failed) throw std::runtime_error(input_error);
        if (call.active) throw std::runtime_error("target watch did not reach configured continuation");
        replay_call_census.write(census_path, full_calls_path);
        input.finish();
        if (replay) {
            std::string extra;
            if (std::getline(expected_frames, extra)) throw std::runtime_error("extra recorded frame after replay end");
            if (diagnostic_replay) {
                calls.flush(); calls.close();
                replay_call_census.write(census_path, full_calls_path);
                input.finish();
                equivalence_trace_owner->write_receipt((run_path / "normalized-trace.receipt.json").string());
                landmark_diagnostic.write_receipt((run_path / "landmark-diagnostic.json").string());
                write_island_metrics("DIAGNOSTIC_ONLY", "see landmark-diagnostic.json");
                std::cout << "DIAGNOSTIC_REPLAY_COMPLETE frames=" << frames << " divergence="
                    << (landmark_diagnostic.has_divergence() ? "YES" : "NO") << '\n';
                core.get<void(*)()>("retro_unload_game")(); core.get<void(*)()>("retro_deinit")();
                return 0;
            }
            calls.flush(); calls.close();
            if (file_sha256(calls_run_path) != manifest.at("calls_sha256")) throw std::runtime_error("target call capture mismatch");
            if (file_sha256(census_path) != manifest.at("call_census_sha256") ||
                file_sha256(full_calls_path) != manifest.at("auto_full_calls_sha256"))
                throw std::runtime_error("replay callable census or auto-capture mismatch");
            write_replay_result(run_path / "replay.result", frames, input.polls().size(),
                file_sha256(polls_path), manifest.at("landmarks_sha256"), manifest.at("calls_sha256"),
                file_sha256(census_path), file_sha256(full_calls_path));
            std::cout << "REPLAY_PASS frames=" << frames << " polls=" << input.polls().size() << '\n';
            write_island_metrics(native_helper_selected == "9d70-step" ? "INSTRUCTION_STEP_DIAGNOSTIC_PASS" : "PASS", "");
        } else {
            actual_frames.close(); input.save(polls_path); calls.flush(); calls.close();
            std::filesystem::copy_file(calls_run_path, calls_path);
            manifest["input_sha256"] = file_sha256(polls_path);
            manifest["landmarks_sha256"] = file_sha256(frames_path);
            manifest["calls_sha256"] = file_sha256(calls_path);
            manifest["call_census_sha256"] = file_sha256(census_path);
            manifest["auto_full_calls_sha256"] = file_sha256(full_calls_path);
            manifest["input_poll_count"] = std::to_string(input.polls().size());
            write_manifest(manifest_path, manifest);
            std::cout << "CAPTURE_COMPLETE frames=" << frames << " polls=" << input.polls().size() << '\n';
        }
        core.get<void(*)()>("retro_unload_game")(); core.get<void(*)()>("retro_deinit")();
        return 0;
    } catch (const std::exception& error) {
        write_island_metrics("FAIL", error.what());
        std::cerr << "SESSION_FAIL_CLOSED: " << error.what() << '\n';
        return 1;
    }
}
