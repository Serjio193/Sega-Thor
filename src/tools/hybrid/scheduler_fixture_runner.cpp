#include "tools/hybrid/external_library.hpp"
#include "core/rom_identity.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Symbol = oasis::hybrid::ExternalLibrary;
constexpr unsigned kProgram = 0x00FF1000;
constexpr unsigned kHandler = 0x00FF1200;
constexpr unsigned kStack = 0x00FF1FFC;
constexpr unsigned kPostHook = 1U << 14;
constexpr unsigned kProgramLimit = kProgram + 0x100;

struct Case {
    const char* name;
    std::vector<unsigned> words;
    int cycles;
    int refresh;
    unsigned sr;
    bool h_irq;
};

struct Event {
    int kind{}, arg0{}, arg1{};
    std::array<int, 16> scheduler{};
    std::array<int, 18> m68k{};
    std::array<int, 17> z80{};
    std::array<int, 9> vdp{};
};
struct BusWrite { int type{}, width{}; unsigned address{}, value{}; };
struct Instruction {
    unsigned pc{}, opcode{};
    int before_cycles{}, before_refresh{}, before_sr{};
    unsigned after_pc{};
    int after_cycles{}, after_refresh{}, after_sr{}, boundary{};
};

int (*scheduler_field)(unsigned){};
int (*z80_field)(unsigned){};
int (*vdp_field)(unsigned){};
int (*peek)(unsigned){};
unsigned (*reg)(unsigned){};
unsigned (*fetch16)(){};
void (*begin_instruction)(unsigned){};
void (*finish_instruction)(unsigned){};
void (*set_reg)(unsigned, unsigned){};
void (*set_return_state)(unsigned, unsigned, unsigned){};
void (*add_cycles)(int){};
unsigned (*instruction_cycles)(unsigned){};
unsigned (*boundary_reason)(){};
int (*not_taken_adjustment)(){};
bool stepping{};
bool irq_case{};
std::string active_case;
std::vector<Event> events;
std::vector<BusWrite> writes;
std::vector<Instruction> instructions;
unsigned current_instruction{};
bool current_instruction_valid{};

void add_event(int kind, int arg0, int arg1) {
    Event event;
    event.kind = kind; event.arg0 = arg0; event.arg1 = arg1;
    for (unsigned i = 0; i < event.scheduler.size(); ++i) event.scheduler[i] = scheduler_field(i);
    for (unsigned i = 0; i < event.m68k.size(); ++i) event.m68k[i] = static_cast<int>(reg(i));
    for (unsigned i = 0; i < event.z80.size(); ++i) event.z80[i] = z80_field(i);
    for (unsigned i = 0; i < event.vdp.size(); ++i) event.vdp[i] = vdp_field(i);
    if (events.size() < 32768) events.push_back(event);
}

void add_marker_event(int kind, int arg0, int arg1) {
    Event event;
    event.kind = kind; event.arg0 = arg0; event.arg1 = arg1;
    events.push_back(event);
}

void scheduler_callback(int kind, int arg0, int arg1) { add_event(kind, arg0, arg1); }

bool in_test_code(unsigned pc) {
    return (pc >= kProgram && pc < kProgramLimit) || (irq_case && pc >= kHandler && pc < kHandler + 2);
}

void cpu_hook(hook_type_t type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if (type == HOOK_M68K_E && in_test_code(address)) {
        current_instruction = address;
        Instruction step;
        step.pc = address;
        step.opcode = (static_cast<unsigned>(peek(address)) << 8) |
                      static_cast<unsigned>(peek(address + 1));
        step.before_cycles = scheduler_field(0);
        step.before_refresh = scheduler_field(2);
        step.before_sr = static_cast<int>(reg(17));
        current_instruction_valid = instructions.size() < 20000;
        if (current_instruction_valid) instructions.push_back(step);
        add_marker_event(11, static_cast<int>(address), static_cast<int>(step.opcode));
    } else if (type == static_cast<hook_type_t>(kPostHook) && current_instruction_valid) {
        auto& step = instructions.back();
        step.after_pc = reg(16);
        step.after_cycles = scheduler_field(0);
        step.after_refresh = scheduler_field(2);
        step.after_sr = static_cast<int>(reg(17));
        step.boundary = boundary_reason();
        add_marker_event(12, static_cast<int>(reg(16)), scheduler_field(0));
        current_instruction_valid = false;
    }
    if (type & (HOOK_M68K_W | HOOK_VRAM_W | HOOK_CRAM_W | HOOK_VSRAM_W |
                HOOK_Z80_W | HOOK_VDP_REG)) {
        if (address >= 0xFF0000 || address >= 0xA00000)
        add_marker_event(13, static_cast<int>(address), static_cast<int>(value));
        if (writes.size() < 4096)
            writes.push_back({static_cast<int>(type), width, address, value});
    }
}

