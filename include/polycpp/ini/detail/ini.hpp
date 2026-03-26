#pragma once

/**
 * @file detail/ini.hpp
 * @brief Inline implementations for polycpp::ini.
 * @since 0.1.0
 */

#include <polycpp/ini/ini.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace polycpp {
namespace ini {

// ===========================================================================
// Internal helpers (anonymous-namespace-like, but inline for header-only)
// ===========================================================================
namespace detail {

/**
 * @brief Minimal JSON string escaper (wraps string in double quotes with escapes).
 */
inline std::string jsonStringify(const std::string& s) {
    std::string result = "\"";
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    result += "\"";
    return result;
}

/**
 * @brief Convert a single hex digit character to its integer value.
 * @return The value 0-15, or -1 on invalid input.
 */
inline int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

/**
 * @brief Parse a JSON-escaped string (input must start and end with `"`).
 *
 * Handles: \\", \\\\, \\/, \\n, \\r, \\t, \\b, \\f, \\uXXXX.
 * On parse failure (e.g., unescaped `"` inside), returns the input unchanged
 * (matching JS try { JSON.parse(val) } catch { } behavior).
 */
inline std::string jsonParse(const std::string& s) {
    if (s.size() < 2 || s.front() != '"' || s.back() != '"') {
        return s;
    }
    std::string result;
    result.reserve(s.size());
    bool valid = true;
    for (size_t i = 1; i < s.size() - 1; ++i) {
        if (s[i] == '\\' && i + 1 < s.size() - 1) {
            char next = s[i + 1];
            switch (next) {
                case '"':  result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '/':  result += '/'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                case 'b':  result += '\b'; ++i; break;
                case 'f':  result += '\f'; ++i; break;
                case 'u': {
                    if (i + 5 < s.size()) {
                        int d0 = hexDigit(s[i + 2]);
                        int d1 = hexDigit(s[i + 3]);
                        int d2 = hexDigit(s[i + 4]);
                        int d3 = hexDigit(s[i + 5]);
                        if (d0 >= 0 && d1 >= 0 && d2 >= 0 && d3 >= 0) {
                            int cp = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
                            // Encode codepoint as UTF-8
                            if (cp < 0x80) {
                                result += static_cast<char>(cp);
                            } else if (cp < 0x800) {
                                result += static_cast<char>(0xC0 | (cp >> 6));
                                result += static_cast<char>(0x80 | (cp & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (cp >> 12));
                                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (cp & 0x3F));
                            }
                            i += 5;
                        } else {
                            valid = false;
                            break;
                        }
                    } else {
                        valid = false;
                    }
                    break;
                }
                default:
                    // Invalid escape sequence in JSON
                    valid = false;
                    break;
            }
            if (!valid) break;
        } else if (s[i] == '"') {
            // Unescaped quote inside the string — invalid JSON
            valid = false;
            break;
        } else {
            result += s[i];
        }
    }
    if (!valid) {
        return s; // Return original on parse failure
    }
    return result;
}

/**
 * @brief Check if a string is quoted (starts and ends with matching `"` or `'`).
 */
inline bool isQuoted(const std::string& val) {
    if (val.size() < 2) return false;
    return (val.front() == '"' && val.back() == '"') ||
           (val.front() == '\'' && val.back() == '\'');
}

/**
 * @brief Trim whitespace from both ends of a string.
 */
inline std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' ||
                                 s[start] == '\r' || s[start] == '\n')) {
        ++start;
    }
    if (start == s.size()) return "";
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' ||
                            s[end - 1] == '\r' || s[end - 1] == '\n')) {
        --end;
    }
    return s.substr(start, end - start);
}

/**
 * @brief Split a string on occurrences of separator that are NOT preceded by backslash.
 *
 * Faithful port of JS splitSections(str, separator).
 */
