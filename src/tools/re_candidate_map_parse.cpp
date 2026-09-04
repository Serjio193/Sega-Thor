#include "tools/re_candidate_map.hpp"

#include <cctype>
#include <map>
#include <stdexcept>

namespace oasis::tools {
namespace {

struct JsonValue {
    enum class Kind { null_value, boolean, number, string, array, object };
    Kind kind{Kind::null_value};
    bool boolean{};
    std::uint64_t number{};
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view input) : input_(input) {}

    JsonValue parse() {
        skip_space();
        auto value = parse_value();
        skip_space();
        if (position_ != input_.size()) fail("trailing JSON data");
        return value;
    }

private:
    std::string_view input_;
    std::size_t position_{};

    [[noreturn]] void fail(const char* message) const {
        throw std::runtime_error(std::string("invalid Ghidra JSON: ") + message);
    }

    void skip_space() {
        while (position_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[position_]))) ++position_;
    }

    bool take(char expected) {
        skip_space();
        if (position_ >= input_.size() || input_[position_] != expected) return false;
        ++position_;
        return true;
    }

    JsonValue parse_value() {
        skip_space();
        if (position_ >= input_.size()) fail("missing value");
        const char current = input_[position_];
        if (current == '{') return parse_object();
        if (current == '[') return parse_array();
        if (current == '"') return JsonValue{JsonValue::Kind::string, false, 0, parse_string()};
        if (current == 't') return parse_literal("true", JsonValue::Kind::boolean, true);
        if (current == 'f') return parse_literal("false", JsonValue::Kind::boolean, false);
        if (current == 'n') return parse_literal("null", JsonValue::Kind::null_value, false);
        if (std::isdigit(static_cast<unsigned char>(current))) return parse_number();
        fail("unexpected value");
    }

    JsonValue parse_literal(std::string_view literal, JsonValue::Kind kind, bool boolean) {
        if (input_.substr(position_, literal.size()) != literal) fail("bad literal");
        position_ += literal.size();
        return JsonValue{kind, boolean, 0, {}};
    }

    JsonValue parse_number() {
        const auto begin = position_;
        while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
        try {
            const auto value = std::stoull(std::string(input_.substr(begin, position_ - begin)));
            return JsonValue{JsonValue::Kind::number, false, value, {}};
        } catch (...) {
            fail("bad number");
        }
    }

    std::string parse_string() {
        if (!take('"')) fail("missing string quote");
        std::string result;
        while (position_ < input_.size()) {
            const char current = input_[position_++];
            if (current == '"') return result;
            if (current == '\\') {
                if (position_ >= input_.size()) fail("truncated escape");
                const char escaped = input_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/') result += escaped;
                else if (escaped == 'n') result += '\n';
                else if (escaped == 'r') result += '\r';
                else if (escaped == 't') result += '\t';
                else fail("unsupported escape");
            } else {
                if (static_cast<unsigned char>(current) < 0x20U) fail("control character in string");
                result += current;
            }
        }
        fail("unterminated string");
    }

    JsonValue parse_array() {
        if (!take('[')) fail("missing array start");
        JsonValue result;
        result.kind = JsonValue::Kind::array;
        skip_space();
        if (take(']')) return result;
        while (true) {
            result.array.push_back(parse_value());
            if (take(']')) return result;
            if (!take(',')) fail("missing array separator");
        }
    }

    JsonValue parse_object() {
        if (!take('{')) fail("missing object start");
        JsonValue result;
        result.kind = JsonValue::Kind::object;
        skip_space();
        if (take('}')) return result;
        while (true) {
            skip_space();
            if (position_ >= input_.size() || input_[position_] != '"') fail("object key is not a string");
            const auto key = parse_string();
            if (!take(':')) fail("missing object colon");
            auto value = parse_value();
            if (!result.object.emplace(key, std::move(value)).second) fail("duplicate object key");
            if (take('}')) return result;
            if (!take(',')) fail("missing object separator");
        }
    }
};

