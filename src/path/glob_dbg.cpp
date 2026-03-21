#include "glob.h"
#include "glob_parser.h"
#include "glob_compiler.h"
#include "glob_executor.h"

#include <iostream>

int main(int argc, char **argv) {
    std::u8string str = u8"🥒";
    // auto glob = brex::GlobParser::parseGlob((uint8_t*) str.data(), str.length(), true);
    auto glob = brex::GlobParser::parseGlobUnicodeString(str);
    auto compiled_glob = brex::GlobCompiler::compile(glob);
    // auto machine = compiled_glob;
    std::cout << glob->toBSQStandard() << std::endl;
    // std::cout << "_" << std::endl;
    brex::UnicodeString test_str(u8"🥒");
    // brex::CString test_str("cucumber/worm");
    std::cout << (int) (brex::UnicodeGlobExecutor(compiled_glob).match(&test_str)) << std::endl;
}