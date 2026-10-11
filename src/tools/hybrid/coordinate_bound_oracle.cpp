#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/hybrid/coordinate_bound_model.hpp"
#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/grid_query_micro_oracle.hpp"

#include <libretro.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr unsigned entry_pc = 0x9D4EU, rts_pc = 0x9D6EU;
constexpr unsigned sentinel_pc = 0xFF6300U, object = 0xFF2000U;
constexpr unsigned descriptor = 0xFF3000U, stack = 0xFF20F0U;
constexpr int execute = 1, memory_write = 4;
using Register = unsigned (*)(unsigned);
using Peek = int (*)(unsigned);
using SetRegister = void (*)(unsigned, unsigned);
using WriteMemory = int (*)(unsigned, int, unsigned);
using RunCpu = void (*)(unsigned);
using StopCpu = void (*)();
using Install = void (*)(unsigned int (*)(int, int, unsigned, unsigned));

Register read_reg{};
Peek peek{};
SetRegister set_reg{};
WriteMemory write_mem{};
RunCpu run_cpu{};
StopCpu stop_cpu{};
const char* system_dir{};

struct State { std::array<unsigned, 8> d{}, a{}; unsigned pc{}, sr{}; };
struct Case { const char* name; unsigned d0, d1, offset; std::uint8_t flags; };
struct Observation {
    State entry{}, rts{}, exit{};
    std::vector<unsigned> path;
    std::vector<std::array<unsigned, 4>> writes;
    std::uint8_t flag_after{};
    bool complete{};
};
Observation* active{};
std::vector<Observation> observations;

State snapshot() {
    State result;
    for (unsigned i = 0; i < 8; ++i) {
        result.d[i] = read_reg(i);
        result.a[i] = read_reg(8U + i);
    }
    result.pc = read_reg(16U);
    result.sr = read_reg(17U) & 0xFFFFU;
    return result;
}

unsigned int hook(int type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if ((type & execute) != 0 && address == entry_pc && active == nullptr) {
        observations.emplace_back();
        active = &observations.back();
        active->entry = snapshot();
        active->path.push_back(address);
    } else if (active != nullptr && (type & memory_write) != 0) {
        active->writes.push_back({read_reg(16U), address,
            width > 0 ? static_cast<unsigned>(width) : 0U, value});
    } else if (active != nullptr && (type & execute) != 0 && address == rts_pc) {
        active->rts = snapshot();
        active->path.push_back(address);
    } else if (active != nullptr && (type & execute) != 0 && address == sentinel_pc) {
        active->exit = snapshot();
        active->complete = true;
        active = nullptr;
        stop_cpu();
    } else if (active != nullptr && (type & execute) != 0) {
        active->path.push_back(address);
    }
    return value;
}

bool environment(unsigned command, void* data) {
    switch (command) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *static_cast<const char**>(data) = system_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *static_cast<bool*>(data) = true;
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return true;
    default:
        return false;
    }
}
void video(const void*, unsigned, unsigned, std::size_t) {}
void audio(std::int16_t, std::int16_t) {}
std::size_t audio_batch(const std::int16_t*, std::size_t count) { return count; }
void input_poll() {}
std::int16_t input_state(unsigned, unsigned, unsigned, unsigned) { return 0; }

void write_value(unsigned address, int width, unsigned value) {
    if (!write_mem(address, width, value))
        throw std::runtime_error("synthetic guest RAM write failed");
}

