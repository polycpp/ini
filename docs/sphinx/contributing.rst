Contributing
============

Contributions are welcome — this is a small library and most issues
turn around in a few days.

Where to file issues
--------------------

Use the GitHub issue tracker:

- https://github.com/polycpp/ini/issues

When reporting a parse or stringify bug, please include:

- The exact input string (or a minimised reproduction).
- The expected output and the actual output.
- Compiler and version (``clang --version`` / ``g++ --version``).
- The operating system and architecture.

Pull requests
-------------

Pull requests go to the same repository:

- https://github.com/polycpp/ini/pulls

Before submitting, please:

- Add a GoogleTest case that pins the new behaviour.
- Run the full test suite locally (see below).
- Match the existing code style — the headers under
  ``include/polycpp/ini/`` are the canonical reference.

Building from source
--------------------

The :doc:`getting-started/installation` page covers the standard
``FetchContent`` integration. To build the library and its tests
directly:

.. code-block:: bash

   git clone https://github.com/polycpp/ini.git
   cd ini
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ctest --test-dir build --output-on-failure

To build the documentation locally:

.. code-block:: bash

   pip install -r docs/requirements.txt
   python3 docs/build.py
   # output lands in docs/build/html/index.html

The docs build runs Doxygen, then Sphinx with ``-W --keep-going`` so
any new warning fails the build.
