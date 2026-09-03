#include "tools/re_resolution.hpp"

#include "tools/re_atlas_ranking.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

constexpr std::uint32_t kTargetEntry = 0x60004U;
using RegisterState = std::array<std::optional<std::uint32_t>, 8>;

struct State {
    RegisterState values{};
    std::array<bool, 8> merge_conflict{};
    std::array<std::vector<ResolutionProofStep>, 8> provenance{};
};

struct Transfer {
    enum class Kind { none, set, copy, add, sub, invalidate } kind{Kind::none};
    std::uint8_t destination{};
    std::uint8_t source{};
    std::uint32_t value{};
    std::string operation;
};

std::uint16_t read16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 1U >= bytes.size()) throw std::runtime_error("truncated bounded instruction");
    return static_cast<std::uint16_t>((bytes[offset] << 8U) | bytes[offset + 1U]);
}

std::uint32_t read32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(read16(bytes, offset)) << 16U) | read16(bytes, offset + 2U);
}

std::int32_t signed_value(std::uint32_t value, unsigned width) {
    if (width == 2U) return static_cast<std::int16_t>(value);
    return static_cast<std::int32_t>(value);
}

std::size_t ea_extension_size(std::uint8_t mode, std::uint8_t reg, unsigned size_bytes) {
    if (mode == 5U || mode == 6U) return 2U;
    if (mode != 7U) return 0U;
    if (reg == 0U) return 2U;
    if (reg == 1U) return 4U;
    if (reg == 2U || reg == 3U) return 2U;
    if (reg == 4U) return size_bytes == 4U ? 4U : 2U;
    return 0U;
}

std::optional<std::size_t> displacement_offset(const DecodedInstruction& instruction,
                                                const AtlasUnresolvedReference& reference) {
    const auto opcode = instruction.opcode;
    const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
    const auto source_reg = static_cast<std::uint8_t>(opcode & 7U);
    if (reference.mode != 5U) return std::nullopt;

    // MOVE has source and destination extension words. Only accept a single
    // unambiguous displacement operand in this bounded pass.
    if ((opcode >> 12U) >= 1U && (opcode >> 12U) <= 3U) {
        const auto destination_mode = static_cast<std::uint8_t>((opcode >> 6U) & 7U);
        const auto destination_reg = static_cast<std::uint8_t>((opcode >> 9U) & 7U);
        const bool source_match = source_mode == 5U && source_reg == reference.register_index;
        const bool destination_match = destination_mode == 5U && destination_reg == reference.register_index;
        if (source_match == destination_match) return std::nullopt;
        if (source_match) return 2U;
        const unsigned size_bytes = (opcode >> 12U) == 2U ? 4U : 2U;
        return 2U + ea_extension_size(source_mode, source_reg, size_bytes);
    }
    if (instruction.bytes.size() < 4U) return std::nullopt;
    return 2U;
}

