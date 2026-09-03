#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

using Bytes = std::span<const std::uint8_t>;

std::uint16_t read16(Bytes bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
}

std::uint32_t read32(Bytes bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(read16(bytes, offset)) << 16U) |
           read16(bytes, offset + 2U);
}

std::size_t size_bytes(unsigned size_code) {
    return size_code == 0U ? 1U : size_code == 1U ? 2U : 4U;
}

std::size_t ea_extension_bytes(unsigned mode, unsigned reg,
                               std::size_t data_width) {
    if (mode == 5U || mode == 6U || (mode == 7U && (reg == 0U || reg == 2U || reg == 3U))) {
        return 2U;
    }
    if (mode == 7U && reg == 1U) return 4U;
    if (mode == 7U && reg == 4U) return data_width <= 2U ? 2U : 4U;
    return 0U;
}

MemoryKind memory_kind(std::uint32_t address) {
    if (address < 0x00400000U) return MemoryKind::rom;
    if (address >= 0x00FF0000U && address <= 0x00FFFFFFU) return MemoryKind::ram;
    return MemoryKind::other;
}

void add_immediate(DecodedInstruction& instruction, std::uint32_t value,
                   std::size_t width) {
    instruction.immediate_constants.push_back(
        {value, static_cast<std::uint8_t>(width)});
}

void add_memory_reference(DecodedInstruction& instruction, std::uint32_t address,
                          std::size_t width, MemoryAccess access) {
    instruction.memory_references.push_back(
        {address, static_cast<std::uint8_t>(width), memory_kind(address), access});
}

void add_unresolved_memory_reference(DecodedInstruction& instruction, unsigned mode,
                                    unsigned reg, const char* reason) {
    instruction.unresolved_memory_references.push_back(
        {static_cast<std::uint8_t>(mode), static_cast<std::uint8_t>(reg), reason});
}

void add_unsupported_addressing(DecodedInstruction& instruction, unsigned mode,
                                unsigned reg, const char* reason) {
    instruction.unsupported_addressing.push_back(
        {static_cast<std::uint8_t>(mode), static_cast<std::uint8_t>(reg), reason});
}

std::string addressing_mode_name(unsigned mode, unsigned reg) {
    if (mode == 0U) return "data_register";
    if (mode == 1U) return "address_register";
    if (mode == 2U) return "address_indirect";
    if (mode == 3U) return "address_postincrement";
    if (mode == 4U) return "address_predecrement";
    if (mode == 5U) return "address_displacement";
    if (mode == 6U) return "address_indexed";
    if (mode == 7U && reg == 0U) return "absolute_word";
    if (mode == 7U && reg == 1U) return "absolute_long";
    if (mode == 7U && reg == 2U) return "pc_displacement";
    if (mode == 7U && reg == 3U) return "pc_indexed";
    if (mode == 7U && reg == 4U) return "immediate";
    return "invalid_mode_7_register";
}

