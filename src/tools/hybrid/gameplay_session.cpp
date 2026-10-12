#include "tools/hybrid/gameplay_session.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace oasis::hybrid::session {
namespace {
std::string source_token(InputSource source) {
    switch (source) {
    case InputSource::FallbackSchedule: return "FALLBACK_SCHEDULE";
    case InputSource::PhysicalOverride: return "PHYSICAL_CONTROLLER_OVERRIDE";
    case InputSource::InjectedScripted: return "INJECTED_SCRIPTED";
    }
    throw std::runtime_error("invalid input source");
}

InputSource parse_source(const std::string& token) {
    if (token == "FALLBACK_SCHEDULE") return InputSource::FallbackSchedule;
    if (token == "PHYSICAL_CONTROLLER_OVERRIDE") return InputSource::PhysicalOverride;
    if (token == "INJECTED_SCRIPTED") return InputSource::InjectedScripted;
    throw std::runtime_error("invalid input source token");
}

std::string poll_line(const InputPoll& poll) {
    std::ostringstream out;
    out << "P " << poll.frame << ' ' << poll.index << ' ' << std::hex << std::setw(4)
        << std::setfill('0') << poll.buttons << std::dec << ' ' << source_token(poll.source)
        << ' ' << poll.queries.size();
    for (const auto& query : poll.queries)
        out << ' ' << static_cast<unsigned>(query.port) << ':'
            << static_cast<unsigned>(query.device) << ':' << query.index << ':' << query.id
            << '=' << query.value;
    return out.str();
}

InputQuery parse_query(const std::string& token) {
    std::istringstream in(token);
    unsigned port{}, device{}, index{}, id{};
    int value{};
    char colon1{}, colon2{}, colon3{}, equals{};
    if (!(in >> port >> colon1 >> device >> colon2 >> index >> colon3 >> id >> equals >> value) ||
        colon1 != ':' || colon2 != ':' || colon3 != ':' || equals != '=' || port > 255 || device > 255 ||
        index > 65535 || id > 65535 || value < -32768 || value > 32767 ||
        in.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("malformed input query");
    return {static_cast<std::uint8_t>(port), static_cast<std::uint8_t>(device),
            static_cast<std::uint16_t>(index), static_cast<std::uint16_t>(id),
            static_cast<std::int16_t>(value)};
}
} // namespace

const char* input_source_name(InputSource source) noexcept {
    switch (source) {
    case InputSource::FallbackSchedule: return "FALLBACK_SCHEDULE";
    case InputSource::PhysicalOverride: return "PHYSICAL_CONTROLLER_OVERRIDE";
    case InputSource::InjectedScripted: return "INJECTED_SCRIPTED";
    }
    return "UNKNOWN";
}

InputJournal::InputJournal(Mode mode) : mode_(mode) {}
InputJournal::InputJournal(Mode mode, std::vector<InputPoll> polls)
    : mode_(mode), polls_(std::move(polls)) {}

InputJournal InputJournal::load(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open input journal");
    std::string line;
    if (!std::getline(in, line) || line != "OASIS_INPUT_POLL_LOG 1")
        throw std::runtime_error("unsupported input journal header");
    std::vector<InputPoll> polls;
    std::uint64_t prior_frame{};
    std::uint32_t prior_index{};
    bool have_prior{};
    while (std::getline(in, line)) {
        if (line.empty()) throw std::runtime_error("blank input journal record");
        std::istringstream row(line);
        char record{};
        InputPoll poll;
        std::string mask, source;
        std::size_t query_count{};
        if (!(row >> record >> poll.frame >> poll.index >> mask >> source >> query_count) || record != 'P')
            throw std::runtime_error("malformed input journal record");
        if (query_count > 1024) throw std::runtime_error("input query count exceeds safety bound");
        std::size_t used{};
        const auto buttons = std::stoul(mask, &used, 16);
        if (used != mask.size() || buttons > 0xFFFF) throw std::runtime_error("invalid input mask");
        poll.buttons = static_cast<std::uint16_t>(buttons);
        poll.source = parse_source(source);
        poll.queries.reserve(query_count);
        std::string token;
        for (std::size_t i = 0; i < query_count; ++i) {
            if (!(row >> token)) throw std::runtime_error("truncated query list");
            const auto query = parse_query(token);
            if (query.port == 0 && query.device == 1 && query.id < 16 &&
                query.value != ((buttons & (1U << query.id)) ? 1 : 0))
                throw std::runtime_error("digital query value conflicts with complete poll state");
            poll.queries.push_back(query);
        }
        if (row >> token) throw std::runtime_error("trailing input journal data");
        if (have_prior && (poll.frame < prior_frame ||
            (poll.frame == prior_frame && poll.index != prior_index + 1) ||
            (poll.frame > prior_frame && poll.index != 0)))
            throw std::runtime_error("input poll sequence is not contiguous");
        if (!have_prior && poll.index != 0) throw std::runtime_error("first poll index is not zero");
        prior_frame = poll.frame;
        prior_index = poll.index;
        have_prior = true;
        polls.push_back(std::move(poll));
    }
    return InputJournal(Mode::Replay, std::move(polls));
}

std::uint16_t InputJournal::begin_poll(std::uint64_t frame, std::uint32_t index,
                                       std::uint16_t capture_buttons,
                                       InputSource capture_source) {
    if (active_) throw std::runtime_error("input poll began before prior poll closed");
    if (have_frame_ && (frame < last_frame_ ||
        (frame == last_frame_ && index != expected_poll_index_) ||
        (frame > last_frame_ && index != 0)))
        throw std::runtime_error("input poll frame/order mismatch");
    if (!have_frame_ && index != 0) throw std::runtime_error("first frame poll index is not zero");
    if (!have_frame_ || frame != last_frame_) expected_poll_index_ = 0;
    last_frame_ = frame;
    have_frame_ = true;
    expected_poll_index_ = index + 1;
    if (mode_ == Mode::Capture) {
        active_ = InputPoll{frame, index, capture_buttons, capture_source, {}};
        return capture_buttons;
    }
    if (replay_cursor_ >= polls_.size()) throw std::runtime_error("extra input poll during replay");
    const auto& expected = polls_[replay_cursor_];
    if (expected.frame != frame || expected.index != index)
        throw std::runtime_error("input poll frame/order differs from recording");
    active_ = expected;
    query_cursor_ = 0;
    return expected.buttons;
}

std::int16_t InputJournal::record_query(InputQuery query, std::int16_t capture_value) {
    if (!active_) throw std::runtime_error("input query without preceding poll");
    if (mode_ == Mode::Capture) {
        query.value = capture_value;
        active_->queries.push_back(query);
        return capture_value;
    }
    else {
        if (query_cursor_ >= active_->queries.size())
            throw std::runtime_error("input query order differs from recording");
        const auto& expected = active_->queries[query_cursor_];
        if (expected.port != query.port || expected.device != query.device ||
            expected.index != query.index || expected.id != query.id)
            throw std::runtime_error("input query order differs from recording");
        ++query_cursor_;
        return expected.value;
    }
}

void InputJournal::end_poll() {
    if (!active_) throw std::runtime_error("input poll ended without start");
    if (mode_ == Mode::Capture) polls_.push_back(std::move(*active_));
    else {
        if (query_cursor_ != active_->queries.size()) throw std::runtime_error("missing input query during replay");
        ++replay_cursor_;
    }
    active_.reset();
}

void InputJournal::end_frame(std::uint64_t frame, std::uint32_t poll_count) {
    if (active_) end_poll();
    if (mode_ == Mode::Replay && replay_cursor_ < polls_.size() && polls_[replay_cursor_].frame <= frame)
        throw std::runtime_error("missing input poll during replay");
    if (mode_ == Mode::Capture) {
        const auto begin = polls_.size() - poll_count;
        for (std::size_t i = 0; i < poll_count; ++i)
            if (polls_[begin + i].frame != frame || polls_[begin + i].index != i)
                throw std::runtime_error("captured poll/frame accounting mismatch");
    }
}

void InputJournal::finish() {
    if (active_) end_poll();
    if (mode_ == Mode::Replay && replay_cursor_ != polls_.size())
        throw std::runtime_error("recorded input polls remain after session end");
}

void InputJournal::save(const std::filesystem::path& path) const {
    if (mode_ != Mode::Capture || active_) throw std::runtime_error("input journal is not complete capture data");
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write input journal");
    out << "OASIS_INPUT_POLL_LOG 1\n";
    for (const auto& poll : polls_) out << poll_line(poll) << '\n';
    if (!out) throw std::runtime_error("input journal write failed");
}

} // namespace oasis::hybrid::session
