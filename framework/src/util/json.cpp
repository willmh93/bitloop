#include <bitloop/util/json.h>

//#include "nlohmann/json.hpp"

// JSON support must go *after* class definition
inline void to_json(nlohmann::json& j, const FiniteDouble& fd) {
    j = static_cast<double>(fd);  // or fd.get();
}

inline void from_json(const nlohmann::json& j, FiniteDouble& fd) {
    fd = j.get<double>();
}

BL_BEGIN_NS;

enum class Ctx { Obj, Arr };

static inline bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}
static inline bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}
static inline bool is_key_start(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return (std::isalpha(uc) || c == '_') != 0;
}
static inline bool is_key_char(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return (std::isalnum(uc) || c == '_') != 0;
}

namespace JSON {

std::string json_add_leading_zeros(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);

    bool in_string = false;
    bool escape = false;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];

        if (in_string) {
            out.push_back(c);
            if (escape) { escape = false; }
            else if (c == '\\') { escape = true; }
            else if (c == '"') { in_string = false; }
            continue;
        }

        if (c == '"') {
            in_string = true;
            out.push_back(c);
            continue;
        }

        // Handle "-.123" or "+.123"
        if ((c == '-' || c == '+') && i + 2 < s.size() && s[i + 1] == '.' && is_digit(s[i + 2])) {
            out.push_back(c);
            out.push_back('0');
            out.push_back('.');
            ++i; // skip the '.'
            continue;
        }

        // Handle ".123" when not part of an existing number
        if (c == '.' && i + 1 < s.size() && is_digit(s[i + 1])) {
            // Look back for the previous non-space character
            char prev = '\0';
            for (size_t j = i; j > 0; ) {
                char p = s[--j];
                if (!is_space(p)) { prev = p; break; }
            }
            // Insert zero if previous char is not a digit or exponent marker
            if (!(is_digit(prev) || prev == 'e' || prev == 'E')) {
                out.push_back('0');
                out.push_back('.');
                continue; // skip writing the original '.'
            }
        }

        out.push_back(c);
    }

    return out;
}

// Return index of closing quote for a JSON string starting at s[i] (i at '"').
// If unterminated, returns s.size()-1.
size_t skip_string(const std::string& s, size_t i) {
    size_t j = i + 1;
    bool esc = false;
    for (; j < s.size(); ++j) {
        char ch = s[j];
        if (esc) { esc = false; continue; }
        if (ch == '\\') { esc = true; continue; }
        if (ch == '"') break;
    }
    return j < s.size() ? j : s.size() - 1;
}

size_t next_nonspace_idx(const std::string& s, size_t j) {
    while (j < s.size() && is_space(s[j])) ++j;
    return j;
}

// Only remove quotes if the key contains no escapes and matches identifier rules.
bool is_safe_quoted_key(const std::string& s, size_t qbeg, size_t qend) {
    // qbeg at opening quote, qend at closing quote
    if (qend <= qbeg + 1) return false; // empty key not allowed
    for (size_t k = qbeg + 1; k < qend; ++k) {
        if (s[k] == '\\') return false; // be conservative
    }
    char first = s[qbeg + 1];
    if (!is_key_start(first)) return false;
    for (size_t k = qbeg + 2; k < qend; ++k) {
        if (!is_key_char(s[k])) return false;
    }
    return true;
}

