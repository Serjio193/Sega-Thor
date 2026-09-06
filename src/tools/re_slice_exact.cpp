#include "tools/re_slice_decoder.hpp"

#include <array>

namespace oasis::tools {
namespace {
DecodedOperand reg(unsigned index, bool address = false) {
    DecodedOperand operand{};
    operand.kind = address ? OperandKind::address_register : OperandKind::data_register;
    operand.register_index = static_cast<std::uint8_t>(index);
    return operand;
}
DecodedOperand immediate(std::uint32_t value, std::uint8_t width) {
    DecodedOperand operand{};
    operand.kind = OperandKind::immediate;
    operand.value = value;
    operand.width_bytes = width;
    return operand;
}
constexpr std::array conditions{"t", "f", "hi", "ls", "cc", "cs", "ne", "eq",
                                "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"};
} // namespace

void normalize_exact_instruction(DecodedInstruction& instruction) {
    instruction.exact.reset();
    if (!instruction.supported) return;
    const auto op = instruction.opcode;
    const auto& ea = instruction.effective_operands;
    const auto& constants = instruction.immediate_constants;
    const auto& family = instruction.mnemonic;
    ExactInstruction result{};
    const auto size = static_cast<unsigned>((op >> 6U) & 3U);
    const auto width = static_cast<std::uint8_t>(1U << (size & 3U));
    const auto dn = reg((op >> 9U) & 7U);
    if (family == "rts" || family == "nop") {
        result.operation = family;
    } else if (family == "move" && ea.size() == 2U) {
        result.operation = ea[1].kind == OperandKind::address_register ? "movea" : "move";
        result.width_bytes = ea[0].width_bytes;
        result.source = ea[0];
        result.destination = ea[1];
    } else if (family == "moveq" && constants.size() == 1U) {
        result.operation = "moveq";
        result.width_bytes = 4;
        result.source = immediate(constants[0].value, 1);
        result.destination = dn;
    } else if (family == "bra" || family == "bsr" || family == "bcc") {
        const auto condition = *instruction.branch_condition_code;
        result.operation = condition < 2 ? family : "b" + std::string(conditions[condition]);
        result.branch_width_bytes = instruction.bytes.size() == 2 ? 1 : 2;
    } else if (family == "dbcc") {
        result.operation = "db" + std::string(conditions[*instruction.branch_condition_code]);
        result.width_bytes = 2;
        result.source = reg(op & 7U);
        result.branch_width_bytes = 2;
    } else if (family == "scc" && ea.size() == 1U) {
        constexpr std::array names{"st", "sf", "shi", "sls", "scc", "scs", "sne", "seq",
                                   "svc", "svs", "spl", "smi", "sge", "slt", "sgt", "sle"};
        result.operation = names[(op >> 8U) & 0x0FU];
        result.width_bytes = 1;
        result.destination = ea[0];
    } else if (family == "lea" && ea.size() == 1U) {
        result.operation = "lea";
        result.width_bytes = 4;
        result.source = ea[0];
        result.destination = reg((op >> 9U) & 7U, true);
    } else if (family == "pea" && ea.size() == 1U) {
        result.operation = "pea";
        result.width_bytes = 4;
        result.source = ea[0];
    } else if ((family == "jsr" || family == "jmp") && ea.size() == 1U) {
        result.operation = family;
        result.width_bytes = 4;
        result.source = ea[0];
    } else if (family == "swap") {
        result.operation = "swap";
        result.width_bytes = 2;
        result.destination = reg(op & 7U);
    } else if (family == "ext") {
        result.operation = (op & 0x40U) ? "ext.l" : "ext.w";
        result.destination = reg(op & 7U);
    } else if ((family == "addq" || family == "subq") && ea.size() == 1U) {
        result.operation = family;
        result.width_bytes = width;
        result.source = immediate(constants[0].value, 1);
        result.destination = ea[0];
    } else if (family == "immediate" && ea.size() == 1U && constants.size() == 1U) {
        constexpr std::array names{"ori", "andi", "subi", "addi", "", "eori", "cmpi"};
        const auto group = (op >> 9U) & 7U;
        if (group >= names.size() || names[group][0] == '\0') return;
        result.operation = names[group];
        result.width_bytes = width;
        result.source = immediate(constants[0].value, constants[0].width_bytes);
        result.source->extension_bytes = width == 4 ? 4 : 2;
        result.source->extension_address = instruction.address + 2;
        result.destination = ea[0];
    } else if (family == "immediate" && ea.empty() && constants.size() == 1U &&
               (op & 0x3FU) == 0x3CU) {
        constexpr std::array names{"ori", "andi", "subi", "addi", "", "eori", "cmpi"};
        const auto group = (op >> 9U) & 7U;
        if (group >= names.size() || names[group][0] == '\0') return;
        result.operation = names[group];
        result.width_bytes = 2;
        result.source = immediate(constants[0].value, 2);
        result.source->extension_bytes = 2;
        result.source->extension_address = instruction.address + 2;
        result.destination = DecodedOperand{};
        result.destination->kind = OperandKind::status_register;
        result.destination->value = (op & 0x40U) ? 1U : 0U;
    } else if (family == "unary" && ea.size() == 1U) {
        if ((op & 0xFF00U) == 0x4000U) result.operation = "negx";
        else if ((op & 0xFF00U) == 0x4200U) result.operation = "clr";
        else if ((op & 0xFF00U) == 0x4400U) result.operation = "neg";
        else if ((op & 0xFF00U) == 0x4600U) result.operation = "not";
        else if ((op & 0xFF00U) == 0x4A00U) result.operation = "tst";
        else return;
        result.width_bytes = width;
        result.destination = ea[0];
    } else if (family == "static_bit" && ea.size() == 1U) {
        constexpr std::array names{"btst", "bchg", "bclr", "bset"};
        result.operation = names[(op >> 6U) & 3U];
        result.width_bytes = ea[0].kind == OperandKind::data_register ? 4 : 1;
        result.source = immediate(constants[0].value, 1);
        result.source->extension_bytes = 2;
        result.source->extension_address = instruction.address + 2;
        result.destination = ea[0];
    } else if (family == "binary" && ea.size() == 1U) {
        const auto group = op >> 12U;
        const auto mode = (op >> 6U) & 7U;
        if (mode == 3U || mode == 7U) {
            if (group == 9U) result.operation = "suba";
            else if (group == 0xBU) result.operation = "cmpa";
            else if (group == 0xDU) result.operation = "adda";
            else return;
            result.width_bytes = mode == 3U ? 2 : 4;
            result.source = ea[0];
            result.destination = reg((op >> 9U) & 7U, true);
        } else {
            if (group == 8U) result.operation = "or";
            else if (group == 9U) result.operation = "sub";
            else if (group == 0xBU) result.operation = "cmp";
            else if (group == 0xCU) result.operation = (op & 0x0100U) ? "exg" : "and";
            else if (group == 0xDU) result.operation = "add";
            else return;
            result.width_bytes = width;
            result.source = mode < 4U ? ea[0] : dn;
            result.destination = mode < 4U ? dn : ea[0];
        }
    } else if (family == "shift_or_rotate" && size != 3U) {
        constexpr std::array names{"as", "ls", "rox", "ro"};
        result.operation = std::string(names[(op >> 3U) & 3U]) + ((op & 0x100U) ? "l" : "r");
        result.width_bytes = width;
        const auto count = (op >> 9U) & 7U;
        result.source = (op & 0x20U) ? reg(count) : immediate(count ? count : 8U, 1);
        result.destination = reg(op & 7U);
    } else if (family == "movem" && ea.size() == 1U && constants.size() == 1U) {
        auto mask = constants[0].value;
        if (ea[0].kind == OperandKind::predecrement) {
            std::uint32_t reversed = 0;
            for (unsigned bit = 0; bit < 16; ++bit) reversed |= ((mask >> bit) & 1U) << (15U - bit);
            mask = reversed;
        }
        auto registers = immediate(mask, 2);
        registers.kind = OperandKind::register_list;
        registers.extension_bytes = 2;
        registers.extension_address = instruction.address + 2;
        result.operation = "movem";
        result.width_bytes = (op & 0x40U) ? 4 : 2;
        result.source = (op & 0x400U) ? ea[0] : registers;
        result.destination = (op & 0x400U) ? registers : ea[0];
    } else return;
    for (auto* operand : {&result.source, &result.destination}) {
        if (*operand && (*operand)->kind != OperandKind::immediate &&
            (*operand)->kind != OperandKind::register_list)
            (*operand)->width_bytes = result.width_bytes;
    }
    instruction.exact = result;
}
} // namespace oasis::tools
