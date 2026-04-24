Load a config file
==================

Reads an INI file from the path given on the command line, surfaces
a typed summary of a few known keys under ``[server]``, and prints
a pretty-printed version of the whole document.

.. literalinclude:: ../../../examples/load_config.cpp
   :language: cpp
   :linenos:

Build and run:

.. code-block:: bash

   cmake -B build -G Ninja -DPOLYCPP_INI_BUILD_EXAMPLES=ON
   cmake --build build --target load_config
   ./build/examples/load_config /etc/app.ini