const JsonValue& required(const JsonValue& object, const char* key, JsonValue::Kind kind) {
    if (object.kind != JsonValue::Kind::object) throw std::runtime_error("invalid Ghidra JSON: expected object");
    const auto found = object.object.find(key);
    if (found == object.object.end() || found->second.kind != kind)
        throw std::runtime_error(std::string("invalid Ghidra JSON field: ") + key);
    return found->second;
}

std::string string_field(const JsonValue& object, const char* key) {
    return required(object, key, JsonValue::Kind::string).string;
}

bool bool_field(const JsonValue& object, const char* key) {
    return required(object, key, JsonValue::Kind::boolean).boolean;
}

std::size_t number_field(const JsonValue& object, const char* key) {
    return static_cast<std::size_t>(required(object, key, JsonValue::Kind::number).number);
}

std::uint32_t address(std::string_view value) {
    if (value.size() < 3 || value[0] != '0' || (value[1] != 'x' && value[1] != 'X'))
        throw std::runtime_error("invalid Ghidra address: expected hexadecimal string");
    std::uint64_t parsed{};
    for (const char digit : value.substr(2)) {
        parsed <<= 4U;
        if (digit >= '0' && digit <= '9') parsed += static_cast<unsigned>(digit - '0');
        else if (digit >= 'a' && digit <= 'f') parsed += static_cast<unsigned>(digit - 'a' + 10);
        else if (digit >= 'A' && digit <= 'F') parsed += static_cast<unsigned>(digit - 'A' + 10);
        else throw std::runtime_error("invalid Ghidra address digit");
        if (parsed > 0xFFFFFFFFULL) throw std::runtime_error("Ghidra address out of range");
    }
    return static_cast<std::uint32_t>(parsed);
}

std::pair<std::optional<std::uint32_t>, std::optional<std::uint32_t>> range_field(const JsonValue& object) {
    const auto value = string_field(object, "range");
    if (value == "UNKNOWN") return {};
    const auto separator = value.find("..");
    if (separator == std::string::npos) throw std::runtime_error("invalid Ghidra range");
    return {address(value.substr(0, separator)), address(value.substr(separator + 2))};
}

std::vector<std::uint32_t> address_array(const JsonValue& object, const char* key) {
    const auto& values = required(object, key, JsonValue::Kind::array).array;
    std::vector<std::uint32_t> result;
    for (const auto& value : values) {
        if (value.kind != JsonValue::Kind::string) throw std::runtime_error("invalid Ghidra address array");
        result.push_back(address(value.string));
    }
    return result;
}

GhidraFunction parse_function(const JsonValue& value) {
    const auto range = range_field(value);
    return {address(string_field(value, "entry")), range.first, range.second,
            address_array(value, "calls"), address_array(value, "called_by"),
            bool_field(value, "has_return"), number_field(value, "instruction_count"),
            number_field(value, "basic_block_count")};
}

GhidraCandidate parse_candidate(const JsonValue& value) {
    const auto range = range_field(value);
    return {address(string_field(value, "entry")), range.first, range.second,
            string_field(value, "source"), bool_field(value, "decoded_as_code")};
}

} // namespace

GhidraMap parse_ghidra_map(std::string_view text) {
    const auto root = JsonParser(text).parse();
    GhidraMap result;
    result.schema = string_field(root, "schema");
    if (result.schema != "oasis.m68k.ghidra-map.v1") throw std::runtime_error("unsupported Ghidra map schema");
    const auto& metadata = required(root, "metadata", JsonValue::Kind::object);
    result.program_name = string_field(metadata, "program_name");
    result.language_id = string_field(metadata, "language_id");
    result.compiler_spec = string_field(metadata, "compiler_spec");
    result.analysis_mode = string_field(metadata, "analysis_mode");
    result.semantic_status = string_field(metadata, "semantic_status");
    for (const auto& value : required(root, "functions", JsonValue::Kind::array).array)
        result.functions.push_back(parse_function(value));
    for (const auto& value : required(root, "candidates", JsonValue::Kind::array).array)
        result.candidates.push_back(parse_candidate(value));
    return result;
}

} // namespace oasis::tools
