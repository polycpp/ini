IniValue
========

The type-tagged variant used for every value in an
:cpp:type:`polycpp::ini::IniDocument`. Five alternatives — null, bool,
string, array, and nested document — discriminated by the
``isX()`` predicates and accessed by the ``asX()`` getters.

.. doxygenclass:: polycpp::ini::IniValue
   :members:
   :undoc-members:
