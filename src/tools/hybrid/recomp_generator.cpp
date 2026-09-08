#include "tools/hybrid/recomp_generator.hpp"

#include "tools/re_assemble.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {

using oasis::tools::DecodedInstruction;
using oasis::tools::DecodedOperand;
using oasis::tools::OperandKind;

std::string hex(std::uint32_t value, unsigned digits) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(digits) << value;
    return out.str();
}

const DecodedOperand& operand(const DecodedInstruction& instruction,
                             const std::optional<DecodedOperand>& value,
                             const char* name) {
    if (!value) throw std::invalid_argument(std::string("missing ") + name + " operand at 0x" +
                                            hex(instruction.address, 6));
    return *value;
}

void require_kind(const DecodedInstruction& instruction, const DecodedOperand& value,
                  OperandKind kind, const char* name) {
    if (value.kind != kind)
        throw std::invalid_argument(std::string("unsupported ") + name + " operand at 0x" +
                                    hex(instruction.address, 6));
}

std::string helper_call(const DecodedInstruction& instruction) {
    const auto& exact = *instruction.exact;
    const auto& source = exact.source;
    const auto& destination = exact.destination;
    if (exact.operation.size() >= 2 && exact.operation[0] == 'b' &&
        exact.operation != "bra" && exact.operation != "bsr" && instruction.direct_target &&
        instruction.branch_condition_code) {
        const auto word_branch = instruction.bytes.size() > 2;
        const auto not_taken = word_branch ? 14 : -14;
        std::ostringstream branch;
        branch << "branch_condition(api, " << unsigned(*instruction.branch_condition_code)
               << "U, 0x" << hex(*instruction.direct_target, 6) << "U, " << not_taken;
        if (word_branch) {
            const auto extension = (static_cast<unsigned>(instruction.bytes[2]) << 8U) |
                                    instruction.bytes[3];
            branch << ", 0x" << hex(extension, 4) << "U";
        }
        branch << ");";
        return branch.str();
    }
    if (exact.operation.size() >= 2 && exact.operation[0] == 'd' &&
        exact.operation[1] == 'b' && instruction.direct_target && source) {
        require_kind(instruction, *source, OperandKind::data_register, "DBcc counter");
        std::ostringstream dbcc;
        dbcc << "dbcc(api, " << unsigned(*instruction.branch_condition_code)
             << "U, " << unsigned(source->register_index) << "U, 0x"
             << hex(*instruction.direct_target, 6) << "U, 0x"
             << hex((static_cast<unsigned>(instruction.bytes[2]) << 8U) |
                    instruction.bytes[3], 4) << "U);";
        return dbcc.str();
    }
    if (exact.operation == "tst" && destination && exact.width_bytes != 4) {
        require_kind(instruction, *destination, OperandKind::absolute_long, "TST destination");
        std::ostringstream tst;
        tst << "test_absolute_long(api, 0x" << hex(destination->value, 6) << "U, "
            << unsigned(exact.width_bytes) << "U);";
        return tst.str();
    }
    const auto& dst = operand(instruction, destination, "destination");
    std::ostringstream out;
    if (exact.operation == "movem" && exact.width_bytes == 4 && source) {
        require_kind(instruction, *source, OperandKind::register_list, "MOVEM source");
        require_kind(instruction, dst, OperandKind::predecrement, "MOVEM destination");
        if (dst.register_index != 7) throw std::invalid_argument("MOVEM stack register is not A7");
        out << "movem_l_predecrement(api, 0x" << hex(source->value, 4) << "U);";
        return out.str();
    }
    if (exact.operation == "clr" && exact.width_bytes == 2) {
        require_kind(instruction, dst, OperandKind::data_register, "CLR destination");
        out << "clear_w_data_register(api, " << unsigned(dst.register_index) << "U);";
        return out.str();
    }
    if (exact.operation == "lea" && exact.width_bytes == 4 && source) {
        require_kind(instruction, *source, OperandKind::absolute_long, "LEA source");
        require_kind(instruction, dst, OperandKind::address_register, "LEA destination");
        out << "lea_absolute_long(api, 0x" << hex(source->value, 6) << "U, "
            << unsigned(dst.register_index) << "U);";
        return out.str();
    }
    if (exact.operation == "adda" && exact.width_bytes == 2 && source) {
        require_kind(instruction, *source, OperandKind::data_register, "ADDA source");
        require_kind(instruction, dst, OperandKind::address_register, "ADDA destination");
        out << "adda_w_data_to_address(api, " << unsigned(source->register_index) << "U, "
            << unsigned(dst.register_index) << "U);";
        return out.str();
    }
    if (exact.operation == "add" && exact.width_bytes == 4 && source) {
        require_kind(instruction, *source, OperandKind::data_register, "ADD source");
        require_kind(instruction, dst, OperandKind::data_register, "ADD destination");
        out << "add_l_data_to_data(api, " << unsigned(source->register_index) << "U, "
            << unsigned(dst.register_index) << "U);";
        return out.str();
    }
    if (exact.operation == "move" && source) {
        require_kind(instruction, *source, OperandKind::postincrement, "MOVE source");
        if (exact.width_bytes == 1 && dst.kind == OperandKind::data_register) {
            out << "move_b_postincrement_to_data_register(api, "
                << unsigned(source->register_index) << "U, " << unsigned(dst.register_index) << "U);";
            return out.str();
        }
        if (exact.width_bytes == 2 && dst.kind == OperandKind::postincrement) {
            out << "move_w_postincrement_to_postincrement(api, "
                << unsigned(source->register_index) << "U, " << unsigned(dst.register_index) << "U);";
            return out.str();
        }
    }
    throw std::invalid_argument("unsupported generated instruction at 0x" +
                                hex(instruction.address, 6));
}

