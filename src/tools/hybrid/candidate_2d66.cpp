#include "tools/hybrid/candidate_2d66.hpp"
#include "core/table_copy_routine.hpp"
#include "core/rom_identity.hpp"
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace oasis::hybrid {
namespace {
constexpr unsigned kTarget = 0x2D66;
constexpr unsigned kDestination = 0xFF134C;
constexpr unsigned kRomLimit = 0x300000;

using PortableState = std::array<std::uint32_t, 18>;
using PortableWrite = std::tuple<unsigned, int, unsigned>;

oasis::core::TableCopyRoutineContract portable_contract() {
    return {0x2D66, 0x2D6A, 0x2D6C, 0x2D6E, 0x2D74, 0x2D76,
            0x2D78, 0x2D7A, 0x2D7E, 0x2D82, 0x2D84, kDestination};
}

class ApiMachine final : public oasis::core::TableCopyRoutineMachine {
public:
    explicit ApiMachine(CandidateApi api) : api_(api) {}

    std::uint32_t reg(unsigned index) const override { return api_.reg(index); }
    void set_reg(unsigned index, std::uint32_t value) override { api_.set_reg(index, value); }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i)
            value = (value << 8U) | static_cast<unsigned>(api_.peek(address + i));
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        api_.poke(address, static_cast<int>(width), value);
    }
    void begin(oasis::core::TableCopyStep) override {}
    void finish(oasis::core::TableCopyStep) override {}
    oasis::core::RoutineBoundaryReason boundary() override {
        return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override {
        api_.set_reg(16, return_pc);
        if (api_.set_return_state)
            api_.set_return_state(return_pc, 0x2D84,
                                   (api_.peek(0x2D84) << 8) | api_.peek(0x2D85));
        if (api_.add_cycles) api_.add_cycles(2828);
        if (api_.skip_bus_refresh) api_.skip_bus_refresh();
    }

private:
    CandidateApi api_;
};

class ShadowMachine final : public oasis::core::TableCopyRoutineMachine {
public:
    ShadowMachine(const PortableState& initial, unsigned source,
                  std::span<const std::uint8_t> source_bytes, unsigned destination,
                  std::span<const std::uint8_t> output, unsigned stack,
                  std::span<const std::uint8_t> stack_bytes)
        : state_(initial), destination_(destination), stack_(stack) {
        load(source, source_bytes);
        load(destination, output);
        load(stack - 8U, stack_bytes);
    }

    std::uint32_t reg(unsigned index) const override { return state_[index]; }
    void set_reg(unsigned index, std::uint32_t value) override { state_[index] = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i)
            value = (value << 8U) | bytes_[address + i];
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        for (unsigned i = 0; i < width; ++i)
            bytes_[address + i] = static_cast<std::uint8_t>(value >> (8U * (width - i - 1U)));
        writes_.emplace_back(address, static_cast<int>(width), value & (width == 2U ? 0xFFFFU : 0xFFFFFFFFU));
    }
    void begin(oasis::core::TableCopyStep) override {}
    void finish(oasis::core::TableCopyStep) override {}
    oasis::core::RoutineBoundaryReason boundary() override {
        return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override { state_[16] = return_pc; }

    const PortableState& state() const { return state_; }
    std::vector<std::uint8_t> range(unsigned address, unsigned size) const {
        std::vector<std::uint8_t> result;
        result.reserve(size);
        for (unsigned i = 0; i < size; ++i) result.push_back(bytes_.at(address + i));
        return result;
    }
    const std::vector<PortableWrite>& writes() const { return writes_; }

private:
    void load(unsigned address, std::span<const std::uint8_t> bytes) {
        for (unsigned i = 0; i < bytes.size(); ++i) bytes_[address + i] = bytes[i];
    }
    PortableState state_{};
    std::unordered_map<unsigned, std::uint8_t> bytes_;
    std::vector<PortableWrite> writes_;
    unsigned destination_{};
    unsigned stack_{};
};
}

