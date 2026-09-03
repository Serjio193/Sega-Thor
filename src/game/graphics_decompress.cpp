#include "game/graphics_decompress.hpp"

#include <stdexcept>

namespace oasis::game {
namespace {

class Decoder {
public:
    Decoder(std::span<const std::uint8_t> source, std::span<std::uint8_t> destination)
        : source_(source), destination_(destination) {}

    DecompressResult run() {
        if (source_.size() < 4) fail("compressed stream is too short");
        if (source_[2] != 0) decode_command_stream();
        else decode_bit_stream();
        return {source_pos_, output_pos_};
    }

private:
    [[noreturn]] static void fail(const char* message) { throw std::runtime_error(message); }

    std::uint8_t read_byte() {
        if (source_pos_ >= source_.size()) fail("compressed stream read past end");
        return source_[source_pos_++];
    }

    void write_byte(std::uint8_t value) {
        if (output_pos_ >= destination_.size()) fail("decompressed output exceeds destination");
        destination_[output_pos_++] = value;
    }

    void copy_literals(std::size_t count) {
        while (count-- != 0) write_byte(read_byte());
    }

    void repeat_byte(std::uint8_t value, std::size_t count) {
        while (count-- != 0) write_byte(value);
    }

    void copy_match(std::size_t distance, std::size_t count) {
        if (distance == 0 || distance > output_pos_) fail("invalid graphics back-reference");
        while (count-- != 0) {
            const auto value = destination_[output_pos_ - distance];
            write_byte(value);
        }
    }

    void decode_command_stream() {
        for (;;) {
            const std::size_t block_start = source_pos_;
            const std::size_t block_end = block_start +
                static_cast<std::size_t>(read_byte()) +
                (static_cast<std::size_t>(read_byte()) << 8U);
            if (block_end < source_pos_ || block_end > source_.size()) fail("invalid command block length");

            while (source_pos_ < block_end) {
                const std::uint8_t command = read_byte();
                if ((command & 0x80U) != 0) {
                    const std::uint8_t code = command & 0x7fU;
                    const std::size_t count = ((code & 0x60U) >> 5U) + 4U;
                    const std::size_t distance =
                        (static_cast<std::size_t>(code & 0x1fU) << 8U) | read_byte();
                    copy_match(distance, count);

                    if (source_pos_ < block_end && (source_[source_pos_] & 0xe0U) == 0x60U) {
                        const std::size_t extension = read_byte() & 0x1fU;
                        copy_match(distance, extension);
                    }
                } else if ((command & 0x40U) != 0) {
                    std::uint16_t count = command & 0x3fU;
                    if ((count & 0x10U) != 0) {
                        count &= static_cast<std::uint16_t>(~0x10U);
                        count = static_cast<std::uint16_t>((count << 8U) | read_byte());
                    }
                    repeat_byte(read_byte(), static_cast<std::size_t>(count) + 4U);
                } else {
                    std::uint16_t count = command;
                    if ((count & 0x20U) != 0) {
                        count &= static_cast<std::uint16_t>(~0x20U);
                        count = static_cast<std::uint16_t>((count << 8U) | read_byte());
                    }
                    if (count == 0) fail("zero-length literal command");
                    copy_literals(count);
                }
            }

            if (source_pos_ != block_end) fail("command block overrun");
            if (read_byte() == 0) return;
        }
    }

    bool read_bit() {
        if (bits_left_ == 0) {
            const std::uint16_t low = read_byte();
            const std::uint16_t high = read_byte();
            bits_ = static_cast<std::uint16_t>(low | (high << 8U));
            bits_left_ = 16;
        }
        const bool bit = (bits_ & 1U) != 0;
        bits_ >>= 1U;
        --bits_left_;
        return bit;
    }

    std::uint16_t read_bits(unsigned count) {
        std::uint16_t value = 0;
        while (count-- != 0) value = static_cast<std::uint16_t>((value << 1U) | read_bit());
        return value;
    }

    void start_bit_block() {
        source_pos_ += 3;
        if (source_pos_ >= source_.size()) fail("bit-stream header exceeds source");
        bits_ = read_byte();
        bits_left_ = 8;
    }

    std::size_t decode_match_length() {
        std::size_t count = 2;
        if (read_bit()) return count;
        ++count;
        if (read_bit()) return count;
        ++count;
        if (read_bit()) return count;
        ++count;
        if (read_bit()) return count;

        if (!read_bit()) return static_cast<std::size_t>(read_byte()) + 14U;
        return static_cast<std::size_t>(read_bits(3)) + 6U;
    }

    void decode_bit_stream() {
        start_bit_block();
        for (;;) {
            if (!read_bit()) {
                write_byte(read_byte());
                continue;
            }

            std::uint16_t distance = 0;
            if (!read_bit()) {
                distance = read_byte();
            } else {
                distance = static_cast<std::uint16_t>((read_bits(5) << 8U) | read_byte());
                if (distance == 0) {
                    if (read_byte() == 0) return;
                    start_bit_block();
                    continue;
                }
                if (distance == 1) {
                    const bool extended = read_bit();
                    std::uint16_t count = read_bits(4);
                    if (extended) count = static_cast<std::uint16_t>((count << 8U) | read_byte());
                    copy_literals(static_cast<std::size_t>(count) + 14U);
                    continue;
                }
            }

            copy_match(distance, decode_match_length());
        }
    }

    std::span<const std::uint8_t> source_;
    std::span<std::uint8_t> destination_;
    std::size_t source_pos_{};
    std::size_t output_pos_{};
    std::uint16_t bits_{};
    unsigned bits_left_{};
};

} // namespace

DecompressResult decompress_graphics(std::span<const std::uint8_t> source,
                                     std::span<std::uint8_t> destination) {
    return Decoder(source, destination).run();
}

} // namespace oasis::game