Transfer recognize_transfer(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    const auto destination = static_cast<std::uint8_t>((opcode >> 9U) & 7U);
    const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
    const auto source_reg = static_cast<std::uint8_t>(opcode & 7U);
    const auto destination_mode = static_cast<std::uint8_t>((opcode >> 6U) & 7U);

    if ((opcode & 0xF1FFU) == 0x207CU || (opcode & 0xF1FFU) == 0x307CU) {
        const auto width = (opcode & 0x3000U) == 0x2000U ? 4U : 2U;
        const auto raw = width == 4U ? read32(instruction.bytes, 2U) : read16(instruction.bytes, 2U);
        return {Transfer::Kind::set, destination, 0, static_cast<std::uint32_t>(signed_value(raw, width)),
                "MOVEA immediate -> A" + std::to_string(destination)};
    }
    if ((opcode & 0xF1C0U) == 0x41C0U) {
        const auto mode = source_mode;
        std::optional<std::uint32_t> address;
        std::string operation;
        if (mode == 7U && source_reg == 0U) {
            address = static_cast<std::uint32_t>(static_cast<std::int16_t>(read16(instruction.bytes, 2U)));
            operation = "LEA absolute-word -> A" + std::to_string(destination);
        } else if (mode == 7U && source_reg == 1U) {
            address = read32(instruction.bytes, 2U);
            operation = "LEA absolute-long -> A" + std::to_string(destination);
        } else if (mode == 7U && source_reg == 2U) {
            const auto displacement = static_cast<std::int16_t>(read16(instruction.bytes, 2U));
            address = static_cast<std::uint32_t>(instruction.address + 2U + displacement);
            operation = "LEA PC-displacement -> A" + std::to_string(destination);
        } else {
            return {Transfer::Kind::invalidate, destination, 0, 0, "unsupported LEA transfer"};
        }
        return {Transfer::Kind::set, destination, 0, *address, std::move(operation)};
    }
    if ((opcode & 0xF1FFU) == 0xD0FCU || (opcode & 0xF1FFU) == 0xD1FCU ||
        (opcode & 0xF1FFU) == 0x90FCU || (opcode & 0xF1FFU) == 0x91FCU) {
        const bool is_sub = (opcode & 0xF000U) == 0x9000U;
        const bool is_long = (opcode & 0x0100U) != 0U;
        const auto width = is_long ? 4U : 2U;
        const auto raw = width == 4U ? read32(instruction.bytes, 2U) : read16(instruction.bytes, 2U);
        const auto value = static_cast<std::uint32_t>(signed_value(raw, width));
        return {is_sub ? Transfer::Kind::sub : Transfer::Kind::add, destination, 0, value,
                std::string(is_sub ? "SUBA immediate -> A" : "ADDA immediate -> A") + std::to_string(destination)};
    }
    if ((opcode & 0xF000U) == 0x2000U && destination_mode == 1U && source_mode == 1U) {
        return {Transfer::Kind::copy, destination, source_reg, 0,
                "MOVEA A" + std::to_string(source_reg) + " -> A" + std::to_string(destination)};
    }
    if ((opcode & 0xF000U) >= 0x2000U && (opcode & 0xF000U) <= 0x3000U && destination_mode == 1U)
        return {Transfer::Kind::invalidate, destination, 0, 0, "unsupported MOVEA transfer"};
    if ((opcode & 0xF100U) == 0x5000U && ((opcode >> 3U) & 7U) == 1U) {
        const auto immediate = static_cast<std::uint32_t>(((opcode >> 9U) & 7U) == 0U ? 8U : (opcode >> 9U) & 7U);
        const bool is_sub = (opcode & 0x0100U) != 0U;
        return {is_sub ? Transfer::Kind::sub : Transfer::Kind::add, static_cast<std::uint8_t>(opcode & 7U), 0,
                immediate, std::string(is_sub ? "SUBQ -> A" : "ADDQ -> A") + std::to_string(opcode & 7U)};
    }
    if ((opcode & 0xF000U) == 0x9000U || (opcode & 0xF000U) == 0xD000U) {
        const auto opmode = static_cast<unsigned>((opcode >> 6U) & 7U);
        if (opmode == 2U || opmode == 3U) return {Transfer::Kind::invalidate, destination, 0, 0, "unsupported address transfer"};
    }
    if (instruction.mnemonic == "link") return {Transfer::Kind::invalidate, 7, 0, 0, "LINK writes A7"};
    if (instruction.mnemonic == "unlk") return {Transfer::Kind::invalidate, static_cast<std::uint8_t>(opcode & 7U), 0, 0, "UNLK writes An"};
    if (instruction.mnemonic == "movem") return {Transfer::Kind::invalidate, 0, 0, 0, "MOVEM address-register transfer"};
    return {};
}

bool same_value(const State& left, const State& right) {
    return left.values == right.values && left.merge_conflict == right.merge_conflict;
}

