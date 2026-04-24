Round-trip a config without drift
=================================

**You'll build:** a test fixture that parses an INI document, mutates
a single value, and writes it back — asserting that unrelated keys,
ordering, comments-as-whitespace, and quoting all survive unchanged.
This is the standard test you want around any "update one setting"
CLI feature.

**You'll use:**
:cpp:func:`polycpp::ini::parse`,
:cpp:func:`polycpp::ini::stringify`,
:cpp:func:`polycpp::ini::set`,
:cpp:struct:`polycpp::ini::EncodeOptions`.

**Prerequisites:** a GoogleTest-enabled build (the repo ships with
one).

Step 1 — understand what round-trip does preserve
-------------------------------------------------

:cpp:type:`IniDocument` is a ``std::vector<std::pair<std::string,
IniValue>>``, so:

- **key order** is preserved — both at the top level and inside
  nested sections;
- **value types** are preserved — a bool stays a bool, a string
  stays a string, and quoted strings round-trip as quoted strings;
- **whitespace inside quoted values** is preserved (that is the
  whole point of the quoting rules);
- **encode options are applied on the write side** — ``align``,
  ``whitespace``, ``sort`` are opt-in, so leaving them off gives you
  minimal diffs.

What does **not** round-trip: inline comments (``;`` / ``#``), and
any stylistic differences (``key=value`` vs. ``key = value``) unless
you explicitly set :cpp:any:`EncodeOptions::whitespace`. The library
normalises output to its own spelling.

Step 2 — build the fixture
--------------------------

.. code-block:: cpp

   #include <gtest/gtest.h>
   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   static const char* FIXTURE =
       "name=polycpp\n"
       "[server]\n"
       "host=0.0.0.0\n"
       "port=8080\n"
       "ssl=true\n";

Step 3 — mutate and compare
---------------------------

The round-trip test does three things: parse, mutate one key,
stringify, then assert that the *only* diff is the one key.

.. code-block:: cpp

   TEST(IniRoundTrip, UpdatePortLeavesEverythingElseAlone) {
       IniDocument doc = parse(FIXTURE);

       // Mutate one value deep in the tree.
       auto& server = find(doc, "server")->asDocument();
       set(server, "port", IniValue("9090"));

       // Stringify back without touching encode options — minimal diff.
       std::string out = stringify(doc);
       EXPECT_EQ(out,
           "name=polycpp\n"
           "[server]\n"
           "host=0.0.0.0\n"
           "port=9090\n"
           "ssl=true\n");
   }

Notice that ``name=polycpp`` still appears first, ``[server]`` still
appears before its keys, and ``host`` still appears before ``port``.
The ``set`` call replaced-in-place instead of appending.

Step 4 — corpus-driven round-trip
---------------------------------

For a realistic confidence check, assert that every fixture in a
corpus round-trips to itself (``parse → stringify == input``) when
you *don't* mutate anything.

.. code-block:: cpp

   TEST(IniRoundTrip, CorpusIdempotent) {
       const std::vector<std::string> corpus = {
           "a=b\n",
           "[s]\nx=1\n",
           "[a]\nk=v\n[b]\nk=w\n",
           "arr[]=one\narr[]=two\n",
           "[srv]\nhost=0.0.0.0\nport=8080\n",
       };
       for (const auto& text : corpus) {
           IniDocument doc = parse(text);
           EXPECT_EQ(stringify(doc), text) << "drift for: " << text;
       }
   }

If a case drifts, it is almost always one of three things:

1. An inline comment was present — those are discarded.
2. The input used ``key = value`` with whitespace but the default
   encode uses ``key=value``; fix by passing
   ``{.whitespace = true}``.
3. A special value (``true`` / ``false`` / ``null``) was quoted in
   the input; quote it in the expected output too.

Step 5 — keep CRLF consistent on Windows
----------------------------------------

The decoder accepts both ``\\n`` and ``\\r\\n`` line endings. The
encoder emits ``\\n`` by default and ``\\r\\n`` when
:cpp:any:`EncodeOptions::platform` is ``"win32"``. For a truly
byte-identical round-trip, set the platform to match the input.

What you learned
----------------

- Round-trip works because :cpp:type:`IniDocument` is insertion-order
  preserving; it is not a map.
- :cpp:func:`set` replaces in place, so mutations do not change key
  ordering.
- Default encode options produce minimal diffs — reach for ``align``
  / ``sort`` / ``whitespace`` only when you deliberately want a
  reformat.
- Comments and stylistic whitespace are not preserved; design your
  round-trip assertions to match the library's canonical output, not
  the user's original bytes.
