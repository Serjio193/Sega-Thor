#include "tools/hybrid/gameplay_session.hpp"
#include "tools/hybrid/gameplay_session_identity.hpp"
#include "tools/hybrid/gameplay_session_receipts.hpp"
#include "tools/hybrid/grid_query_native_model.hpp"
#include "tools/hybrid/m68k_call_census.hpp"
#include "core/rom_identity.hpp"

#include <filesystem>
#include <array>
#include <fstream>
#include <stdexcept>

using namespace oasis::hybrid::session;

namespace {
std::array<std::uint8_t, 0x10000> mock_ram{};

int mock_peek(unsigned address) {
    if (address < 0xFF0000U || address > 0xFFFFFFU) return -1;
    return mock_ram[address - 0xFF0000U];
}

void mock_write(unsigned address, unsigned width, unsigned value) {
    for (unsigned i = 0; i < width; ++i) {
        const auto shift = (width - i - 1U) * 8U;
        mock_ram[address - 0xFF0000U + i] = static_cast<std::uint8_t>(value >> shift);
    }
}

void check(bool condition) {
    if (!condition) throw std::runtime_error("gameplay session test assertion failed");
}

template<class F> bool throws(F&& action) {
    try { action(); } catch (const std::runtime_error&) { return true; }
    return false;
}

void capture_replay_roundtrip(const std::filesystem::path& path) {
    InputJournal capture(InputJournal::Mode::Capture);
    capture.begin_poll(0, 0, 1U << 7, InputSource::InjectedScripted);
    check(capture.record_query({0, 1, 0, 7}, 1) == 1);
    check(capture.record_query({0, 1, 0, 6}, 0) == 0);
    capture.end_poll();
    capture.end_frame(0, 1);
    capture.begin_poll(2, 0, 1U << 3, InputSource::PhysicalOverride);
    check(capture.record_query({0, 1, 0, 3}, 1) == 1);
    capture.end_poll();
    capture.end_frame(2, 1);
    capture.finish();
    capture.save(path);

    auto replay = InputJournal::load(path);
    check(replay.begin_poll(0, 0) == (1U << 7));
    check(replay.record_query({0, 1, 0, 7}) == 1);
    check(replay.record_query({0, 1, 0, 6}) == 0);
    replay.end_poll();
    replay.end_frame(0, 1);
    check(replay.begin_poll(2, 0) == (1U << 3));
    check(replay.record_query({0, 1, 0, 3}) == 1);
    replay.end_poll();
    replay.end_frame(2, 1);
    replay.finish();
}

void rejects_changed_poll_or_query(const std::filesystem::path& path) {
    auto replay = InputJournal::load(path);
    check(throws([&] { replay.begin_poll(0, 1); }));
    replay = InputJournal::load(path);
    replay.begin_poll(0, 0);
    check(throws([&] { replay.record_query({0, 1, 0, 6}); }));
    replay = InputJournal::load(path);
    replay.begin_poll(0, 0);
    check(throws([&] { replay.record_query({0, 1, 0, 7}); replay.end_poll();
                       replay.begin_poll(0, 1); }));
}

void rejects_missing_and_extra_polls(const std::filesystem::path& path) {
    auto replay = InputJournal::load(path);
    check(throws([&] { replay.finish(); }));
    replay = InputJournal::load(path);
    replay.begin_poll(0, 0);
    replay.record_query({0, 1, 0, 7});
    replay.record_query({0, 1, 0, 6});
    replay.end_poll();
    replay.end_frame(0, 1);
    replay.begin_poll(2, 0);
    replay.record_query({0, 1, 0, 3});
    replay.end_poll();
    replay.end_frame(2, 1);
    check(throws([&] { replay.begin_poll(3, 0); }));
}

void rejects_malformed_log(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::trunc);
    out << "OASIS_INPUT_POLL_LOG 1\nP 0 1 0001 F 0\n";
    out.close();
    check(throws([&] { (void)InputJournal::load(path); }));
}

void checks_manifest_and_build_identity(const std::filesystem::path& path) {
    const std::string content = "stable-input\n";
    { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << content; }
    const std::vector<std::uint8_t> bytes(content.begin(), content.end());
    check(file_sha256(path) == oasis::calculate_sha256(bytes));
    check(config_identity(3600, "fallback") == config_identity(3600, "fallback"));
    check(config_identity(3600, "fallback") != config_identity(1800, "fallback"));
    const Manifest values{{"rom_sha256", "abc"}, {"frames", "3600"}};
    write_manifest(path, values);
    check(read_manifest(path) == values);
}

