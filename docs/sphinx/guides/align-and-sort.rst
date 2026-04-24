Pretty-print with alignment and sorting
=======================================

**When to reach for this:** you're writing a config file that humans
will edit, and you want keys column-aligned and alphabetically
ordered within each section.

All formatting toggles live on
:cpp:struct:`polycpp::ini::EncodeOptions`:

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   using namespace polycpp::ini;

   EncodeOptions opts;
   opts.align      = true;   // pad keys to the widest within a section
   opts.whitespace = true;   // use "key = value" with spaces
   opts.sort       = true;   // alphabetise keys within each section
   opts.newline    = true;   // blank line after each section header

   std::cout << stringify(doc, opts);

The four knobs are independent. Sort without alignment if you just
want deterministic ordering. Align without sort to keep the original
key order but column-aligned for readability. Enable ``newline`` to
visually separate sections — useful in files with a dozen sections.

.. warning::

   ``sort=true`` destroys insertion order. If you care about
   round-trip fidelity (parse → stringify yields the input byte-for-
   byte), leave it off — see :doc:`../tutorials/round-trip`.

Diff tip: if you are writing a config file that other tools will
also edit, pick ``align=false`` and ``sort=false``. Column alignment
is sensitive to the longest key in the section, so adding one long
key re-aligns everything and produces a noisy diff.