inline std::vector<std::string> splitSections(const std::string& str,
                                               char separator) {
    std::vector<std::string> sections;
    int lastMatchIndex = 0;
    int lastSeparatorIndex = 0;
    int nextIndex = 0;

    do {
        // Find next occurrence of separator starting at lastMatchIndex
        auto pos = str.find(separator, static_cast<size_t>(lastMatchIndex));
        if (pos == std::string::npos) {
            nextIndex = -1;
        } else {
            nextIndex = static_cast<int>(pos);
        }

        if (nextIndex != -1) {
            lastMatchIndex = nextIndex + 1;

            if (nextIndex > 0 && str[static_cast<size_t>(nextIndex) - 1] == '\\') {
                continue;
            }

            sections.push_back(
                str.substr(static_cast<size_t>(lastSeparatorIndex),
                           static_cast<size_t>(nextIndex) - static_cast<size_t>(lastSeparatorIndex)));
            lastSeparatorIndex = nextIndex + 1;
        }
    } while (nextIndex != -1);

    sections.push_back(str.substr(static_cast<size_t>(lastSeparatorIndex)));
    return sections;
}

/**
 * @brief Convert an IniValue to its string representation for encoding.
 */
inline std::string valueToString(const IniValue& val) {
    if (val.isNull()) return "null";
    if (val.isBool()) return val.asBool() ? "true" : "false";
    if (val.isString()) return val.asString();
    return "";
}

/**
 * @brief Check if a string ends with a given suffix.
 */
inline bool endsWith(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Check if a string starts with a given prefix.
 */
inline bool startsWith(const std::string& s, const std::string& prefix) {
    if (prefix.size() > s.size()) return false;
    return s.compare(0, prefix.size(), prefix) == 0;
}

} // namespace detail

// ===========================================================================
// IniValue implementation
// ===========================================================================

inline IniValue::IniValue() : value_(nullptr) {}

inline IniValue::IniValue(std::nullptr_t) : value_(nullptr) {}

inline IniValue::IniValue(bool b) : value_(b) {}

inline IniValue::IniValue(const std::string& s) : value_(s) {}

inline IniValue::IniValue(const char* s) : value_(std::string(s)) {}

inline IniValue::IniValue(std::string&& s) : value_(std::move(s)) {}

inline IniValue::IniValue(const ArrayType& arr) : value_(arr) {}

inline IniValue::IniValue(ArrayType&& arr) : value_(std::move(arr)) {}

inline IniValue::IniValue(const IniDocument& doc) : value_(doc) {}

inline IniValue::IniValue(IniDocument&& doc) : value_(std::move(doc)) {}

inline bool IniValue::isNull() const {
    return std::holds_alternative<std::nullptr_t>(value_);
}

inline bool IniValue::isBool() const {
    return std::holds_alternative<bool>(value_);
}

inline bool IniValue::isString() const {
    return std::holds_alternative<std::string>(value_);
}

inline bool IniValue::isArray() const {
    return std::holds_alternative<ArrayType>(value_);
}

inline bool IniValue::isDocument() const {
    return std::holds_alternative<IniDocument>(value_);
}

inline bool IniValue::asBool() const {
    return std::get<bool>(value_);
}

inline const std::string& IniValue::asString() const {
    return std::get<std::string>(value_);
}

inline std::string& IniValue::asString() {
    return std::get<std::string>(value_);
}

inline const IniValue::ArrayType& IniValue::asArray() const {
    return std::get<ArrayType>(value_);
}

inline IniValue::ArrayType& IniValue::asArray() {
    return std::get<ArrayType>(value_);
}

inline const IniDocument& IniValue::asDocument() const {
    return std::get<IniDocument>(value_);
}

inline IniDocument& IniValue::asDocument() {
    return std::get<IniDocument>(value_);
}

inline bool IniValue::operator==(const IniValue& other) const {
    return value_ == other.value_;
}

inline bool IniValue::operator!=(const IniValue& other) const {
    return value_ != other.value_;
}

// ===========================================================================
// IniDocument helper functions
// ===========================================================================

inline IniValue* find(IniDocument& doc, const std::string& key) {
    for (auto& [k, v] : doc) {
        if (k == key) return &v;
    }
    return nullptr;
}

inline const IniValue* find(const IniDocument& doc, const std::string& key) {
    for (const auto& [k, v] : doc) {
        if (k == key) return &v;
    }
    return nullptr;
}

inline bool hasKey(const IniDocument& doc, const std::string& key) {
    return find(doc, key) != nullptr;
}

