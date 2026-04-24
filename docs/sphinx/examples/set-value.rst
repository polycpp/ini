Set a value in a section
========================

Reads an INI document on stdin, updates one key inside a named
section (creating the section if needed), and emits the result on
stdout. Mirrors the npm ``ini`` CLI ``set`` mode.

.. literalinclude:: ../../../examples/set_value.cpp
   :language: cpp
   :linenos:

Build and run:

.. code-block:: bash

   cmake -B build -G Ninja -DPOLYCPP_INI_BUILD_EXAMPLES=ON
   cmake --build build --target set_value
   printf '[server]\nport=8080\n' | ./build/examples/set_value server port 9090
