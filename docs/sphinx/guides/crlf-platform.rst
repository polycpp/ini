Emit CRLF line endings
======================

**When to reach for this:** you're writing a config that will be
consumed by a Windows tool that chokes on bare LF, or you want to
round-trip a CRLF-encoded file without silently converting to LF.

Set :cpp:any:`EncodeOptions::platform` to ``"win32"``:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   EncodeOptions opts;
   opts.platform = "win32";   // any other value (including "") emits \n

   std::string text = stringify(doc, opts);
   // Every line ends in \r\n instead of \n.

The decoder is platform-agnostic — :cpp:func:`parse` accepts both
``\\n`` and ``\\r\\n`` regardless of options — so you only need to
think about this when writing.

For byte-identical round-trip on a mixed-ending corpus, sniff the
first line ending in the input before calling :cpp:func:`stringify`:

.. code-block:: cpp

   bool crlf = text.find("\r\n") != std::string::npos;
   EncodeOptions opts;
   opts.platform = crlf ? "win32" : "";
   auto out = stringify(parse(text), opts);
