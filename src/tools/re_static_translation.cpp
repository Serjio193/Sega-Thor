#include "tools/re_static_translation.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::tools {
namespace {

constexpr std::uint16_t kNegative = 1U << 3U;
constexpr std::uint16_t kZero = 1U << 2U;
constexpr std::uint16_t kOverflow = 1U << 1U;
constexpr std::uint16_t kCarry = 1U << 0U;

void set_move_flags(M68kState& state, std::uint16_t value) {
    state.ccr = value == 0 ? kZero : static_cast<std::uint16_t>(
        (value & 0x8000U) != 0 ? kNegative : 0);
}

void set_compare_flags(M68kState& state, std::uint16_t left, std::uint16_t right) {
    const auto result = static_cast<std::uint16_t>(left - right);
    const bool overflow = ((left ^ right) & (left ^ result) & 0x8000U) != 0;
    const bool carry = left < right;
    state.ccr = (result == 0 ? kZero : 0) |
        ((result & 0x8000U) != 0 ? kNegative : 0) |
        (overflow ? kOverflow : 0) | (carry ? kCarry : 0);
}

void set_add_flags(M68kState& state, std::uint16_t left, std::uint16_t right,
                   std::uint16_t result) {
    const bool overflow = (~(left ^ right) & (left ^ result) & 0x8000U) != 0;
    const bool carry = static_cast<std::uint32_t>(left) + right > 0xFFFFU;
    state.ccr = (result == 0 ? kZero : 0) |
        ((result & 0x8000U) != 0 ? kNegative : 0) |
        (overflow ? kOverflow : 0) | (carry ? kCarry : 0);
}

class Stream {
public:
    explicit Stream(std::span<const std::uint8_t> source) : source_(source) {}

    [[nodiscard]] std::size_t position() const noexcept { return position_; }
    [[nodiscard]] std::uint8_t peek() const { return read_at(position_); }
    std::uint8_t read() { return read_at(position_++); }
    void skip(std::size_t count) { position_ += count; check(position_); }

private:
    [[nodiscard]] std::uint8_t read_at(std::size_t position) const {
        check(position);
        return source_[position];
    }
    void check(std::size_t position) const {
        if (position >= source_.size()) throw std::runtime_error("mechanical 0x3820 source bound");
    }

    std::span<const std::uint8_t> source_;
    std::size_t position_{};
};

class Output {
public:
    explicit Output(std::span<std::uint8_t> destination) : destination_(destination) {}

