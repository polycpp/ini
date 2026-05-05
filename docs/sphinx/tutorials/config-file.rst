Load a config file from disk
============================

**You'll build:** a small app-config loader that reads ``app.ini`` on
startup, falls back to sensible defaults when keys are missing, and
surfaces a typed ``Config`` struct the rest of your program can use.

**You'll use:**
:cpp:func:`polycpp::ini::parse`,
:cpp:func:`polycpp::ini::find`,
:cpp:class:`polycpp::ini::IniValue`,
:cpp:func:`polycpp::ini::hasKey`.

**Prerequisites:** a CMake project that links ``polycpp::ini``. See
:doc:`../getting-started/installation`.

Step 1 — define the target type
-------------------------------

Keep the rest of your program ignorant of INI. The loader's job is
to produce a ``Config`` struct from a string; everything downstream
speaks typed fields.

.. code-block:: cpp

   struct Config {
       std::string host     = "127.0.0.1";
       int         port     = 8080;
       bool        ssl      = false;
       std::string log_level = "info";
   };

Step 2 — read the file
----------------------

``parse`` takes a ``std::string``. Any source works — file, stdin,
embedded resource. The idiomatic file read is a two-liner:

.. code-block:: cpp

   #include <fstream>
   #include <sstream>

   std::string slurp(const std::string& path) {
       std::ifstream f(path);
       std::ostringstream ss;
       ss << f.rdbuf();
       return ss.str();
   }

Step 3 — write a typed-accessor helper
--------------------------------------

Repeatedly writing ``find(doc, key)`` then branching on ``isString``
is noisy. Fold it into a helper that returns an ``std::optional`` so
callers can use the ``value_or`` idiom.

.. code-block:: cpp

   #include <polycpp/ini.hpp>
   #include <optional>

   using namespace polycpp::ini;

   std::optional<std::string> get_str(const IniDocument& d, const std::string& k) {
       const auto* v = find(d, k);
       if (!v || !v->isString()) return std::nullopt;
       return v->asString();
   }

   std::optional<bool> get_bool(const IniDocument& d, const std::string& k) {
       const auto* v = find(d, k);
       if (!v || !v->isBool()) return std::nullopt;
       return v->asBool();
   }

Step 4 — assemble the Config
----------------------------

.. code-block:: cpp

   #include <charconv>

   Config load_config(const std::string& path) {
       Config cfg;
       IniDocument doc = parse(slurp(path));

       const auto* server = find(doc, "server");
       if (server && server->isDocument()) {
           const auto& s = server->asDocument();
           cfg.host = get_str(s, "host").value_or(cfg.host);
           // INI has no native numeric type — parse() surfaces port=8080
           // as the string "8080". Use std::from_chars for a safe parse:
           // a non-numeric line like `port=abc` does not throw, it just
           // leaves cfg.port at its default.
           if (auto port = get_str(s, "port")) {
               int parsed = 0;
               auto [ptr, ec] = std::from_chars(
                   port->data(), port->data() + port->size(), parsed);
               if (ec == std::errc() && ptr == port->data() + port->size()) {
                   cfg.port = parsed;
               }
           }
           cfg.ssl  = get_bool(s, "ssl").value_or(cfg.ssl);
       }
       cfg.log_level = get_str(doc, "log_level").value_or(cfg.log_level);
       return cfg;
   }

The ``server`` section may be absent altogether, so the outer
``if (server && server->isDocument())`` is non-negotiable. Inside it,
every individual key falls back to the ``Config`` default.

``std::from_chars`` keeps the loader total: a malformed line like
``port=abc`` leaves ``cfg.port`` at its default instead of throwing
an uncaught ``std::invalid_argument`` the way ``std::stoi`` would. A
``try`` / ``catch`` around ``std::stoi`` is the equivalent fallback
if you prefer it stylistically.

Step 5 — smoke-test it
----------------------

.. code-block:: cpp

   int main() {
       Config c = load_config("app.ini");
       std::cout << c.host << ':' << c.port
                 << (c.ssl ? " (tls)\n" : " (plain)\n");
   }

With ``app.ini``:

.. code-block:: ini

   log_level=debug

   [server]
   host=0.0.0.0
   port=8443
   ssl=true

The program prints ``0.0.0.0:8443 (tls)``.

What you learned
----------------

- :cpp:func:`parse` accepts any string — slurp your file however you
  like.
- The ``isX`` / ``asX`` accessor pattern is the safe way to consume
  :cpp:class:`IniValue`.
- Wrap the branchy accessors in small ``std::optional``-returning
  helpers; the rest of your loader reads like config schema
  declaration instead of type-switching plumbing.
- Accept that any section and any key may be absent and plan the
  fall-back at the top of the loader.
