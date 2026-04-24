Quote values with special characters
====================================

**When to reach for this:** your value contains ``=``, ``;``, ``#``,
a newline, leading/trailing whitespace, or starts with ``[``. Without
quoting, the parser would cut the value short or misread it as a
section header.

The encoder does the right thing automatically — :cpp:func:`safe`
applies the quoting rules, and :cpp:func:`stringify` calls it for
you. So for the common case you do nothing:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniDocument doc;
   set(doc, "path",    IniValue("/etc/nginx/conf.d/"));
   set(doc, "banner",  IniValue("  hello, world  "));    // leading/trailing space
   set(doc, "format",  IniValue("key=value"));            // contains =
   set(doc, "pattern", IniValue("[warn]"));               // starts with [

   std::cout << stringify(doc);
   // path=/etc/nginx/conf.d/
   // banner="  hello, world  "
   // format="key=value"
   // pattern="[warn]"

Round-trip is symmetric — parsing the output above recovers the exact
original strings. :cpp:func:`unsafe` is the inverse of :cpp:func:`safe`
and is what :cpp:func:`parse` uses internally; it handles JSON-style
double quotes, ``\\r\\n`` escapes, and single-quoted strings.

If you're hand-building INI text (not going through
:cpp:func:`stringify`), call :cpp:func:`safe` on each value yourself:

.. code-block:: cpp

   std::string line = "message=" + safe("hello; world\n");
   // message="hello; world\n"

Comments (``;`` and ``#``) inside an unquoted value are stripped by
the parser. Escape them with a backslash (``val\\; not-a-comment``)
or — cleaner — put the whole value in quotes.
