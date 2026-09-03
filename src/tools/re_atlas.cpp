#include "tools/re_atlas.hpp"

#include "tools/re_diff.hpp"
#include "tools/re_program.hpp"
#include "tools/re_trace.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

struct ManifestItem {
    const char* id;
    AtlasEntryType type;
    std::uint32_t start;
    std::optional<std::uint32_t> end;
    std::size_t byte_budget;
    AtlasConfidence semantic_confidence;
    AtlasConfidence boundary_confidence;
    const char* native_path;
    NativeStatus native_status;
    const char* verification_status;
    const char* notes;
    std::optional<std::uint32_t> beta_address;
    AtlasCorrespondence beta_match;
};

const std::vector<ManifestItem>& manifest() {
    static const std::vector<ManifestItem> items{
        {"rom_00003820", AtlasEntryType::function, 0x3820, 0x3B3E, 0,
         AtlasConfidence::confirmed, AtlasConfidence::confirmed,
         "src/game/graphics_decompress.cpp", NativeStatus::verified,
         "VERIFIED_LOCAL_ORACLE", "Exact documented decompressor boundary; raw routine ID.", 0x37D0,
         AtlasCorrespondence::exact},
        {"rom_00060004", AtlasEntryType::bounded_code, 0x60004, std::nullopt, 0x1200,
         AtlasConfidence::unknown, AtlasConfidence::unknown, nullptr, NativeStatus::not_implemented,
         "VERIFIED_BOUNDED_STATIC", "Bounded entry; window is not an ownership range.", 0x60004,
         AtlasCorrespondence::exact},
        {"rom_00007A28", AtlasEntryType::bounded_code, 0x7A28, std::nullopt, 0x180,
         AtlasConfidence::unknown, AtlasConfidence::unknown, "src/game/scripts/event_router.cpp",
         NativeStatus::verified, "VERIFIED_LOCAL_ORACLE", "Raw bounded dispatch ranges; handler semantics unknown.", 0x79D8,
         AtlasCorrespondence::exact},
        {"rom_000082AE", AtlasEntryType::bounded_code, 0x82AE, std::nullopt, 0x180,
         AtlasConfidence::unknown, AtlasConfidence::unknown, "src/game/scripts/event_router.cpp",
         NativeStatus::verified, "VERIFIED_LOCAL_ORACLE", "Raw type-8 record transfer; field semantics unknown.", 0x825E,
         AtlasCorrespondence::exact},
        {"rom_00008E90", AtlasEntryType::bounded_code, 0x8E90, std::nullopt, 0x120,
         AtlasConfidence::unknown, AtlasConfidence::unknown, "src/game/entities/entity_pool.cpp",
         NativeStatus::present_unverified, "VERIFIED_BOUNDED_STATIC", "Bounded active-record loop; callback semantics unknown.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"rom_0000938E", AtlasEntryType::bounded_code, 0x938E, std::nullopt, 0x100,
         AtlasConfidence::confirmed, AtlasConfidence::unknown, "src/game/world/terrain_collision.cpp",
         NativeStatus::verified, "VERIFIED_NATIVE_RAW_CONTRACT", "Directional gate evidence is raw; no semantic field names added.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"rom_00009BF2", AtlasEntryType::bounded_code, 0x9BF2, std::nullopt, 0x100,
         AtlasConfidence::confirmed, AtlasConfidence::unknown, "src/game/world/byte_grid.cpp",
         NativeStatus::verified, "VERIFIED_NATIVE_RAW_CONTRACT", "Footprint aggregation evidence is raw; no semantic field names added.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"rom_0000A6A4", AtlasEntryType::bounded_code, 0xA6A4, std::nullopt, 0x180,
         AtlasConfidence::unknown, AtlasConfidence::unknown, nullptr, NativeStatus::not_implemented,
         "VERIFIED_BOUNDED_STATIC_DYNAMIC", "Beta structural match has changed block ordinal 10; raw only.", 0xA654,
         AtlasCorrespondence::structural},
        {"rom_0000D3B2", AtlasEntryType::function, 0xD3B2, 0xD406, 0,
         AtlasConfidence::unknown, AtlasConfidence::confirmed, nullptr, NativeStatus::not_implemented,
         "VERIFIED_LOCAL_ORACLE", "Exact reader boundary; pointer/index semantics remain raw evidence.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"table_0005CE96", AtlasEntryType::table, 0x5CE96, 0x5D046, 0,
         AtlasConfidence::unknown, AtlasConfidence::confirmed, nullptr, NativeStatus::not_applicable,
         "VERIFIED_READER_REFERENCE", "108 four-byte entries are bounded by the documented reader evidence.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"table_000096E8", AtlasEntryType::table, 0x96E8, std::nullopt, 0,
         AtlasConfidence::unknown, AtlasConfidence::unknown, nullptr, NativeStatus::not_applicable,
         "VERIFIED_TABLE_REFERENCE", "Table start is confirmed; size and ownership remain unknown.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"table_000096F8", AtlasEntryType::table, 0x96F8, std::nullopt, 0,
         AtlasConfidence::unknown, AtlasConfidence::unknown, nullptr, NativeStatus::not_applicable,
         "VERIFIED_TABLE_REFERENCE", "Table start is confirmed; size and ownership remain unknown.", std::nullopt,
         AtlasCorrespondence::not_checked},
        {"table_0000C92C", AtlasEntryType::table, 0xC92C, std::nullopt, 0,
         AtlasConfidence::unknown, AtlasConfidence::unknown, nullptr, NativeStatus::not_applicable,
         "VERIFIED_TABLE_REFERENCE", "Group table start is confirmed; full table size remains unknown.", std::nullopt,
         AtlasCorrespondence::not_checked},
    };
    return items;
}

