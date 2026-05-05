IniValue
========

The type-tagged variant used for every value in an
:cpp:type:`polycpp::ini::IniDocument`. Five alternatives — null, bool,
string, array, and nested document — discriminated by the
``isX()`` predicates and accessed by the ``asX()`` getters.

.. doxygenclass:: polycpp::ini::IniValue
   :members:
   :undoc-members:

Examples
--------

Constructing each variant kind:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   IniValue nul;                         // null (default ctor)
   IniValue alsoNul(nullptr);            // null

   IniValue flag(true);                  // bool
   IniValue name(std::string("server")); // string (lvalue)
   IniValue host("0.0.0.0");             // string (C string literal)

   IniValue::ArrayType hosts;
   hosts.emplace_back(IniValue("a.local"));
   hosts.emplace_back(IniValue("b.local"));
   IniValue arr(std::move(hosts));       // array

   IniDocument inner;
   set(inner, "port", IniValue("8080"));
   IniValue section(std::move(inner));   // nested document

Discriminating with the ``isX()`` predicates and reading with the
``asX()`` accessors:

.. code-block:: cpp

   IniValue v("hello");
   if (v.isString()) {
       std::cout << v.asString() << '\n';   // hello
   }
   // Calling the wrong accessor throws std::bad_variant_access:
   try {
       v.asBool();
   } catch (const std::bad_variant_access&) {
       // expected — v holds a string, not a bool
   }

   IniValue b(true);
   bool flag = b.asBool();                 // true

   IniValue::ArrayType items;
   items.emplace_back(IniValue("one"));
   items.emplace_back(IniValue("two"));
   IniValue list(std::move(items));
   for (const auto& item : list.asArray()) {
       std::cout << item.asString() << '\n';
   }

   IniDocument inner;
   set(inner, "k", IniValue("v"));
   IniValue node(std::move(inner));
   const auto& d = node.asDocument();
   for (const auto& [key, val] : d) {
       // key == "k", val.asString() == "v"
   }

Round-tripping through JSON via :cpp:func:`toJSON`:

.. code-block:: cpp

   #include <polycpp/core/json.hpp>

   IniDocument doc = parse("[server]\nhost=0.0.0.0\nport=8080\n");
   IniValue wrapped(std::move(doc));

   // Direct conversion:
   polycpp::JsonValue j = wrapped.toJSON();

   // Via polycpp::JSON::stringify (uses HasToJson concept):
   std::string text = polycpp::JSON::stringify(wrapped);
   // text == "{\"server\":{\"host\":\"0.0.0.0\",\"port\":\"8080\"}}"

Numeric-looking values stay as JSON strings — INI has no native
number type, so the JSON projection preserves the source spelling
verbatim.