    [[nodiscard]] std::size_t size() const noexcept { return position_; }
    void write(std::uint8_t value) {
        if (position_ >= destination_.size()) throw std::runtime_error("mechanical 0x3820 destination bound");
        destination_[position_++] = value;
    }
    void copy(std::size_t distance, std::size_t count) {
        if (distance == 0 || distance > position_) throw std::runtime_error("mechanical 0x3820 back-reference");
        while (count-- != 0) write(destination_[position_ - distance]);
    }

private:
    std::span<std::uint8_t> destination_;
    std::size_t position_{};
};

class Bits {
public:
    explicit Bits(Stream& stream) : stream_(stream), bits_(stream.read()), remaining_(8) {}
    bool read_bit() {
        --remaining_;
        if (remaining_ < 0) {
            const auto low = stream_.read();
            const auto high = stream_.read();
            bits_ = static_cast<std::uint16_t>(low | (high << 8U));
            remaining_ = 15;
        }
        const bool result = (bits_ & 1U) != 0;
        bits_ >>= 1U;
        return result;
    }
    std::uint16_t read_code(unsigned count) {
        std::uint16_t result = 0;
        while (count-- != 0) result = static_cast<std::uint16_t>(
            (result << 1U) | (read_bit() ? 1U : 0U));
        return result;
    }

private:
    Stream& stream_;
    std::uint16_t bits_{};
    int remaining_{};
};

void mechanical_format_a(Stream& stream, Output& output) {
    for (;;) {
        const auto block_start = stream.position();
        const auto span = static_cast<std::uint16_t>(stream.read() | (stream.read() << 8U));
        const auto block_end = block_start + span;
        while (stream.position() < block_end) {
            auto command = stream.read();
            const bool match = (command & 0x80U) != 0;
            command &= 0x7FU;
            if (match) {
                const auto count = static_cast<std::size_t>(((command & 0x60U) >> 5U) + 4U);
                const auto distance = static_cast<std::size_t>(((command & 0x1FU) << 8U) | stream.read());
                output.copy(distance, count);
                while (stream.position() < block_end && (stream.peek() & 0xE0U) == 0x60U) {
                    const auto extension = static_cast<std::size_t>(stream.read() & 0x1FU);
                    output.copy(distance, extension == 0 ? 256U : extension);
                }
                continue;
            }
            const bool repeat = (command & 0x40U) != 0;
            command &= 0x3FU;
            if (repeat) {
                const bool extended = (command & 0x10U) != 0;
                command &= 0x0FU;
                auto count = static_cast<std::uint16_t>(command);
                if (extended) count = static_cast<std::uint16_t>((count << 8U) | stream.read());
                const auto length = static_cast<std::size_t>(count + 4U);
                const auto value = stream.read();
                for (std::size_t index = 0; index < length; ++index) output.write(value);
                continue;
            }
            const bool extended = (command & 0x20U) != 0;
            command &= 0x1FU;
            auto count = static_cast<std::uint16_t>(command);
            if (extended) count = static_cast<std::uint16_t>((count << 8U) | stream.read());
            const auto length = static_cast<std::size_t>(count == 0 ? 65536U : count);
            for (std::size_t index = 0; index < length; ++index) output.write(stream.read());
        }
        if (stream.position() != block_end) throw std::runtime_error("mechanical 0x3820 block bound");
        if (stream.read() == 0) return;
    }
}

std::size_t match_length(Bits& bits, Stream& stream) {
    if (bits.read_bit()) return 2;
    if (bits.read_bit()) return 3;
    if (bits.read_bit()) return 4;
    if (bits.read_bit()) return 5;
    if (bits.read_bit()) return static_cast<std::size_t>(bits.read_code(3)) + 6U;
    return static_cast<std::size_t>(stream.read()) + 14U;
}

void mechanical_format_b(Stream& stream, Output& output) {
    for (;;) {
        stream.skip(3);
        Bits bits(stream);
        for (;;) {
            if (!bits.read_bit()) {
                output.write(stream.read());
                continue;
            }
            std::uint16_t distance = 0;
            if (!bits.read_bit()) distance = stream.read();
            else {
                distance = static_cast<std::uint16_t>((bits.read_code(5) << 8U) | stream.read());
                if (distance == 0) break;
                if (distance == 1) {
                    const bool extended = bits.read_bit();
                    auto count = bits.read_code(4);
                    if (extended) count = static_cast<std::uint16_t>((count << 8U) | stream.read());
                    const auto value = stream.read();
                    for (std::size_t index = 0; index < count + 14U; ++index) output.write(value);
                    continue;
                }
            }
            output.copy(distance, match_length(bits, stream));
        }
        if (stream.read() == 0) return;
    }
}

} // namespace

BoundedMemory::BoundedMemory(std::uint32_t base, std::span<std::uint8_t> bytes)
    : base_(base), bytes_(bytes) {}

std::size_t BoundedMemory::offset(std::uint32_t address, std::size_t width) const {
    const auto delta = static_cast<std::uint64_t>(address) - base_;
    if (address < base_ || delta + width > bytes_.size()) throw std::out_of_range("bounded memory access");
    return static_cast<std::size_t>(delta);
}

std::uint32_t BoundedMemory::read_u16(std::uint32_t address) const {
    const auto at = offset(address, 2U);
    return (static_cast<std::uint32_t>(bytes_[at]) << 8U) | bytes_[at + 1U];
}

std::uint32_t BoundedMemory::read_u32(std::uint32_t address) const {
    const auto at = offset(address, 4U);
    return (static_cast<std::uint32_t>(bytes_[at]) << 24U) |
        (static_cast<std::uint32_t>(bytes_[at + 1U]) << 16U) |
        (static_cast<std::uint32_t>(bytes_[at + 2U]) << 8U) | bytes_[at + 3U];
}

void BoundedMemory::write_u16(std::uint32_t address, std::uint16_t value) {
    const auto at = offset(address, 2U);
    bytes_[at] = static_cast<std::uint8_t>(value >> 8U);
    bytes_[at + 1U] = static_cast<std::uint8_t>(value);
    writes_.push_back({address, 2U, value});
}

void BoundedMemory::write_u32(std::uint32_t address, std::uint32_t value) {
    const auto at = offset(address, 4U);
    bytes_[at] = static_cast<std::uint8_t>(value >> 24U);
    bytes_[at + 1U] = static_cast<std::uint8_t>(value >> 16U);
    bytes_[at + 2U] = static_cast<std::uint8_t>(value >> 8U);
    bytes_[at + 3U] = static_cast<std::uint8_t>(value);
    writes_.push_back({address, 4U, value});
}

StateDiff compare_state(const M68kState& expected, const M68kState& actual,
                        const BoundedMemory& expected_memory,
                        const BoundedMemory& actual_memory) {
    for (std::size_t index = 0; index < expected.d.size(); ++index)
        if (expected.d[index] != actual.d[index]) return {false, "D" + std::to_string(index)};
    for (std::size_t index = 0; index < expected.a.size(); ++index)
        if (expected.a[index] != actual.a[index]) return {false, "A" + std::to_string(index)};
    if (expected.ccr != actual.ccr) return {false, "CCR"};
    if (expected_memory.writes().size() != actual_memory.writes().size())
        return {false, "memory write count"};
    for (std::size_t index = 0; index < expected_memory.writes().size(); ++index) {
        const auto& left = expected_memory.writes()[index];
        const auto& right = actual_memory.writes()[index];
        if (left.address != right.address || left.width != right.width || left.value != right.value)
            return {false, "memory write " + std::to_string(index)};
    }
    const auto left = expected_memory.bytes();
    const auto right = actual_memory.bytes();
    if (left.size() != right.size()) return {false, "memory size"};
    for (std::size_t index = 0; index < left.size(); ++index)
        if (left[index] != right[index]) return {false, "memory byte " + std::to_string(index)};
    return {true, {}};
}

oasis::game::DecompressResult mechanical_3820(std::span<const std::uint8_t> source,
                                               std::span<std::uint8_t> destination) {
    if (source.size() < 4U) throw std::runtime_error("mechanical 0x3820 header bound");
    Stream stream(source);
    Output output(destination);
    if (source[2] != 0) mechanical_format_a(stream, output);
    else mechanical_format_b(stream, output);
    return {stream.position(), output.size()};
}

TranslationRun mechanical_A8DA(M68kState& state) {
    set_compare_flags(state, static_cast<std::uint16_t>(state.d[5]), 0x0050U);
    if ((state.ccr & kCarry) == 0) return {TranslationStatus::verified, 3U, {}};
    state.d[5] = (state.d[5] & 0xFFFF0000U) | static_cast<std::uint16_t>(state.d[5] + 1U);
    set_move_flags(state, static_cast<std::uint16_t>(state.d[5]));
    state.d[5] = (state.d[5] & 0xFFFF0000U) | static_cast<std::uint16_t>(state.d[2]);
    set_move_flags(state, static_cast<std::uint16_t>(state.d[5]));
    state.d[0] = (state.d[0] & 0xFFFF0000U) | static_cast<std::uint16_t>(state.d[5]);
    set_move_flags(state, static_cast<std::uint16_t>(state.d[0]));
    const auto sum = static_cast<std::uint16_t>(state.d[0] + state.d[4]);
    set_add_flags(state, static_cast<std::uint16_t>(state.d[0]), static_cast<std::uint16_t>(state.d[4]), sum);
    state.d[0] = (state.d[0] & 0xFFFF0000U) | sum;
    state.d[5] = (state.d[5] & 0xFFFF0000U) | sum;
    set_move_flags(state, sum);
    state.d[5] = (state.d[5] & 0xFFFF0000U) | static_cast<std::uint16_t>(state.d[3]);
    set_move_flags(state, static_cast<std::uint16_t>(state.d[5]));
    state.d[5] = (state.d[5] & 0xFFFF0000U) | static_cast<std::uint16_t>(state.d[1]);
    set_move_flags(state, static_cast<std::uint16_t>(state.d[5]));
    return {TranslationStatus::verified, 10U, {}};
}

TranslationRun mechanical_62CC(M68kState& state, BoundedMemory& memory) {
    state.d[0] = 0;
    const auto base = state.a[6];
    memory.write_u32(base + 0x4EU, 0);
    memory.write_u32(base + 0x52U, 0);
    memory.write_u16(base + 0x2AU, 0);
    memory.write_u16(base + 0x04U, 0);
    state.ccr = kZero;
    return {TranslationStatus::verified, 6U, {}};
}

TranslationRun unsupported_opcode(std::uint16_t opcode) {
    return {TranslationStatus::unsupported, 0U, "opcode 0x" +
        std::to_string(static_cast<unsigned>(opcode)) + " is unsupported; translation stopped"};
}

} // namespace oasis::tools