std::vector<FunctionTarget> function_targets() {
    std::vector<FunctionTarget> result;
    for (const auto& item : manifest()) {
        if (item.type == AtlasEntryType::function || item.type == AtlasEntryType::bounded_code) {
            result.push_back({item.start, item.byte_budget, item.end});
        }
    }
    return result;
}

std::vector<std::uint32_t> unique_sorted(std::vector<std::uint32_t> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

const AnalyzedFunction* find_function(const MultiSliceReport& report, std::uint32_t entry) {
    const auto found = std::find_if(report.functions.begin(), report.functions.end(),
                                    [=](const auto& item) { return item.entry == entry; });
    return found == report.functions.end() ? nullptr : &*found;
}

const DecodedInstruction* find_instruction(const AnalyzedFunction& function,
                                           std::uint32_t address) {
    const auto found = std::find_if(function.slice.instructions.begin(), function.slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == function.slice.instructions.end() ? nullptr : &*found;
}

std::string addressing_mode_name(std::uint8_t mode, std::uint8_t reg) {
    if (mode == 2U) return "address_indirect";
    if (mode == 3U) return "address_postincrement";
    if (mode == 4U) return "address_predecrement";
    if (mode == 5U) return "address_displacement";
    if (mode == 6U) return "address_indexed";
    if (mode == 7U && reg == 3U) return "pc_indexed";
    return "unknown_addressing_mode";
}

std::string instruction_family(const std::string& mnemonic) {
    if (mnemonic == "move" || mnemonic == "moveq" || mnemonic == "movem" || mnemonic == "lea" || mnemonic == "pea") return "move_address";
    if (mnemonic == "binary" || mnemonic == "addq" || mnemonic == "subq") return "arithmetic";
    if (mnemonic == "unary" || mnemonic == "ext" || mnemonic == "swap") return "unary";
    if (mnemonic == "shift_or_rotate") return "shift_rotate";
    if (mnemonic == "immediate" || mnemonic == "static_bit") return "immediate_bit";
    if (mnemonic == "branch" || mnemonic == "bcc" || mnemonic == "dbcc" || mnemonic == "scc") return "control_condition";
    return mnemonic.empty() ? "unknown" : mnemonic;
}

void add_program_evidence(AtlasEntry& entry, const MultiSliceReport& program) {
    std::vector<std::uint32_t> callees;
    for (const auto& call : program.direct_call_sites) {
        if (call.caller_entry == entry.start) callees.push_back(call.target);
    }
    entry.callees = unique_sorted(std::move(callees));
    for (const auto& call : program.direct_call_sites) {
        if (call.target == entry.start) entry.callers.push_back(call.caller_entry);
    }
    entry.callers = unique_sorted(std::move(entry.callers));

    for (const auto& reference : program.confirmed_memory_references) {
        if (reference.function_entry != entry.start) continue;
        if (reference.reference.kind == MemoryKind::rom) entry.direct_rom_refs.push_back(reference.reference.address);
        if (reference.reference.kind == MemoryKind::ram) entry.direct_ram_refs.push_back(reference.reference.address);
    }
    entry.direct_rom_refs = unique_sorted(std::move(entry.direct_rom_refs));
    entry.direct_ram_refs = unique_sorted(std::move(entry.direct_ram_refs));
    entry.unresolved_reference_count = static_cast<std::size_t>(std::count_if(
        program.unresolved_memory_references.begin(), program.unresolved_memory_references.end(),
        [=](const auto& item) { return item.function_entry == entry.start; }));
    entry.unsupported_evidence_count = static_cast<std::size_t>(std::count_if(
        program.unsupported_addressing.begin(), program.unsupported_addressing.end(),
        [=](const auto& item) { return item.function_entry == entry.start; }));
    entry.unsupported_evidence_count += static_cast<std::size_t>(std::count_if(
        program.unsupported_instructions.begin(), program.unsupported_instructions.end(),
        [=](const auto& item) { return item.function_entry == entry.start; }));
    entry.indirect_control_flow_count = static_cast<std::size_t>(std::count_if(
        program.unresolved_control_flow.begin(), program.unresolved_control_flow.end(),
        [=](const auto& item) { return item.function_entry == entry.start; }));
    const auto* function = find_function(program, entry.start);
    if (!function) return;
    for (const auto& item : program.unresolved_memory_references) {
        if (item.function_entry != entry.start) continue;
        const auto* instruction = find_instruction(*function, item.instruction_address);
        if (!instruction) continue;
        entry.unresolved_references.push_back({
            item.instruction_address, item.block_start, item.reference.mode,
            item.reference.register_index, addressing_mode_name(item.reference.mode, item.reference.register_index),
            instruction_family(instruction->mnemonic), item.reference.reason,
            entry.start == kReTraceFunctionEntry &&
                (item.instruction_address == 0xA7D4U || item.instruction_address == 0xA7DEU),
            !instruction->immediate_constants.empty()});
    }
    for (const auto& item : program.unsupported_addressing) {
        if (item.function_entry == entry.start)
            entry.unsupported_evidence.push_back({item.instruction_address, item.block_start, "addressing", item.reference.reason});
    }
    for (const auto& item : program.unsupported_instructions) {
        if (item.function_entry == entry.start)
            entry.unsupported_evidence.push_back({item.instruction_address, item.block_start, "opcode", "unsupported_opcode"});
    }
}

AtlasCorrespondence map_match(MatchKind match) {
    switch (match) {
    case MatchKind::exact_match: return AtlasCorrespondence::exact;
    case MatchKind::structural_match: return AtlasCorrespondence::structural;
    case MatchKind::changed_blocks: return AtlasCorrespondence::changed;
    case MatchKind::unmatched: return AtlasCorrespondence::unmatched;
    }
    return AtlasCorrespondence::not_checked;
}

void add_beta_evidence(AtlasEntry& entry, const DifferentialReport& diff) {
    const auto target = std::find_if(diff.targets.begin(), diff.targets.end(),
                                     [&](const auto& item) { return item.target.entry == entry.start; });
    if (target == diff.targets.end()) return;
    const AnalogCandidate* candidate = nullptr;
    for (const auto& analog : target->analogs) {
        if (analog.match != MatchKind::unmatched) {
            candidate = &analog;
            break;
        }
    }
    if (!candidate && target->same_address_match != MatchKind::unmatched) {
        entry.beta = AtlasBetaCorrespondence{entry.start, map_match(target->same_address_match), {}};
        return;
    }
    if (!candidate) {
        entry.beta = AtlasBetaCorrespondence{entry.start, AtlasCorrespondence::unmatched, {}};
        return;
    }
    AtlasBetaCorrespondence beta{candidate->beta_entry, map_match(candidate->match), {}};
    for (const auto& block : candidate->changed_blocks) beta.changed_blocks.push_back(block.ordinal);
    entry.beta = std::move(beta);
}

bool has_trace_signature(std::span<const std::uint8_t> rom) {
    if (rom.size() < 0xA7E6U) return false;
    const auto word = [&](std::uint32_t address) {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom[address]) << 8U) | rom[address + 1U]);
    };
    return word(0xA7D4) == 0x0C6E && word(0xA7DA) == 0x6700 &&
           word(0xA7DE) == 0x206E && word(0xA7E2) == 0x4ED0 && word(0xA7E4) == 0x4E75;
}

