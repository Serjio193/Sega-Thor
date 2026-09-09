#include "tools/hybrid/checkpoint_evidence.hpp"
#include "tools/hybrid/gpgx_checkpoint_layout.hpp"
#include "core/rom_identity.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> read_bytes(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

int replay_raw_records(const char* path) {
    const auto bytes = read_bytes(path);
    constexpr std::size_t state_size = 0xfd000;
    assert(bytes.size() % state_size == 0);
    std::string hashes;
    for (std::size_t offset = 0; offset < bytes.size(); offset += state_size) {
        hashes += oasis::hybrid::checkpoint_identity_hash(
            std::span<const std::uint8_t>(bytes.data() + offset, state_size));
    }
    std::cout << oasis::calculate_sha256({reinterpret_cast<const std::uint8_t*>(hashes.data()),
                                           hashes.size()}) << '\n';
    return 0;
}

} // namespace

int main() {
    if (const auto* replay = std::getenv("OASIS_REPLAY_CHECKPOINTS")) return replay_raw_records(replay);
    const auto& layout = oasis::hybrid::gpgx_checkpoint_layout();
    assert(layout.state_size == 0xfd000);
    assert(layout.ym2612_offset == 140652);
    assert(layout.ym2612_size == 3576);
    assert(layout.z80_offset == 144504);
    assert(layout.z80_size == 88);
    assert(layout.state_version == "GENPLUS-GX 1.7.6");

    std::size_t pointer_count = 0;
    std::size_t function_count = 0;
    for (const auto& span : layout.representation_spans) {
        assert(span.size != 0);
        assert(span.offset + span.size <= layout.state_size);
        if (span.kind == oasis::hybrid::CheckpointRepresentationKind::HostPointer) ++pointer_count;
        if (span.kind == oasis::hybrid::CheckpointRepresentationKind::FunctionPointer) ++function_count;
    }
    for (std::size_t i = 1; i < layout.representation_spans.size(); ++i) {
        const auto& previous = layout.representation_spans[i - 1];
        const auto& current = layout.representation_spans[i];
        assert(previous.offset + previous.size <= current.offset);
    }
    assert(pointer_count == 55);
    assert(function_count == 1);

    auto find = [&](std::size_t offset) {
        return std::find_if(layout.representation_spans.begin(), layout.representation_spans.end(),
                            [&](const auto& span) {
                                return span.offset <= offset && offset < span.offset + span.size;
                            });
    };
    assert(find(140652) != layout.representation_spans.end());
    assert(find(140732) != layout.representation_spans.end());
    assert(find(140734) != layout.representation_spans.end());
    assert(find(144576) != layout.representation_spans.end());
    assert(find(144584) != layout.representation_spans.end());

    std::vector<std::uint8_t> baseline(layout.state_size);
    const std::string version(layout.state_version);
    std::copy(version.begin(), version.end(), baseline.begin());
    const auto untouched = baseline;
    const auto before = oasis::hybrid::checkpoint_identity_hash(baseline);
    assert(oasis::hybrid::checkpoint_identity_hash(baseline) == before);
    assert(baseline == untouched);
    for (const auto& span : layout.representation_spans) {
        auto changed = baseline;
        changed[span.offset + span.size - 1] = 0xa5;
        assert(oasis::hybrid::checkpoint_identity_hash(changed) == before);
    }
    auto semantic = baseline;
    semantic[140732 + 8] = 0xa5;
    assert(oasis::hybrid::checkpoint_identity_hash(semantic) != before);

    auto unknown_version = baseline;
    unknown_version[0] = 'X';
    bool rejected = false;
    try { (void)oasis::hybrid::checkpoint_identity_hash(unknown_version); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    return 0;
}
