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
        if (block.start == pc) return &block;
    return nullptr;
}

std::size_t BasicBlockRegistry::index_of(unsigned pc) const {
    for (std::size_t i = 0; i < metrics_.per_block.size(); ++i)
        if (metrics_.per_block[i].target == pc) return i;
    return metrics_.per_block.size();
}

bool BasicBlockRegistry::in_registered_range(unsigned address) const {
    for (const auto& block : generated::blocks())
        if (address >= block.start && address < block.end) return true;
    return false;
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict(
    const GeneratedBlockSpec& block, const State& entry) const {
    return predict_generated_block(api_, block, entry);
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

void BasicBlockRegistry::start_shadow(unsigned pc) {
    active_pc_ = pc;
    active_block_ = find(pc);
    if (!active_block_) throw std::runtime_error("unregistered basic block");
    prediction_ = predict(*active_block_, state());
    writes_.clear(); reads_.clear(); active_ = true; shadow_ = true;
    instruction_exit_seen_ = false; instruction_exits_ = 0;
}

void BasicBlockRegistry::finish_shadow() {
    compare(prediction_); ++metrics_.shadow_comparisons;
    ++metrics_.per_block.at(index_of(active_pc_)).shadow_comparisons;
    active_ = false; shadow_ = false; instruction_exit_seen_ = false; instruction_exits_ = 0;
    active_block_ = nullptr;
}

void BasicBlockRegistry::finish_native(const Prediction& prediction) {
    compare(prediction); active_ = false; shadow_ = false; instruction_exit_seen_ = false;
    instruction_exits_ = 0; active_block_ = nullptr;
}

void BasicBlockRegistry::execute(const GeneratedBlockSpec& block) {
    active_pc_ = block.start; active_block_ = &block;
    prediction_ = predict(block, state());
    writes_.clear(); reads_.clear(); active_ = true; shadow_ = false;
    block.execute(api_);
    finish_native(prediction_); ++metrics_.translated_entries;
    ++metrics_.per_block.at(index_of(block.start)).translated_entries;
    metrics_.translated_instructions += block.instruction_count;
    ++metrics_.translated_blocks;
    log_ << "{\"block\":\"0x" << std::hex << block.start << std::dec
         << "\",\"translated_instructions\":" << block.instruction_count
         << ",\"timing_exact\":true}\n";
}

int BasicBlockRegistry::dispatch(unsigned pc) noexcept {
    if (!error_.empty()) return 0;
    try {
        if (active_ && shadow_ && pc == prediction_.state[16])
            finish_shadow();
        const auto* block = find(pc);
        if (!block) return 0;
        const auto index = index_of(pc);
        if (active_) {
            std::ostringstream nested;
            nested << "nested translated block pc=0x" << std::hex << pc
                   << " expected=0x" << prediction_.state[16];
            throw std::runtime_error(nested.str());
        }
        ++metrics_.natural_entries;
        ++metrics_.per_block.at(index).natural_entries;
        if (mode_ == BasicBlockMode::SHADOW_NATIVE) { start_shadow(pc); return 0; }
        execute(*block); return 1;
    } catch (const std::exception& error) { fail(error.what()); return 0; }
}

void BasicBlockRegistry::event(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        address &= 0xFFFFFFU;
        if (type == 1) {
            if (shadow_ && in_block(address)) ++metrics_.original_starts_inside_translated;
            if (in_registered_range(address) && !in_block(address)) ++metrics_.fallback_entries;
            return;
        }
        if (type == (1 << 14)) {
            instruction_exit_seen_ = true;
            ++instruction_exits_;
            if (shadow_ && active_block_ &&
                instruction_exits_ >= active_block_->instruction_count) finish_shadow();
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