void add_dynamic_evidence(AtlasEntry& entry, std::span<const std::uint8_t> rom) {
    if (entry.start != kReTraceFunctionEntry || !has_trace_signature(rom)) return;
    const auto trace = trace_m68k_scenario(rom);
    AtlasDynamicSummary summary{
        trace.executed_instructions.size(), trace.executed_basic_blocks.size(), trace.memory_reads.size(),
        trace.memory_writes.size(), trace.branches.size(), trace.calls.size(), trace.returns.size(),
        {"0xA7D4 -> 0x00FF2954", "0xA7DE -> 0x00FF2976", "0xA7E2 -> 0xA7E4"}};
    entry.dynamic = std::move(summary);
}

std::vector<std::pair<std::uint32_t, std::uint32_t>> intervals(const AtlasEntry& entry) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> result;
    if (entry.end && *entry.end > entry.start) result.emplace_back(entry.start, *entry.end);
    else if (entry.bounded_evidence_end && *entry.bounded_evidence_end > entry.start) {
        result.emplace_back(entry.start, *entry.bounded_evidence_end);
    }
    return result;
}

std::size_t overlap_bytes(std::span<const AtlasEntry> entries) {
    std::vector<std::uint32_t> points;
    for (const auto& entry : entries) for (const auto& range : intervals(entry)) {
        points.push_back(range.first); points.push_back(range.second);
    }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    std::size_t total = 0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        std::size_t count = 0;
        for (const auto& entry : entries) for (const auto& range : intervals(entry))
            if (range.first < points[i] && range.second > points[i - 1]) ++count;
        if (count > 1) total += points[i] - points[i - 1];
    }
    return total;
}

