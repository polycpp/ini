Navigate nested sections
========================

**You'll build:** a reader for an INI file with multi-level section
headers (``[a.b.c]``) that walks to the deepest child and enumerates
every value underneath. Useful for systems like ``.gitconfig`` or
``ini``-flavoured configuration for logging frameworks.

**You'll use:**
:cpp:class:`polycpp::ini::IniValue` (the ``isDocument`` /
``asDocument`` predicates), :cpp:func:`polycpp::ini::keys`,
:cpp:func:`polycpp::ini::find`.

**Prerequisites:** you've read :doc:`config-file` and consumed a flat
INI document.

Step 1 — understand dotted section names
----------------------------------------

A section header like ``[log.file.rotate]`` creates the tree
``log → file → rotate``. The same key pattern with a literal dot is
spelled ``[log\.file\.rotate]`` (escape the dot).

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   const std::string src =
       "[log]\n"
       "level=info\n"
       "[log.file]\n"
       "path=/var/log/app\n"
       "[log.file.rotate]\n"
       "max_size=100M\n"
       "max_age=30d\n";

   IniDocument doc = parse(src);

After parsing, ``doc`` contains one top-level key, ``log``, whose
value is an ``IniDocument`` containing ``level`` (string) and
``file`` (another nested document), and so on recursively.

Step 2 — walk the tree
----------------------

A two-liner recursive dumper exposes the shape:

.. code-block:: cpp

   void dump(const IniDocument& d, const std::string& prefix = "") {
       for (const auto& [k, v] : d) {
           const std::string path = prefix.empty() ? k : prefix + "." + k;
           if (v.isDocument())
               dump(v.asDocument(), path);
           else if (v.isString())
               std::cout << path << " = " << v.asString() << '\n';
           else if (v.isBool())
               std::cout << path << " = " << (v.asBool() ? "true" : "false") << '\n';
           else if (v.isNull())
               std::cout << path << " = null\n";
       }
   }

For the fixture above, ``dump(doc)`` prints::

   log.level = info
   log.file.path = /var/log/app
   log.file.rotate.max_size = 100M
   log.file.rotate.max_age = 30d

Step 3 — look up a specific path
--------------------------------

``find`` only descends one level. For a multi-segment path, walk
segment by segment:

.. code-block:: cpp

   const IniValue* lookup(const IniDocument& d,
                          const std::vector<std::string>& path) {
       const IniDocument* cur = &d;
       const IniValue* last = nullptr;
       for (size_t i = 0; i < path.size(); ++i) {
           last = find(*cur, path[i]);
           if (!last) return nullptr;
           if (i + 1 < path.size()) {
               if (!last->isDocument()) return nullptr;
               cur = &last->asDocument();
           }
       }
       return last;
   }

   auto* v = lookup(doc, {"log", "file", "rotate", "max_size"});
   std::cout << v->asString() << '\n';   // 100M

Index-based iteration is safer than comparing the segment value against
``*(path.end() - 1)`` — the latter misbehaves when a path contains
duplicate segments (e.g. ``{"a", "b", "a"}``), since the comparison
matches every occurrence of ``"a"`` and not just the final one.

Step 4 — enumerate top-level keys
---------------------------------

:cpp:func:`keys` returns a ``std::vector<std::string>`` in
insertion order — useful for UI listings or a ``--list-sections``
flag.

.. code-block:: cpp

   for (const auto& k : keys(doc)) {
       std::cout << "top-level section: " << k << '\n';
   }

What you learned
----------------

- ``[a.b.c]`` creates a three-level tree; escape the dot with
  ``\\.`` for a literal key name.
- :cpp:func:`find` returns one level at a time — wrap it in a
  ``lookup`` helper for multi-segment paths.
- The ``isDocument`` branch of :cpp:class:`IniValue` is how you
  recognise a subsection in the dispatch code.
