#include "runtime_rom_properties.hpp"

#include "core/rom_identity.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace oasis::thor::runtime_map {
namespace {

constexpr std::string_view MAGIC = "OASROMP1";
constexpr std::uint32_t VERSION = 1;
constexpr std::size_t HASH_HEX_SIZE = 64;
std::atomic<std::uint64_t> temp_counter{};

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t n) {
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(n >> (i * 8U)));
}
void put_u64(std::vector<std::uint8_t>& out, std::uint64_t n) {
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(n >> (i * 8U)));
}
void put_text(std::vector<std::uint8_t>& out, const std::string& value) {
    if (value.size() > UINT32_MAX) throw std::length_error("metadata field too large");
    put_u32(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& data) : data_(data) {}
    std::uint32_t u32() {
        need(4); std::uint32_t n = 0;
        for (unsigned i = 0; i < 4; ++i) n |= std::uint32_t(data_[pos_++]) << (i * 8U);
        return n;
    }
    std::uint64_t u64() {
        need(8); std::uint64_t n = 0;
        for (unsigned i = 0; i < 8; ++i) n |= std::uint64_t(data_[pos_++]) << (i * 8U);
        return n;
    }
    std::string text() {
        const auto length = u32(); need(length);
        std::string value(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
                          data_.begin() + static_cast<std::ptrdiff_t>(pos_ + length));
        pos_ += length; return value;
    }
    std::size_t position() const noexcept { return pos_; }
private:
    void need(std::size_t n) const {
        if (n > data_.size() - (std::min)(pos_, data_.size())) throw std::runtime_error("truncated ROM property file");
    }
    const std::vector<std::uint8_t>& data_;
    std::size_t pos_{};
};

void append_identity(std::vector<std::uint8_t>& out, const Identity& id) {
    put_text(out, id.rom_sha256); put_u64(out, id.rom_size); put_text(out, id.schema_id);
    put_text(out, id.contract_sha256); put_text(out, id.core_build_id); put_text(out, id.run_id);
    put_u64(out, id.generation); put_u64(out, id.capabilities); put_text(out, id.validation_state);
}
Identity read_identity(Reader& in) {
    Identity id;
    id.rom_sha256 = in.text(); id.rom_size = in.u64(); id.schema_id = in.text();
    id.contract_sha256 = in.text(); id.core_build_id = in.text(); id.run_id = in.text();
    id.generation = in.u64(); id.capabilities = in.u64(); id.validation_state = in.text();
    validate_identity(id); return id;
}
std::string sha256(std::span<const std::uint8_t> data) { return calculate_sha256(data); }

std::vector<std::uint8_t> encode(const Identity& id, const std::vector<std::uint16_t>& map) {
    validate_identity(id);
    if (id.rom_size != map.size()) throw std::invalid_argument("ROM size and property map differ");
    std::vector<std::uint8_t> out(MAGIC.begin(), MAGIC.end());
    put_u32(out, VERSION); append_identity(out, id); put_u64(out, map.size());
    for (const auto properties : map) {
        if (properties & ~ALL_PROPERTIES) throw std::invalid_argument("unknown ROM property bit");
        out.push_back(static_cast<std::uint8_t>(properties));
        out.push_back(static_cast<std::uint8_t>(properties >> 8U));
    }
    const auto digest = sha256(out);
    out.insert(out.end(), digest.begin(), digest.end());
    return out;
}

void durable_replace(const std::filesystem::path& path, const std::vector<std::uint8_t>& data) {
    static constexpr char digits[] = "0123456789abcdef";
    auto temp = path;
    const auto suffix = temp_counter.fetch_add(1, std::memory_order_relaxed) ^
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    temp += ".tmp.";
    for (int shift = 60; shift >= 0; shift -= 4) temp += digits[(suffix >> shift) & 15U];
#ifdef _WIN32
    {
        std::ofstream file(temp, std::ios::binary | std::ios::out);
        if (!file) throw std::runtime_error("cannot create ROM property checkpoint temp file");
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        file.flush();
        if (!file) { file.close(); std::filesystem::remove(temp); throw std::runtime_error("checkpoint write failed"); }
    }
    HANDLE handle = CreateFileW(temp.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE || !FlushFileBuffers(handle)) {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        std::filesystem::remove(temp); throw std::runtime_error("checkpoint flush failed");
    }
    CloseHandle(handle);
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp); throw std::runtime_error("atomic checkpoint replacement failed");
    }
