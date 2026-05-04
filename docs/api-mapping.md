# API Mapping

| Upstream symbol | C++ symbol | Status | Notes |
|---|---|---|---|
| `ini.parse(str, opt?)` | `polycpp::ini::parse(const std::string&, const DecodeOptions&)` | direct | Inline alias delegating to `decode`, matching upstream's `parse: decode` export. |
| `ini.decode(str, opt?)` | `polycpp::ini::decode(const std::string&, const DecodeOptions&)` | direct | Same regex (`^\[([^\]]*)\]\s*$|^([^=]+)(=(.*))?$`), same line splitter on `[\r\n]+`, same `__proto__` filtering, same dotted-section merge with `\.` unescape. |
| `ini.stringify(obj, opt?)` | `polycpp::ini::stringify(const IniDocument&, const EncodeOptions&)` | direct | Inline alias delegating to `encode`. |
| `ini.encode(obj, opt?)` | `polycpp::ini::encode(const IniDocument&, const EncodeOptions&)` | direct | Same alignment, `whitespace`, `newline`, `sort`, `bracketedArray`, `platform`, and `section` semantics. |
| `ini.safe(val)` | `polycpp::ini::safe(const std::string&)` | direct | JSON-stringifies when `val` matches `/[=\r\n]/`, starts with `[`, is quoted, or differs from its trimmed form; otherwise escapes `;` and `#` with backslashes. |
| `ini.unsafe(val)` | `polycpp::ini::unsafe(const std::string&)` | direct | Trims, optionally strips one layer of single quotes, attempts `polycpp::JSON::parse`, otherwise walks the string, applies backslash escapes for `;`/`#`/`\`, and stops at the first unescaped `;`/`#`. |
| `encode(obj, "section")` two-arg JS shorthand | none | omitted | C++ has typed `EncodeOptions{ .section = ... }`; the dynamic two-arg shorthand is a JS overload that does not need a C++ counterpart. Documented as a deliberate behavior change. |
| `process.platform` default for `EncodeOptions::platform` | none | omitted | C++ parser library does not introspect the host OS. Caller must set `platform = "win32"` to opt into `\r\n`. Documented as a deliberate behavior change. |
| `EncodeOptions.bracketedArray` (default `true`) | `EncodeOptions::bracketedArray = true` | direct | Default-true `bool` matches upstream `bracketedArray !== false` semantics — only an explicit `false` disables `[]` suffix. |
| `EncodeOptions.align` | `EncodeOptions::align = false` | direct | Same alignment width computation: `safe(longest filtered key).length`. Setting `align = true` implies `whitespace = true`. |
| `EncodeOptions.newline` | `EncodeOptions::newline = false` | direct | Adds an extra `eol` after each section header. |
| `EncodeOptions.sort` | `EncodeOptions::sort = false` | direct | Lexicographic sort of keys before emission. |
| `EncodeOptions.whitespace` | `EncodeOptions::whitespace = false` | direct | Switches separator from `=` to ` = `. |
| `EncodeOptions.platform` | `EncodeOptions::platform = ""` | adapted | Empty string means LF; `"win32"` means CRLF. Default differs from upstream (no host detection). |
| `EncodeOptions.section` | `EncodeOptions::section = ""` | direct | Section prefix prepended as `[<safe(section)>]<eol>` before non-section keys; nested children recurse with `section + "." + childKey`. |
| `DecodeOptions.bracketedArray` (default `true`) | `DecodeOptions::bracketedArray = true` | direct | Same `key[]` array-syntax recognition; when `false`, duplicate keys promote to arrays on the second occurrence. |
| `__proto__` key filter | filter in `decode()` | direct | Skips top-level `__proto__` keys, top-level `[__proto__]` sections (parsed into a throwaway sink), and intermediate `__proto__` parts inside dotted section names. |
| dotted-section merge `{a:{},"a.b":{x}} → {a:{b:{x}}}` | post-pass in `decode()` | direct | Snapshot-and-merge order matching upstream. |
| `\\.` literal-dot escape inside section names | `splitSections('.', sep)` + leaf/intermediate `\.` → `.` unescape | direct | Same back-quote escape semantics as upstream's `splitSections`. |
| Bare key without `=` interpreted as `true` | bare key produces `IniValue(true)` | direct | Matches upstream `match[3] ? unsafe(match[4]) : true`. |
| Special string values `"true"`/`"false"`/`"null"` parsed to `bool`/`null` | same in C++ port | direct | Matches upstream's `JSON.parse(valueRaw)` branch for these three literals. |
| Comment lines (`^\s*[;#]`) and blank lines | comment/blank-line skip in `decode()` | direct | Same skip rule. |
| Inline comment strip in `unsafe` | inline `;` / `#` stop in `unsafe()` | direct | Backslash before `;`/`#` escapes; bare `;`/`#` stops the value. |
| `tap` snapshot output for `foo.ini` (decode + encode) | `IniTest.FooIniDecodeFromFile`, `IniTest.FooIniEncodeFromData`, `IniTest.EncodeWithAlign`, `IniTest.EncodeWithSort`, `IniTest.EncodeWithAlignAndSort` | direct | Verbatim port of upstream snapshot expectations. |
| `tap` snapshot output for `duplicate.ini` (`bracketedArray` true and false) | `IniTest.DuplicatePropertiesBracketedArrayTrue`, `IniTest.DuplicatePropertiesBracketedArrayFalse`, `IniTest.DuplicatePropertiesEncodeBracketed`, `IniTest.DuplicatePropertiesEncodeUnbracketed` | direct | Verbatim port. |
| `Object.create(null)` proto-safety result | `IniDocument` (`std::vector<std::pair<std::string,IniValue>>`) | adapted | C++ container has no prototype chain; `__proto__` keys are still filtered for behavior parity. |
| Plain JS object as `obj[k]` mutation interface | `polycpp::ini::find/hasKey/set/keys/remove` helpers | adapted | `vector<pair>` lookup is verbose; helpers preserve insertion order and replace-in-place. |
| not upstream | `polycpp::ini::IniValue` (variant) and `polycpp::ini::IniDocument` typedef | adapted | Required because INI's value model differs from `polycpp::JsonValue`. See `docs/research.md` `## Polycpp ecosystem reuse analysis`. |

