#include "tools/re_assemble.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::tools {
namespace {
std::string hex(std::uint32_t value, unsigned digits = 0) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(digits) << value;
    return out.str();
}
std::string operand_text(const DecodedOperand& operand) {
    const auto index = std::to_string(operand.register_index);
    const auto address = "A" + index;
    switch (operand.kind) {
    case OperandKind::data_register: return "D" + index;
    case OperandKind::address_register: return address;
    case OperandKind::indirect: return "(" + address + ")";
    case OperandKind::postincrement: return "(" + address + ")+";
    case OperandKind::predecrement: return "-(" + address + ")";
    case OperandKind::displacement:
        return std::to_string(operand.displacement) + "(" + address + ")";
    case OperandKind::indexed:
    case OperandKind::pc_indexed: {
        const auto index = std::string(operand.index_is_address ? "A" : "D") +
            std::to_string(operand.index_register) + (operand.index_long ? ".L" : ".W");
        const auto base = operand.kind == OperandKind::pc_indexed ? "PC" : address;
        return std::to_string(operand.displacement) + "(" + base + "," + index + ")";
    }
    case OperandKind::absolute_word: return "($" + hex(operand.value, 4) + ").W";
    case OperandKind::absolute_long: return "($" + hex(operand.value, 8) + ").L";
    case OperandKind::pc_displacement:
        return "($" + hex(static_cast<std::uint32_t>(operand.displacement), 4) + ",PC)";
    case OperandKind::immediate: return "#$" + hex(operand.value);
    case OperandKind::status_register: return operand.value ? "SR" : "CCR";
    case OperandKind::register_list: {
        std::string text;
        for (unsigned bit = 0; bit < 16; ++bit) {
            if (!(operand.value & (1U << bit))) continue;
            if (!text.empty()) text += '/';
            text += std::string(bit < 8 ? "D" : "A") + std::to_string(bit % 8);
        }
        if (text.empty()) throw std::invalid_argument("empty MOVEM mask is outside PoC");
        return text;
    }
    default: throw std::invalid_argument("indexed operand is outside exact PoC");
    }
}

DecodedOperand encoding_operand(const DecodedInstruction& instruction,
                                const DecodedOperand& operand) {
    if (operand.kind != OperandKind::immediate || operand.width_bytes != 1 ||
        operand.extension_bytes != 2 || operand.extension_address < instruction.address)
        return operand;
    const auto offset = operand.extension_address - instruction.address;
    if (offset + 1 >= instruction.bytes.size()) return operand;
    auto result = operand;
    result.value = (static_cast<std::uint32_t>(instruction.bytes[offset]) << 8U) |
                   instruction.bytes[offset + 1];
    return result;
}
std::string raw_words(const DecodedInstruction& instruction) {
    std::ostringstream out;
    out << "dc.w";
    for (std::size_t offset = 0; offset < instruction.bytes.size(); offset += 2U)
        out << (offset ? "," : " ") << "$" << hex(
            (static_cast<std::uint16_t>(instruction.bytes[offset]) << 8U) |
            instruction.bytes[offset + 1U], 4);
    return out.str();
}
} // namespace

std::string exact_instruction_asm(const DecodedInstruction& instruction) {
    if (!instruction.supported || !instruction.exact)
        throw std::invalid_argument("no exact IR at 0x" + hex(instruction.address));
    const auto& exact = *instruction.exact;
    if (exact.source && exact.source->kind == OperandKind::immediate &&
        exact.source->width_bytes == 1U && instruction.bytes.size() >= 4U &&
        instruction.bytes[2] == 0xFFU)
        return raw_words(instruction);
    std::string text = exact.operation;
    if (exact.branch_width_bytes) {
        if (exact.operation.substr(0, 2) != "db")
            text += exact.branch_width_bytes == 1 ? ".s" : ".w";
    } else if (exact.width_bytes && exact.operation != "moveq" && exact.operation != "exg") {
        const bool ccr_immediate = exact.destination &&
            exact.destination->kind == OperandKind::status_register &&
            exact.destination->value == 0 && exact.width_bytes == 2 &&
            (exact.operation == "ori" || exact.operation == "andi" ||
             exact.operation == "eori");
        text += ccr_immediate ? ".b" :
            exact.width_bytes == 1 ? ".b" : exact.width_bytes == 2 ? ".w" : ".l";
    }
    if (exact.source) {
        const auto source = encoding_operand(instruction, *exact.source);
        if (exact.operation == "moveq")
            text += " #" + std::to_string(static_cast<std::int32_t>(source.value));
        else text += " " + operand_text(source);
    }
    if (exact.destination) text += (exact.source ? "," : " ") + operand_text(*exact.destination);
    if (exact.branch_width_bytes) {
        if (!instruction.direct_target) throw std::invalid_argument("branch lacks target");
        text += (exact.source ? "," : " ") + std::string("loc_") + hex(*instruction.direct_target, 6);
    }
    return text;
}

