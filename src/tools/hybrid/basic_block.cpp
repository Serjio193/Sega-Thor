#include "tools/hybrid/basic_block.hpp"
#include "tools/hybrid/basic_block_reference.hpp"
#include "tools/hybrid/generated_blocks.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kRamBase = 0xFF0000;

std::uint32_t read_be(const BasicBlockApi& api, unsigned address, int width) {
    std::uint32_t value = 0;
    for (int i = 0; i < width; ++i) {
        const auto byte = api.peek(address + static_cast<unsigned>(i));
        if (byte < 0 || byte > 0xFF) throw std::runtime_error("basic-block peek failed");
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return value;
}

int dispatch_result(BlockExitReason reason) {
    switch (reason) {
    case BlockExitReason::CONTINUE_BLOCK:
    case BlockExitReason::NORMAL_EXIT: return 1;
    case BlockExitReason::EVENT_BOUNDARY: return 2;
    case BlockExitReason::INTERRUPT_BOUNDARY: return 3;
    case BlockExitReason::TRACE_BOUNDARY: return 4;
    case BlockExitReason::FALLBACK: return 0;
    }
    return 0;
}

} // namespace

BasicBlockRegistry::BasicBlockRegistry(BasicBlockApi api, BasicBlockMode mode,
                                       std::ostream& log)
    : api_(api), mode_(mode), log_(log) {
    for (const auto& block : generated::blocks())
        metrics_.per_block.push_back({block.start, block.end, block.instruction_count});
}

BasicBlockRegistry::State BasicBlockRegistry::state() const {
    State result{};
    for (unsigned i = 0; i < result.size(); ++i) result[i] = api_.reg(i);
    return result;
}

unsigned BasicBlockRegistry::field(unsigned index) const {
    return api_.cpu_field ? api_.cpu_field(index) : 0;
}

BasicBlockRegistry::Prediction BasicBlockRegistry::capture() const {
    Prediction result{};
    result.state = state();
    result.ir = field(18);
    result.pref_addr = field(19);
    result.pref_data = field(20);
    result.cycles = field(21);
    result.refresh = field(22);
    result.cycle_end = field(23);
    result.interrupt_level = field(24);
    result.interrupt_mask = field(25);
    result.tracing = field(26);
    result.stopped = field(27);
    return result;
}

int BasicBlockRegistry::peek(unsigned address) const {
    const auto value = api_.peek(address);
    if (value < 0 || value > 0xFF) throw std::runtime_error("basic-block memory peek failed");
    return value;
}

unsigned BasicBlockRegistry::read(unsigned address, int width) const {
    if (api_.read) return api_.read(address, width);
    return read_be(api_, address, width);
}

void BasicBlockRegistry::write(unsigned address, int width, unsigned value) const {
    require(api_.write != nullptr, "basic-block write bridge unavailable");
    api_.write(address, width, value);
}

void BasicBlockRegistry::require(bool condition, const std::string& message) const {
    if (!condition) throw std::runtime_error(message);
}

void BasicBlockRegistry::fail(const std::string& message) {
    ++metrics_.divergences;
    std::ostringstream out;
    out << "FIRST_DIVERGENCE block=0x" << std::hex << active_pc_ << std::dec << ' ' << message;
    error_ = out.str();
    active_ = false;
}

bool BasicBlockRegistry::in_block(unsigned pc) const {
    return active_block_ && pc >= active_block_->start && pc < active_block_->end;
}

const GeneratedBlockSpec* BasicBlockRegistry::find(unsigned pc) const {
    for (const auto& block : generated::blocks())
        if (pc >= block.start && pc < block.end &&
            block.instruction_count_from_entry &&
            block.instruction_count_from_entry(pc)) return &block;
    return nullptr;
}

std::size_t BasicBlockRegistry::index_of(unsigned pc) const {
    const auto* block = find(pc);
    if (!block) return metrics_.per_block.size();
    for (std::size_t i = 0; i < metrics_.per_block.size(); ++i)
        if (metrics_.per_block[i].target == block->start) return i;
    return metrics_.per_block.size();
}

