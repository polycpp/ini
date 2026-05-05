API reference
=============

The complete public surface of ini, generated from the Doxygen
comments in the headers. If a symbol is missing here, either it's internal
(under a ``detail`` namespace) or its header comment needs more love —
please open an issue.

Topics
------

.. toctree::
   :maxdepth: 1

   parse-stringify
   value
   document

Namespace overview
------------------

.. doxygennamespace:: polycpp::ini
   :desc-only:

Thread safety
-------------

:cpp:class:`polycpp::ini::IniValue` and
:cpp:type:`polycpp::ini::IniDocument` are **not thread-safe**.
Concurrent reads of an unmutated graph are fine (a bare
``std::vector`` of ``std::pair`` is safe to read from multiple
threads), but any concurrent mutation — including a single
:cpp:func:`set` or :cpp:func:`remove` — requires external
synchronisation.

:cpp:func:`parse` / :cpp:func:`decode` and :cpp:func:`stringify` /
:cpp:func:`encode` are pure functions and reentrant: calling them
from multiple threads on different inputs (or even on the same
``const`` input) is safe. They allocate locally and do not touch
shared state.
