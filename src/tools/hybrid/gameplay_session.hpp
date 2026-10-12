#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace oasis::hybrid::session {

enum class InputSource : std::uint8_t {
    FallbackSchedule = 0,
    PhysicalOverride = 1,
    InjectedScripted = 2,
};

struct InputQuery {
    std::uint8_t port{};
    std::uint8_t device{};
    std::uint16_t index{};
    std::uint16_t id{};
    std::int16_t value{};
};

struct InputPoll {
    std::uint64_t frame{};
    std::uint32_t index{};
    std::uint16_t buttons{};
    InputSource source{InputSource::FallbackSchedule};
    std::vector<InputQuery> queries;
};

class InputJournal final {
public:
    enum class Mode { Capture, Replay };

    explicit InputJournal(Mode mode);
    static InputJournal load(const std::filesystem::path& path);

    std::uint16_t begin_poll(std::uint64_t frame, std::uint32_t index,
                             std::uint16_t capture_buttons = 0,
                             InputSource capture_source = InputSource::FallbackSchedule);
    std::int16_t record_query(InputQuery query, std::int16_t capture_value = 0);
    void end_poll();
    void end_frame(std::uint64_t frame, std::uint32_t poll_count);
    void finish();
    void save(const std::filesystem::path& path) const;

    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] const std::vector<InputPoll>& polls() const noexcept { return polls_; }

private:
    InputJournal(Mode mode, std::vector<InputPoll> polls);
    Mode mode_;
    std::vector<InputPoll> polls_;
    std::optional<InputPoll> active_;
    std::size_t replay_cursor_{};
    std::size_t query_cursor_{};
    std::uint64_t last_frame_{};
    std::uint32_t expected_poll_index_{};
    bool have_frame_{};
};

[[nodiscard]] const char* input_source_name(InputSource source) noexcept;

} // namespace oasis::hybrid::session