inline void set(IniDocument& doc, const std::string& key, IniValue value) {
    for (auto& [k, v] : doc) {
        if (k == key) {
            v = std::move(value);
            return;
        }
    }
    doc.emplace_back(key, std::move(value));
}

inline std::vector<std::string> keys(const IniDocument& doc) {
    std::vector<std::string> result;
    result.reserve(doc.size());
    for (const auto& [k, v] : doc) {
        result.push_back(k);
    }
    return result;
}

inline bool remove(IniDocument& doc, const std::string& key) {
    for (auto it = doc.begin(); it != doc.end(); ++it) {
        if (it->first == key) {
            doc.erase(it);
            return true;
        }
    }
    return false;
}

// ===========================================================================
// safe / unsafe
// ===========================================================================

inline std::string safe(const std::string& val) {
    // Check if the value needs JSON stringification
    bool needsQuote = false;

    // Contains = or \r or \n
    for (char c : val) {
        if (c == '=' || c == '\r' || c == '\n') {
            needsQuote = true;
            break;
        }
    }

    // Starts with [
    if (!needsQuote && !val.empty() && val[0] == '[') {
        needsQuote = true;
    }

    // Is a quoted string (length > 1 and starts+ends with " or ')
    if (!needsQuote && val.size() > 1 && detail::isQuoted(val)) {
        needsQuote = true;
    }

    // Has leading or trailing whitespace
    if (!needsQuote && !val.empty()) {
        if (val != detail::trim(val)) {
            needsQuote = true;
        }
    }

    if (needsQuote) {
        return detail::jsonStringify(val);
    }

    // Escape ; and #
    std::string result;
    result.reserve(val.size());
    for (char c : val) {
        if (c == ';') {
            result += "\\;";
        } else if (c == '#') {
            result += "\\#";
        } else {
            result += c;
        }
    }
    return result;
}

inline std::string unsafe(const std::string& val) {
    std::string trimmed = detail::trim(val);

    if (trimmed.empty()) {
        return "";
    }

    if (detail::isQuoted(trimmed)) {
        // If single-quoted, strip quotes first
        if (trimmed.front() == '\'') {
            trimmed = trimmed.substr(1, trimmed.size() - 2);
        } else {
            // Double-quoted: try JSON parse
            std::string parsed = detail::jsonParse(trimmed);
            trimmed = parsed;
        }
        return trimmed;
    }

    // Walk the value to find the first not-escaped ; or # character
    bool esc = false;
    std::string unesc;
    for (size_t i = 0; i < trimmed.size(); ++i) {
        char c = trimmed[i];
        if (esc) {
            if (c == '\\' || c == ';' || c == '#') {
                unesc += c;
            } else {
                unesc += '\\';
                unesc += c;
            }
            esc = false;
        } else if (c == ';' || c == '#') {
            break;
        } else if (c == '\\') {
            esc = true;
        } else {
            unesc += c;
        }
    }
    if (esc) {
        unesc += '\\';
    }
    return detail::trim(unesc);
}

// ===========================================================================
// decode / parse
// ===========================================================================

