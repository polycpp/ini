Parse and stringify
===================

Entry points for turning INI text into an
:cpp:type:`polycpp::ini::IniDocument` and back. Each call is
stateless; the options structs carry every knob that affects output
formatting or input interpretation.

``parse`` / ``stringify`` are the canonical names; ``decode`` /
``encode`` are aliases preserved for parity with the npm ``ini``
package.

Functions
---------

.. doxygenfunction:: polycpp::ini::parse
.. doxygenfunction:: polycpp::ini::decode
.. doxygenfunction:: polycpp::ini::stringify
.. doxygenfunction:: polycpp::ini::encode

Examples
--------

A round-trip showing the canonical entry points:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniDocument doc = parse(
       "[server]\n"
       "host=0.0.0.0\n"
       "port=8080\n"
       "ssl=true\n");

   std::string text = stringify(doc);
   // text == "[server]\nhost=0.0.0.0\nport=8080\nssl=true\n"

The ``decode`` / ``encode`` aliases are interchangeable with
``parse`` / ``stringify``:

.. code-block:: cpp

   IniDocument d = decode("a=1\n");
   std::string  s = encode(d);              // "a=1\n"

Errors
------

``parse()`` / ``decode()`` are tolerant by design — they mirror the
``npm ini`` parser, which silently drops malformed lines instead of
throwing.

- Lines that do not match the section-header or ``key=value`` regex
  are skipped (e.g. ``=just junk!``).
- Blank lines and lines starting with ``;`` or ``#`` are treated as
  comments and ignored.
- Sections or keys named ``__proto__`` are filtered out for
  prototype-pollution parity with the JS upstream.
- Bad UTF-8 inside JSON-quoted values is surfaced through the
  underlying ``polycpp::JSON`` parse, which preserves lone surrogates
  as WTF-8.

The result is always a valid :cpp:type:`IniDocument`; on completely
unparseable input, that document is simply empty. **Neither**
``parse`` **nor** ``decode`` **throws** for malformed INI input.

``stringify()`` / ``encode()`` are likewise total functions for the
documented variants — they never throw on a well-formed
:cpp:type:`IniValue` graph. The only way to get an exception is to
construct a deliberately invalid graph (for example, a self-
referencing :cpp:type:`IniDocument`) such that recursion exhausts
the stack; the library does not attempt to detect cycles.

Type-mismatched accessors on :cpp:class:`IniValue` (calling
``asBool()`` on a string, etc.) throw ``std::bad_variant_access`` —
guard with the matching ``isX()`` predicate before reading. See
:doc:`../guides/error-handling` for patterns.

Value escaping
--------------

.. doxygenfunction:: polycpp::ini::safe
.. doxygenfunction:: polycpp::ini::unsafe

``safe`` is what :cpp:func:`stringify` calls internally on every
value. ``unsafe`` is its inverse — what :cpp:func:`parse` uses on
every right-hand side. They are exposed for the case where you are
hand-building INI text outside of :cpp:func:`stringify`.

.. code-block:: cpp

   safe("hello; world");           // "hello\\; world"
   safe("contains=equals");        // "\"contains=equals\""
   safe("  spaces  ");             // "\"  spaces  \""
   unsafe("hello\\; world");       // "hello; world"
   unsafe("\"  spaces  \"");       // "  spaces  "
   unsafe("foo ; trailing junk");  // "foo"  (inline comment stripped)

Options
-------

.. doxygenstruct:: polycpp::ini::DecodeOptions
   :members:
   :undoc-members:

.. doxygenstruct:: polycpp::ini::EncodeOptions
   :members:
   :undoc-members:

EncodeOptions in action
-----------------------

``section`` — wrap the encoded output in a section header. Useful
when you have a flat document that you want to land under a parent
section in a larger file:

.. code-block:: cpp

   IniDocument doc;
   set(doc, "host", IniValue("0.0.0.0"));
   set(doc, "port", IniValue("8080"));

   // No section: flat output.
   stringify(doc);
   // host=0.0.0.0
   // port=8080

   // With section: a header is emitted and nested keys are scoped.
   EncodeOptions opts;
   opts.section = "server";
   stringify(doc, opts);
   // [server]
   // host=0.0.0.0
   // port=8080

Nested documents combine with ``section`` to produce dotted section
headers (``[server.tls]`` etc.).

``sort`` — alphabetise keys within each section:

.. code-block:: cpp

   IniDocument doc;
   set(doc, "zeta",  IniValue("z"));
   set(doc, "alpha", IniValue("a"));
   EncodeOptions opts;
   opts.sort = true;
   stringify(doc, opts);
   // alpha=a
   // zeta=z

``align`` — pad keys to a column grid (implies ``whitespace``):

.. code-block:: cpp

   IniDocument doc;
   set(doc, "host",    IniValue("0.0.0.0"));
   set(doc, "workers", IniValue("4"));
   EncodeOptions opts;
   opts.align = true;
   stringify(doc, opts);
   // host    = 0.0.0.0
   // workers = 4
