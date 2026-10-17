#include "tools/hybrid/m68k_call_census.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid::call_census {
namespace {
constexpr unsigned address_mask = 0xFFFFFFU;
constexpr unsigned event_read = 2U;
constexpr unsigned event_write = 4U;
constexpr std::size_t max_instructions = 100;
constexpr std::size_t max_events = 4096;
constexpr std::size_t max_captures_per_target = 24;

int sign_extend(unsigned value, unsigned bits) {
    const auto sign = 1U << (bits - 1U);
    return static_cast<int>((value ^ sign) - sign);
}

bool is_bsr(unsigned opcode) { return (opcode & 0xFF00U) == 0x6100U; }
bool is_jsr(unsigned opcode) { return (opcode & 0xFFC0U) == 0x4E80U; }
bool is_indirect_control(unsigned opcode) {
    if ((opcode & 0xFFC0U) != 0x4E80U && (opcode & 0xFFC0U) != 0x4EC0U)
        return false;
    const auto mode = (opcode >> 3U) & 7U;
    const auto reg = opcode & 7U;
    return mode != 7U || reg > 1U;
}

bool is_supported_call(unsigned opcode) {
    return is_bsr(opcode) || opcode == 0x4EB8U || opcode == 0x4EB9U ||
           (is_jsr(opcode) && ((opcode >> 3U) & 7U) == 2U);
}

std::string hex(unsigned value, unsigned width = 6) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(static_cast<int>(width))
        << std::setfill('0') << value;
    return out.str();
}

void write_cpu(std::ostream& out, const CpuState& state) {
    out << "{\"pc\":" << state.pc << ",\"sr\":" << state.sr << ",\"d\":[";
    for (std::size_t i = 0; i < state.d.size(); ++i)
        out << (i ? "," : "") << state.d[i];
    out << "],\"a\":[";
    for (std::size_t i = 0; i < state.a.size(); ++i)
        out << (i ? "," : "") << state.a[i];
    out << "]}";
}
} // namespace

std::string format_hex(unsigned value, unsigned width) { return hex(value, width); }

void M68kCallCensus::set_rom(const std::uint8_t* rom, std::size_t rom_size) noexcept {
    rom_ = rom; rom_size_ = rom_size;
}

void M68kCallCensus::on_hook(unsigned type, int width, unsigned address, unsigned value,
                             unsigned previous_pc, unsigned frame, RegisterRead read_register,
                             MemoryPeek peek_memory) {
    const auto word = [&](unsigned at) {
        at &= address_mask;
        if (at + 1U < rom_size_)
            return (static_cast<unsigned>(rom_[at]) << 8U) | rom_[at + 1U];
        const auto hi = peek_memory(at), lo = peek_memory((at + 1U) & address_mask);
        return hi < 0 || lo < 0 ? 0U :
            (static_cast<unsigned>(hi & 0xFF) << 8U) | static_cast<unsigned>(lo & 0xFF);
    };
    if (type & 1U) {
        const auto source_opcode = word(previous_pc);
        const bool candidate = is_supported_call(source_opcode);
        if (!candidate && active_.empty()) return;
        CpuState state;
        for (unsigned i = 0; i < 8; ++i) {
            state.d[i] = read_register(i);
            state.a[i] = read_register(i + 8U);
        }
        state.pc = read_register(16) & address_mask;
        state.sr = read_register(17) & 0xFFFFU;
        const auto current_opcode = active_.empty() && !candidate ? 0U : word(address);
        const auto extension = candidate ? word(previous_pc + 2U) : 0U;
        const auto long_extension = candidate ?
            (word(previous_pc + 2U) << 16U) | word(previous_pc + 4U) : 0U;
        unsigned return_address{};
        if (candidate && state.a[7] <= address_mask - 3U) {
            const auto a7 = state.a[7] & address_mask;
            const auto b0 = peek_memory(a7), b1 = peek_memory(a7 + 1U);
            const auto b2 = peek_memory(a7 + 2U), b3 = peek_memory(a7 + 3U);
            if (b0 >= 0 && b1 >= 0 && b2 >= 0 && b3 >= 0)
                return_address = ((static_cast<unsigned>(b0) << 24U) |
                    (static_cast<unsigned>(b1) << 16U) |
                    (static_cast<unsigned>(b2) << 8U) | static_cast<unsigned>(b3)) & address_mask;
        }
        execute(address, previous_pc, frame, state, current_opcode, source_opcode,
                extension, long_extension, return_address);
    } else if (!active_.empty() && (type & (event_read | event_write))) {
        memory(type, read_register(16) & address_mask, address,
               static_cast<unsigned>(width), value);
    }
}

