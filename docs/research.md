# Research

- package: ini
- npm url: https://www.npmjs.com/package/ini
- source url: https://github.com/npm/ini.git
- upstream version basis: 6.0.0
- upstream revision analyzed: 180a8d5c72d7f13ed70c53619132f3a8ee5ac6ed (tag `v6.0.0`)
- upstream default branch: latest
- license: ISC
- license evidence: `package.json` `license: "ISC"` field plus inspection of upstream `LICENSE` file (ISC text by Isaac Z. Schlueter and Contributors)
- category: text-format parser/serializer (INI configuration files)

## Package purpose

`ini` parses INI-format text into a nested key/value structure and serializes
the same structure back to INI text. The serializer supports section-prefix
emission, alignment padding, sorting, whitespace around `=`, platform line
endings, and a `bracketedArray` toggle controlling how repeated keys are
emitted (`key[]=` rows or duplicate `key=` rows).

It is a pure utility used by Node tooling (npm, npm-config, package
installers) to read and write `.npmrc`-style files. There is no network,
filesystem, or crypto behavior in the package itself.

## Runtime assumptions

- browser: works in browsers as a pure string utility; the only Node-runtime
  reference is `process.platform` for the encode default and is feature-tested
  with `typeof process !== 'undefined'`.
- node.js: ESM/CJS compatible; engine pin `^20.17.0 || >=22.9.0` reflects the
  upstream maintainer support window, not a runtime requirement of the parser.
- filesystem: none.
- network: none.
- crypto: none.
- terminal: none.

## Dependency summary

- package.json present: yes
- hard dependencies: none (no `dependencies` field)
- peer dependencies: none
- optional dependencies: none
- package main: `lib/ini.js`
- package module: not declared
- package exports: not declared
- package types: not declared (no `*.d.ts` ships, no `@types/ini` is required —
  callers commonly use `// @ts-ignore` or local declarations)
- package bin: not declared
- dependency analysis report: `docs/dependency-analysis.md`

## Upstream repo layout summary

- Clone path used for analysis: `/data/work/lib/ini/.tmp/upstream/ini` pinned to tag `v6.0.0`
- `lib/ini.js` — the entire implementation (280 lines, ISC, no imports)
- `test/foo.js`, `test/duplicate-properties.js`, `test/proto.js`, `test/win32.js`, `test/bar.js` — tap-based tests
- `test/fixtures/foo.ini`, `test/fixtures/duplicate.ini` — text fixtures consumed by the foo/duplicate tests
- `tap-snapshots/test/foo.js.test.cjs`, `tap-snapshots/test/duplicate-properties.js.test.cjs` — golden output captured from the parser; behavior contract for snapshot tests
- `package.json`, `README.md`, `LICENSE`, `SECURITY.md`, `CHANGELOG.md` — project metadata
- `.github/`, `release-please-config.json`, `.npmcli/template-oss` config — repo automation; not relevant to the C++ port

The published npm artifact at `.tmp/npm-package/lib/ini.js` is byte-identical to
the Git source at `v6.0.0` (`diff -u` returns empty), so the Git source is the
canonical runtime artifact for analysis.

## Entry points used by consumers

- `require('ini').parse` / `import { parse } from 'ini'` — alias for `decode`
- `require('ini').decode` / `import { decode } from 'ini'`
- `require('ini').stringify` / `import { stringify } from 'ini'` — alias for `encode`
- `require('ini').encode` / `import { encode } from 'ini'`
- `require('ini').safe`
- `require('ini').unsafe`

TypeScript declarations: not shipped. No `@types/ini` package is maintained at
v6 either; the public API is thin enough (six functions) that callers either
infer or hand-write a `.d.ts` shim. The README is the canonical contract.

## Important files and why they matter

- `lib/ini.js` — the only behavior file. Contains `encode`, `decode`, the
  internal `splitSections`, `safe`, `unsafe`, and `isQuoted` helpers, and the
  module exports. Every behavior decision in the port traces back to one of
  these functions.
