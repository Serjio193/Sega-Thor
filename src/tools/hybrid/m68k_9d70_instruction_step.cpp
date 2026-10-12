#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/external_library.hpp"

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x009D70U;
constexpr unsigned kEnd = 0x009D92U;
constexpr unsigned kCaller = 0x009144U;
constexpr unsigned kReturn = 0x009148U;
constexpr unsigned kRtsKeep = 0x009D86U;
constexpr unsigned kRtsWrite = 0x009D90U;
constexpr unsigned kRamStart = 0x00FF0000U;
constexpr unsigned kRamEnd = 0x01000000U;

bool has_callbacks(const M68k9D70StepApi& api) noexcept {
    return api.read_register && api.write_register && api.peek && api.read &&
        api.write && api.fetch16 && api.set_return_state &&
        api.begin_instruction && api.finish_instruction && api.add_cycles &&
        api.word_not_taken_adjustment && api.observe_instruction;
}

bool be_read(const M68k9D70StepApi& api, unsigned address, unsigned width,
             unsigned& value) noexcept {
    value = 0;
    for (unsigned i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, address + i);
        if (byte < 0 || byte > 0xFF) return false;
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

bool accepted_caller(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned stack_return{}, call_word{}, displacement{};
    if (sp < kRamStart || sp > kRamEnd - 4U ||
        !be_read(api, sp, 4U, stack_return) || stack_return != kReturn ||
        !be_read(api, kCaller, 2U, call_word) || call_word != 0x6100U ||
        !be_read(api, kCaller + 2U, 2U, displacement)) return false;
    const auto signed_displacement = static_cast<std::int16_t>(displacement);
    return static_cast<unsigned>(static_cast<int>(kCaller + 2U) + signed_displacement) == kStart;
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t flags) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | flags);
}

std::uint16_t move_flags(std::uint16_t value, std::uint16_t old_ccr) noexcept {
    return static_cast<std::uint16_t>((old_ccr & 0x10U) |
        (value == 0 ? 0x04U : 0U) | ((value & 0x8000U) ? 0x08U : 0U));
}

std::uint16_t subtract_flags(std::uint16_t lhs, std::uint16_t rhs,
                             std::uint16_t result, std::uint16_t old_ccr,
                             bool update_extend) noexcept {
    const bool carry = lhs < rhs;
    const bool overflow = ((lhs ^ rhs) & (lhs ^ result) & 0x8000U) != 0U;
    const auto x = update_extend ? (carry ? 0x10U : 0U) : (old_ccr & 0x10U);
    return static_cast<std::uint16_t>(x | (result == 0 ? 0x04U : 0U) |
        ((result & 0x8000U) ? 0x08U : 0U) | (overflow ? 0x02U : 0U) |
        (carry ? 0x01U : 0U));
}

void finish(const M68k9D70StepApi& api, unsigned opcode) noexcept {
    api.finish_instruction(api.context, opcode);
}

M68k9D70StepResult fault() noexcept { return M68k9D70StepResult::ContractFault; }

} // namespace

