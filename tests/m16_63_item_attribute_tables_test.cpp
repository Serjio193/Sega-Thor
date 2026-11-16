#include "game/event/event_vm.hpp"
#include "game/items/item_attribute_tables.hpp"
#include "game/items/item_effects.hpp"
#include "game/save/sram_save_system.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#define OASIS_CHECK(cond)                                                                      \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << std::endl;          \
            std::exit(1);                                                                      \
        }                                                                                      \
    } while (0)

namespace {

std::string resolve_path(const std::string& rel) {
    for (const auto* p : {"", "../", "../../", "../../../"}) {
        std::filesystem::path candidate = std::string(p) + rel;
        if (std::filesystem::exists(candidate)) return candidate.string();
    }
    return rel;
}

std::vector<std::uint8_t> read_binary(const std::string& p) {
    std::ifstream in(p, std::ios::binary | std::ios::ate);
    OASIS_CHECK(in.is_open());
    const auto sz = in.tellg();
    OASIS_CHECK(sz > 0);
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

void test_item_attributes_0x05CE16(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::items;

    OASIS_CHECK(get_item_attribute_byte(rom, 0x01U) == 0xA1U);
    OASIS_CHECK(is_item_non_discardable(rom, 0x01U));  // Knife
    OASIS_CHECK(get_item_attribute_byte(rom, 0x02U) == 0x01U);
    OASIS_CHECK(!is_item_non_discardable(rom, 0x02U)); // Sword
    OASIS_CHECK(is_item_non_discardable(rom, 0x0DU));  // 0xA2
    OASIS_CHECK(is_item_non_discardable(rom, 0x10U));  // 0xA2
    OASIS_CHECK(!is_item_non_discardable(rom, 0x1EU)); // 0x00 consumable
    OASIS_CHECK(is_item_non_discardable(rom, 0x31U));  // 0xA0 key item
    OASIS_CHECK(is_item_non_discardable(rom, 0x3FU));  // 0xA0 key item

    std::cout << "[PASS] Phase 1: ROM 0x05CE16 Item Attribute Table Verified.\n";
}

void test_pickup_tiers_and_flags(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::items;

    // Tier 0 (0x0E) -> 0xC0035F
    const auto t0 = resolve_pickup_outcome(rom, 0x0EU);
    OASIS_CHECK(t0.tier == 0U && !t0.is_fanfare && t0.sound_cmd == 0x00C0035FU);

    // Tier 1 (0x01) -> 0xC00360
    const auto t1 = resolve_pickup_outcome(rom, 0x01U);
    OASIS_CHECK(t1.tier == 1U && !t1.is_fanfare && t1.sound_cmd == 0x00C00360U);

    // Tier 3 (0x0D) -> Fanfare 0x0508 + State 0x44 / Action 0x4A
    const auto t3 = resolve_pickup_outcome(rom, 0x0DU);
    OASIS_CHECK(t3.tier == 3U && t3.is_fanfare && t3.sound_cmd == 0x00000508U);
    OASIS_CHECK(t3.player_state == 0x44U && t3.player_action == 0x4AU);

    // Tier 4 (0x12) -> Fanfare 0x050D + State 0x44 / Action 0x4A
    const auto t4 = resolve_pickup_outcome(rom, 0x12U);
    OASIS_CHECK(t4.tier == 4U && t4.is_fanfare && t4.sound_cmd == 0x0000050DU);
    OASIS_CHECK(t4.player_state == 0x44U && t4.player_action == 0x4AU);

    // Pickup persistent flag setter (0x007AF0)
    oasis::game::event::EventVm vm{};
    OASIS_CHECK(apply_pickup_persistent_flag(vm, 1U, 12U));
    OASIS_CHECK(apply_pickup_persistent_flag(vm, 2U, 5U));
    OASIS_CHECK(apply_pickup_persistent_flag(vm, 3U, 7U));
    OASIS_CHECK(!apply_pickup_persistent_flag(vm, 0U, 1U));
    OASIS_CHECK(!apply_pickup_persistent_flag(vm, 4U, 1U));
    OASIS_CHECK(vm.get_event_flag(12U, 1U) && vm.get_event_flag(5U, 2U) && vm.get_event_flag(7U, 3U));

    std::cout << "[PASS] Phase 2: Pickup Tiers (0x05CE56 / 0x007A6C) & Flag Setter (0x007AF0) Verified.\n";
}

void test_discard_guard(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;
    save::PersistentWramBlock wram{};
    wram.set_item_slot(0U, 0x0DU, 1U); // Non-discardable
    wram.set_item_slot(1U, 0x1EU, 2U); // Discardable consumable

    OASIS_CHECK(!items::discard_inventory_slot(wram, 0U, rom));
    OASIS_CHECK(wram.item_id(0U) == 0x0DU && wram.item_count(0U) == 1U);

    OASIS_CHECK(items::discard_inventory_slot(wram, 1U, rom));
    OASIS_CHECK(wram.item_id(1U) == 0U && wram.item_count(1U) == 0U);

    std::cout << "[PASS] Phase 3: Non-Discardable Key Item Guard (0x05CE16 bit 5) Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.63: ITEM ATTRIBUTE & PICKUP FANFARE TIER TABLES\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_item_attributes_0x05CE16(rom);
    test_pickup_tiers_and_flags(rom);
    test_discard_guard(rom);
    std::cout << "\n[PASS] All M16.63 Item Attribute & Pickup Tier Tests PASSED!\n";
    return 0;
}
