#include "glob.h"
#include "glob_parser.h"
#include "glob_compiler.h"
#include "glob_executor.h"

#include <iostream>

int main(int argc, char **argv) {
    std::string str = "(pickle|🥒)";
    auto glob = brex::GlobParser::parseGlob((uint8_t*) str.data(), str.length(), true);
    auto compiled_glob = brex::GlobCompiler::compile(glob);
    auto machine = compiled_glob;
    std::cout << glob->toBSQStandard() << std::endl;
    std::cout << (int) brex::match(machine, "🥒", true) << std::endl;
}