State merge_state(const State& existing, const State& incoming) {
    State result = existing;
    for (std::size_t i = 0; i < 8U; ++i) {
        if (existing.values[i] && incoming.values[i] && *existing.values[i] == *incoming.values[i] &&
            !existing.merge_conflict[i] && !incoming.merge_conflict[i]) continue;
        if (!existing.values[i] && !incoming.values[i] && !existing.merge_conflict[i] && !incoming.merge_conflict[i]) continue;
        result.values[i] = std::nullopt;
        result.merge_conflict[i] = existing.merge_conflict[i] || incoming.merge_conflict[i] ||
            (existing.values[i] != incoming.values[i]);
    }
    return result;
}

void clear_state(State& state) {
    for (std::size_t i = 0; i < 8U; ++i) {
        state.values[i] = std::nullopt;
        state.merge_conflict[i] = false;
        state.provenance[i].clear();
    }
}

void apply_transfer(State& state, const Transfer& transfer, std::uint32_t address, std::uint32_t block_start) {
    if (transfer.kind == Transfer::Kind::none) return;
    const auto destination = transfer.destination;
    if (transfer.kind == Transfer::Kind::invalidate) {
        if (transfer.operation == "MOVEM address-register transfer") clear_state(state);
        else {
            state.values[destination] = std::nullopt;
            state.merge_conflict[destination] = false;
            state.provenance[destination].clear();
        }
        return;
    }
    if (transfer.kind == Transfer::Kind::copy) {
        state.values[destination] = state.values[transfer.source];
        state.merge_conflict[destination] = state.merge_conflict[transfer.source];
        state.provenance[destination] = state.provenance[transfer.source];
    } else if (transfer.kind == Transfer::Kind::set) {
        state.values[destination] = transfer.value;
        state.merge_conflict[destination] = false;
        state.provenance[destination].clear();
    } else {
        if (state.values[destination]) {
            const auto old = *state.values[destination];
            state.values[destination] = transfer.kind == Transfer::Kind::add ? old + transfer.value : old - transfer.value;
        }
        state.merge_conflict[destination] = state.merge_conflict[destination] || !state.values[destination];
    }
    if (state.values[destination]) {
        state.provenance[destination].push_back({address, block_start, transfer.operation, *state.values[destination]});
        if (state.provenance[destination].size() > 12U)
            state.provenance[destination].erase(state.provenance[destination].begin());
    }
}

EffectiveAddressClass classify(std::uint32_t address) {
    if (address < 0x00400000U) return EffectiveAddressClass::rom;
    if (address >= 0x00FF0000U && address <= 0x00FFFFFFU) return EffectiveAddressClass::ram;
    return EffectiveAddressClass::outside_known_address_space;
}

std::string register_name(std::uint8_t index) { return "A" + std::to_string(index); }

void add_count(std::vector<ResolutionCount>& counts, const std::string& key) {
    const auto found = std::find_if(counts.begin(), counts.end(), [&](const auto& item) { return item.key == key; });
    if (found == counts.end()) counts.push_back({key, 1}); else ++found->count;
}

void sort_counts(std::vector<ResolutionCount>& counts) {
    std::sort(counts.begin(), counts.end(), [](const auto& left, const auto& right) {
        return std::tie(left.key, left.count) < std::tie(right.key, right.count);
    });
}

std::vector<std::pair<std::uint32_t, std::uint32_t>> cfg_successors(const DecodedSlice& slice,
                                                                      const BasicBlock& block) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> result;
    if (block.instruction_addresses.empty()) return result;
    const auto last_address = block.instruction_addresses.back();
    const auto instruction = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                           [=](const auto& item) { return item.address == last_address; });
    if (instruction == slice.instructions.end()) return result;
    for (const auto& edge : slice.control_flow)
        if (edge.source == last_address) result.emplace_back(edge.target, static_cast<std::uint32_t>(edge.kind));
    if (instruction->flow == FlowKind::direct_call ||
        (instruction->flow == FlowKind::direct_branch && instruction->mnemonic != "bra")) {
        const auto fallthrough = block.end;
        if (std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                        [=](const auto& candidate) { return candidate.start == fallthrough; }))
            result.emplace_back(fallthrough, static_cast<std::uint32_t>(FlowKind::none));
    } else if (instruction->flow == FlowKind::none) {
        if (std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                        [=](const auto& candidate) { return candidate.start == block.end; }))
            result.emplace_back(block.end, static_cast<std::uint32_t>(FlowKind::none));
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

