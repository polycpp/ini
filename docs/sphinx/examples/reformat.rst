Reformat an INI file
====================

A ``jq``-style filter for INI: reads stdin, writes a pretty-printed
version on stdout. Use it to apply the ``align`` / ``whitespace`` /
``newline`` knobs to an existing config without changing semantics.

.. literalinclude:: ../../../examples/reformat.cpp
   :language: cpp
   :linenos:

Build and run:

.. code-block:: bash

   cmake -B build -G Ninja -DPOLYCPP_INI_BUILD_EXAMPLES=ON
   cmake --build build --target reformat
   cat foo.ini | ./build/examples/reformat > foo.pretty.ini

Expected output
---------------

Given an input ``foo.ini`` such as:

.. code-block:: ini

   name=polycpp
   [server]
   host=0.0.0.0
   port=8080
   ssl=true

the program prints:

.. code-block:: text

   name = polycpp

   [server]

   host = 0.0.0.0
   port = 8080
   ssl  = true

Keys are aligned within each section, ``=`` is surrounded by spaces,
and a blank line separates each section header from its body. Order
and values are unchanged.
