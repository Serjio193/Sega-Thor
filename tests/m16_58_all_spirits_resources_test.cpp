#include "game/items/item_effects.hpp"
#include "game/save/sram_save_system.hpp"
#include "game/spirits/all_spirits_registry.hpp"
#include "genesis/vdp_state.hpp"

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

std::uint32_t read_be32(const std::vector<std::uint8_t>& rom, std::size_t off) {
    return (static_cast<std::uint32_t>(rom[off]) << 24U) |
           (static_cast<std::uint32_t>(rom[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(rom[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(rom[off + 3U]);
}

void test_rom_specs_and_dma_branches(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::spirits;

    for (std::uint8_t idx = 0; idx < 4U; ++idx) {
        const auto spec = get_spirit_spec(idx);
        OASIS_CHECK(spec.has_value());
        OASIS_CHECK(get_spirit_spec_by_type(spec->type_id).has_value());
        OASIS_CHECK(spec->vram_byte_addr == spec->vram_tile * 32U);
        // Verify ROM DMA branch starts with LEA <nemesis_rom_addr>.L, A0 (41 F9 <u32>)
        OASIS_CHECK(rom[spec->dma_branch_pc] == 0x41U && rom[spec->dma_branch_pc + 1U] == 0xF9U);
        OASIS_CHECK(read_be32(rom, spec->dma_branch_pc + 2U) == spec->nemesis_rom_addr);
    }

    OASIS_CHECK(!get_spirit_spec(4U).has_value());
    OASIS_CHECK(!get_spirit_spec_by_type(0x0020U).has_value());
    std::cout << "[PASS] Phase 1: All 4 Spirits ROM Specs & DMA Branches Verified.\n";
}

void test_all_spirits_vram_decompression(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game::spirits;

    for (std::uint8_t idx = 0; idx < 4U; ++idx) {
        const auto spec = get_spirit_spec(idx);
        OASIS_CHECK(spec.has_value());

        oasis::genesis::VdpState vdp{};
        vdp.vram.fill(0xAAU);
        const std::size_t bytes = upload_spirit_tiles_to_vram(rom, idx, vdp);
        OASIS_CHECK(bytes > 0U && (bytes % 32U) == 0U);
        OASIS_CHECK(vdp.vram[spec->vram_byte_addr - 1U] == 0xAAU);

        bool any_non_aa = false;
        for (std::size_t i = 0; i < bytes; ++i) {
            if (vdp.vram[spec->vram_byte_addr + i] != 0xAAU) {
                any_non_aa = true;
                break;
            }
        }
        OASIS_CHECK(any_non_aa);
    }

    std::cout << "[PASS] Phase 2: All 4 Spirits Nemesis VRAM Decompression Verified.\n";
}

void test_ring_summon_all_four_spirits(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;
    save::PersistentWramBlock wram{};
    wram.set_item_slot(0U, 0x1AU, 1U); // Dytto Ring
    wram.set_item_slot(1U, 0x1BU, 1U); // Efreet Ring
    wram.set_item_slot(2U, 0x1CU, 1U); // Shade Ring
    wram.set_item_slot(3U, 0x1DU, 1U); // Bow Ring

    spirits::ActiveSpiritController spirit_ctrl{};
    const std::uint16_t expected_types[4] = {0x0018U, 0x001AU, 0x001CU, 0x001EU};
    const std::uint16_t expected_tiles[4] = {0x04D0U, 0x0450U, 0x0280U, 0x0360U};

    for (std::size_t i = 0; i < 4U; ++i) {
        items::ItemUseContext ctx{};
        ctx.area_id = 0x0106U;
        ctx.sp = 40;
        ctx.max_sp = 100;
        ctx.unlocked_spirit_mask = 0x0FU;

        oasis::genesis::VdpState vdp{};
        const auto res = items::use_inventory_slot(wram, i, ctx, spirit_ctrl, 160, 120, rom, &vdp);
        OASIS_CHECK(res.applied && res.summon_requested && res.summon_spirit_index == i);
        OASIS_CHECK(spirit_ctrl.is_active());
        OASIS_CHECK(spirit_ctrl.spirit_index() == i);
        OASIS_CHECK(spirit_ctrl.type_id() == expected_types[i]);
        OASIS_CHECK(spirit_ctrl.vram_tile() == expected_tiles[i]);
        OASIS_CHECK(spirit_ctrl.max_sp() == 100U && spirit_ctrl.sp_drain_rate() == 10U);
        OASIS_CHECK(spirit_ctrl.vram_uploaded_bytes() > 0U);
        OASIS_CHECK(spirit_ctrl.dytto().is_active() == (i == 0U));
    }

    std::cout << "[PASS] Phase 3: Unified Ring Summon (0x1A..0x1D) for All 4 Spirits Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.58: ALL-SPIRITS REGISTRY, VRAM DMA & SUMMON INTEGRATION\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_rom_specs_and_dma_branches(rom);
    test_all_spirits_vram_decompression(rom);
    test_ring_summon_all_four_spirits(rom);
    std::cout << "\n[PASS] All M16.58 All-Spirits Registry & Summon Tests PASSED!\n";
    return 0;
}