inline IniDocument decode(const std::string& str, const DecodeOptions& opt) {
    IniDocument out;

    // Current target document (either out or a section's document)
    IniDocument* p = &out;
    bool inProtoSection = false;
    IniDocument protoSink; // throwaway for __proto__ sections

    // Regex: section header OR key=value
    static const std::regex re(R"(^\[([^\]]*)\]\s*$|^([^=]+)(=(.*))?$)");

    // Split input on \r\n or \n or \r (split on [\r\n]+)
    std::vector<std::string> lines;
    {
        size_t start = 0;
        size_t len = str.size();
        while (start < len) {
            size_t end = start;
            while (end < len && str[end] != '\r' && str[end] != '\n') {
                ++end;
            }
            if (end > start) {
                lines.push_back(str.substr(start, end - start));
            }
            // Skip consecutive \r and \n
            while (end < len && (str[end] == '\r' || str[end] == '\n')) {
                ++end;
            }
            if (end == start) {
                // Avoid infinite loop on empty string
                break;
            }
            start = end;
        }
    }

    // Duplicate tracking for bracketedArray=false
    std::unordered_map<std::string, int> duplicates;

    for (const auto& line : lines) {
        // Skip blank lines and comment lines
        if (line.empty()) continue;

        // Check if line is a comment (starts with optional whitespace then ; or #)
        {
            size_t idx = 0;
            while (idx < line.size() && (line[idx] == ' ' || line[idx] == '\t')) {
                ++idx;
            }
            if (idx < line.size() && (line[idx] == ';' || line[idx] == '#')) {
                continue;
            }
            // All whitespace line
            if (idx == line.size()) {
                continue;
            }
        }

        std::smatch match;
        if (!std::regex_match(line, match, re)) {
            continue;
        }

        // Section header: match[1]
        if (match[1].matched) {
            std::string section = unsafe(match[1].str());
            if (section == "__proto__") {
                // Parse into throwaway doc
                inProtoSection = true;
                protoSink.clear();
                p = &protoSink;
                continue;
            }
            inProtoSection = false;

            // Find or create the section in out
            IniValue* existing = find(out, section);
            if (existing && existing->isDocument()) {
                p = &existing->asDocument();
            } else {
                IniDocument newDoc;
                set(out, section, IniValue(std::move(newDoc)));
                p = &find(out, section)->asDocument();
            }
            // Reset duplicates for each section? No — JS code uses a single
            // duplicates object across all sections. But actually looking at
            // the JS code, duplicates is only declared once and used for the
            // entire parse. Let's keep it global.
            continue;
        }

        // Key-value pair: match[2] is key, match[3] is =value or absent
        std::string keyRaw = unsafe(match[2].str());
        bool isArray = false;

        if (opt.bracketedArray) {
            isArray = keyRaw.size() > 2 && detail::endsWith(keyRaw, "[]");
        } else {
            duplicates[keyRaw] = (duplicates.count(keyRaw) ? duplicates[keyRaw] : 0) + 1;
            isArray = duplicates[keyRaw] > 1;
        }

        std::string key = (isArray && detail::endsWith(keyRaw, "[]"))
            ? keyRaw.substr(0, keyRaw.size() - 2)
            : keyRaw;

        if (key == "__proto__") {
            continue;
        }

        // Determine the value
        IniValue value;
        if (match[3].matched) {
            // Has = sign: use match[4] (may be empty)
            std::string valueRaw = unsafe(match[4].str());
            if (valueRaw == "true") {
                value = IniValue(true);
            } else if (valueRaw == "false") {
                value = IniValue(false);
            } else if (valueRaw == "null") {
                value = IniValue(nullptr);
            } else {
                value = IniValue(valueRaw);
            }
        } else {
            // No = sign: bare key → bool true
            value = IniValue(true);
        }

        // Convert keys with '[]' suffix to an array
        if (isArray) {
            IniValue* existing = find(*p, key);
            if (!existing) {
                // Create new array
                IniValue::ArrayType arr;
                set(*p, key, IniValue(std::move(arr)));
            } else if (!existing->isArray()) {
                // Promote existing value to array
                IniValue::ArrayType arr;
                arr.push_back(std::move(*existing));
                *existing = IniValue(std::move(arr));
            }
        }

        // Set or append the value
        IniValue* slot = find(*p, key);
        if (slot && slot->isArray()) {
            slot->asArray().push_back(std::move(value));
        } else {
            set(*p, key, std::move(value));
        }
    }

    // Post-processing: merge dotted section names
    // {a:{y:1},"a.b":{x:2}} --> {a:{y:1,b:{x:2}}}
    // Take a snapshot of keys to avoid issues with modifying during iteration
    std::vector<std::string> outKeys = keys(out);
    std::vector<std::string> toRemove;

    for (const auto& k : outKeys) {
        IniValue* vPtr = find(out, k);
        if (!vPtr || !vPtr->isDocument()) continue;

        // see if the parent section is also an object.
        // if so, add it to that, and mark this one for deletion
        auto parts = detail::splitSections(k, '.');

        IniDocument* target = &out;
        std::string lastPart = parts.back();
        parts.pop_back();

        // Unescape dots in the last part: replace \. with .
        std::string nl;
        for (size_t i = 0; i < lastPart.size(); ++i) {
            if (lastPart[i] == '\\' && i + 1 < lastPart.size() && lastPart[i + 1] == '.') {
                nl += '.';
                ++i;
            } else {
                nl += lastPart[i];
            }
        }

        for (const auto& part : parts) {
            if (part == "__proto__") continue;

            // Unescape dots in intermediate parts too
            std::string unescPart;
            for (size_t i = 0; i < part.size(); ++i) {
                if (part[i] == '\\' && i + 1 < part.size() && part[i + 1] == '.') {
                    unescPart += '.';
                    ++i;
                } else {
                    unescPart += part[i];
                }
            }

            IniValue* existing = find(*target, unescPart);
            if (!existing || !existing->isDocument()) {
                IniDocument newDoc;
                set(*target, unescPart, IniValue(std::move(newDoc)));
                existing = find(*target, unescPart);
            }
            target = &existing->asDocument();
        }

        // Only do the merge if we actually navigated somewhere or the name changed
        if (target == &out && nl == lastPart) {
            continue;
        }

        // Re-fetch vPtr since out may have been modified
        vPtr = find(out, k);
        if (vPtr) {
            set(*target, nl, std::move(*vPtr));
            toRemove.push_back(k);
        }
    }

    // Remove the merged keys
    for (const auto& del : toRemove) {
        remove(out, del);
    }

    return out;
}

