#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/subtraction_bound_model.hpp"

#include <libretro.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr unsigned entry_pc = 0x9D70U;
constexpr unsigned sentinel_pc = 0xFF6300U;
constexpr unsigned object = 0xFF2000U;
constexpr unsigned stack = 0xFF20F0U;
constexpr int execute = 1, memory_write = 4;
using Register = unsigned (*)(unsigned);
using Peek = int (*)(unsigned);
using SetRegister = void (*)(unsigned, unsigned);
using WriteMemory = int (*)(unsigned, int, unsigned);
using RunCpu = void (*)(unsigned);
using StopCpu = void (*)();
using Install = void (*)(unsigned int (*)(int, int, unsigned, unsigned));

Register read_register{};
Peek peek_memory{};
SetRegister set_register{};
WriteMemory write_memory{};
RunCpu run_cpu{};
StopCpu stop_cpu{};
const char* system_directory{};

struct State { std::array<unsigned, 8> d{}, a{}; unsigned pc{}, sr{}; };
struct Scenario {
    const char* name;
    unsigned d0, d1, d3, limit;
    std::uint8_t marker;
    unsigned sr;
    unsigned expected_rts;
};
struct Observation {
    State entry{}, rts{}, exit{};
    std::vector<unsigned> path;
    std::vector<std::array<unsigned, 4>> writes;
    bool complete{};
};
Observation* active{};
std::vector<Observation> observations;

State snapshot() {
    State value;
    for (unsigned i = 0; i < 8; ++i) {
        value.d[i] = read_register(i);
        value.a[i] = read_register(i + 8U);
    }
    value.pc = read_register(16U);
    value.sr = read_register(17U) & 0xFFFFU;
    return value;
}