bool contains(const AtlasEntry& entry, std::uint32_t address) {
    for (const auto& range : intervals(entry)) if (address >= range.first && address < range.second) return true;
    return entry.start == address;
}

} // namespace

AtlasReport build_rom_atlas(std::span<const std::uint8_t> retail_rom,
                            std::optional<std::span<const std::uint8_t>> beta_rom) {
    AtlasReport report;
    report.retail = identify_rom(retail_rom);
    if (beta_rom) report.beta = identify_rom(*beta_rom);
    const auto program = analyze_m68k_functions(retail_rom, function_targets());

    for (const auto& item : manifest()) {
        AtlasEntry entry{.id = item.id, .type = item.type, .start = item.start, .end = item.end,
                         .bounded_evidence_end = item.byte_budget ? item.start + item.byte_budget : item.end,
                         .semantic_confidence = item.semantic_confidence,
                         .boundary_confidence = item.boundary_confidence,
                         .evidence_sources = {"docs/REVERSE_ENGINEERING.md", "src/tools/re_program.cpp",
                                               "src/tools/re_trace.cpp", "src/tools/re_diff.cpp"},
                         .native = {item.native_status, item.native_path ? std::optional<std::string>(item.native_path) : std::nullopt},
                         .verification_status = item.verification_status, .notes = item.notes};
        if (const auto* function = find_function(program, item.start)) add_program_evidence(entry, program);
        if (item.beta_address) entry.beta = AtlasBetaCorrespondence{*item.beta_address, item.beta_match, {}};
        add_dynamic_evidence(entry, retail_rom);
        report.entries.push_back(std::move(entry));
    }

    for (const auto& call : program.direct_call_sites) {
        auto found = std::find_if(report.call_edges.begin(), report.call_edges.end(), [&](const auto& edge) {
            return edge.caller == call.caller_entry && edge.callee == call.target;
        });
        if (found == report.call_edges.end()) report.call_edges.push_back({call.caller_entry, call.target, {call.instruction_address}});
        else found->call_sites.push_back(call.instruction_address);
    }
    for (const auto& edge : report.call_edges) {
        for (auto& entry : report.entries) {
            if (entry.start == edge.callee) entry.callers.push_back(edge.caller);
        }
    }
    for (auto& entry : report.entries) {
        entry.callers = unique_sorted(std::move(entry.callers));
        entry.callees = unique_sorted(std::move(entry.callees));
    }
    if (beta_rom) {
        static const DifferentialTarget targets[] = {
            {0x3820, 0, 0x3B3E}, {0x60004, 0x1200, std::nullopt}, {0x82AE, 0x180, std::nullopt},
            {0x7A28, 0x180, std::nullopt}, {0xA6A4, 0x180, std::nullopt},
        };
        const auto diff = compare_m68k_revisions(retail_rom, *beta_rom, targets);
        for (auto& entry : report.entries) if (entry.beta) add_beta_evidence(entry, diff);
    }
    report.conflicts = detect_atlas_conflicts(report.entries);
    report.coverage.rom_size = retail_rom.size();
    report.coverage.atlas_entries = report.entries.size();
    for (const auto& entry : report.entries) {
        if (entry.end && entry.boundary_confidence == AtlasConfidence::confirmed)
            report.coverage.confirmed_classified_bytes += *entry.end - entry.start;
        if (entry.type == AtlasEntryType::bounded_code && entry.bounded_evidence_end && !entry.end)
            report.coverage.bounded_evidence_bytes += *entry.bounded_evidence_end - entry.start;
        if (entry.native.status == NativeStatus::verified) ++report.coverage.verified_native_implementations;
        report.coverage.unresolved_unknown_references += entry.unresolved_reference_count +
                                                         entry.unsupported_evidence_count +
                                                         entry.indirect_control_flow_count;
    }
    report.coverage.overlapping_evidence_bytes = overlap_bytes(report.entries);
    report.coverage.unknown_remainder_bytes = report.retail.fingerprint.size > report.coverage.confirmed_classified_bytes
                                                  ? report.retail.fingerprint.size - report.coverage.confirmed_classified_bytes : 0;
    return report;
}