Status values:

- `direct`: same behavior with an idiomatic C++ spelling.
- `compatibility layer`: same user-facing behavior through a different
  implementation shape.
- `adapted`: preserves the upstream intent with a typed C++ API.
- `deferred`: planned future work for an upstream surface or explicitly
  accepted C++ extension that is intentionally not implemented yet.
- `omitted`: deliberately not part of this port.

## TypeScript Declaration Review

- Declaration source used: not applicable. Upstream `ini@6.0.0` ships no
  `*.d.ts` and no `@types/ini` package is published. The README (and the JS
  source itself) are the canonical contract.
- Public APIs, overloads, options, callbacks, streams, or literal unions
  found only or most clearly in declarations: not applicable; everything was
  read from `lib/ini.js` and the upstream tap snapshots.
- Declaration-only globals, caches, deprecated fields, or runtime-specific
  surfaces mapped as unsupported/not-applicable: not applicable.

## Framework object boundary review

- Upstream reads or mutates framework/request/response/context objects: no.
- Upstream fields or methods read: not applicable. `decode` reads only the
  caller's `string` and option fields; `encode` reads the caller's plain
  object and option fields.
- Upstream fields or methods written: not applicable. `decode` returns a
  fresh `Object.create(null)`; `encode` returns a fresh `string`.
- C++ adapter boundary: none required. Public API takes `std::string` /
  `IniDocument` / typed options structs and returns `IniDocument` / `std::string`.
- Partial mutation risk on validation failure: not applicable; both
  `decode()` and `encode()` build fresh outputs and never mutate caller data.

## Node parity surface review

- Callback APIs: not applicable. Upstream has none.
- Promise APIs: not applicable. Upstream is fully synchronous.
- EventEmitter APIs: not applicable. Upstream emits no events.
- Server/listener APIs: not applicable. INI is a string parser; no listen
  modes (TCP, Unix/IPC, adopted handles), no TLS client/server, no accepted
  connection types.
- Diagnostic/tracing APIs: not applicable. No `node:diagnostics_channel`
  usage, no trace callbacks.
- Stream APIs: not applicable in v0. Upstream operates on whole strings.
  Stream-mode parse/encode is recorded in `docs/research.md` under
  `## Non-parity extension candidates`.
- Buffer and binary APIs: not applicable. Upstream operates on `string`
  only.
- URL, timer, process, and filesystem APIs: only `process.platform` is read
  by upstream `encode()`. The C++ port omits the host-detection default;
  callers explicitly set `EncodeOptions::platform`. No `URL`, `timer`, or
  filesystem surface in either side.
- Crypto, compression, TLS, network, and HTTP APIs: not applicable. No
  client transport, server/listener lifecycle, Unix/IPC path support, or TLS
  client/server modes.
- Unsupported or non-meaningful Node-specific APIs and audit reason: the
  `Object.prototype`-pollution invariants asserted by upstream `test/proto.js`
  (`Object.prototype.foo === undefined`, `Array.prototype[0] === undefined`)
  are not meaningful for `std::vector<std::pair<...>>`; the C++ port still
  filters `__proto__` keys for behavior parity.
