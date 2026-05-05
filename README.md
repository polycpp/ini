# polycpp/ini

C++ port of the npm [ini](https://www.npmjs.com/package/ini) package for [polycpp](https://github.com/enricohuang/polycpp).

An INI format parser and serializer for C++20, using polycpp's Node.js-like API conventions.

## Status

Port version: `1.0.0`

Initial port based on upstream version: `6.0.0`

The C++ port owns its versioning independently from the upstream npm package. See `docs/research.md` and `docs/divergences.md` for the implemented surface and the deliberate behavior changes from upstream.

## Features

- Parse INI text into structured C++ objects
- Serialize C++ objects to INI format
- Section support with dot-notation nesting
- Array support via `key[]` bracket syntax or duplicate keys
- Comment handling (`;` and `#`)
- Configurable output: alignment, sorting, whitespace, platform-specific line endings
- Prototype pollution protection (`__proto__` keys are ignored)

## Prerequisites

- C++20 compiler (GCC 13+ or Clang 16+)
- CMake 3.20+
- Ninja (recommended)

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## Usage

```cpp
#include <polycpp/ini.hpp>
#include <iostream>

int main() {
    // Parse INI text
    std::string ini_text = R"(
[database]
host=localhost
port=5432
name=mydb
)";
    auto config = polycpp::ini::parse(ini_text);

    // Access values
    auto* db = polycpp::ini::find(config, "database");
    if (db && db->isDocument()) {
        auto* host = polycpp::ini::find(db->asDocument(), "host");
        if (host) std::cout << "Host: " << host->asString() << "\n";
    }

    // Serialize back to INI
    std::string output = polycpp::ini::stringify(config);
    std::cout << output;

    return 0;
}
```

## API

### `polycpp::ini::parse(str, opt)` / `polycpp::ini::decode(str, opt)`
Parse an INI string into an `IniDocument`.

### `polycpp::ini::stringify(obj, opt)` / `polycpp::ini::encode(obj, opt)`
Serialize an `IniDocument` to an INI string.

### `polycpp::ini::safe(val)`
Escape a string value for safe INI output.

### `polycpp::ini::unsafe(val)`
Unescape an INI value string.

### `polycpp::ini::find/hasKey/set/keys/remove(doc, key)`
Order-preserving helpers over `IniDocument`. `find` returns a pointer (or `nullptr`), `set` overwrites in place or appends at the end, `keys` returns insertion order.

### `IniValue::toJSON()` / `polycpp::JSON::stringify(IniValue)`
Recursively converts an `IniValue` into a `polycpp::JsonValue`. Combined with polycpp's `HasToJson` concept, calling `polycpp::JSON::stringify(v)` on any `IniValue` works directly. Numeric-looking INI values stay as JSON strings, matching upstream `ini` semantics.

## License

MIT License. See [LICENSE](LICENSE) for details.

See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for attribution to the original npm ini package.
