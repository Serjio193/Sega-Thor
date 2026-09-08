#include "tools/hybrid/basic_block_reference.hpp"

#include <cstdint>
#include <map>
#include <stdexcept>

namespace oasis::hybrid {
namespace {

struct Context {
    BasicBlockApi source{};
    BasicBlockRegistry::Prediction result{};
    std::map<unsigned, std::uint8_t> overlay;
    unsigned instruction_limit{};
    unsigned instructions_seen{};
    static Context* current;
};

Context* Context::current = nullptr;

std::uint32_t read_word(const Context& context, unsigned address) {
    std::uint32_t value = 0;
    for (unsigned offset = 0; offset < 2; ++offset) {
        const auto it = context.overlay.find(address + offset);
        const auto byte = it == context.overlay.end() ?
            context.source.peek(address + offset) : static_cast<int>(it->second);
        if (byte < 0 || byte > 0xFF) throw std::runtime_error("generated prediction fetch failed");
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return value;
}

unsigned reg(unsigned index) { return Context::current->result.state.at(index); }
void set_reg(unsigned index, unsigned value) { Context::current->result.state.at(index) = value; }

int peek(unsigned address) {
    const auto it = Context::current->overlay.find(address);
    if (it != Context::current->overlay.end()) return it->second;
    return Context::current->source.peek(address);
}

unsigned fetch16() {
    auto& context = *Context::current;
    auto& result = context.result;
    if (result.state[16] != result.pref_addr) result.pref_data = read_word(context, result.state[16]);
    const auto value = result.pref_data;
    result.state[16] += 2U;
    result.pref_addr = result.state[16];
    result.pref_data = read_word(context, result.pref_addr);
    return value;
}

unsigned read(unsigned address, int width) {
    auto& context = *Context::current;
    unsigned value = 0;
    for (int offset = 0; offset < width; ++offset) {
        const auto byte = peek(address + static_cast<unsigned>(offset));
        if (byte < 0 || byte > 0xFF) throw std::runtime_error("generated prediction read failed");
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    context.result.reads.push_back({address, width});
    return value;
}

void write(unsigned address, int width, unsigned value) {
    auto& context = *Context::current;
    for (int offset = 0; offset < width; ++offset)
        context.overlay[address + static_cast<unsigned>(offset)] =
            static_cast<std::uint8_t>(value >> (8 * (width - offset - 1)));
    context.result.writes.push_back({address, width, value});
}

void begin_instruction(unsigned opcode) {
    auto& context = *Context::current;
    auto& result = context.result;
    const auto period = context.source.refresh_period();
    const auto penalty = context.source.refresh_penalty();
    if (static_cast<std::int32_t>(result.cycles) >= static_cast<std::int32_t>(result.refresh)) {
        result.refresh = result.cycles + period;
        result.cycles += penalty;
    }
    result.ir = opcode;
}

void finish_instruction(unsigned opcode) {
    auto& context = *Context::current;
    context.result.ir = opcode;
    context.result.cycles += context.source.instruction_cycles(opcode);
}

void add_cycles(int delta) {
    auto& cycles = Context::current->result.cycles;
    cycles = static_cast<unsigned>(static_cast<std::int64_t>(cycles) + delta);
}

void skip_bus_refresh() {
    auto& context = *Context::current;
    if (static_cast<std::int32_t>(context.result.cycles) >=
        static_cast<std::int32_t>(context.result.refresh))
        context.result.refresh += context.source.refresh_period();
}

BlockExitReason boundary_reason() {
    auto& context = *Context::current;
    ++context.instructions_seen;
    return context.instruction_limit &&
        context.instructions_seen >= context.instruction_limit ?
        BlockExitReason::EVENT_BOUNDARY : BlockExitReason::CONTINUE_BLOCK;
}

BasicBlockApi api_for(Context& context) {
    Context::current = &context;
    return {reg, set_reg, peek, fetch16, read, write, begin_instruction,
            finish_instruction, add_cycles, skip_bus_refresh,
            context.source.instruction_cycles, context.source.cpu_field,
            context.source.refresh_period, context.source.refresh_penalty,
            boundary_reason};
}

} // namespace

BasicBlockRegistry::Prediction predict_generated_block(
    const BasicBlockApi& source, const GeneratedBlockSpec& block,
    const BasicBlockRegistry::Prediction& entry, unsigned entry_pc,
    unsigned instruction_limit) {
    Context context{source};
    context.result = entry;
    context.result.state[16] = entry_pc;
    context.result.writes.clear();
    context.result.reads.clear();
    context.instruction_limit = instruction_limit;
    auto api = api_for(context);
    block.execute(api, entry_pc);
    Context::current = nullptr;
    return context.result;
}

} // namespace oasis::hybrid
