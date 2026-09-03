#include "game/graphics_decompression.hpp"

#include <cstdint>
#include <stdexcept>

namespace oasis::game {
namespace {

class Stream {
public:
    explicit Stream(std::span<const std::uint8_t> source) : source_(source) {}

    [[nodiscard]] std::size_t position() const noexcept { return position_; }

    [[nodiscard]] std::uint8_t peek_u8() const {
        if (position_ >= source_.size()) {
            throw std::runtime_error("graphics stream truncated while peeking");
        }
        return source_[position_];
    }

    std::uint8_t read_u8() {
        if (position_ >= source_.size()) {
            throw std::runtime_error("graphics stream truncated");
        }
        return source_[position_++];
    }

    void skip(std::size_t count) {
        if (count > source_.size() - position_) {
            throw std::runtime_error("graphics stream truncated while skipping header");
        }
        position_ += count;
    }

private:
    std::span<const std::uint8_t> source_;
    std::size_t position_{};
};

class Output {
public:
    explicit Output(std::span<std::uint8_t> destination) : destination_(destination) {}

    [[nodiscard]] std::size_t size() const noexcept { return position_; }

    void write(std::uint8_t value) {
        if (position_ >= destination_.size()) {
            throw std::runtime_error("graphics decompression output overflow");
        }
        destination_[position_++] = value;
    }

    void copy_back(std::size_t distance, std::size_t count) {
        if (distance == 0 || distance > position_) {
            throw std::runtime_error("invalid graphics backreference distance");
        }
        for (std::size_t i = 0; i < count; ++i) {
            write(destination_[position_ - distance]);
        }
    }

private:
    std::span<std::uint8_t> destination_;
    std::size_t position_{};
};

class BitReader {
public:
    explicit BitReader(Stream& stream) : stream_(stream), bits_(stream_.read_u8()), remaining_(8) {}

    bool read_bit() {
        --remaining_;
        if (remaining_ < 0) {
            const auto low = stream_.read_u8();
            const auto high = stream_.read_u8();
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
        for (unsigned i = 0; i < count; ++i) {
            value = static_cast<std::uint16_t>((value << 1U) | (read_bit() ? 1U : 0U));
        }
        return value;
    }

private:
    Stream& stream_;
    std::uint16_t bits_{};
    int remaining_{};
};

std::size_t dbf_count(std::uint16_t counter) {
    return static_cast<std::size_t>(counter) + 1U;
}

void decompress_format_a(Stream& stream, Output& output) {
    for (;;) {
        const auto block_start = stream.position();
        const std::uint16_t block_span = static_cast<std::uint16_t>(
            stream.read_u8() | static_cast<std::uint16_t>(stream.read_u8() << 8U));
        const auto block_end = block_start + block_span;

        for (;;) {
            if (stream.position() == block_end) break;
            if (stream.position() > block_end) {
                throw std::runtime_error("format-A command crossed block boundary");
            }

            std::uint16_t command = stream.read_u8();
            const bool backreference = (command & 0x80U) != 0;
            command &= 0x7FU;

            if (!backreference) {
                const bool repeat = (command & 0x40U) != 0;
                command &= 0x3FU;

                if (!repeat) {
                    const bool extended = (command & 0x20U) != 0;
                    command &= 0x1FU;
                    std::uint16_t count = command;
                    if (extended) {
                        count = static_cast<std::uint16_t>((count << 8U) | stream.read_u8());
                    }
                    const std::size_t bytes = count == 0 ? 65536U : count;
                    for (std::size_t i = 0; i < bytes; ++i) output.write(stream.read_u8());
                    continue;
                }

                const bool extended = (command & 0x10U) != 0;
                command &= static_cast<std::uint16_t>(~0x10U);
                std::uint16_t counter = command;
                if (extended) {
                    counter = static_cast<std::uint16_t>((counter << 8U) | stream.read_u8());
                }
                counter = static_cast<std::uint16_t>(counter + 3U);
                const auto value = stream.read_u8();
                for (std::size_t i = 0; i < dbf_count(counter); ++i) output.write(value);
                continue;
            }

            const std::uint16_t length_counter = static_cast<std::uint16_t>(
                ((command & 0x60U) >> 5U) + 3U);
            const std::uint16_t distance = static_cast<std::uint16_t>(
                ((command & 0x1FU) << 8U) | stream.read_u8());
            output.copy_back(distance, dbf_count(length_counter));

            while (stream.position() != block_end &&
                   (stream.peek_u8() & 0xE0U) == 0x60U) {
                const auto extension = static_cast<std::uint8_t>(stream.read_u8() & 0x1FU);
                const std::size_t count = extension == 0 ? 256U : extension;
                output.copy_back(distance, count);
            }
        }

        if (stream.read_u8() == 0) return;
    }
}

std::size_t decode_backref_length(BitReader& bits, Stream& stream) {
    if (bits.read_bit()) return 2;
    if (bits.read_bit()) return 3;
    if (bits.read_bit()) return 4;
    if (bits.read_bit()) return 5;
    if (bits.read_bit()) return static_cast<std::size_t>(bits.read_code(3)) + 6U;
    return static_cast<std::size_t>(stream.read_u8()) + 14U;
}

void decompress_format_b(Stream& stream, Output& output) {
    for (;;) {
        stream.skip(3);
        BitReader bits(stream);

        for (;;) {
            if (!bits.read_bit()) {
                output.write(stream.read_u8());
                continue;
            }

            std::uint16_t distance = 0;
            if (!bits.read_bit()) {
                distance = stream.read_u8();
            } else {
                distance = static_cast<std::uint16_t>((bits.read_code(5) << 8U) | stream.read_u8());
                if (distance == 0) break;
                if (distance == 1) {
                    const bool extended = bits.read_bit();
                    std::uint16_t count_code = bits.read_code(4);
                    if (extended) {
                        count_code = static_cast<std::uint16_t>((count_code << 8U) | stream.read_u8());
                    }
                    const auto count = static_cast<std::size_t>(count_code) + 14U;
                    const auto value = stream.read_u8();
                    for (std::size_t i = 0; i < count; ++i) output.write(value);
                    continue;
                }
            }

            output.copy_back(distance, decode_backref_length(bits, stream));
        }

        if (stream.read_u8() == 0) return;
    }
}

} // namespace

DecompressResult decompress_graphics(
    std::span<const std::uint8_t> source,
    std::span<std::uint8_t> destination) {
    if (source.size() < 3) throw std::runtime_error("graphics stream too small");

    Stream stream(source);
    Output output(destination);
    if (source[2] != 0) {
        decompress_format_a(stream, output);
    } else {
        decompress_format_b(stream, output);
    }
    return {stream.position(), output.size()};
}

} // namespace oasis::game
