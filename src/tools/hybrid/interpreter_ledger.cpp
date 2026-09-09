#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/hybrid/recomp_generator.hpp"
#include "tools/re_assemble.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using oasis::tools::DecodedInstruction;
using oasis::tools::DecodedOperand;
using oasis::tools::FlowKind;
using oasis::tools::MemoryAccess;
using oasis::tools::MemoryKind;
using oasis::tools::OperandKind;

struct ProfileEntry { std::uint32_t pc{}; std::uint64_t count{}; };

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read profile");
    return {std::istreambuf_iterator<char>(input), {}};
}

std::vector<ProfileEntry> read_profile(const char* path) {
    const auto text = read_file(path);
    const std::regex entry(R"REGEX(\{"pc":"(0x[0-9A-Fa-f]+)","count":([0-9]+)\})REGEX");
    std::vector<ProfileEntry> result;
    for (std::sregex_iterator it(text.begin(), text.end(), entry), end; it != end; ++it)
        result.push_back({static_cast<std::uint32_t>(std::stoul((*it)[1].str(), nullptr, 16)),
                          std::stoull((*it)[2].str())});
    if (result.empty()) throw std::runtime_error("profile has no PC entries");
    return result;
}

std::string hex(std::uint32_t value, unsigned width = 6) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << value;
    return out.str();
}

std::string bytes(const DecodedInstruction& instruction) {
    std::ostringstream out;
    for (const auto byte : instruction.bytes)
        out << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
            << static_cast<unsigned>(byte);
    return out.str();
}

std::string predecessor(std::span<const std::uint8_t> rom, std::uint32_t pc) {
    for (unsigned distance = 2; distance <= 16 && distance <= pc; distance += 2) {
        const auto candidate = pc - distance;
        const auto slice = oasis::tools::decode_m68k_slice(
            rom, {.entry = candidate, .byte_budget = distance, .instruction_budget = 1});
        if (!slice.instructions.empty() && slice.instructions.front().supported &&
            candidate + slice.instructions.front().bytes.size() == pc)
            return "fallthrough from " + hex(candidate);
    }
    return "no bounded fallthrough predecessor";
}

std::string successor(const DecodedInstruction& instruction) {
    const auto fallthrough = instruction.address + static_cast<std::uint32_t>(instruction.bytes.size());
    if (instruction.direct_target && instruction.flow == FlowKind::direct_branch)
        return "fallthrough " + hex(fallthrough) + "; branch target " + hex(*instruction.direct_target);
    if (instruction.direct_target)
        return "direct target " + hex(*instruction.direct_target);
    if (instruction.flow == FlowKind::return_instruction) return "return; no static successor";
    return "fallthrough " + hex(fallthrough);
}

std::string memory(const DecodedInstruction& instruction) {
    std::ostringstream out;
    bool first = true;
    for (const auto& ref : instruction.memory_references) {
        if (!first) out << "; ";
        first = false;
        out << memory_kind_name(ref.kind) << ' ' << memory_access_name(ref.access)
            << ' ' << hex(ref.address) << '/' << unsigned(ref.width_bytes);
    }
    if (!instruction.unresolved_memory_references.empty()) {
        if (!first) out << "; ";
        out << "register-based unresolved (" << instruction.unresolved_memory_references.size() << ')';
    }
    return first ? "none" : out.str();
}

bool hardware_visible(std::uint32_t pc) {
    return pc == 0x060BA4U;
}

bool operand_is(const std::optional<DecodedOperand>& operand, OperandKind kind) {
    return operand && operand->kind == kind;
}