std::uint32_t edge_kind(const std::pair<std::uint32_t, std::uint32_t>& edge) { return edge.second; }

void make_ranges(ResolutionReport& report) {
    std::map<EffectiveAddressClass, std::vector<std::uint32_t>> addresses;
    for (const auto& item : report.items)
        if (item.effective_address) addresses[item.address_class].push_back(*item.effective_address);
    std::set<std::uint32_t> unique;
    for (const auto& [kind, values] : addresses) {
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());
        for (const auto value : sorted) unique.insert(value);
        std::size_t index = 0;
        while (index < sorted.size()) {
            const auto start = sorted[index];
            auto end = start;
            std::size_t count = 0;
            while (index < sorted.size() && (index == 0U || sorted[index] <= end + 1U)) {
                end = sorted[index++];
                ++count;
            }
            report.effective_address_ranges.push_back({kind, start, end, count});
        }
    }
    report.unique_concrete_address_count = unique.size();
    report.rom_effective_address_count = addresses[EffectiveAddressClass::rom].size();
    report.ram_effective_address_count = addresses[EffectiveAddressClass::ram].size();
}

ResolutionReport resolve_slice(const AtlasEntry& atlas_entry, const DecodedSlice& slice) {
    ResolutionReport report;
    report.target_entry = atlas_entry.start;
    std::map<std::uint32_t, std::vector<const AtlasUnresolvedReference*>> candidates;
    for (const auto& reference : atlas_entry.unresolved_references) {
        if (reference.addressing_mode == "address_displacement")
            candidates[reference.instruction_address].push_back(&reference);
    }
    report.static_candidate_count = 0;
    for (const auto& [address, references] : candidates) report.static_candidate_count += references.size();
    std::map<std::uint32_t, std::size_t> block_by_address;
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto address : slice.basic_blocks[i].instruction_addresses) block_by_address[address] = i;
    std::map<std::uint32_t, std::vector<std::pair<std::uint32_t, std::uint32_t>>> successors;
    for (const auto& block : slice.basic_blocks) successors[block.start] = cfg_successors(slice, block);

    std::vector<std::optional<State>> incoming(slice.basic_blocks.size());
    if (!slice.basic_blocks.empty()) incoming[block_by_address[slice.entry]] = State{};
    std::vector<std::size_t> worklist;
    if (!slice.basic_blocks.empty()) worklist.push_back(block_by_address[slice.entry]);
    std::set<std::tuple<std::uint32_t, std::uint32_t, std::uint8_t>> seen_items;
    while (!worklist.empty()) {
        const auto block_index = worklist.front();
        worklist.erase(worklist.begin());
        if (!incoming[block_index]) continue;
        auto state = *incoming[block_index];
        const auto& block = slice.basic_blocks[block_index];
        for (const auto address : block.instruction_addresses) {
            const auto instruction = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                                   [=](const auto& item) { return item.address == address; });
            if (instruction == slice.instructions.end()) continue;
            const auto candidate = candidates.find(address);
            if (candidate != candidates.end()) {
                for (const auto* reference : candidate->second) {
                    ResolutionItem item;
                    item.instruction_address = address;
                    item.block_start = block.start;
                    item.base_register = reference->register_index;
                    const auto offset = displacement_offset(*instruction, *reference);
                    if (!offset || *offset + 1U >= instruction->bytes.size()) {
                        item.status = ResolutionStatus::unresolved_unsupported_transfer;
                        item.reason = "ambiguous displacement extension";
                    } else {
                        item.displacement = static_cast<std::int16_t>(read16(instruction->bytes, *offset));
                        if (state.merge_conflict[item.base_register]) {
                            item.status = ResolutionStatus::unresolved_cfg_merge;
                            item.reason = "conflicting predecessor base values";
                        } else if (!state.values[item.base_register]) {
                            item.status = ResolutionStatus::unresolved_unknown_base;
                            item.reason = "base register value unknown";
                        } else {
                            item.base_value = state.values[item.base_register];
                            item.effective_address = *item.base_value + item.displacement;
                            item.address_class = classify(*item.effective_address);
                            item.status = ResolutionStatus::resolved;
                            item.reason = "base + signed displacement";
                            item.provenance = state.provenance[item.base_register];
                            if (item.provenance.empty()) ++report.provenance_failures;
                        }
                    }
                    add_count(report.reason_counts, resolution_status_name(item.status));
                    if (item.status == ResolutionStatus::resolved) {
                        ++report.newly_resolved;
                        add_count(report.base_register_counts, register_name(item.base_register));
                    } else ++report.still_unresolved;
                    report.items.push_back(std::move(item));
                    seen_items.emplace(address, block.start, reference->register_index);
                }
            }
            const auto transfer = recognize_transfer(*instruction);
            apply_transfer(state, transfer, address, block.start);
            if (instruction->flow == FlowKind::direct_call || instruction->flow == FlowKind::indirect_call)
                clear_state(state);
        }
        for (const auto& successor : successors[block.start]) {
            const auto next = std::find_if(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                                           [&](const auto& candidate) { return candidate.start == successor.first; });
            if (next == slice.basic_blocks.end()) continue;
            const auto next_index = static_cast<std::size_t>(std::distance(slice.basic_blocks.begin(), next));
            State merged = state;
            if (edge_kind(successor) == static_cast<std::uint32_t>(FlowKind::direct_call)) clear_state(merged);
            if (!incoming[next_index]) {
                incoming[next_index] = merged;
                worklist.push_back(next_index);
            } else {
                const auto combined = merge_state(*incoming[next_index], merged);
                if (!same_value(*incoming[next_index], combined)) {
                    incoming[next_index] = combined;
                    worklist.push_back(next_index);
                }
            }
        }
    }
    // Preserve candidates in the Atlas even when the bounded CFG did not
    // reach their block. They remain unresolved evidence, never silently drop.
    for (const auto& [address, references] : candidates) {
        const auto instruction = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                               [=](const auto& item) { return item.address == address; });
        for (const auto* reference : references) {
            if (seen_items.contains({address, reference->block_start, reference->register_index})) continue;
            ResolutionItem item;
            item.instruction_address = address;
            item.block_start = reference->block_start;
            item.base_register = reference->register_index;
            if (instruction == slice.instructions.end()) {
                item.status = ResolutionStatus::unresolved_unsupported_transfer;
                item.reason = "instruction absent from bounded slice";
            } else {
                const auto offset = displacement_offset(*instruction, *reference);
                if (offset && *offset + 1U < instruction->bytes.size())
                    item.displacement = static_cast<std::int16_t>(read16(instruction->bytes, *offset));
                item.status = ResolutionStatus::unresolved_unknown_base;
                item.reason = "bounded CFG path not reached";
            }
            add_count(report.reason_counts, resolution_status_name(item.status));
            ++report.still_unresolved;
            report.items.push_back(std::move(item));
        }
    }
    // A bounded CFG can be reached by more than one path. Keep report order
    // tied to instruction address and register, independent of worklist order.
    std::sort(report.items.begin(), report.items.end(), [](const auto& left, const auto& right) {
        return std::tie(left.instruction_address, left.base_register) < std::tie(right.instruction_address, right.base_register);
    });
    sort_counts(report.reason_counts);
    sort_counts(report.base_register_counts);
    make_ranges(report);
    return report;
}

