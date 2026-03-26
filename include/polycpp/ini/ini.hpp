#pragma once

/**
 * @file ini.hpp
 * @brief C++ port of npm ini v6.0.0 — INI format parser and serializer.
 * @see https://www.npmjs.com/package/ini
 * @since 0.1.0
 */

#include <string>
#include <vector>
#include <variant>
#include <utility>
#include <cstddef>
#include <optional>
#include <algorithm>

namespace polycpp {
namespace ini {

// Forward declarations
class IniValue;

/**
 * @brief Ordered key-value container for INI documents.
 *
 * Uses vector<pair> instead of map to preserve insertion order,
 * which is critical for round-trip fidelity of INI files.
 */
using IniDocument = std::vector<std::pair<std::string, IniValue>>;

/**
 * @brief Dynamic value type for INI entries.
 *
 * An IniValue can hold:
 * - nullptr_t (null)
 * - bool (true/false)
 * - std::string (plain string values)
 * - std::vector<IniValue> (arrays, e.g. key[] syntax)
 * - IniDocument (nested sections)
 *
 * @see https://www.npmjs.com/package/ini
 * @since 0.1.0
 */
class IniValue {
public:
    using ArrayType = std::vector<IniValue>;

    /** @brief Construct a null value. */
    IniValue();

    /** @brief Construct a null value from nullptr. */
    IniValue(std::nullptr_t);

    /** @brief Construct a boolean value. */
    IniValue(bool b);

    /** @brief Construct a string value. */
    IniValue(const std::string& s);

    /** @brief Construct a string value from C string literal. */
    IniValue(const char* s);

    /** @brief Construct a string value (move). */
    IniValue(std::string&& s);

    /** @brief Construct an array value. */
    IniValue(const ArrayType& arr);

    /** @brief Construct an array value (move). */
    IniValue(ArrayType&& arr);

    /** @brief Construct a document (nested section) value. */
    IniValue(const IniDocument& doc);

    /** @brief Construct a document (nested section) value (move). */
    IniValue(IniDocument&& doc);

    /** @brief Check if this value is null. */
    bool isNull() const;

    /** @brief Check if this value is a boolean. */
    bool isBool() const;

    /** @brief Check if this value is a string. */
    bool isString() const;

    /** @brief Check if this value is an array. */
    bool isArray() const;

    /** @brief Check if this value is a document (nested section). */
    bool isDocument() const;

    /**
     * @brief Get the boolean value.
     * @return The bool value.
     * @throws std::bad_variant_access if not a bool.
     */
    bool asBool() const;

    /**
     * @brief Get the string value.
     * @return Reference to the string.
     * @throws std::bad_variant_access if not a string.
     */
    const std::string& asString() const;

    /**
     * @brief Get the string value (mutable).
     * @return Mutable reference to the string.
     * @throws std::bad_variant_access if not a string.
     */
    std::string& asString();

    /**
     * @brief Get the array value.
     * @return Reference to the array.
     * @throws std::bad_variant_access if not an array.
     */
    const ArrayType& asArray() const;

    /**
     * @brief Get the array value (mutable).
     * @return Mutable reference to the array.
     * @throws std::bad_variant_access if not an array.
     */
    ArrayType& asArray();

    /**
     * @brief Get the document value.
     * @return Reference to the document.
     * @throws std::bad_variant_access if not a document.
     */
    const IniDocument& asDocument() const;

    /**
     * @brief Get the document value (mutable).
     * @return Mutable reference to the document.
     * @throws std::bad_variant_access if not a document.
     */
    IniDocument& asDocument();

    /** @brief Equality comparison. */
    bool operator==(const IniValue& other) const;

    /** @brief Inequality comparison. */
    bool operator!=(const IniValue& other) const;

private:
    std::variant<std::nullptr_t, bool, std::string, ArrayType, IniDocument> value_;
};

/**
 * @brief Options for encoding (stringify) an IniDocument.
 * @see https://github.com/npm/ini#encodeobject-options
 * @since 0.1.0
 */
struct EncodeOptions {
    /** @brief Section prefix for the encoded output. */
    std::string section;

    /** @brief Align keys on the separator. */
    bool align = false;

    /** @brief Add a blank line after each section header. */
    bool newline = false;

    /** @brief Sort keys alphabetically. */
    bool sort = false;

    /** @brief Use ` = ` separator (with spaces) instead of `=`. */
    bool whitespace = false;

    /** @brief Platform for line endings: "win32" for \\r\\n, empty for \\n. */
    std::string platform;