unsigned int hook(int type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if ((type & execute) != 0 && address == entry_pc && active == nullptr) {
        observations.emplace_back();
        active = &observations.back();
        active->entry = snapshot();
        active->path.push_back(address);
    } else if (active != nullptr && (type & memory_write) != 0) {
        active->writes.push_back({read_register(16U), address,
            width > 0 ? static_cast<unsigned>(width) : 0U, value});
    } else if (active != nullptr && (type & execute) != 0 &&
               (address == 0x9D86U || address == 0x9D90U)) {
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
        *static_cast<const char**>(data) = system_directory;
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
    if (!write_memory(address, width, value))
        throw std::runtime_error("synthetic guest RAM write failed");
}

bool matches(const Scenario& scenario, const Observation& observed) {
    const auto model = oasis::hybrid::apply_subtraction_bound({
        scenario.d0, scenario.d1, scenario.d3,
        static_cast<std::uint16_t>(scenario.limit), scenario.marker,
        static_cast<std::uint16_t>(scenario.sr)});
    const auto& state = observed.rts;
    if (!observed.complete || state.pc != scenario.expected_rts ||
        state.sr != model.sr || state.a != observed.entry.a || state.d[0] != model.d0 ||
        state.d[3] != model.d3 || observed.writes.size() !=
            (model.marker_written ? 1U : 0U)) {
        std::cerr << scenario.name << " rts=" << std::hex << state.pc
                  << " expected=" << scenario.expected_rts << " sr=" << state.sr
                  << " model=" << model.sr << " d0=" << state.d[0] << "/" << model.d0
                  << " d3=" << state.d[3] << "/" << model.d3 << " writes="
                  << std::dec << observed.writes.size() << '\n';
        return false;
    }
    for (unsigned i = 0; i < 8; ++i)
        if (i != 0U && i != 3U && state.d[i] != observed.entry.d[i]) {
            std::cerr << scenario.name << " unexpected D" << i << " change\n";
            return false;
        }
    if (model.marker_written) {
        const auto& write = observed.writes.front();
        if (write[1] != object + 0x40U ||
            write[2] != 1U || (write[3] & 0xFFU) != model.marker) {
            std::cerr << scenario.name << " write pc=" << std::hex << write[0]
                      << " addr=" << write[1] << " width=" << std::dec << write[2]
                      << " value=" << std::hex << write[3] << " expected="
                      << static_cast<unsigned>(model.marker) << '\n';
            return false;
        }
    }
    if (peek_memory(object + 0x40U) != model.marker) {
        std::cerr << scenario.name << " marker final mismatch\n";
        return false;
    }
    auto expected_exit_d = observed.entry.d;
    expected_exit_d[0] = model.d0;
    expected_exit_d[3] = model.d3;
    const bool exit_matches = observed.exit.pc == sentinel_pc && observed.exit.sr == model.sr &&
        observed.exit.d == expected_exit_d &&
        observed.exit.a[7] == observed.entry.a[7] + 4U;
    if (!exit_matches)
        std::cerr << scenario.name << " exit pc=" << std::hex << observed.exit.pc
                  << " sr=" << observed.exit.sr << "/" << model.sr << " A7="
                  << observed.exit.a[7] << "/" << (observed.entry.a[7] + 4U) << '\n';
    return exit_matches;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_subtraction_bound_oracle <HOOK_CPU DLL> <canonical ROM> <receipt.json>\n";
        return 2;
    }
    try {
        constexpr char rom_sha[] = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
        const auto rom = oasis::Rom::load(argv[2]);
        if (oasis::identify_rom(rom.bytes()).fingerprint.sha256 != rom_sha)
            throw std::runtime_error("pinned canonical ROM required");
        const auto output = std::filesystem::absolute(argv[3]);
        std::filesystem::create_directories(output.parent_path());
        const auto runtime_directory = output.parent_path().string();
        system_directory = runtime_directory.c_str();
        oasis::hybrid::ExternalLibrary library(argv[1]);
        if (library.get<unsigned(*)()>("retro_9c40_capture_abi")() != 1U)
            throw std::runtime_error("HOOK_CPU capture ABI mismatch");
        read_register = library.get<Register>("retro_9c40_capture_register");
        peek_memory = library.get<Peek>("retro_9c40_capture_peek");
        set_register = library.get<SetRegister>("retro_9c40_capture_set_register");
        write_memory = library.get<WriteMemory>("retro_9c40_capture_write");
        run_cpu = library.get<RunCpu>("retro_9c40_capture_run");
        stop_cpu = library.get<StopCpu>("retro_9c40_capture_stop");
        const auto install = library.get<Install>("retro_9c40_capture_install");
        library.get<void(*)(decltype(&environment))>("retro_set_environment")(environment);
        library.get<void(*)(decltype(&video))>("retro_set_video_refresh")(video);
        library.get<void(*)(decltype(&audio))>("retro_set_audio_sample")(audio);
        library.get<void(*)(decltype(&audio_batch))>("retro_set_audio_sample_batch")(audio_batch);
        library.get<void(*)(decltype(&input_poll))>("retro_set_input_poll")(input_poll);
        library.get<void(*)(decltype(&input_state))>("retro_set_input_state")(input_state);
        library.get<void(*)()>("retro_init")();
        const retro_game_info game{argv[2], rom.bytes().data(), rom.size(), nullptr};
        if (!library.get<bool(*)(const retro_game_info*)>("retro_load_game")(&game))
            throw std::runtime_error("canonical ROM load failed");
        install(hook);
        constexpr std::array scenarios{
            Scenario{"in-range-keep",0x20,0x30,0x7777,0x08,0x01,0x2000,0x9D86},
            Scenario{"clamp-nonzero",0x30,0x30,0x7777,0x08,0x02,0x2010,0x9D86},
            Scenario{"clamp-zero-write",0x01,0x08,0x7777,0x08,0x00,0x2000,0x9D90},
            Scenario{"negative-write",0xABCD,0x03,0x7777,0x05,0x11,0x2010,0x9D90},
        };
        for (const auto& scenario : scenarios) {
            write_value(object + 0x46U, 2, scenario.limit);
            write_value(object + 0x40U, 1, scenario.marker);
            write_value(stack, 4, sentinel_pc);
            for (unsigned i = 0; i < 8; ++i) {
                set_register(i, 0xA0B00000U + i * 0x010101U);
                set_register(i + 8U, 0xFF1000U + i * 0x100U);
            }
            set_register(0, scenario.d0);
            set_register(1, scenario.d1);
            set_register(3, scenario.d3);
            set_register(14, object);
            set_register(15, stack);
            set_register(16, entry_pc);
            set_register(17, scenario.sr);
            const auto before = observations.size();
            run_cpu(10000U);
            if (active || observations.size() != before + 1U ||
                !matches(scenario, observations.back()))
                throw std::runtime_error(std::string("ROM/model differential failed: ") + scenario.name);
        }
        install(nullptr);
        std::ofstream receipt(output);
        receipt.exceptions(std::ios::failbit | std::ios::badbit);
        receipt << "{\"schema\":\"oasis.m68k.9d70-rom-native-oracle.v1\","
                << "\"rom_sha256\":\"" << rom_sha << "\","
                << "\"routine_range\":\"[0x009D70,0x009D92)\","
                << "\"path_scope\":\"0x009144 BSR.W\","
                << "\"bridge_abi\":1,\"case_count\":" << scenarios.size()
                << ",\"all_match\":true,\"cases\":[";
        for (std::size_t i = 0; i < scenarios.size(); ++i)
            receipt << (i ? "," : "") << "{\"id\":\"" << scenarios[i].name
                    << "\",\"rts_pc\":\"0x" << std::hex << observations[i].rts.pc
                    << "\",\"write_count\":" << std::dec << observations[i].writes.size() << '}';
        receipt << "]}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