#else
    const int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) throw std::runtime_error("cannot create ROM property checkpoint temp file");
    std::size_t offset = 0;
    while (offset < data.size()) {
        const auto wrote = ::write(fd, data.data() + offset, data.size() - offset);
        if (wrote <= 0) { ::close(fd); ::unlink(temp.c_str()); throw std::runtime_error("checkpoint write failed"); }
        offset += static_cast<std::size_t>(wrote);
    }
    const int flush_status = ::fsync(fd);
    const int close_status = ::close(fd);
    if (flush_status != 0 || close_status != 0) {
        ::unlink(temp.c_str()); throw std::runtime_error("checkpoint flush failed");
    }
    if (::rename(temp.c_str(), path.c_str()) != 0) {
        ::unlink(temp.c_str()); throw std::runtime_error("atomic checkpoint replacement failed");
    }
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
    const int dir = ::open(parent.c_str(), O_RDONLY);
    if (dir >= 0) { (void)::fsync(dir); ::close(dir); }
#endif
}

} // namespace

PropertyMap::PropertyMap(std::size_t rom_size)
    : properties_(rom_size), dirty_((rom_size + DIRTY_PAGE_BYTES - 1) / DIRTY_PAGE_BYTES) {
    if (!rom_size) throw std::invalid_argument("ROM size must be positive");
}
std::uint16_t PropertyMap::at(std::size_t offset) const { return properties_.at(offset); }
void PropertyMap::clear_dirty() noexcept { std::fill(dirty_.begin(), dirty_.end(), 0); }
void PropertyMap::apply_validated(std::uint64_t start, std::uint64_t length, std::uint16_t property) {
    if (!property || (property & ~ALL_PROPERTIES) || start > size() || length > size() - start)
        throw std::invalid_argument("invalid proven ROM property span");
    const auto end = static_cast<std::size_t>(start + length);
    for (auto i = static_cast<std::size_t>(start); i < end; ++i) {
        const auto merged = static_cast<std::uint16_t>(properties_[i] | property);
        if (merged != properties_[i]) { properties_[i] = merged; dirty_[i / DIRTY_PAGE_BYTES] = 1; }
    }
}
void PropertyMap::merge_validated(const PropertyMap& other) {
    if (size() != other.size()) throw std::invalid_argument("ROM property map size mismatch");
    for (std::size_t i = 0; i < size(); ++i) {
        const auto merged = static_cast<std::uint16_t>(properties_[i] | other.properties_[i]);
        if (merged != properties_[i]) { properties_[i] = merged; dirty_[i / DIRTY_PAGE_BYTES] = 1; }
    }
}
std::vector<Range> PropertyMap::ranges() const {
    std::vector<Range> out;
    std::size_t start = 0;
    while (start < size()) {
        const auto value = properties_[start];
        auto end = start + 1;
        while (end < size() && properties_[end] == value) ++end;
        out.push_back({start, end, value}); start = end;
    }
    return out;
}
PropertyMap PropertyMap::from_ranges(std::size_t rom_size, const std::vector<Range>& ranges) {
    PropertyMap out(rom_size); std::uint64_t cursor = 0;
    for (const auto& range : ranges) {
        if (range.start != cursor || range.end <= range.start || range.end > rom_size ||
            (range.properties & ~ALL_PROPERTIES)) throw std::invalid_argument("range map has gap, overlap, bounds, or property error");
        std::fill(out.properties_.begin() + static_cast<std::ptrdiff_t>(range.start),
                  out.properties_.begin() + static_cast<std::ptrdiff_t>(range.end), range.properties);
        cursor = range.end;
    }
    if (cursor != rom_size) throw std::invalid_argument("range map does not cover full ROM");
    return out;
}

