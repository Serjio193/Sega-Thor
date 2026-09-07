#include "tools/gpgx_json.hpp"

#include <cctype>
#include <stdexcept>

namespace oasis::tools {
namespace {
class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    JsonValue parse() {
        skip_space();
        auto result = value();
        skip_space();
        if (position_ != text_.size()) fail("trailing data");
        return result;
    }

private:
    [[noreturn]] void fail(std::string_view message) const {
        throw std::runtime_error("invalid JSON at " + std::to_string(position_) + ": " +
                                 std::string(message));
    }

    void skip_space() {
        while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_])))
            ++position_;
    }

    bool consume(char expected) {
        skip_space();
        if (position_ >= text_.size() || text_[position_] != expected) return false;
        ++position_;
        return true;
    }

    JsonValue value() {
        skip_space();
        if (position_ >= text_.size()) fail("missing value");
        switch (text_[position_]) {
        case '{': return object();
        case '[': return array();
        case '"': return {JsonValue::Kind::string, string(), {}, {}};
        case 't': return literal("true", JsonValue::Kind::boolean);
        case 'f': return literal("false", JsonValue::Kind::boolean);
        case 'n': return literal("null", JsonValue::Kind::null_value);
        default: return number();
        }
    }

    JsonValue literal(std::string_view expected, JsonValue::Kind kind) {
        if (text_.substr(position_, expected.size()) != expected) fail("bad literal");
        position_ += expected.size();
        return {kind, std::string(expected), {}, {}};
    }

    JsonValue number() {
        const auto start = position_;
        if (position_ < text_.size() && text_[position_] == '-') ++position_;
        if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_])))
            fail("bad number");
        if (text_[position_] == '0') ++position_;
        else while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        if (position_ < text_.size() && text_[position_] == '.') {
            ++position_;
            while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        }
        return {JsonValue::Kind::number, std::string(text_.substr(start, position_ - start)), {}, {}};
    }

    std::string string() {
        if (!consume('"')) fail("missing string");
        std::string result;
        while (position_ < text_.size()) {
            const char character = text_[position_++];
            if (character == '"') return result;
            if (static_cast<unsigned char>(character) < 0x20U) fail("control character in string");
            if (character != '\\') { result += character; continue; }
            if (position_ >= text_.size()) fail("unfinished escape");
            const char escaped = text_[position_++];
            if (escaped == '"' || escaped == '\\' || escaped == '/') result += escaped;
            else if (escaped == 'b') result += '\b';
            else if (escaped == 'f') result += '\f';
            else if (escaped == 'n') result += '\n';
            else if (escaped == 'r') result += '\r';
            else if (escaped == 't') result += '\t';
            else fail("unsupported string escape");
        }
        fail("unterminated string");
    }

    JsonValue array() {
        if (!consume('[')) fail("missing array");
        JsonValue result{JsonValue::Kind::array};
        skip_space();
        if (consume(']')) return result;
        while (true) {
            result.array.push_back(value());
            if (consume(']')) return result;
            if (!consume(',')) fail("missing array separator");
        }
    }

    JsonValue object() {
        if (!consume('{')) fail("missing object");
        JsonValue result{JsonValue::Kind::object};
        skip_space();
        if (consume('}')) return result;
        while (true) {
            skip_space();
            if (position_ >= text_.size() || text_[position_] != '"') fail("object key expected");
            const auto key = string();
            if (!consume(':')) fail("missing object colon");
            if (!result.object.emplace(key, value()).second) fail("duplicate object key");
            if (consume('}')) return result;
            if (!consume(',')) fail("missing object separator");
        }
    }

    std::string_view text_;
    std::size_t position_ = 0;
};
} // namespace

const JsonValue* JsonValue::find(std::string_view key) const {
    if (kind != Kind::object) return nullptr;
    const auto item = object.find(std::string(key));
    return item == object.end() ? nullptr : &item->second;
}

std::string JsonValue::string_value(std::string_view context) const {
    if (kind != Kind::string) throw std::runtime_error(std::string(context) + " must be a string");
    return scalar;
}

std::uint64_t JsonValue::integer_value(std::string_view context) const {
    if (kind != Kind::number || scalar.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error(std::string(context) + " must be a non-negative integer");
    try { return std::stoull(scalar); }
    catch (...) { throw std::runtime_error(std::string(context) + " is out of range"); }
}

JsonValue parse_json(std::string_view text) { return Parser(text).parse(); }

} // namespace oasis::tools
