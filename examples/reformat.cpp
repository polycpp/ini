// Read an INI document on stdin and write it back with keys aligned,
// values spaced, and sections separated by a blank line. Purely
// cosmetic — values and their order are unchanged.
//
//   $ cat foo.ini | ./reformat > foo.pretty.ini

#include <iostream>
#include <sstream>

#include <polycpp/ini.hpp>

using namespace polycpp::ini;

int main() {
    std::ostringstream buf;
    buf << std::cin.rdbuf();

    IniDocument doc = parse(buf.str());

    EncodeOptions opts;
    opts.align      = true;
    opts.whitespace = true;
    opts.newline    = true;
    std::cout << stringify(doc, opts);
    return 0;
}
