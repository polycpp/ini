Parse and stringify
===================

Entry points for turning INI text into an
:cpp:type:`polycpp::ini::IniDocument` and back. Each call is
stateless; the options structs carry every knob that affects output
formatting or input interpretation.

Functions
---------

.. doxygenfunction:: polycpp::ini::parse
.. doxygenfunction:: polycpp::ini::decode
.. doxygenfunction:: polycpp::ini::stringify
.. doxygenfunction:: polycpp::ini::encode

Value escaping
--------------

.. doxygenfunction:: polycpp::ini::safe
.. doxygenfunction:: polycpp::ini::unsafe

Options
-------

.. doxygenstruct:: polycpp::ini::DecodeOptions
   :members:
   :undoc-members:

.. doxygenstruct:: polycpp::ini::EncodeOptions
   :members:
   :undoc-members:
