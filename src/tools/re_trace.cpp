#include "tools/re_trace.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace oasis::tools {
namespace {

using Bytes = std::span<const std::uint8_t>;

std::uint16_t read16(Bytes bytes, std::uint32_t address) {
    if (static_cast<std::size_t>(address) + 2U > bytes.size()) {
        throw std::out_of_range("trace ROM read16 out of range");
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[address]) << 8U) |
                                      bytes[address + 1U]);
}

std::uint32_t read32(Bytes bytes, std::uint32_t address) {
    return (static_cast<std::uint32_t>(read16(bytes, address)) << 16U) |
           read16(bytes, address + 2U);
}

std::uint32_t block_for(const DecodedSlice& slice, std::uint32_t address) {
    for (const auto& block : slice.basic_blocks) {
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(),
                      address) != block.instruction_addresses.end()) {
            return block.start;
        }
    }
    throw std::logic_error("trace PC is outside static basic blocks");
}

TraceRegisterSnapshot snapshot(std::optional<std::uint32_t> a0,
                               std::optional<std::uint32_t> a6) {
    return {.a0 = a0, .a6 = a6};
}

class ScenarioMachine {
public:
    ScenarioMachine(Bytes rom, TraceReport& report, const TraceOptions& options)
        : rom_(rom), report_(report), a6_(options.initial_a6), a0_(0),
          ram_(0x10000U, 0) {
        write16(options.initial_a6, options.initial_record_word);
        write32(options.initial_a6 + 0x22U, options.callback_target);
    }

    void run(const TraceOptions& options) {
        bool zero = false;
        auto pc = options.start_pc;
        for (std::size_t step = 0; step < options.instruction_budget; ++step) {
            const auto block = block_for(report_.static_slice, pc);
            const auto opcode = read16(rom_, pc);
            std::optional<TraceRegisterSnapshot> registers;
            if (pc == 0xA7D4U || pc == 0xA7DEU) registers = snapshot(std::nullopt, a6_);
            if (pc == 0xA7E2U) registers = snapshot(a0_, std::nullopt);
            report_.executed_instructions.push_back({step, pc, opcode, block, registers});
            if (report_.executed_basic_blocks.empty() || report_.executed_basic_blocks.back() != block) {
                report_.executed_basic_blocks.push_back(block);
            }

            if (pc == 0xA7D4U) {
                require(opcode, 0x0C6EU, "CMPI.W scenario opcode");
                const auto address = static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(static_cast<std::int16_t>(read16(rom_, pc + 4U))) +
                    a6_);
                const auto value = read_data16(address, pc, block, registers);
                zero = value == read16(rom_, pc + 2U);
                report_.dynamic_observed.push_back({pc, block, "memory_read", address, 2U});
                pc += 6U;
            } else if (pc == 0xA7DAU) {
                require(opcode, 0x6700U, "BEQ.W scenario opcode");
                const auto target = static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(static_cast<std::int16_t>(read16(rom_, pc + 2U))) +
                    pc + 2U);
                report_.branches.push_back({pc, block, target, zero});
                report_.dynamic_observed.push_back({pc, block, "branch_outcome", zero ? 1U : 0U, 1U});
                pc = zero ? target : pc + 4U;
            } else if (pc == 0xA7DEU) {
                require(opcode, 0x206EU, "MOVE.L scenario opcode");
                const auto address = static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(static_cast<std::int16_t>(read16(rom_, pc + 2U))) +
                    a6_);
                a0_ = read_data32(address, pc, block, registers);
                report_.dynamic_observed.push_back({pc, block, "memory_read", address, 4U});
                pc += 4U;
            } else if (pc == 0xA7E2U) {
                require(opcode, 0x4ED0U, "JMP (A0) scenario opcode");
                report_.indirect_targets.push_back({pc, block, a0_, registers});
                report_.dynamic_observed.push_back({pc, block, "indirect_control_flow", a0_, 4U});
                pc = a0_;
            } else if (pc == 0xA7E4U) {
                require(opcode, 0x4E75U, "RTS scenario opcode");
                report_.returns.push_back({pc, block});
                report_.dynamic_observed.push_back({pc, block, "return", 0U, 0U});
                report_.stop_reason = "return";
                return;
            } else {
                report_.stop_reason = "unsupported_scenario_opcode";
                return;
            }
        }
        report_.stop_reason = "instruction_budget";
    }

private:
    std::uint8_t raw8(std::uint32_t address) const {
        if (address < 0x00FF0000U || address > 0x00FFFFFFU) {
            throw std::out_of_range("trace RAM read outside work RAM");
        }
        return ram_[address - 0x00FF0000U];
    }

    std::uint16_t read_data16(std::uint32_t address, std::uint32_t instruction,
                              std::uint32_t block, std::optional<TraceRegisterSnapshot> registers) {
        const auto value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(raw8(address)) << 8U) |
                                                       raw8(address + 1U));
        report_.memory_reads.push_back({instruction, block, address, 2U, MemoryAccess::read, value, registers});
        return value;
    }

    std::uint32_t read_data32(std::uint32_t address, std::uint32_t instruction,
                              std::uint32_t block, std::optional<TraceRegisterSnapshot> registers) {
        const auto value = (static_cast<std::uint32_t>(raw8(address)) << 24U) |
                           (static_cast<std::uint32_t>(raw8(address + 1U)) << 16U) |
                           (static_cast<std::uint32_t>(raw8(address + 2U)) << 8U) |
                           raw8(address + 3U);
        report_.memory_reads.push_back({instruction, block, address, 4U, MemoryAccess::read, value, registers});
        return value;
    }

    void write16(std::uint32_t address, std::uint16_t value) {
        if (address < 0x00FF0000U || address + 1U > 0x00FFFFFFU) {
            throw std::out_of_range("trace RAM write16 outside work RAM");
        }
        ram_[address - 0x00FF0000U] = static_cast<std::uint8_t>(value >> 8U);
        ram_[address - 0x00FF0000U + 1U] = static_cast<std::uint8_t>(value);
    }

    void write32(std::uint32_t address, std::uint32_t value) {
        write16(address, static_cast<std::uint16_t>(value >> 16U));
        write16(address + 2U, static_cast<std::uint16_t>(value));
    }

    static void require(std::uint16_t actual, std::uint16_t expected, const char* context) {
        if (actual != expected) throw std::runtime_error(context);
    }

    Bytes rom_;
    TraceReport& report_;
    std::uint32_t a6_;
    std::uint32_t a0_;
    std::vector<std::uint8_t> ram_;
};

} // namespace

TraceReport trace_m68k_scenario(Bytes rom, const TraceOptions& options) {
    if (options.instruction_budget == 0U || options.static_byte_budget < 2U) {
        throw std::invalid_argument("invalid bounded trace options");
    }
    TraceReport report{.function_entry = options.function_entry,
                       .start_pc = options.start_pc,
                       .instruction_budget = options.instruction_budget,
                       .static_slice = decode_m68k_slice(
                           rom, {.entry = options.function_entry, .byte_budget = options.static_byte_budget})};

    std::map<std::uint32_t, std::uint32_t> blocks;
    for (const auto& block : report.static_slice.basic_blocks) {
        for (const auto address : block.instruction_addresses) blocks.emplace(address, block.start);
    }
    std::vector<StaticEvidence> static_unresolved;
    for (const auto& instruction : report.static_slice.instructions) {
        const auto block = blocks.at(instruction.address);
        for (const auto& reference : instruction.memory_references) {
            report.static_confirmed.push_back(
                {instruction.address, block, "confirmed_memory_reference", reference.address, "absolute"});
        }
        for (const auto& reference : instruction.unresolved_memory_references) {
            static_unresolved.push_back(
                {instruction.address, block, "unresolved_memory_reference", std::nullopt, reference.reason});
        }
    }
    for (const auto& flow : report.static_slice.unresolved_control_flow) {
        const auto block = blocks.at(flow.address);
        static_unresolved.push_back(
            {flow.address, block, "unresolved_control_flow", std::nullopt, flow_kind_name(flow.kind)});
    }

    ScenarioMachine machine(rom, report, options);
    machine.run(options);

    std::set<std::uint32_t> observed_memory;
    std::set<std::uint32_t> observed_indirect;
    for (const auto& item : report.dynamic_observed) {
        if (item.kind == "memory_read") observed_memory.insert(item.instruction_address);
        if (item.kind == "indirect_control_flow") observed_indirect.insert(item.instruction_address);
    }
    for (const auto& item : static_unresolved) {
        const bool resolved = (item.kind == "unresolved_memory_reference" &&
                               observed_memory.contains(item.instruction_address)) ||
                              (item.kind == "unresolved_control_flow" &&
                               observed_indirect.contains(item.instruction_address));
        if (resolved) {
            const auto observed = std::find_if(
                report.dynamic_observed.begin(), report.dynamic_observed.end(),
                [&](const auto& value) {
                    return value.instruction_address == item.instruction_address &&
                           ((item.kind == "unresolved_memory_reference" && value.kind == "memory_read") ||
                            (item.kind == "unresolved_control_flow" && value.kind == "indirect_control_flow"));
                });
            report.newly_resolved.push_back(
                {item.instruction_address, item.block_start, item.kind, observed->observed_value,
                 observed->width_bytes});
        } else {
            report.still_unresolved.push_back(item);
        }
    }
    return report;
}

} // namespace oasis::tools
