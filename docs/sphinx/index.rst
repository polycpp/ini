ini
===

**INI config parse and serialize**

**Status:** port version 1.0.0 — based on npm ``ini`` 6.0.0.

Order-preserving, round-trip-safe INI parser and serialiser. Port of the npm ``ini`` package, with the same bracketed-array semantics, the same quoting rules, and a typed ``IniValue`` variant for nested sections, arrays, booleans, and strings.

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   // Parse — every value is an IniValue variant.
   auto doc = parse("[server]\nhost=0.0.0.0\nport=8080\nssl=true\n");
   if (auto* server = find(doc, "server"); server && server->isDocument()) {
       const auto& s = server->asDocument();
       if (auto* host = find(s, "host"); host && host->isString()) {
           // host->asString() == "0.0.0.0"
       }
       if (auto* ssl = find(s, "ssl"); ssl && ssl->isBool()) {
           // ssl->asBool() == true
       }
   }

   // Build and serialise.
   IniDocument out;
   set(out, "name", IniValue("polycpp"));
   std::string text = stringify(out);   // "name=polycpp\n"

.. grid:: 2

   .. grid-item-card:: Drop-in familiarity
      :margin: 1

      Mirrors the npm ``ini`` API — ``parse(s)`` / ``stringify(doc)`` (aliased to ``decode`` / ``encode``), plus ``safe`` and ``unsafe`` helpers for value escaping.

   .. grid-item-card:: C++20 native
      :margin: 1

      Header-only where possible, zero-overhead abstractions, ``constexpr``
      and ``std::string_view`` throughout.

   .. grid-item-card:: Tested
      :margin: 1

      66 GoogleTest cases cover parse, stringify, quoting, bracketed-array syntax, nested sections, encoding options (align / sort / whitespace / platform), and round-trip fidelity.

   .. grid-item-card:: Plays well with polycpp
      :margin: 1

      Uses the same JSON value, error, and typed-event types as the rest of
      the polycpp ecosystem — no impedance mismatch.

Getting started
---------------

.. code-block:: bash

   # With FetchContent (recommended)
   FetchContent_Declare(
       polycpp_ini
       GIT_REPOSITORY https://github.com/polycpp/ini.git
       GIT_TAG        master
   )
   FetchContent_MakeAvailable(polycpp_ini)
   target_link_libraries(my_app PRIVATE polycpp::ini)

:doc:`Installation <getting-started/installation>` · :doc:`Quickstart <getting-started/quickstart>` · :doc:`Tutorials <tutorials/index>` · :doc:`API reference <api/index>`

.. toctree::
   :hidden:
   :caption: Getting started

   getting-started/installation
   getting-started/quickstart

.. toctree::
   :hidden:
   :caption: Tutorials

   tutorials/index

.. toctree::
   :hidden:
   :caption: How-to guides

   guides/index

.. toctree::
   :hidden:
   :caption: API reference

   api/index

.. toctree::
   :hidden:
   :caption: Examples

   examples/index

.. toctree::
   :hidden:
   :caption: About

   about/differences-from-upstream

.. toctree::
   :hidden:
   :caption: Project

   changelog
   contributing
   license