std::size_t parse_ea(Bytes rom, std::uint32_t pc, std::uint32_t range_end,
                     unsigned mode, unsigned reg, std::size_t data_width,
                     MemoryAccess access, DecodedInstruction& instruction,
                     std::size_t extension_offset) {
    instruction.addressing_modes.push_back(addressing_mode_name(mode, reg));
    const auto extension_size = ea_extension_bytes(mode, reg, data_width);
    if (mode == 7U && reg >= 5U) {
        add_unsupported_addressing(instruction, mode, reg, "invalid_mode_7_register");
        return extension_size;
    }
    if (mode == 7U && reg == 4U && extension_offset + extension_size <= rom.size() &&
        extension_offset + extension_size <= range_end) {
        if (data_width <= 2U) {
            const auto word = read16(rom, extension_offset);
            add_immediate(instruction, data_width == 1U ? word & 0xFFU : word,
                          data_width);
        } else {
            add_immediate(instruction, read32(rom, extension_offset), data_width);
        }
        if (access == MemoryAccess::write) {
            add_unsupported_addressing(instruction, mode, reg, "immediate_destination");
        }
    }
    if (mode >= 2U && mode <= 6U) {
        add_unresolved_memory_reference(instruction, mode, reg, "register_based");
    } else if (mode == 7U && reg == 3U) {
        add_unresolved_memory_reference(instruction, mode, reg, "pc_indexed");
    }
    if (mode != 7U || (reg != 0U && reg != 1U && reg != 2U)) {
        return extension_size;
    }
    if (extension_offset + extension_size > rom.size() ||
        extension_offset + extension_size > range_end) {
        return extension_size;
    }
    if (reg == 1U) {
        add_memory_reference(instruction, read32(rom, extension_offset), data_width, access);
    } else if (reg == 0U) {
        const auto address = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(read16(rom, extension_offset))) &
            0x00FFFFFF);
        add_memory_reference(instruction, address, data_width, access);
    } else {
        const auto displacement = static_cast<std::int32_t>(
            static_cast<std::int16_t>(read16(rom, extension_offset)));
        add_memory_reference(instruction,
                             static_cast<std::uint32_t>(pc + 2U + displacement),
                             data_width, access);
    }
    return extension_size;
}

bool is_immediate_group(std::uint16_t opcode) {
    const auto group = static_cast<std::uint16_t>(opcode & 0xFF00U);
    return group == 0x0000U || group == 0x0200U || group == 0x0400U ||
           group == 0x0600U || group == 0x0A00U || group == 0x0C00U;
}

bool is_no_extension_binary(std::uint16_t opcode) {
    if ((opcode & 0xF130U) == 0x8100U || (opcode & 0xF130U) == 0x9100U ||
        (opcode & 0xF130U) == 0xD100U) {
        return true;
    }
    if ((opcode & 0xF1F8U) == 0xC100U || (opcode & 0xF138U) == 0xC140U) {
        return true;
    }
    return false;
}

