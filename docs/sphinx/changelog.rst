Changelog
=========

1.0.0 — 2026-05-05
------------------

Initial public release. Port of npm ``ini`` 6.0.0 with a typed C++
variant for INI values.

**Public API**

- ``polycpp::ini::parse`` / ``decode`` — parse an INI string into an
  :cpp:type:`polycpp::ini::IniDocument`.
- ``polycpp::ini::stringify`` / ``encode`` — serialise an
  :cpp:type:`polycpp::ini::IniDocument` back to INI text.
- ``polycpp::ini::safe`` / ``unsafe`` — the value-escaping helpers
  used internally by ``stringify`` / ``parse`` and exposed for
  hand-built INI text.
- :cpp:class:`polycpp::ini::IniValue` — typed variant
  (``null`` / ``bool`` / ``string`` / array / nested document) with
  ``isX()`` predicates and ``asX()`` accessors.
- :cpp:type:`polycpp::ini::IniDocument` — alias for
  ``std::vector<std::pair<std::string, IniValue>>``, with
  :cpp:func:`find`, :cpp:func:`hasKey`, :cpp:func:`set`,
  :cpp:func:`keys`, and :cpp:func:`remove` helpers.
- :cpp:struct:`polycpp::ini::EncodeOptions` —
  ``section`` / ``align`` / ``newline`` / ``sort`` / ``whitespace`` /
  ``platform`` / ``bracketedArray`` knobs for the encoder.
- :cpp:struct:`polycpp::ini::DecodeOptions` — ``bracketedArray``
  knob for the decoder.
- :cpp:func:`polycpp::ini::IniValue::toJSON` — projects an
  :cpp:class:`IniValue` graph onto a ``polycpp::JsonValue`` for
  interop with the rest of the polycpp ecosystem. Combined with the
  ``HasToJson`` concept this enables
  ``polycpp::JSON::stringify(IniValue)`` directly.

**Differences from upstream**

See :doc:`about/differences-from-upstream` for the full list. The
notable ones:

- Result type is :cpp:type:`IniDocument` (insertion-ordered vector)
  rather than a JS plain object.
- :cpp:any:`EncodeOptions::platform` defaults to LF — the library
  does not introspect ``process.platform``.
- Section headers always produce navigable nested documents, even
  when a same-named scalar already exists at that path.
- Numeric-looking values stay as strings; consumers parse them on
  demand.