Candidate2D66::Candidate2D66(CandidateApi api, Mode mode,
                             std::span<const std::uint8_t> rom, std::ostream& log)
    : api_(api), mode_(mode), rom_(rom), log_(log) {}

Candidate2D66::State Candidate2D66::state() const {
    State value{};
    for (unsigned i = 0; i < value.size(); ++i) value[i] = api_.reg(i);
    return value;
}

int Candidate2D66::peek(unsigned address) const {
    const auto value = api_.peek(address);
    if (value < 0 || value > 0xFF) throw std::runtime_error("candidate memory peek failed");
    return value;
}

void Candidate2D66::require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void Candidate2D66::fail(const std::string& message) {
    ++divergences;
    error_ = "FIRST_DIVERGENCE call=" + std::to_string(calls) + " pc=" +
             std::to_string(current_pc_) + " " + message;
    active_ = false;
}

void Candidate2D66::begin() {
    require(++calls <= 64, "candidate call budget exceeded");
    entry_ = state();
    require(entry_[16] == kTarget &&
            ((entry_[14] < rom_.size()) ||
             (entry_[14] >= 0xFF0000 && entry_[14] < 0x1000000)),
            "candidate source is outside canonical ROM/RAM");
    require(entry_[15] >= 12 && !(entry_[15] & 1), "candidate stack bound");
    entry_stack_ = entry_[15];
    entry_cycles_ = api_.cycles ? api_.cycles() : 0;
    entry_refresh_ = api_.refresh_cycles ? api_.refresh_cycles() : 0;
    const auto offset = static_cast<unsigned>(peek(entry_[14]));
    const auto count = static_cast<unsigned>(peek(entry_[14] + 1));
    loop_count_ = count;
    source_size_ = 2U + 2U * (count + 1U);
    output_size_ = 2U * (count + 1U);
    destination_ = kDestination + offset;
    source_.resize(source_size_);
    for (unsigned i = 0; i < source_size_; ++i) source_[i] =
        static_cast<std::uint8_t>(peek(entry_[14] + i));
    require(source_[0] == offset && source_[1] == count, "candidate source preflight changed");
    if (entry_[14] < rom_.size()) {
        require(entry_[14] + source_size_ <= rom_.size() &&
                std::equal(source_.begin(), source_.end(), rom_.begin() + entry_[14]),
                "candidate ROM source differs from canonical ROM");
    }
    initial_output_.resize(output_size_);
    for (unsigned i = 0; i < output_size_; ++i) initial_output_[i] =
        static_cast<std::uint8_t>(peek(destination_ + i));
    initial_stack_.resize(12);
    for (unsigned i = 0; i < initial_stack_.size(); ++i)
        initial_stack_[i] = static_cast<std::uint8_t>(peek(entry_stack_ - 8 + i));
    return_pc_ = (initial_stack_[8] << 24U) | (initial_stack_[9] << 16U) |
                 (initial_stack_[10] << 8U) | initial_stack_[11];
    require(return_pc_ < kRomLimit && !(return_pc_ & 1), "candidate return PC bound");
    expected_stack_ = initial_stack_;
    // GPGX's MOVEM pre-decrement walks the mask in ascending register order,
    // decrementing before each longword.  For mask 0x0110 this leaves D7 at
    // entry A7-8 and A3 at entry A7-4.
    expected_stack_[0] = static_cast<std::uint8_t>(entry_[7] >> 24);
    expected_stack_[1] = static_cast<std::uint8_t>(entry_[7] >> 16);
    expected_stack_[2] = static_cast<std::uint8_t>(entry_[7] >> 8);
    expected_stack_[3] = static_cast<std::uint8_t>(entry_[7]);
    expected_stack_[4] = static_cast<std::uint8_t>(entry_[11] >> 24);
    expected_stack_[5] = static_cast<std::uint8_t>(entry_[11] >> 16);
    expected_stack_[6] = static_cast<std::uint8_t>(entry_[11] >> 8);
    expected_stack_[7] = static_cast<std::uint8_t>(entry_[11]);
    writes_.clear();
    body_starts_ = 0;
    active_ = true;
    overridden_ = false;
    portable_shadow_ready_ = false;
    if (mode_ == Mode::SHADOW_NATIVE) {
        ShadowMachine shadow(entry_, entry_[14], source_, destination_, initial_output_,
                             entry_stack_, initial_stack_);
        oasis::core::RoutineContinuation continuation{};
        const auto result = oasis::core::execute_table_copy_routine(
            shadow, portable_contract(), kTarget, continuation);
        require(result.reason == oasis::core::RoutineExitReason::NORMAL &&
                    !continuation.active && result.next_token == 0x2D84,
                "portable shadow routine did not return normally");
        portable_state_ = shadow.state();
        portable_output_ = shadow.range(destination_, output_size_);
        portable_stack_ = shadow.range(entry_stack_ - 8U, initial_stack_.size());
        portable_writes_.clear();
        for (const auto& write : shadow.writes())
            portable_writes_.push_back({std::get<0>(write), std::get<1>(write), std::get<2>(write)});
        portable_shadow_ready_ = true;
    } else if (mode_ == Mode::NATIVE_OVERRIDE) {
        apply_override();
    }
}

