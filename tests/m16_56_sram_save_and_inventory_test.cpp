#include "game/save/sram_save_system.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/world_graph.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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

void test_rom_magic_and_wram_layout(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::save;
    for (std::size_t i = 0; i < SramConfig::kMagicBytesLen; ++i) {
        OASIS_CHECK(rom[0x001754U + i] == SramConfig::kMagic[i]);
    }

    PersistentWramBlock blk{};
    blk.init_default_inventory();
    OASIS_CHECK(blk.equip_status_word() == 0x01FFU);
    OASIS_CHECK(blk.weapon_slot(0) == 0x05FEU);
    OASIS_CHECK(blk.weapon_slot(6) == 0x0BFEU);
    for (std::size_t i = 0; i < 4U; ++i) {
        OASIS_CHECK(blk.spirit_level(i) == 0x0FU);
    }

    blk.set_power_word(1234U);
    blk.set_item_slot(0U, 0x1AU, 3U);
    blk.set_spirit_slot_flags(0x01U);
    blk.set_current_room_id(0x0D01U);
    OASIS_CHECK(blk.power_word() == 1234U);
    OASIS_CHECK(blk.item_id(0U) == 0x1AU && blk.item_count(0U) == 3U);
    OASIS_CHECK(blk.spirit_slot_flags() == 0x01U);
    OASIS_CHECK(blk.current_room_id() == 0x0D01U);

    std::cout << "[PASS] Phase 1: ROM Magic (0x001754) & 596B WRAM Layout Verified.\n";
}

void test_sram_slots_and_backup_repair() {
    using namespace oasis::game::save;
    SramSaveBank bank{};
    OASIS_CHECK(bank.scan_slots() == 0U);

    PersistentWramBlock s0{};
    s0.init_default_inventory();
    s0.set_power_word(500U);
    s0.set_current_room_id(0x0106U);

    PersistentWramBlock s2{};
    s2.init_default_inventory();
    s2.set_power_word(999U);
    s2.set_current_room_id(0x0D00U);

    OASIS_CHECK(bank.save_slot(0U, s0));
    OASIS_CHECK(bank.save_slot(2U, s2));
    OASIS_CHECK(bank.scan_slots() == 0x05U); // bits 0 and 2
    OASIS_CHECK(bank.last_selected_slot() == 2U);

    // Corrupt Slot 0 Primary -> validate_region fails, load_slot repairs from Backup (0x0012E8)
    const std::size_t corrupt_off = static_cast<std::size_t>(SramConfig::kPrimarySlotAddrs[0] - SramConfig::kSramBaseAddr) + 20U;
    bank.raw_sram_mut()[corrupt_off] ^= 0xFFU;
    OASIS_CHECK(!bank.validate_region(SramConfig::kPrimarySlotAddrs[0]));

    PersistentWramBlock loaded{};
    bool repaired = false;
    OASIS_CHECK(bank.load_slot(0U, loaded, &repaired));
    OASIS_CHECK(repaired);
    OASIS_CHECK(loaded.power_word() == 500U && loaded.current_room_id() == 0x0106U);
    OASIS_CHECK(bank.validate_region(SramConfig::kPrimarySlotAddrs[0])); // Repaired!

    // Corrupt both Primary and Backup for Slot 2 -> load fails & scan clears bit 2
    const std::size_t p2 = static_cast<std::size_t>(SramConfig::kPrimarySlotAddrs[2] - SramConfig::kSramBaseAddr);
    const std::size_t b2 = static_cast<std::size_t>(SramConfig::kBackupSlotAddrs[2] - SramConfig::kSramBaseAddr);
    bank.raw_sram_mut()[p2] ^= 0x55U;
    bank.raw_sram_mut()[b2] ^= 0x55U;
    OASIS_CHECK(!bank.load_slot(2U, loaded, &repaired));
    OASIS_CHECK(bank.scan_slots() == 0x01U);

    std::cout << "[PASS] Phase 2: SRAM 4+4 Slots, Checksum (0x00129A) & Backup Repair (0x0012E8) Verified.\n";
}

void test_simulation_save_load_roundtrip(const std::vector<std::uint8_t>& rom) {
    using namespace oasis;
    auto sim1 = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim1->initialize_native_from_rom(rom, game::world::kBeachMapId));
    sim1->event_vm().set_event_flag(3U, true, 1U);   // Global Flag 3
    sim1->event_vm().set_event_flag(10U, true, 2U);  // Water Shrine Area Flag
    sim1->event_vm().set_active_spirit_bit(0U, true); // Dytto active

    game::save::PersistentWramBlock wram{};
    wram.init_default_inventory();
    game::save::capture_simulation_to_wram(*sim1, wram);

    game::save::SramSaveBank bank{};
    OASIS_CHECK(bank.save_slot(1U, wram));

    game::save::PersistentWramBlock loaded{};
    OASIS_CHECK(bank.load_slot(1U, loaded));

    auto sim2 = std::make_unique<game::StandaloneGameSimulation>();
    OASIS_CHECK(sim2->initialize_native_from_rom(rom, game::world::kBeachMapId));
    OASIS_CHECK(!sim2->event_vm().get_event_flag(3U, 1U));
    OASIS_CHECK(!sim2->dytto_controller().is_active());

    game::save::restore_wram_to_simulation(loaded, *sim2);
    OASIS_CHECK(sim2->event_vm().get_event_flag(3U, 1U));
    OASIS_CHECK(sim2->event_vm().get_event_flag(10U, 2U));
    OASIS_CHECK(sim2->event_vm().has_active_spirit(0U));
    OASIS_CHECK(sim2->dytto_controller().is_active());

    std::cout << "[PASS] Phase 3: StandaloneGameSimulation Save/Load Round-Trip Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.56: SRAM SAVE/LOAD & PERSISTENT INVENTORY BLOCK\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_rom_magic_and_wram_layout(rom);
    test_sram_slots_and_backup_repair();
    test_simulation_save_load_roundtrip(rom);
    std::cout << "\n[PASS] All M16.56 SRAM Save/Load & Inventory Tests PASSED!\n";
    return 0;
}
