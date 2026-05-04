# Dependency and JavaScript API Analysis

- package: ini
- package version: 6.0.0
- package root: `/data/work/lib/ini/.tmp/upstream/ini` (pinned to tag `v6.0.0`, commit `180a8d5c72d7f13ed70c53619132f3a8ee5ac6ed`)
- analyzer json: `.tmp/dependency-analysis.json`
- published npm artifact path: `/data/work/lib/ini/.tmp/npm-package`
- published npm artifact analyzed: yes; `lib/ini.js` is byte-identical between
  the Git source at `v6.0.0` and the published `ini@6.0.0` tarball
  (`diff -u` produces no output)
- include dev dependencies: no
- dependency source install used: not applicable; upstream `ini` declares no
  runtime `dependencies`, so the analyzer's npm install step had nothing to
  fetch
- companion root checked: `/data/work/lib`

## Package entry metadata

- main: `lib/ini.js`
- module: not declared
- types: not declared (no `*.d.ts` shipped with the npm package; no
  `@types/ini` package is published either)
- exports: not declared
- bin: not declared
- missing declared entries in repo clone: none. The analyzer reports
  `entryPoints: []` because its heuristic looks for `module`/`exports`/`types`
  hints; the only declared entry is the legacy `main: "lib/ini.js"`, which
  exists in both the Git tree and the npm tarball.
- TypeScript source files detected: 0
- TypeScript declarations reviewed: not applicable; upstream ships none and no
  community declarations are required.
- declaration-source decision: README contract is the canonical public API;
  `lib/ini.js` exports object is the secondary contract.
- source-vs-published artifact decision: source tree at tag `v6.0.0` is the
  runtime artifact (byte-identical to the published tarball).

## Direct dependencies

- none. Upstream `package.json` `dependencies` is absent. All dev-only
  packages (`tap`, `@npmcli/eslint-config`, `@npmcli/template-oss`) are out of
  shipping scope.

## Dependency ownership decisions

There are no runtime dependencies to allocate. License-strategy table is
filled with the single row representing the upstream package itself for
auditability; analyzer-only license states are not applicable.

| Package | Kind | Requested | Installed | License | License evidence | License impact | License strategy | Affects repo license | Deps | Source files | Node API calls | JS API calls | Recommendation | Rationale |
|---|---|---|---|---|---|---|---|---|---:|---:|---:|---:|---|---|
| ini | upstream package itself | n/a (port target) | tag `v6.0.0` | ISC | manual SPDX check: upstream `LICENSE` file holds ISC text by Isaac Z. Schlueter; `package.json` `license: "ISC"` field | permissive | permissive dependency ok with notice | yes-if-shipped | 0 | 1 | 0 | 1 (`JSON.parse`, `JSON.stringify`) | direct port (no third-party C++ dep needed) | upstream has no runtime deps and is small enough to reimplement in C++ on top of `polycpp::JSON` |

## License impact summary

- upstream package license: ISC (permissive, OSI-approved)
- repo license decision: MIT (existing). MIT and ISC are both permissive and
  mutually compatible; the C++ port carries the upstream ISC notice in
  `THIRD_PARTY_LICENSES.md` while shipping under MIT.
- GPL/AGPL dependencies: none.
- LGPL/MPL dependencies: none.
- permissive dependencies requiring notices: ISC (for the upstream `ini`
  package) — already recorded in `THIRD_PARTY_LICENSES.md`.
- dev/test-only dependencies excluded from shipped artifacts: gtest is fetched
  at test time via `FetchContent` and is not linked into `polycpp_ini`.
- dependency license notices to add to `THIRD_PARTY_LICENSES.md`: ISC notice
  for `ini` (already present and verified).

## Transitive dependency summary

- transitive count: 0 (upstream `package.json` has no `dependencies`).
- transitive risks: none.

## Runtime API usage

### Target package

- entry points analyzed: `lib/ini.js` (manual; analyzer auto-detection
  returned 0 entry points — see `## Analyzer warnings`)
- source files analyzed by analyzer: 0 (analyzer warning, see below)
- source files manually inspected: `lib/ini.js` (every function), `test/*.js`
  and `test/fixtures/*.ini` and `tap-snapshots/test/*.cjs`
- external imports seen from target: none. `lib/ini.js` opens with
  `const { hasOwnProperty } = Object.prototype` and never `require`s or
  `import`s another module.

### Analyzer porting gates

- polycpp reuse hints consumed: yes; analyzer emitted no hints, consistent
  with manual inspection — the only base-polycpp surface required is
  `polycpp::JSON::parse` and `polycpp::JSON::stringify`, both already used by
  the existing implementation.
- Node parity hints consumed: yes; analyzer emitted no parity hints.
  Manually verified the only Node-runtime touch point is `process.platform`
  inside `encode`; recorded as a deliberate C++ API change in
  `docs/divergences.md`.
- security hints consumed: yes; `securityHints.securitySensitive: false`,
  `cryptoApis: []`, `requiredReviews: []`. Manual review confirmed no
  network, crypto, or filesystem surface; the only security-adjacent concern
  is `__proto__` filtering, treated as a parser correctness contract.