DecodedInstruction decode_one(Bytes rom, std::uint32_t pc,
                              std::uint32_t range_end) {
    DecodedInstruction instruction{};
    instruction.address = pc;
    if (pc + 2U > rom.size() || pc + 2U > range_end) {
        instruction.mnemonic = "truncated";
        instruction.supported = false;
        instruction.bytes.assign(rom.begin() + pc, rom.begin() + std::min<std::size_t>(rom.size(), range_end));
        return instruction;
    }
    instruction.opcode = read16(rom, pc);
    const auto opcode = instruction.opcode;
    std::size_t length = 2U;
    bool recognized = true;
    auto set_length = [&](std::size_t value) { length = value; };
    auto parse_single = [&](unsigned mode, unsigned reg, std::size_t width,
                            MemoryAccess access, std::size_t offset = 2U) {
        set_length(offset + parse_ea(rom, pc, range_end, mode, reg, width, access,
                                     instruction, pc + offset));
    };

    const auto branch_group = static_cast<unsigned>(opcode >> 12U);
    if (branch_group == 6U) {
        const auto condition = static_cast<unsigned>((opcode >> 8U) & 0x0FU);
        instruction.branch_condition_code = static_cast<std::uint8_t>(condition);
        const auto displacement_byte = static_cast<std::uint8_t>(opcode & 0xFFU);
        std::int32_t displacement = static_cast<std::int8_t>(displacement_byte);
        if (displacement_byte == 0U) {
            if (pc + 4U > rom.size() || pc + 4U > range_end) recognized = false;
            else {
                displacement = static_cast<std::int16_t>(read16(rom, pc + 2U));
                set_length(4U);
            }
        }
        instruction.mnemonic = condition == 0U ? "bra" : condition == 1U ? "bsr" : "bcc";
        instruction.flow = condition == 1U ? FlowKind::direct_call : FlowKind::direct_branch;
        instruction.direct_target = static_cast<std::uint32_t>(pc + 2U + displacement);
    } else if ((opcode & 0xFFF0U) == 0x4E40U) {
        instruction.mnemonic = "trap";
        add_immediate(instruction, opcode & 0x0FU, 1U);
    } else if (opcode == 0x4E75U || opcode == 0x4E73U || opcode == 0x4E77U) {
        instruction.mnemonic = opcode == 0x4E75U ? "rts" : opcode == 0x4E73U ? "rte" : "rtr";
        instruction.flow = FlowKind::return_instruction;
    } else if (opcode == 0x4E71U || opcode == 0x4E70U || opcode == 0x4E76U) {
        instruction.mnemonic = opcode == 0x4E71U ? "nop" : opcode == 0x4E70U ? "reset" : "trapv";
    } else if (opcode == 0x4E72U || opcode == 0x4E74U) {
        instruction.mnemonic = opcode == 0x4E72U ? "stop" : "rtd";
        set_length(4U);
        if (pc + 4U <= rom.size() && pc + 4U <= range_end) add_immediate(instruction, read16(rom, pc + 2U), 2U);
    } else if ((opcode & 0xFFF8U) == 0x4E50U) {
        instruction.mnemonic = "link";
        set_length(4U);
        if (pc + 4U <= rom.size() && pc + 4U <= range_end) add_immediate(instruction, read16(rom, pc + 2U), 2U);
    } else if ((opcode & 0xFFF8U) == 0x4E58U) {
        instruction.mnemonic = "unlk";
    } else if ((opcode & 0xFFF0U) == 0x4E60U) {
        instruction.mnemonic = "move_usp";
    } else if ((opcode & 0xFFC0U) == 0x4E80U || (opcode & 0xFFC0U) == 0x4EC0U) {
        const bool is_call = (opcode & 0xFFC0U) == 0x4E80U;
        const auto mode = static_cast<unsigned>((opcode >> 3U) & 7U);
        const auto reg = static_cast<unsigned>(opcode & 7U);
        instruction.mnemonic = is_call ? "jsr" : "jmp";
        if (mode == 7U && reg == 1U && pc + 6U <= rom.size() && pc + 6U <= range_end) {
            set_length(6U);
            instruction.direct_target = read32(rom, pc + 2U);
            instruction.flow = is_call ? FlowKind::direct_call : FlowKind::direct_jump;
            add_memory_reference(instruction, *instruction.direct_target, 0U, MemoryAccess::address);
        } else {
            instruction.flow = is_call ? FlowKind::indirect_call : FlowKind::indirect_jump;
            parse_single(mode, reg, 4U, MemoryAccess::address);
        }
    } else if ((opcode & 0xF100U) == 0x7000U) {
        instruction.mnemonic = "moveq";
        const auto value = static_cast<std::int32_t>(static_cast<std::int8_t>(opcode & 0xFFU));
        add_immediate(instruction, static_cast<std::uint32_t>(value), 1U);
    } else if ((opcode >> 12U) == 1U || (opcode >> 12U) == 2U || (opcode >> 12U) == 3U) {
        const auto source_mode = static_cast<unsigned>((opcode >> 3U) & 7U);
        const auto source_reg = static_cast<unsigned>(opcode & 7U);
        const auto destination_mode = static_cast<unsigned>((opcode >> 6U) & 7U);
        const auto destination_reg = static_cast<unsigned>((opcode >> 9U) & 7U);
        const auto width = (opcode >> 12U) == 1U ? 1U : (opcode >> 12U) == 3U ? 2U : 4U;
        instruction.mnemonic = "move";
        const auto source_size = parse_ea(rom, pc, range_end, source_mode, source_reg, width,
                                          MemoryAccess::read, instruction, pc + 2U);
        const auto destination_offset = pc + 2U + source_size;
        const auto destination_size = parse_ea(rom, pc, range_end, destination_mode, destination_reg,
                                               width, MemoryAccess::write, instruction,
                                               destination_offset);
        set_length(2U + source_size + destination_size);
    } else if (is_immediate_group(opcode)) {
        const auto size_code = static_cast<unsigned>((opcode >> 6U) & 3U);
        const auto group_low = static_cast<unsigned>(opcode & 0x3FU);
        instruction.mnemonic = "immediate";
        if (group_low == 0x3CU) {
            set_length(4U);
            if (pc + 4U <= rom.size() && pc + 4U <= range_end) {
                const auto width = size_code == 0U ? 1U : 2U;
                add_immediate(instruction, size_code == 0U ? rom[pc + 3U] : read16(rom, pc + 2U), width);
            }
        } else if (size_code == 3U) {
            recognized = false;
        } else {
            const auto width = size_bytes(size_code);
            if (pc + 2U + width > rom.size() || pc + 2U + width > range_end) recognized = false;
            else {
                add_immediate(instruction, width == 1U ? rom[pc + 3U] : width == 2U ? read16(rom, pc + 2U) : read32(rom, pc + 2U), width);
                const auto mode = static_cast<unsigned>((opcode >> 3U) & 7U);
                const auto reg = static_cast<unsigned>(opcode & 7U);
                const auto immediate_extension = width == 4U ? 4U : 2U;
                parse_single(mode, reg, width, MemoryAccess::unknown,
                             immediate_extension + 2U);
            }
        }
    } else if ((opcode & 0xFF00U) == 0x0800U) {
        instruction.mnemonic = "static_bit";
        if (pc + 4U > rom.size() || pc + 4U > range_end) recognized = false;
        else {
            add_immediate(instruction, rom[pc + 3U], 1U);
            const auto mode = static_cast<unsigned>((opcode >> 3U) & 7U);
            const auto reg = static_cast<unsigned>(opcode & 7U);
            parse_single(mode, reg, 1U, MemoryAccess::unknown, 4U);
        }
    } else if ((opcode & 0xF0F8U) == 0x50C8U) {
        instruction.mnemonic = "dbcc";
        instruction.branch_condition_code = static_cast<std::uint8_t>((opcode >> 8U) & 0x0FU);
        set_length(4U);
        if (pc + 4U <= rom.size() && pc + 4U <= range_end) {
            const auto displacement = static_cast<std::int16_t>(read16(rom, pc + 2U));
            instruction.flow = FlowKind::direct_branch;
            instruction.direct_target = static_cast<std::uint32_t>(pc + 2U + displacement);
        } else recognized = false;
    } else if ((opcode & 0xF0C0U) == 0x50C0U) {
        instruction.mnemonic = "scc";
        const auto mode = static_cast<unsigned>((opcode >> 3U) & 7U);
        parse_single(mode, static_cast<unsigned>(opcode & 7U), 1U, MemoryAccess::write);
    } else if ((opcode & 0xF000U) == 0x5000U) {
        instruction.mnemonic = (opcode & 0x0100U) != 0U ? "subq" : "addq";
        add_immediate(instruction, ((opcode >> 9U) & 7U) == 0U ? 8U : (opcode >> 9U) & 7U, 1U);
        const auto size_code = static_cast<unsigned>((opcode >> 6U) & 3U);
        if (size_code == 3U) recognized = false;
        else parse_single(static_cast<unsigned>((opcode >> 3U) & 7U), opcode & 7U,
                          size_bytes(size_code), MemoryAccess::unknown);
    } else if ((opcode & 0xF1C0U) == 0x41C0U) {
        instruction.mnemonic = "lea";
        parse_single((opcode >> 3U) & 7U, opcode & 7U, 0U, MemoryAccess::address);
    } else if ((opcode & 0xFFC0U) == 0x4840U) {
        instruction.mnemonic = "pea";
        parse_single((opcode >> 3U) & 7U, opcode & 7U, 4U, MemoryAccess::read);
    } else if ((opcode & 0xFB80U) == 0x4880U) {
        instruction.mnemonic = "movem";
        set_length(4U + parse_ea(rom, pc, range_end, (opcode >> 3U) & 7U, opcode & 7U,
                                 4U, MemoryAccess::unknown, instruction, pc + 4U));
        if (pc + 4U <= rom.size() && pc + 4U <= range_end) add_immediate(instruction, read16(rom, pc + 2U), 2U);
    } else if ((opcode & 0xFFF8U) == 0x4840U || (opcode & 0xFFF8U) == 0x4880U ||
               (opcode & 0xFFF8U) == 0x48C0U) {
        instruction.mnemonic = (opcode & 0xFFF8U) == 0x4840U ? "swap" : "ext";
    } else if ((opcode & 0xFFC0U) == 0x40C0U || (opcode & 0xFFC0U) == 0x44C0U ||
               (opcode & 0xFFC0U) == 0x46C0U) {
        instruction.mnemonic = "move_status";
        parse_single((opcode >> 3U) & 7U, opcode & 7U, 2U, MemoryAccess::unknown);
    } else if ((opcode & 0xFF00U) == 0x4000U || (opcode & 0xFF00U) == 0x4200U ||
               (opcode & 0xFF00U) == 0x4400U || (opcode & 0xFF00U) == 0x4600U ||
               (opcode & 0xFF00U) == 0x4A00U) {
        const auto size_code = static_cast<unsigned>((opcode >> 6U) & 3U);
        if (size_code == 3U) recognized = false;
        else {
            instruction.mnemonic = "unary";
            parse_single((opcode >> 3U) & 7U, opcode & 7U, size_bytes(size_code),
                         MemoryAccess::unknown);
        }
    } else if ((opcode >> 12U) == 8U || (opcode >> 12U) == 9U ||
               (opcode >> 12U) == 0xBU || (opcode >> 12U) == 0xCU ||
               (opcode >> 12U) == 0xDU) {
        instruction.mnemonic = "binary";
        if (is_no_extension_binary(opcode)) {
            set_length(2U);
        } else {
            const auto operation_mode = static_cast<unsigned>((opcode >> 6U) & 7U);
            const auto width = operation_mode == 3U || operation_mode == 7U
                                   ? 2U
                                   : size_bytes(operation_mode & 3U);
            parse_single((opcode >> 3U) & 7U, opcode & 7U, width, MemoryAccess::unknown);
        }
    } else if ((opcode >> 12U) == 0xEU) {
        instruction.mnemonic = "shift_or_rotate";
        if ((opcode & 0xC0U) == 0xC0U) {
            parse_single((opcode >> 3U) & 7U, opcode & 7U, 2U, MemoryAccess::unknown);
        }
    } else {
        instruction.mnemonic = "unsupported";
        recognized = false;
    }

    const auto available_end = std::min<std::size_t>(rom.size(), range_end);
    if (!instruction.unsupported_addressing.empty()) recognized = false;
    if (pc + length > available_end || length < 2U || (length & 1U) != 0U) {
        recognized = false;
        length = std::min<std::size_t>(2U, available_end - pc);
    }
    instruction.bytes.assign(rom.begin() + pc, rom.begin() + pc + length);
    instruction.supported = recognized;
    if (!recognized && instruction.mnemonic != "unsupported") instruction.mnemonic = "unsupported";
    if (!recognized) instruction.flow = FlowKind::unsupported;
    return instruction;
}

