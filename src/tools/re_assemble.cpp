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
    case OperandKind::absolute_word: return "($" + hex(operand.value, 4) + ").W";
    case OperandKind::absolute_long: return "($" + hex(operand.value, 8) + ").L";
    case OperandKind::pc_displacement:
        return "($" + hex(operand.extension_address + operand.displacement) + ",PC)";
    case OperandKind::immediate: return "#$" + hex(operand.value);
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
} // namespace

std::string exact_instruction_asm(const DecodedInstruction& instruction) {
    if (!instruction.supported || !instruction.exact)
        throw std::invalid_argument("no exact IR at 0x" + hex(instruction.address));
    const auto& exact = *instruction.exact;
    std::string text = exact.operation;
    if (exact.branch_width_bytes) {
        if (exact.operation.substr(0, 2) != "db")
            text += exact.branch_width_bytes == 1 ? ".s" : ".w";
    } else if (exact.width_bytes && exact.operation != "moveq")
        text += exact.width_bytes == 1 ? ".b" : exact.width_bytes == 2 ? ".w" : ".l";
    if (exact.source) {
        if (exact.operation == "moveq")
            text += " #" + std::to_string(static_cast<std::int32_t>(exact.source->value));
        else text += " " + operand_text(*exact.source);
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
        if (instruction.direct_target && (*instruction.direct_target < slice.entry ||
                                         *instruction.direct_target >= slice.range_end))
            throw std::invalid_argument("external flow is outside selected PoC");
    }
    if (cursor != slice.range_end) throw std::invalid_argument("incomplete selected slice");
    for (const auto& instruction : slice.instructions) {
        if (instruction.direct_target && std::none_of(slice.instructions.begin(), slice.instructions.end(),
            [&](const auto& target) { return target.address == *instruction.direct_target; }))
            throw std::invalid_argument("branch into instruction interior");
    }
    out << "    org $" << hex(slice.entry) << "\nsub_" << hex(slice.entry, 6) << ":\n";
    for (const auto& instruction : slice.instructions)
        out << "loc_" << hex(instruction.address, 6) << ":\n    "
            << exact_instruction_asm(instruction) << '\n';
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