- `test/foo.js` + `test/fixtures/foo.ini` + `tap-snapshots/test/foo.js.test.cjs` —
  the most exhaustive parse/encode behavior fixture. Contains escaped comments,
  quoted keys/values, single-quoted values, JSON-stringified embedded JSON,
  arrays, dotted-section literal escaping, and section-prefix emission.
- `test/duplicate-properties.js` + `fixtures/duplicate.ini` + snapshot — pins
  `bracketedArray: true` vs `false` semantics, including the array-promotion
  rule when a duplicate appears.
- `test/proto.js` — pins prototype-pollution mitigation for keys/sections named
  `__proto__`, including in nested section paths.
- `test/win32.js` — pins `process.platform = 'win32'` defaulting, the
  `[x]\r\ny=1\r\ny[]=2` array-merge case, the `=just junk!` malformed-line
  drop, the `unsafe('x;y')` inline-comment strip, the `unsafe('x  # y')`
  inline-`#`-comment strip, and the `unsafe('x "\\')` trailing-backslash
  preservation.
- `test/bar.js` — pins `parse(stringify(x)) == x` round-trip for typed values.

## Files likely irrelevant to the C++ port

- `release-please-config.json`, `.release-please-manifest.json`, `.npmrc`,
  `.commitlintrc.js`, `.eslintrc.js`, `.github/workflows/*` — all repo
  automation and JS toolchain config; the C++ port owns its own CMake/CI.
- `tap-snapshots/` is consulted as a behavior oracle but is not ported as-is.
- `SECURITY.md` is GitHub policy boilerplate; the C++ repo has its own.

## Test directories worth mining first

- `test/fixtures/foo.ini` and `tap-snapshots/test/foo.js.test.cjs` together
  define the most-frequent behavior path (encode + decode + align + sort).
- `test/proto.js` is the prototype-pollution contract.
- `test/duplicate-properties.js` is the array-semantics contract.
- `test/win32.js` is the Windows-line-endings + `unsafe` corner-case contract.
- `test/bar.js` is the round-trip identity contract.

## Implementation risks discovered from the source layout

- The decode regex `/^\[([^\]]*)\]\s*$|^([^=]+)(=(.*))?$/i` does not validate
  characters inside the section name beyond rejecting `]`. Keys with
  `bracketedArray: true` and a `[]` suffix overlap with this — a key like
  `"[]"` parses as the literal key `"[]"`, not as an empty section header,
  because the value-form pattern requires `=` to be absent or appear after a
  non-`]` first character. Behavior must be preserved exactly.
- `unsafe` first trims, then if quoted strips one layer of single quotes and
  attempts `JSON.parse`. This means single-quoted values that happen to
  contain valid JSON get JSON-parsed (e.g. `'"hello"'` → `hello`). C++ must
  not "fix" this: it is part of the wire contract.
