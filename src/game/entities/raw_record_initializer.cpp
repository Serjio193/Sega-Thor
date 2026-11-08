#include "game/entities/raw_record_initializer.hpp"

namespace oasis::game::entities {
namespace {
void write_word(std::span<std::uint8_t> bytes, std::size_t offset,
                std::uint16_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value);
}
} // namespace

bool initialize_raw_record_fields(std::span<std::uint8_t> record,
                                  std::uint16_t input_word) noexcept {
    if (record.size() < 0x38U) return false;
    write_word(record, 0x2AU, input_word);
    write_word(record, 0x2CU, 0xFFFFU);
    record[0x37U] = static_cast<std::uint8_t>(record[0x37U] & 0xF9U);
    write_word(record, 0x30U, 0U);
    write_word(record, 0x2EU, 0U);
    return true;
}

} // namespace oasis::game::entities
