#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace oasis::game::video {

struct VintConfig {
    static constexpr std::uint32_t kVintEntryAddr = 0x00001F76U;
    static constexpr std::uint32_t kDmaFlushAddr = 0x00002112U;
    static constexpr std::uint32_t kDmaEmitterAddr = 0x000027FCU;
    static constexpr std::uint8_t kDmaFlushFlagBit = 0x02U; // $FF164E bit 1
    static constexpr std::uint32_t kSoftResetComboFF165E = 0xE0000202U;
    static constexpr std::uint32_t kSoftResetTargetAddr = 0x0000020EU;
    static constexpr std::size_t kInputRingSize = 64U;
    static constexpr std::uint8_t kInputRingByteMask = 0x7EU;
    static constexpr std::uint8_t kInputRingMaxCount = 0x3FU; // 63
};

struct VdpDmaEntry {
    std::uint32_t src_addr_words{0U};
    std::uint16_t vram_dst{0U};
    std::uint16_t word_len{0U};
};

// ROM 0x002112: LIFO VDP DMA queue flushed from tail ($FF1892) down to base ($FF10B4)
// when $FF164E bit 1 is set.
class VdpDmaQueue {
public:
    void reset() noexcept;
    void enqueue(const VdpDmaEntry& entry);
    void set_flush_enabled(bool enabled) noexcept;

    // Flushes queued entries in LIFO order if $FF164E bit 1 is set, then clears queue & bit 1.
    [[nodiscard]] std::vector<VdpDmaEntry> flush_if_enabled();

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] std::uint8_t flags_ff164e() const noexcept { return flags_ff164e_; }

private:
    std::vector<VdpDmaEntry> entries_{};
    std::uint8_t flags_ff164e_{0U};
};

// ROM 0x0021B2: 64-word input history ring ($FF1668), reverse-indexed by (index - 2) & 0x7E
// with count clamped to 0x3F (63).
class InputHistoryRing64 {
public:
    void reset() noexcept;
    void record(std::uint16_t input_word) noexcept;

    // Returns input recorded `ago` frames ago (0 = most recent), or std::nullopt if unavailable.
    [[nodiscard]] std::optional<std::uint16_t> frames_ago(std::size_t ago) const noexcept;

    [[nodiscard]] std::uint8_t byte_index() const noexcept { return byte_index_; }
    [[nodiscard]] std::uint8_t count() const noexcept { return count_; }
    void clear_count() noexcept { count_ = 0U; }

    // ROM 0x00217C: Checks $FF165E.L == 0xE0000202 (A+B+C+Start soft reset).
    [[nodiscard]] static constexpr bool is_soft_reset_combo(std::uint32_t input_ff165e_l) noexcept {
        return input_ff165e_l == VintConfig::kSoftResetComboFF165E;
    }

private:
    std::array<std::uint16_t, VintConfig::kInputRingSize> ring_{};
    std::uint8_t byte_index_{0U}; // $FF16E8 (even byte offset 0..0x7E)
    std::uint8_t count_{0U};      // $FF16EA (clamped to 0x3F)
};

// ROM 0x0021EC: V-Int frame counter ($FF1656) and two countdown timers ($FF1658, $FF1654).
struct VintFrameTimers {
    std::uint32_t frame_counter{0U}; // $FF1656
    std::uint16_t countdown_1{0U};   // $FF1658
    std::uint16_t countdown_2{0U};   // $FF1654

    void reset() noexcept;
    void step_frame() noexcept;
};

} // namespace oasis::game::video
