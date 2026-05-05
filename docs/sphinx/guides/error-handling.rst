Error handling
==============

**When to reach for this:** you want to know exactly where the
library can throw, what kinds of malformed input it tolerates
silently, and how to surface a user-friendly error from a parse
failure.

For the error contract on the parse / stringify entry points see
:doc:`../api/parse-stringify`. The summary:

- :cpp:func:`parse` / :cpp:func:`decode` **never throw on malformed
  INI input** — unparseable lines are dropped and the result is
  whatever was parseable.
- :cpp:func:`stringify` / :cpp:func:`encode` are total functions
  for any well-formed :cpp:class:`IniValue` graph.
- :cpp:class:`IniValue` ``asX()`` accessors throw
  ``std::bad_variant_access`` on type mismatch.
- :cpp:func:`remove` returns ``false`` when the key was not present;
  it does not throw.

Defending against type mismatches
---------------------------------

The typical pattern is "predicate, then accessor":

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniDocument doc = parse("ssl=true\n");

   if (const auto* v = find(doc, "ssl"); v && v->isBool()) {
       bool flag = v->asBool();
       // ...
   } else {
       // ssl absent or not a bool — fall back
   }

If you need a single-expression read, catch the variant exception:

.. code-block:: cpp

   try {
       bool flag = find(doc, "ssl")->asBool();
   } catch (const std::bad_variant_access&) {
       // ssl was a string or null
   }

The predicate-first form is preferred — exceptions are reserved for
cases where the type really is unexpected.

Tolerating malformed parse input
--------------------------------

Because :cpp:func:`parse` swallows malformed input, you cannot
detect it by catching an exception. The signal is **what is
missing**:

.. code-block:: cpp

   IniDocument doc = parse(maybe_bad_text);

   if (find(doc, "required_key") == nullptr) {
       // either the input did not contain required_key, or the line
       // that should have set it was malformed.
       throw std::runtime_error("missing required_key");
   }

For stricter parsing, validate the document against a schema after
the call returns — there is no built-in "parse strict" mode.

Reporting "key not found" from ``remove``
-----------------------------------------

:cpp:func:`remove` returns a bool — ``true`` if the key was present
and erased, ``false`` otherwise. Use the return value to surface a
"no such key" message:

.. code-block:: cpp

   if (!remove(doc, target)) {
       std::cerr << "no such key: " << target << '\n';
       return 1;
   }

Consumer-side numeric parsing
-----------------------------

INI has no native numeric type, so converting ``port=8080`` to an
``int`` is the consumer's job. ``std::stoi`` throws on bad input;
``std::from_chars`` reports the error in its return value. Both
patterns are valid:

.. code-block:: cpp

   #include <charconv>

   if (auto* v = find(doc, "port"); v && v->isString()) {
       int port = 0;
       const auto& s = v->asString();
       auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), port);
       if (ec != std::errc() || ptr != s.data() + s.size()) {
           // malformed — fall back, or raise
       }
   }
