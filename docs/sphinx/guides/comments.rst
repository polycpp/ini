Comments and the ``;`` / ``#`` markers
======================================

**When to reach for this:** you want to know which characters start a
comment, whether comments survive parse → stringify, and how to
include a literal ``;`` or ``#`` in a value.

Recognised comment markers
--------------------------

Both ``;`` and ``#`` start a comment. They are recognised in two
positions:

1. At the beginning of a line (after optional whitespace) — the entire
   line is skipped.
2. After an unquoted value — the rest of the line is treated as an
   inline comment and stripped.

.. code-block:: ini

   ; full-line comment
   # also a full-line comment

   key=value         ; inline comment, stripped
   greeting=hello    # also stripped

After parsing, both ``key`` and ``greeting`` hold their values without
the trailing comment text.

Round-trip behaviour
--------------------

Comments are **not** preserved through round-trip. :cpp:func:`parse`
discards them, and :cpp:func:`stringify` has no concept of a comment
to re-emit:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   auto doc = parse("; banner\nname=polycpp\n");
   std::cout << stringify(doc);
   // name=polycpp

If you want to preserve commentary across an edit, store it as
documentation alongside the file (in source control) rather than
relying on the parser.

Escaping ``;`` and ``#`` inside values
--------------------------------------

To include a literal ``;`` or ``#`` in an unquoted value, escape it
with a backslash:

.. code-block:: cpp

   auto doc = parse("path=C:\\;foo\n");
   find(doc, "path")->asString();   // "C:;foo"

   IniDocument out;
   set(out, "path", IniValue("C:;foo"));
   std::cout << stringify(out);
   // path=C:\;foo

The encoder applies the escape automatically — :cpp:func:`safe` turns
a ``;`` into ``\;`` and a ``#`` into ``\#``:

.. code-block:: cpp

   IniDocument out;
   set(out, "msg", IniValue("hello; world"));
   std::cout << stringify(out);
   // msg=hello\; world

If the value also contains characters that force quoting (``=``,
``\r``, ``\n``, leading ``[``, leading/trailing whitespace, or it
already looks quoted), :cpp:func:`safe` JSON-stringifies the whole
value and the escaping rules above no longer apply — the embedded
``;`` and ``#`` ride along inside the quoted string.

Inline comments after a value are stripped at parse time, and any
trailing whitespace between the value and the comment marker is
trimmed — so ``key=val   # note`` round-trips as ``key=val`` (no
trailing whitespace).
