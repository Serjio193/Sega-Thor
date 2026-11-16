#include "game/audio/sound_command_dispatch.hpp"
#include "game/combat/sound_command_queue.hpp"

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

void test_rom_entrypoints(const std::vector<std::uint8_t>& rom) {
    using oasis::game::audio::SoundDriverConfig;

    // 0x060000..0x060008 contain 4E F9 (JMP abs.l) or BRA.W? Let's check:
    // Actually 0x060000: JMP/BRA.W to 0x060090, 0x06042A, 0x061232 (4-byte slots: 60 00 xx xx or 4E F9)
    // Verify the 4-byte branch targets at 0x060000, 0x060004, 0x060008 resolve to 0x060090, 0x06042A, 0x061232
    auto resolve_bra_w = [&](std::size_t pc) -> std::uint32_t {
        if (rom[pc] == 0x60U && rom[pc + 1U] == 0x00U) {
            const auto disp = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom[pc + 2U]) << 8U) | rom[pc + 3U]);
            return static_cast<std::uint32_t>(static_cast<std::int32_t>(pc) + 2 + disp);
        }
        return read_be32(rom, pc + 2U);
    };
    OASIS_CHECK(resolve_bra_w(0x060000U) == SoundDriverConfig::kEntryInitTick);
    OASIS_CHECK(resolve_bra_w(0x060004U) == SoundDriverConfig::kEntryEnqueue);
    OASIS_CHECK(resolve_bra_w(0x060008U) == SoundDriverConfig::kEntryHardwareTick);
    std::cout << "[PASS] Phase 1: Sound Driver ROM Entrypoints (0x060000..0x060008) Verified.\n";
}

void test_command_classification_and_dispatch() {
    using namespace oasis::game::audio;

    // System commands 0x00..0x08
    const auto sys2 = classify_sound_command(0x00000002U);
    OASIS_CHECK(sys2.category == SoundCommandCategory::System && sys2.sub_id == 0x02U && sys2.target_pc == 0x0006064CU);

    // BGM 0x0100
    const auto bgm = classify_sound_command(0x00000106U);
    OASIS_CHECK(bgm.category == SoundCommandCategory::Bgm && bgm.sub_id == 0x06U && bgm.target_pc == 0x00060B50U);

    // SFX Bank 2 (0xA00234 -> priority 0xA0, sub 0x34)
    const auto sfx2 = classify_sound_command(0x00A00234U);
    OASIS_CHECK(sfx2.category == SoundCommandCategory::SfxBank2 && sfx2.priority == 0xA0U && sfx2.sub_id == 0x34U);
    OASIS_CHECK(sfx2.target_pc == 0x0006112EU);

    // SFX Bank 3 (0x80034F, 0x800399)
    const auto sfx3 = classify_sound_command(0x0080034FU);
    OASIS_CHECK(sfx3.category == SoundCommandCategory::SfxBank3 && sfx3.priority == 0x80U && sfx3.sub_id == 0x4FU);
    OASIS_CHECK(sfx3.target_pc == 0x00060F1CU);

    // SFX Bank 4 (0x8004D9)
    const auto sfx4 = classify_sound_command(0x008004D9U);
    OASIS_CHECK(sfx4.category == SoundCommandCategory::SfxBank4 && sfx4.priority == 0x80U && sfx4.sub_id == 0xD9U);
    OASIS_CHECK(sfx4.target_pc == 0x00060F54U);

    // Category 5
    const auto cat5 = classify_sound_command(0x00000501U);
    OASIS_CHECK(cat5.category == SoundCommandCategory::Category5 && cat5.target_pc == 0x00060CDAU);

    // Unknown
    OASIS_CHECK(classify_sound_command(0x00000900U).category == SoundCommandCategory::Unknown);
    OASIS_CHECK(classify_sound_command(0x0000000FU).category == SoundCommandCategory::Unknown);

    // SoundDriverState + SoundCommandQueue dispatch
    SoundDriverState drv{};
    drv.init_or_tick();
    OASIS_CHECK(drv.initialized());
    drv.enqueue_command(0x00000106U);
    OASIS_CHECK(drv.active_bgm_id() == 0x06U && !drv.bgm_fading());
    drv.enqueue_command(0x00000002U);
    OASIS_CHECK(!drv.active_bgm_id().has_value() && drv.bgm_fading());

    oasis::game::combat::SoundCommandQueue q{};
    q.push(1, 0x0080034FU);
    q.push(2, 0x00800399U);
    q.push(3, 0x008004D9U);
    q.push(4, 0x00A00234U);
    OASIS_CHECK(drv.dispatch_queue(q) == 4U);
    OASIS_CHECK(drv.history().size() == 6U);
    drv.hardware_tick();
    OASIS_CHECK(drv.tick_count() == 1U);

    std::cout << "[PASS] Phase 2: Sound Command Classification & Queue Dispatch Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.61: SOUND DRIVER ENTRYPOINTS & COMMAND DISPATCHER\n"
              << "=====================================================================\n";
    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_rom_entrypoints(rom);
    test_command_classification_and_dispatch();
    std::cout << "\n[PASS] All M16.61 Sound Command Dispatch Tests PASSED!\n";
    return 0;
}