void validate_contiguous(const GeneratedBlock& block) {
    if (block.instructions.empty()) throw std::invalid_argument("generated block is empty");
    auto cursor = block.start;
    for (const auto& instruction : block.instructions) {
        if (!instruction.supported || !instruction.exact || instruction.address != cursor)
            throw std::invalid_argument("generated block is unsupported or non-contiguous");
        cursor += static_cast<std::uint32_t>(instruction.bytes.size());
    }
    if (cursor != block.end) throw std::invalid_argument("generated block boundary is incomplete");
}

} // namespace

GeneratedBlock generate_block(std::span<const std::uint8_t> rom,
                              std::uint32_t start, std::uint32_t end) {
    if (start >= end || (start & 1U) || end > rom.size())
        throw std::invalid_argument("invalid generated block range");
    const auto slice = oasis::tools::decode_m68k_slice(
        rom, {.entry = start, .byte_budget = end - start, .instruction_budget = 128});
    if (slice.range_end != end || !slice.unsupported_instruction_addresses.empty() ||
        !slice.unresolved_control_flow.empty())
        throw std::invalid_argument("generated block has unsupported control/data flow");
    GeneratedBlock result{start, end, slice.instructions};
    validate_contiguous(result);
    for (const auto& instruction : result.instructions) (void)helper_call(instruction);
    return result;
}