const AtlasEntry* find_entry(const AtlasReport& atlas, std::uint32_t entry) {
    const auto found = std::find_if(atlas.entries.begin(), atlas.entries.end(),
                                    [=](const auto& item) { return item.start == entry; });
    return found == atlas.entries.end() ? nullptr : &*found;
}

std::size_t ranking_value(const AtlasRankingReport& ranking, const std::string& dimension, const std::string& key) {
    const auto found = std::find_if(ranking.groups.begin(), ranking.groups.end(), [&](const auto& item) {
        return item.dimension == dimension && item.key == key;
    });
    return found == ranking.groups.end() ? 0U : found->potentially_resolvable_refs;
}

} // namespace

ResolutionReport resolve_decoded_displacements(const AtlasEntry& atlas_entry, const DecodedSlice& slice) {
    return resolve_slice(atlas_entry, slice);
}

ResolutionReport resolve_bounded_displacements(const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom) {
    const auto* entry = find_entry(atlas, kTargetEntry);
    if (!entry) throw std::invalid_argument("Atlas does not contain bounded 0x60004 entry");
    const auto slice = decode_m68k_slice(retail_rom, {.entry = kTargetEntry, .byte_budget = 0x1200U});
    auto report = resolve_slice(*entry, slice);
    const auto before = rank_atlas_unresolved(atlas);
    auto after_atlas = atlas;
    auto* after_entry = const_cast<AtlasEntry*>(find_entry(after_atlas, kTargetEntry));
    if (!after_entry) throw std::logic_error("Atlas copy lost bounded entry");
    for (const auto& item : report.items) {
        if (item.status != ResolutionStatus::resolved) continue;
        const auto found = std::find_if(after_entry->unresolved_references.begin(), after_entry->unresolved_references.end(),
                                        [&](const auto& candidate) {
            return candidate.instruction_address == item.instruction_address &&
                candidate.mode == 5U && candidate.register_index == item.base_register;
        });
        if (found != after_entry->unresolved_references.end()) after_entry->unresolved_references.erase(found);
    }
    after_entry->unresolved_reference_count = after_entry->unresolved_references.size();
    const auto after = rank_atlas_unresolved(after_atlas);
    report.atlas_unresolved_before = before.atlas_unresolved_reference_count;
    report.atlas_unresolved_after = after.atlas_unresolved_reference_count;
    const std::vector<std::pair<std::string, std::string>> tracked{
        {"addressing_mode", "address_displacement"},
        {"register", "A6"},
        {"likely_constant_propagation_candidate", "instruction_has_immediate"},
    };
    for (const auto& [dimension, key] : tracked) {
        const auto before_value = ranking_value(before, dimension, key);
        const auto after_value = ranking_value(after, dimension, key);
        report.ranking_delta.push_back({dimension, key, before_value, after_value,
                                        static_cast<std::int64_t>(after_value) - static_cast<std::int64_t>(before_value)});
    }
    return report;
}

std::string resolution_status_name(ResolutionStatus status) {
    switch (status) {
    case ResolutionStatus::resolved: return "resolved";
    case ResolutionStatus::unresolved_unknown_base: return "unresolved_unknown_base";
    case ResolutionStatus::unresolved_conflicting_base: return "unresolved_conflicting_base";
    case ResolutionStatus::unresolved_unsupported_transfer: return "unresolved_unsupported_transfer";
    case ResolutionStatus::unresolved_cfg_merge: return "unresolved_cfg_merge";
    }
    return "unresolved_unknown_base";
}

std::string effective_address_class_name(EffectiveAddressClass address_class) {
    switch (address_class) {
    case EffectiveAddressClass::rom: return "rom";
    case EffectiveAddressClass::ram: return "ram";
    case EffectiveAddressClass::outside_known_address_space: return "outside_known_address_space";
    case EffectiveAddressClass::unknown: return "unknown";
    }
    return "unknown";
}

} // namespace oasis::tools