bool in_range(std::uint32_t address, std::uint32_t start, std::uint32_t end) {
    return address >= start && address < end && (address & 1U) == 0U;
}

} // namespace

DecodedSlice decode_m68k_slice(Bytes rom, const DecodeOptions& options) {
    if (options.entry >= rom.size() || (options.entry & 1U) != 0U || options.byte_budget < 2U) {
        throw std::invalid_argument("invalid bounded m68k slice options");
    }
    const auto range_end = static_cast<std::uint32_t>(std::min<std::size_t>(
        rom.size(), static_cast<std::size_t>(options.entry) + options.byte_budget));
    std::map<std::uint32_t, DecodedInstruction> decoded;
    std::set<std::uint32_t> pending{options.entry};
    DecodedSlice slice{.entry = options.entry, .range_end = range_end};
    while (!pending.empty() && decoded.size() < options.instruction_budget) {
        const auto pc = *pending.begin();
        pending.erase(pending.begin());
        if (!in_range(pc, options.entry, range_end) || decoded.contains(pc)) continue;
        auto instruction = decode_one(rom, pc, range_end);
        if (instruction.bytes.empty()) continue;
        decoded.emplace(pc, std::move(instruction));
        const auto& current = decoded.at(pc);
        const auto next = pc + current.bytes.size();
        if (current.direct_target) {
            slice.control_flow.push_back({pc, *current.direct_target, current.flow});
            if (current.flow == FlowKind::direct_call) pending.insert(next);
            else if (current.flow == FlowKind::direct_branch && current.mnemonic != "bra") pending.insert(next);
            pending.insert(*current.direct_target);
        } else if (current.flow == FlowKind::indirect_call) {
            slice.unresolved_control_flow.push_back({pc, current.opcode, current.flow});
            pending.insert(next);
        } else if (current.flow == FlowKind::indirect_jump) {
            slice.unresolved_control_flow.push_back({pc, current.opcode, current.flow});
        } else if (current.flow != FlowKind::return_instruction && current.flow != FlowKind::unsupported) {
            pending.insert(next);
        }
    }
    for (auto& [address, instruction] : decoded) {
        slice.instructions.push_back(std::move(instruction));
        if (!slice.instructions.back().supported) slice.unsupported_instruction_addresses.push_back(address);
    }
    std::sort(slice.control_flow.begin(), slice.control_flow.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source, left.target, left.kind) < std::tie(right.source, right.target, right.kind);
    });
    std::sort(slice.unresolved_control_flow.begin(), slice.unresolved_control_flow.end(),
              [](const auto& left, const auto& right) { return left.address < right.address; });

    std::set<std::uint32_t> leaders{options.entry};
    for (const auto& instruction : slice.instructions) {
        if (instruction.direct_target && in_range(*instruction.direct_target, options.entry, range_end)) {
            leaders.insert(*instruction.direct_target);
        }
        const auto next = instruction.address + instruction.bytes.size();
        if (instruction.flow == FlowKind::direct_branch && instruction.mnemonic != "bra" &&
            in_range(next, options.entry, range_end)) leaders.insert(next);
        if (instruction.flow == FlowKind::direct_call &&
            in_range(*instruction.direct_target, options.entry, range_end)) leaders.insert(*instruction.direct_target);
    }
    std::set<std::uint32_t> instruction_addresses;
    for (const auto& instruction : slice.instructions) instruction_addresses.insert(instruction.address);
    for (const auto leader : leaders) {
        if (!instruction_addresses.contains(leader)) continue;
        BasicBlock block{.start = leader};
        auto current = leader;
        while (instruction_addresses.contains(current)) {
            const auto& instruction = slice.instructions[static_cast<std::size_t>(
                std::lower_bound(slice.instructions.begin(), slice.instructions.end(), current,
                                 [](const auto& item, std::uint32_t address) {
                                     return item.address < address;
                                 }) - slice.instructions.begin())];
            block.instruction_addresses.push_back(current);
            block.end = current + instruction.bytes.size();
            const auto next = block.end;
            if (instruction.flow == FlowKind::direct_branch || instruction.flow == FlowKind::direct_jump ||
                instruction.flow == FlowKind::indirect_jump || instruction.flow == FlowKind::return_instruction ||
                instruction.flow == FlowKind::unsupported || leaders.contains(next)) break;
            current = next;
        }
        slice.basic_blocks.push_back(std::move(block));
    }
    std::sort(slice.basic_blocks.begin(), slice.basic_blocks.end(),
              [](const auto& left, const auto& right) { return left.start < right.start; });
    return slice;
}

} // namespace oasis::tools