std::string emit_translation_unit(const std::vector<GeneratedBlock>& blocks) {
    if (blocks.empty()) throw std::invalid_argument("no blocks to generate");
    std::ostringstream out;
    out << "// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.\n"
        << "#include \"tools/hybrid/generated_block_runtime.hpp\"\n"
        << "#include \"tools/hybrid/generated_blocks.hpp\"\n\n"
        << "namespace oasis::hybrid::generated {\n\n";
    for (const auto& block : blocks) {
        validate_contiguous(block);
        out << "unsigned instruction_count_from_0x" << hex(block.start, 6)
            << "(unsigned entry_pc) {\n";
        for (std::size_t i = 0; i < block.instructions.size(); ++i)
            out << "    if (entry_pc == 0x" << hex(block.instructions[i].address, 6)
                << "U) return " << (block.instructions.size() - i) << "U;\n";
        out << "    return 0;\n}\n\n";
        out << "BlockExit execute_0x" << hex(block.start, 6)
            << "(BasicBlockApi& api, unsigned entry_pc) {\n"
            << "    unsigned instructions_executed = 0;\n"
            << "    bool execute_from_here = false;\n"
            << "    if (entry_pc != ";
        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            if (i) out << " && entry_pc != ";
            out << "0x" << hex(block.instructions[i].address, 6) << "U";
        }
        out << ") return {entry_pc, BlockExitReason::FALLBACK, 0};\n";
        for (const auto& instruction : block.instructions) {
            out << "    if (execute_from_here || entry_pc == 0x" << hex(instruction.address, 6) << "U) {\n"
                << "    // guest 0x" << hex(instruction.address, 6) << " opcode 0x"
                << hex(instruction.opcode, 4) << " ";
            for (std::size_t i = 0; i < instruction.bytes.size(); i += 2) {
                if (i) out << ' ';
                out << hex((static_cast<std::uint32_t>(instruction.bytes[i]) << 8U) |
                               instruction.bytes[i + 1U], 4);
            }
            out << " " << oasis::tools::exact_instruction_asm(instruction) << "\n"
                << "    const auto opcode_0x" << hex(instruction.address, 6)
                << " = fetch_checked(api, 0x" << hex(instruction.opcode, 4) << "U);\n"
                << "    api.begin_instruction(opcode_0x" << hex(instruction.address, 6) << ");\n";
            const auto conditional_extension = instruction.bytes.size() > 2 &&
                ((instruction.exact->operation.size() >= 2 && instruction.exact->operation[0] == 'b') ||
                 (instruction.exact->operation.size() >= 2 && instruction.exact->operation[0] == 'd' &&
                  instruction.exact->operation[1] == 'b'));
            if (!conditional_extension) {
                for (std::size_t i = 2; i < instruction.bytes.size(); i += 2)
                    out << "    (void)fetch_checked(api, 0x"
                        << hex((static_cast<std::uint32_t>(instruction.bytes[i]) << 8U) |
                                   instruction.bytes[i + 1U], 4) << "U);\n";
            }
            out << "    " << helper_call(instruction) << "\n"
                << "    api.finish_instruction(opcode_0x" << hex(instruction.address, 6) << ");\n";
            if (instruction.exact->operation == "movem")
                out << "    api.add_cycles(112);\n    api.skip_bus_refresh();\n";
            out << "    ++instructions_executed;\n"
                << "    execute_from_here = true;\n"
                << "    if (api.boundary_reason) {\n"
                << "        const auto reason = api.boundary_reason();\n"
                << "        if (reason != BlockExitReason::CONTINUE_BLOCK)\n"
                << "            return {api.reg(16), reason, instructions_executed};\n"
                << "    }\n"
                << "    }\n";
        }
        out << "    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};\n"
            << "}\n\n";
    }
    out << "} // namespace oasis::hybrid::generated\n";
    return out.str();
}

std::string emit_registry_translation_unit(const std::vector<GeneratedBlock>& blocks) {
    if (blocks.empty()) throw std::invalid_argument("no blocks to generate");
    std::ostringstream out;
    out << "// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.\n"
        << "#include \"tools/hybrid/generated_blocks.hpp\"\n\n"
        << "namespace oasis::hybrid::generated {\n\n"
        << "const GeneratedBlockSpec kBlocks[] = {\n";
    for (const auto& block : blocks) {
        validate_contiguous(block);
        out << "    {0x" << hex(block.start, 6) << "U, 0x" << hex(block.end, 6)
            << "U, " << block.instructions.size() << "U, execute_0x"
            << hex(block.start, 6) << ", instruction_count_from_0x"
            << hex(block.start, 6) << "},\n";
    }
    out << "};\n\nstd::span<const GeneratedBlockSpec> blocks() { return kBlocks; }\n\n"
        << "} // namespace oasis::hybrid::generated\n";
    return out.str();
}

} // namespace oasis::hybrid
