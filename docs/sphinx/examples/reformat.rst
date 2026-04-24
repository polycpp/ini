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