std::string slice_asm(const DecodedSlice& slice) {
    std::ostringstream out;
    auto cursor = slice.entry;
    out << "; Local ROM-derived code. Do not commit. vasm -m68000 -no-opt -Fbin\n";
    for (const auto& instruction : slice.instructions) {
        if (instruction.address != cursor) throw std::invalid_argument("slice has gap/overlap");
        cursor += static_cast<std::uint32_t>(instruction.bytes.size());
    }
    if (cursor != slice.range_end) throw std::invalid_argument("incomplete selected slice");
    for (const auto& instruction : slice.instructions) {
        if (instruction.direct_target && *instruction.direct_target >= slice.entry &&
            *instruction.direct_target < slice.range_end &&
            std::none_of(slice.instructions.begin(), slice.instructions.end(),
            [&](const auto& target) { return target.address == *instruction.direct_target; }))
            throw std::invalid_argument("branch into instruction interior");
    }
    out << "    org $" << hex(slice.entry) << "\n";
    out << "sub_" << hex(slice.entry, 6) << ":\n";
    for (const auto& instruction : slice.instructions)
    {
        auto assembly = exact_instruction_asm(instruction);
        if (instruction.direct_target && instruction.exact && instruction.exact->branch_width_bytes &&
            (*instruction.direct_target < slice.entry || *instruction.direct_target >= slice.range_end)) {
            // vasm's '*' denotes the address after the branch opcode for this form.
            const auto delta = static_cast<std::int64_t>(*instruction.direct_target) -
                               static_cast<std::int64_t>(instruction.address);
            const auto target = "loc_" + hex(*instruction.direct_target, 6);
            const auto expression = "*" + std::string(delta < 0 ? "-$" : "+$") +
                                    hex(static_cast<std::uint32_t>(delta < 0 ? -delta : delta));
            const auto position = assembly.find(target);
            if (position != std::string::npos) assembly.replace(position, target.size(), expression);
        }
        out << "loc_" << hex(instruction.address, 6) << ":\n    " << assembly << '\n';
    }
    return out.str();
}

std::optional<ByteDifference> first_byte_difference(
    std::span<const std::uint8_t> rom, std::span<const std::uint8_t> rebuilt,
    std::uint32_t start, std::uint32_t end) {
    if (start >= end || end > rom.size()) throw std::invalid_argument("invalid comparison range");
    const auto original = rom.subspan(start, end - start);
    const auto common = std::min(original.size(), rebuilt.size());
    std::size_t offset = 0;
    while (offset < common && original[offset] == rebuilt[offset]) ++offset;
    if (offset == original.size() && offset == rebuilt.size()) return std::nullopt;
    return ByteDifference{offset, start + static_cast<std::uint32_t>(offset),
        offset < original.size() ? std::optional{original[offset]} : std::nullopt,
        offset < rebuilt.size() ? std::optional{rebuilt[offset]} : std::nullopt};
}

std::string difference_text(const std::optional<ByteDifference>& difference,
                            const DecodedSlice* slice) {
    if (!difference) return "MATCH\n";
    const auto& d = *difference;
    std::string text = "FIRST_DIFFERENCE rom_offset=0x" + hex(d.rom_offset, 6) +
        " slice_offset=0x" + hex(static_cast<std::uint32_t>(d.slice_offset)) +
        " expected=" + (d.expected ? "0x" + hex(*d.expected, 2) : "EOF") +
        " actual=" + (d.actual ? "0x" + hex(*d.actual, 2) : "EOF");
    if (slice) {
        for (const auto& instruction : slice->instructions) {
            if (d.rom_offset >= instruction.address &&
                d.rom_offset < instruction.address + instruction.bytes.size()) {
                text += " instruction=0x" + hex(instruction.address, 6) + " " + exact_instruction_asm(instruction);
                break;
            }
        }
    }
    return text + '\n';
}
} // namespace oasis::tools