bool condition_true(unsigned opcode, unsigned sr) {
    const bool z = (sr & 4U) != 0;
    const bool n = (sr & 8U) != 0;
    const bool v = (sr & 2U) != 0;
    const bool c = (sr & 1U) != 0;
    switch ((opcode >> 8) & 0xFU) {
    case 0x6: return !z; // BNE
    case 0x7: return z;  // BEQ
    case 0xA: return !c;
    case 0xB: return c;
    case 0xC: return !n;
    case 0xD: return n;
    case 0xE: return n == v;
    case 0xF: return n != v;
    default: return false;
    }
}

void record_native_instruction(unsigned pc, unsigned opcode, int before_cycles,
                               int before_refresh, unsigned before_sr) {
    Instruction step;
    step.pc = pc; step.opcode = opcode;
    step.before_cycles = before_cycles; step.before_refresh = before_refresh;
    step.before_sr = static_cast<int>(before_sr);
    step.after_pc = reg(16);
    step.after_cycles = scheduler_field(0);
    step.after_refresh = scheduler_field(2);
    step.after_sr = static_cast<int>(reg(17));
    step.boundary = boundary_reason();
    if (instructions.size() < 20000) instructions.push_back(step);
}

int block_hook(unsigned pc) {
    if (!stepping || pc < kProgram || pc >= kProgramLimit) return 0;
    const int high = peek(pc);
    const int low = peek(pc + 1);
    if (high < 0 || low < 0) return 0;
    const unsigned opcode = (static_cast<unsigned>(high) << 8) | static_cast<unsigned>(low);
    const unsigned condition = (opcode >> 8) & 0xFU;
    const bool is_moveq = (opcode & 0xF100U) == 0x7000U;
    const bool is_nop = opcode == 0x4E71U;
    const bool is_short_branch = (opcode & 0xF000U) == 0x6000U &&
        (opcode & 0xFFU) != 0 && (condition == 0 || condition == 6 || condition == 7 ||
        (condition >= 0xA && condition <= 0xF));
    if (!is_moveq && !is_nop && !is_short_branch) return 0;
    add_marker_event(11, static_cast<int>(pc), static_cast<int>(opcode));
    if ((fetch16() & 0xFFFFU) != opcode) return 0;
    const int before_cycles = scheduler_field(0);
    const int before_refresh = scheduler_field(2);
    const unsigned before_sr = reg(17);
    begin_instruction(opcode);

    if (is_moveq) {
        const unsigned d = (opcode >> 9) & 7U;
        const unsigned value = static_cast<unsigned>(static_cast<std::int32_t>(
            static_cast<std::int8_t>(opcode & 0xFFU)));
        set_reg(d, value);
        const unsigned sr = reg(17);
        const unsigned flags = value == 0 ? 4U : (value & 0x80000000U) ? 8U : 0U;
        set_reg(17, (sr & ~0x0FU) | flags);
    } else if (is_short_branch) {
        const bool take = ((opcode >> 8) & 0xFU) == 0 ? true : condition_true(opcode, before_sr);
        if (take) {
            const auto displacement = static_cast<std::int8_t>(opcode & 0xFFU);
            const unsigned target = reg(16) + static_cast<std::int32_t>(displacement);
            const unsigned high = static_cast<unsigned>(peek(target)) & 0xFFU;
            const unsigned low = static_cast<unsigned>(peek(target + 1)) & 0xFFU;
            set_return_state(target, target + 2, (high << 8) | low);
        } else {
            finish_instruction(opcode);
            add_cycles(not_taken_adjustment());
            record_native_instruction(pc, opcode, before_cycles, before_refresh, before_sr);
            add_marker_event(12, static_cast<int>(reg(16)), scheduler_field(0));
            return 1;
        }
    } else if (is_nop) {
        // NOP has no architectural effect.
    } else {
        return 0;
    }

    finish_instruction(opcode);
    record_native_instruction(pc, opcode, before_cycles, before_refresh, before_sr);
    add_marker_event(12, static_cast<int>(reg(16)), scheduler_field(0));
    return 1;
}

