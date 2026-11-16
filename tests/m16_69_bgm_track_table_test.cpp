#include "game/audio/sound_command_dispatch.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void write_be32(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t v) {
    buf[off] = static_cast<std::uint8_t>((v >> 24U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>((v >> 16U) & 0xFFU);
    buf[off + 2U] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    buf[off + 3U] = static_cast<std::uint8_t>(v & 0xFFU);
}

} // namespace

int main() {
    using oasis::game::audio::lookup_bgm_sequence_rom_addr;
    using oasis::game::audio::SoundDriverConfig;
    using oasis::game::audio::SoundDriverState;
    using oasis::game::audio::Z80BusHandshakeState;
    using oasis::game::combat::SoundCommandQueue;

    // 1. Synthetic ROM matching 0x0638D4 table layout (track 0x00 -> +0x000400 = 0x063CD4,
    //    track 0x01 -> +0x000C3A = 0x06450E, track 0x02 -> rel 0 -> std::nullopt).
    std::vector<std::uint8_t> rom(0x00080000U, 0x00U);
    write_be32(rom, SoundDriverConfig::kBgmOffsetTableRom + 0U * 4U, 0x00000400U);
    write_be32(rom, SoundDriverConfig::kBgmOffsetTableRom + 1U * 4U, 0x00000C3AU);
    write_be32(rom, SoundDriverConfig::kBgmOffsetTableRom + 2U * 4U, 0x00000000U);

    const auto t0 = lookup_bgm_sequence_rom_addr(rom, 0x00U);
    assert(t0.has_value() && *t0 == 0x00063CD4U);

    const auto t1 = lookup_bgm_sequence_rom_addr(rom, 0x01U);
    assert(t1.has_value() && *t1 == 0x0006450EU);

    assert(!lookup_bgm_sequence_rom_addr(rom, 0x02U).has_value());
    assert(!lookup_bgm_sequence_rom_addr({}, 0x00U).has_value());

    // 2. Z80 Bus Handshake lifecycle (0x060B68..0x060BC4).
    Z80BusHandshakeState z80;
    assert(z80.is_z80_ready());
    z80.begin_stop_and_load(0x00063CD4U);
    assert(z80.bus_requested);
    assert(z80.bus_req_word == 0x0100U);
    assert(z80.z80_cmd_byte == 0x83U);
    assert(z80.awaiting_ack);
    assert(!z80.is_z80_ready());

    z80.release_bus();
    assert(!z80.bus_requested);
    assert(z80.bus_req_word == 0U);
    assert(z80.awaiting_ack);

    z80.acknowledge_z80_ready();
    assert(z80.is_z80_ready());
    assert(z80.completed_handshakes == 1U);

    // 3. SoundDriverState::dispatch_queue resolves last_bgm_rom_addr and initiates Z80 handshake.
    SoundDriverState driver;
    SoundCommandQueue queue;
    queue.push(10, 0x00000100U); // BGM Track 0x00 at frame 10
    assert(driver.dispatch_queue(queue, rom) == 1U);
    assert(driver.active_bgm_id().has_value() && *driver.active_bgm_id() == 0x00U);
    assert(driver.last_bgm_rom_addr().has_value() && *driver.last_bgm_rom_addr() == 0x00063CD4U);
    assert(driver.z80_handshake().z80_cmd_byte == 0x83U);
    assert(driver.z80_handshake().pending_bgm_rom_addr == 0x00063CD4U);

    return 0;
}