void checks_byte_grid_candidate() {
    mock_ram.fill(0);
    mock_write(0xFF16F0, 1, 0);
    mock_write(0xFF1766, 4, 0x00FF4000);
    mock_write(0xFF176E, 2, 6);
    mock_write(0xFF1770, 4, 0x00FF5000);
    mock_write(0xFF1778, 2, 6);
    mock_write(0xFF2008, 2, 16);
    mock_write(0xFF200C, 2, 24);
    mock_write(0xFF4000 + 3U * 64U + 2U, 1, 0);
    mock_write(0xFF5000 + 3U * 64U + 2U, 1, 0x80);
    std::array<unsigned, 8> d{};
    std::array<unsigned, 8> a{};
    d[5] = 0xA1B2C300;
    a[6] = 0xFF2000;
    auto result = oasis::hybrid::grid_query::predict(d, a, mock_peek);
    check(result.has_value());
    check(result->selector == 0 && result->cell_address == 0xFF40C2U);
    check(result->cell_value == 0 && result->d5 == 0xA1B2C300U);
    mock_write(0xFF16F0, 1, 0x80);
    result = oasis::hybrid::grid_query::predict(d, a, mock_peek);
    check(result.has_value());
    check(result->selector == 0x80 && result->descriptor == 0xFF1770U);
    check(result->cell_address == 0xFF50C2U && result->cell_value == 0x80);
    check(result->d5 == 0xA1B2C380U);
    mock_write(0xFF2008, 2, 512);
    check(!oasis::hybrid::grid_query::predict(d, a, mock_peek));
    mock_write(0xFF2008, 2, 0xFFFF);
    check(!oasis::hybrid::grid_query::predict(d, a, mock_peek));
}

void checks_call_transfer_decoding() {
    using oasis::hybrid::call_census::CpuState;
    using oasis::hybrid::call_census::Transfer;
    CpuState state;
    state.a[2] = 0x123456;
    Transfer transfer;
    check(oasis::hybrid::call_census::decode_transfer(
        0x1000, 0x2000, state, 0x6100, 0x0FFE, 0, transfer));
    check(transfer.kind == "BSR.W" && transfer.return_pc == 0x1004);
    check(oasis::hybrid::call_census::decode_transfer(
        0x1000, 0x1003, state, 0x6101, 0, 0, transfer));
    check(transfer.kind == "BSR.B" && transfer.return_pc == 0x1002);
    check(oasis::hybrid::call_census::decode_transfer(
        0x1000, 0x123456, state, 0x4EB9, 0, 0x00123456, transfer));
    check(transfer.kind == "JSR_ABS.L" && transfer.return_pc == 0x1006);
    check(oasis::hybrid::call_census::decode_transfer(
        0x1000, 0x123456, state, 0x4E92, 0, 0, transfer));
    check(transfer.kind == "JSR_(An)" && transfer.return_pc == 0x1002);
    check(!oasis::hybrid::call_census::decode_transfer(
        0x1000, 0x2000, state, 0x4EAA, 0, 0, transfer));
}

void checks_census_return_and_full_capture(const std::filesystem::path& base) {
    using namespace oasis::hybrid::call_census;
    M68kCallCensus census;
    for (unsigned call = 0; call < 25; ++call) {
        CpuState entry; entry.pc = 0x2000; entry.a[7] = 0xFF00FC; entry.d[3] = 0xAABBCCDD;
        census.execute(0x2000, 0x1000, 7, entry, 0x4E71, 0x6100, 0x0FFE, 0, 0x1004);
        CpuState rts = entry; rts.pc = 0x2002;
        census.execute(0x2002, 0x2000, 7, rts, 0x4E75, 0x4E71, 0, 0, 0);
        CpuState returned = rts; returned.pc = 0x1004; returned.a[7] += 4;
        census.execute(0x1004, 0x2002, 7, returned, 0x4E71, 0x4E75, 0, 0, 0);
    }
    const auto summary = base.string() + ".json";
    const auto captures = base.string() + ".jsonl";
    census.write(summary, captures);
    check(census.target_count() == 1 && census.full_capture_count() == 24);
    std::ifstream input(captures);
    const std::string line((std::istreambuf_iterator<char>(input)), {});
    check(line.find("0x001000") != std::string::npos);
    check(line.find("\"instruction_count\":2") != std::string::npos);
    input.close();
    std::filesystem::remove(summary);
    std::filesystem::remove(captures);
}

void checks_replay_receipt(const std::filesystem::path& base) {
    const auto path = base.string() + ".result";
    oasis::hybrid::session::write_replay_result(path, 3600, 3600,
        "input", "landmarks", "calls", "census", "full-calls");
    std::ifstream input(path);
    const std::string receipt((std::istreambuf_iterator<char>(input)), {});
    check(receipt.find("status=PASS\nframes=3600\npolls=3600") != std::string::npos);
    check(receipt.find("call_census_sha256=census") != std::string::npos);
    check(receipt.find("auto_full_calls_sha256=full-calls") != std::string::npos);
    input.close();
    std::filesystem::remove(path);
}
} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path() / "oasis-gameplay-session-test.polls";
    capture_replay_roundtrip(path);
    rejects_changed_poll_or_query(path);
    rejects_missing_and_extra_polls(path);
    rejects_malformed_log(path);
    checks_manifest_and_build_identity(path);
    checks_byte_grid_candidate();
    checks_call_transfer_decoding();
    checks_census_return_and_full_capture(path);
    checks_replay_receipt(path);
    std::filesystem::remove(path);
}
