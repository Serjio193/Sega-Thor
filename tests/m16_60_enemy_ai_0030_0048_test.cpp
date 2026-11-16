#include "game/enemy/enemy_ai_0030_0048.hpp"
#include "game/entity/native_entity.hpp"

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

void test_rom_aabb_table(const std::vector<std::uint8_t>& rom) {
    using oasis::game::enemy::Charger0048Ai;
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::size_t off = Charger0048Ai::kAabbTableRom + i * 4U;
        OASIS_CHECK(static_cast<std::int8_t>(rom[off + 0U]) == Charger0048Ai::kDirectionalAabbs[i].dx1);
        OASIS_CHECK(static_cast<std::int8_t>(rom[off + 1U]) == Charger0048Ai::kDirectionalAabbs[i].dy1);
        OASIS_CHECK(static_cast<std::int8_t>(rom[off + 2U]) == Charger0048Ai::kDirectionalAabbs[i].dx2);
        OASIS_CHECK(static_cast<std::int8_t>(rom[off + 3U]) == Charger0048Ai::kDirectionalAabbs[i].dy2);
    }
    std::cout << "[PASS] Phase 1: ROM 0x01783E Directional AABB Table Verified.\n";
}

void test_charger_0048_state_machine() {
    using namespace oasis::game;
    enemy::Charger0048Ai ai{};
    NativeEntity ent{};
    ent.active = true;
    ent.type_id = 0x0048U;
    ent.world_x = 160;
    ent.world_y = 120;
    ent.facing = 1U; // Right

    // Substate 0 -> 1 (saves home)
    ai.step(ent, 160, 200);
    OASIS_CHECK(ai.substate() == 1U && ai.home_x() == 160 && ai.home_y() == 120 && ai.action() == 0U);

    // Player outside right AABB -> stays in substate 1
    ai.step(ent, 100, 120);
    OASIS_CHECK(ai.substate() == 1U);

    // Player inside right AABB -> triggers charge (substate 2, action 2, SFX 0x800399)
    ai.step(ent, 200, 120);
    OASIS_CHECK(ai.substate() == 2U && ai.action() == 2U);
    OASIS_CHECK(ai.last_sound_cmd() == 0x00800399U);

    // Run through substate 2 -> flips facing (1 -> 3) and enters substate 3
    for (int f = 0; f < 16; ++f) {
        ai.step(ent, 200, 120);
    }
    OASIS_CHECK(ai.substate() == 3U && ent.facing == 3U && ent.world_x > 160);

    // Return home in substate 3 -> flips facing back (3 -> 1) and enters substate 1
    for (int f = 0; f < 64; ++f) {
        ai.step(ent, 160, 200);
        if (ai.substate() == 1U) break;
    }
    OASIS_CHECK(ai.substate() == 1U && ent.world_x == 160 && ent.world_y == 120 && ent.facing == 1U);

    std::cout << "[PASS] Phase 2: Entity 0x0048 Charger 4-Substate AI (0x01778C) Verified.\n";
}

void test_gate_boss_0030_state_machine() {
    using namespace oasis::game;
    enemy::GateBoss0030Ai ai{};
    NativeEntity boss{};
    boss.active = true;
    boss.type_id = 0x0030U;
    boss.hp = 50;
    boss.world_x = 160;
    boss.world_y = 80;

    std::vector<NativeEntity> children;
    ai.step(boss, children);
    OASIS_CHECK(ai.substate() == 1U);
    OASIS_CHECK(children.size() == 5U);
    for (std::size_t i = 0; i < 5U; ++i) {
        OASIS_CHECK(children[i].active && children[i].type_id == 0x0030U);
        OASIS_CHECK(children[i].tile_base == 0x4200U);
        OASIS_CHECK(children[i].anim_frame == (((i % 2U) == 0U) ? 0x3AU : 0x3CU));
    }

    ai.step(boss, children);
    OASIS_CHECK(ai.substate() == 2U);

    boss.hp = 0;
    ai.step(boss, children);
    OASIS_CHECK(ai.substate() == 3U && !boss.active);
    for (const auto& c : children) {
        OASIS_CHECK(!c.active);
    }

    std::cout << "[PASS] Phase 3: Entity 0x0030 Gate Boss AI (0x015AE2) Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.60: ENEMY AI CONTRACTS (0x0048 CHARGER & 0x0030 BOSS)\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_rom_aabb_table(rom);
    test_charger_0048_state_machine();
    test_gate_boss_0030_state_machine();
    std::cout << "\n[PASS] All M16.60 Enemy AI Contract Tests PASSED!\n";
    return 0;
}
