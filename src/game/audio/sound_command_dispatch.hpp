#pragma once

#include "game/combat/sound_command_queue.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::audio {

// ROM 0x060000..0x06000C & 0x06042A: Sound Driver Entrypoints & Command Categories.
enum class SoundCommandCategory : std::uint8_t {
    System = 0,
    Bgm,
    SfxBank2,
    SfxBank3,
    SfxBank4,
    Category5,
    Unknown
};

struct SoundDriverConfig {
    static constexpr std::uint32_t kEntryInitTick = 0x00060090U;     // 0x060000
    static constexpr std::uint32_t kEntryEnqueue = 0x0006042AU;      // 0x060004
    static constexpr std::uint32_t kEntryHardwareTick = 0x00061232U; // 0x060008

    static constexpr std::array<std::uint32_t, 9> kSystemHandlers = {{
        0x000604E6U, // 0x00
        0x00060A8AU, // 0x01
        0x0006064CU, // 0x02 (Fade/Stop BGM)
        0x0006074EU, // 0x03
        0x00060884U, // 0x04
        0x000609BCU, // 0x05
        0x000609C6U, // 0x06
        0x00060516U, // 0x07
        0x00060D10U, // 0x08
    }};

    static constexpr std::uint32_t kHandlerBgm = 0x00060B50U;       // 0x0100
    static constexpr std::uint32_t kHandlerSfxBank2 = 0x0006112EU;  // 0x0200 (priority in d0[23:16])
    static constexpr std::uint32_t kHandlerSfxBank3 = 0x00060F1CU;  // 0x0300 (e.g. 0x80034F, 0x800399)
    static constexpr std::uint32_t kHandlerSfxBank4 = 0x00060F54U;  // 0x0400 (e.g. 0x8004D9)
    static constexpr std::uint32_t kHandlerCategory5 = 0x00060CDAU; // 0x0500

    // ROM 0x0638D4: BGM Sequence Offset Table & Z80 Bus Handshake (0x060B50 / 0x060B68..0x060BC4).
    static constexpr std::uint32_t kBgmOffsetTableRom = 0x000638D4U;
    static constexpr std::uint32_t kZ80BusReqAddr = 0x00A11100U;
    static constexpr std::uint32_t kZ80CmdAddr = 0x00A00003U;
    static constexpr std::uint16_t kZ80BusRequestWord = 0x0100U;
    static constexpr std::uint8_t kZ80StopByte = 0x83U;
};

// ROM 0x0638D4 (Loader 0x060B50): Resolves `0x0638D4 + u32_be(0x0638D4 + track_id * 4)`.
// Returns std::nullopt if `rel == 0` or out of bounds.
[[nodiscard]] std::optional<std::uint32_t> lookup_bgm_sequence_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint8_t track_id) noexcept;

// Models Z80 Bus Handshake (0x060B68..0x060BC4):
// 1. Write 0x0100 to $00A11100 (Z80 bus request)
// 2. Write 0x83 to $00A00003 (stop byte)
// 3. Release Z80 bus ($00A11100 = 0x0000)
// 4. Poll $00A00003 == 0 until Z80 acknowledges ready
struct Z80BusHandshakeState {
    bool bus_requested{false};
    std::uint16_t bus_req_word{0U};
    std::uint8_t z80_cmd_byte{0U};
    bool awaiting_ack{false};
    std::uint32_t pending_bgm_rom_addr{0U};
    std::uint32_t completed_handshakes{0U};

    void reset() noexcept;
    void begin_stop_and_load(std::uint32_t bgm_rom_addr) noexcept;
    void release_bus() noexcept;
    void acknowledge_z80_ready() noexcept;
    [[nodiscard]] bool is_z80_ready() const noexcept {
        return z80_cmd_byte == 0U && !awaiting_ack;
    }
};

struct DecodedSoundCommand {
    std::uint32_t raw_cmd{0U};
    SoundCommandCategory category{SoundCommandCategory::Unknown};
    std::uint8_t priority{0U}; // d0[23:16]
    std::uint8_t sub_id{0U};   // d0[7:0]
    std::uint32_t target_pc{0U};
};

// Classifies a 32-bit `d0.L` sound/BGM command per ROM 0x06042A.
[[nodiscard]] DecodedSoundCommand classify_sound_command(std::uint32_t cmd_d0) noexcept;

// Models ROM sound driver state across 0x060090 (init/tick), 0x06042A (enqueue), 0x061232 (hw tick).
class SoundDriverState {
public:
    void reset() noexcept;

    void init_or_tick() noexcept;                                    // 0x060000 -> 0x060090
    DecodedSoundCommand enqueue_command(std::uint32_t cmd_d0) noexcept; // 0x060004 -> 0x06042A
    void hardware_tick() noexcept;                                   // 0x060008 -> 0x061232

    // Classifies and dispatches all commands in `queue`; when `rom` is provided, Bgm commands
    // (0x01xx) also resolve `last_bgm_rom_addr` via 0x0638D4 and initiate Z80 handshake.
    std::size_t dispatch_queue(
        const combat::SoundCommandQueue& queue,
        std::span<const std::uint8_t> rom = {}) noexcept;

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_count_; }
    [[nodiscard]] std::optional<std::uint8_t> active_bgm_id() const noexcept { return active_bgm_id_; }
    [[nodiscard]] std::optional<std::uint32_t> last_bgm_rom_addr() const noexcept { return last_bgm_rom_addr_; }
    [[nodiscard]] const Z80BusHandshakeState& z80_handshake() const noexcept { return z80_handshake_; }
    [[nodiscard]] Z80BusHandshakeState& z80_handshake() noexcept { return z80_handshake_; }
    [[nodiscard]] bool bgm_fading() const noexcept { return bgm_fading_; }
    [[nodiscard]] const std::vector<DecodedSoundCommand>& history() const noexcept { return history_; }

private:
    bool initialized_{false};
    std::uint64_t tick_count_{0U};
    std::optional<std::uint8_t> active_bgm_id_{};
    std::optional<std::uint32_t> last_bgm_rom_addr_{};
    Z80BusHandshakeState z80_handshake_{};
    bool bgm_fading_{false};
    std::vector<DecodedSoundCommand> history_{};
};

} // namespace oasis::game::audio