bool decode_transfer(unsigned source_pc, unsigned target_pc,
                     const CpuState& target_state, unsigned opcode,
                     unsigned extension, unsigned long_extension,
                     Transfer& out) {
    source_pc &= address_mask;
    target_pc &= address_mask;
    unsigned length{};
    unsigned destination{};
    std::string kind;
    if (is_bsr(opcode)) {
        const auto displacement = opcode & 0xFFU;
        if (displacement == 0) {
            length = 4;
            destination = (source_pc + 2U + static_cast<unsigned>(sign_extend(extension, 16))) & address_mask;
            kind = "BSR.W";
        } else {
            length = 2;
            destination = (source_pc + 2U + static_cast<unsigned>(sign_extend(displacement, 8))) & address_mask;
            kind = "BSR.B";
        }
    } else if (opcode == 0x4EB9U) {
        length = 6; destination = long_extension & address_mask; kind = "JSR_ABS.L";
    } else if (opcode == 0x4EB8U) {
        length = 4; destination = static_cast<unsigned>(sign_extend(extension, 16)) & address_mask;
        kind = "JSR_ABS.W";
    } else if (is_jsr(opcode) && ((opcode >> 3U) & 7U) == 2U) {
        length = 2; destination = target_state.a[opcode & 7U] & address_mask;
        kind = "JSR_(An)";
    } else {
        return false;
    }
    if (destination != target_pc) return false;
    out = {source_pc, destination, (source_pc + length) & address_mask, kind, opcode};
    return true;
}

void M68kCallCensus::execute(unsigned pc, unsigned previous_pc, unsigned frame,
                             const CpuState& state, unsigned current_opcode,
                             unsigned source_opcode, unsigned source_extension,
                             unsigned source_long_extension, unsigned return_address) {
    pc &= address_mask;
    for (std::size_t i = active_.size(); i > 0; --i) {
        auto& active = active_[i - 1];
        if (pc == active.transfer.return_pc && state.a[7] == active.entry_sp + 4U) {
            active.exit = state;
            auto& stats = stats_[active.transfer.target_pc];
            ++stats.completed_returns;
            if (active.saw_rts && !active.nested_call && !active.indirect_control &&
                !active.overflow && active.instructions <= max_instructions &&
                active.events.size() <= max_events && stats.full_captures < max_captures_per_target) {
                ++stats.full_captures;
                captures_.push_back(active);
            }
            active_.erase(active_.begin() + static_cast<std::ptrdiff_t>(i - 1), active_.end());
        }
    }

    for (auto& active : active_) {
        if (++active.instructions > max_instructions || active.pc_path.size() >= max_instructions) {
            active.overflow = true;
        } else {
            active.pc_path.push_back(pc);
        }
        if (is_indirect_control(current_opcode)) active.indirect_control = true;
        if (current_opcode == 0x4E75U) { active.rts_pre = state; active.saw_rts = true; }
    }

    Transfer transfer;
    if (!decode_transfer(previous_pc, pc, state, source_opcode, source_extension,
                         source_long_extension, transfer)) return;

    auto& stats = stats_[pc];
    if (stats.calls == 0) stats.first_frame = frame;
    stats.last_frame = frame;
    ++stats.calls;
    ++stats.callers[transfer.source_pc];
    if (pc == 0x00E268U && transfer.source_pc == 0x00359AU)
        ++stats.selected_caller_frame_calls[frame];
    ++stats.kinds[transfer.kind];
    if (return_address == transfer.return_pc) ++stats.stack_return_matches;
    else ++stats.stack_return_mismatches;
    for (std::size_t i = 0; i < 8; ++i) {
        stats.register_values[i].insert(state.d[i]);
        stats.register_values[i + 8].insert(state.a[i]);
    }
    stats.sr_values.insert(state.sr);
    for (auto& active : active_) active.nested_call = true;
    if (return_address != transfer.return_pc || state.a[7] < 4U) return;
    Active active;
    active.transfer = transfer;
    active.frame = frame;
    active.entry_sp = state.a[7];
    active.entry = state;
    active.instructions = 1;
    active.pc_path.push_back(pc);
    active.indirect_control = is_indirect_control(current_opcode);
    active.nested_call = is_bsr(current_opcode) || is_jsr(current_opcode);
    if (current_opcode == 0x4E75U) { active.rts_pre = state; active.saw_rts = true; }
    active_.push_back(std::move(active));
}

void M68kCallCensus::memory(unsigned type, unsigned pc, unsigned address,
                            unsigned width, unsigned value) {
    for (auto& active : active_) {
        auto& stats = stats_[active.transfer.target_pc];
        if ((type & event_write) && address >= 0xFF0000U) ++stats.ram_writes;
        if ((type & event_read) && address < 0x300000U) ++stats.rom_reads;
        if (active.events.size() < max_events)
            active.events.push_back({type, pc, address, width, value});
        else active.overflow = true;
    }
}

