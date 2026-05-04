# Divergences From Upstream

## Deferred Features

- None identified for v0. Upstream's full public surface (`parse`, `decode`,
  `stringify`, `encode`, `safe`, `unsafe`, plus all encode/decode options) is
  implemented.

## Deliberate Behavior Changes

- `EncodeOptions::platform` default. Upstream `encode()` reads
  `process.platform` (when defined) to decide between `\r\n` and `\n`. The
  C++ port leaves `platform` empty by default, which means LF on every host.
  Callers who want CRLF must set `EncodeOptions{ .platform = "win32" }`
  explicitly. Rationale: a parser library should not introspect the host OS,
  and there is no equivalent of `process.platform` that does not pull in a
  larger ambient surface. Tested by `IniTest.EncodeWithPlatformWin32` and
  the upstream `win32.js` round-trip.
- `encode(obj, "section")` two-argument JS shorthand. Upstream accepts a
  string second argument as `{ section: <string> }`. The C++ port requires
  the typed `EncodeOptions` form. Rationale: C++ does not have JS-style
  dynamic overloads; the typed struct is unambiguous and idiomatic.
- Result document type. Upstream returns a JS plain object built from
  `Object.create(null)`. The C++ port returns `IniDocument`
  (`std::vector<std::pair<std::string, IniValue>>`) for guaranteed insertion
  order (which `std::unordered_map` would lose) and avoids alphabetical
  ordering (which `std::map` would impose). Order preservation matters for
  encode round-trip fidelity. Tested by `IniTest.DocumentHelperKeys` and
  every snapshot test that asserts byte-exact output.
- Numeric values stay as strings. JS `JSON.parse('"123"')` returns the
  string `"123"`, and upstream stores it as a string; the C++ port matches
  this. INI has no native number type, so callers do their own parsing.
- IniValue accessors throw on type mismatch. `IniValue::asString()` /
  `asBool()` / `asArray()` / `asDocument()` are typed accessors backed by
  `std::variant`; calling the wrong one throws `std::bad_variant_access`.
  Upstream JS has no compile-time or runtime type guard.
- Section header always produces a section. When a section header `[foo]`
  is encountered and `out["foo"]` already holds a non-document value
  (string, bool, array), the C++ port replaces it with a fresh
  `IniDocument`. Upstream JS uses `out[section] = out[section] ||
  Object.create(null)`, which keeps the truthy primitive and silently
  no-ops every subsequent `key=val` write under the section header.
  Rationale: a section header is an explicit author intent to introduce a
  section; silently dropping the rows that follow it is a footgun.
  Pinning tests: `IniTest.SectionHeaderOverwritesPreExistingScalar` and
  `IniTest.SectionHeaderOverwritesPreExistingArray`.
- Dotted-section merge always produces nested documents. When the
  post-pass merge that turns `{a:{}, "a.b":{...}} → {a:{b:{...}}}`
  encounters an intermediate path part that already holds an array, the
  C++ port replaces the array with a fresh `IniDocument`. Upstream JS
  treats arrays as `typeof === 'object'` and reuses them, then sets a
  hidden named property on the array that is invisible to subsequent
  JSON serialization. Rationale: same as above — dotted section headers
  are explicit author intent and should produce navigable nested
  sections, not arrays with invisible named properties. Pinning test:
  `IniTest.DottedSectionMergeOverwritesIntermediateArray`.

## Unsupported Runtime-Specific Features

- `Object.create(null)` prototype safety as an observable invariant. The C++
  container has no prototype chain to begin with; `Object.prototype.foo ===
  undefined` is not a meaningful assertion against `std::vector<pair>`. The
  C++ port still filters `__proto__` keys/sections so consumer expectations
  carry over from JS, but the JS-specific invariants from `test/proto.js`
  (such as `Object.prototype[0] === undefined`) are recorded as
  not-applicable in `docs/test-plan.md`.
- `process.platform` ambient access. See "Deliberate Behavior Changes"
  above.
- ES module `default`/named export distinction. C++ has no module loader;
  consumers `#include <polycpp/ini.hpp>` and call free functions in the
  `polycpp::ini` namespace.
- `JSON.parse(valueRaw)` for the `true`/`false`/`null` tokens. The C++ port
  branches explicitly on the literal strings instead of calling
  `polycpp::JSON::parse`. Behavior is identical for the three documented
  literals; the change avoids constructing a `JsonValue` for what is a
  three-string compare.

## Audit findings (libgen catch-up)