bool matches(const Case& scenario, const Observation& observation) {
    const auto model = oasis::hybrid::apply_coordinate_bound({
        static_cast<std::uint16_t>(scenario.d0),
        static_cast<std::uint16_t>(scenario.d1),
        static_cast<std::uint16_t>(scenario.offset), 64U, scenario.flags});
    const auto low = [](unsigned value) { return value & 0xFFFFU; };
    const auto expected_flag = static_cast<unsigned>(model.flags);
    const int actual_flag = peek(object + 0x40U);
    bool pass = observation.complete && observation.path.size() >= 9U &&
        low(observation.rts.d[0]) == model.coordinate &&
        low(observation.rts.d[3]) == model.maximum &&
        low(observation.rts.d[7]) == model.coordinate_sum &&
        actual_flag >= 0 && static_cast<unsigned>(actual_flag) == expected_flag &&
        (observation.rts.d[0] & 0xFFFF0000U) ==
            (observation.entry.d[0] & 0xFFFF0000U) &&
        (observation.rts.d[3] & 0xFFFF0000U) ==
            (observation.entry.d[3] & 0xFFFF0000U) &&
        (observation.rts.d[7] & 0xFFFF0000U) ==
            (observation.entry.d[7] & 0xFFFF0000U) &&
        observation.rts.a[7] == observation.entry.a[7];
    const auto expected_writes = model.flags_written ? 1U : 0U;
    pass = pass && observation.writes.size() == expected_writes;
    if (model.flags_written && !observation.writes.empty()) {
        const auto& write = observation.writes.front();
        pass = pass && write[1] == object + 0x40U && write[2] == 1U &&
            (write[3] & 0xFFU) == expected_flag;
    }
    for (unsigned i = 0; i < 8; ++i) {
        if (i == 0 || i == 3 || i == 7) continue;
        pass = pass && observation.rts.d[i] == observation.entry.d[i];
    }
    for (unsigned i = 0; i < 7; ++i)
        pass = pass && observation.rts.a[i] == observation.entry.a[i];
    return pass;
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_coordinate_bound_oracle <HOOK_CPU DLL> <canonical ROM> <receipt.json>\n";
        return 2;
    }
    try {
        constexpr char rom_sha[] = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
        const auto rom = oasis::Rom::load(argv[2]);
        if (oasis::identify_rom(rom.bytes()).fingerprint.sha256 != rom_sha)
            throw std::runtime_error("pinned canonical ROM required");
        const auto output = std::filesystem::absolute(argv[3]);
        std::filesystem::create_directories(output.parent_path());
        const auto directory = output.parent_path().string();
        system_dir = directory.c_str();
        oasis::hybrid::ExternalLibrary lib(argv[1]);
        if (lib.get<unsigned(*)()>("retro_9c40_capture_abi")() != 1U)
            throw std::runtime_error("HOOK_CPU capture ABI mismatch");
        read_reg = lib.get<Register>("retro_9c40_capture_register");
        peek = lib.get<Peek>("retro_9c40_capture_peek");
        set_reg = lib.get<SetRegister>("retro_9c40_capture_set_register");
        write_mem = lib.get<WriteMemory>("retro_9c40_capture_write");
        run_cpu = lib.get<RunCpu>("retro_9c40_capture_run");
        stop_cpu = lib.get<StopCpu>("retro_9c40_capture_stop");
        lib.get<void(*)(decltype(&environment))>("retro_set_environment")(environment);
        lib.get<void(*)(decltype(&video))>("retro_set_video_refresh")(video);
        lib.get<void(*)(decltype(&audio))>("retro_set_audio_sample")(audio);
        lib.get<void(*)(decltype(&audio_batch))>("retro_set_audio_sample_batch")(audio_batch);
        lib.get<void(*)(decltype(&input_poll))>("retro_set_input_poll")(input_poll);
        lib.get<void(*)(decltype(&input_state))>("retro_set_input_state")(input_state);
        lib.get<void(*)()>("retro_init")();
        const retro_game_info game{argv[2], rom.bytes().data(), rom.size(), nullptr};
        if (!lib.get<bool(*)(const retro_game_info*)>("retro_load_game")(&game))
            throw std::runtime_error("canonical ROM load failed");
        const auto install = lib.get<Install>("retro_9c40_capture_install");
        const oasis::hybrid::MicroBridge bridge{
            read_reg, peek, set_reg, write_mem, run_cpu, stop_cpu, install};
        const auto micro = oasis::hybrid::run_m68k_rts_micro_oracle(bridge);
        if (!micro.passed)
            throw std::runtime_error("RTS harness micro-oracle failed: " + micro.failure);
        install(hook);
        constexpr std::array cases{
            Case{"in_range", 3U, 160U, 10U, 0x05U},
            Case{"clamp_nonzero", 600U, 1U, 10U, 0x05U},
            Case{"clamp_zero_and_set_flags", 1U, 502U, 9U, 0x21U},
        };
        for (const auto& scenario : cases) {
            write_value(object + 0x46U, 2, scenario.offset);
            write_value(object + 0x40U, 1, scenario.flags);
            write_value(descriptor + 4U, 2, 64U);
            write_value(stack, 4, sentinel_pc);
            for (unsigned i = 0; i < 8; ++i) {
                set_reg(i, 0xA0B00000U + i * 0x010101U);
                set_reg(8U + i, 0xFF1000U + i * 0x100U);
            }
            set_reg(0, 0x11220000U | scenario.d0);
            set_reg(1, 0x33440000U | scenario.d1);
            set_reg(3, 0x55667788U);
            set_reg(7, 0x99AABBCCU);
            set_reg(8, descriptor);
            set_reg(14, object);
            set_reg(15, stack);
            set_reg(16, entry_pc);
            set_reg(17, 0x2000U);
            const auto prior = observations.size();
            run_cpu(10000U);
            if (active != nullptr || observations.size() != prior + 1U ||
                !observations.back().complete)
                throw std::runtime_error(std::string("synthetic routine failed: ") + scenario.name);
            if (!matches(scenario, observations.back()))
                throw std::runtime_error(std::string("ROM/native mismatch: ") + scenario.name);
        }
        install(nullptr);
        std::ofstream out(output);
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out << "{\"schema\":\"oasis.m68k.9d4e-native-oracle.v1\",\"rom_sha256\":\""
            << rom_sha << "\",\"routine_range\":\"[0x009D4E,0x009D70)\",\"caller\":\"0x00909E BSR.W\","
            << "\"harness_validated\":true,\"rts_micro_oracle\":\"PASS\",\"case_count\":" << observations.size()
            << ",\"cases\":[";
        for (std::size_t i = 0; i < cases.size(); ++i) {
            if (i != 0U) out << ',';
            out << "{\"id\":\"" << cases[i].name << "\",\"matched\":true,\"path\":[";
            for (std::size_t n = 0; n < observations[i].path.size(); ++n)
                out << (n ? "," : "") << observations[i].path[n];
            out << "],\"write_count\":" << observations[i].writes.size() << '}';
        }
        out << "],\"all_match\":true,\"runtime_replacement_active\":false,"
            << "\"authority_deltas\":0}\n";
        lib.get<void(*)()>("retro_unload_game")();
        lib.get<void(*)()>("retro_deinit")();
        std::cout << "synthetic cases matched=" << observations.size() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