M68k9D70StepResult M68k9D70InstructionStepper::step(
    unsigned pc, const M68k9D70StepApi& api) noexcept {
    pc &= 0xFFFFFFU;
    if (!has_callbacks(api)) return fault();
    if (!active_) {
        if (pc != kStart || consumed_ || !accepted_caller(api)) return M68k9D70StepResult::NotHandled;
        active_ = true;
        consumed_ = true;
    } else if (pc == kStart) {
        active_ = false;
        return fault(); // recursive entry is outside the accepted path contract
    }

    if (pc == kRtsKeep || pc == kRtsWrite) {
        active_ = false;
        return M68k9D70StepResult::NotHandled; // preserve the core's RTS implementation
    }
    if (pc < kStart || pc >= kEnd) return M68k9D70StepResult::NotHandled;

    unsigned instruction_word{};
    if (!be_read(api, pc, 2U, instruction_word)) return fault();
    const auto opcode = instruction_word & 0xFFFFU;
    const auto a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    if (pc == 0x009D72U || pc == 0x009D8AU) {
        unsigned extension{};
        if (!be_read(api, pc + 2U, pc == 0x009D72U ? 2U : 4U, extension)) return fault();
        const unsigned address = pc == 0x009D72U ? a6 + 0x46U : a6 + 0x40U;
        const unsigned width = pc == 0x009D72U ? 2U : 1U;
        if (extension != (pc == 0x009D72U ? 0x0046U : 0x00480040U) ||
            a6 < kRamStart || address > kRamEnd - width) return fault();
        int first = api.peek(api.context, address);
        const int second = width == 2U ? api.peek(api.context, address + 1U) : 0;
        if (first < 0 || first > 0xFF || second < 0 || second > 0xFF)
            return fault();
    }
    const bool branch = pc == 0x009D76U || pc == 0x009D7CU || pc == 0x009D82U;
    bool branch_taken{};
    if (branch) {
        unsigned displacement{};
        if (!be_read(api, pc + 2U, 2U, displacement)) return fault();
        const unsigned expected = pc == 0x009D76U ? 0x0010U :
            (pc == 0x009D7CU ? 0x0008U : 0x0006U);
        if (displacement != expected) return fault();
    }

    api.observe_instruction(api.context, pc, opcode);
    if ((api.fetch16(api.context) & 0xFFFFU) != opcode) return fault();
    api.begin_instruction(api.context, opcode);

    if (pc == 0x009D70U && opcode == 0x3601U) { // MOVE.W D1,D3
        const auto d1 = api.read_register(api.context, 1U);
        const auto d3 = api.read_register(api.context, 3U);
        const auto value = (d3 & 0xFFFF0000U) | (d1 & 0xFFFFU);
        api.write_register(api.context, 3U, value);
        set_ccr(api, move_flags(static_cast<std::uint16_t>(value), ccr(api)));
    } else if (pc == 0x009D72U && opcode == 0x966EU) { // SUB.W (d16,A6),D3
        if ((api.fetch16(api.context) & 0xFFFFU) != 0x0046U) return fault();
        const unsigned address = a6 + 0x46U;
        const auto source = static_cast<std::uint16_t>(api.read(api.context, address, 2));
        const auto d3 = api.read_register(api.context, 3U);
        const auto old_value = static_cast<std::uint16_t>(d3);
        const auto result = static_cast<std::uint16_t>(old_value - source);
        api.write_register(api.context, 3U, (d3 & 0xFFFF0000U) | result);
        set_ccr(api, subtract_flags(old_value, source, result, ccr(api), true));
    } else if (branch) {
        const unsigned extension_pc = api.read_register(api.context, 16U);
        const auto displacement = static_cast<std::int16_t>(api.fetch16(api.context));
        const auto flags = ccr(api);
        const bool take = pc == 0x009D76U ? (flags & 0x08U) != 0U :
            (pc == 0x009D7CU ? (flags & 0x05U) != 0U : (flags & 0x04U) != 0U);
        branch_taken = take;
        if (take) {
            const unsigned target = static_cast<unsigned>(
                static_cast<int>(extension_pc) + displacement) & 0xFFFFFFU;
            unsigned target_word{};
            if (!be_read(api, target, 2U, target_word)) return fault();
            api.set_return_state(api.context, target, target + 2U, target_word);
        }
    } else if (pc == 0x009D7AU && opcode == 0xB043U) { // CMP.W D3,D0
        const auto d0 = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
        const auto d3 = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
        const auto result = static_cast<std::uint16_t>(d0 - d3);
        set_ccr(api, subtract_flags(d0, d3, result, ccr(api), false));
    } else if (pc == 0x009D80U && opcode == 0x3003U) { // MOVE.W D3,D0
        const auto d3 = api.read_register(api.context, 3U);
        const auto d0 = api.read_register(api.context, 0U);
        const auto value = (d0 & 0xFFFF0000U) | (d3 & 0xFFFFU);
        api.write_register(api.context, 0U, value);
        set_ccr(api, move_flags(static_cast<std::uint16_t>(value), ccr(api)));
    } else if (pc == 0x009D88U && opcode == 0x4240U) { // CLR.W D0
        api.write_register(api.context, 0U, api.read_register(api.context, 0U) & 0xFFFF0000U);
        set_ccr(api, static_cast<std::uint16_t>((ccr(api) & 0x10U) | 0x04U));
    } else if (pc == 0x009D8AU && opcode == 0x002EU) { // ORI.B #$48,(d16,A6)
        if ((api.fetch16(api.context) & 0xFFFFU) != 0x0048U ||
            (api.fetch16(api.context) & 0xFFFFU) != 0x0040U) return fault();
        const unsigned address = a6 + 0x40U;
        const auto old_value = static_cast<std::uint8_t>(api.read(api.context, address, 1));
        const auto value = static_cast<std::uint8_t>(old_value | 0x48U);
        api.write(api.context, address, 1, value);
        set_ccr(api, static_cast<std::uint16_t>((ccr(api) & 0x10U) |
            (value == 0U ? 0x04U : 0U) | ((value & 0x80U) ? 0x08U : 0U)));
    } else {
        active_ = false;
        return fault();
    }

    finish(api, opcode);
    // The core table charges the taken cost; this exact word-branch adjustment
    // supplies the not-taken cost used by the ROM's Bcc.W implementation.
    if (branch && !branch_taken)
        api.add_cycles(api.context, api.word_not_taken_adjustment(api.context));
    return M68k9D70StepResult::InstructionStepped;
}