bool semantics_verified(const DecodedInstruction& instruction) {
    if (!instruction.exact) return false;
    const auto& exact = *instruction.exact;
    if (exact.branch_width_bytes) return true;
    if (exact.operation == "tst" && exact.width_bytes <= 2 &&
        operand_is(exact.destination, OperandKind::absolute_long)) return true;
    if (exact.operation == "movem" && exact.width_bytes == 4 &&
        operand_is(exact.source, OperandKind::register_list) &&
        operand_is(exact.destination, OperandKind::predecrement)) return true;
    if (exact.operation == "move" && exact.width_bytes == 4 &&
        operand_is(exact.source, OperandKind::data_register) &&
        operand_is(exact.destination, OperandKind::predecrement)) return true;
    if (exact.operation == "move" && exact.width_bytes == 1 &&
        operand_is(exact.source, OperandKind::postincrement) &&
        operand_is(exact.destination, OperandKind::postincrement)) return true;
    if (exact.operation == "move" && exact.width_bytes == 1 &&
        operand_is(exact.source, OperandKind::immediate) &&
        operand_is(exact.destination, OperandKind::postincrement)) return true;
    if (exact.operation == "clr" && exact.width_bytes == 2 &&
        operand_is(exact.destination, OperandKind::data_register)) return true;
    if (exact.operation == "clr" && exact.width_bytes == 1 &&
        operand_is(exact.destination, OperandKind::postincrement)) return true;
    if (exact.operation == "clr" && exact.width_bytes == 2 &&
        operand_is(exact.destination, OperandKind::postincrement)) return true;
    if (exact.operation == "move" && exact.width_bytes == 1 &&
        operand_is(exact.source, OperandKind::postincrement) &&
        operand_is(exact.destination, OperandKind::data_register)) return true;
    if (exact.operation == "move" && exact.width_bytes == 2 &&
        operand_is(exact.source, OperandKind::postincrement) &&
        operand_is(exact.destination, OperandKind::postincrement)) return true;
    if (exact.operation == "btst" && exact.width_bytes == 1 &&
        operand_is(exact.source, OperandKind::immediate) &&
        operand_is(exact.destination, OperandKind::displacement)) return true;
    if (exact.operation == "lea" && exact.width_bytes == 4 &&
        operand_is(exact.source, OperandKind::absolute_long) &&
        operand_is(exact.destination, OperandKind::address_register)) return true;
    if (exact.operation == "adda" && exact.width_bytes == 2 &&
        operand_is(exact.source, OperandKind::data_register) &&
        operand_is(exact.destination, OperandKind::address_register)) return true;
    if (exact.operation == "add" && exact.width_bytes == 4 &&
        operand_is(exact.source, OperandKind::data_register) &&
        operand_is(exact.destination, OperandKind::data_register)) return true;
    if (exact.operation == "add" && exact.width_bytes == 2 &&
        operand_is(exact.source, OperandKind::postincrement) &&
        operand_is(exact.destination, OperandKind::data_register)) return true;
    return false;
}

struct Classification {
    std::string terminal;
    std::string blocker;
    std::string history;
};

Classification classify(const DecodedInstruction& instruction, std::uint32_t pc,
                        std::uint64_t count, bool generated) {
    if ((pc == 0x03A7AEU || pc == 0x03A7B4U) && generated)
        return {"PROMOTABLE_EXISTING_PROOF", "none; M11.44 shadow/native certified this generated block", "M11.35 rejection retained; obsolete after M11.36+ bridge and M11.43 canonicalization"};
    if (hardware_visible(pc))
        return {"HARDWARE_VISIBLE_BLOCKED", "absolute access is outside the proven shared bus contract", "M11.37 hardware fallback retained"};
    if (instruction.flow == FlowKind::indirect_jump || instruction.flow == FlowKind::indirect_call)
        return {"INDIRECT_CFG_BLOCKED", "indirect target set is not proven by this bounded trace", "none recorded"};
    if (!instruction.supported || !instruction.exact)
        return {"DECODER_BLOCKED", "exact decoder/IR is unavailable", "none recorded"};
    if (!instruction.unresolved_memory_references.empty())
        return {"UNKNOWN_WITH_EVIDENCE", "runtime register-based address and hardware class are not proven", "decoder reports register_based unresolved reference"};
    if (generated && semantics_verified(instruction))
        return {"PROMOTABLE_EXISTING_PROOF", "existing bounded semantic proof; candidate still needs independent shadow/native gate", "none recorded"};
    if (!generated)
        return {"NEW_SEMANTICS_REQUIRED", "exact form has no mechanical emitter and needs bounded independent vectors", "none recorded"};
    if (count < 1000)
        return {"COLD_OR_LOW_PAYOFF", "dynamic count is below the bounded high-payoff threshold", "none recorded"};
    return {"NEW_SEMANTICS_REQUIRED", "generator support is not paired with an accepted exact semantic proof", "none recorded"};
}