inline IniDocument parse(const std::string& str, const DecodeOptions& opt) {
    return decode(str, opt);
}

// ===========================================================================
// encode / stringify
// ===========================================================================

inline std::string stringify(const IniDocument& obj, const EncodeOptions& opt) {
    // Effective options — align implies whitespace
    bool useWhitespace = opt.whitespace || opt.align;
    std::string eol = (opt.platform == "win32") ? "\r\n" : "\n";
    std::string separator = useWhitespace ? " = " : "=";
    std::string arraySuffix = opt.bracketedArray ? "[]" : "";

    // Get keys
    std::vector<std::string> keyList = keys(obj);
    if (opt.sort) {
        std::sort(keyList.begin(), keyList.end());
    }

    // Calculate padding for alignment
    size_t padToChars = 0;
    if (opt.align) {
        // Filter: non-object keys (null, arrays, and non-object values)
        // Then map: arrays get [] suffix
        // Find the longest safe key
        std::string longestKey;
        for (const auto& k : keyList) {
            const IniValue* val = find(obj, k);
            if (!val) continue;
            // Include: null, arrays, and non-object types
            // Exclude: documents (unless null, which we handle)
            if (val->isDocument()) continue;

            std::string displayKey = k;
            if (val->isArray()) {
                displayKey = k + "[]";
            }
            if (safe(displayKey).size() > safe(longestKey).size()) {
                longestKey = displayKey;
            }
        }
        padToChars = safe(longestKey).size();
    }

    std::string out;
    std::vector<std::string> children;

    for (const auto& k : keyList) {
        const IniValue* val = find(obj, k);
        if (!val) continue;

        if (val->isArray()) {
            for (const auto& item : val->asArray()) {
                std::string safeKey = safe(k + arraySuffix);
                while (safeKey.size() < padToChars) safeKey += ' ';
                out += safeKey + separator + safe(detail::valueToString(item)) + eol;
            }
        } else if (val->isDocument()) {
            children.push_back(k);
        } else {
            std::string safeKey = safe(k);
            while (safeKey.size() < padToChars) safeKey += ' ';
            out += safeKey + separator + safe(detail::valueToString(*val)) + eol;
        }
    }

    if (!opt.section.empty() && !out.empty()) {
        out = "[" + safe(opt.section) + "]" + (opt.newline ? eol + eol : eol) + out;
    }

    for (const auto& k : children) {
        // Escape dots in child key
        auto parts = detail::splitSections(k, '.');
        std::string nk;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) nk += "\\.";
            nk += parts[i];
        }
        std::string section = opt.section.empty() ? nk : (opt.section + "." + nk);

        EncodeOptions childOpt = opt;
        childOpt.section = section;

        const IniValue* val = find(obj, k);
        std::string child = stringify(val->asDocument(), childOpt);

        if (!out.empty() && !child.empty()) {
            out += eol;
        }
        out += child;
    }

    return out;
}

inline std::string encode(const IniDocument& obj, const EncodeOptions& opt) {
    return stringify(obj, opt);
}

} // namespace ini
} // namespace polycpp
