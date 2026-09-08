#include "tools/hybrid/basic_block.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned k2D66 = 0x2D66;
constexpr unsigned k2D66Exit = 0x2D7A;
constexpr unsigned k604BC = 0x604BC;
constexpr unsigned k604BCExit = 0x604C2;
constexpr unsigned k61032 = 0x61032;
constexpr unsigned k61032Exit = 0x61034;
constexpr unsigned kRamBase = 0xFF0000;
constexpr unsigned kRefreshPeriod = 128U * 7U;
constexpr unsigned kRefreshPenalty = 2U * 7U;

std::uint32_t read_be(const BasicBlockApi& api, unsigned address, int width) {
    std::uint32_t value = 0;
    for (int i = 0; i < width; ++i) {
        const auto byte = api.peek(address + static_cast<unsigned>(i));
        if (byte < 0 || byte > 0xFF) throw std::runtime_error("basic-block peek failed");
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return value;
}

void set_move_flags(std::uint32_t& sr, std::uint32_t value, unsigned width) {
    const auto mask = width == 1 ? 0x80U : width == 2 ? 0x8000U : 0x80000000U;
    const auto value_mask = width == 1 ? 0xFFU : width == 2 ? 0xFFFFU : 0xFFFFFFFFU;
    sr = (sr & ~0x0FU) | (value & value_mask ? 0U : 4U) |
         (value & mask ? 8U : 0U);
}

void set_add_long_flags(std::uint32_t& sr, std::uint32_t lhs,
                        std::uint32_t rhs, std::uint32_t result) {
    const auto carry = (static_cast<std::uint64_t>(lhs) + rhs) > 0xFFFFFFFFULL;
    const auto overflow = ((~(lhs ^ rhs) & (lhs ^ result)) & 0x80000000U) != 0;
    sr = (sr & ~0x1FU) | (carry ? 0x11U : 0U) |
         (result & 0x80000000U ? 0x08U : 0U) |
         (result == 0 ? 0x04U : 0U) | (overflow ? 0x02U : 0U);
}

void add_prediction_cycle(const BasicBlockApi& api, unsigned opcode,
                          unsigned& cycles, unsigned& refresh) {
    const auto period = api.refresh_period ? api.refresh_period() : kRefreshPeriod;
    const auto penalty = api.refresh_penalty ? api.refresh_penalty() : kRefreshPenalty;
    if (cycles >= refresh) {
        refresh = cycles + period;
        cycles += penalty;
    }
    cycles += api.instruction_cycles(opcode);
}

void add_write(BasicBlockRegistry::Prediction& result, unsigned address, int width,
               unsigned value) {
    result.writes.push_back({address, width, value});
}

void add_read(BasicBlockRegistry::Prediction& result, unsigned address, int width) {
    result.reads.push_back({address, width});
}

void predict_fetch(const BasicBlockApi& api, BasicBlockRegistry::Prediction& result,
                  unsigned& pc, unsigned opcode) {
    const auto actual = read_be(api, pc, 2);
    if (actual != opcode) {
        std::ostringstream message;
        message << "basic-block opcode mismatch pc=0x" << std::hex << pc
                << " expected=0x" << opcode << " actual=0x" << actual;
        throw std::runtime_error(message.str());
    }
    pc += 2;
    result.pref_addr = pc;
    result.pref_data = read_be(api, pc, 2);
    // timing is accumulated below using the same CPU table as GPGX
    add_prediction_cycle(api, opcode, result.cycles, result.refresh);
    result.ir = opcode;
}

void predict_extension16(const BasicBlockApi& api,
                         BasicBlockRegistry::Prediction& result, unsigned& pc) {
    (void)read_be(api, pc, 2);
    pc += 2;
    result.pref_addr = pc;
    result.pref_data = read_be(api, pc, 2);
}

} // namespace

BasicBlockRegistry::BasicBlockRegistry(BasicBlockApi api, BasicBlockMode mode,
                                       std::ostream& log)
    : api_(api), mode_(mode), log_(log) {}

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

unsigned BasicBlockRegistry::fetch() {
    require(api_.fetch16 != nullptr, "basic-block fetch bridge unavailable");
    return api_.fetch16();
}

void BasicBlockRegistry::step(unsigned opcode) {
    require(api_.begin_instruction && api_.finish_instruction,
            "basic-block timing bridge unavailable");
    api_.begin_instruction(opcode);
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
    if (active_pc_ == k2D66) return pc == k2D66 || pc == 0x2D6A || pc == 0x2D6C ||
        pc == 0x2D6E || pc == 0x2D74 || pc == 0x2D76 || pc == 0x2D78;
    return pc == active_pc_;
}

unsigned block_index(unsigned pc) {
    return pc == k2D66 ? 0U : pc == k604BC ? 1U : 2U;
}