std::string asm_text(const DecodedInstruction& instruction) {
    if (!instruction.supported || !instruction.exact) return "UNSUPPORTED";
    try { return oasis::tools::exact_instruction_asm(instruction); }
    catch (const std::exception&) { return "EXACT_IR_NO_ASM"; }
}

std::string semantic_text(const DecodedInstruction& instruction) {
    return semantics_verified(instruction) ? "BOUNDED_PROOF" :
        instruction.exact ? "UNVERIFIED_EXACT_FORM" : "NO_EXACT_IR";
}

void write_report(const oasis::Rom& rom, const std::vector<ProfileEntry>& profile,
                  const char* output_path, std::uint64_t expected) {
    std::uint64_t total = 0;
    for (const auto& item : profile) total += item.count;
    if (total != expected) throw std::runtime_error("profile count does not match expected count");
    std::ofstream output(output_path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot write ledger");
    output << "# M11.44 — Remaining Interpreter Attribution Ledger\n\n"
           << "Result: exhaustive decoder-backed ledger for `" << expected
           << "` remaining interpreter executions.\n\n"
           << "The profile is an M11.44 `BASIC_BLOCK_NATIVE` 600-frame\n"
           << "baseline. Each row is one executed PC; counts are dynamic executions,\n"
           << "not ROM-byte coverage. Predecessor/successor fields are bounded ROM\n"
           << "fallthrough/branch evidence from the existing `re_slice_decoder`.\n\n"
           << "| PC | count | opcode bytes | exact decoded form | decoder ownership/range | natural predecessor/successor | semantic verification | generator support | memory class | hardware visibility | prior rejection/history | terminal class | exact current blocker |\n"
           << "| --- | ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
    std::map<std::string, std::uint64_t> totals;
    for (const auto& item : profile) {
        const auto slice = oasis::tools::decode_m68k_slice(
            rom.bytes(), {.entry = item.pc, .byte_budget = 16, .instruction_budget = 1});
        if (slice.instructions.empty()) throw std::runtime_error("profile PC did not decode");
        const auto& instruction = slice.instructions.front();
        const auto end = instruction.address + static_cast<std::uint32_t>(instruction.bytes.size());
        bool generated = false;
        try { (void)oasis::hybrid::generate_block(rom.bytes(), item.pc, end); generated = true; }
        catch (const std::exception&) {}
        const auto classification = classify(instruction, item.pc, item.count, generated);
        totals[classification.terminal] += item.count;
        output << "| " << hex(item.pc) << " | " << item.count << " | `" << bytes(instruction)
               << "` | `" << asm_text(instruction) << "` | `re_slice_decoder/"
               << (instruction.supported && instruction.exact ? "exact" : "unsupported")
               << " [" << hex(item.pc) << ", " << hex(end) << ")` | `"
               << predecessor(rom.bytes(), item.pc) << "; " << successor(instruction) << "` | `"
               << semantic_text(instruction) << "` | `"
               << (generated ? "SUPPORTED_SINGLE_INSTRUCTION" : "FAIL_CLOSED") << "` | `"
               << memory(instruction) << "` | `" << (hardware_visible(item.pc) ? "YES" : "not proven")
               << "` | " << classification.history << " | `" << classification.terminal << "` | "
               << classification.blocker << " |\n";
    }
    output << "\n## Count closure\n\n| terminal class | executions | % of total | % of remaining |\n| --- | ---: | ---: | ---: |\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& [category, count] : totals)
        output << "| `" << category << "` | " << count << " | "
               << (100.0 * count / 6488773.0) << "% | " << (100.0 * count / expected) << "% |\n";
    output << "| **TOTAL** | **" << total << "** | **"
           << (100.0 * total / 6488773.0) << "%** | **100.0000%** |\n\n"
           << "No generic `other` bucket is used. Every row is assigned to one of\n"
           << "the M11.44 terminal classes, with unresolved register-based memory\n"
           << "retained as `UNKNOWN_WITH_EVIDENCE` rather than guessed.\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: oasis_hybrid_interpreter_ledger <rom> <profile.json> <report.md> [expected_count]\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::invalid_argument("canonical supported USA ROM required");
        const auto expected = argc == 5 ? std::stoull(argv[4]) : 661916U;
        write_report(rom, read_profile(argv[2]), argv[3], expected);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
