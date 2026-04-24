// Load an INI file from disk, print a typed summary of a few known
// keys, then dump the file in a pretty-printed form.
//
//   $ echo '[server]
//   > host=0.0.0.0
//   > port=8443
//   > ssl=true' | ./load_config /dev/stdin

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <polycpp/ini.hpp>

using namespace polycpp::ini;

static std::string slurp(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: load_config <path>\n";
        return 2;
    }

    IniDocument doc = parse(slurp(argv[1]));

    // Typed summary.
    if (auto* srv = find(doc, "server"); srv && srv->isDocument()) {
        const auto& s = srv->asDocument();
        if (auto* h = find(s, "host"))
            std::cout << "host: " << h->asString() << '\n';
        if (auto* p = find(s, "port"))
            std::cout << "port: " << p->asString() << '\n';
        if (auto* ssl = find(s, "ssl"); ssl && ssl->isBool())
            std::cout << "ssl : " << (ssl->asBool() ? "yes" : "no") << '\n';
    }

    // Pretty-printed dump.
    EncodeOptions opts;
    opts.align      = true;
    opts.whitespace = true;
    opts.newline    = true;
    std::cout << "---\n" << stringify(doc, opts);
    return 0;
}