bool BasicBlockRegistry::in_registered_range(unsigned address) const {
    return (address >= k2D66 && address < k2D66Exit) ||
           (address >= k604BC && address < k604BCExit) ||
           (address >= k61032 && address < k61032Exit);
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict(unsigned pc,
                                                            const State& entry) const {
    if (pc == k2D66) return predict_2d66(entry);
    if (pc == k604BC) return predict_604bc(entry);
    if (pc == k61032) return predict_61032(entry);
    throw std::runtime_error("unregistered basic block");
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict_2d66(const State& entry) const {
    Prediction result{};
    result.state = entry;
    result.cycles = field(21);
    result.refresh = field(22);
    auto pc = k2D66;
    predict_fetch(api_, result, pc, 0x48E7);
    predict_extension16(api_, result, pc);
    result.cycles += 2U * 8U * 7U;
    if (result.cycles >= result.refresh) result.refresh += kRefreshPeriod;
    auto sp = result.state[15] - 2; add_write(result, sp, 2, result.state[11] & 0xFFFFU);
    sp -= 2; add_write(result, sp, 2, result.state[11] >> 16U);
    sp -= 2; add_write(result, sp, 2, result.state[7] & 0xFFFFU);
    sp -= 2; add_write(result, sp, 2, result.state[7] >> 16U);
    result.state[15] = sp;
    predict_fetch(api_, result, pc, 0x4247);
    result.state[7] &= 0xFFFF0000U; result.state[17] = (result.state[17] & ~0x0FU) | 4U;
    predict_fetch(api_, result, pc, 0x1E1E);
    auto value = read_be(api_, result.state[14], 1); add_read(result, result.state[14], 1);
    result.state[14] += 1; result.state[7] = (result.state[7] & 0xFFFFFF00U) | value;
    set_move_flags(result.state[17], value, 1);
    predict_fetch(api_, result, pc, 0x47F9);
    result.state[11] = read_be(api_, pc, 4);
    predict_extension16(api_, result, pc);
    predict_extension16(api_, result, pc);
    predict_fetch(api_, result, pc, 0xD6C7); result.state[11] += static_cast<std::int16_t>(result.state[7]);
    predict_fetch(api_, result, pc, 0x1E1E);
    value = read_be(api_, result.state[14], 1); add_read(result, result.state[14], 1);
    result.state[14] += 1; result.state[7] = (result.state[7] & 0xFFFFFF00U) | value;
    set_move_flags(result.state[17], value, 1);
    predict_fetch(api_, result, pc, 0x36DE);
    value = read_be(api_, result.state[14], 2); add_read(result, result.state[14], 2);
    result.state[14] += 2; add_write(result, result.state[11], 2, value); result.state[11] += 2;
    set_move_flags(result.state[17], value, 2);
    result.state[16] = k2D66Exit; result.ir = 0x36DE;
    result.pref_addr = k2D66Exit; result.pref_data = read_be(api_, k2D66Exit, 2);
    return result;
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict_604bc(const State& entry) const {
    Prediction result{}; result.state = entry; result.cycles = field(21); result.refresh = field(22);
    auto pc = k604BC; predict_fetch(api_, result, pc, 0x4DF9);
    result.state[14] = read_be(api_, pc, 4);
    predict_extension16(api_, result, pc);
    predict_extension16(api_, result, pc);
    result.state[16] = k604BCExit; result.ir = 0x4DF9; return result;
}

BasicBlockRegistry::Prediction BasicBlockRegistry::predict_61032(const State& entry) const {
    Prediction result{}; result.state = entry; result.cycles = field(21); result.refresh = field(22);
    auto pc = k61032; predict_fetch(api_, result, pc, 0xD481);
    const auto lhs = result.state[2];
    result.state[2] = lhs + result.state[1];
    set_add_long_flags(result.state[17], lhs, result.state[1], result.state[2]);
    result.state[16] = k61032Exit; result.ir = 0xD481;
    result.pref_addr = pc; result.pref_data = read_be(api_, pc, 2); return result;
}

void BasicBlockRegistry::execute_2d66() {
    auto opcode = fetch(); require(opcode == 0x48E7, "0x2D66 opcode mismatch"); step(opcode);
    (void)fetch();
    auto sp = api_.reg(15) - 2; write(sp, 2, api_.reg(11) & 0xFFFFU); sp -= 2;
    write(sp, 2, api_.reg(11) >> 16U); sp -= 2; write(sp, 2, api_.reg(7) & 0xFFFFU);
    sp -= 2; write(sp, 2, api_.reg(7) >> 16U); api_.set_reg(15, sp); api_.finish_instruction(opcode);
    api_.add_cycles(2 * 8 * 7); api_.skip_bus_refresh();
    opcode = fetch(); require(opcode == 0x4247, "0x2D6A opcode mismatch"); step(opcode);
    api_.set_reg(7, api_.reg(7) & 0xFFFF0000U); api_.set_reg(17, (api_.reg(17) & ~0x0FU) | 4U); api_.finish_instruction(opcode);
    opcode = fetch(); require(opcode == 0x1E1E, "0x2D6C opcode mismatch"); step(opcode);
    auto value = read(api_.reg(14), 1); api_.set_reg(14, api_.reg(14) + 1);
    api_.set_reg(7, (api_.reg(7) & 0xFFFFFF00U) | value); auto sr = api_.reg(17);
    set_move_flags(sr, value, 1); api_.set_reg(17, sr); api_.finish_instruction(opcode);
    opcode = fetch(); require(opcode == 0x47F9, "0x2D6E opcode mismatch"); step(opcode);
    const auto address = (fetch() << 16U) | fetch(); api_.set_reg(11, address); api_.finish_instruction(opcode);
    opcode = fetch(); require(opcode == 0xD6C7, "0x2D74 opcode mismatch"); step(opcode);
    api_.set_reg(11, api_.reg(11) + static_cast<std::int16_t>(api_.reg(7))); api_.finish_instruction(opcode);
    opcode = fetch(); require(opcode == 0x1E1E, "0x2D76 opcode mismatch"); step(opcode);
    value = read(api_.reg(14), 1); api_.set_reg(14, api_.reg(14) + 1);
    api_.set_reg(7, (api_.reg(7) & 0xFFFFFF00U) | value); sr = api_.reg(17);
    set_move_flags(sr, value, 1); api_.set_reg(17, sr); api_.finish_instruction(opcode);
    opcode = fetch(); require(opcode == 0x36DE, "0x2D78 opcode mismatch"); step(opcode);
    value = read(api_.reg(14), 2); api_.set_reg(14, api_.reg(14) + 2);
    write(api_.reg(11), 2, value); api_.set_reg(11, api_.reg(11) + 2); sr = api_.reg(17);
    set_move_flags(sr, value, 2); api_.set_reg(17, sr); api_.finish_instruction(opcode);
}

void BasicBlockRegistry::execute_604bc() {
    auto opcode = fetch(); require(opcode == 0x4DF9, "0x604BC opcode mismatch"); step(opcode);
    api_.set_reg(14, (fetch() << 16U) | fetch()); api_.finish_instruction(opcode);
}

void BasicBlockRegistry::execute_61032() {
    const auto opcode = fetch(); require(opcode == 0xD481, "0x61032 opcode mismatch"); step(opcode);
    const auto lhs = api_.reg(2);
    const auto rhs = api_.reg(1);
    const auto sum = lhs + rhs;
    api_.set_reg(2, sum);
    auto sr = api_.reg(17);
    set_add_long_flags(sr, lhs, rhs, sum);
    api_.set_reg(17, sr); api_.finish_instruction(opcode);
}

void BasicBlockRegistry::compare(const Prediction& prediction) {
    const auto actual = state();
    for (unsigned i = 0; i < actual.size(); ++i)
        require(actual[i] == prediction.state[i], "register[" + std::to_string(i) + "]");
    require(field(18) == prediction.ir, "IR");
    require(field(19) == prediction.pref_addr && field(20) == prediction.pref_data,
            "prefetch");
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
    active_pc_ = pc; prediction_ = predict(pc, state()); writes_.clear(); reads_.clear(); active_ = true; shadow_ = true;
}

void BasicBlockRegistry::finish_shadow() {
    compare(prediction_); ++metrics_.shadow_comparisons;
    ++metrics_.shadow_by_block[block_index(active_pc_)];
    active_ = false; shadow_ = false;
}

void BasicBlockRegistry::finish_native(const Prediction& prediction) {
    compare(prediction); active_ = false; shadow_ = false;
}

void BasicBlockRegistry::execute(unsigned pc) {
    active_pc_ = pc; prediction_ = predict(pc, state()); writes_.clear(); reads_.clear(); active_ = true; shadow_ = false;
    if (pc == k2D66) execute_2d66(); else if (pc == k604BC) execute_604bc(); else execute_61032();
    finish_native(prediction_); ++metrics_.translated_entries;
    ++metrics_.translated_by_block[block_index(pc)];
    metrics_.translated_instructions += pc == k2D66 ? 7U : 1U;
    ++metrics_.translated_blocks;
    log_ << "{\"block\":\"0x" << std::hex << pc << std::dec << "\",\"translated_instructions\":"
         << (pc == k2D66 ? 7 : 1) << ",\"timing_exact\":true}\n";
}

int BasicBlockRegistry::dispatch(unsigned pc) noexcept {
    if (!error_.empty()) return 0;
    try {
        if (pc != k2D66 && pc != k604BC && pc != k61032) return 0;
        require(!active_, "nested translated block");
        ++metrics_.natural_entries;
        ++metrics_.natural_by_block[block_index(pc)];
        if (mode_ == BasicBlockMode::SHADOW_NATIVE) { start_shadow(pc); return 0; }
        execute(pc); return 1;
    } catch (const std::exception& error) { fail(error.what()); return 0; }
}

void BasicBlockRegistry::event(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        address &= 0xFFFFFFU;
        if (type == 1) {
            if (shadow_ && address == (active_pc_ == k2D66 ? k2D66Exit : active_pc_ == k604BC ? k604BCExit : k61032Exit)) {
                finish_shadow(); return;
            }
            if (shadow_ && in_block(address)) ++metrics_.original_starts_inside_translated;
            if (in_registered_range(address) && !in_block(address)) ++metrics_.fallback_entries;
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