void M68kCallCensus::write(const std::filesystem::path& census_path,
                           const std::filesystem::path& captures_path) const {
    std::ofstream census(census_path, std::ios::trunc);
    std::ofstream captures(captures_path, std::ios::trunc);
    if (!census || !captures) throw std::runtime_error("call census output open failed");
    census << "{\"schema\":\"oasis.m68k.replay-call-census.v1\","
           << "\"MEMORY_COUNTS_ARE_INCLUSIVE\":true,\"targets\":[";
    bool first_target = true;
    for (const auto& [target, stats] : stats_) {
        census << (first_target ? "" : ","); first_target = false;
        census << "{\"ENTRY_PC\":\"" << hex(target) << "\",\"CALL_COUNT\":" << stats.calls
               << ",\"HAS_FULL_CALL_CAPTURE\":" << (stats.full_captures ? "true" : "false")
               << ",\"UNIQUE_CALLER_PCS\":[";
        bool first = true;
        for (const auto& [caller, count] : stats.callers) {
            census << (first ? "" : ",") << "{\"pc\":\"" << hex(caller)
                   << "\",\"count\":" << count << '}'; first = false;
        }
        census << ']';
        if (target == 0x00E268U) {
            census << ",\"CALLER_00359A_BY_FRAME\":[";
            bool first_frame = true;
            for (const auto& [frame, count] : stats.selected_caller_frame_calls) {
                census << (first_frame ? "" : ",") << "{\"frame\":" << frame
                       << ",\"count\":" << count << '}';
                first_frame = false;
            }
            census << ']';
        }
        census << ",\"TRANSFER_KINDS\":{";
        first = true;
        for (const auto& [kind, count] : stats.kinds) {
            census << (first ? "" : ",") << '"' << kind << "\":" << count; first = false;
        }
        census << "},\"FIRST_FRAME\":" << stats.first_frame << ",\"LAST_FRAME\":" << stats.last_frame
               << ",\"STACK_RETURN_MATCH_COUNT\":" << stats.stack_return_matches
               << ",\"STACK_RETURN_MISMATCH_COUNT\":" << stats.stack_return_mismatches
               << ",\"COMPLETED_RETURNS\":" << stats.completed_returns
               << ",\"FULL_CALL_CAPTURE_COUNT\":" << stats.full_captures
               << ",\"REGISTER_VARIATION\":{";
        for (std::size_t i = 0; i < stats.register_values.size(); ++i) {
            if (i) census << ',';
            census << '"' << (i < 8 ? "D" : "A") << (i % 8) << "\":"
                   << stats.register_values[i].size();
        }
        census << "},\"SR_VARIATION\":" << stats.sr_values.size()
               << ",\"RAM_WRITE_COUNT\":" << stats.ram_writes
               << ",\"ROM_READ_COUNT\":" << stats.rom_reads << '}';
    }
    census << "]}\n";
    std::map<unsigned, std::size_t> capture_ids;
    for (const auto& capture : captures_) {
        const auto id = ++capture_ids[capture.transfer.target_pc];
        captures << "{\"capture_id\":\"" << hex(capture.transfer.target_pc)
                 << "#" << id << "\",\"source_pc\":\"" << hex(capture.transfer.source_pc)
                 << "\",\"target_pc\":\"" << hex(capture.transfer.target_pc)
                 << "\",\"transfer_kind\":\"" << capture.transfer.kind
                 << "\",\"opcode\":\"" << hex(capture.transfer.opcode, 4)
                 << "\",\"frame\":" << capture.frame << ",\"return_pc\":\""
                 << hex(capture.transfer.return_pc) << "\",\"entry\":";
        write_cpu(captures, capture.entry);
        captures << ",\"rts_pre\":"; write_cpu(captures, capture.rts_pre);
        captures << ",\"return_state\":"; write_cpu(captures, capture.exit);
        captures << ",\"instruction_count\":" << capture.instructions << ",\"pc_path\":[";
        for (std::size_t i = 0; i < capture.pc_path.size(); ++i)
            captures << (i ? "," : "") << capture.pc_path[i];
        captures << "],\"memory_events\":[";
        for (std::size_t i = 0; i < capture.events.size(); ++i) {
            const auto& event = capture.events[i];
            captures << (i ? "," : "") << "{\"type\":" << event.type
                     << ",\"pc\":" << event.pc << ",\"address\":" << event.address
                     << ",\"width\":" << event.width << ",\"value\":" << event.value << '}';
        }
        captures << "]}\n";
    }
}

std::size_t M68kCallCensus::target_count() const noexcept { return stats_.size(); }
std::size_t M68kCallCensus::full_capture_count() const noexcept { return captures_.size(); }
bool M68kCallCensus::has_active() const noexcept { return !active_.empty(); }

} // namespace oasis::hybrid::call_census
