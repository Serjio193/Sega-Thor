#include "game/combat/elemental_damage.hpp"
#include "game/entity/native_entity.hpp"
#include "game/entity/rom_entity_tables.hpp"
#include "game/spirits/dytto_abilities.hpp"

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

void test_descriptor_table(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::entity;

    const auto ali = load_rom_entity_descriptor(rom, 0x0002U);
    OASIS_CHECK(ali.has_value());
    OASIS_CHECK(ali->descriptor_addr == 0x03F2FAU);
    OASIS_CHECK(ali->z_radius_4a == 48U);
    OASIS_CHECK(ali->xy_radius_46 == 10U);
    OASIS_CHECK(ali->dim_x_48 == 10U && ali->dim_y_49 == 10U);
    OASIS_CHECK(ali->render_prio_3d == 16U);
    OASIS_CHECK(ali->anim_ptr_1a == 0x0013CDFCU);

    const auto dytto_proj = load_rom_entity_descriptor(rom, 0x0024U);
    OASIS_CHECK(dytto_proj.has_value());
    OASIS_CHECK(dytto_proj->descriptor_addr == 0x03F2FAU - 0x20U + (0x0024U << 4U));

    const auto guard = load_rom_entity_descriptor(rom, 0x002CU);
    OASIS_CHECK(guard.has_value());
    oasis::game::NativeEntity ent{};
    apply_descriptor_to_entity(*guard, ent);
    OASIS_CHECK(ent.type_id == 0x002CU);
    OASIS_CHECK(ent.height == static_cast<std::int16_t>(guard->z_radius_4a));

    OASIS_CHECK(load_rom_entity_descriptor(rom, 0x00C6U).has_value());
    OASIS_CHECK(!load_rom_entity_descriptor(rom, 0x0000U).has_value());
    OASIS_CHECK(!load_rom_entity_descriptor(rom, 0x0003U).has_value());
    OASIS_CHECK(!load_rom_entity_descriptor(rom, 0x00C8U).has_value());

    std::cout << "[PASS] Phase 1: ROM 32-Byte Entity Descriptor Table (0x03F2FA) Verified.\n";
}

void test_update_dispatch_table(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::entity;

    // End sentinel RTS (0x4E75) immediately after 0x00C6 at 0x00E0B8
    OASIS_CHECK(rom[0x00E0B8] == 0x4E && rom[0x00E0B9] == 0x75);

    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0016U) == 0x000115C8U);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0018U) == 0x00011B4EU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0020U) == 0x0000E20AU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0024U) == 0x00014848U);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x002CU) == 0x000254EAU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0034U) == 0x0000EA04U);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x003CU) == 0x00017D12U);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0048U) == 0x0001778CU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x004CU) == 0x00016DCEU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x0060U) == 0x000178EAU);
    OASIS_CHECK(lookup_entity_update_handler(rom, 0x00C6U) == 0x00016CBEU);

    OASIS_CHECK(!lookup_entity_update_handler(rom, 0x0014U).has_value());
    OASIS_CHECK(!lookup_entity_update_handler(rom, 0x0025U).has_value());
    OASIS_CHECK(!lookup_entity_update_handler(rom, 0x00C8U).has_value());

    std::cout << "[PASS] Phase 2: ROM Entity Update Dispatch Table (0x00DF54..0x00E0B8) Verified.\n";
}

void test_elemental_damage_and_dytto_wiring() {
    using namespace oasis::game;

    // 1. Water Elemental bit-13 (0x2008) with defense 3 -> 5 damage, flags39 bit 3, SFX 0x8004D9
    NativeEntity ent{};
    ent.active = true;
    ent.hp = 20;
    ent.defense = 3;
    ent.damage_mailbox = 0x2008U;
    const auto r1 = combat::apply_universal_elemental_damage(ent);
    OASIS_CHECK(r1.had_damage && r1.elemental_bit_13);
    OASIS_CHECK(r1.applied_damage == 5U && r1.hp_after == 15);
    OASIS_CHECK(ent.hp == 15 && ent.damage_mailbox == 0U);
    OASIS_CHECK((ent.flags39 & 0x08U) != 0U);
    OASIS_CHECK(ent.last_sound_cmd == 0x008004D9U);
    OASIS_CHECK(!r1.died);

    // 2. Minimum damage floor max(1, dmg - defense)
    ent.damage_mailbox = 0x2004U;
    ent.defense = 10;
    const auto r2 = combat::apply_universal_elemental_damage(ent);
    OASIS_CHECK(r2.applied_damage == 1U && ent.hp == 14);

    // 3. Lethal damage -> death State 2, timer 0x10, $38 bit 3, invulnerable (0x008EF8)
    ent.hp = 5;
    ent.defense = 0;
    ent.damage_mailbox = 0xA008U;
    const auto r3 = combat::apply_universal_elemental_damage(ent);
    OASIS_CHECK(r3.died && ent.hp == 0);
    OASIS_CHECK(ent.state == 0x02U && ent.action_timer == 0x10U);
    OASIS_CHECK((ent.flags38 & 0x08U) != 0U && ent.invulnerable);

    // 4. DyttoProjectile (0x0024) wired through 0x017E30 pipeline
    std::vector<spirits::DyttoProjectile> projs;
    spirits::spawn_dytto_spread_projectiles(projs, 100, 100, 0, 0U);
    std::vector<NativeEntity> enemies(1);
    enemies[0].active = true;
    enemies[0].type_id = 0x0048U;
    enemies[0].hp = 20;
    enemies[0].defense = 2;
    enemies[0].world_x = 130;
    enemies[0].world_y = 100;
    enemies[0].world_z = 0;
    for (int f = 0; f < 15; ++f) {
        spirits::step_dytto_projectiles(projs, enemies, true);
    }
    OASIS_CHECK(enemies[0].damage_mailbox == 0U);
    OASIS_CHECK(enemies[0].hp == 14); // 8 - 2 = 6 damage
    OASIS_CHECK((enemies[0].flags39 & 0x08U) != 0U);

    std::cout << "[PASS] Phase 3: Universal Damage & Elemental Bit-13 Pipeline (0x017E30) Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.55: ROM ENTITY MASTER TABLES & ELEMENTAL DAMAGE\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_descriptor_table(rom);
    test_update_dispatch_table(rom);
    test_elemental_damage_and_dytto_wiring();
    std::cout << "\n[PASS] All M16.55 Entity Master Tables & Elemental Damage Tests PASSED!\n";
    return 0;
}