| ID | Severity | Location | Description | Recommended classification |
|---|---|---|---|---|
| AF-2026-05-04-A | low | `examples/{load_config,reformat,set_value}.cpp` and `CMakeLists.txt` | Example sources exist on disk but are not registered as CMake executables. Sibling companions (`cookie`, `qs`, `dotenv`) gate their examples behind `POLYCPP_<NAME>_BUILD_EXAMPLES`. | resolved 2026-05-04 — wired all three examples behind `POLYCPP_INI_BUILD_EXAMPLES` (default `OFF`) using a `foreach` loop, matching the cookie/qs/dotenv pattern. Pinning test: each binary was smoke-tested with representative INI input (load_config / set_value / reformat all produce the expected output). |
| AF-2026-05-04-B | low | `docs/Doxyfile:44-45` | `WARN_IF_UNDOCUMENTED = NO` and `WARN_AS_ERROR = NO` (libgen template ships both as `YES`). Public-release gate requires both `YES`; tightening these will surface any undocumented public symbol before the 1.0.0 promotion. | resolved 2026-05-04 — flipped both flags to `YES`. The only new warning was an undocumented `IniValue::ArrayType` typedef; added a `@brief` line. `python3 docs/build.py` builds clean. Pinning test: `python3 docs/build.py` is now strict and would re-fail on regression. |
| AF-2026-05-04-C | low | `docs/sphinx/api/placeholder.rst` and `docs/sphinx/{tutorials,guides,examples}/planned.rst` | Generated Sphinx placeholder pages remain. `scripts/check-port-validation.py` treats placeholder pages as a blocker for the validation gate. | resolved 2026-05-04 — verified during catch-up that the existing Sphinx tree already has real `api/{document,parse-stringify,value}.rst`, `tutorials/{config-file,nested-sections,round-trip}.rst`, `guides/{align-and-sort,array-values,crlf-platform,quoting-special-chars,remove-key}.rst`, and `examples/{load-config,reformat,set-value}.rst` pages with no `placeholder.rst`/`planned.rst` files. Pinning test: `python3 scripts/check-port-validation.py --run-docs-build` passes. |
| AF-2026-05-04-D | low | `include/polycpp/ini/detail/ini.hpp:391` | `protoSink` document is built locally for `[__proto__]` sections but is never re-cleared if a second `[__proto__]` section appears. Behavior is still correct (the sink is only ever read into the throwaway pointer), but the `protoSink.clear()` call shadows what is functionally a per-section reset of `p`. | resolved 2026-05-04 — dropped `protoSink.clear()` and refreshed the surrounding comment. Pinning test: `IniTest.ParseProtoProtection`. |
| AF-2026-05-04-E | low | `include/polycpp/ini/detail/ini.hpp:469-470` | Inline comment misstates JS behavior ("`duplicates` is only declared once and used for the entire parse"). Upstream actually re-uses the duplicates map across sections, so the C++ port is correct, but the comment leaves the reader confused. | resolved 2026-05-04 — removed the meandering comment; the global-`duplicates` choice was already documented as `## Implementation risks` in `docs/research.md`. Pinning test: `IniTest.DuplicatePropertiesBracketedArrayFalse`. |
| AF-2026-05-04-F | low | `include/polycpp/ini/detail/ini.hpp:127-132` | `valueToString(IniValue)` returns empty string for arrays/documents, but those branches are unreachable in `stringify()` because arrays are flattened and documents are recursed into. Dead-code-style accessor that is harmless but invites future misuse. | resolved 2026-05-04 — added `assert(false && "valueToString called on non-scalar IniValue")` so a future caller cannot accidentally call this on an array/document and silently get `""`. Pinning tests: existing `IniTest.Encode*` coverage continues to pass; the assert fires only on mis-use. |
| AF-2026-05-04-G | low | `tests/test_ini.cpp:13-108` | `FOO_INI` raw-string literal includes the entire upstream fixture but with leading/trailing whitespace tweaked (notably `s4 = ` no longer carries the upstream trailing spaces, and `[b] ` lost its trailing space). Behavior assertions still pass because `unsafe()` trims, but the embedded fixture is no longer byte-identical to the upstream `test/fixtures/foo.ini`. | resolved 2026-05-04 — copied upstream `test/fixtures/foo.ini` and `duplicate.ini` byte-identically into `tests/fixtures/`, replaced the embedded raw-string literals with a `slurpFixture()` loader driven by the CMake `INI_TESTS_FIXTURES_DIR` define, and added a fixture-provenance note to `THIRD_PARTY_LICENSES.md`. Pinning tests: `IniTest.FooIni*`, `IniTest.DuplicateProperties*`, `IniTest.EncodeWithAlign*`, `IniTest.EncodeWithSort*` all pass against the byte-exact fixtures. |
| AF-2026-05-04-H | medium | `include/polycpp/ini/detail/ini.hpp:458-466` | When a section header `[foo]` is encountered and `out["foo"]` already holds a non-document value (string, bool, array), the C++ port replaces it with a fresh `IniDocument`, losing the original value. Upstream JS uses `out[section] = out[section] || Object.create(null)`, which keeps the truthy primitive and silently no-ops the subsequent `p[key] = value` writes onto a string/bool. Neither behavior is asserted by upstream tests; the C++ behavior is arguably more useful, but it is a quiet divergence. | resolved 2026-05-04 — kept the C++ behavior (a section header should always produce a section). Reclassified as a Deliberate Behavior Change above. Pinning tests: `IniTest.SectionHeaderOverwritesPreExistingScalar`, `IniTest.SectionHeaderOverwritesPreExistingArray`. |
| AF-2026-05-04-I | low | `include/polycpp/ini/detail/ini.hpp:577-585` | Inside the dotted-section merge post-pass, when an intermediate part name collides with an existing array, the C++ port replaces the array with a fresh `IniDocument`. Upstream JS treats arrays as `typeof === 'object'` and reuses them, then sets named properties that are invisible to subsequent JSON serialization. The C++ outcome is a clean nested document; the JS outcome is the original array preserved with a hidden named property. Like AF-H, neither behavior is asserted by upstream tests. | resolved 2026-05-04 — kept the C++ behavior (dotted section headers should always produce navigable nested sections). Reclassified as a Deliberate Behavior Change above. Pinning test: `IniTest.DottedSectionMergeOverwritesIntermediateArray`. |
