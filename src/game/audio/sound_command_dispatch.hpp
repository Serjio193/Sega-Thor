#pragma once

#include "game/combat/sound_command_queue.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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

    // Classifies and dispatches all commands in `queue`; returns count dispatched.
    std::size_t dispatch_queue(const combat::SoundCommandQueue& queue) noexcept;

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_count_; }
    [[nodiscard]] std::optional<std::uint8_t> active_bgm_id() const noexcept { return active_bgm_id_; }
    [[nodiscard]] bool bgm_fading() const noexcept { return bgm_fading_; }
    [[nodiscard]] const std::vector<DecodedSoundCommand>& history() const noexcept { return history_; }

private:
    bool initialized_{false};
    std::uint64_t tick_count_{0U};
    std::optional<std::uint8_t> active_bgm_id_{};
    bool bgm_fading_{false};
    std::vector<DecodedSoundCommand> history_{};
};

} // namespace oasis::game::audio
