#include "game/entities/entity_pool.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

void write_word(std::span<std::uint8_t> bytes,
                std::size_t offset,
                std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_long(std::span<std::uint8_t> bytes,
                std::size_t offset,
                std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
    bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

} // namespace

int main() {
    using oasis::game::entities::EntityPoolView;
    using oasis::game::entities::kEntityFieldCounter9C;
    using oasis::game::entities::kEntityFieldPointer22;
    using oasis::game::entities::kEntityPoolAtFf19e8;
    using oasis::game::entities::kEntityPoolAtFf2954;
    using oasis::game::entities::kEntityPoolAtFf2d8c;

    std::array<std::uint8_t, 21U * 0xBCU> main_storage{};
    write_word(main_storage, 0, 2);
    write_word(main_storage, 20U * 0xBCU, 0x7FFF);
    const EntityPoolView main_pool(main_storage, kEntityPoolAtFf19e8);
    assert(main_pool.active(0));
    assert(main_pool.active(20));
    assert(!main_pool.active(1));
    assert(main_pool.record(1)->size() == 0xBCU);
    assert(main_pool.record(21) == std::nullopt);
    write_long(main_storage, 0x22U, 0x00059D9CU);
    write_word(main_storage, 0x9CU, 0x1234);
    const auto main_view = main_pool.record_view(0);
    assert(main_view);
    assert(main_view->pointer22() == 0x00059D9CU);
    assert(main_view->read_u16(kEntityFieldCounter9C) == 0x1234);
    assert(!main_view->read_u32(0xBCU));

    std::array<std::uint8_t, 4U * 0x5AU> auxiliary_storage{};
    write_word(auxiliary_storage, 2U * 0x5AU, 1);
    const EntityPoolView auxiliary_pool(auxiliary_storage,
                                        kEntityPoolAtFf2954);
    assert(auxiliary_pool.active(2));
    assert(!auxiliary_pool.active(3));
    assert(auxiliary_pool.spec().movement_entry_address == 0x8F12);
    assert(auxiliary_pool.record_view(0)->read_u32(kEntityFieldCounter9C) ==
           std::nullopt);

    std::array<std::uint8_t, 6U * 0x5AU> secondary_storage{};
    const EntityPoolView secondary_pool(secondary_storage, kEntityPoolAtFf2d8c);
    assert(secondary_pool.spec().record_count == 6);
    assert(secondary_pool.spec().record_stride == 0x5A);
    assert(!secondary_pool.active(0));
    return 0;
}
