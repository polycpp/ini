# Test Plan

## Unit tests

- `safe()` JSON-quoting trigger conditions: contains `=`, `\r`, `\n`,
  starts with `[`, is fully quoted (`"..."` or `'...'`), or differs from
  its trimmed form.
- `safe()` backslash-escape of `;` and `#` for plain values.
- `unsafe()` trim, single-quote strip, JSON-parse fallback, walk-and-stop
  on unescaped `;`/`#`, backslash escapes for `;`/`#`/`\`, trailing
  backslash preservation.
- `parse()` regex split: section header vs key=value vs bare key vs
  malformed line drop.
- `parse()` nested section merge: `[a.b.c]` produces `a → b → c`.
- `parse()` literal dotted section: `[x\.y\.z]` produces a single
  literal `x.y.z` key.
- `parse()` array via `key[] =` syntax (`bracketedArray = true`).
- `parse()` array via duplicate keys (`bracketedArray = false`).
- `parse()` array promotion: scalar value already present is wrapped
  into a single-element array on the next `[]` write.
- `parse()` `__proto__` filter at top-level keys, top-level sections,
  and intermediate section parts.
- `parse()` special string values `"true"`/`"false"`/`"null"` →
  `IniValue(true)` / `IniValue(false)` / `IniValue(nullptr)`.
- `parse()` bare key with no `=` → `IniValue(true)`.
- `parse()` empty value (`s3 =`) → `IniValue("")`.
- `parse()` quoted-whitespace preservation (`s5 = '   '`).
- `encode()` separator: `=` vs ` = ` based on `whitespace`/`align`.
- `encode()` line endings: LF vs CRLF based on `platform`.
- `encode()` `bracketedArray` toggle for arrays.
- `encode()` `align` width and padding (longest filtered key).
- `encode()` `sort` lexicographic key order.
- `encode()` `newline` blank-line-after-header insertion.
- `encode()` `section` prefix prepend, with nested children recursing.
- `encode()` never emits a blank first or trailing-blank-second line.
- `IniValue` constructors and accessors for `null`/`bool`/`string`/
  `array`/`document` variants, plus equality.
- `IniDocument` helpers: `find`, `hasKey`, `set` (overwrite vs append),
  `keys` (insertion order), `remove`.

## Integration tests

- `parse(stringify(x)) == x` round-trip for typed values
  (`number-as-string`, `string`, `bool`, nested `IniDocument`). Mirrors
  upstream `test/bar.js`.
- Snapshot-equivalent encode of the foo fixture with default options,
  with `align`, with `sort`, with `align + sort`. Mirrors upstream
  `test/foo.js` `tap-snapshots`.
- Snapshot-equivalent decode of the foo fixture: every key, value,
  array, and nested section asserted by exact string match. Mirrors
  upstream `test/foo.js` decode snapshot.
- Encode of the prefixed `[prefix.log]` / `[prefix.log.level]` example
  from upstream's encode-with-option test.

## Compatibility tests adapted from upstream

- upstream compatibility layout: a single `tests/test_ini.cpp` consolidates
  every upstream test cluster. Embedded `FOO_INI` and `DUPLICATE_INI` raw
  string literals carry the upstream fixture content (see
  `docs/divergences.md` audit finding AF-2026-05-04-G for the byte-exact
  fidelity follow-up).
- upstream-to-local coverage map:
  - `test/foo.js` → `IniTest.FooIniDecodeFromFile`,
    `IniTest.FooIniEncodeFromData`, `IniTest.EncodeWithSection`,
    `IniTest.EncodeWithWhitespace`, `IniTest.EncodeWithNewline`,
    `IniTest.EncodeWithPlatformWin32`, `IniTest.EncodeNeverBlankFirstOrLastLine`,
    `IniTest.EncodeWithAlign`, `IniTest.EncodeWithSort`,
    `IniTest.EncodeWithAlignAndSort`
  - `test/duplicate-properties.js` →
    `IniTest.DuplicatePropertiesBracketedArrayTrue`,
    `IniTest.DuplicatePropertiesBracketedArrayFalse`,
    `IniTest.DuplicatePropertiesEncodeBracketed`,
    `IniTest.DuplicatePropertiesEncodeUnbracketed`
  - `test/proto.js` → `IniTest.ParseProtoProtection` (data + nested-section
    coverage)
  - `test/win32.js` → `IniTest.Win32Encode`,
    `IniTest.Win32EncodeWithSectionString`, `IniTest.Win32DecodeWithCRLF`,
    `IniTest.Win32DecodeArrayMerge`, plus the `unsafe('')`,
    `unsafe('x;y')`, `unsafe('x  # y')`, `unsafe('x "\\')` cases under
    `IniTest.UnsafeHandlesEmptyString`,
    `IniTest.UnsafeStripsInlineComments`, `IniTest.UnsafeTrailingBackslash`
  - `test/bar.js` → `IniTest.RoundTripNumber`, `IniTest.RoundTripString`,
    `IniTest.RoundTripBoolean`, `IniTest.RoundTripNested`
- omitted upstream cases:
  - `Object.defineProperty(process, 'platform', { value: undefined })` /
    `value: 'win32'` test scaffolding from `test/win32.js` and the
    "encode within browser context" sub-test of `test/foo.js` —
    not applicable because the C++ port does not consult `process.platform`
    at all (see `docs/divergences.md`). Equivalent behavior is exercised
    by passing `EncodeOptions::platform` explicitly.
  - `Object.prototype.foo === undefined` and `Array.prototype[0] ===
    undefined` invariants from `test/proto.js` — not applicable because
    `std::vector<std::pair<...>>` has no prototype chain. The
    `__proto__`-skipping behavior they pin is still covered by
    `IniTest.ParseProtoProtection`.

## Security and fail-closed tests

- `__proto__` filter at every documented position
  (`IniTest.ParseProtoProtection`).
- Malformed input lines are dropped, not crashed on
  (`IniTest.ParseJunkLine`, `IniTest.Win32DecodeWithCRLF`).
- WTF-8 lone-surrogate input must not crash and must round-trip through
  `polycpp::JSON::parse` (`IniTest.UnsafeLoneSurrogatePreservesAsWtf8`,
  `IniTest.UnsafeLoneLowSurrogatePreservesAsWtf8`).
- Surrogate-pair emoji must round-trip from `😀` JSON-quoted
  input to the canonical UTF-8 byte sequence
  (`IniTest.UnsafeSurrogatePairEmoji`,
  `IniTest.UnsafeSurrogatePairInValue`).
- Single-quoted values whose body parses as JSON must JSON-parse
  (`IniTest.UnsafeSingleQuotedWithJsonContent`); plain single-quoted
  values fall through to the strip-and-return branch.

## Protocol/client tests

- Not applicable because `polycpp::ini` is a pure string format
  parser/serializer with no service backend, network transport, database
  protocol, or session state.

## Release-blocking behaviors

- Snapshot-equivalent foo and duplicate decode/encode tests must pass; a
  snapshot regression is a release blocker.
- `__proto__` filter regressions are a release blocker (security
  contract carried from upstream).
- WTF-8 lone-surrogate handling is a release blocker; regression would
  diverge from polycpp's JSON parser contract.
- `encode()` must never emit a blank first line or a trailing blank
  second-to-last line; release blocker for tooling that
  diff-compares output.

## Current validation

Record exact commands run, service versions, and notable environment variables.

- toolchain: Clang 14.0.0 (gap-analysis only; libgen recommends Clang 16+ or GCC 13+), CMake 3.22, Ninja, gtest 1.15.2 (via FetchContent), polycpp at HEAD `c0f97702`.
- configure: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_SOURCE_DIR_POLYCPP=/data/repo/polycpp -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<local-googletest-checkout>`
- build: `cmake --build build -j$(nproc)`
- test: `cd build && ctest --output-on-failure`
- last validation date: 2026-05-04 (libgen catch-up audit run).
- last validation result: 66/66 tests passed in 0.21 s.
