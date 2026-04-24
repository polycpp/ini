// Read an INI document on stdin, set one key inside a named section,
// and emit the result on stdout. Roughly equivalent to the npm
// ``ini`` CLI `set` mode.
//
//   $ printf '[server]\nport=8080\n' | ./set_value server port 9090
//   [server]
//   port=9090

#include <iostream>
#include <sstream>
#include <string>

#include <polycpp/ini.hpp>

using namespace polycpp::ini;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: set_value <section> <key> <value>\n";
        return 2;
    }
    const std::string section = argv[1];
    const std::string key     = argv[2];
    const std::string value   = argv[3];

    std::ostringstream buf;
    buf << std::cin.rdbuf();

    IniDocument doc = parse(buf.str());

    IniValue* sec = find(doc, section);
    if (!sec || !sec->isDocument()) {
        IniDocument fresh;
        set(fresh, key, IniValue(value));
        set(doc, section, IniValue(std::move(fresh)));
    } else {
        set(sec->asDocument(), key, IniValue(value));
    }

    std::cout << stringify(doc);
    return 0;
}
