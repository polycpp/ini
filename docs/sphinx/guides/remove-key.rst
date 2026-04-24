Remove a key
============

**When to reach for this:** a user unset a configuration field and
you want the corresponding INI line gone from the file — not left
behind as ``key=``.

Use :cpp:func:`polycpp::ini::remove`:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniDocument doc = parse("a=1\nb=2\nc=3\n");
   bool erased = remove(doc, "b");
   // erased == true
   std::cout << stringify(doc);
   // a=1
   // c=3

``remove`` returns ``true`` when the key was present and erased,
``false`` otherwise — handy for reporting "no such key" in a CLI.

For nested sections, find the parent first:

.. code-block:: cpp

   auto* server = find(doc, "server");
   if (server && server->isDocument()) {
       remove(server->asDocument(), "deprecated_flag");
   }

Do **not** write ``set(doc, "key", IniValue(nullptr))`` to delete —
that stores a ``null`` value and the corresponding line will still
be emitted (as ``key=``). :cpp:func:`remove` is the correct tool.
