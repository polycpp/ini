Differences from upstream
=========================

This C++ port is faithful to npm ``ini`` 6.0.0 in input/output behaviour
— the test suite exercises the upstream fixtures byte-for-byte. The
divergences below are intentional and mostly stem from the move from a
dynamic JS object to a typed C++ variant.

For an exhaustive, line-referenced list see ``docs/divergences.md`` in
the source tree.

Typed ``IniValue`` variant
--------------------------

Upstream returns a JS plain object built from ``Object.create(null)``
where every value is whatever ``JSON.parse`` produced (or a string).
The C++ port returns an :cpp:type:`polycpp::ini::IniDocument`
(``std::vector<std::pair<std::string, IniValue>>``) with each entry
typed as an :cpp:class:`polycpp::ini::IniValue` —
a five-way variant of ``null``, ``bool``, ``std::string``, an array of
``IniValue``, or a nested ``IniDocument``.

The benefits:

- Insertion order is guaranteed, which matters for round-trip
  fidelity.
- ``isX()`` predicates and ``asX()`` accessors give compile-time
  type clarity.
- Type mismatches surface as ``std::bad_variant_access``, not a
  silent ``undefined``.

The cost: callers have to discriminate the variant explicitly. The
:doc:`error-handling guide <../guides/error-handling>` covers the
patterns.

No native numeric type
----------------------

Upstream stores numeric-looking values (``port=8080``) as JS strings,
because INI itself has no number type and upstream uses
``JSON.parse('"123"')`` which yields ``"123"``. The C++ port matches
this: ``find(server, "port")->asString()`` is the right call,
followed by your own ``std::stoi`` / ``std::from_chars``. There is
no ``IniValue::isNumber`` / ``asNumber``.

Only three tokens are recognised as typed (non-string) scalars:
``true``, ``false``, ``null``.

``safe`` and ``unsafe`` are exposed
-----------------------------------

The npm package exports ``safe()`` and ``unsafe()`` as part of its
public surface, and so does the C++ port —
:cpp:func:`polycpp::ini::safe` and :cpp:func:`polycpp::ini::unsafe`.
They are what :cpp:func:`stringify` and :cpp:func:`parse` call
internally on every value, and they are exposed for the case where
you are hand-building INI text outside of the round-trip helpers.

JSON interop via ``toJSON``
---------------------------

The C++ port adds :cpp:func:`polycpp::ini::IniValue::toJSON` (since
1.0.0) which recursively projects an :cpp:class:`IniValue` onto a
``polycpp::JsonValue``. Combined with polycpp's ``HasToJson``
concept, this enables ``polycpp::JSON::stringify(IniValue)``
directly. Numeric-looking values stay as JSON strings — the
mapping does not invent number types where the source had none.

This has no upstream analogue (npm ``ini`` simply hands you a JS
object that ``JSON.stringify`` will serialise natively).

Pinned behavioural divergences
------------------------------

The following behaviours differ from upstream and are pinned by
GoogleTest cases.

``EncodeOptions::platform`` default
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Upstream ``encode()`` reads ``process.platform`` to decide between
``\r\n`` and ``\n``. The C++ port leaves :cpp:any:`EncodeOptions::platform`
empty by default, which always emits LF. Set it explicitly to
``"win32"`` for CRLF. Rationale: a parser library should not
introspect the host OS.

Section header always produces a section
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a section header ``[foo]`` is encountered and ``out["foo"]``
already holds a non-document value (string, bool, array), the C++
port replaces it with a fresh nested document. Upstream JS
preserves the truthy primitive and silently no-ops every
subsequent ``key=val`` row inside that section.

Rationale: a section header is an explicit author intent to
introduce a section; silently dropping the rows that follow it is a
footgun.

Dotted-section merge always produces nested documents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the post-pass merge that turns ``{a:{}, "a.b":{...}}`` into
``{a:{b:{...}}}`` encounters an intermediate path part that already
holds an array, the C++ port replaces the array with a nested
document. Upstream JS reuses the array and sets a hidden named
property on it (since arrays are ``typeof === 'object'`` in JS).

Rationale: dotted section headers should produce navigable nested
sections, not arrays with invisible named properties.

``encode(obj, "section")`` two-argument shorthand
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Upstream accepts a string second argument as
``{ section: <string> }``. The C++ port requires the typed
:cpp:struct:`polycpp::ini::EncodeOptions` form. Rationale: C++ does
not have JS-style dynamic overloads.

Unsupported runtime-specific features
-------------------------------------

- ``Object.prototype`` invariants from upstream's ``test/proto.js``
  are not applicable — ``std::vector<pair>`` has no prototype
  chain. ``__proto__`` keys and sections are still filtered for
  consumer parity.
- ``process.platform`` ambient access — see the platform section
  above.
- ES module ``default``/named export distinction — C++ has no
  module loader; consumers ``#include <polycpp/ini.hpp>`` and call
  free functions in the ``polycpp::ini`` namespace.
