#include "tools/gpgx_import_io.hpp"

#include "tools/gpgx_json.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace oasis::tools::gpgx {
namespace {
using JsonValue = oasis::tools::JsonValue;

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to read " + path.string());
    return {std::istreambuf_iterator<char>(input), {}};
}

const JsonValue& object(const JsonValue& value, std::string_view context) {
    if (value.kind != JsonValue::Kind::object)
        throw std::runtime_error(std::string(context) + " must be an object");
    return value;
}

std::uint32_t number(const JsonValue& value, std::string_view context) {
    const auto text = value.kind == JsonValue::Kind::string ||
                              value.kind == JsonValue::Kind::number ? value.scalar : "";
    if (text.empty()) throw std::runtime_error(std::string(context) + " must be numeric");
    std::size_t used = 0;
    const auto parsed = std::stoull(text, &used, 0);
    if (used != text.size() || parsed > 0xFFFFFFFFULL)
        throw std::runtime_error("invalid " + std::string(context));
    return static_cast<std::uint32_t>(parsed);
}

const JsonValue& required(const JsonValue& value, std::string_view key) {
    const auto* result = value.find(key);
    if (result == nullptr) throw std::runtime_error("missing JSON field " + std::string(key));
    return *result;
}

std::string text(const JsonValue& value, std::string_view key) {
    return required(value, key).string_value(key);
}

std::uint64_t integer(const JsonValue& value, std::string_view key) {
    return required(value, key).integer_value(key);
}
} // namespace

std::string field_string(std::string_view text, std::string_view key) {
    const auto document = parse_json(text);
    const auto& root = object(document, "JSON document");
    const auto* value = root.find(key);
    return value == nullptr ? std::string{} : value->string_value(key);
}

std::uint64_t field_number(std::string_view text, std::string_view key) {
    const auto document = parse_json(text);
    const auto& root = object(document, "JSON document");
    const auto* value = root.find(key);
    return value == nullptr ? 0U : value->integer_value(key);
}

bool allowed_gpgx_build(std::string_view build_id) {
    return build_id == "7e2fe295e905e6046b043155b249c7fd289701ab" ||
           build_id == "d60d079934977aa6973e220d123533387159f66e";
}

std::vector<Range> load_ranges(const std::filesystem::path& path) {
    const auto document = parse_json(read_file(path));
    const JsonValue* sequence = document.kind == JsonValue::Kind::array ? &document : nullptr;
    if (sequence == nullptr) {
        const auto& root = object(document, "range artifact");
        sequence = root.find("ranges");
        if (sequence == nullptr) sequence = root.find("entries");
        if (sequence == nullptr) sequence = root.find("instructions");
    }
    if (sequence == nullptr || sequence->kind != JsonValue::Kind::array)
        throw std::runtime_error("range artifact must contain an array of entries");
    std::vector<Range> ranges;
    for (const auto& item : sequence->array) {
        const auto& entry = object(item, "range entry");
        const auto* label = entry.find("classification");
        if (label == nullptr) label = entry.find("trust_level");
        if (label == nullptr) label = entry.find("kind");
        if (label == nullptr) throw std::runtime_error("range entry missing classification");
        const auto classification = label->string_value("classification");
        ranges.push_back({number(required(entry, "start"), "range start"),
                          number(required(entry, "end"), "range end"), classification,
                          classification.rfind("DATA_", 0) == 0});
    }
    return ranges;
}

Store load_store(const std::filesystem::path& path, std::string_view canonical_sha) {
    Store store;
    if (!std::filesystem::exists(path)) return store;
    const auto document = parse_json(read_file(path));
    const auto& root = object(document, "runtime evidence");
    if (text(root, "canonical_rom_sha256") != canonical_sha)
        throw std::runtime_error("existing evidence has a different canonical ROM");
    if (const auto* addresses = root.find("executed_addresses")) {
        if (addresses->kind != JsonValue::Kind::array)
            throw std::runtime_error("executed_addresses must be an array");
        for (const auto& value : addresses->array)
            store.addresses.insert(number(value, "executed address"));
    }
    if (const auto* facts = root.find("execution_facts")) {
        if (facts->kind != JsonValue::Kind::array)
            throw std::runtime_error("execution_facts must be an array");
        for (const auto& fact : facts->array) {
            const auto& entry = object(fact, "execution fact");
            store.facts[number(required(entry, "address"), "execution address")].insert(
                text(entry, "capture_id"));
        }
    }
    if (const auto* captures = root.find("imported_captures")) {
        if (captures->kind != JsonValue::Kind::array)
            throw std::runtime_error("imported_captures must be an array");
        for (const auto& capture : captures->array) {
            const auto& entry = object(capture, "imported capture");
            Capture value{text(entry, "capture_id"), text(entry, "bitmap_kind"),
                          text(entry, "bitmap_sha256"),
                          static_cast<std::size_t>(integer(entry, "address_count")),
                          integer(entry, "frames"), integer(entry, "new_pcs"), {}};
            if (const auto* build = entry.find("gpgx_build_id"))
                value.build_id = build->string_value("gpgx_build_id");
            else store.legacy_provenance = true;
            if (!value.build_id.empty()) store.build_ids.insert(value.build_id);
            if (std::none_of(store.captures.begin(), store.captures.end(),
                             [&](const auto& item) { return item.id == value.id; }))
                store.captures.push_back(std::move(value));
        }
    }
    if (store.build_ids.size() > 1)
        throw std::runtime_error("existing evidence combines incompatible GPGX builds");
    return store;
}
} // namespace oasis::tools::gpgx
