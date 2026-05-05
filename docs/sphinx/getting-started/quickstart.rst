Quickstart
==========

This page walks through a minimal ini program end-to-end. Copy the
snippet, run it, then jump to :doc:`../tutorials/index` for task-oriented
walkthroughs or :doc:`../api/index` for the full reference.

The program below reads a literal INI document, pokes at a few
values, adds one more, and writes the result back out with
key-alignment enabled so the output is easier for a human to read.
It exercises parsing, the :cpp:class:`polycpp::ini::IniValue` variant,
the :cpp:func:`polycpp::ini::set` mutator, and the
:cpp:struct:`polycpp::ini::EncodeOptions` struct.

Full example
------------

.. code-block:: cpp

   #include <iostream>
   #include <polycpp/ini.hpp>

   using namespace polycpp::ini;

   int main() {
       const std::string src =
           "name=polycpp\n"
           "[server]\n"
           "host=0.0.0.0\n"
           "port=8080\n"
           "ssl=true\n";

       IniDocument doc = parse(src);

       // Read — every value is a typed variant.
       std::cout << "name: " << find(doc, "name")->asString() << '\n';

       IniDocument& server = find(doc, "server")->asDocument();
       std::cout << "host: " << find(server, "host")->asString() << '\n';
       std::cout << "port: " << find(server, "port")->asString() << '\n';
       std::cout << "ssl : " << (find(server, "ssl")->asBool() ? "yes" : "no")
                 << '\n';

       // Mutate — set() replaces-in-place if the key exists, appends otherwise.
       set(server, "workers", IniValue("4"));

       // Write — align=true puts keys on a column grid; sort=false (default)
       // preserves the original insertion order, which is critical for
       // round-trip fidelity.
       EncodeOptions opts;
       opts.align      = true;
       opts.whitespace = true;

       std::cout << "---\n" << stringify(doc, opts);
       return 0;
   }

Compile it with the same CMake wiring from :doc:`installation`:

.. code-block:: bash

   cmake -B build -G Ninja
   cmake --build build
   ./build/my_app

Expected output:

.. code-block:: text

   name: polycpp
   host: 0.0.0.0
   port: 8080
   ssl : yes
   ---
   name = polycpp
   [server]
   host    = 0.0.0.0
   port    = 8080
   ssl     = true
   workers = 4

The first block shows the program's own readback (``"yes"`` / ``"no"``
strings come from the ``? :`` in user code), while the second block is
``stringify`` output where the typed boolean is serialised back to the
canonical ``true`` token.

What just happened
------------------

:cpp:func:`polycpp::ini::parse` returns an
:cpp:type:`polycpp::ini::IniDocument`, which is a
``std::vector<std::pair<std::string, IniValue>>`` —
**insertion-order preserving**, not a map. That's deliberate:
the original ordering of an INI file carries intent (logically
grouped keys, commentary flow), and preserving it is what makes a
parse → mutate → stringify round-trip non-destructive.

Each value in the document is an :cpp:class:`polycpp::ini::IniValue`,
a type-tagged variant with five alternatives — null, bool, string,
array, and nested document. The ``isBool`` / ``asBool`` /
``isString`` / ``asString`` predicate-and-accessor pairs let you
discriminate safely; the ``as*()`` accessors throw on mismatch, so
the typical pattern is ``if (v->isBool()) { v->asBool() }``.

``parse`` recognises ``true`` / ``false`` / ``null`` as typed values
(matching the npm ``ini`` package), so the ``ssl=true`` line above
surfaces as a bool, not a string. For values that happen to look like
booleans but should be kept as strings, wrap them in single quotes:
``mode='true'``.

.. note::

   INI has no native numeric type. Numeric-looking values like
   ``port=8080`` round-trip as the **string** ``"8080"``, accessed via
   :cpp:func:`IniValue::asString()`. Convert at the consumer with
   ``std::stoi`` (wrap in ``try`` / ``catch``) or ``std::from_chars``
   for a safe parse. The only tokens recognised as typed (non-string)
   scalars are ``true``, ``false``, and ``null``.

:cpp:func:`polycpp::ini::set` does the obvious thing: if the key
already exists, it replaces the value in place; otherwise it appends
a new pair at the end.

Finally, :cpp:func:`polycpp::ini::stringify` takes an
:cpp:struct:`polycpp::ini::EncodeOptions` that control formatting
without affecting semantics: ``align`` pads the separator into a
column, ``whitespace`` uses ``key = value`` with spaces instead of
``key=value``, ``sort`` alphabetises keys within each section,
``newline`` inserts a blank line after each section header, and
``platform = "win32"`` emits CRLF line endings.

Next steps
----------

- :doc:`../tutorials/index` — step-by-step walkthroughs of common tasks.
- :doc:`../guides/index` — short how-tos for specific problems.
- :doc:`../api/index` — every public type, function, and option.
- :doc:`../examples/index` — runnable programs you can drop into a sandbox.