bool environment(unsigned command, void* data) {
    switch (command) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *static_cast<bool*>(data) = true; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *static_cast<bool*>(data) = false; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        static_cast<retro_variable*>(data)->value = nullptr; return false;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *static_cast<const char**>(data) = "."; return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
    case RETRO_ENVIRONMENT_SET_MESSAGE:
        return true;
    default: return false;
    }
}
void video(const void*, unsigned, unsigned, std::size_t) {}
void audio(std::int16_t, std::int16_t) {}
std::size_t audio_batch(const std::int16_t*, std::size_t frames) { return frames; }
void input_poll() {}
std::int16_t input_state(unsigned, unsigned, unsigned, unsigned) { return 0; }

std::vector<std::uint8_t> read_file(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open ROM");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

Case get_case(const std::string& name) {
    if (name == "A_BEFORE") return {"A_BEFORE", {0x7001, 0x4E71, 0x60FE}, 100, 10000, 0x2000, false};
    if (name == "B_EXACT") return {"B_EXACT", {0x7001, 0x60FE}, 3392, 10000, 0x2000, false};
    if (name == "B_NEAR") return {"B_NEAR", {0x7001, 0x60FE}, 3391, 10000, 0x2000, false};
    if (name == "C_CROSS") return {"C_CROSS", {0x7001, 0x60FE}, 3393, 10000, 0x2000, false};
    if (name == "D_TAKEN") return {"D_TAKEN", {0x6604, 0x7007, 0x4E71, 0x7209, 0x60FE}, 0, 10000, 0x2000, false};
    if (name == "D_NOT_TAKEN") return {"D_NOT_TAKEN", {0x6604, 0x7007, 0x4E71, 0x7209, 0x60FE}, 0, 10000, 0x2004, false};
    if (name == "E_REFRESH") return {"E_REFRESH", {0x7001, 0x60FE}, 3000, 3000, 0x2000, false};
    if (name == "F_REFRESH_CROSS") return {"F_REFRESH_CROSS", {0x7001, 0x60FE}, 3393, 3393, 0x2000, false};
    if (name == "G_IRQ") return {"G_IRQ", {0x4E71, 0x60FE}, 0, 10000, 0x2000, true};
    if (name == "H_RESUME") return {"H_RESUME", {0x7201, 0x7402, 0x60FE}, 3393, 10000, 0x2000, false};
    throw std::runtime_error("unknown fixture case");
}

void emit_array(const int* values, std::size_t count) {
    std::cout << '[';
    for (std::size_t i = 0; i < count; ++i) { if (i) std::cout << ','; std::cout << values[i]; }
    std::cout << ']';
}
void emit_event(const Event& event) {
    std::cout << "{\"kind\":" << event.kind << ",\"arg0\":" << event.arg0
              << ",\"arg1\":" << event.arg1;
    if (event.kind >= 11) { std::cout << '}'; return; }
    std::cout << ",\"scheduler\":";
    emit_array(event.scheduler.data(), event.scheduler.size());
    std::cout << ",\"m68k\":"; emit_array(event.m68k.data(), event.m68k.size());
    std::cout << ",\"z80\":"; emit_array(event.z80.data(), event.z80.size());
    std::cout << ",\"vdp\":"; emit_array(event.vdp.data(), event.vdp.size());
    std::cout << '}';
}
void emit_instruction(const Instruction& step) {
    std::cout << "{\"pc\":" << step.pc << ",\"opcode\":" << step.opcode
              << ",\"before_cycles\":" << step.before_cycles
              << ",\"before_refresh\":" << step.before_refresh
              << ",\"before_sr\":" << step.before_sr
              << ",\"after_pc\":" << step.after_pc
              << ",\"after_cycles\":" << step.after_cycles
              << ",\"after_refresh\":" << step.after_refresh
              << ",\"after_sr\":" << step.after_sr
              << ",\"boundary\":" << step.boundary << '}';
}

int run(int argc, char** argv) {
    if (argc != 5) throw std::runtime_error("usage: scheduler_fixture_runner <core> <rom> <case> <ROM|STEP>");
    const auto test = get_case(argv[3]);
    stepping = std::string(argv[4]) == "STEP";
    if (!stepping && std::string(argv[4]) != "ROM") throw std::runtime_error("mode must be ROM or STEP");
    irq_case = test.h_irq;
    active_case = test.name;
    auto rom = read_file(argv[2]);
    const auto rom_hash = oasis::calculate_sha256(rom);
    Symbol core(argv[1]);
#define LOAD(type, name) auto name = core.get<type>(#name)
    LOAD(void(*)(retro_environment_t), retro_set_environment);
    LOAD(void(*)(retro_video_refresh_t), retro_set_video_refresh);
    LOAD(void(*)(retro_audio_sample_t), retro_set_audio_sample);
    LOAD(void(*)(retro_audio_sample_batch_t), retro_set_audio_sample_batch);
    LOAD(void(*)(retro_input_poll_t), retro_set_input_poll);
    LOAD(void(*)(retro_input_state_t), retro_set_input_state);
    LOAD(void(*)(), retro_init);
    LOAD(void(*)(), retro_deinit);
    LOAD(bool(*)(const retro_game_info*), retro_load_game);
    LOAD(void(*)(void(*)(int,int,int)), retro_hybrid_install_scheduler);
    LOAD(void(*)(void(*)(hook_type_t,int,unsigned,unsigned)), retro_hybrid_install);
    LOAD(void(*)(int(*)(unsigned)), retro_hybrid_install_block);
    LOAD(int(*)(unsigned,unsigned,unsigned,int,int,int,unsigned), retro_hybrid_fixture_prepare);
    LOAD(void(*)(unsigned), retro_hybrid_fixture_h_irq);
    LOAD(void(*)(unsigned,int,unsigned), retro_hybrid_poke);
    LOAD(int(*)(unsigned,unsigned), retro_hybrid_fixture_patch_rom);
    LOAD(void(*)(), retro_hybrid_fixture_run_frame);
    LOAD(unsigned(*)(), retro_hybrid_fixture_viewport_height);
    LOAD(void(*)(), retro_hybrid_fixture_enable_z80);
    LOAD(unsigned(*)(unsigned), retro_hybrid_register);
    LOAD(int(*)(unsigned), retro_hybrid_scheduler_field);
    LOAD(int(*)(unsigned), retro_hybrid_z80_field);
    LOAD(int(*)(unsigned), retro_hybrid_vdp_field);
    LOAD(int(*)(unsigned), retro_hybrid_peek);
    LOAD(unsigned(*)(unsigned,int), retro_hybrid_read);
    LOAD(unsigned(*)(), retro_hybrid_fetch16);
    LOAD(void(*)(unsigned), retro_hybrid_begin_instruction);
    LOAD(void(*)(unsigned), retro_hybrid_finish_instruction);
    LOAD(void(*)(unsigned,unsigned), retro_hybrid_set_register);
    LOAD(void(*)(unsigned,unsigned,unsigned), retro_hybrid_set_return_state);
    LOAD(void(*)(int), retro_hybrid_add_cycles);
    LOAD(unsigned(*)(unsigned), retro_hybrid_instruction_cycles);
    LOAD(unsigned(*)(), retro_hybrid_boundary_reason);
    LOAD(int(*)(), retro_hybrid_bcc_not_taken_adjustment);
#undef LOAD

    scheduler_field = retro_hybrid_scheduler_field; z80_field = retro_hybrid_z80_field;
    vdp_field = retro_hybrid_vdp_field; peek = retro_hybrid_peek; reg = retro_hybrid_register;
    fetch16 = retro_hybrid_fetch16; begin_instruction = retro_hybrid_begin_instruction;
    finish_instruction = retro_hybrid_finish_instruction; set_reg = retro_hybrid_set_register;
    set_return_state = retro_hybrid_set_return_state; add_cycles = retro_hybrid_add_cycles;
    instruction_cycles = retro_hybrid_instruction_cycles; boundary_reason = retro_hybrid_boundary_reason;
    not_taken_adjustment = retro_hybrid_bcc_not_taken_adjustment;

    retro_set_environment(environment); retro_set_video_refresh(video);
    retro_set_audio_sample(audio); retro_set_audio_sample_batch(audio_batch);
    retro_set_input_poll(input_poll); retro_set_input_state(input_state); retro_init();
    retro_game_info game{}; game.path = argv[2]; game.data = rom.data(); game.size = rom.size();
    if (!retro_load_game(&game)) throw std::runtime_error("retro_load_game failed");
    const unsigned handler[] = {0x4E73};
    for (std::size_t i = 0; i < test.words.size(); ++i)
        retro_hybrid_poke(kProgram + static_cast<unsigned>(i * 2), 2, test.words[i]);
    retro_hybrid_poke(kProgram + static_cast<unsigned>(test.words.size() * 2), 2, 0x60FE);
    retro_hybrid_poke(kHandler, 2, handler[0]);
    if (test.h_irq) {
        const unsigned vector = kHandler;
        for (unsigned i = 0; i < 4; ++i) {
            if (!retro_hybrid_fixture_patch_rom(0x70 + i, (vector >> (24 - 8 * i)) & 0xFFU))
                throw std::runtime_error("fixture interrupt vector patch rejected");
        }
    }
    const int first = static_cast<int>(peek(kProgram));
    const int second = static_cast<int>(peek(kProgram + 1));
    if (!retro_hybrid_fixture_prepare(kProgram, kStack, test.sr, test.cycles,
                                      test.refresh, test.cycles,
                                      retro_hybrid_fixture_viewport_height()))
        throw std::runtime_error("fixture initial state rejected");
    retro_hybrid_fixture_enable_z80();
    retro_hybrid_set_register(0, 0xA5A50000U);
    retro_hybrid_set_register(1, 0x5A5A0000U);
    retro_hybrid_install_scheduler(scheduler_callback);
    retro_hybrid_install(cpu_hook);
    retro_hybrid_install_block(stepping ? block_hook : nullptr);
    if (test.h_irq) retro_hybrid_fixture_h_irq(1);
    if (!first || second < 0) throw std::runtime_error("fixture program bytes unreadable");
    retro_hybrid_fixture_run_frame();

    std::vector<std::uint8_t> work_ram(0x10000);
    for (unsigned i = 0; i < work_ram.size(); ++i) {
        const int value = peek(0xFF0000U + i);
        if (value < 0) throw std::runtime_error("work RAM snapshot unavailable");
        work_ram[i] = static_cast<std::uint8_t>(value);
    }
    std::array<unsigned, 18> final_m68k{};
    for (unsigned i = 0; i < final_m68k.size(); ++i) final_m68k[i] = reg(i);
    std::array<int, 4> irq_vector{};
    for (unsigned i = 0; i < irq_vector.size(); ++i) irq_vector[i] = peek(0x70 + i);
    const unsigned irq_vector_read = retro_hybrid_read(0x70, 4);

    std::cout << "{\"case\":\"" << test.name << "\",\"mode\":\"" << argv[4]
              << "\",\"canonical_rom_sha256\":\"" << rom_hash << "\",\"irq_vector_read\":"
              << irq_vector_read << ",\"irq_vector\":[";
    for (unsigned i = 0; i < irq_vector.size(); ++i) { if (i) std::cout << ','; std::cout << irq_vector[i]; }
    std::cout << "],\"events\":[";
    for (std::size_t i = 0; i < events.size(); ++i) { if (i) std::cout << ','; emit_event(events[i]); }
    std::cout << "],\"instructions\":[";
    for (std::size_t i = 0; i < instructions.size(); ++i) { if (i) std::cout << ','; emit_instruction(instructions[i]); }
    std::cout << "],\"writes\":[";
    for (std::size_t i = 0; i < writes.size(); ++i) {
        if (i) std::cout << ',';
        const auto& w = writes[i];
        std::cout << "{\"type\":" << w.type << ",\"width\":" << w.width
                  << ",\"address\":" << w.address << ",\"value\":" << w.value << '}';
    }
    std::cout << "],\"final_m68k\":[";
    for (std::size_t i = 0; i < final_m68k.size(); ++i) { if (i) std::cout << ','; std::cout << final_m68k[i]; }
    std::cout << "],\"final_scheduler\":[";
    for (unsigned i = 0; i < 16; ++i) { if (i) std::cout << ','; std::cout << scheduler_field(i); }
    std::cout << "],\"final_z80\":[";
    for (unsigned i = 0; i < 17; ++i) { if (i) std::cout << ','; std::cout << z80_field(i); }
    std::cout << "],\"final_vdp\":[";
    for (unsigned i = 0; i < 9; ++i) { if (i) std::cout << ','; std::cout << vdp_field(i); }
    std::cout << "],\"work_ram_sha256\":\"" << oasis::calculate_sha256(work_ram) << "\"}\n";
    retro_deinit();
    return 0;
}
}

int main(int argc, char** argv) {
    try { return run(argc, argv); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
