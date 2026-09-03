#include "game/graphics_decompress.hpp"

#include <cstdint>
#include <stdexcept>

namespace oasis::game {
namespace {

class Stream {
public:
    explicit Stream(std::span<const std::uint8_t> source) : source_(source) {}

    [[nodiscard]] std::size_t position() const noexcept { return position_; }

    [[nodiscard]] std::uint8_t peek_byte() const {
        if (position_ >= source_.size()) fail("compressed stream peek past end");
        return source_[position_];
    }

    std::uint8_t read_byte() {
        if (position_ >= source_.size()) fail("compressed stream read past end");
        return source_[position_++];
    }

    void skip(std::size_t count) {
        if (count > source_.size() - position_) fail("compressed stream header exceeds source");
        position_ += count;
    }

private:
    [[noreturn]] static void fail(const char* message) { throw std::runtime_error(message); }

    std::span<const std::uint8_t> source_;
    std::size_t position_{};
};

class Output {
public:
    explicit Output(std::span<std::uint8_t> destination) : destination_(destination) {}

    [[nodiscard]] std::size_t size() const noexcept { return position_; }

    void write(std::uint8_t value) {
        if (position_ >= destination_.size()) fail("decompressed output exceeds destination");
        destination_[position_++] = value;
    }

    void copy_match(std::size_t distance, std::size_t count) {
        if (distance == 0 || distance > position_) fail("invalid graphics back-reference");
        while (count-- != 0) write(destination_[position_ - distance]);
    }

private:
    [[noreturn]] static void fail(const char* message) { throw std::runtime_error(message); }

    std::span<std::uint8_t> destination_;
    std::size_t position_{};
};

class BitReader {
public:
    explicit BitReader(Stream& stream) : stream_(stream), bits_(stream_.read_byte()), remaining_(8) {}

    bool read_bit() {
        --remaining_;
        if (remaining_ < 0) {
            const auto low = stream_.read_byte();
            const auto high = stream_.read_byte();
            bits_ = static_cast<std::uint16_t>(low) |
                    static_cast<std::uint16_t>(high << 8U);
            remaining_ = 15;
        }
        const bool bit = (bits_ & 1U) != 0;
        bits_ >>= 1U;
        return bit;
    }

    std::uint16_t read_code(unsigned count) {
        std::uint16_t value = 0;
        while (count-- != 0) {
            value = static_cast<std::uint16_t>((value << 1U) | (read_bit() ? 1U : 0U));
        }
        return value;
    }

private:
    Stream& stream_;
    std::uint16_t bits_{};
    int remaining_{};
};

void decode_command_stream(Stream& stream, Output& output) {
    for (;;) {
        const auto block_start = stream.position();
        const std::uint16_t span = static_cast<std::uint16_t>(
            stream.read_byte() | static_cast<std::uint16_t>(stream.read_byte() << 8U));
        const auto block_end = block_start + span;

        while (stream.position() < block_end) {
            std::uint16_t command = stream.read_byte();
            const bool match = (command & 0x80U) != 0;
            command &= 0x7FU;

            if (match) {
                const auto count = static_cast<std::size_t>(((command & 0x60U) >> 5U) + 4U);
                const auto distance = static_cast<std::size_t>(
                    ((command & 0x1FU) << 8U) | stream.read_byte());
                output.copy_match(distance, count);

                while (stream.position() < block_end &&
                       (stream.peek_byte() & 0xE0U) == 0x60U) {
                    const auto extension = static_cast<std::uint8_t>(stream.read_byte() & 0x1FU);
                    output.copy_match(distance, extension == 0 ? 256U : extension);
                }
                continue;
            }

            const bool repeat = (command & 0x40U) != 0;
            command &= 0x3FU;
            if (repeat) {
                const bool extended = (command & 0x10U) != 0;
                command &= static_cast<std::uint16_t>(~0x10U);
                std::uint16_t counter = command;
                if (extended) {
                    counter = static_cast<std::uint16_t>((counter << 8U) | stream.read_byte());
                }
                const auto count = static_cast<std::size_t>(
                    static_cast<std::uint16_t>(counter + 3U)) + 1U;
                const auto value = stream.read_byte();
                for (std::size_t i = 0; i < count; ++i) output.write(value);
                continue;
            }

            const bool extended = (command & 0x20U) != 0;
            command &= 0x1FU;
            std::uint16_t count = command;
            if (extended) {
                count = static_cast<std::uint16_t>((count << 8U) | stream.read_byte());
            }
            const std::size_t literal_count = count == 0 ? 65536U : count;
            for (std::size_t i = 0; i < literal_count; ++i) output.write(stream.read_byte());
        }

        if (stream.position() != block_end) {
            throw std::runtime_error("format-A command crossed block boundary");
        }
        if (stream.read_byte() == 0) return;
    }
}

std::size_t decode_match_length(BitReader& bits, Stream& stream) {
    if (bits.read_bit()) return 2;
    if (bits.read_bit()) return 3;
    if (bits.read_bit()) return 4;
    if (bits.read_bit()) return 5;
    if (bits.read_bit()) return static_cast<std::size_t>(bits.read_code(3)) + 6U;
    return static_cast<std::size_t>(stream.read_byte()) + 14U;
}

void decode_bit_stream(Stream& stream, Output& output) {
    for (;;) {
        stream.skip(3);
        BitReader bits(stream);

        for (;;) {
            if (!bits.read_bit()) {
                output.write(stream.read_byte());
                continue;
            }

            std::uint16_t distance = 0;
            if (!bits.read_bit()) {
                distance = stream.read_byte();
            } else {
                distance = static_cast<std::uint16_t>(
                    (bits.read_code(5) << 8U) | stream.read_byte());
                if (distance == 0) break;

                if (distance == 1) {
                    const bool extended = bits.read_bit();
                    std::uint16_t count_code = bits.read_code(4);
                    if (extended) {
                        count_code = static_cast<std::uint16_t>(
                            (count_code << 8U) | stream.read_byte());
                    }
                    const auto value = stream.read_byte();
                    const auto count = static_cast<std::size_t>(count_code) + 14U;
                    for (std::size_t i = 0; i < count; ++i) output.write(value);
                    continue;
                }
            }

            output.copy_match(distance, decode_match_length(bits, stream));
        }

        if (stream.read_byte() == 0) return;
    }
}

} // namespace

DecompressResult decompress_graphics(std::span<const std::uint8_t> source,
                                     std::span<std::uint8_t> destination) {
    if (source.size() < 4) throw std::runtime_error("compressed stream is too short");

    Stream stream(source);
    Output output(destination);
    if (source[2] != 0) decode_command_stream(stream, output);
    else decode_bit_stream(stream, output);
    return {stream.position(), output.size()};
}

} // namespace oasis::game