- security-sensitive package: no. Recorded.
- polycpp capability snapshot consumed: yes (HEAD `c0f97702`, 2026-05-04).
- transport/listener capability hints consumed: not applicable; INI parser
  has no transport surface.

### Node.js API usage

- only direct Node API touch point in upstream: `process.platform` inside
  `encode()` (feature-tested with `typeof process !== 'undefined'`). The C++
  port omits this default (caller must set `EncodeOptions::platform`).

### Node parity surface usage

- callbacks: none.
- Promise APIs: none.
- EventEmitter APIs: none.
- server/listener APIs: none.
- diagnostic/tracing APIs: none.
- streams: none.
- Buffer and binary data: none. Upstream operates on `string` only.
- URL/timer/process/filesystem APIs: only `process.platform` (encode default).
- crypto/compression/TLS/network/HTTP APIs: none.

### JavaScript API usage

- `Object.create(null)` — used to build the result document with no
  prototype, mitigating `__proto__` injection. The C++ port does not need
  this (its `IniDocument` is `std::vector<std::pair<...>>` and has no
  prototype chain) but still skips `__proto__` keys for behavior parity.
- `Object.prototype.hasOwnProperty` — destructured at top of file and used
  for prototype-safe key existence checks. The C++ port performs the
  equivalent check via `find()` over `vector<pair>`.
- `Object.keys(obj)`, `Array.isArray(val)`, `Array.prototype.sort`,
  `Array.prototype.reduce`, `Array.prototype.filter`, `Array.prototype.map`,
  `Array.prototype.concat`, `Array.prototype.push` — used in `encode()` for
  alignment computation and section iteration. The C++ port maps each to
  `std::vector` operations and a `keys()` helper.
- `String.prototype.match`, `String.prototype.split`, `String.prototype.padEnd`,
  `String.prototype.startsWith`/`endsWith`, `String.prototype.slice`,
  `String.prototype.indexOf`, `String.prototype.charAt`,
  `String.prototype.trim`, `String.prototype.replace` — used throughout
  `decode`/`encode`/`safe`/`unsafe`. The C++ port uses `std::string` /
  `std::regex` / hand-written equivalents, including a manual line splitter
  that handles `\r` and `\n` and a manual padding loop.
- `JSON.parse`, `JSON.stringify` — used inside `unsafe` and `safe`. The C++
  port uses `polycpp::JSON::parse` and `polycpp::JSON::stringify` directly,
  preserving UTF-8 + WTF-8 surrogate handling that a hand-written escape
  routine would not match.
- `RegExp` literal `/^\[([^\]]*)\]\s*$|^([^=]+)(=(.*))?$/i`, `/[\r\n]+/g`,
  `/^\s*[;#]/`, `/^\s*$/`, `/[=\r\n]/`, `/^\[/`, `/\\\./g` — used in
  `decode`/`encode`/`safe`. The C++ port uses `std::regex` for the master
  decode regex and hand-written byte loops for the simpler patterns.

### Framework object boundary usage

- analyzer-reported target-package framework object accesses: 0
- analyzer-reported dependency framework object accesses: 0
- manual review decision: no framework object boundary. `ini` reads no
  `req`/`res`/`ctx` shapes; the public input is `string` and the output is a
  plain JS object. The C++ port's public input/output is `std::string` and
  `IniDocument`; no framework adapter is required.

## Porting decisions

- replicate the upstream parser/serializer in C++ from the documented
  algorithm in `lib/ini.js`. No third-party C/C++ INI library is wrapped.
- use `std::regex` for the master decode regex, matching the upstream pattern
  exactly (`^\[([^\]]*)\]\s*$|^([^=]+)(=(.*))?$`).
- use `polycpp::JSON::parse` / `polycpp::JSON::stringify` to preserve
  upstream's reliance on `JSON.parse` / `JSON.stringify` for value
  quoting/unquoting; this guarantees UTF-16 surrogate-pair / WTF-8 handling
  that hand-written code would not match.
- preserve insertion order with `std::vector<std::pair<std::string, IniValue>>`
  rather than `std::map` (alphabetical) or `std::unordered_map` (no order).
- expose six free functions in `polycpp::ini` matching upstream
  (`parse`/`decode`/`stringify`/`encode`/`safe`/`unsafe`).
- expose minimal helpers (`find`/`hasKey`/`set`/`keys`/`remove`) so callers
  can manipulate the order-preserving container without touching its
  underlying `vector<pair>` shape.

These decisions are consistent with the ecosystem reuse decisions recorded in
`docs/research.md` under `## Polycpp ecosystem reuse analysis`.

## Analyzer warnings

- `ini: no entry points found for ini` — emitted because the analyzer's
  entry-point heuristic looks for `module`/`exports`/`types` hints in
  `package.json`. Upstream `ini` declares only the legacy `main: "lib/ini.js"`
  field. Manually inspected: `lib/ini.js` exists in both the Git tree and the
  published tarball, exports `parse`, `decode`, `stringify`, `encode`,
  `safe`, `unsafe` via `module.exports`, and is the sole runtime source.
  No further analyzer iteration is needed.