    /** @brief Use `key[]` syntax for arrays (true) or plain `key` (false). */
    bool bracketedArray = true;
};

/**
 * @brief Options for decoding (parsing) an INI string.
 * @see https://github.com/npm/ini#decodeinistring-options
 * @since 0.1.0
 */
struct DecodeOptions {
    /** @brief Interpret `key[]` as array syntax (true) or literal key (false). */
    bool bracketedArray = true;
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @brief Parse an INI-format string into an IniDocument.
 *
 * @param str The INI-format string to parse.
 * @param opt Decode options (bracketedArray).
 * @return Parsed IniDocument.
 *
 * @par Example
 * @code
 * auto doc = polycpp::ini::parse("[section]\nkey=value\n");
 * @endcode
 *
 * @see https://github.com/npm/ini#parseinistring-options
 * @since 0.1.0
 */
IniDocument parse(const std::string& str, const DecodeOptions& opt = {});

/**
 * @brief Alias for parse().
 * @see parse()
 * @since 0.1.0
 */
IniDocument decode(const std::string& str, const DecodeOptions& opt = {});

/**
 * @brief Encode an IniDocument into an INI-format string.
 *
 * @param obj The IniDocument to encode.
 * @param opt Encode options (section, align, whitespace, etc.).
 * @return INI-format string.
 *
 * @par Example
 * @code
 * IniDocument doc;
 * polycpp::ini::set(doc, "key", "value");
 * std::string result = polycpp::ini::stringify(doc);
 * @endcode
 *
 * @see https://github.com/npm/ini#encodeobject-options
 * @since 0.1.0
 */
std::string stringify(const IniDocument& obj, const EncodeOptions& opt = {});

/**
 * @brief Alias for stringify().
 * @see stringify()
 * @since 0.1.0
 */
std::string encode(const IniDocument& obj, const EncodeOptions& opt = {});

/**
 * @brief Escape a string value for safe inclusion in INI output.
 *
 * Values containing `=`, `\\r`, `\\n`, starting with `[`, quoted strings,
 * or strings with leading/trailing whitespace are JSON-stringified.
 * Otherwise `;` and `#` are escaped with backslash.
 *
 * @param val The string to make safe.
 * @return Escaped string.
 *
 * @see https://github.com/npm/ini#safevalue
 * @since 0.1.0
 */
std::string safe(const std::string& val);

/**
 * @brief Unescape an INI value string.
 *
 * Handles JSON-quoted strings, single-quoted strings, backslash escapes
 * for `;`, `#`, and `\\`, and strips inline comments.
 *
 * @param val The raw INI value string.
 * @return Unescaped string.
 *
 * @see https://github.com/npm/ini#unsafevalue
 * @since 0.1.0
 */
std::string unsafe(const std::string& val);

// ---------------------------------------------------------------------------
// IniDocument helper functions
// ---------------------------------------------------------------------------

/**
 * @brief Find a value in an IniDocument by key.
 * @param doc The document to search.
 * @param key The key to find.
 * @return Pointer to the value, or nullptr if not found.
 * @since 0.1.0
 */
IniValue* find(IniDocument& doc, const std::string& key);

/**
 * @brief Find a value in an IniDocument by key (const).
 * @param doc The document to search.
 * @param key The key to find.
 * @return Const pointer to the value, or nullptr if not found.
 * @since 0.1.0
 */
const IniValue* find(const IniDocument& doc, const std::string& key);

/**
 * @brief Check if an IniDocument contains a key.
 * @param doc The document to search.
 * @param key The key to check.
 * @return true if the key exists.
 * @since 0.1.0
 */
bool hasKey(const IniDocument& doc, const std::string& key);

/**
 * @brief Set a key-value pair in an IniDocument.
 *
 * If the key already exists, its value is replaced. Otherwise a new
 * pair is appended at the end.
 *
 * @param doc The document to modify.
 * @param key The key to set.
 * @param value The value to assign.
 * @since 0.1.0
 */
void set(IniDocument& doc, const std::string& key, IniValue value);

/**
 * @brief Get all keys in an IniDocument in insertion order.
 * @param doc The document.
 * @return Vector of key strings.
 * @since 0.1.0
 */
std::vector<std::string> keys(const IniDocument& doc);

/**
 * @brief Remove a key from an IniDocument.
 * @param doc The document to modify.
 * @param key The key to remove.
 * @return true if the key was found and removed.
 * @since 0.1.0
 */
bool remove(IniDocument& doc, const std::string& key);

} // namespace ini
} // namespace polycpp