std::vector<AtlasConflict> detect_atlas_conflicts(std::span<const AtlasEntry> entries) {
    std::vector<AtlasConflict> result;
    for (std::size_t i = 0; i < entries.size(); ++i) for (std::size_t j = i + 1; j < entries.size(); ++j) {
        for (const auto& left : intervals(entries[i])) for (const auto& right : intervals(entries[j])) {
            const auto start = std::max(left.first, right.first);
            const auto end = std::min(left.second, right.second);
            if (start >= end) continue;
            if (entries[i].type == entries[j].type && entries[i].end == entries[j].end) continue;
            result.push_back({entries[i].id, entries[j].id, start, end, "incompatible typed evidence overlap"});
        }
    }
    return result;
}

std::vector<const AtlasEntry*> atlas_entries_at(const AtlasReport& report, std::uint32_t address) {
    std::vector<const AtlasEntry*> result;
    for (const auto& entry : report.entries) if (contains(entry, address)) result.push_back(&entry);
    return result;
}

std::vector<std::uint32_t> atlas_callers(const AtlasReport& report, std::uint32_t entry) {
    for (const auto& item : report.entries) if (item.start == entry) return item.callers;
    return {};
}

std::vector<std::uint32_t> atlas_callees(const AtlasReport& report, std::uint32_t entry) {
    for (const auto& item : report.entries) if (item.start == entry) return item.callees;
    return {};
}

std::vector<const AtlasEntry*> atlas_refs_to_rom(const AtlasReport& report, std::uint32_t address) {
    std::vector<const AtlasEntry*> result;
    for (const auto& entry : report.entries)
        if (std::find(entry.direct_rom_refs.begin(), entry.direct_rom_refs.end(), address) != entry.direct_rom_refs.end()) result.push_back(&entry);
    return result;
}

std::vector<const AtlasEntry*> atlas_refs_to_ram(const AtlasReport& report, std::uint32_t address) {
    std::vector<const AtlasEntry*> result;
    for (const auto& entry : report.entries)
        if (std::find(entry.direct_ram_refs.begin(), entry.direct_ram_refs.end(), address) != entry.direct_ram_refs.end()) result.push_back(&entry);
    return result;
}

std::vector<const AtlasEntry*> atlas_entries_without_native(const AtlasReport& report) {
    std::vector<const AtlasEntry*> result;
    for (const auto& entry : report.entries) if (entry.native.status != NativeStatus::verified) result.push_back(&entry);
    return result;
}

std::vector<const AtlasEntry*> atlas_entries_with_unresolved(const AtlasReport& report) {
    std::vector<const AtlasEntry*> result;
    for (const auto& entry : report.entries)
        if (entry.unresolved_reference_count || entry.unsupported_evidence_count || entry.indirect_control_flow_count) result.push_back(&entry);
    return result;
}

std::optional<AtlasBetaCorrespondence> atlas_beta_lookup(const AtlasReport& report, std::uint32_t entry) {
    for (const auto& item : report.entries) if (item.start == entry && item.beta) return item.beta;
    return std::nullopt;
}

} // namespace oasis::tools