bool BasicBlockRegistry::in_registered_range(unsigned address) const {
    for (const auto& block : generated::blocks())
        if (address >= block.start && address < block.end) return true;
    return false;
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict(
    const GeneratedBlockSpec& block, const Prediction& entry,
    unsigned entry_pc, unsigned instruction_limit) const {
    return predict_generated_block(api_, block, entry, entry_pc, instruction_limit);
}

void BasicBlockRegistry::compare(const Prediction& prediction) {
    const auto actual = state();
    for (unsigned i = 0; i < actual.size(); ++i) {
        if (actual[i] != prediction.state[i]) {
            std::ostringstream register_error;
            register_error << "register[" << i << "] actual=0x" << std::hex << actual[i]
                           << " expected=0x" << prediction.state[i];
            throw std::runtime_error(register_error.str());
        }
    }
    if (field(18) != prediction.ir) {
        std::ostringstream ir;
        ir << "IR actual=0x" << std::hex << field(18) << " expected=0x" << prediction.ir;
        throw std::runtime_error(ir.str());
    }
    if (field(19) != prediction.pref_addr || field(20) != prediction.pref_data) {
        std::ostringstream prefetch;
        prefetch << "prefetch actual_addr=0x" << std::hex << field(19)
                 << " expected_addr=0x" << prediction.pref_addr
                 << " actual_data=0x" << field(20)
                 << " expected_data=0x" << prediction.pref_data;
        throw std::runtime_error(prefetch.str());
    }
    if (field(21) != prediction.cycles || field(22) != prediction.refresh) {
        std::ostringstream timing;
        timing << "timing actual_cycles=" << field(21) << " expected_cycles=" << prediction.cycles
               << " actual_refresh=" << field(22) << " expected_refresh=" << prediction.refresh;
        throw std::runtime_error(timing.str());
    }
    require(field(23) == prediction.cycle_end && field(24) == prediction.interrupt_level &&
                field(25) == prediction.interrupt_mask && field(26) == prediction.tracing &&
                field(27) == prediction.stopped,
            "boundary state");
    require(writes_.size() == prediction.writes.size(), "write count");
    for (std::size_t i = 0; i < writes_.size(); ++i)
        require(writes_[i].address == prediction.writes[i].address &&
                writes_[i].width == prediction.writes[i].width && writes_[i].value == prediction.writes[i].value,
                "write[" + std::to_string(i) + "]");
    require(reads_.size() == prediction.reads.size(), "read count");
    for (std::size_t i = 0; i < reads_.size(); ++i)
        require(reads_[i].address == prediction.reads[i].address && reads_[i].width == prediction.reads[i].width,
                "read[" + std::to_string(i) + "]");
}

void BasicBlockRegistry::compare_shadow_boundary() {
    compare(prediction_);
    ++metrics_.shadow_comparisons;
    ++metrics_.per_block.at(index_of(active_pc_)).shadow_comparisons;
}

BlockExitReason BasicBlockRegistry::boundary_reason() const {
    return api_.boundary_reason ? api_.boundary_reason() : BlockExitReason::CONTINUE_BLOCK;
}

void BasicBlockRegistry::record_yield(BlockExitReason reason) {
    if (reason == BlockExitReason::CONTINUE_BLOCK || reason == BlockExitReason::NORMAL_EXIT)
        return;
    ++metrics_.boundary_yields;
    if (active_block_) ++metrics_.per_block.at(index_of(active_pc_)).boundary_yields;
    if (reason == BlockExitReason::EVENT_BOUNDARY) {
        ++metrics_.event_boundary_yields;
        if (active_block_) ++metrics_.per_block.at(index_of(active_pc_)).event_boundary_yields;
    }
    if (reason == BlockExitReason::INTERRUPT_BOUNDARY) {
        ++metrics_.interrupt_boundary_yields;
        if (active_block_) ++metrics_.per_block.at(index_of(active_pc_)).interrupt_boundary_yields;
    }
    if (reason == BlockExitReason::TRACE_BOUNDARY) {
        ++metrics_.trace_boundary_yields;
        if (active_block_) ++metrics_.per_block.at(index_of(active_pc_)).trace_boundary_yields;
    }
    if (active_block_ && instructions_remaining_ > 1 &&
        prediction_.state[16] >= active_block_->start &&
        prediction_.state[16] < active_block_->end) {
        awaiting_interrupt_ = true;
        continuation_pc_ = prediction_.state[16];
        continuation_block_ = active_block_->start;
    }
}

void BasicBlockRegistry::finish_shadow() {
    active_ = false;
    shadow_ = false;
    instruction_exit_seen_ = false;
    entry_event_pending_ = false;
    instruction_exits_ = 0;
    instructions_remaining_ = 0;
    active_block_ = nullptr;
    expected_entry_pc_ = 0;
}

void BasicBlockRegistry::start_shadow(unsigned pc) {
    active_block_ = find(pc);
    if (!active_block_) throw std::runtime_error("unregistered basic block");
    active_pc_ = active_block_->start;
    active_entry_pc_ = pc;
    instructions_remaining_ = active_block_->instruction_count_from_entry(pc);
    prediction_ = predict(*active_block_, capture(), pc, 1);
    expected_entry_pc_ = prediction_.state[16];
    writes_.clear(); reads_.clear(); active_ = true; shadow_ = true;
    entry_event_pending_ = true;
    instruction_exit_seen_ = false; instruction_exits_ = 0;
}

void BasicBlockRegistry::finish_native(const Prediction& prediction) {
    compare(prediction); active_ = false; shadow_ = false; instruction_exit_seen_ = false;
    entry_event_pending_ = false; instruction_exits_ = 0; instructions_remaining_ = 0;
    active_block_ = nullptr;
}

BlockExit BasicBlockRegistry::execute(const GeneratedBlockSpec& block, unsigned entry_pc) {
    active_pc_ = block.start; active_entry_pc_ = entry_pc; active_block_ = &block;
    instructions_remaining_ = block.instruction_count_from_entry(entry_pc);
    const auto entry = capture();
    writes_.clear(); reads_.clear(); active_ = true; shadow_ = false;
    const auto exit = block.execute(api_, entry_pc);
    if (exit.reason == BlockExitReason::FALLBACK || !exit.instructions_executed)
        throw std::runtime_error("generated block continuation unavailable");
    prediction_ = predict(block, entry, entry_pc,
                          exit.reason == BlockExitReason::NORMAL_EXIT ? 0 :
                          exit.instructions_executed);
    record_yield(exit.reason);
    finish_native(prediction_); ++metrics_.translated_entries;
    ++metrics_.per_block.at(index_of(block.start)).translated_entries;
    metrics_.translated_instructions += exit.instructions_executed;
    if (block.instruction_count > 1) ++metrics_.translated_multi_instruction_entries;
    ++metrics_.translated_blocks;
    log_ << "{\"block\":\"0x" << std::hex << block.start << std::dec
         << "\",\"translated_instructions\":" << exit.instructions_executed
         << ",\"reason\":" << static_cast<unsigned>(exit.reason)
         << ",\"timing_exact\":true}\n";
    return exit;
}

int BasicBlockRegistry::dispatch(unsigned pc) noexcept {
    if (!error_.empty()) return 0;
    try {
        const auto* block = find(pc);
        if (!block) return 0;
        const auto index = index_of(pc);
        if (active_) {
            if (shadow_ && pc == expected_entry_pc_ && in_block(pc)) return 0;
            std::ostringstream nested;
            nested << "nested translated block pc=0x" << std::hex << pc
                   << " expected=0x" << expected_entry_pc_
                   << " actual_pref=0x" << field(19) << "/0x" << field(20)
                   << " predicted_pref=0x" << prediction_.pref_addr << "/0x"
                   << prediction_.pref_data;
            throw std::runtime_error(nested.str());
        }
        ++metrics_.natural_entries;
        ++metrics_.per_block.at(index).natural_entries;
        if (mode_ == BasicBlockMode::SHADOW_NATIVE) { start_shadow(pc); return 0; }
        const auto exit = execute(*block, pc);
        if (interrupt_after_yield_ && pc == continuation_pc_ &&
            block->start == continuation_block_) {
            ++metrics_.interrupted_resumptions;
            ++metrics_.per_block.at(index).interrupted_resumptions;
            interrupt_after_yield_ = false;
            awaiting_interrupt_ = false;
        } else if (awaiting_interrupt_ && pc == continuation_pc_) {
            awaiting_interrupt_ = false;
        }
        return dispatch_result(exit.reason);
    } catch (const std::exception& error) { fail(error.what()); return 0; }
}

void BasicBlockRegistry::event(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        address &= 0xFFFFFFU;
        if (type == 1) {
            if (shadow_ && entry_event_pending_) {
                if (address != active_entry_pc_)
                    throw std::runtime_error("basic-block entry event mismatch");
                entry_event_pending_ = false;
                return;
            }
            if (shadow_ && in_block(address)) ++metrics_.original_starts_inside_translated;
            if (in_registered_range(address) && !in_block(address)) ++metrics_.fallback_entries;
            return;
        }
        if (type == (1 << 15)) {
            ++metrics_.interrupts;
            if (awaiting_interrupt_) interrupt_after_yield_ = true;
            return;
        }
        if (type == (1 << 14)) {
            instruction_exit_seen_ = true;
            ++instruction_exits_;
            if (shadow_ && active_block_) {
                compare_shadow_boundary();
                --instructions_remaining_;
                const auto reason = boundary_reason();
                if (reason != BlockExitReason::CONTINUE_BLOCK || !instructions_remaining_) {
                    record_yield(reason);
                    finish_shadow();
                } else {
                    const auto next_entry_pc = prediction_.state[16];
                    prediction_ = predict(*active_block_, prediction_, next_entry_pc, 1);
                    expected_entry_pc_ = next_entry_pc;
                    writes_.clear();
                    reads_.clear();
                }
            }
            return;
        }
        if (!active_) return;
        if (type == 2) {
            if (address < kRamBase && address >= 0xA00000U) ++metrics_.hardware_accesses;
            reads_.push_back({address, width}); return;
        }
        if (type == 4) {
            if (address < kRamBase && address >= 0xA00000U) ++metrics_.hardware_accesses;
            writes_.push_back({address, width, value}); return;
        }
    } catch (const std::exception& error) { fail(error.what()); }
}

} // namespace oasis::hybrid