std::string Candidate2D66::compare_state(const State& expected, const State& actual) const {
    for (unsigned i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) return "FIRST_DIVERGENCE register[" +
            std::to_string(i) + "] expected=" + std::to_string(expected[i]) +
            " actual=" + std::to_string(actual[i]);
    }
    return {};
}

std::string Candidate2D66::compare_memory(unsigned address,
                                          std::span<const std::uint8_t> expected,
                                          const char* name) const {
    for (std::size_t i = 0; i < expected.size(); ++i)
        if (peek(address + static_cast<unsigned>(i)) != expected[i])
            return std::string("FIRST_DIVERGENCE ") + name + " byte[" + std::to_string(i) + "]";
    return {};
}

void Candidate2D66::finish() {
    State expected = entry_;
    expected[14] += source_size_;
    // The routine restores its eight-byte MOVEM frame, then RTS consumes the
    // four-byte return address that was already present at entry A7.
    expected[15] += 4;
    expected[16] = return_pc_;
    const auto last = static_cast<unsigned>((source_[source_size_ - 2] << 8U) |
                                            source_[source_size_ - 1]);
    expected[17] = (entry_[17] & 0xFFF0U) |
                   ((last & 0x8000U) ? 8U : 0U) | (last == 0 ? 4U : 0U);
    auto diff = compare_state(expected, state());
    require(diff.empty(), diff);
    diff = compare_memory(destination_, std::span<const std::uint8_t>(source_).subspan(2), "output");
    // The output bytes are source words after the two count bytes; this span is exact.
    require(diff.empty(), diff);
    diff = compare_memory(entry_stack_ - 8, expected_stack_, "stack");
    require(diff.empty(), diff);
    std::vector<Write> expected_writes;
    expected_writes.reserve(4U + loop_count_ + 1U);
    expected_writes.push_back({entry_stack_ - 2, 2, entry_[11] & 0xFFFFU});
    expected_writes.push_back({entry_stack_ - 4, 2, entry_[11] >> 16});
    expected_writes.push_back({entry_stack_ - 6, 2, entry_[7] & 0xFFFFU});
    expected_writes.push_back({entry_stack_ - 8, 2, entry_[7] >> 16});
    for (unsigned i = 0; i < output_size_; i += 2)
        expected_writes.push_back({destination_ + i, 2,
                                   static_cast<unsigned>((source_[i + 2] << 8U) | source_[i + 3])});
    require(writes_.size() == expected_writes.size(), "candidate write count");
    for (std::size_t i = 0; i < writes_.size(); ++i) {
        require(writes_[i].address == expected_writes[i].address &&
                writes_[i].width == expected_writes[i].width &&
                writes_[i].value == expected_writes[i].value,
                "FIRST_DIVERGENCE write[" + std::to_string(i) + "]");
    }
    if (portable_shadow_ready_) {
        require(compare_state(portable_state_, state()).empty(),
                "FIRST_DIVERGENCE portable state");
        require(compare_memory(destination_, portable_output_, "portable output").empty(),
                "FIRST_DIVERGENCE portable output");
        require(compare_memory(entry_stack_ - 8U, portable_stack_, "portable stack").empty(),
                "FIRST_DIVERGENCE portable stack");
        require(writes_.size() == portable_writes_.size(), "portable write count");
        for (std::size_t i = 0; i < writes_.size(); ++i)
            require(writes_[i].address == portable_writes_[i].address &&
                        writes_[i].width == portable_writes_[i].width &&
                        writes_[i].value == portable_writes_[i].value,
                    "FIRST_DIVERGENCE portable write[" + std::to_string(i) + "]");
    }
    ++comparisons;
    log_ << "{\"call\":" << calls << ",\"source\":" << entry_[14]
         << ",\"destination\":" << destination_ << ",\"source_size\":" << source_size_
         << ",\"output_size\":" << output_size_ << ",\"body_instruction_starts\":"
         << body_starts_ << ",\"cycle_delta\":"
         << (api_.cycles ? api_.cycles() - entry_cycles_ : 0)
         << ",\"refresh_delta\":" << (api_.refresh_cycles ? api_.refresh_cycles() - entry_refresh_ : 0)
         << ",\"override\":false,\"interrupts\":" << interrupt_count << "}\n";
    active_ = false;
}