- `splitSections` ignores `\` only when it appears immediately before the
  separator (back-quote escape), so `\.` inside a section header escapes the
  dot at each position — but other backslash uses pass through.
- The post-processing pass that merges `{a:{},"a.b":{x}}` → `{a:{b:{x}}}`
  iterates over a snapshot of top-level keys and may modify `out` while
  walking, including unescaping `\.` only at the leaf and intermediate parts.
  Order is: snapshot keys → for each k, split on `.`, walk parts, set leaf,
  remember `k` for deletion → bulk delete. Maintaining this order is
  important; the existing C++ port must match.
- `__proto__` filtering is applied at three points: top-level key, top-level
  section, and intermediate section parts. JS-specific concerns about
  `Object.prototype` mutation are not reachable in C++, but the *filter* is
  still meaningful so downstream consumers porting JS configurations behave
  the same way.
- Encoder default `bracketedArray = opt.bracketedArray !== false` means
  `bracketedArray` is `true` unless explicitly `false` — including `undefined`
  results in `true`. C++ must use a default-true bool to preserve behavior.
- `process.platform` is read at encode time only when `opt.platform` is not
  supplied. There is no equivalent ambient API in C++ that should leak into a
  parser library; this is recorded as a deliberate behavior change in
  `docs/divergences.md`.

## Companion repo alignment

- companion repos inspected: `cookie` (small utility, similar text-format,
  shipping at 1.0.0), `qs` (parser + encoder of structured key/value strings),
  `dotenv` (similar config-format parser, shipping at 1.0.0)
- CMake target and alias pattern: `polycpp_ini` + `polycpp::ini` ALIAS, matching
  every inspected companion. Already in place.
- public header layout: top-level `<polycpp/ini.hpp>` umbrella +
  `<polycpp/ini/ini.hpp>` typed declarations + `<polycpp/ini/detail/*.hpp>`
  inline implementation. Matches `cookie`, `qs`, `dotenv`. Already in place.
- detail/private header strategy: implementation lives in
  `include/polycpp/ini/detail/ini.hpp` and is included via the aggregator. The
  `src/ini.cpp` translation unit only includes the aggregator so the library
  has one compiled object for linker compatibility. Matches `qs` and `cookie`.
- aggregator header strategy: `include/polycpp/ini/detail/aggregator.hpp` pulls
  in the public typed header and detail implementations. Matches the companion
  pattern.
- examples strategy: `cookie`, `qs`, and `dotenv` register their `examples/*.cpp`
  as optional CMake executables guarded by `POLYCPP_<NAME>_BUILD_EXAMPLES`. This
  port's `examples/` directory contains three example sources that are NOT yet
  wired into `CMakeLists.txt` — recorded as audit finding in
  `docs/divergences.md`.
- documentation site strategy: Doxygen + Sphinx (Breathe) under `docs/sphinx`
  with a GitHub Pages workflow at `.github/workflows/docs.yml`. Matches
  companion pattern. The Sphinx scaffold already exists; `docs/build.py` (the
  libgen-standard entrypoint) and the workflow file are added during this
  catch-up.
- README structure: aligned with companion siblings (Features → Prerequisites →
  Build → Usage → API → License). Already in place.
- deliberate deviations from existing companions: the legacy `docs/build.sh`
  is kept alongside `docs/build.py` per catch-up playbook guidance ("removing
  it is a separate cleanup, not a catch-up step"). `docs/Doxyfile` was
  hand-customized to set `WARN_IF_UNDOCUMENTED = NO` and `WARN_AS_ERROR = NO`;
  the public-release gate requires both to be `YES`, so this is recorded as an
  audit finding for follow-up before the 1.0.0 promotion (not catch-up scope).

## Polycpp ecosystem reuse analysis

- polycpp core paths inspected: `/data/repo/polycpp/include/polycpp/core/`
  (`json.hpp` for `JsonValue` and `JSON::parse`/`JSON::stringify`),
  `/data/repo/polycpp/include/polycpp/{buffer,fs,events,stream,url,intl,crypto,
  http,timers,event_loop,interfaces}` for primitive surfaces.
- polycpp capability snapshot: HEAD `c0f97702 Migrate stable Error surfaces to
  Error family`, captured 2026-05-04. Modules confirmed present:
  `polycpp::JSON::parse`/`JSON::stringify` and `polycpp::JsonValue` (all used
  by the existing implementation), `polycpp::Buffer`, `polycpp::events`,
  `polycpp::stream`, `polycpp::url`, `polycpp::http`, `polycpp::https`,
  `polycpp::tls`, `polycpp::net`, `polycpp::fs`, `polycpp::crypto`,
  `polycpp::timers`.
- transport/listener capability review: not applicable. `ini` has no socket,
  listener, TLS client, TLS server, TCP, Unix/IPC, or adopted-handle surface.
  The package operates entirely on `std::string` input/output.
- polycpp core types/functions selected: `polycpp::JSON::stringify(polycpp::JsonValue(s))` for JSON-quoting strings (used in `detail::jsonStringify`), and `polycpp::JSON::parse(s)` returning `polycpp::JsonValue` for decoding JSON-quoted INI values (used in `detail::jsonParse`). Both preserve full UTF-8 / UTF-16 surrogate handling and control-character escaping that hand-written code would not match.
  - `polycpp::JSON::stringify(polycpp::JsonValue(s))` — used in
    `detail::jsonStringify` to JSON-quote a string with full UTF-8/UTF-16
    surrogate handling, control-character escaping, and validation. This is
    the direct C++ analogue of upstream `JSON.stringify(val)`.
  - `polycpp::JSON::parse(s)` returning `polycpp::JsonValue` — used in
    `detail::jsonParse` to decode JSON-quoted INI values, including
    `\uXXXX` (single and surrogate-pair) escapes and standard escape
    sequences. This is the direct C++ analogue of upstream `JSON.parse(val)`
    inside `unsafe`.
- polycpp core types/functions rejected: `polycpp::String` (no UTF-16 code-unit semantics needed), `polycpp::JsonValue`/`JsonObject`/`JsonArray` as the public document type (INI value model is a strict subset and JsonValue would mislead), `polycpp::stream` (no upstream chunked surface), `polycpp::events`/`EventEmitter` (no events emitted), `polycpp::fs` (caller owns file I/O), and the unused `polycpp::Date`/`URL`/`http::Headers`/`timers`/`crypto`/`tls`/`net` modules.
  - `polycpp::String` (UTF-16 code-unit string) — rejected. INI is a byte text
    format; `std::string` UTF-8 is the natural representation. Using
    `polycpp::String` would force an unjustified encoding round-trip on every
    parse and serialize call.
  - `polycpp::JsonValue`/`JsonObject`/`JsonArray` as the public document type —
    rejected. INI's value model is `null|bool|string|array<value>|nested`,
    which is a strict subset of JSON minus numbers (INI numerics are stored
    as strings) and minus general object support (INI nested values are only
    sections). Exposing `JsonValue` would falsely suggest INI supports JSON
    types it does not. The port introduces `IniValue` and `IniDocument` for
    this reason.
  - `polycpp::stream` — rejected. Upstream `ini` is a whole-string parser/
    serializer with no chunked-input or chunked-output mode. Adding stream
    adapters here would invent new surface beyond upstream and is recorded
    as a non-parity extension candidate.
  - `polycpp::events`/`polycpp::EventEmitter` — rejected. Upstream emits no
    events.
  - `polycpp::fs` — rejected. Upstream does not read or write files; the
    examples (`load_config.cpp` etc.) use `std::ifstream` directly. File I/O
    belongs to the caller.
  - `polycpp::Date`, `polycpp::URL`, `polycpp::http::Headers`,
    `polycpp::timers`, `polycpp::crypto`, `polycpp::tls`, `polycpp::net` —
    rejected: not used by upstream and not relevant to INI parsing.
- public polycpp interop review:
  - string policy: `std::string`/UTF-8 throughout. The implementation operates
    on bytes and only reinterprets UTF-8 inside JSON-quoted values where
    `polycpp::JSON::parse` handles UTF-8 + WTF-8 (lone surrogate) correctly.
  - JsonValue/Object/Array policy: `polycpp::JsonValue` is used as a private
    transport into `polycpp::JSON::stringify`/`parse`, but is not part of the
    public API. Public callers see only `IniValue`.
  - Date/time interop policy: not applicable.
  - diagnostic/config object policy: `IniValue::isString()`/`asString()`/
    `isBool()`/`isArray()`/`isDocument()` are typed accessors. `IniValue::toJSON()`
    converts to `polycpp::JsonValue` (added in `0.2.0`); a JSON-to-INI
    `fromJSON()` adapter is not provided yet.
  - toJSON/stringify policy: `polycpp::JSON::stringify(IniValue)` is supported
    via the `HasToJson` concept (`IniValue::toJSON()` returns
    `polycpp::JsonValue`); the templated overload in `polycpp::JSON::stringify`
    is auto-enabled. Direct INI stringification remains via
    `polycpp::ini::stringify(IniDocument&)`.
- companion libs inspected for reusable APIs: `cookie`, `qs`, `dotenv`. None
  expose a structure that can be reused directly. `qs` parses a different
  format (URL-encoded query strings); `dotenv` parses a different format
  (`.env`); `cookie` parses HTTP cookie headers. No shared parser/serializer
  to inherit.
- companion libs selected for reuse: none.
- companion libs rejected or deferred: none specifically; this port has no
  companion-shaped overlap to reject.
- new local abstractions introduced: `polycpp::ini::IniValue` (variant), `polycpp::ini::IniDocument` (`std::vector<std::pair<std::string,IniValue>>`), and `find/hasKey/set/keys/remove` helpers over `IniDocument`. All three are justified by the INI value model below; no ecosystem primitive matches.
  - `polycpp::ini::IniValue` — variant of `nullptr_t | bool | std::string |
    std::vector<IniValue> | IniDocument`. Justified above; no ecosystem
    primitive matches the INI value model.
  - `polycpp::ini::IniDocument = std::vector<std::pair<std::string,IniValue>>`
    — order-preserving key/value container. Justified because INI round-trip
    fidelity requires insertion order and `std::map` would alphabetize while
    `std::unordered_map` would lose order entirely.
  - `polycpp::ini::find/hasKey/set/keys/remove` — minimal helpers over
    `IniDocument`. Justified because `vector<pair>` lookup is verbose at the
    call site; helpers keep tests and consumers idiomatic.
- reuse risks or integration gaps: none. The port depends on `polycpp` solely
  for `polycpp::JSON::parse`/`stringify` and `polycpp::JsonValue`, which are
  stable public surfaces.

## Node parity surface audit

- callback APIs: not applicable. Upstream has no callbacks.
- Promise APIs: not applicable. Upstream is fully synchronous.
- EventEmitter APIs: not applicable. Upstream emits nothing.
- server/listener APIs: not applicable. Upstream has no socket/listener
  surface (TCP, Unix/IPC, adopted handles, TLS client/server are all
  irrelevant to a string parser).
- diagnostic/tracing APIs: not applicable. Upstream uses no
  `node:diagnostics_channel` and emits no traces.
- stream APIs: not applicable in v0. Upstream has no stream interface;
  callers buffer the entire INI text. Stream-mode parse/encode is recorded as
  a non-parity extension candidate.
- Buffer and binary APIs: not applicable. Upstream operates on
  `string`-typed input only.
- URL, timer, process, and filesystem APIs: `process.platform` is read at
  encode time as a fallback for the win32 line-ending default. The C++ port
  does not call any equivalent (no implicit OS detection in a parser
  library); callers must pass `EncodeOptions::platform = "win32"` explicitly.
  Recorded as a deliberate behavior change in `docs/divergences.md`.
- crypto, compression, TLS, network, and HTTP APIs: not applicable.
- unsupported Node-specific APIs and audit reason: `__proto__` keys and
  sections are recognized and skipped to preserve behavior, but the JS
  `Object.prototype.foo === undefined` invariants asserted in `test/proto.js`
  are not meaningful in C++ (`std::vector<pair>` cannot be polluted via
  prototype). Recorded as a not-applicable note in `docs/test-plan.md`.

## External SDK and native driver strategy

- upstream external services/protocols: not applicable. `ini` is a pure
  string parser with no service, database, protocol, or external SDK
  dependency.
- native SDKs/client libraries to use: not applicable.
- SDKs/protocols explicitly not reimplemented: not applicable.
- adapter/linking strategy: link to base `polycpp` only.
- test environment needs: none beyond a C++20 compiler, CMake, gtest. No
  service backend, network, or external runtime is needed.

## Compatibility foundation review

- downstream dependency role: foundational. `ini` is consumed transitively by
  npm tooling (`npm config`, `npmrc` parsing). For the polycpp ecosystem, the
  port is most likely to be used by future polycpp companions that need to
  read or write `.npmrc`, `.gitconfig`-shaped, or other INI-format files. It
  is therefore a behavior-compatibility port.
- native substitution risk: high if a third-party C/C++ INI library is
  substituted. Common C INI libraries (`inih`, `libconfini`, `simpleini`)
  have different escape rules, different array semantics, and do not
  implement upstream's section-prefix dotted-merge or `__proto__` filtering.
  The port deliberately reimplements the upstream parser instead of wrapping
  a third-party C library so behavior matches the npm package.
- upstream implementation data to preserve: exact decode regex, `unsafe` quoting/escape rules, `safe` JSON-stringify trigger conditions, dotted-section merge order with `\.` unescape, `__proto__` filter points, bracketed-array vs duplicate-key array-promotion rule, and the `bracketedArray = opt.bracketedArray !== false` default.
  - exact decode regex
  - exact `unsafe` quoting/escape rules
  - exact `safe` JSON-stringify trigger conditions
  - exact dotted-section merge order and `\.` unescape rule
  - exact `__proto__` filter points
  - exact bracketed-array vs duplicate-key array-promotion rule
  - exact `bracketedArray = opt.bracketedArray !== false` default
- generated or vendored data plan: not applicable. There are no generated
  tables, codecs, or fixtures in upstream `ini`.
- compatibility fixture strategy: upstream `test/fixtures/foo.ini` and
  `test/fixtures/duplicate.ini` are embedded as raw string literals in
  `tests/test_ini.cpp` and asserted against the upstream tap snapshot
  outputs. This pins exact wire-level parse and serialize compatibility.

## Security and fail-closed review

- security-sensitive behavior: low. `ini` is a string-format parser with no
  network, crypto, or filesystem access. The only security-adjacent concern
  is JavaScript prototype pollution via attacker-controlled INI files —
  upstream mitigates by skipping `__proto__` keys and intermediate path
  parts.
- trust boundary: caller-provided INI text is treated as untrusted. The
  parser must not crash, must not loop forever, and must skip prototype
  pollution attempts.
- supported protocol or algorithm matrix: not applicable.
- unsupported behavior and fail-closed policy: invalid lines (lines that
  fail the regex) are silently dropped, matching upstream. Lines that begin
  with `;` or `#` are treated as comments. Inside `unsafe`, malformed JSON
  inside double-quoted values is caught and the original text is returned
  (matches `try { JSON.parse(val) } catch { }`).
- result-set/framing drain policy, if protocol client: not applicable.
- binary payload type-mapping policy, if protocol client: not applicable.
- stateful parser/session-state policy, if protocol client/server: not
  applicable.
- server/listener response writer matrix, if protocol server surface exists: not applicable.
- key, secret, credential, or user-controlled input handling: INI files
  commonly hold credentials (`.npmrc` auth tokens, `.gitconfig` user data).
  The parser must round-trip token-bearing values byte-exactly, must not
  log values, and must not transform whitespace inside quoted values.
  Verified by the `s5`/`s6` quoted-whitespace cases in `test_ini.cpp`.
- misuse cases that must be tested: prototype-pollution attempts (top-level key, top-level section, nested section parts, `__proto__[]` arrays — `IniTest.ParseProtoProtection`), keys containing `=`/leading-`[`/surrounding whitespace round-tripped via `safe`/`unsafe` (`IniTest.SafeJsonStringifies*` and the foo-fixture round-trip), and lone-surrogate WTF-8 inputs in JSON-quoted values handled by polycpp's WTF-8-aware `JSON::parse` (`IniTest.UnsafeLoneSurrogatePreservesAsWtf8` and `IniTest.UnsafeLoneLowSurrogatePreservesAsWtf8`).
  - prototype-pollution attempts (top-level key, top-level section, nested
    section parts, `__proto__[]` arrays). All exercised by
    `IniTest.ParseProtoProtection`.
  - keys containing `=`, leading `[`, or surrounding whitespace must be
    safely round-tripped via `safe`/`unsafe`. Exercised by
    `IniTest.SafeJsonStringifies*` and the foo-fixture round-trip.
  - lone-surrogate WTF-8 inputs in JSON-quoted values must not crash and
    must round-trip through polycpp's WTF-8-aware `JSON::parse`. Exercised
    by `IniTest.UnsafeLoneSurrogatePreservesAsWtf8` and
    `IniTest.UnsafeLoneLowSurrogatePreservesAsWtf8`.

## Core use cases

- read existing INI files (npm `.npmrc`-like, application config files) into a
  typed C++ document
- programmatically modify a parsed document and serialize it back, preserving
  comments-stripped content with stable key ordering
- emit nested-section files with optional alignment, sorting, and Windows
  line endings

## Key features to port first

- `parse`/`decode` of INI text into `IniDocument`
- `stringify`/`encode` of `IniDocument` into INI text with `section`,
  `align`, `newline`, `sort`, `whitespace`, `platform`, `bracketedArray`
  options
- `safe`/`unsafe` round-trip helpers
- `__proto__` filter and dotted-section merge

## Features to defer

- None. v0 implements the full upstream public surface (`parse`/`decode`,
  `stringify`/`encode`, `safe`/`unsafe`, plus all encode/decode options). The
  domain or standard extensions that upstream does not implement are recorded
  separately under `## Non-parity extension candidates` below.

## Non-parity extension candidates

These are domain or standard features that upstream does not implement and
that the C++ port has NOT accepted as future work. They are recorded so a
future maintainer can revisit them, not promised:

- `polycpp::JsonValue → IniValue` (`fromJSON`) adapter — the inverse of
  `IniValue::toJSON()`. Lossy by nature: JSON numbers must collapse into INI
  strings (since INI has no number type). Worth adding once a concrete
  consumer asks. Outbound `IniValue::toJSON()` and
  `polycpp::JSON::stringify(IniValue)` are now implemented (added in `0.2.0`);
  see `docs/api-mapping.md`.
- streaming/chunked parse and serialize — would let consumers handle very
  large INI files without buffering the entire text. Upstream `ini` reads the
  whole string at once, so this is a domain extension, not a parity defer.
- typed numeric value accessor (`IniValue::isNumber`/`asNumber`) — INI does
  not have a number type; upstream returns numeric-looking values as
  strings. A typed numeric accessor that parses on demand would be a domain
  extension.
- INI variant dialects (e.g. `[section.key]` shorthand, `key = "long\nvalue"`
  multi-line escapes outside JSON-style quoting). Out of scope for v0 and
  out of scope for upstream as well.

## v0 scope

- port version: 0.2.0
- versioning note: port version is independent from upstream npm versioning;
  the README declares "Initial port based on upstream version: 6.0.0".
- supported APIs: `polycpp::ini::parse`, `decode`, `stringify`, `encode`, `safe`, `unsafe`, plus the `IniValue`/`IniDocument`/`EncodeOptions`/`DecodeOptions` types, the `find/hasKey/set/keys/remove` helpers, and (since `0.2.0`) `IniValue::toJSON()` plus the `polycpp::JSON::stringify(IniValue)` template overload it enables.
  - `polycpp::ini::parse(const std::string&, const DecodeOptions&)` — alias for `decode`
  - `polycpp::ini::decode(const std::string&, const DecodeOptions&)`
  - `polycpp::ini::stringify(const IniDocument&, const EncodeOptions&)` — alias for `encode`
  - `polycpp::ini::encode(const IniDocument&, const EncodeOptions&)`
  - `polycpp::ini::safe(const std::string&)`
  - `polycpp::ini::unsafe(const std::string&)`
  - `polycpp::ini::find/hasKey/set/keys/remove` (helpers over `IniDocument`)
  - `polycpp::ini::IniValue`/`IniDocument`/`EncodeOptions`/`DecodeOptions`
- unsupported APIs: ambient `process.platform` default for `EncodeOptions::platform` (caller must set `platform = "win32"` explicitly for `\r\n`), and the JS-shaped two-arg `encode(obj, "section")` shorthand (use `EncodeOptions{ .section = "..." }`).
  - `EncodeOptions::platform` defaulting to `process.platform`. Caller must
    set `platform = "win32"` explicitly to get `\r\n`. Default is `\n`.
  - JS-shaped `encode(obj, "section")` two-arg form — pass `EncodeOptions{
    .section = "..." }` instead.
- dependency plan: link to base `polycpp` only (for `JSON::parse`/`stringify`
  and `JsonValue`). No external libraries.
- polycpp modules to use: `polycpp::core` (for `JSON` and `JsonValue`).
- missing polycpp primitives: none; everything required by the v0 scope is
  already public in base `polycpp`.
