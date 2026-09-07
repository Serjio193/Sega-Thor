#include "tools/hybrid/dispatch.hpp"
#include "core/rom_identity.hpp"
#include "game/graphics_decompress.hpp"
#include <algorithm>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
bool within(unsigned address, std::size_t count, unsigned base, std::size_t size) {
    return address >= base && address - base <= size && count <= size - (address - base);
}
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void write_state(std::ostream& out, const State& state) {
    out << '[';
    for (unsigned i = 0; i < state.size(); ++i) out << (i ? "," : "") << state[i];
    out << ']';
}
}
Dispatch::Dispatch(Api api, Mode mode, std::span<const std::uint8_t> rom, std::ostream& log)
    : api_(api), mode_(mode), rom_(rom), log_(log) { require_mode(mode); }

State Dispatch::state() const {
    State value;
    for (unsigned i = 0; i < value.size(); ++i) value[i] = api_.reg(i);
    return value;
}
std::vector<std::uint8_t> Dispatch::bytes(unsigned address, std::size_t count) const {
    std::vector<std::uint8_t> result(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto value = api_.peek(address + static_cast<unsigned>(i));
        require(value >= 0 && value < 256, "memory outside side-effect-free bridge");
        result[i] = static_cast<std::uint8_t>(value);
    }
    return result;
}
void Dispatch::begin() {
    require(++calls <= 64, "natural-call budget exceeded");
    entry_ = state();
    entry_frame_ = frame_;
    const auto source = entry_[8];
    const auto destination = entry_[9];
    require(source < rom_.size() && destination >= 0xFF0000 && destination < 0x1000000,
            "unsupported source/destination mapping");
    require(entry_[15] >= 0xFF001C && entry_[15] <= 0xFFFFFC && !(entry_[15] & 1),
            "unsupported stack mapping");
    // Preflight computes only an exact capture footprint from immutable ROM.
    // Both independent implementations are run again on the preserved copy at return.
    std::vector<std::uint8_t> scratch(0x1000000 - destination);
    const auto footprint = game::decompress_graphics(
        rom_.subspan(source, std::min<std::size_t>(65536, rom_.size() - source)), scratch);
    require(footprint.source_consumed <= 65536, "source footprint exceeds bound");
    source_ = bytes(source, footprint.source_consumed);
    require(std::equal(source_.begin(), source_.end(), rom_.begin() + source),
            "emulator source differs from canonical ROM");
    stack_base_ = entry_[15] - (source_[2] == 0 ? 28 : 16);
    require(destination + footprint.output_size <= stack_base_ || destination >= entry_[15] + 4,
            "overlapping stack/output");
    initial_output_ = bytes(destination, footprint.output_size);
    initial_stack_ = bytes(stack_base_, entry_[15] + 4 - stack_base_);
    const auto n = initial_stack_.size();
    return_pc_ = (initial_stack_[n-4] << 24U) | (initial_stack_[n-3] << 16U) |
                 (initial_stack_[n-2] << 8U) | initial_stack_[n-1];
    require(return_pc_ < rom_.size() && !(return_pc_ & 1) &&
            !(return_pc_ >= target && return_pc_ < 0x3B3E), "unsupported return PC");
    output_writes_.clear();
    stack_writes_.clear();
    instructions_ = 0;
    active_ = true;
}
void Dispatch::finish() {
    const auto prediction = predict(entry_, return_pc_, source_, initial_output_.size());
    const auto actual = state();
    auto diff = compare_registers(prediction.state, actual);
    require(diff.empty(), diff);
    require(prediction.consumed == source_.size(), "FIRST_DIVERGENCE source footprint");
    diff = compare_bytes(prediction.output, bytes(entry_[9], initial_output_.size()), "output RAM");
    require(diff.empty(), diff);
    diff = compare_bytes(prediction.output, output_writes_, "output write effects");
    require(diff.empty(), diff);
    require(stack_writes_.size() == prediction.stack_writes.size(), "FIRST_DIVERGENCE stack write count");
    auto expected_stack = initial_stack_;
    for (std::size_t i = 0; i < prediction.stack_writes.size(); ++i) {
        const auto expected = prediction.stack_writes[i];
        require(stack_writes_[i].address == expected.address && stack_writes_[i].value == expected.value,
                "FIRST_DIVERGENCE stack write[" + std::to_string(i) + "] expected_address=" +
                std::to_string(expected.address) + " actual_address=" + std::to_string(stack_writes_[i].address) +
                " expected_value=" + std::to_string(expected.value) +
                " actual_value=" + std::to_string(stack_writes_[i].value));
        expected_stack[expected.address - stack_base_] = expected.value;
    }
    diff = compare_bytes(expected_stack, bytes(stack_base_, expected_stack.size()), "stack RAM");
    require(diff.empty(), diff);
    ++comparisons;
    log_ << "{\"call\":" << calls << ",\"entry_frame\":" << entry_frame_
         << ",\"return_frame\":" << frame_ << ",\"body_instructions\":" << instructions_
         << ",\"consumed\":" << prediction.consumed << ",\"output_size\":" << prediction.output.size()
         << ",\"source_sha256\":\"" << calculate_sha256(source_)
         << "\",\"output_sha256\":\"" << calculate_sha256(prediction.output)
         << "\",\"entry_stack_sha256\":\"" << calculate_sha256(initial_stack_)
         << "\",\"return_stack_sha256\":\"" << calculate_sha256(expected_stack)
         << "\",\"entry_output_sha256\":\"" << calculate_sha256(initial_output_)
         << "\",\"sr_comparison_mask\":65519,\"x_contract\":\"UNMODELED\",\"entry\":";
    write_state(log_, entry_);
    log_ << ",\"return\":";
    write_state(log_, actual);
    log_ << ",\"bounded_effects_equal\":true}\n";
    active_ = false;
}
void Dispatch::event(int type, int width, unsigned address, unsigned value) {
    if (active_ && in_interrupt_) {
        if (type == 1 && address == suspended_[16] && api_.reg(15) == suspended_[15]) {
            const auto resumed = state();
            for (unsigned i = 0; i < 17; ++i)
                require(resumed[i] == suspended_[i], "interrupt changed suspended routine register");
            // At the vector read GPGX has already raised the interrupt mask.
            // Its saved CCR must nevertheless survive the external ISR exactly.
            require((resumed[17] & 0xFF) == (suspended_[17] & 0xFF), "interrupt changed suspended CCR");
            in_interrupt_ = false;
        } else {
            if (type == 4) {
                const auto bus_address = address & 0xFFFFFF;
                const auto overlaps = [&](unsigned base, std::size_t count) {
                    return bus_address < base + count && bus_address + width > base;
                };
                require(!overlaps(entry_[8], source_.size()) &&
                        !overlaps(entry_[9], initial_output_.size()) &&
                        !overlaps(stack_base_, initial_stack_.size()),
                        "interrupt modified required routine footprint");
            }
            return; // External ISR effects remain authoritative and are not translated.
        }
    }
    // GPGX m68ki_exception_interrupt reads its autovector before stacking state.
    if (active_ && type == 2 && width == 4 && address >= 0x64 && address <= 0x7C && !(address & 3)) {
        suspended_ = state();
        in_interrupt_ = true;
        ++interrupts;
        return;
    }
    if (type == 1) {
        current_pc_ = address;
        if (active_ && address == return_pc_ && api_.reg(15) == entry_[15] + 4) finish();
        if (address == target) {
            if (mode_ == Mode::EMULATED) { ++calls; return; }
            require(!active_, "nested target call");
            begin();
        }
        if (active_) {
            require(address >= target && address < 0x3B3E, "control left routine before proven return");
            require(++instructions_ <= 200000, "routine instruction budget exceeded");
            ++body_instructions;
        }
        return;
    }
    if (!active_ || (type != 2 && type != 4)) return;
    address &= 0xFFFFFF;
    require(width == 1 || width == 2 || width == 4, "unsupported memory width");
    if (type == 2) {
        require(within(address, width, entry_[8], source_.size()) ||
                within(address, width, entry_[9], output_writes_.size()) ||
                within(address, width, stack_base_, initial_stack_.size()),
                "read outside required source/written-output/stack footprint at " + std::to_string(address));
    } else if (within(address, width, entry_[9], initial_output_.size())) {
        require(address == entry_[9] + output_writes_.size(), "nonsequential output write");
        for (int i = width - 1; i >= 0; --i) output_writes_.push_back(value >> (8 * i));
    } else {
        require(within(address, width, stack_base_, initial_stack_.size() - 4),
                "write outside required output/saved-stack footprint");
        require(stack_writes_.size() + width <= 28, "stack write budget exceeded");
        for (int i = 0; i < width; ++i) stack_writes_.push_back(
            {address + static_cast<unsigned>(i), static_cast<std::uint8_t>(value >> (8 * (width-1-i)))});
    }
}
void Dispatch::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try { event(type, width, address, value); }
    catch (const std::exception& error) {
        ++divergences;
        error_ = "FIRST_DIVERGENCE call=" + std::to_string(calls) + " pc=" +
            std::to_string(current_pc_) + " " + error.what();
    }
}
}