void Candidate2D66::apply_override() {
    ApiMachine machine(api_);
    oasis::core::RoutineContinuation continuation{};
    const auto result = oasis::core::execute_table_copy_routine(
        machine, portable_contract(), kTarget, continuation);
    require(result.reason == oasis::core::RoutineExitReason::NORMAL &&
                !continuation.active && result.next_token == 0x2D84,
            "portable native routine did not return normally");
    native_routine_instructions += result.guest_instructions;
    native_routine_invocations += 1U;
    native_routine_iterations += result.copy_iterations;
    native_routine_boundary_yields += result.boundary_yields;
    ++override_calls;
    overridden_ = true;
    active_ = false;
    log_ << "{\"call\":" << calls << ",\"source\":" << entry_[14]
         << ",\"destination\":" << destination_ << ",\"source_size\":" << source_size_
         << ",\"output_size\":" << output_size_ << ",\"body_instruction_starts\":0"
         << ",\"override\":true,\"interrupts\":0}\n";
}

void Candidate2D66::event(int type, int width, unsigned address, unsigned value) {
    if (type == 1) {
        current_pc_ = address;
        if (active_ && address == return_pc_ && api_.reg(15) == entry_stack_ + 4) {
            finish();
            return;
        }
        if (address == kTarget) {
            if (mode_ == Mode::EMULATED) { ++calls; return; }
            require(!active_, "nested candidate call");
            begin();
            return;
        }
        if (active_) {
            require(address >= kTarget && address < 0x2D84,
                    "candidate left body without RTS");
            ++body_starts_;
            ++body_instructions;
        }
        if (overridden_ && address >= kTarget && address < 0x2D84)
            require(false, "original candidate body was executed after override");
        return;
    }
    if (!active_) return;
    if (type == 2) {
        if (width == 4 && address >= 0x64 && address <= 0x7C && !(address & 3)) {
            ++interrupt_count;
            require(false, "interrupt observed inside candidate");
        }
        const auto in_source = address >= entry_[14] && address + width <= entry_[14] + source_size_;
        const auto in_stack = address >= entry_stack_ - 8 && address + width <= entry_stack_ + 4;
        require(in_source || in_stack, "candidate read outside source/stack footprint");
        return;
    }
    if (type != 4) return;
    const auto in_stack = address >= entry_stack_ - 8 && address + width <= entry_stack_;
    const auto in_output = address >= destination_ && address + width <= destination_ + output_size_;
    require(width == 2 && (in_stack || in_output), "candidate write outside bounded footprint");
    writes_.push_back({address, width, value & 0xFFFFU});
}

void Candidate2D66::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try { event(type, width, address, value); }
    catch (const std::exception& error) { fail(error.what()); }
}

} // namespace oasis::hybrid