std::string json_remove_key_quotes(const std::string& s) {
    std::string out;
    out.reserve(s.size());

    std::vector<Ctx> ctx;
    std::vector<bool> obj_expect_key; // parallel to ctx for Obj
    auto in_obj_and_expect_key = [&]() -> bool {
        return !ctx.empty() && ctx.back() == Ctx::Obj && obj_expect_key.back();
    };

    bool in_string = false;
    bool esc = false;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];

        if (in_string) {
            out.push_back(c);
            if (esc) { esc = false; }
            else if (c == '\\') { esc = true; }
            else if (c == '"') { in_string = false; }
            continue;
        }

        if (c == '"') {
            if (in_obj_and_expect_key()) {
                // Potential key string
                size_t qend = skip_string(s, i);
                size_t after = next_nonspace_idx(s, qend + 1);
                if (after < s.size() && s[after] == ':' && is_safe_quoted_key(s, i, qend)) {
                    // Emit unquoted key
                    out.append(s, i + 1, qend - (i + 1));
                    // Advance i to end quote; next loop will handle the ':' normally
                    i = qend;
                    continue;
                }
                else {
                    // Not a removable key; just copy the whole string
                    out.append(s, i, qend - i + 1);
                    i = qend;
                    continue;
                }
            }
            else {
                // Value string
                in_string = true;
                out.push_back(c);
                continue;
            }
        }

        // Structural handling
        if (c == '{') { ctx.push_back(Ctx::Obj); obj_expect_key.push_back(true); }
        else if (c == '}') { if (!ctx.empty() && ctx.back() == Ctx::Obj) { ctx.pop_back(); obj_expect_key.pop_back(); } }
        else if (c == '[') { ctx.push_back(Ctx::Arr); }
        else if (c == ']') { if (!ctx.empty() && ctx.back() == Ctx::Arr) ctx.pop_back(); }
        else if (c == ':') { if (!ctx.empty() && ctx.back() == Ctx::Obj) obj_expect_key.back() = false; }
        else if (c == ',') { if (!ctx.empty() && ctx.back() == Ctx::Obj) obj_expect_key.back() = true; }

        out.push_back(c);
    }

    return out;
}

std::string json_add_key_quotes(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);

    std::vector<Ctx> ctx;
    std::vector<bool> obj_expect_key;
    auto in_obj_and_expect_key = [&]() -> bool {
        return !ctx.empty() && ctx.back() == Ctx::Obj && obj_expect_key.back();
    };

    bool in_string = false;
    bool esc = false;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];

        if (in_string) {
            out.push_back(c);
            if (esc) { esc = false; }
            else if (c == '\\') { esc = true; }
            else if (c == '"') { in_string = false; }
            continue;
        }

        if (c == '"') {
            // Already a string (could be key or value). Copy through.
            size_t qend = skip_string(s, i);
            out.append(s, i, qend - i + 1);
            i = qend;
            continue;
        }

        if (in_obj_and_expect_key()) {
            // Skip spaces before key
            if (is_space(c)) { out.push_back(c); continue; }

            // If next token looks like an unquoted identifier and is followed by ':', quote it.
            if (is_key_start(c)) {
                size_t j = i + 1;
                while (j < s.size() && is_key_char(s[j])) ++j;

                // Peek next non-space to confirm it's a colon (so this is indeed a key)
                size_t after = next_nonspace_idx(s, j);
                if (after < s.size() && s[after] == ':') {
                    out.push_back('"');
                    out.append(s, i, j - i);
                    out.push_back('"');
                    i = j - 1; // continue; ':' and spaces will be handled next
                    continue;
                }
                // Not followed by ':', fall through: just copy as-is
            }
            // If it's something else (like '{' from malformed input), just fall through.
        }

        // Structural handling
        if (c == '{') { ctx.push_back(Ctx::Obj); obj_expect_key.push_back(true); }
        else if (c == '}') { if (!ctx.empty() && ctx.back() == Ctx::Obj) { ctx.pop_back(); obj_expect_key.pop_back(); } }
        else if (c == '[') { ctx.push_back(Ctx::Arr); }
        else if (c == ']') { if (!ctx.empty() && ctx.back() == Ctx::Arr) ctx.pop_back(); }
        else if (c == ':') { if (!ctx.empty() && ctx.back() == Ctx::Obj) obj_expect_key.back() = false; }
        else if (c == ',') { if (!ctx.empty() && ctx.back() == Ctx::Obj) obj_expect_key.back() = true; }

        out.push_back(c);
    }

    return out;
}

} // End namespace JSON

BL_END_NS;