void validate_identity(const Identity& id) {
    const auto hex64 = [](const std::string& s) {
        return s.size() == HASH_HEX_SIZE && std::all_of(s.begin(), s.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    };
    const bool valid_run_id = !id.run_id.empty() && id.run_id.size() <= 128 &&
        std::all_of(id.run_id.begin(), id.run_id.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '-' || c == '_';
        });
    if (!hex64(id.rom_sha256) || !id.rom_size || id.schema_id != SCHEMA_ID ||
        id.contract_sha256 != CONTRACT_SHA256 || !hex64(id.core_build_id) || id.run_id.empty() ||
        !valid_run_id ||
        (id.capabilities & ~ALL_PROPERTIES) ||
        (id.validation_state != "VALIDATED" && id.validation_state != "PARTIAL"))
        throw std::invalid_argument("incomplete or invalid ROM property identity");
}
void require_compatible(const Identity& a, const Identity& b) {
    validate_identity(a); validate_identity(b);
    if (a.rom_sha256 != b.rom_sha256 || a.rom_size != b.rom_size || a.schema_id != b.schema_id ||
        a.contract_sha256 != b.contract_sha256 || a.core_build_id != b.core_build_id ||
        a.capabilities != b.capabilities || a.validation_state != b.validation_state)
        throw std::invalid_argument("ROM property run identities are incompatible");
}
PropertyMap rebuild_union(const std::vector<Contribution>& runs, const Identity& target) {
    validate_identity(target); PropertyMap out(static_cast<std::size_t>(target.rom_size));
    for (const auto& run : runs) {
        require_compatible(target, run.identity);
        if (run.properties.size() != target.rom_size) throw std::invalid_argument("run map size mismatch");
        PropertyMap one(run.properties.size());
        for (std::size_t i = 0; i < run.properties.size(); ++i) {
            const auto p = run.properties[i];
            if (p & ~ALL_PROPERTIES) throw std::invalid_argument("unknown property in run");
            if (p) one.apply_validated(i, 1, p);
        }
        out.merge_validated(one);
    }
    return out;
}
std::vector<std::uint8_t> encode_checkpoint(const Identity& identity, const PropertyMap& map) {
    return encode(identity, map.bytes());
}
PropertyMap decode_checkpoint(const std::vector<std::uint8_t>& file, Identity& identity) {
    if (file.size() < MAGIC.size() + 4 + HASH_HEX_SIZE ||
        !std::equal(MAGIC.begin(), MAGIC.end(), file.begin())) throw std::runtime_error("invalid ROM property checkpoint header");
    const auto payload_size = file.size() - HASH_HEX_SIZE;
    const std::string stored(file.begin() + static_cast<std::ptrdiff_t>(payload_size), file.end());
    if (sha256(std::span(file.data(), payload_size)) != stored) throw std::runtime_error("ROM property checkpoint checksum mismatch");
    std::vector<std::uint8_t> body(file.begin() + static_cast<std::ptrdiff_t>(MAGIC.size()),
                                   file.begin() + static_cast<std::ptrdiff_t>(payload_size));
    Reader in(body);
    if (in.u32() != VERSION) throw std::runtime_error("unsupported ROM property checkpoint version");
    identity = read_identity(in);
    const auto count = in.u64();
    if (count != identity.rom_size || count > ((std::numeric_limits<std::size_t>::max)() / 2) ||
        body.size() - in.position() != count * 2) throw std::runtime_error("ROM property payload size mismatch");
    PropertyMap map(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        const auto p = static_cast<std::uint16_t>(body[in.position() + i * 2] |
            (std::uint16_t(body[in.position() + i * 2 + 1]) << 8U));
        if (p & ~ALL_PROPERTIES) throw std::runtime_error("checkpoint contains unknown property bits");
        if (p) map.apply_validated(i, 1, p);
    }
    map.clear_dirty(); return map;
}
void save_checkpoint(const std::filesystem::path& path, const Identity& identity, const PropertyMap& map) {
    durable_replace(path, encode_checkpoint(identity, map));
}
PropertyMap load_checkpoint(const std::filesystem::path& path, Identity& identity) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open ROM property checkpoint");
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
    if (!file.eof() && file.fail()) throw std::runtime_error("checkpoint read failed");
    return decode_checkpoint(bytes, identity);
}
void save_contribution(const std::filesystem::path& path, const Contribution& contribution) {
    PropertyMap map(contribution.properties.size());
    for (std::size_t i = 0; i < contribution.properties.size(); ++i)
        if (contribution.properties[i]) map.apply_validated(i, 1, contribution.properties[i]);
    durable_replace(path, encode_checkpoint(contribution.identity, map));
}
Contribution load_contribution(const std::filesystem::path& path) {
    Identity identity; auto map = load_checkpoint(path, identity);
    return {std::move(identity), map.bytes()};
}

} // namespace oasis::thor::runtime_map
