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
.. doxygenfunction:: polycpp::ini::keys
.. doxygenfunction:: polycpp::ini::remove