namespace {
using ReadRegister = std::uint32_t (*)(unsigned);
using WriteRegister = void (*)(unsigned, unsigned);
using PeekMemory = int (*)(unsigned);
using ReadMemory = std::uint32_t (*)(unsigned, int);
using WriteMemory = void (*)(unsigned, int, unsigned);
using Fetch16 = std::uint32_t (*)();
using SetReturn = void (*)(unsigned, unsigned, unsigned);
using Instruction = void (*)(unsigned);
using AddCycles = void (*)(int);
using WordBranchAdjustment = int (*)();
using InstallBlock = void (*)(int (*)(unsigned));

ReadRegister g_read_register{};
WriteRegister g_write_register{};
PeekMemory g_peek{};
ReadMemory g_read{};
WriteMemory g_write{};
Fetch16 g_fetch16{};
SetReturn g_set_return{};
Instruction g_begin_instruction{};
Instruction g_finish_instruction{};
AddCycles g_add_cycles{};
WordBranchAdjustment g_word_branch_adjustment{};
void (*g_observe)(unsigned, unsigned){};
void (*g_fault)(unsigned){};
M68k9D70InstructionStepper g_stepper;

std::uint32_t read_register(void*, unsigned index) { return g_read_register(index); }
void write_register(void*, unsigned index, std::uint32_t value) { g_write_register(index, value); }
int peek(void*, unsigned address) { return g_peek(address); }
std::uint32_t read(void*, unsigned address, int width) { return g_read(address, width); }
void write(void*, unsigned address, int width, std::uint32_t value) { g_write(address, width, value); }
std::uint32_t fetch16(void*) { return g_fetch16(); }
void set_return_state(void*, unsigned pc, unsigned pref_addr, unsigned pref_data) {
    g_set_return(pc, pref_addr, pref_data);
}
void begin_instruction(void*, unsigned opcode) { g_begin_instruction(opcode); }
void finish_instruction(void*, unsigned opcode) { g_finish_instruction(opcode); }
void add_cycles(void*, int cycles) { g_add_cycles(cycles); }
int word_not_taken_adjustment(void*) { return g_word_branch_adjustment(); }
void observe_instruction(void*, unsigned pc, unsigned opcode) { g_observe(pc, opcode); }

int block_hook(unsigned pc) {
    const M68k9D70StepApi api{nullptr, read_register, write_register, peek, read, write,
        fetch16, set_return_state, begin_instruction, finish_instruction, add_cycles,
        word_not_taken_adjustment, observe_instruction};
    switch (g_stepper.step(pc, api)) {
    case M68k9D70StepResult::NotHandled: return 0;
    case M68k9D70StepResult::InstructionStepped: return 1;
    case M68k9D70StepResult::ContractFault:
        if (g_fault) g_fault(pc);
        return 1; // Fail closed: never execute a partially consumed ROM instruction.
    }
    return 1;
}
} // namespace

void install_m68k_9d70_instruction_step(
    ExternalLibrary& core, void (*observe)(unsigned, unsigned),
    void (*fault)(unsigned)) {
    g_read_register = core.get<ReadRegister>("retro_9c40_capture_register");
    g_write_register = core.get<WriteRegister>("retro_9c40_capture_set_register");
    g_peek = core.get<PeekMemory>("retro_9c40_capture_peek");
    g_read = core.get<ReadMemory>("retro_hybrid_read");
    g_write = core.get<WriteMemory>("retro_hybrid_write");
    g_fetch16 = core.get<Fetch16>("retro_hybrid_fetch16");
    g_set_return = core.get<SetReturn>("retro_hybrid_set_return_state");
    g_begin_instruction = core.get<Instruction>("retro_hybrid_begin_instruction");
    g_finish_instruction = core.get<Instruction>("retro_hybrid_finish_instruction");
    g_add_cycles = core.get<AddCycles>("retro_hybrid_add_cycles");
    g_word_branch_adjustment = core.get<WordBranchAdjustment>(
        "retro_hybrid_bcc_not_taken_word_adjustment");
    g_observe = observe;
    g_fault = fault;
    if (!g_observe || !g_fault) throw std::runtime_error("9D70 step callbacks are required");
    core.get<InstallBlock>("retro_hybrid_install_block")(block_hook);
}

} // namespace oasis::hybrid
