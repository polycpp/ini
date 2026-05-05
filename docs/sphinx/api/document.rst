IniDocument helpers
===================

:cpp:type:`polycpp::ini::IniDocument` is an alias for
``std::vector<std::pair<std::string, IniValue>>`` — a vector rather
than a map so that insertion order is preserved, which is essential
for lossless round-trip. The helper functions below operate on that
vector directly.

Document type
-------------

.. doxygentypedef:: polycpp::ini::IniDocument

Helpers
-------

.. doxygenfunction:: polycpp::ini::find(IniDocument&, const std::string&)
.. doxygenfunction:: polycpp::ini::find(const IniDocument&, const std::string&)
.. doxygenfunction:: polycpp::ini::hasKey
.. doxygenfunction:: polycpp::ini::set

.. warning::

   Pointers and references obtained from :cpp:func:`find` are
   invalidated when :cpp:func:`set` appends a **new** key, or when
   :cpp:func:`remove` erases one. The document is a
   ``std::vector<std::pair<...>>`` under the hood, so any operation
   that grows the vector or shifts elements may move existing
   storage.

   In-place replacement of an existing key by :cpp:func:`set` does
   **not** invalidate the pointer to that key's value — it assigns
   into the same slot. The dangerous case is appending a new key
   while still holding a pointer or reference into the same
   document. Re-fetch via :cpp:func:`find` after any mutation if you
   are unsure.

.. doxygenfunction:: polycpp::ini::keys
.. doxygenfunction:: polycpp::ini::remove

Examples
--------

``find`` — look up a key by name; returns a (possibly null) pointer:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniDocument doc = parse("name=polycpp\nport=8080\n");

   if (const IniValue* v = find(doc, "name")) {
       std::cout << v->asString() << '\n';   // polycpp
   }
   // find returns nullptr for missing keys:
   assert(find(doc, "missing") == nullptr);

``hasKey`` — boolean presence check, useful inside ``if`` conditions:

.. code-block:: cpp

   if (hasKey(doc, "port")) {
       // ... port is present
   }

``set`` — replace in place if the key exists, append at the end
otherwise:

.. code-block:: cpp

   IniDocument out;
   set(out, "host", IniValue("0.0.0.0"));
   set(out, "port", IniValue("8080"));
   set(out, "host", IniValue("127.0.0.1"));   // replace, not append
   // keys(out) == {"host", "port"}

``keys`` — enumerate keys in insertion order:

.. code-block:: cpp

   for (const auto& k : keys(doc)) {
       std::cout << k << '\n';
   }

``remove`` — erase a key; returns ``true`` if it was present:

.. code-block:: cpp

   bool erased = remove(doc, "port");
   // erased == true; subsequent stringify will not emit a "port=" line.
   bool again = remove(doc, "port");
   // again == false; key was already gone.
