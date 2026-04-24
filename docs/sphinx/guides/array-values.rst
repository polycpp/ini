Handle repeated keys as arrays
==============================

**When to reach for this:** your config represents a list —
``allowed_host[]=a.local`` appearing multiple times — and you want
each value without hand-walking the key collisions.

The library recognises ``key[]=value`` as array syntax by default
(:cpp:any:`DecodeOptions::bracketedArray` is ``true``). Repeated
``key[]`` lines accumulate into a single
:cpp:any:`IniValue::ArrayType`.

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   auto doc = parse("host[]=a.local\nhost[]=b.local\nhost[]=c.local\n");
   const auto& arr = find(doc, "host")->asArray();
   for (const auto& v : arr) {
       std::cout << v.asString() << '\n';
   }
   // a.local
   // b.local
   // c.local

To produce an array on the **write** side, build an
:cpp:any:`IniValue::ArrayType` and ``set`` it:

.. code-block:: cpp

   IniDocument out;
   IniValue::ArrayType hosts;
   hosts.emplace_back(IniValue("a.local"));
   hosts.emplace_back(IniValue("b.local"));
   set(out, "host", IniValue(std::move(hosts)));

   // Default encode uses the key[] syntax:
   std::string s = stringify(out);
   // host[]=a.local
   // host[]=b.local

If your downstream consumer can't handle ``key[]``, set
:cpp:any:`EncodeOptions::bracketedArray` to ``false`` — the encoder
will emit repeated ``host=a.local`` / ``host=b.local`` lines instead.
On the decode side, pass ``{.bracketedArray = false}`` to
:cpp:func:`parse` to treat ``key[]`` as the literal key
``"key[]"``.
